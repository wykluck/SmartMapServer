#pragma once
#include <vector>
#include <thread>
#include <mutex>
#include <opencv2/core.hpp>
#include "blockingconcurrentqueue.h"
namespace cvGIS {
	class BlockImageProcessor
	{
	public:
		enum class BlockImgStructType
		{
			ReadCacheFile,
			BlockForProcess,
			ProcessedBlock
		};
		struct BlockImgStruct
		{
			BlockImgStruct() {};
			BlockImgStruct(int xIndex_, int yIndex_, 
				moodycamel::BlockingConcurrentQueue<BlockImgStruct>* pQueueForOutput_) :
				xIndex(xIndex_), yIndex(yIndex_), type(BlockImgStructType::BlockForProcess), pQueueForOutput(pQueueForOutput_)
			{

			};
			BlockImgStruct(int xIndex_, int yIndex_, const std::string& blockFileCachePath_,
				moodycamel::BlockingConcurrentQueue<BlockImgStruct>* pQueueForOutput_) :
				xIndex(xIndex_), yIndex(yIndex_), type(BlockImgStructType::ReadCacheFile),
				blockFileCachePath(blockFileCachePath_), pQueueForOutput(pQueueForOutput_)
			{

			}
			BlockImgStruct(int xIndex_, int yIndex_) :
				xIndex(xIndex_), yIndex(yIndex_), type(BlockImgStructType::ProcessedBlock), pQueueForOutput(nullptr)
			{

			}

			int xIndex;
			int yIndex;
			cv::Mat blockImg;
			BlockImgStructType type;
			std::string blockFileCachePath;
			moodycamel::BlockingConcurrentQueue<BlockImgStruct>* pQueueForOutput;
		};

		void startProcessImg(moodycamel::BlockingConcurrentQueue<BlockImgStruct>& readBlockImgQueue, 
			moodycamel::BlockingConcurrentQueue<cvGIS::BlockImageProcessor::BlockImgStruct>& processedBlockImgQueue);
	
		void BlockImageProcessor::postprocess(cv::Mat& img, const cv::Scalar& colorDiff = cv::Scalar::all(1));

		static std::string getBlockFileCachePath(int xIndex, int yIndex, const std::string& cacheDirUtf8);

		void setReadComplete() {
			std::unique_lock<std::mutex> lock(m_readCompleteMutex);
			m_hasReadComplete = true;
		}

		BlockImageProcessor(std::size_t threadCount, const cv::String& outputDir);
		~BlockImageProcessor();
	private: 
		std::vector<std::thread> m_processThreadVec;
		std::size_t m_threadCount;
		cv::String m_cacheDir;
		bool m_hasReadComplete;
		std::mutex m_readCompleteMutex;
	};

}
