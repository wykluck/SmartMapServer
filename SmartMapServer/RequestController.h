#pragma once
#include <cpprest/uri.h>  
#include <cpprest/http_listener.h>
#include <opencv2/core/types.hpp>

namespace cvGIS
{ 

class RequestController
{
public:
	RequestController();

	void handleRequest(const web::http::http_request &request);

	virtual ~RequestController();
private:
	std::unique_ptr<cv::Rect2i> extractBBoxParam(const std::map<utility::string_t, utility::string_t>& http_get_vars);

	void handleSegmentRequest(const utility::string_t& queryString, const web::http::http_request &request,
		const std::string& imageFilePathUtf8);
	void handleMetaDataRequest(const utility::string_t& queryString, const web::http::http_request &request,
		const std::string& imageFilePathUtf8);
	void handleExportRequest(const utility::string_t& queryString, const web::http::http_request &request,
		const std::string& imageFilePathUtf8);
};

}
