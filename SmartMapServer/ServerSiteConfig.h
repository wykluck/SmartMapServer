#pragma once
#include <cpprest/details/basic_types.h>
#include <boost/fusion/adapted/struct/adapt_struct.hpp>
#include <boost/fusion/include/adapt_struct.hpp>
namespace cvGIS {
	struct WebConfigStruct
	{
		std::string datasetRootDir;
		std::string processedCacheDir;
		std::string defaultImageResponseFormat;
	};
	
	struct ProcessConfigStruct
	{
		int blockProcessorThreadCount;
	};

	struct ServerSiteConfigStruct
	{
		WebConfigStruct webConfig;
		ProcessConfigStruct processConfig;
	};

	class ServerSiteConfig
	{
	public:
		ServerSiteConfig();
		virtual ~ServerSiteConfig();

		static const ServerSiteConfigStruct& get();
	private:
		static ServerSiteConfigStruct s_serverSiteConfigStruct;
	};
}

BOOST_FUSION_ADAPT_STRUCT(cvGIS::WebConfigStruct,
	(std::string, datasetRootDir)
	(std::string, processedCacheDir)
	(std::string, defaultImageResponseFormat)
)

BOOST_FUSION_ADAPT_STRUCT(cvGIS::ProcessConfigStruct,
	(int, blockProcessorThreadCount)
)

BOOST_FUSION_ADAPT_STRUCT(cvGIS::ServerSiteConfigStruct,
	(cvGIS::WebConfigStruct, webConfig)
	(cvGIS::ProcessConfigStruct, processConfig)
)