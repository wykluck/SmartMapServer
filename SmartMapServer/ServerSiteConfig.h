#pragma once
#include <cpprest/details/basic_types.h>
class ServerSiteConfig
{
public:
	ServerSiteConfig();
	virtual ~ServerSiteConfig();

	static const utility::string_t& getImageRootDir();
	static const utility::string_t& getImageCacheDir();
private:
	static utility::string_t s_imageRootDir;
	static utility::string_t s_imageCacheDir;
};

