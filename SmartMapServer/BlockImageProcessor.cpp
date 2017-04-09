#include "BlockImageProcessor.h"
#include "ServerSiteConfig.h"
#include <opencv2/core.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/highgui.hpp>
#include <chrono>
#include <thread>
#include "opencv2/imgproc/types_c.h"
#include <locale>
#include <codecvt>

using namespace cvGIS;
using namespace std;

std::shared_ptr<BlockImageProcessor> BlockImageProcessor::s_blockImgPtr;

std::shared_ptr<BlockImageProcessor> BlockImageProcessor::Instance()
{
	//TODO: need double checked pattern to gurantee uniqueness in multi-threading situation
	if (!s_blockImgPtr)
	{
		//TODO: need to set the thread count according to the cpu type
		std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> convert;
		std::string cacheDirUtf8 = convert.to_bytes(ServerSiteConfig::getImageCacheDir().c_str());
		s_blockImgPtr.reset(new BlockImageProcessor(6, cacheDirUtf8));
	}
	return s_blockImgPtr;
}

BlockImageProcessor::BlockImageProcessor(std::size_t threadCount, const cv::String& cacheDir)
	:m_threadCount(threadCount), m_cacheDir(cacheDir), m_pReadBlockImgQueue(nullptr)
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
				bool res = false;
				{
					std::unique_lock<std::mutex> lock(m_cvMutex);
					if (m_pReadBlockImgQueue != nullptr)
					{
						res = m_pReadBlockImgQueue->wait_dequeue_timed(inBlockStruct, std::chrono::milliseconds(100));
					}
					if (!res && m_startedTimes == 0)
						s_cv.wait(lock, [&]() { return m_startedTimes > 0; });
				}
				if (res)
				{
					BlockImgStruct processedBlockStruct(inBlockStruct.xIndex, inBlockStruct.yIndex);
					if (inBlockStruct.type == BlockImgStructType::ReadCacheFile)
					{
						processedBlockStruct.blockImg = cv::imread(inBlockStruct.blockFileCachePath);
						//push the processedBlockStruct to its corresponding output queue
						while (!inBlockStruct.pQueueForOutput->enqueue(processedBlockStruct))
						{
							std::this_thread::sleep_for(std::chrono::milliseconds(50));
						}
					}
					else if (inBlockStruct.type == BlockImgStructType::BlockForProcess)
					{
						cv::cvtColor(inBlockStruct.blockImg, hsvBlockImg, cv::COLOR_BGR2HSV);
						cv::pyrMeanShiftFiltering(hsvBlockImg, inBlockStruct.blockImg, 20, 15, 1);
						cv::cvtColor(inBlockStruct.blockImg, outBlockImg, cv::COLOR_HSV2BGR);
						processedBlockStruct.blockImg = outBlockImg;
						//first push the processedBlockStruct to its corresponding output queue
						while (!inBlockStruct.pQueueForOutput->enqueue(processedBlockStruct))
						{
							std::this_thread::sleep_for(std::chrono::milliseconds(50));
						}
						//then write the result to the output tile cache directory
						cv::imwrite(BlockImageProcessor::getBlockFileCachePath(inBlockStruct.xIndex, inBlockStruct.yIndex, m_cacheDir),
							outBlockImg);
					}
					else
					{
						//TODO: should not come to here
					}


				}
			}
		}
		));
	}
}

void BlockImageProcessor::setReadComplete() {
	std::unique_lock<std::mutex> lock(m_cvMutex);
	m_startedTimes--;
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

void BlockImageProcessor::startProcessImg(moodycamel::BlockingConcurrentQueue<BlockImgStruct>& readBlockImgQueue)
{
	std::unique_lock<std::mutex> lock(m_cvMutex);
	m_pReadBlockImgQueue = &readBlockImgQueue;
	m_startedTimes++;
	if (m_startedTimes > 0)
		s_cv.notify_all();
}
