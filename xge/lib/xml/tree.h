// tree.h
// XML Game Engine
// author: beefviper
// date: Nov 19, 2026

#pragma once

#include "xml/document.h"

#include <cstddef>
#include <memory>

namespace xge
{
	// An XML element held as plain C++ data: what a generated program builds
	// its game into (games/generators/engine-cpp/tree.xsl), so it reads the
	// game with load/xml.cpp and no XML library. Every part is a pointer to
	// text or to an array that outlives the tree, as the static arrays of a
	// generated program do.
	struct TreeAttribute
	{
		const char* name;
		const char* value;
	};

	struct TreeElement
	{
		const char* name;
		const TreeAttribute* attributes;
		std::size_t attributeCount;
		const char* text;
		const TreeElement* children;
		std::size_t childCount;
		int line;
	};

	// The tree under `root` read as an XmlNode, as load/xml.cpp reads a file.
	std::unique_ptr<XmlNode> viewOfTree(const TreeElement& root);
}
