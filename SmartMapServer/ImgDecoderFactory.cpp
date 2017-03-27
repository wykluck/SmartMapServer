#include "ImgDecoderFactory.h"
#include <boost/bind.hpp>
#include <boost/thread.hpp>
#include <boost/asio.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>

using namespace cvGIS;

std::unordered_map<std::string, ImgDecoderFactory::ExpirableDecoderStruct> ImgDecoderFactory::s_decoderMap;
boost::asio::io_service ImgDecoderFactory::s_ioservice;
std::shared_ptr<boost::asio::deadline_timer> ImgDecoderFactory::s_expiredTimerPtr;


ImgDecoderFactory::ImgDecoderFactory()
{
	
}


ImgDecoderFactory::~ImgDecoderFactory()
{
}


void ImgDecoderFactory::checkExpired(const boost::system::error_code &e)
{
	//check expirable count
	for (auto expirableDecoderStrutItr = s_decoderMap.begin(); expirableDecoderStrutItr != s_decoderMap.end(); )
	{
		if (expirableDecoderStrutItr->second.expirableCounter == 0)
		{
			//delete the entry if it is expired
			s_decoderMap.erase(expirableDecoderStrutItr);
		}
		else
		{
			//decrease the reference count if it is not expired
			expirableDecoderStrutItr->second.expirableCounter--;
			expirableDecoderStrutItr++;
		}
	}
	//cancel the timer if the map is empty so that the thread will not be called.
	if (s_decoderMap.empty())
	{
		s_expiredTimerPtr->cancel();
	}
}

std::shared_ptr<GdalDecoder> ImgDecoderFactory::getDecoder(const std::string& filePath) 
{
	//start the timer for cleaning the map
	if (s_expiredTimerPtr.get() == nullptr)
	{
		s_expiredTimerPtr.reset(new boost::asio::deadline_timer(s_ioservice));
	}
	s_expiredTimerPtr->expires_from_now(boost::posix_time::minutes(1));
	s_expiredTimerPtr->async_wait(boost::bind(&ImgDecoderFactory::checkExpired,  boost::asio::placeholders::error));

	
	auto expirableDecoderItr = s_decoderMap.find(filePath);
	if (expirableDecoderItr == s_decoderMap.end())
	{
		std::shared_ptr<GdalDecoder> gdalDecoderPtr(new GdalDecoder());
		gdalDecoderPtr->setSource(filePath);
		if (!gdalDecoderPtr->readHeader())
		{
			printf("Error happens when reading file at (%s)", filePath.c_str());
			//return ProcessResult(false, std::vector<uchar>());
		}
		//everything will be expired roughly within 5 minutes
		ExpirableDecoderStruct expirableDecoderStruct = { gdalDecoderPtr, 5 };

		auto insertRes = s_decoderMap.insert(std::make_pair(filePath, expirableDecoderStruct));
		if (insertRes.second)
		{
			expirableDecoderItr = insertRes.first;
		}
	}
	else
	{
		expirableDecoderItr->second.expirableCounter = 5;
	}
	return expirableDecoderItr->second.decoder;
	
}