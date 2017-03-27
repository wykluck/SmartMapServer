#include "ServerSiteConfig.h"


utility::string_t ServerSiteConfig::s_imageRootDir = U("C:\\datasets");
utility::string_t ServerSiteConfig::s_imageCacheDir = U("C:\\TEMP\\tile_output");

ServerSiteConfig::ServerSiteConfig()
{
}


ServerSiteConfig::~ServerSiteConfig()
{
}


const utility::string_t& ServerSiteConfig::getImageRootDir()
{
	return s_imageRootDir;
}

const utility::string_t& ServerSiteConfig::getImageCacheDir()
{
	return s_imageCacheDir;
}
