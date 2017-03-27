#pragma once
#include <vector>
#include <thread>
#include <opencv2/core.hpp>
#include "blockingconcurrentqueue.h"
namespace cvGIS {
	class BlockImageProcessor
	{
	public:
		struct BlockImgStruct
		{
			BlockImgStruct() {};
			BlockImgStruct(int xIndex_, int yIndex_) :
				xIndex(xIndex_), yIndex(yIndex_)
			{

			};

			int xIndex;
			int yIndex;
			cv::Mat blockImg;
		};

		void startProcessImg(moodycamel::BlockingConcurrentQueue<BlockImgStruct>& readBlockImgQueue, 
			moodycamel::BlockingConcurrentQueue<cvGIS::BlockImageProcessor::BlockImgStruct>& processedBlockImgQueue);
	
		void BlockImageProcessor::postprocess(cv::Mat& img, const cv::Scalar& colorDiff = cv::Scalar::all(1));

		void setReadComplete() {
			m_hasReadComplete = true;
		}

		BlockImageProcessor(std::size_t threadCount, const cv::String& outputDir);
		~BlockImageProcessor();
	private: 
		std::vector<std::thread> m_processThreadVec;
		std::size_t m_threadCount;
		cv::String m_cacheDir;
		bool m_hasReadComplete;
	};

}
