#pragma once
#include <opencv2/core.hpp>

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

	ProcessResult readForProcessing(const cv::String& datasetFilePath, const cv::Rect2i& bbox);

};

}