#pragma once
#include "json/json.h"
#include "NFCommonDef.h"
#include "NFExceptions.h"
#include <sstream>

namespace NF
{

class JsonElement
{
public:
	JsonElement(Json::Value& value, ElementType elementType = AttributeChild) :
		m_JsonValue(value),
		m_ElementType(elementType),
		m_NameValueElementName(""),
		m_valueIter(value.begin())
	{

	};

	~JsonElement()
	{

	}

	JsonElement& operator=(const JsonElement& other)
	{
		m_ChildElements = other.m_ChildElements;
		m_JsonValue = other.m_JsonValue;
		m_ElementType = other.m_ElementType;
		m_NameValueElementName = other.m_NameValueElementName;
		m_valueIter = other.m_valueIter;
		return *this;
	};


	Json::Value& GetValue()
	{
		return m_JsonValue;
	};

	bool end(const std::string& name)
	{
		return m_valueIter == m_JsonValue.end();
	};

	JsonElement& SetElement(ElementType eleType, const std::string& nameValueElementName)
	{
		m_ElementType = eleType;
		return *this;
	};

	JsonElement& AddArrayChildElement(const std::string& name, ArrayType SimpleElementType)
	{
		return AddInternalChildElement(name, Json::arrayValue);
	};

	JsonElement& AddChildElement(const std::string& name)
	{
		return AddInternalChildElement(name, Json::nullValue);
	};
	template< typename T >
	JsonElement& AddNameValueChild(const std::string& name, const T& value)
	{
		if (m_JsonValue.isArray())
		{
			Json::Value tempValue = (T)value;
			m_JsonValue.append(tempValue);
		}
		else
		{
			m_JsonValue[name] = value;
		}
		return *this;
	};

	JsonElement* GetChildElement(const std::string& name, bool bCanBeNull, bool bIsArray)
	{
		if (m_JsonValue.isArray())
		{
			//parent is a json array, push back a new json element with current iterator
			if (m_valueIter == m_JsonValue.end())
				return NULL;
			m_ChildElements.push_back(JsonElement(*m_valueIter));
			++m_valueIter;
		}
		else
		{
			if (m_JsonValue.isMember(name))
			{
				m_ChildElements.push_back(JsonElement(m_JsonValue[name]));
			}
			else
			{
				if (!bCanBeNull)
				{	
					throw MissingElementException(name);
				}
				else
					return NULL;
			}
		}
		
		return &m_ChildElements.back();
	};


	template< typename T >
	void GetNameValueChild(const std::string& name, T& value)
	{
		try
		{
			if (m_JsonValue.isArray())
			{
				if (m_valueIter == m_JsonValue.end())
				{
					throw NoMoreElementException(name);
				}
				value = (T)(m_valueIter->asInt64());
				++m_valueIter;
			}
			else if (m_JsonValue.isMember(name))
			{
				value = (T)(m_JsonValue[name].asInt64());
			}
			else
			{
				throw MissingElementException(name);
			}
		}
		catch (std::runtime_error&)
		{
			throw IncorrectElementTypeException(name, typeid(T).name());
		}	
		
	};

	template<>
	void GetNameValueChild< bool >(const std::string& name, bool& value)
	{
		try
		{

			if (m_JsonValue.isArray())
			{
				if (m_valueIter == m_JsonValue.end())
				{
					throw NoMoreElementException(name);
				}
				value = m_valueIter->asBool();
				m_valueIter++;
			}
			else if (m_JsonValue.isMember(name))
			{
				value = m_JsonValue[name].asBool();
			}
			else
			{
				throw MissingElementException(name);
			}
		}
		catch (std::runtime_error&)
		{
			throw IncorrectElementTypeException(name, typeid(bool).name());
		}

	};

	template<>
	void GetNameValueChild< float >(const std::string& name, float& value)
	{
		try
		{
			if (m_JsonValue.isArray())
			{
				if (m_valueIter == m_JsonValue.end())
				{
					throw NoMoreElementException(name);
				}
				value = m_valueIter->asFloat();
				m_valueIter++;
			}
			else if (m_JsonValue.isMember(name))
			{
				value = m_JsonValue[name].asFloat();
			}
			else
			{
				throw MissingElementException(name);
			}
		}
		catch (std::runtime_error&)
		{
			throw IncorrectElementTypeException(name, typeid(float).name());
		}


	};

	template<>
	void GetNameValueChild< double >(const std::string& name, double& value)
	{
		try
		{
			if (m_JsonValue.isArray())
			{
				if (m_valueIter == m_JsonValue.end())
				{
					throw NoMoreElementException(name);
				}
				value = m_valueIter->asDouble();
				m_valueIter++;
			}
			else if (m_JsonValue.isMember(name))
			{
				value = m_JsonValue[name].asDouble();
			}
			else
			{
				throw MissingElementException(name);
			}
		}
		catch (std::runtime_error&)
		{
			throw IncorrectElementTypeException(name, typeid(double).name());
		}

	};

	template<>
	void GetNameValueChild< std::string >(const std::string& name, std::string& value)
	{
		try
		{
			if (m_JsonValue.isArray())
			{
				if (m_valueIter == m_JsonValue.end())
				{
					throw NoMoreElementException(name);
				}
				value = m_valueIter->asString();
				m_valueIter++;
			}
			else if (m_JsonValue.isMember(name))
			{
				value = m_JsonValue[name].asString();
			}
			else
			{
				throw MissingElementException(name);
			}
		}
		catch (std::runtime_error&)
		{
			throw IncorrectElementTypeException(name, typeid(std::string).name());
		}


	}


private:
	JsonElement& AddInternalChildElement(const std::string& name, Json::ValueType type)
	{
		Json::Value *pTempValue = NULL;
		if (m_JsonValue.isArray())
		{
			pTempValue = &m_JsonValue.append(Json::Value(type));
		}
		else
		{
			m_JsonValue[name] = Json::Value(type);
			pTempValue = &m_JsonValue[name];
		}
		m_ChildElements.push_back(JsonElement(*pTempValue));
		return m_ChildElements.back();
	}
	
	std::vector<JsonElement> m_ChildElements;
	Json::Value &m_JsonValue;
	ElementType m_ElementType;
	std::string m_NameValueElementName;
	Json::ValueIterator m_valueIter;
};



}