#pragma once
#include <opencv2/core.hpp>
#include "blockingconcurrentqueue.h"
#include "BlockImageProcessor.h"

namespace cvGIS
{
class ImageFileReader
{
public:
	struct ProcessResult
	{
		ProcessResult(bool isSuccessful_) : isSuccessful(isSuccessful)
		{}
		ProcessResult(ProcessResult&& processRes_)
		{
			isSuccessful = processRes_.isSuccessful;
			resBuf = std::move(processRes_.resBuf);
		}
		bool isSuccessful;
		std::vector<uchar> resBuf;
	};
	
	ImageFileReader();
	virtual ~ImageFileReader();

	
	ProcessResult readForProcessing(const cv::String& datasetFilePath, const cv::Rect2i& bbox, const std::pair<int, int>& objSizeRange);

private:
	static void ImageFileReader::postprocess(cv::Mat& img, const cv::Scalar& colorDiff, const std::pair<int, int>& objSizeRange);
	static moodycamel::BlockingConcurrentQueue<cvGIS::BlockImageProcessor::BlockImgStruct> s_readBlockImgQueue;
	moodycamel::BlockingConcurrentQueue<cvGIS::BlockImageProcessor::BlockImgStruct> m_processedBlockImgQueue;

};

}