// xml_interface.h
// XML Game Engine
// author: beefviper
// date: 20-Jul-26

#pragma once

#include <string>
#include <vector>

namespace xge
{
class XMLNode
{
public:
    virtual ~XMLNode() = default;

    virtual std::string getName() const = 0;
    virtual std::string getText() const = 0;

    virtual std::string getAttribute(const std::string& name) const = 0;
    virtual bool hasAttribute(const std::string& name) const = 0;

    virtual XMLNode* getFirstChild(const std::string& name) const = 0;
    virtual XMLNode* getNextSibling(const std::string& name) const = 0;
    virtual std::vector<XMLNode*> getChildren(const std::string& name) const = 0;
};

class XMLDocument
{
public:
    virtual ~XMLDocument() = default;

    virtual bool load(const std::string& filePath) = 0;
    virtual bool isValid() const = 0;

    virtual XMLNode* getRootElement() const = 0;

    virtual std::string getErrorMessage() const = 0;
};
}
