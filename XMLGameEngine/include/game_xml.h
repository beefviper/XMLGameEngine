// xml.h
// XML Game Engine
// author: beefviper
// date: Sept 18, 2020

#pragma once

#include <iostream>
#include <string>

#include <tinyxml2.h>

namespace tx = tinyxml2;

namespace xge
{
	void checkXMLResult(tx::XMLDocument& xml, tx::XMLError result);
	tx::XMLElement* getXMLRoot(tx::XMLElement* element);
	tx::XMLElement* getXMLElement(tx::XMLElement* element, std::string tag);
	tx::XMLElement* getXMLElementOptional(tx::XMLElement* element, std::string tag);
	std::string getXMLText(tx::XMLElement* element);
	std::string getXMLAttribute(tx::XMLElement* element, std::string attribute);
	std::string getXMLAttributeOptional(tx::XMLElement* element, std::string attribute);
	int getXMLAttributeInt(tx::XMLElement* element, std::string attribute);
	float getXMLAttributeFloat(tx::XMLElement* element, std::string attribute);
}