// xml_pugixml.cpp
// XML Game Engine
// author: beefviper
// date: Sept 28, 2026

#include "xml_pugixml.h"

#include <iostream>

namespace xge
{
	PugiXmlNode::PugiXmlNode(pugi::xml_node node) noexcept :
		node(node)
	{
	}

	std::string PugiXmlNode::getName() const
	{
		return node.name();
	}

	std::string PugiXmlNode::getAttribute(const std::string& name) const
	{
		return node.attribute(name.c_str()).value();
	}

	std::string PugiXmlNode::getText() const
	{
		std::string text;

		for (pugi::xml_node child = node.first_child(); child; child = child.next_sibling())
		{
			if (child.type() == pugi::node_pcdata || child.type() == pugi::node_cdata)
			{
				text += child.value();
			}
		}

		return text;
	}

	std::unique_ptr<XmlNode> PugiXmlNode::getFirstChild() const
	{
		// The first child that is an element, whatever its tag name is - see
		// the matching comment on XmlNode::getFirstChild() (xml_document.h).
		// A plain first_child() would stop on the text of <x>100</x>.
		pugi::xml_node child = node.first_child();
		while (child && child.type() != pugi::node_element) { child = child.next_sibling(); }
		return child ? std::make_unique<PugiXmlNode>(child) : nullptr;
	}

	std::unique_ptr<XmlNode> PugiXmlNode::getNextSibling() const
	{
		pugi::xml_node sibling = node.next_sibling();
		while (sibling && sibling.type() != pugi::node_element) { sibling = sibling.next_sibling(); }
		return sibling ? std::make_unique<PugiXmlNode>(sibling) : nullptr;
	}

	bool PugiXmlDocument::load(const std::string& filename)
	{
		std::cout << "[PugiXML] Loading file: " << filename << '\n';

		const pugi::xml_parse_result result = document.load_file(filename.c_str());
		if (!result)
		{
			errorMessage = result.description();
			std::cout << "[PugiXML] XML file failed to parse: " << errorMessage << "\n\n";
			return false;
		}

		std::cout << "[PugiXML] XML file was parsed successfully\n\n";
		return true;
	}

	std::string PugiXmlDocument::getErrorMessage() const
	{
		return errorMessage;
	}

	std::unique_ptr<XmlNode> PugiXmlDocument::getRootElement() const
	{
		pugi::xml_node root = document.document_element();
		return root ? std::make_unique<PugiXmlNode>(root) : nullptr;
	}
}
