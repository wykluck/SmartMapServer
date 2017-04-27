#include "RequestController.h"
#include "ServerSiteConfig.h"
#include "ImageFileReader.h"
#include <locale>
#include <codecvt>
#include <cpprest/filestream.h>
#include <opencv2/core/types.hpp>
#include <boost/algorithm/string.hpp>
#include <string>
#include <vector>
using namespace cvGIS;
using namespace web;                        // Common features like URIs.
using namespace web::http;                  // Common HTTP functionality
using namespace web::http::client;          // HTTP client features


RequestController::RequestController()
{
}


RequestController::~RequestController()
{
}

void RequestController::handleRequest(const http_request &request)
{
	//decode the url and convert to lower case
	utility::string_t decodedUrlString = web::uri::decode(request.request_uri().to_string());
	std::transform(decodedUrlString.begin(), decodedUrlString.end(), decodedUrlString.begin(), ::tolower);
	web::uri decodedUri(decodedUrlString);
	std::wcout << "GET " << decodedUrlString << std::endl;

	//url is supposed to be http://hostname:port/image/<imagePath>/<action>
	//get image file physical path
	auto http_path_vec = uri::split_path(decodedUri.path());
	
	std::string imageFilePath = ServerSiteConfig::get().webConfig.datasetRootDir;
	if (http_path_vec.size() == 3 && http_path_vec[0] == U("image"))
	{
		imageFilePath += ("\\");
		imageFilePath += utility::conversions::utf16_to_utf8(http_path_vec[1]);
	}


	if (http_path_vec[2] == U("segment"))
	{
		handleSegmentRequest(decodedUri.query(), request, imageFilePath);
	}
	else if (http_path_vec[2] == U("export"))
	{
		handleExportRequest(decodedUri.query(), request, imageFilePath);
	}
	else if (http_path_vec[2] == U("metadata"))
	{
		handleMetaDataRequest(decodedUri.query(), request, imageFilePath);
	}
}

std::unique_ptr<cv::Rect2i> RequestController::extractBBoxParam(const std::map<utility::string_t, utility::string_t>& http_get_vars)
{
	auto param_bbox = http_get_vars.find(U("bbox"));
	std::unique_ptr<cv::Rect2i> bboxPtr;
	if (param_bbox != end(http_get_vars))
	{
		std::vector<std::string> bboxStrVec;
		boost::split(bboxStrVec, param_bbox->second, boost::is_any_of(","));
		if (bboxStrVec.size() == 4)
		{
			//image coordinates tl.y < br.y
			int topX = std::stoi(bboxStrVec[0]);
			int topY = std::stoi(bboxStrVec[1]);
			int botX = std::stoi(bboxStrVec[2]);
			int botY = std::stoi(bboxStrVec[3]);
			bboxPtr = std::make_unique<cv::Rect2i>(cv::Rect2i(cv::Point2i(topX, topY), cv::Point2i(botX, botY)));
		}
	}
	return bboxPtr;
}

void sendProcessResult(const ImageFileReader::ProcessResult& processRes, const http_request &request)
{
	http_response httpResponse;
	if (processRes.isSuccessful)
	{
		httpResponse.set_body(processRes.resBuf);
		httpResponse.headers().add(U("Content-Type"), "image/png");
		httpResponse.set_status_code(http::status_codes::OK);
	}
	else
	{
		httpResponse.set_body("Error happened when trying to process the result");
		httpResponse.set_status_code(http::status_codes::InternalError);
	}
	request.reply(httpResponse);
}

void RequestController::handleSegmentRequest(const utility::string_t& queryString, const http_request &request,
	const std::string& imageFilePathUtf8)
{
	// url should be http://hostname:port/image/<imagePath>/segment?bbox=<xmin>,<ymin>,<xmax>,<ymax>&width=<width>&height=<height>&style=<style>&format=<imageformat>
	auto http_get_vars = uri::split_query(queryString);
	auto bboxPtr = extractBBoxParam(http_get_vars);

	std::pair<int, int> objSizeRange = std::make_pair<int, int>(INT_MAX, INT_MIN);
	auto param_minObjSize = http_get_vars.find(U("minobjsize"));
	if (param_minObjSize != end(http_get_vars))
	{
		objSizeRange.first = std::stoi(param_minObjSize->second);
	}
	auto param_maxObjSize = http_get_vars.find(U("maxobjsize"));
	if (param_maxObjSize != end(http_get_vars))
	{
		objSizeRange.second = std::stoi(param_maxObjSize->second);
	}

	ImageFileReader imageReader;
	auto processRes = imageReader.readForSegmentation(imageFilePathUtf8.c_str(), *bboxPtr.get(), objSizeRange);
	sendProcessResult(processRes, request);
}

void RequestController::handleMetaDataRequest(const utility::string_t& queryString, const http_request &request,
	const std::string& imageFilePathUtf8)
{
	// url should be http://hostname:port/image/<imagePath>/metadata
	ImageFileReader imageReader;
	auto metadataJsonString = imageReader.readForMetaData(imageFilePathUtf8.c_str());
	http_response httpResponse;
	httpResponse.set_body(metadataJsonString.c_str(), "application/json; charset=utf-8");
	httpResponse.set_status_code(http::status_codes::OK);
	request.reply(httpResponse);
}

void RequestController::handleExportRequest(const utility::string_t& queryString, const http_request &request,
	const std::string& imageFilePathUtf8)
{
	// url should be http://hostname:port/image/<imagePath>/export?bbox=<xmin>,<ymin>,<xmax>,<ymax>&width=<width>&height=<height>&style=<style>&format=<imageformat>
	auto http_get_vars = uri::split_query(queryString);
	auto bboxPtr = extractBBoxParam(http_get_vars);

	ImageFileReader imageReader;
	auto processRes = imageReader.readForExport(imageFilePathUtf8.c_str(), *bboxPtr.get());
	sendProcessResult(processRes, request);
}