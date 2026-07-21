// xml_xerces.h
// XML Game Engine
// author: beefviper
// date: 20-Jul-26

#pragma once

#include "xml_interface.h"
#include <xercesc/dom/DOM.hpp>
#include <xercesc/parsers/XercesDOMParser.hpp>
#include <memory>

XERCES_CPP_NAMESPACE_BEGIN
class DOMElement;
XERCES_CPP_NAMESPACE_END

using namespace xercesc;

namespace xge
{
class XercesNode : public XMLNode
{
public:
    explicit XercesNode(DOMElement* element);

    std::string getName() const override;
    std::string getText() const override;

    std::string getAttribute(const std::string& name) const override;
    bool hasAttribute(const std::string& name) const override;

    XMLNode* getFirstChild(const std::string& name) const override;
    XMLNode* getNextSibling(const std::string& name) const override;
    std::vector<XMLNode*> getChildren(const std::string& name) const override;

private:
    DOMElement* m_element;
    mutable std::vector<std::unique_ptr<XMLNode>> m_childCache;
};

class XercesDocument : public XMLDocument
{
public:
    XercesDocument();
    ~XercesDocument();

    bool load(const std::string& filePath) override;
    bool isValid() const override;

    XMLNode* getRootElement() const override;
    std::string getErrorMessage() const override;

private:
    void initXerces();
    void terminateXerces();

    XercesDOMParser* m_parser;
    DOMDocument* m_document;
    std::unique_ptr<XMLNode> m_rootNode;
    bool m_isValid;
    bool m_xercesInitialized;
    std::string m_errorMessage;
};
}