// xml_tinyxml2.cpp
// XML Game Engine
// author: beefviper
// date: 20-Jul-26

#include "xml/xml_tinyxml2.h"
#include <iostream>

namespace xge
{
TinyXML2Node::TinyXML2Node(tinyxml2::XMLElement* element)
    : m_element(element)
{
}

std::string TinyXML2Node::getName() const
{
    if (!m_element)
        return "";
    return m_element->Name();
}

std::string TinyXML2Node::getText() const
{
    if (!m_element)
        return "";

    const char* text = m_element->GetText();
    return text ? text : "";
}

std::string TinyXML2Node::getAttribute(const std::string& name) const
{
    if (!m_element)
        return "";

    const char* attr = m_element->Attribute(name.c_str());
    return attr ? attr : "";
}

bool TinyXML2Node::hasAttribute(const std::string& name) const
{
    if (!m_element)
        return false;

    return m_element->Attribute(name.c_str()) != nullptr;
}

XMLNode* TinyXML2Node::getFirstChild(const std::string& name) const
{
    if (!m_element)
        return nullptr;

    tinyxml2::XMLElement* child = m_element->FirstChildElement(name.c_str());
    if (!child)
        return nullptr;

    auto nodePtr = std::make_unique<TinyXML2Node>(child);
    XMLNode* result = nodePtr.get();
    m_childCache.push_back(std::move(nodePtr));

    return result;
}

XMLNode* TinyXML2Node::getNextSibling(const std::string& name) const
{
    if (!m_element)
        return nullptr;

    tinyxml2::XMLElement* sibling = m_element->NextSiblingElement(name.c_str());
    if (!sibling)
        return nullptr;

    auto nodePtr = std::make_unique<TinyXML2Node>(sibling);
    XMLNode* result = nodePtr.get();
    m_childCache.push_back(std::move(nodePtr));

    return result;
}

std::vector<XMLNode*> TinyXML2Node::getChildren(const std::string& name) const
{
    std::vector<XMLNode*> result;

    if (!m_element)
        return result;

    for (tinyxml2::XMLElement* child = m_element->FirstChildElement(name.c_str());
        child;
        child = child->NextSiblingElement(name.c_str()))
    {
        auto nodePtr = std::make_unique<TinyXML2Node>(child);
        XMLNode* ptr = nodePtr.get();
        m_childCache.push_back(std::move(nodePtr));
        result.push_back(ptr);
    }

    return result;
}

TinyXML2Document::TinyXML2Document()
    : m_isValid(false)
    , m_errorMessage("")
{
}

TinyXML2Document::~TinyXML2Document() = default;

bool TinyXML2Document::load(const std::string& filePath)
{
    std::cout << "[TinyXML2] Loading file: " << filePath << std::endl;

    tinyxml2::XMLError error = m_document.LoadFile(filePath.c_str());
    if (error != tinyxml2::XML_SUCCESS)
    {
        m_errorMessage = std::string("Failed to load file: ") + m_document.ErrorStr();
        std::cerr << "[TinyXML2] ERROR: " << m_errorMessage << std::endl;
        m_isValid = false;
        return false;
    }

    tinyxml2::XMLElement* rootElement = m_document.RootElement();
    if (!rootElement)
    {
        m_errorMessage = "No root element found";
        std::cerr << "[TinyXML2] ERROR: " << m_errorMessage << std::endl;
        m_isValid = false;
        return false;
    }

    m_rootNode = std::make_unique<TinyXML2Node>(rootElement);
    m_isValid = true;

    std::cout << "[TinyXML2] File loaded successfully, root element: " << rootElement->Name() << std::endl;
    return true;
}

bool TinyXML2Document::isValid() const
{
    return m_isValid;
}

XMLNode* TinyXML2Document::getRootElement() const
{
    return m_rootNode.get();
}

std::string TinyXML2Document::getErrorMessage() const
{
    return m_errorMessage;
}
}