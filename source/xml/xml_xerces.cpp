// xml_xerces.cpp
// XML Game Engine
// author: beefviper
// date: 20-Jul-26

#include "xml/xml_xerces.h"

#include <xercesc/util/PlatformUtils.hpp>
#include <xercesc/util/XMLString.hpp>
#include <xercesc/dom/DOM.hpp>
#include <xercesc/parsers/XercesDOMParser.hpp>

#include <iostream>

namespace xge
{
static std::string xmlChToString(const XMLCh* ch)
{
    if (!ch)
        return "";

    char* cstr = xc::XMLString::transcode(ch);
    std::string result(cstr);
    xc::XMLString::release(&cstr);
    return result;
}

XercesNode::XercesNode(xc::DOMElement* element)
    : m_element(element)
{
}

std::string XercesNode::getName() const
{
    if (!m_element)
        return "";
    return xmlChToString(m_element->getTagName());
}

std::string XercesNode::getText() const
{
    if (!m_element)
        return "";

    xc::DOMNode* node = m_element->getFirstChild();
    while (node)
    {
        if (node->getNodeType() == xc::DOMNode::TEXT_NODE)
        {
            return xmlChToString(node->getNodeValue());
        }
        node = node->getNextSibling();
    }

    return "";
}

std::string XercesNode::getAttribute(const std::string& name) const
{
    if (!m_element)
        return "";

    XMLCh* xmlName = xc::XMLString::transcode(name.c_str());
    const XMLCh* value = m_element->getAttribute(xmlName);
    std::string result = xmlChToString(value);
    xc::XMLString::release(&xmlName);

    return result;
}

bool XercesNode::hasAttribute(const std::string& name) const
{
    if (!m_element)
        return false;

    XMLCh* xmlName = xc::XMLString::transcode(name.c_str());
    bool result = m_element->hasAttribute(xmlName);
    xc::XMLString::release(&xmlName);

    return result;
}

XMLNode* XercesNode::getFirstChild(const std::string& name) const
{
    if (!m_element)
        return nullptr;

    XMLCh* xmlName = xc::XMLString::transcode(name.c_str());
    xc::DOMNodeList* nodeList = m_element->getElementsByTagName(xmlName);
    xc::XMLString::release(&xmlName);

    if (nodeList && nodeList->getLength() > 0)
    {
        xc::DOMElement * child = dynamic_cast<xc::DOMElement*>(nodeList->item(0));
        if (child && child->getParentNode() == m_element)
        {
            auto nodePtr = std::make_unique<XercesNode>(child);
            XMLNode* result = nodePtr.get();
            m_childCache.push_back(std::move(nodePtr));
            return result;
        }
    }

    return nullptr;
}

XMLNode* XercesNode::getNextSibling(const std::string& name) const
{
    if (!m_element)
        return nullptr;

    XMLCh* xmlName = xc::XMLString::transcode(name.c_str());
    xc::DOMElement* sibling = dynamic_cast<xc::DOMElement*>(m_element->getNextSibling());

    while (sibling)
    {
        if (xc::XMLString::equals(sibling->getTagName(), xmlName))
        {
            xc::XMLString::release(&xmlName);
            auto nodePtr = std::make_unique<XercesNode>(sibling);
            XMLNode* result = nodePtr.get();
            m_childCache.push_back(std::move(nodePtr));
            return result;
        }
        sibling = dynamic_cast<xc::DOMElement*>(sibling->getNextSibling());
    }

    xc::XMLString::release(&xmlName);
    return nullptr;
}

std::vector<XMLNode*> XercesNode::getChildren(const std::string& name) const
{
    std::vector<XMLNode*> result;

    if (!m_element)
        return result;

    XMLCh* xmlName = xc::XMLString::transcode(name.c_str());
    xc::DOMNodeList* nodeList = m_element->getElementsByTagName(xmlName);
    xc::XMLString::release(&xmlName);

    if (!nodeList)
        return result;

    for (XMLSize_t i = 0; i < nodeList->getLength(); ++i)
    {
        xc::DOMElement* child = dynamic_cast<xc::DOMElement*>(nodeList->item(i));
        if (child && child->getParentNode() == m_element)
        {
            auto nodePtr = std::make_unique<XercesNode>(child);
            XMLNode* ptr = nodePtr.get();
            m_childCache.push_back(std::move(nodePtr));
            result.push_back(ptr);
        }
    }

    return result;
}

// XercesDocument implementation
XercesDocument::XercesDocument()
    : m_parser(nullptr)
    , m_document(nullptr)
    , m_isValid(false)
    , m_xercesInitialized(false)
    , m_errorMessage("")
{
    initXerces();
}

XercesDocument::~XercesDocument()
{
    if (m_parser)
    {
        delete m_parser;
        m_parser = nullptr;
    }
    terminateXerces();
}

void XercesDocument::initXerces()
{
    try
    {
        xc::XMLPlatformUtils::Initialize();
        m_xercesInitialized = true;
        std::cout << "[Xerces] Xerces initialized successfully" << std::endl;
    }
    catch (const xc::XMLException& e)
    {
        m_errorMessage = std::string("Failed to initialize Xerces: ") + xmlChToString(e.getMessage());
        std::cerr << "[Xerces] ERROR: " << m_errorMessage << std::endl;
        m_xercesInitialized = false;
    }
    catch (const std::exception& e)
    {
        m_errorMessage = std::string("Standard exception during Xerces init: ") + e.what();
        std::cerr << "[Xerces] ERROR: " << m_errorMessage << std::endl;
        m_xercesInitialized = false;
    }
}

void XercesDocument::terminateXerces()
{
    try
    {
        if (m_xercesInitialized)
        {
            xc::XMLPlatformUtils::Terminate();
            m_xercesInitialized = false;
        }
    }
    catch (const xc::XMLException& e)
    {
        std::cerr << "[Xerces] ERROR during termination: " << xmlChToString(e.getMessage()) << std::endl;
    }
}

bool XercesDocument::load(const std::string& filePath)
{
    if (!m_xercesInitialized)
    {
        m_errorMessage = "Xerces not initialized";
        std::cerr << "[Xerces] ERROR: " << m_errorMessage << std::endl;
        return false;
    }

    std::cout << "[Xerces] Loading file: " << filePath << std::endl;

    try
    {
        m_parser = new xc::XercesDOMParser();
        m_parser->setValidationScheme(xc::XercesDOMParser::Val_Never);
        m_parser->setDoNamespaces(false);
        m_parser->setDoSchema(false);

        m_parser->parse(filePath.c_str());

        m_document = m_parser->getDocument();
        if (!m_document)
        {
            m_errorMessage = "Failed to get document from parser";
            std::cerr << "[Xerces] ERROR: " << m_errorMessage << std::endl;
            delete m_parser;
            m_parser = nullptr;
            m_isValid = false;
            return false;
        }

        xc::DOMElement* rootElement = m_document->getDocumentElement();
        if (!rootElement)
        {
            m_errorMessage = "Failed to get root element";
            std::cerr << "[Xerces] ERROR: " << m_errorMessage << std::endl;
            delete m_parser;
            m_parser = nullptr;
            m_isValid = false;
            return false;
        }

        m_rootNode = std::make_unique<XercesNode>(rootElement);
        m_isValid = true;

        std::cout << "[Xerces] File loaded successfully, root element: " << xmlChToString(rootElement->getTagName()) << std::endl;
        return true;
    }
    catch (const xc::XMLException& e)
    {
        m_errorMessage = std::string("XML Exception: ") + xmlChToString(e.getMessage());
        std::cerr << "[Xerces] ERROR: " << m_errorMessage << std::endl;
        if (m_parser)
        {
            delete m_parser;
            m_parser = nullptr;
        }
        m_isValid = false;
        return false;
    }
    catch (const xc::DOMException& e)
    {
        m_errorMessage = std::string("DOM Exception: ") + xmlChToString(e.getMessage());
        std::cerr << "[Xerces] ERROR: " << m_errorMessage << std::endl;
        if (m_parser)
        {
            delete m_parser;
            m_parser = nullptr;
        }
        m_isValid = false;
        return false;
    }
    catch (const std::exception& e)
    {
        m_errorMessage = std::string("Standard exception: ") + e.what();
        std::cerr << "[Xerces] ERROR: " << m_errorMessage << std::endl;
        if (m_parser)
        {
            delete m_parser;
            m_parser = nullptr;
        }
        m_isValid = false;
        return false;
    }
}

bool XercesDocument::isValid() const
{
    return m_isValid;
}

XMLNode* XercesDocument::getRootElement() const
{
    return m_rootNode.get();
}

std::string XercesDocument::getErrorMessage() const
{
    return m_errorMessage;
}
}
