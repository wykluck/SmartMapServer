#include "ServerSiteConfig.h"
#include "NeutralFormatTemplates/NFJsonElement.h"
#include "NeutralFormatTemplates/NFParser.h"
#include "NeutralFormatTemplates/NFCommonDef.h"

using namespace cvGIS;
ServerSiteConfigStruct ServerSiteConfig::s_serverSiteConfigStruct;
#define DEFAULT_CONFIGFILE_PATH "../config/config.json"

ServerSiteConfig::ServerSiteConfig()
{
	Json::Value jsonRootForRead;
	try
	{
		std::ifstream jsonFileStreamForRead(DEFAULT_CONFIGFILE_PATH);
		jsonFileStreamForRead >> jsonRootForRead;
	}
	catch (...)
	{
		std::ostringstream stringStream;
		stringStream << "Error happens when loading json file from " << DEFAULT_CONFIGFILE_PATH << " .";
		std::string errorMessage = stringStream.str();
		throw std::runtime_error(errorMessage.c_str());
	}

	NF::JsonElement jsonServerSiteConfig(jsonRootForRead);
	NF::parser< ServerSiteConfigStruct >::parse(jsonServerSiteConfig, "", s_serverSiteConfigStruct);
}


ServerSiteConfig::~ServerSiteConfig()
{
}

const ServerSiteConfigStruct& ServerSiteConfig::get()
{
	return s_serverSiteConfigStruct;
}