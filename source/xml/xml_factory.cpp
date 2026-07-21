// xml_factory.cpp
// XML Game Engine
// author: beefviper
// date: 20-Jul-26

#include "xml/xml_factory.h"

#include <memory>
#include <iostream>

namespace xge
{
std::unique_ptr<XMLDocument> XMLFactory::createXMLDocument(XMLBackEnd backend)
{
    switch (backend)
    {
    case XMLBackEnd::tinyxml2:
        std::cout << "[XML] Creating TinyXML2 document" << std::endl;
        // return std::make_unique<TinyXML2Document>();
        break;
    case XMLBackEnd::xerces:
        std::cout << "[XML] Creating Xerces document" << std::endl;
        // return std::make_unique<XercesDocument>();
        break;
    default:
        std::cout << "[XML] Unknown backend, using TinyXML2" << std::endl;
        // return std::make_unique<TinyXML2Document>();
        break;
    }
    return nullptr;
}
}
