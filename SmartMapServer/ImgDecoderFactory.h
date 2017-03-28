#pragma once
#include <unordered_map>
#include <string>
#include <memory>
#include "grfmt_gdal.hpp"
#include <boost/asio.hpp>
#include <mutex>
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
	
	static std::shared_ptr<ImgDecoderFactory> Instance();
	~ImgDecoderFactory();

	//TODO: need to throw an exception if it can't open the dataset
	std::shared_ptr<GdalDecoder> getDecoder(const std::string& filePath);
private: 
	ImgDecoderFactory();
	void checkExpired(const boost::system::error_code &e);

	//TODO: need to have a time expired mechanism to expire entries in the below map
	std::unordered_map<std::string, ExpirableDecoderStruct> m_decoderMap;
	boost::asio::io_service m_ioservice;
	std::shared_ptr<boost::asio::deadline_timer> m_expiredTimerPtr;
	std::mutex m_decoderMapMutex;
	

	static std::shared_ptr<ImgDecoderFactory> s_factoryPtr;
};

}