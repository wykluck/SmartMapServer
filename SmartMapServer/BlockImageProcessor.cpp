#include "BlockImageProcessor.h"
#include <opencv2/core.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/highgui.hpp>
#include <chrono>
#include <thread>
#include "opencv2/imgproc/types_c.h"

using namespace cvGIS;
using namespace std;

BlockImageProcessor::BlockImageProcessor(std::size_t threadCount, const cv::String& cacheDir)
	:m_threadCount(threadCount), m_cacheDir(cacheDir), m_hasReadComplete(false)
{

}



BlockImageProcessor::~BlockImageProcessor()
{
	for (std::size_t i = 0; i < m_processThreadVec.size(); i++)
	{
		m_processThreadVec[i].join();
	}
}

std::string BlockImageProcessor::getBlockFileCachePath(int xIndex, int yIndex, const std::string& cacheDirUtf8)
{
	std::string outputFilePath = cacheDirUtf8;
	outputFilePath += "\\";
	outputFilePath += std::to_string(xIndex);
	outputFilePath += "_";
	outputFilePath += std::to_string(yIndex);
	outputFilePath += ".jpg";
	return outputFilePath;
}

void BlockImageProcessor::postprocess(cv::Mat& img, const cv::Scalar& colorDiff)
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
				if (area > 1600 && area < 4000 && boundingRect.height > 10 && boundingRect.width > 10)
				{
					boundingRect = boundingRect + cv::Point(-1, -1);
					cv::rectangle(img, boundingRect, cv::Scalar(0, 0, 0));
				}
			}
		}
	}

}

void BlockImageProcessor::startProcessImg(moodycamel::BlockingConcurrentQueue<BlockImgStruct>& readBlockImgQueue, 
	moodycamel::BlockingConcurrentQueue<cvGIS::BlockImageProcessor::BlockImgStruct>& processedBlockImgQueue)
{
	for (std::size_t i = 0; i < m_threadCount; i++)
	{
		m_processThreadVec.push_back(std::thread(
			[&]()
		{
			while (1)
			{
				BlockImgStruct inBlockStruct;
				cv::Mat hsvBlockImg, outBlockImg;
				bool res = readBlockImgQueue.wait_dequeue_timed(inBlockStruct, std::chrono::milliseconds(100));
				if (!res && m_hasReadComplete)
					break;
				if (res)
				{
					//processedBlockImgQueue.try_enqueue(inBlockStruct);
					cv::cvtColor(inBlockStruct.blockImg, hsvBlockImg, cv::COLOR_BGR2HSV);
					cv::pyrMeanShiftFiltering(hsvBlockImg, inBlockStruct.blockImg, 20, 15, 1);
					cv::cvtColor(inBlockStruct.blockImg, outBlockImg, cv::COLOR_HSV2BGR);
					BlockImgStruct processedBlockStruct(inBlockStruct.xIndex, inBlockStruct.yIndex);
					processedBlockStruct.blockImg = outBlockImg;
					processedBlockImgQueue.try_enqueue(processedBlockStruct);
					
					
					cv::imwrite(BlockImageProcessor::getBlockFileCachePath(inBlockStruct.xIndex, inBlockStruct.yIndex, m_cacheDir), outBlockImg);
				}
			}
		}
		));
	}
}
