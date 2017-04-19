#pragma once
#include "tinyxml.h"
#include "NFCommonDef.h"
#include "NFExceptions.h"
#include <boost/algorithm/string/split.hpp>
#include <boost/algorithm/string/classification.hpp>
#include <map>
#include <typeinfo> 
#include <sstream>

namespace NF
{

	class XmlElement
	{
	public:
		typedef TiXmlNode* Iterator;
		
		class ChildElementIter
		{
		public:
			ChildElementIter() : m_bStarted(false), m_pCurChild(NULL)
			{

			};
			bool m_bStarted;
			TiXmlElement* m_pCurChild;
		};

		XmlElement(TiXmlElement& value) :
			m_XmlValue(value),
			m_ElementType(AttributeChild),
			m_NameValueElementName(""),
			m_CurSeqNo(-1)
		{
		};


		XmlElement& operator=(const XmlElement& other)
		{
			m_ChildElements = other.m_ChildElements;
			m_XmlValue = other.m_XmlValue;
			m_CurSeqNo = other.m_CurSeqNo;
			m_ElementType = other.m_ElementType;
			m_NameValueElementName = other.m_NameValueElementName;
			m_childIterMap = other.m_childIterMap;
			m_seqValueVec = other.m_seqValueVec;
			return *this;
		};

		TiXmlElement& GetValue()
		{
			return m_XmlValue;
		};

		bool end(const std::string& name)
		{
			if (!name.empty())
			{
				std::map<std::string, ChildElementIter>::iterator it = m_childIterMap.find(name);
				return (it != m_childIterMap.end() && it->second.m_bStarted && it->second.m_pCurChild == NULL);
			}
			else
			{
				return m_CurSeqNo == m_seqValueVec.size();
			}
		}
		XmlElement& SetElement(ElementType eleType, const std::string& nameValueElementName)
		{
			m_ElementType = eleType;
			m_NameValueElementName = nameValueElementName;
			return (*this);
		};
		template< typename T >
		XmlElement& AddNameValueChild(const std::string& name, const T& value)
		{
			std::string tempValueStr;
			tempValueStr = boost::lexical_cast<std::string>(value);
			
			if (name.empty())
			{
				if (m_CurSeqNo == -1)
				{
					TiXmlText *text = new TiXmlText(tempValueStr);
					m_XmlValue.LinkEndChild(text);
					m_CurSeqNo = 0;
				}
				else
				{
					TiXmlText *text = (TiXmlText*)m_XmlValue.FirstChild();
					std::string curText;
					if (text->Value())
					{
						curText.append(text->Value());
						curText.append(",");
					}
					curText.append(tempValueStr);
					text->SetValue(curText);
				}
				m_CurSeqNo++;
				return *this;
			}
			switch (m_ElementType)
			{
			case SimpleChildElement:
			{
									   TiXmlElement * element = new TiXmlElement(name);
									   m_XmlValue.LinkEndChild(element);

									   TiXmlText *text = new TiXmlText(tempValueStr);
									   element->LinkEndChild(text);
			}
				break;
			case NameValueChildElement:
			{
						TiXmlElement * element = new TiXmlElement(m_NameValueElementName);
						m_XmlValue.LinkEndChild(element);
						element->SetAttribute("name", name);
						SetAttributeValue<T>(*element, "value", value);
			}
				break;
			case AttributeChild:
			default:
				SetAttributeValue<T>(m_XmlValue, name, value);
				break;
			};

			return *this;
		};
		
		XmlElement& AddArrayChildElement(const std::string& name, ArrayType arrayType)
		{
			
			if (arrayType == ComplexElementType)
				return (*this);
			else
			{
				TiXmlElement* pChildElement = (TiXmlElement*)m_XmlValue.LinkEndChild(new TiXmlElement(name));
				m_ChildElements.push_back(XmlElement(*pChildElement));

				return m_ChildElements.back();
			}
		}

		XmlElement& AddChildElement(const std::string& name)
		{
			TiXmlElement* pChildElement = (TiXmlElement*)m_XmlValue.LinkEndChild(new TiXmlElement(name));
			m_ChildElements.push_back(XmlElement(*pChildElement));

			return m_ChildElements.back();
		}

		XmlElement* GetChildElement(const std::string& name, bool bCanBeNull, bool bIsArray)
		{
			TiXmlElement* pChildElement = NULL;
			if (!bIsArray)
			{
				std::map<std::string, ChildElementIter>::iterator it = m_childIterMap.find(name);


				if (it == m_childIterMap.end())
				{
					std::pair<std::map<std::string, ChildElementIter>::iterator, bool> resultPair;
					resultPair = m_childIterMap.insert(std::make_pair(name, ChildElementIter()));
					if (resultPair.second)
						it = resultPair.first;
				}
				if (!it->second.m_bStarted)
				{
					//first return the first element
					pChildElement = (TiXmlElement*)m_XmlValue.FirstChildElement(name);
					if (pChildElement)
					{
						it->second.m_pCurChild = (TiXmlElement*)pChildElement->NextSiblingElement(name);
					}
					it->second.m_bStarted = true;
				}
				else
				{
					//return the current siblings
					pChildElement = it->second.m_pCurChild;
					if (pChildElement)
					{
						it->second.m_pCurChild = (TiXmlElement*)pChildElement->NextSiblingElement(name);
					}
				}

				if (pChildElement)
				{
					m_ChildElements.push_back(XmlElement(*pChildElement));
					return &m_ChildElements.back();
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
			else
			{
				std::map<std::string, ChildElementIter>::iterator it = m_childIterMap.find(name);
				//not started, just return this
				if (it == m_childIterMap.end())
				{
					m_childIterMap.insert(std::make_pair(name, ChildElementIter()));
					return this;
				}
			}
			return NULL;
		};

		template< typename T >
		void GetNameValueChild(const std::string& name, T& value)
		{
			if (name.empty())
			{
				if (m_CurSeqNo == -1)
				{
					std::string seqValue = m_XmlValue.GetText();
					boost::split(m_seqValueVec, seqValue, boost::is_any_of(","));
					m_CurSeqNo = 0;
				}
				if (m_seqValueVec.size() > 0)
				{
					try
					{
						value = boost::lexical_cast< T >(m_seqValueVec[m_CurSeqNo]);
					}
					catch (const boost::bad_lexical_cast &)
					{
						throw IncorrectElementTypeException(m_CurSeqNo + 1, name, typeid(T).name());
					}
					m_CurSeqNo++;
				}
				return ;
			}
			
			switch (m_ElementType)
			{
			case SimpleChildElement:
			{
						TiXmlElement * pElement = m_XmlValue.FirstChildElement(name);
						if (pElement == NULL)
						{
							throw MissingElementException(name);
						}
						std::string strValue = pElement->GetText();
						try
						{
							value = boost::lexical_cast<T>(strValue);
						}
						catch (const boost::bad_lexical_cast &)
						{
							throw IncorrectElementTypeException(name, typeid(T).name());
						}
						
			}
				break;
			case NameValueChildElement:
			{
						TiXmlElement* pChildElement	=  m_XmlValue.FirstChildElement(m_NameValueElementName);
						while (pChildElement)
						{
							const char* sAttributeName = pChildElement->Attribute("name");
							if (strcmp(sAttributeName,name.c_str()) == 0)
							{
								QueryAttributeValue<T>(*pChildElement, "value", value);
								return;
							}
							pChildElement = pChildElement->NextSiblingElement(m_NameValueElementName);
						}
						if (pChildElement == NULL)
						{
							throw MissingElementException(name);
						}
			}
			break;
			case AttributeChild:
			default:
				QueryAttributeValue< T >(m_XmlValue, name, value);
				break;
			};
			
			return ;
		};
	private:
		template< typename T >
		void SetAttributeValue(TiXmlElement& xmlEle, const std::string& name, const T& value)
		{
			xmlEle.SetAttribute(name.c_str(), value);
		};
		

		template< typename T >
		void QueryAttributeValue(TiXmlElement& xmlEle, const std::string& name, T& value)
		{
			if (xmlEle.Attribute(name))
			{
				if (xmlEle.QueryValueAttribute< T >(name, &value) == TIXML_WRONG_TYPE)
				{
					throw IncorrectElementTypeException(name, typeid(T).name());
				}
			}
			else
			{
				throw MissingElementException(name);
			}
		};

		std::vector<XmlElement> m_ChildElements;
		TiXmlElement &m_XmlValue;
		INT32 m_CurSeqNo;
		ElementType m_ElementType;
		std::string m_NameValueElementName;
		std::map<std::string, ChildElementIter> m_childIterMap;
		std::vector<std::string> m_seqValueVec;
	};

	template<>
	XmlElement& XmlElement::AddNameValueChild< bool >(const std::string& name, const bool& value)
	{
		std::string  valueString = value ? "true" : "false";
		return AddNameValueChild<std::string>(name, valueString);
	}

	template<>
	void XmlElement::SetAttributeValue< double >(TiXmlElement& xmlEle, const std::string& name, const double& value)
	{
		xmlEle.SetDoubleAttribute(name.c_str(), value);
	};

	template<>
	void XmlElement::QueryAttributeValue< std::string >(TiXmlElement& xmlEle, const std::string& name, std::string& value)
	{
		if (xmlEle.Attribute(name))
		{
			value = xmlEle.Attribute(name.c_str());
		}
	};
	template<>
	void XmlElement::QueryAttributeValue< bool >(TiXmlElement& xmlEle, const std::string& name, bool& value)
	{
		if (xmlEle.Attribute(name))
		{
			const char* strValue = xmlEle.Attribute(name.c_str());
			if (strcmp(strValue, "true") == 0)
				value = true;
			else if (strcmp(strValue, "false") == 0)
				value = false;
			else
			{
				throw IncorrectElementTypeException(name, typeid(bool).name());
			}
		}
	};
}