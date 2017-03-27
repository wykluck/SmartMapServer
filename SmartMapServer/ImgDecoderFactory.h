#pragma once
#include <unordered_map>
#include <string>
#include <memory>
#include "grfmt_gdal.hpp"
#include <boost/asio.hpp>
namespace boost
{
	namespace system {
		class error_code;
	}
}

namespace cvGIS
{ 
class ImgDecoderFactory
{
public:
	struct ExpirableDecoderStruct
	{
		std::shared_ptr<GdalDecoder> decoder;
		int expirableCounter;
	};
	
	ImgDecoderFactory();
	virtual ~ImgDecoderFactory();

	//TODO: need to throw an exception if it can't open the dataset
	static std::shared_ptr<GdalDecoder> getDecoder(const std::string& filePath);
private: 
	//TODO: need to have a time expired mechanism to expire entries in the below map
	static std::unordered_map<std::string, ExpirableDecoderStruct> s_decoderMap;

	static boost::asio::io_service s_ioservice;

	static std::shared_ptr<boost::asio::deadline_timer> s_expiredTimerPtr;

	static void checkExpired(const boost::system::error_code &e);
};

}