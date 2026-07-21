// xml_factory.h
// XML Game Engine
// author: beefviper
// date: 20-Jul-26

#pragma once

#include <memory>

enum class XMLBackEnd
{
	null,
	xerces,
	tinyxml2
};

class XMLFactory
{
public:
	static std::unique_ptr<XMLDocument> createXMLDocument(XMLBackEnd backend);
};
