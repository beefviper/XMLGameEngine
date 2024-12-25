// game_xml.h
// XML Game Engine
// author: beefviper
// date: Sept 18, 2020

#pragma once

#include "object.h"
#include "states.h"

#include <xercesc/dom/DOM.hpp>
#include <xercesc/parsers/XercesDOMParser.hpp>
#include <xercesc/framework/LocalFileInputSource.hpp>
#include <xercesc/sax/HandlerBase.hpp>
#include <xercesc/sax/ErrorHandler.hpp>
#include <xercesc/sax/SAXParseException.hpp>
#include <xercesc/validators/common/Grammar.hpp>
#include <xercesc/util/PlatformUtils.hpp>
#include <xercesc/util/XMLString.hpp>
#include <xercesc/util/XMLChar.hpp>
#include <xercesc/util/XMLChTranscoder.hpp>

#include <iostream>
#include <string>

namespace xc = xercesc;

namespace xge
{
	class game_xml
	{
	public:
		game_xml() noexcept;
		~game_xml();

		game_xml(const game_xml&) = delete;
		game_xml& operator=(const game_xml&) = delete;
		game_xml(game_xml&& other) = delete;
		game_xml& operator=(game_xml&& other) = delete;

		void init(const std::string& filename, WindowDesc& windowDesc,
			std::map<std::string, float>& variables,
			std::vector<RawState>& rawStates,
			std::vector<RawObject>& rawObjects);

		std::unique_ptr<xc::XercesDOMParser> domParser{};

		class ParserErrorHandler : public xc::ErrorHandler
		{
		private:
			void reportParseException(const xc::SAXParseException& ex);

		public:
			void warning(const xc::SAXParseException& ex) override;
			void error(const xc::SAXParseException& ex) override;
			void fatalError(const xc::SAXParseException& ex) override;
			void resetErrors() noexcept override;
		};

		ParserErrorHandler parserErrorHandler;

		std::string getAttributeByName(const xc::DOMElement* element, const std::string& attribute);
	};

	std::string XMLChToStr(const XMLCh* toTranscode);

	class StrToXMLCh
	{
	public:
		StrToXMLCh(const std::string toTranscode);
		~StrToXMLCh();

		StrToXMLCh(const StrToXMLCh&) = delete;
		StrToXMLCh& operator=(const StrToXMLCh&) = delete;
		StrToXMLCh(StrToXMLCh&& other) = delete;
		StrToXMLCh& operator=(StrToXMLCh&& other) = delete;

		XMLCh* value() noexcept;

	private:
		XMLCh* data;
	};
}
