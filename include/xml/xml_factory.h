// xml_factory.h
// XML Game Engine
// author: beefviper
// date: 20-Jul-26

#pragma once

#include "xml/xml_interface.h"

#include <memory>

namespace xge
{
enum class XMLBackEnd;

class XMLFactory
{
public:
	static std::unique_ptr<XMLDocument> createXMLDocument(XMLBackEnd backend);
};

enum class XMLBackEnd
{
    null,
    xerces,
    tinyxml2
};
}
