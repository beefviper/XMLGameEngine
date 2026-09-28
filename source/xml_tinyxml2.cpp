// xml_tinyxml2.cpp
// XML Game Engine
// author: beefviper
// date: Sept 28, 2026

#include "xml_tinyxml2.h"

#include <iostream>

namespace xge
{
	TinyXml2Node::TinyXml2Node(const tinyxml2::XMLElement* element) noexcept :
		element(element)
	{
	}

	std::string TinyXml2Node::getName() const
	{
		return element ? element->Name() : std::string{};
	}

	std::string TinyXml2Node::getAttribute(const std::string& name) const
	{
		if (!element) { return {}; }

		const char* value = element->Attribute(name.c_str());
		return value ? value : std::string{};
	}

	std::unique_ptr<XmlNode> TinyXml2Node::getFirstChild() const
	{
		if (!element) { return nullptr; }

		// No-argument overload - "the first child element, whatever its tag
		// name is" - see the matching comment on XmlNode::getFirstChild()
		// (xml_document.h).
		const tinyxml2::XMLElement* child = element->FirstChildElement();
		return child ? std::make_unique<TinyXml2Node>(child) : nullptr;
	}

	std::unique_ptr<XmlNode> TinyXml2Node::getNextSibling() const
	{
		if (!element) { return nullptr; }

		const tinyxml2::XMLElement* sibling = element->NextSiblingElement();
		return sibling ? std::make_unique<TinyXml2Node>(sibling) : nullptr;
	}

	bool TinyXml2Document::load(const std::string& filename)
	{
		std::cout << "[TinyXML2] Loading file: " << filename << '\n';

		const tinyxml2::XMLError error = document.LoadFile(filename.c_str());
		if (error != tinyxml2::XML_SUCCESS)
		{
			errorMessage = document.ErrorStr();
			std::cout << "[TinyXML2] XML file failed to parse: " << errorMessage << "\n\n";
			return false;
		}

		std::cout << "[TinyXML2] XML file was parsed successfully\n\n";
		return true;
	}

	std::string TinyXml2Document::getErrorMessage() const
	{
		return errorMessage;
	}

	std::unique_ptr<XmlNode> TinyXml2Document::getRootElement() const
	{
		const tinyxml2::XMLElement* root = document.RootElement();
		return root ? std::make_unique<TinyXml2Node>(root) : nullptr;
	}
}
