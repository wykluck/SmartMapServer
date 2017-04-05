#include "ImgDecoderFactory.h"
#include <boost/bind.hpp>
#include <boost/thread.hpp>
#include <boost/asio.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>

using namespace cvGIS;



std::shared_ptr<ImgDecoderFactory> ImgDecoderFactory::s_factoryPtr;

std::shared_ptr<ImgDecoderFactory> ImgDecoderFactory::Instance()
{
	//TODO: need double checked pattern to gurantee uniqueness in multi-threading situation
	if (!s_factoryPtr)
	{
		s_factoryPtr.reset(new ImgDecoderFactory());
	}
	return s_factoryPtr;
}


ImgDecoderFactory::ImgDecoderFactory()
{
	m_expiredTimerPtr.reset(new boost::asio::deadline_timer(m_ioservice, boost::posix_time::seconds(5)));
	std::thread(
		[&]()
	{
		//start the timer for cleaning the map
		m_expiredTimerPtr->async_wait(boost::bind(&ImgDecoderFactory::checkExpired, this, boost::asio::placeholders::error));
		m_ioservice.run();
	}
	).detach();
}


ImgDecoderFactory::~ImgDecoderFactory()
{
}


void ImgDecoderFactory::checkExpired(const boost::system::error_code &e)
{
	//check expirable count
	std::unique_lock<std::mutex> lock(m_decoderMapMutex);
	
	for (auto expirableDecoderStrutItr = m_decoderMap.begin(); expirableDecoderStrutItr != m_decoderMap.end(); )
	{
		if (expirableDecoderStrutItr->second.expirableCounter == 0)
		{
			//delete the entry if it is expired
			m_decoderMap.erase(expirableDecoderStrutItr++);
		}
		else
		{
			//decrease the reference count if it is not expired
			expirableDecoderStrutItr->second.expirableCounter--;
			expirableDecoderStrutItr++;
		}
	}
	//cancel the timer if the map is empty so that the thread will not be called.
	if (!m_decoderMap.empty())
	{
		m_expiredTimerPtr->expires_from_now(boost::posix_time::seconds(5));
		m_expiredTimerPtr->async_wait(boost::bind(&ImgDecoderFactory::checkExpired, this, boost::asio::placeholders::error));
	}
}

std::shared_ptr<GdalDecoder> ImgDecoderFactory::getDecoder(const std::string& filePath) 
{
	if (m_ioservice.stopped())
	{
		std::thread(
			[&]()
		{
			//start the timer for cleaning the map
			m_expiredTimerPtr->async_wait(boost::bind(&ImgDecoderFactory::checkExpired, this, boost::asio::placeholders::error));
			m_ioservice.reset();
			m_ioservice.run();
		}
		).detach();
	}
	std::unique_lock<std::mutex> lock(m_decoderMapMutex);
	auto expirableDecoderItr = m_decoderMap.find(filePath);
	if (expirableDecoderItr == m_decoderMap.end())
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

		auto insertRes = m_decoderMap.insert(std::make_pair(filePath, expirableDecoderStruct));
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