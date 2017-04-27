#include "ServerSiteConfig.h"
#include "NeutralFormatTemplates/NFJsonElement.h"
#include "NeutralFormatTemplates/NFParser.h"
#include "NeutralFormatTemplates/NFCommonDef.h"
#include "windows.h"
#include <boost/filesystem.hpp>
using namespace cvGIS;
ServerSiteConfig ServerSiteConfig::s_serverSiteConfig;


std::string getConfigFilePath()
{
#ifdef _WIN32
	__wchar_t pBuf[1024];
	DWORD len = 1024;
	int bytes = GetModuleFileName(NULL, pBuf, len);
	
	boost::filesystem::path configPath(pBuf);
	auto configFilePath = configPath.parent_path().append(L"..\\config\\config.json");
	return configFilePath.string();
#else
	#error Not implemented for the platform
#endif
}

ServerSiteConfig::ServerSiteConfig()
{
	Json::Value jsonRootForRead;
	try
	{
		std::ifstream jsonFileStreamForRead(getConfigFilePath());
		jsonFileStreamForRead >> jsonRootForRead;
	}
	catch (...)
	{
		std::ostringstream stringStream;
		stringStream << "Error happens when loading json file from " << getConfigFilePath() << " .";
		std::string errorMessage = stringStream.str();
		throw std::runtime_error(errorMessage.c_str());
	}

	NF::JsonElement jsonServerSiteConfig(jsonRootForRead);
	NF::parser< ServerSiteConfigStruct >::parse(jsonServerSiteConfig, "", m_serverSiteConfigStruct);
}


ServerSiteConfig::~ServerSiteConfig()
{
}

const ServerSiteConfigStruct& ServerSiteConfig::get()
{
	return s_serverSiteConfig.m_serverSiteConfigStruct;
}