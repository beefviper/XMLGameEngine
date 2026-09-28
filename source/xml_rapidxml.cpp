// xml_rapidxml.cpp
// XML Game Engine
// author: beefviper
// date: Sept 28, 2026

#include "xml_rapidxml.h"

#include <fstream>
#include <iostream>
#include <sstream>

namespace xge
{
	RapidXmlNode::RapidXmlNode(const rapidxml::xml_node<>* node) noexcept :
		node(node)
	{
	}

	std::string RapidXmlNode::getName() const
	{
		// name()/value() never return null in RapidXML - an element with no
		// name falls back to its own static empty string - so this is safe
		// even for a default-constructed node.
		return node ? node->name() : std::string{};
	}

	std::string RapidXmlNode::getAttribute(const std::string& name) const
	{
		if (!node) { return {}; }

		const rapidxml::xml_attribute<>* attribute = node->first_attribute(name.c_str());
		return attribute ? attribute->value() : std::string{};
	}

	std::unique_ptr<XmlNode> RapidXmlNode::getFirstChild() const
	{
		if (!node) { return nullptr; }

		// No-argument call - "the first child element, whatever its tag name
		// is" - see the matching comment on XmlNode::getFirstChild()
		// (xml_document.h). RapidXML's own first_node()/next_sibling() only
		// filter by name when one is passed in, so leaving every argument at
		// its default already gives unfiltered traversal.
		const rapidxml::xml_node<>* child = node->first_node();
		return child ? std::make_unique<RapidXmlNode>(child) : nullptr;
	}

	std::unique_ptr<XmlNode> RapidXmlNode::getNextSibling() const
	{
		if (!node) { return nullptr; }

		const rapidxml::xml_node<>* sibling = node->next_sibling();
		return sibling ? std::make_unique<RapidXmlNode>(sibling) : nullptr;
	}

	bool RapidXmlDocument::load(const std::string& filename)
	{
		std::cout << "[RapidXML] Loading file: " << filename << '\n';

		std::ifstream file(filename, std::ios::binary);
		if (!file.is_open())
		{
			errorMessage = "Failed to open file: " + filename;
			std::cout << "[RapidXML] XML file failed to parse: " << errorMessage << "\n\n";
			return false;
		}

		std::stringstream buffer;
		buffer << file.rdbuf();
		fileContent = buffer.str();

		try
		{
			// RapidXML parses in place and needs a mutable, zero-terminated
			// buffer - fileContent (above) is that buffer, and it has to
			// outlive this document (see the class comment in
			// xml_rapidxml.h). std::string::data() returns a mutable char*
			// as of C++17, and a std::string's buffer is already guaranteed
			// null-terminated at data()[size()].
			document.parse<0>(fileContent.data());
		}
		catch (const rapidxml::parse_error& e)
		{
			errorMessage = e.what();
			std::cout << "[RapidXML] XML file failed to parse: " << errorMessage << "\n\n";
			return false;
		}

		std::cout << "[RapidXML] XML file was parsed successfully\n\n";
		return true;
	}

	std::string RapidXmlDocument::getErrorMessage() const
	{
		return errorMessage;
	}

	std::unique_ptr<XmlNode> RapidXmlDocument::getRootElement() const
	{
		const rapidxml::xml_node<>* root = document.first_node();
		return root ? std::make_unique<RapidXmlNode>(root) : nullptr;
	}
}
