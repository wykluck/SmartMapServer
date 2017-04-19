#pragma once

#include <stdexcept>
#include <sstream>
namespace NF
{
	class CommonException : public std::exception
	{
	public:
		virtual ~CommonException() throw() {};
		virtual const char* what()
		{
			return m_sMessage.c_str();
		}
	protected:
		std::string m_sMessage;
	};
	
	class IllegalElementException : public CommonException
	{
	public:
		IllegalElementException(const std::string& elementName)
		{
			std::ostringstream strStream;
			strStream << "The " << elementName << " element is not allowed.";
			m_sMessage = strStream.str();
		};
		virtual ~IllegalElementException() throw() {};
	private:
		
	};


	class IncorrectElementTypeException : public CommonException
	{
	public:
		IncorrectElementTypeException(const std::string& elementName, const std::string& elementType)
		{
			std::ostringstream strStream;
			strStream << "The " << elementName << " element must be in " << elementType << ".";
			m_sMessage = strStream.str();
		};
		IncorrectElementTypeException(unsigned int seqNo, const std::string& elementName, const std::string& elementType)
		{
			std::ostringstream stringStream;
			stringStream << "The " << seqNo << " child element in " <<
				elementName << " element has to be type of " << elementType << " .";
			m_sMessage = stringStream.str();
		};
		virtual ~IncorrectElementTypeException() throw() {};

	};


	class MissingElementException : public CommonException
	{
	public:
		MissingElementException(const std::string& elementName) : m_sName(elementName)
		{
			std::ostringstream strStream;
			strStream << "The mandatory " << elementName << " element is missing.";
			m_sMessage = strStream.str();
		};
		const std::string& name() const
		{
			return m_sName;
		}
		
		virtual ~MissingElementException() throw() {};
	private:
		std::string m_sName;
	};


	class NoMoreElementException : public CommonException
	{
	public:
		NoMoreElementException(const std::string& elementName)
		{
			std::ostringstream strStream;
			strStream << "There is no " << elementName << " element any more.";
			m_sMessage = strStream.str();
		};
		virtual ~NoMoreElementException() throw() {};
	};

	class WrongElementCountException : public CommonException
	{
	public:
		WrongElementCountException(const std::string& elementName)
		{
			std::ostringstream strStream;
			strStream << "The number of sub element in element " << elementName <<
				" in xml or json doesn't match the counterparts in native code";
			m_sMessage = strStream.str();
		};
		virtual ~WrongElementCountException() throw() {};
	
	};
}