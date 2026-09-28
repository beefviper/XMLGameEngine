// xml_xerces.h
// XML Game Engine
// author: beefviper
// date: Sept 28, 2026

#pragma once

#include "xml_document.h"

#include <xercesc/dom/DOM.hpp>
#include <xercesc/parsers/XercesDOMParser.hpp>
#include <xercesc/sax/ErrorHandler.hpp>
#include <xercesc/sax/SAXParseException.hpp>

namespace xc = xercesc;

namespace xge
{
	// The Xerces XmlNode wrapper. Nothing outside this file, xml_xerces.cpp,
	// and XmlDocumentFactory::create (the one place that constructs one)
	// ever names a Xerces type - see xml_document.h.
	class XercesXmlNode : public XmlNode
	{
	public:
		explicit XercesXmlNode(const xc::DOMElement* element) noexcept;

		std::string getName() const override;
		std::string getAttribute(const std::string& name) const override;
		std::unique_ptr<XmlNode> getFirstChild() const override;
		std::unique_ptr<XmlNode> getNextSibling() const override;

	private:
		const xc::DOMElement* element;
	};

	// The Xerces XmlDocument backend - the only one of the three (see also
	// xml_tinyxml2.h, xml_pugixml.h) that can validate against
	// assets/xmlgameengine.xsd, since neither TinyXML2 nor PugiXML has a
	// schema validator at all; those two only ever check well-formedness.
	// Absorbs what used to be the standalone game_xml class's own Xerces
	// lifecycle (XMLPlatformUtils::Initialize/Terminate, the DOM parser, the
	// SAX error handler) - game_xml.cpp itself no longer names a Xerces type
	// anywhere, same as Engine no longer naming an SFML/Raylib/SDL2 type.
	class XercesXmlDocument : public XmlDocument
	{
	public:
		XercesXmlDocument() noexcept;
		~XercesXmlDocument() override;

		XercesXmlDocument(const XercesXmlDocument&) = delete;
		XercesXmlDocument& operator=(const XercesXmlDocument&) = delete;
		XercesXmlDocument(XercesXmlDocument&& other) = delete;
		XercesXmlDocument& operator=(XercesXmlDocument&& other) = delete;

		bool load(const std::string& filename) override;
		std::string getErrorMessage() const override;
		std::unique_ptr<XmlNode> getRootElement() const override;

	private:
		class ParserErrorHandler : public xc::ErrorHandler
		{
		public:
			void warning(const xc::SAXParseException& ex) override;
			void error(const xc::SAXParseException& ex) override;
			void fatalError(const xc::SAXParseException& ex) override;
			void resetErrors() noexcept override;

		private:
			void reportParseException(const xc::SAXParseException& ex);
		};

		std::unique_ptr<xc::XercesDOMParser> domParser;
		ParserErrorHandler parserErrorHandler;
		std::string errorMessage;
	};
}
