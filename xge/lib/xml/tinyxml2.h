// tinyxml2.h
// XML Game Engine
// author: beefviper
// date: Sept 28, 2026

#pragma once

#include "xml/document.h"

#include <tinyxml2.h>

namespace xge
{
	class TinyXml2Node : public XmlNode
	{
	public:
		explicit TinyXml2Node(const tinyxml2::XMLElement* element) noexcept;

		std::string getName() const override;
		std::string getAttribute(const std::string& name) const override;
		std::vector<std::pair<std::string, std::string>> getAttributes() const override;
		std::string getText() const override;
		std::unique_ptr<XmlNode> getFirstChild() const override;
		std::unique_ptr<XmlNode> getNextSibling() const override;
		int line() const override;

	private:
		const tinyxml2::XMLElement* element;
	};

	// Parses only; the schema check is load/xsd.h's.
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
