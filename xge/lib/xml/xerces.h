// xerces.h
// XML Game Engine
// author: beefviper
// date: Sept 28, 2026

#pragma once

#include "xml/document.h"

#include <xercesc/dom/DOM.hpp>
#include <xercesc/parsers/XercesDOMParser.hpp>
#include <xercesc/sax/ErrorHandler.hpp>
#include <xercesc/sax/SAXParseException.hpp>
#include <xercesc/util/XMLEntityResolver.hpp>

#include <string>
#include <unordered_map>

namespace xc = xercesc;

namespace xge
{
	// The Xerces XmlNode wrapper. Nothing outside this file, xml/xerces.cpp,
	// and XmlDocumentFactory::create (the one place that constructs one)
	// ever names a Xerces type - see xml/document.h.
	// The line each element's tag was read on, which the DOM does not keep.
	using XercesLines = std::unordered_map<const xc::DOMNode*, int>;

	class XercesXmlNode : public XmlNode
	{
	public:
		// `lines`: the lines its parser noted (XercesLineParser), or null.
		explicit XercesXmlNode(const xc::DOMElement* element, const XercesLines* lines = nullptr) noexcept;

		std::string getName() const override;
		std::string getAttribute(const std::string& name) const override;
		std::vector<std::pair<std::string, std::string>> getAttributes() const override;
		std::string getText() const override;
		std::unique_ptr<XmlNode> getFirstChild() const override;
		std::unique_ptr<XmlNode> getNextSibling() const override;
		int line() const override;

	private:
		const xc::DOMElement* element;
		const XercesLines* lines;
	};

	// Xerces's DOM parser, noting the line each element is read on: where
	// its start tag ends, which for a tag on one line is its line.
	class XercesLineParser : public xc::XercesDOMParser
	{
	public:
		XercesLines lines;

		void startElement(const xc::XMLElementDecl& elemDecl, const unsigned int urlId, const XMLCh* const elemPrefix,
			const xc::RefVectorOf<xc::XMLAttr>& attrList, const XMLSize_t attrCount, const bool isEmpty, const bool isRoot) override;
	};

	// The Xerces XmlDocument backend - the only one of the four (see also
	// xml/tinyxml2.h, xml/pugixml.h, xml/rapidxml.h) that can validate against
	// xgedef.xsd by itself; the other three only check
	// well-formedness, and load/xml.cpp runs load/xsd.h's validator for them.
	// Absorbs what used to be the standalone load/xml class's own Xerces
	// lifecycle (XMLPlatformUtils::Initialize/Terminate, the DOM parser, the
	// SAX error handler) - load/xml.cpp itself no longer names a Xerces type
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

			// The first error or fatal error since the last resetErrors(), as
			// "line L column C: message"; empty when there was none.
			const std::string& firstError() const noexcept { return first; }

		private:
			std::string first;

			void reportParseException(const xc::SAXParseException& ex, bool isError);
		};

		// Finds the schema where findSchema (engine/folder.h) does when it is
		// not where the game names it.
		class SchemaResolver : public xc::XMLEntityResolver
		{
		public:
			xc::InputSource* resolveEntity(xc::XMLResourceIdentifier* resource) override;

			std::string gameFile;
		};

		std::unique_ptr<XercesLineParser> domParser;
		ParserErrorHandler parserErrorHandler;
		SchemaResolver schemaResolver;
		std::string errorMessage;
	};
}
