// xml_rapidxml.h
// XML Game Engine
// author: beefviper
// date: Sept 28, 2026

#pragma once

#include "xml_document.h"

#include <rapidxml.hpp>

namespace xge
{
	// The RapidXML XmlNode wrapper. Nothing outside this file, xml_rapidxml.cpp,
	// and XmlDocumentFactory::create ever names a RapidXML type - see
	// xml_document.h.
	class RapidXmlNode : public XmlNode
	{
	public:
		explicit RapidXmlNode(const rapidxml::xml_node<>* node) noexcept;

		std::string getName() const override;
		std::string getAttribute(const std::string& name) const override;
		std::unique_ptr<XmlNode> getFirstChild() const override;
		std::unique_ptr<XmlNode> getNextSibling() const override;

	private:
		const rapidxml::xml_node<>* node;
	};

	// The RapidXML XmlDocument backend - well-formedness parsing only, no
	// schema validation (see xml_xerces.h for the one backend that has it -
	// RapidXML has no XSD validator at all).
	//
	// RapidXML parses in place: xml_document<>::parse() takes a mutable,
	// zero-terminated buffer, and every node/attribute name and value it
	// hands back just points into that same buffer - so the buffer has to
	// stay alive as long as the document does. fileContent below is that
	// buffer: read once in load() and never touched again afterwards.
	class RapidXmlDocument : public XmlDocument
	{
	public:
		bool load(const std::string& filename) override;
		std::string getErrorMessage() const override;
		std::unique_ptr<XmlNode> getRootElement() const override;

	private:
		rapidxml::xml_document<> document;
		std::string fileContent;
		std::string errorMessage;
	};
}
