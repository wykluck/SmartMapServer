#include "ImageFileReader.h"

#include <locale>
#include <codecvt>
#include "ServerSiteConfig.h"
#include "grfmt_gdal.hpp"
#include "opencv2/imgproc/types_c.h"
#include "ImgDecoderFactory.h"
#include <boost/filesystem.hpp>
#include <opencv2/highgui.hpp>
#include <opencv2/imgproc.hpp>

using namespace cvGIS;
using namespace boost::filesystem;
moodycamel::BlockingConcurrentQueue<BlockImageProcessor::BlockImgStruct> ImageFileReader::s_readBlockImgQueue;
ImageFileReader::ImageFileReader()
{
}


ImageFileReader::~ImageFileReader()
{
}

void ImageFileReader::postprocess(cv::Mat& img, const cv::Scalar& colorDiff, const std::pair<int, int>& objSizeRange)
{
	cv::RNG rng = cv::theRNG();
	cv::Mat mask(img.rows + 2, img.cols + 2, CV_8UC1, cv::Scalar::all(0));
	cv::Rect boundingRect;
	for (int y = 0; y < img.rows; y++)
	{
		for (int x = 0; x < img.cols; x++)
		{
			if (mask.at<uchar>(y + 1, x + 1) == 0)
			{
				cv::Scalar ignoredNewVal(0, 0, 0);

				int area = cv::floodFill(img, mask, cv::Point(x, y), ignoredNewVal, &boundingRect, colorDiff, colorDiff, cv::FLOODFILL_MASK_ONLY);
				if (area >= objSizeRange.first && area <= objSizeRange.second && boundingRect.height > 10 && boundingRect.width > 10)
				{
					boundingRect = boundingRect + cv::Point(-1, -1);
					cv::rectangle(img, boundingRect, cv::Scalar(0, 0, 255));
				}
			}
		}
	}

}


std::string ImageFileReader::readForMetaData(const cv::String& datasetFilePath)
{
	auto gdalDecoderPtr = ImgDecoderFactory::Instance()->getDecoder(datasetFilePath);
	auto imageMetaData = gdalDecoderPtr->getMetaData();
	return imageMetaData.ToJsonString();
}

ImageFileReader::ProcessResult ImageFileReader::readForExport(const cv::String& datasetFilePath, const cv::Rect2i& bbox)
{
	//TODO: not implemented.
	auto gdalDecoderPtr = ImgDecoderFactory::Instance()->getDecoder(datasetFilePath);
	std::string cacheDirUtf8 = ServerSiteConfig::get().webConfig.processedCacheDir;

	
	std::unique_ptr<cv::Mat> assembledImagePtr;

	if (gdalDecoderPtr->supportBlockRead())
	{
		//calculate the required blocks that need to process
		int startBlockX = bbox.tl().x / gdalDecoderPtr->GetXBlockSize();
		int startBlockY = bbox.tl().y / gdalDecoderPtr->GetYBlockSize();
		int endBlockX = bbox.br().x % gdalDecoderPtr->GetXBlockSize() == 0 ? (bbox.br().x / gdalDecoderPtr->GetXBlockSize()) : (bbox.br().x / gdalDecoderPtr->GetXBlockSize() + 1);
		int endBlockY = bbox.br().y % gdalDecoderPtr->GetYBlockSize() == 0 ? (bbox.br().y / gdalDecoderPtr->GetYBlockSize()) : (bbox.br().y / gdalDecoderPtr->GetYBlockSize() + 1);

		//wait to reassemble the processed blocks
		int assembleImageRows = (endBlockY - startBlockY + 1) * gdalDecoderPtr->GetYBlockSize();
		int assembleImageCols = (endBlockX - startBlockX + 1) * gdalDecoderPtr->GetXBlockSize();
		cv::Mat readBlockImg;
		for (auto yIndex = startBlockY; yIndex <= endBlockY; yIndex++)
			for (auto xIndex = startBlockX; xIndex <= endBlockX; xIndex++)
			{
				if (gdalDecoderPtr->readBlockData(xIndex, yIndex, readBlockImg))
				{
					if (!assembledImagePtr)
					{
						assembledImagePtr.reset(new cv::Mat(assembleImageRows, assembleImageCols, readBlockImg.type()));
					}
					int xStart = (xIndex - startBlockX) * gdalDecoderPtr->GetXBlockSize();
					int yStart = (yIndex - startBlockY) * gdalDecoderPtr->GetYBlockSize();
					readBlockImg.copyTo((*assembledImagePtr)(cv::Rect(xStart, yStart, readBlockImg.cols, readBlockImg.rows)));
				}
				else
				{
					printf("Error happens when reading block data at (%d, %d)", yIndex, xIndex);
				}
			}

		
		//compress the reassembled image to jpg and send the response back
		ProcessResult processRes(true);
		auto xBlockStart = startBlockX * gdalDecoderPtr->GetXBlockSize();
		auto yBlockStart = startBlockY * gdalDecoderPtr->GetYBlockSize();
		cv::Mat& resMat = assembledImagePtr->rowRange(bbox.tl().y - yBlockStart, bbox.br().y - yBlockStart + 1)
			.colRange(bbox.tl().x - xBlockStart, bbox.br().x - xBlockStart + 1);
		cv::imencode(ServerSiteConfig::get().webConfig.imageResponseFormat, resMat, processRes.resBuf);
		return processRes;
	}
	else
		return ProcessResult(false);
}

ImageFileReader::ProcessResult ImageFileReader::readForSegmentation(const cv::String& datasetFilePath, const cv::Rect2i& bbox,
	const std::pair<int, int>& objSizeRange)
{
	
	auto gdalDecoderPtr = ImgDecoderFactory::Instance()->getDecoder(datasetFilePath);

	
	auto blockImgProcessorPtr = BlockImageProcessor::Instance();
	std::string cacheDirUtf8 = ServerSiteConfig::get().webConfig.processedCacheDir;
	
	//TDDO: only check block counts
	std::size_t totalBlockCounts = 0;

	if (gdalDecoderPtr->supportBlockRead())
	{
		//calculate the required blocks that need to process
		int startBlockX = bbox.tl().x / gdalDecoderPtr->GetXBlockSize();
		int startBlockY = bbox.tl().y / gdalDecoderPtr->GetYBlockSize();
		int endBlockX = bbox.br().x % gdalDecoderPtr->GetXBlockSize() == 0 ? (bbox.br().x / gdalDecoderPtr->GetXBlockSize()) : (bbox.br().x / gdalDecoderPtr->GetXBlockSize() + 1);
		int endBlockY = bbox.br().y % gdalDecoderPtr->GetYBlockSize() == 0 ? (bbox.br().y / gdalDecoderPtr->GetYBlockSize()) : (bbox.br().y / gdalDecoderPtr->GetYBlockSize() + 1);
		
		blockImgProcessorPtr->startProcessImg(s_readBlockImgQueue);
		for (auto yIndex = startBlockY; yIndex <= endBlockY; yIndex++)
			for (auto xIndex = startBlockX; xIndex <= endBlockX; xIndex++)
			{
				totalBlockCounts++;
				std::string blockFileCachePath = BlockImageProcessor::getBlockFileCachePath(xIndex, yIndex, cacheDirUtf8);
				std::unique_ptr<BlockImageProcessor::BlockImgStruct> pBlockForProcess;
				if (exists(path(blockFileCachePath)))
				{
					pBlockForProcess.reset(new BlockImageProcessor::BlockImgStruct(xIndex, yIndex,
						blockFileCachePath, &m_processedBlockImgQueue));
				}
				else
				{
					//the block have not been processed and cached, read it and push into readBlock queue for process
					pBlockForProcess.reset(new BlockImageProcessor::BlockImgStruct(xIndex, yIndex,
						&m_processedBlockImgQueue));
					if (!gdalDecoderPtr->readBlockData(xIndex, yIndex, pBlockForProcess->blockImg))
					{
						printf("Error happens when reading block data at (%d, %d)", yIndex, xIndex);
					}
				}
				if (s_readBlockImgQueue.size_approx() > 5000)
				{
					std::this_thread::sleep_for(std::chrono::milliseconds(50));
				}
				else
				{
					while (!s_readBlockImgQueue.enqueue(*pBlockForProcess))
					{
						std::this_thread::sleep_for(std::chrono::milliseconds(50));
					}
				}
			}
		blockImgProcessorPtr->setReadComplete();

		//wait to reassemble the processed blocks
		int assembleImageRows = (endBlockY - startBlockY + 1) * gdalDecoderPtr->GetYBlockSize();
		int assembleImageCols = (endBlockX - startBlockX + 1) * gdalDecoderPtr->GetXBlockSize();
		std::unique_ptr<cv::Mat> assembledImagePtr;
		
		BlockImageProcessor::BlockImgStruct blockImgStruct;
		while (totalBlockCounts > 0)
		{
			m_processedBlockImgQueue.wait_dequeue(blockImgStruct);
			if (!assembledImagePtr)
			{
				assembledImagePtr.reset(new cv::Mat(assembleImageRows, assembleImageCols, blockImgStruct.blockImg.type()));
			}
			int xStart = (blockImgStruct.xIndex - startBlockX) * gdalDecoderPtr->GetXBlockSize();
			int yStart = (blockImgStruct.yIndex - startBlockY) * gdalDecoderPtr->GetYBlockSize();
			blockImgStruct.blockImg.copyTo((*assembledImagePtr)(cv::Rect(xStart, yStart, blockImgStruct.blockImg.cols, blockImgStruct.blockImg.rows)));
			totalBlockCounts--;
		}


		//compress the reassembled image to jpg and send the response back
		ProcessResult processRes(true);
		std::vector<int> param = std::vector<int>(2);
		auto xBlockStart = startBlockX * gdalDecoderPtr->GetXBlockSize();
		auto yBlockStart = startBlockY * gdalDecoderPtr->GetYBlockSize();
		cv::Mat& resMat = assembledImagePtr->rowRange(bbox.tl().y - yBlockStart, bbox.br().y - yBlockStart + 1)
			.colRange(bbox.tl().x - xBlockStart, bbox.br().x - xBlockStart + 1);
		cv::imencode(ServerSiteConfig::get().webConfig.imageResponseFormat, resMat, processRes.resBuf, param);
		return processRes;
	}
	else
	{
		return ProcessResult(false);
	}

	

}