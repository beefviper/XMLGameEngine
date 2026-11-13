// pugixml.h
// XML Game Engine
// author: beefviper
// date: Sept 28, 2026

#pragma once

#include "xml/document.h"

#include <pugixml.hpp>

namespace xge
{
	// The PugiXML XmlNode wrapper. Nothing outside this file, xml/pugixml.cpp,
	// and XmlDocumentFactory::create ever names a PugiXML type - see
	// xml/document.h. pugi::xml_node is a small value type (a pointer plus a
	// null-object pattern - name()/attribute() are always safe to call even
	// on an empty node), so this just stores one by value, unlike the raw
	// pointer the other two backends keep.
	class PugiXmlNode : public XmlNode
	{
	public:
		// `lines`: where each line of the file starts (lineStarts), or null.
		PugiXmlNode(pugi::xml_node node, const std::vector<std::size_t>* lines) noexcept;

		std::string getName() const override;
		std::string getAttribute(const std::string& name) const override;
		std::vector<std::pair<std::string, std::string>> getAttributes() const override;
		std::string getText() const override;
		std::unique_ptr<XmlNode> getFirstChild() const override;
		std::unique_ptr<XmlNode> getNextSibling() const override;
		int line() const override;

	private:
		pugi::xml_node node;
		const std::vector<std::size_t>* lines;
	};

	// The PugiXML XmlDocument backend - well-formedness parsing only, no
	// schema validation (see xml/xerces.h for the one backend that has it -
	// PugiXML has no XSD validator at all).
	class PugiXmlDocument : public XmlDocument
	{
	public:
		bool load(const std::string& filename) override;
		std::string getErrorMessage() const override;
		std::unique_ptr<XmlNode> getRootElement() const override;

	private:
		pugi::xml_document document;
		std::string errorMessage;
		std::vector<std::size_t> lines;
	};
}
