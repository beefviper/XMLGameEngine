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
		game_xml();
		~game_xml();

		void init(std::string& filename, WindowDesc& windowDesc,
			std::map<std::string, float>& variables,
			std::vector<RawState>& rawStates,
			std::vector<RawObject>& rawObjects);

		xc::XercesDOMParser* domParser = nullptr;

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
}
