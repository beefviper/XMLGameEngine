// xml_factory.cpp
// XML Game Engine
// author: beefviper
// date: 20-Jul-26

#include "xml/xml_factory.h"

#include <memory>

std::unique_ptr<XMLDocument> XMLFactory::createXMLDocument(XMLBackEnd backend)
{
    return std::unique_ptr<XMLDocument>();
}
