// xml_tinyxml2.h
// XML Game Engine
// author: beefviper
// date: 20-Jul-26

#pragma once

#include "xml_interface.h"
#include <tinyxml2.h>
#include <memory>

namespace xge
{
class TinyXML2Node : public XMLNode
{
public:
    explicit TinyXML2Node(tinyxml2::XMLElement* element);

    std::string getName() const override;
    std::string getText() const override;

    std::string getAttribute(const std::string& name) const override;
    bool hasAttribute(const std::string& name) const override;

    XMLNode* getFirstChild(const std::string& name) const override;
    XMLNode* getNextSibling(const std::string& name) const override;
    std::vector<XMLNode*> getChildren(const std::string& name) const override;

private:
    tinyxml2::XMLElement* m_element;
    mutable std::vector<std::unique_ptr<XMLNode>> m_childCache;
};

class TinyXML2Document : public XMLDocument
{
public:
    TinyXML2Document();
    ~TinyXML2Document();

    bool load(const std::string& filePath) override;
    bool isValid() const override;

    XMLNode* getRootElement() const override;
    std::string getErrorMessage() const override;

private:
    tinyxml2::XMLDocument m_document;
    std::unique_ptr<XMLNode> m_rootNode;
    bool m_isValid;
    std::string m_errorMessage;
};
}
