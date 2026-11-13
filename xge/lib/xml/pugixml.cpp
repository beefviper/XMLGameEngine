// pugixml.cpp
// XML Game Engine
// author: beefviper
// date: Sept 28, 2026

#include "xml/pugixml.h"

#include <cstddef>
#include <fstream>
#include <iostream>
#include <sstream>

namespace xge
{
	PugiXmlNode::PugiXmlNode(pugi::xml_node pugiNode, const std::vector<std::size_t>* fileLines) noexcept :
		node(pugiNode),
		lines(fileLines)
	{
	}

	int PugiXmlNode::line() const
	{
		// Where the tag is in the file as it was read; -1 when not known.
		const std::ptrdiff_t offset = node.offset_debug();
		return lines && offset >= 0 ? lineAt(*lines, static_cast<std::size_t>(offset)) : 0;
	}

	std::string PugiXmlNode::getName() const
	{
		return node.name();
	}

	std::string PugiXmlNode::getAttribute(const std::string& name) const
	{
		return node.attribute(name.c_str()).value();
	}

	std::vector<std::pair<std::string, std::string>> PugiXmlNode::getAttributes() const
	{
		std::vector<std::pair<std::string, std::string>> attributes;
		for (pugi::xml_attribute attribute = node.first_attribute(); attribute; attribute = attribute.next_attribute())
		{
			attributes.emplace_back(attribute.name(), attribute.value());
		}
		return attributes;
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
		// the matching comment on XmlNode::getFirstChild() (xml/document.h).
		// A plain first_child() would stop on the text of <x>100</x>.
		pugi::xml_node child = node.first_child();
		while (child && child.type() != pugi::node_element) { child = child.next_sibling(); }
		return child ? std::make_unique<PugiXmlNode>(child, lines) : nullptr;
	}

	std::unique_ptr<XmlNode> PugiXmlNode::getNextSibling() const
	{
		pugi::xml_node sibling = node.next_sibling();
		while (sibling && sibling.type() != pugi::node_element) { sibling = sibling.next_sibling(); }
		return sibling ? std::make_unique<PugiXmlNode>(sibling, lines) : nullptr;
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

		// The lines, for each element's line(): PugiXML knows where in the
		// file a tag is, not on which line.
		std::ifstream file(filename, std::ios::binary);
		std::stringstream text;
		text << file.rdbuf();
		lines = lineStarts(text.str());

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
		return root ? std::make_unique<PugiXmlNode>(root, &lines) : nullptr;
	}
}
