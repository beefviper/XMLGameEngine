// pugixml.h
// XML Game Engine
// author: beefviper
// date: Sept 28, 2026

#pragma once

#include "xml/document.h"

#include <pugixml.hpp>

namespace xge
{
	// pugi::xml_node is a small value, safe to ask even when empty, so it is
	// held by value.
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

	// Parses only; the schema check is load/xsd.h's.
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
