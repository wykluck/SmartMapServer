#include "ImageMetaData.h"
#include "NeutralFormatTemplates/NFJsonElement.h"
#include "NeutralFormatTemplates/NFSerializer.h"
using namespace cvGIS;

std::string ImageMetaData::ToJsonString()
{
	Json::Value jsonRootForWrite;
	NF::JsonElement imageMetadataForWrite(jsonRootForWrite);
	NF::serializer< ImageMetaData >::serialize(imageMetadataForWrite, "", *this);
	Json::FastWriter jsonWriter;
	return jsonWriter.write(imageMetadataForWrite.GetValue());
}

ImageMetaData::ImageMetaData() : width(0), height(0), isGeoSpatialInfoValid(false)
{

}