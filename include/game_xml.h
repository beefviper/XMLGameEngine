// game_xml.h
// XML Game Engine
// author: beefviper
// date: Sept 18, 2020

#pragma once

#include "object.h"
#include "states.h"
#include "xml_document.h"

#include <map>
#include <string>
#include <vector>

namespace xge
{
	// Deserializes a game XML file into Game's raw (unevaluated) data -
	// windowDesc, variables, rawStates, rawObjects. Only ever talks to the
	// abstract XmlNode/XmlDocument interface (xml_document.h), never a
	// particular XML library directly - same separation as Engine/Window on
	// the graphics side. No longer owns any per-run state itself (the whole
	// XmlDocument lives and dies inside a single init() call), so unlike the
	// old Xerces-only version of this class, there's nothing here to
	// construct, destroy, or forbid copying/moving of.
	class game_xml
	{
	public:
		// validation is an out-param reporting how filename's contents were
		// checked against assets/xmlgameengine.xsd - Strong if backend is
		// Xerces and the file named a schema (Xerces already validated it
		// for real as part of loading - see xml_xerces.cpp), Weak if some
		// other backend loaded it and this project's own XsdLiteValidator
		// (xsd_lite.h) checked it instead, or None if the file named no
		// schema at all. See Game::printGame().
		void init(const std::string& filename, XmlBackend backend, WindowDesc& windowDesc,
			std::map<std::string, float>& variables,
			std::vector<RawState>& rawStates,
			std::vector<RawObject>& rawObjects,
			SchemaValidation& validation);

	private:
		// The first child of parent named `name`, or nullptr if parent is
		// null or none match. Schema elements have fixed names but aren't
		// otherwise guaranteed to stay in a fixed order, so this looks a
		// child up by name the same way getAttribute below looks up an
		// attribute by name, instead of chaining getFirstChild()/
		// getNextSibling() calls that assume a particular position.
		static std::unique_ptr<XmlNode> findChild(const XmlNode* parent, const std::string& name);

		// "" if node is null or the attribute isn't present. This schema
		// allows some elements to be absent entirely (e.g. an object with no
		// <collisions>), so the traversal below routinely ends up with a
		// null XmlNode it still wants to read a (missing) attribute from -
		// every attribute read goes through here rather than assuming a
		// non-null node.
		static std::string getAttribute(const XmlNode* node, const std::string& name);
	};
}
