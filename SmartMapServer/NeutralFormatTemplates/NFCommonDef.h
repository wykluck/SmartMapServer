#pragma once

namespace NF
{

	typedef enum
	{
		AttributeChild,
		SimpleChildElement,
		NameValueChildElement
	}ElementType;

	typedef enum
	{
		SimpleElementType,
		ComplexElementType
	}ArrayType;

	struct SimpleChildElementStruct
	{};

	struct NameValueChildElementStruct
	{
		NameValueChildElementStruct(const std::string& _itemName) :itemName(_itemName)
		{};
		std::string itemName;
	};


}