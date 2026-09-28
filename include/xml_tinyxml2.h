// xml_tinyxml2.h
// XML Game Engine
// author: beefviper
// date: Sept 28, 2026

#pragma once

#include "xml_document.h"

#include <tinyxml2.h>

namespace xge
{
	// The TinyXML2 XmlNode wrapper. Nothing outside this file, xml_tinyxml2.cpp,
	// and XmlDocumentFactory::create ever names a TinyXML2 type - see
	// xml_document.h.
	class TinyXml2Node : public XmlNode
	{
	public:
		explicit TinyXml2Node(const tinyxml2::XMLElement* element) noexcept;

		std::string getName() const override;
		std::string getAttribute(const std::string& name) const override;
		std::unique_ptr<XmlNode> getFirstChild() const override;
		std::unique_ptr<XmlNode> getNextSibling() const override;

	private:
		const tinyxml2::XMLElement* element;
	};

	// The TinyXML2 XmlDocument backend - well-formedness parsing only, no
	// schema validation (see xml_xerces.h for the one backend that has it -
	// TinyXML2 has no XSD validator at all).
	class TinyXml2Document : public XmlDocument
	{
	public:
		bool load(const std::string& filename) override;
		std::string getErrorMessage() const override;
		std::unique_ptr<XmlNode> getRootElement() const override;

	private:
		tinyxml2::XMLDocument document;
		std::string errorMessage;
	};
}
