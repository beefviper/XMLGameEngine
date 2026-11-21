// rapidxml.h
// XML Game Engine
// author: beefviper
// date: Sept 28, 2026

#pragma once

#include "xml/document.h"

#include <rapidxml.hpp>

namespace xge
{
	class RapidXmlNode : public XmlNode
	{
	public:
		// `file`: the buffer it was parsed from, and where each of its lines
		// starts (lineStarts); either may be null.
		RapidXmlNode(const rapidxml::xml_node<>* node, const char* file, const std::vector<std::size_t>* lines) noexcept;

		std::string getName() const override;
		std::string getAttribute(const std::string& name) const override;
		std::vector<std::pair<std::string, std::string>> getAttributes() const override;
		std::string getText() const override;
		std::unique_ptr<XmlNode> getFirstChild() const override;
		std::unique_ptr<XmlNode> getNextSibling() const override;
		int line() const override;

	private:
		const rapidxml::xml_node<>* node;
		const char* file;
		const std::vector<std::size_t>* lines;
	};

	// Parses only; the schema check is load/xsd.h's. RapidXML parses in
	// place, so every name and value points into fileContent, which lives
	// as long as the document.
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
		std::vector<std::size_t> lines;
	};
}
