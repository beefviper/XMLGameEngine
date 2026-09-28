// xml_xerces.cpp
// XML Game Engine
// author: beefviper
// date: Sept 28, 2026

#include "xml_xerces.h"

#include <xercesc/util/PlatformUtils.hpp>
#include <xercesc/util/XMLString.hpp>

#include <iostream>

namespace xge
{
	namespace
	{
		std::string xmlChToStr(const XMLCh* toTranscode)
		{
			char* transcodeChar = xc::XMLString::transcode(toTranscode);
			std::string transcodeStr = transcodeChar;
			xc::XMLString::release(&transcodeChar);
			return transcodeStr;
		}

		// RAII std::string -> XMLCh* transcode, released automatically -
		// exactly what game_xml.cpp's own StrToXMLCh used to be, just no
		// longer needed anywhere outside this file.
		class StrToXMLCh
		{
		public:
			explicit StrToXMLCh(const std::string& toTranscode)
			{
				data = xc::XMLString::transcode(toTranscode.c_str());
			}

			~StrToXMLCh()
			{
				try
				{
					xc::XMLString::release(&data);
				}
				catch (...)
				{
				}
			}

			StrToXMLCh(const StrToXMLCh&) = delete;
			StrToXMLCh& operator=(const StrToXMLCh&) = delete;
			StrToXMLCh(StrToXMLCh&& other) = delete;
			StrToXMLCh& operator=(StrToXMLCh&& other) = delete;

			XMLCh* value() const noexcept { return data; }

		private:
			XMLCh* data;
		};
	}

	XercesXmlNode::XercesXmlNode(const xc::DOMElement* element) noexcept :
		element(element)
	{
	}

	std::string XercesXmlNode::getName() const
	{
		return element ? xmlChToStr(element->getTagName()) : std::string{};
	}

	std::string XercesXmlNode::getAttribute(const std::string& name) const
	{
		if (!element) { return {}; }

		StrToXMLCh attr(name);
		return xmlChToStr(element->getAttribute(attr.value()));
	}

	std::unique_ptr<XmlNode> XercesXmlNode::getFirstChild() const
	{
		if (!element) { return nullptr; }
		const auto* child = element->getFirstElementChild();
		return child ? std::make_unique<XercesXmlNode>(child) : nullptr;
	}

	std::unique_ptr<XmlNode> XercesXmlNode::getNextSibling() const
	{
		if (!element) { return nullptr; }
		const auto* sibling = element->getNextElementSibling();
		return sibling ? std::make_unique<XercesXmlNode>(sibling) : nullptr;
	}

	void XercesXmlDocument::ParserErrorHandler::reportParseException(const xc::SAXParseException& ex)
	{
		char* msg = xc::XMLString::transcode(ex.getMessage());
		std::cout << "at line " << ex.getLineNumber() << " column " << ex.getColumnNumber() << " " << msg << '\n';
		xc::XMLString::release(&msg);
	}

	void XercesXmlDocument::ParserErrorHandler::warning(const xc::SAXParseException& ex) { reportParseException(ex); }
	void XercesXmlDocument::ParserErrorHandler::error(const xc::SAXParseException& ex) { reportParseException(ex); }
	void XercesXmlDocument::ParserErrorHandler::fatalError(const xc::SAXParseException& ex) { reportParseException(ex); }
	void XercesXmlDocument::ParserErrorHandler::resetErrors() noexcept {}

	XercesXmlDocument::XercesXmlDocument() noexcept
	{
		try
		{
			xc::XMLPlatformUtils::Initialize("en_US");
			domParser = std::make_unique<xc::XercesDOMParser>();
		}
		catch (...)
		{
		}
	}

	XercesXmlDocument::~XercesXmlDocument()
	{
		domParser.reset();
		try
		{
			xc::XMLPlatformUtils::Terminate();
		}
		catch (...)
		{
		}
	}

	bool XercesXmlDocument::load(const std::string& filename)
	{
		domParser->setErrorHandler(&parserErrorHandler);
		domParser->setValidationScheme(xc::XercesDOMParser::Val_Auto);
		domParser->setDoNamespaces(true);
		domParser->setDoSchema(true);
		domParser->setValidationConstraintFatal(true);

		domParser->parse(filename.c_str());

		const auto* documentElement = domParser->getDocument() ? domParser->getDocument()->getDocumentElement() : nullptr;

		// Only to read the schema-location attribute below, to decide which
		// of the four messages (matching game_xml.cpp's own original
		// behaviour) applies - not part of the real document tree walk,
		// which starts from getRootElement() instead.
		const XercesXmlNode rootProbe(documentElement);
		const std::string schemaLocation = rootProbe.getAttribute("xsi:noNamespaceSchemaLocation");
		const auto errorCount = domParser->getErrorCount();

		if (errorCount == 0 && !schemaLocation.empty())
		{
			std::cout << "XML file validated against the schema successfully\n\n";
			return true;
		}
		if (errorCount == 0)
		{
			std::cout << "XML file was parsed successfully\n\n";
			return true;
		}

		errorMessage = !schemaLocation.empty()
			? "XML file failed to validate against the schema"
			: "XML file failed to parse";
		return false;
	}

	std::string XercesXmlDocument::getErrorMessage() const
	{
		return errorMessage;
	}

	std::unique_ptr<XmlNode> XercesXmlDocument::getRootElement() const
	{
		if (!domParser || !domParser->getDocument()) { return nullptr; }

		const auto* documentElement = domParser->getDocument()->getDocumentElement();
		return documentElement ? std::make_unique<XercesXmlNode>(documentElement) : nullptr;
	}
}
