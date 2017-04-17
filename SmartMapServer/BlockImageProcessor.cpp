#include "BlockImageProcessor.h"
#include "ServerSiteConfig.h"
#include <opencv2/core.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/highgui.hpp>
#include <opencv2/core/ocl.hpp>
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

void tryToSetOpenCLDevice()
{
	
	if (!cv::ocl::haveOpenCL())
	{
		cout << "OpenCL is not available..." << endl;
		//return;
	}

	std::vector<cv::ocl::PlatformInfo> platforms;
	cv::ocl::getPlatfomsInfo(platforms);

	//OpenCL Platforms
	for (size_t i = 0; i < platforms.size(); i++)
	{

		//Access to Platform
		const cv::ocl::PlatformInfo* platform = &platforms[i];

		//Platform Name
		std::cout << "Platform Name: " << platform->name().c_str() << "\n";

		//Access Device within Platform
		cv::ocl::Device current_device;
		for (int j = 0; j < platform->deviceNumber(); j++)
		{
			//Access Device
			platform->getDevice(current_device, j);

			cout << "name:              " << current_device.name() << endl;
			cout << "available:         " << current_device.available() << endl;
			cout << "imageSupport:      " << current_device.imageSupport() << endl;
			cout << "type:				" << current_device.type() << endl;
			cout << "OpenCL_C_Version:  " << current_device.OpenCL_C_Version() << endl;
			cout << endl;

		}
	}
	
	
	//cv::ocl::Device(context.device(0)); //Here is where you change which GPU to use (e.g. 0 or 1)
}

BlockImageProcessor::BlockImageProcessor(std::size_t threadCount, const cv::String& cacheDir)
	:m_threadCount(threadCount), m_cacheDir(cacheDir), m_pReadBlockImgQueue(nullptr)
{
	tryToSetOpenCLDevice();
	m_meanShiftSegPtr = createMeanShiftSegmentation(8, 5.5f, 20, true);
	for (std::size_t i = 0; i < m_threadCount; i++)
	{
		m_processThreadVec.push_back(std::thread(
			[&]()
		{
			while (1)
			{
				BlockImgStruct inBlockStruct;
				cv::UMat hsvBlockImg, meanshifBlockImg;
				cv::Mat outBlockImg;
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
						m_meanShiftSegPtr->processImage(inBlockStruct.blockImg, outBlockImg);
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



void BlockImageProcessor::startProcessImg(moodycamel::BlockingConcurrentQueue<BlockImgStruct>& readBlockImgQueue)
{
	std::unique_lock<std::mutex> lock(m_cvMutex);
	m_pReadBlockImgQueue = &readBlockImgQueue;
	m_startedTimes++;
	if (m_startedTimes > 0)
		s_cv.notify_all();
}
