#pragma once
#include <string>
#include <boost/fusion/adapted/struct/adapt_struct.hpp>
#include <boost/fusion/include/adapt_struct.hpp>
#include "NeutralFormatTemplates/NFCommonDef.h"


namespace cvGIS {

	struct ImageMetaData {
		int width;
		int height;
		double pixelWidthInMeters;
		double pixelHeightInMeters;
		double originX;
		double originY;
		std::string projectionIdentifier;
		bool isGeoSpatialInfoValid;
		std::string ToJsonString();
		ImageMetaData();
	};
	
};

BOOST_FUSION_ADAPT_STRUCT(cvGIS::ImageMetaData,
(int, width)
(int, height)
(double, pixelWidthInMeters)
(double, pixelHeightInMeters)
(double, originX)
(double, originY)
(std::string, projectionIdentifier)
(bool, isGeoSpatialInfoValid)
)