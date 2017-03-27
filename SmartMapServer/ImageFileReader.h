#pragma once
#include <opencv2/core.hpp>

namespace cvGIS
{
class ImageFileReader
{
public:
	struct ProcessResult
	{
		ProcessResult(bool isSuccessful_, const std::vector<uchar>& resBuf_)
		{
			isSuccessful = isSuccessful_;
			resBuf = std::move(resBuf_);
		}
		bool isSuccessful;
		std::vector<uchar> resBuf;
	};
	
	ImageFileReader();
	virtual ~ImageFileReader();

	ProcessResult readForProcessing(const cv::String& datasetFilePath, const cv::Rect2i& bbox);

};

}