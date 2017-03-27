#include <cpprest/http_client.h>
#include <cpprest/filestream.h>
#include <cpprest/http_listener.h>
#include <cpprest/json.h>                       // JSON library
#include <cpprest/uri.h>  
#include <string>
#include <iostream>
#include <iomanip>
#include <sstream>
#include <thread>
#include <chrono>
#include <ctime>
#include <locale>
#include <codecvt>
#include "ServerSiteConfig.h"
#include "ImageFileReader.h"
#include "blockingconcurrentqueue.h"
#include "BlockImageProcessor.h"
#include <opencv2/core/types.hpp>
#include <boost/algorithm/string.hpp>

using namespace utility;                    // Common utilities like string conversions
using namespace web;                        // Common features like URIs.
using namespace web::http;                  // Common HTTP functionality
using namespace web::http::client;          // HTTP client features
using namespace concurrency::streams;       // Asynchronous streams
using namespace web::http::experimental::listener;
using namespace std;
using namespace cvGIS;


std::string read_file(std::string filename) {
	ifstream inFile;
	inFile.open(filename);

	std::stringstream strStream;
	strStream << inFile.rdbuf();
	string str = strStream.str();
	return str;
}
/*
boost::format get_template(std::string filename) {
	std::string content = read_file(filename);
	boost::format con = boost::format(content);
	return con;
}
*/


int main(int argc, char* argv[])
{
	http_listener listener(U("http://localhost:12345"));

	int count = 0;

	listener.support(methods::GET, [count](http_request request) mutable {
		std::wcout << "GET " << request.request_uri().to_string() << std::endl;

		//url is supposed to be http://hostname:port/image/<imagePath>/export?bbox=<xmin>,<ymin>,<xmax>,<ymax> 
		
		//get image file physical path
		auto http_path_vec = uri::split_path(request.request_uri().path());
		utility::string_t imageFilePath = ServerSiteConfig::getImageRootDir();
		if (http_path_vec.size() == 3 && http_path_vec[0] == U("image") && http_path_vec[2] == U("export"))
		{
			imageFilePath += U("\\");
			imageFilePath += http_path_vec[1];
		}

		
		std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> convert;
		std::string imageFilePathUtf8 = convert.to_bytes(imageFilePath.c_str());


		auto http_get_vars = uri::split_query(request.request_uri().query());
		auto param_bbox = http_get_vars.find(U("bbox"));
		std::unique_ptr<cv::Rect2i> bboxPtr;
		if (param_bbox != end(http_get_vars))
		{
			vector<string> bboxStrVec;
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


		ImageFileReader imageReader;
		auto processedRes = imageReader.readForProcessing(imageFilePathUtf8.c_str(), *bboxPtr.get());
		http_response httpResponse;
		if (processedRes.isSuccessful)
		{
			httpResponse.set_body(processedRes.resBuf);
			httpResponse.headers().add(U("Content-Type"), "image/jpeg");
			httpResponse.set_status_code(http::status_codes::OK);
		}
		else
		{
			httpResponse.set_body("Error happened when trying to process the result");
			httpResponse.headers().add(U("Content-Type"), "text/plain");
			httpResponse.set_status_code(http::status_codes::InternalError);
		}
		request.reply(httpResponse);
	});

	listener.open().wait();
	std::wcout << "Web server started on: " << listener.uri().to_string() << std::endl;

	while (true) {
		this_thread::sleep_for(chrono::milliseconds(2000));
	}

	std::cout << "Terminating JSON listener." << endl;
	listener.close().wait();

	return 0;
}