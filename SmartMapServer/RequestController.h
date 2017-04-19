#pragma once
#include <cpprest/uri.h>  
#include <cpprest/http_listener.h>
namespace cvGIS
{ 
class RequestController
{
public:
	RequestController();

	void handleRequest(const web::http::http_request &request);

	virtual ~RequestController();
private:
	void handleSegmentRequest(const utility::string_t& queryString, const web::http::http_request &request,
		const std::string& imageFilePathUtf8);
	void handleMetaDataRequest(const utility::string_t& queryString, const web::http::http_request &request,
		const std::string& imageFilePathUtf8);
	void handleExportRequest(const utility::string_t& queryString, const web::http::http_request &request,
		const std::string& imageFilePathUtf8);
};

}
