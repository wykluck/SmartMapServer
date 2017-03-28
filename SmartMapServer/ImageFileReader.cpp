#include "ImageFileReader.h"
#include "blockingconcurrentqueue.h"
#include "BlockImageProcessor.h"
#include <locale>
#include <codecvt>
#include "ServerSiteConfig.h"
#include "grfmt_gdal.hpp"
#include "opencv2/imgproc/types_c.h"
#include "ImgDecoderFactory.h"
#include <opencv2/highgui.hpp>

using namespace cvGIS;

ImageFileReader::ImageFileReader()
{
}


ImageFileReader::~ImageFileReader()
{
}

ImageFileReader::ProcessResult ImageFileReader::readForProcessing(const cv::String& datasetFilePath, const cv::Rect2i& bbox)
{
	
	auto gdalDecoderPtr = ImgDecoderFactory::Instance()->getDecoder(datasetFilePath);

	moodycamel::BlockingConcurrentQueue<cvGIS::BlockImageProcessor::BlockImgStruct> readBlockImgQueue;
	std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> convert;
	std::string cacheDirUtf8 = convert.to_bytes(ServerSiteConfig::getImageCacheDir().c_str());
	cvGIS::BlockImageProcessor blockprocessor(6, cacheDirUtf8);
	moodycamel::BlockingConcurrentQueue<cvGIS::BlockImageProcessor::BlockImgStruct> processedBlockImgQueue;
	//TDDO: only check block counts
	std::size_t totalBlockCounts = 0;

	if (gdalDecoderPtr->GetXBlockSize() > 0 && gdalDecoderPtr->GetYBlockSize() > 0)
	{
		//calculate the required blocks that need to process
		int startBlockX = bbox.tl().x / gdalDecoderPtr->GetXBlockSize();
		int startBlockY = bbox.tl().y / gdalDecoderPtr->GetYBlockSize();
		int endBlockX = bbox.br().x % gdalDecoderPtr->GetXBlockSize() == 0 ? (bbox.br().x / gdalDecoderPtr->GetXBlockSize()) : (bbox.br().x / gdalDecoderPtr->GetXBlockSize() + 1);
		int endBlockY = bbox.br().y % gdalDecoderPtr->GetYBlockSize() == 0 ? (bbox.br().y / gdalDecoderPtr->GetYBlockSize()) : (bbox.br().y / gdalDecoderPtr->GetYBlockSize() + 1);
		
		blockprocessor.startProcessImg(readBlockImgQueue, processedBlockImgQueue);
		int imgType;
		for (auto yIndex = startBlockY; yIndex <= endBlockY; yIndex++)
			for (auto xIndex = startBlockX; xIndex <= endBlockX; xIndex++)
			{
				totalBlockCounts++;
				cvGIS::BlockImageProcessor::BlockImgStruct readBlockStruct(xIndex, yIndex);
				if (gdalDecoderPtr->readBlockData(xIndex, yIndex, readBlockStruct.blockImg))
				{
					imgType = readBlockStruct.blockImg.type();
					if (readBlockImgQueue.size_approx() > 5000
						|| !readBlockImgQueue.enqueue(readBlockStruct))
					{
						std::this_thread::sleep_for(std::chrono::milliseconds(50));
					}
				}
				else
				{
					printf("Error happens when reading block data at (%d, %d)", yIndex, xIndex);
				}

			}
		blockprocessor.setReadComplete();

		//wait to reassemble the processed blocks
		int assembleImageRows = (endBlockY - startBlockY + 1) * gdalDecoderPtr->GetYBlockSize();
		int assembleImageCols = (endBlockX - startBlockX + 1) * gdalDecoderPtr->GetXBlockSize();
		cv::Mat assembledImage(assembleImageRows, assembleImageCols, imgType);
		BlockImageProcessor::BlockImgStruct blockImgStruct;
		while (totalBlockCounts > 0)
		{
			processedBlockImgQueue.wait_dequeue(blockImgStruct);
			int xStart = (blockImgStruct.xIndex - startBlockX) * gdalDecoderPtr->GetXBlockSize();
			int yStart = (blockImgStruct.yIndex - startBlockY) * gdalDecoderPtr->GetYBlockSize();
			blockImgStruct.blockImg.copyTo(assembledImage(cv::Rect(xStart, yStart, blockImgStruct.blockImg.cols, blockImgStruct.blockImg.rows)));
			totalBlockCounts--;
		}


		//compress the reassembled image to jpg and send the response back
		std::vector<uchar> resBuf;
		std::vector<int> param = std::vector<int>(2);
		param[0] = CV_IMWRITE_JPEG_QUALITY;
		param[1] = 95;
		cv::imencode(".jpg", assembledImage.rowRange(bbox.tl().y - startBlockY * gdalDecoderPtr->GetYBlockSize(), bbox.br().y - bbox.tl().y + 1)
			.colRange(bbox.tl().x - startBlockX * gdalDecoderPtr->GetXBlockSize(), bbox.br().x - bbox.tl().x + 1),
			resBuf, param);
		return ProcessResult(true, resBuf);
	}


	

}