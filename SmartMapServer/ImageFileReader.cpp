#include "ImageFileReader.h"

#include <locale>
#include <codecvt>
#include "ServerSiteConfig.h"
#include "grfmt_gdal.hpp"
#include "opencv2/imgproc/types_c.h"
#include "ImgDecoderFactory.h"
#include <boost/filesystem.hpp>
#include <opencv2/highgui.hpp>

using namespace cvGIS;
using namespace boost::filesystem;
moodycamel::BlockingConcurrentQueue<BlockImageProcessor::BlockImgStruct> ImageFileReader::s_readBlockImgQueue;
ImageFileReader::ImageFileReader()
{
}


ImageFileReader::~ImageFileReader()
{
}

bool ImageFileReader::readCachedProcessResult(int xIndex, int yIndex, const std::string& cacheDirUtf8, int& imgType)
{
	std::string blockFileCachePath = BlockImageProcessor::getBlockFileCachePath(xIndex, yIndex, cacheDirUtf8);
	if (exists(path(blockFileCachePath)))
	{
		BlockImageProcessor::BlockImgStruct processedBlockStruct(xIndex, yIndex);
		processedBlockStruct.blockImg = cv::imread(blockFileCachePath);
		imgType = processedBlockStruct.blockImg.type();
		m_processedBlockImgQueue.enqueue(processedBlockStruct);
		return true;
	}
	else
		return false;
}

ImageFileReader::ProcessResult ImageFileReader::readForProcessing(const cv::String& datasetFilePath, const cv::Rect2i& bbox)
{
	
	auto gdalDecoderPtr = ImgDecoderFactory::Instance()->getDecoder(datasetFilePath);

	
	auto blockImgProcessorPtr = BlockImageProcessor::Instance();
	std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> convert;
	std::string cacheDirUtf8 = convert.to_bytes(ServerSiteConfig::getImageCacheDir().c_str());
	
	//TDDO: only check block counts
	std::size_t totalBlockCounts = 0;

	if (gdalDecoderPtr->GetXBlockSize() > 1 && gdalDecoderPtr->GetYBlockSize() > 1)
	{
		//calculate the required blocks that need to process
		int startBlockX = bbox.tl().x / gdalDecoderPtr->GetXBlockSize();
		int startBlockY = bbox.tl().y / gdalDecoderPtr->GetYBlockSize();
		int endBlockX = bbox.br().x % gdalDecoderPtr->GetXBlockSize() == 0 ? (bbox.br().x / gdalDecoderPtr->GetXBlockSize()) : (bbox.br().x / gdalDecoderPtr->GetXBlockSize() + 1);
		int endBlockY = bbox.br().y % gdalDecoderPtr->GetYBlockSize() == 0 ? (bbox.br().y / gdalDecoderPtr->GetYBlockSize()) : (bbox.br().y / gdalDecoderPtr->GetYBlockSize() + 1);
		
		blockImgProcessorPtr->startProcessImg(s_readBlockImgQueue);
		int imgType;
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
		param[0] = CV_IMWRITE_JPEG_QUALITY;
		param[1] = 95;
		auto xBlockStart = startBlockX * gdalDecoderPtr->GetXBlockSize();
		auto yBlockStart = startBlockY * gdalDecoderPtr->GetYBlockSize();
		cv::imencode(".jpg", assembledImagePtr->rowRange(bbox.tl().y - yBlockStart, bbox.br().y - yBlockStart + 1)
			.colRange(bbox.tl().x - xBlockStart, bbox.br().x - xBlockStart + 1),
			processRes.resBuf, param);
		return processRes;
	}
	else
	{
		return ProcessResult(false);
	}

	

}