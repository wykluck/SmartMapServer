#include <cpprest/http_client.h>
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

#include <algorithm>
#include <climits>
#include "ServerSiteConfig.h"

#include "blockingconcurrentqueue.h"
#include "BlockImageProcessor.h"

#include "RequestController.h"

using namespace utility;                    // Common utilities like string conversions
using namespace web;                        // Common features like URIs.
using namespace web::http;                  // Common HTTP functionality
using namespace web::http::client;          // HTTP client features
using namespace concurrency::streams;       // Asynchronous streams
using namespace web::http::experimental::listener;
using namespace std;
using namespace cvGIS;

cvGIS::RequestController requestController;

int main(int argc, char* argv[])
{
	uri baseSiteUrl(conversions::utf8_to_utf16(ServerSiteConfig::get().webConfig.baseSiteUrl));
	http_listener listener(baseSiteUrl);

	int count = 0;

	listener.support(methods::GET, [count](http_request request) mutable {
		
		requestController.handleRequest(request);

		
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