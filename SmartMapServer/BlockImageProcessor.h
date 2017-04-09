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

		static std::shared_ptr<BlockImageProcessor> BlockImageProcessor::Instance();

		~BlockImageProcessor();

		void startProcessImg(moodycamel::BlockingConcurrentQueue<BlockImgStruct>& readBlockImgQueue);
	
		void BlockImageProcessor::postprocess(cv::Mat& img, const cv::Scalar& colorDiff = cv::Scalar::all(1));

		static std::string getBlockFileCachePath(int xIndex, int yIndex, const std::string& cacheDirUtf8);


		void setReadComplete();

	private: 
		BlockImageProcessor(std::size_t threadCount, const cv::String& outputDir);
	
		std::vector<std::thread> m_processThreadVec;
		std::size_t m_threadCount;
		cv::String m_cacheDir;
		std::mutex m_cvMutex;
		std::size_t m_startedTimes;
		std::condition_variable s_cv;
		moodycamel::BlockingConcurrentQueue<BlockImgStruct>* m_pReadBlockImgQueue;

		static std::shared_ptr<BlockImageProcessor> s_blockImgPtr;
	};

}
