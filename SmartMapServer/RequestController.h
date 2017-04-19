#pragma once
#include <cpprest/uri.h>  
namespace cvGIS
{ 
class RequestController
{
public:
	RequestController();

	void handleRequest(const http_request &request);

	virtual ~RequestController();
private:
	void handleSegmentRequest(const utility::string_t& queryString);
	void handleMetaDataRequest(const utility::string_t& queryString);
	void handleExportRequest(const utility::string_t& queryString);
};

}
