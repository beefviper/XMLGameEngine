// game_xml.h
// XML Game Engine
// author: beefviper
// date: Sept 18, 2020

#pragma once

#include "object.h"
#include "states.h"
#include "xml_document.h"

#include <string>
#include <utility>
#include <vector>

namespace xge
{
	// Deserializes a game XML file into Game's raw (unevaluated) data -
	// windowDesc, rawVariables, rawStates, rawObjects - the values, sprites and
	// commands exactly as the file says them, none of it worked out yet (that is
	// game_expr's job). Only ever talks to the abstract XmlNode/XmlDocument
	// interface (xml_document.h), never a particular XML library directly - same
	// separation as Engine/Window on the graphics side. No longer owns any
	// per-run state itself (the whole XmlDocument lives and dies inside a single
	// init() call), so there's nothing here to construct, destroy, or forbid
	// copying/moving of.
	//
	// A tag the file gets wrong (a missing <radius>, an unknown command, a
	// value with both text and a <random>) throws std::runtime_error saying
	// where, whether or not the file names a schema.
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
		//
		// rawVariables keeps the order the file declares them in, so that
		// game_expr can let one variable's value use the ones before it.
		void init(const std::string& filename, XmlBackend backend, WindowDesc& windowDesc,
			std::vector<std::pair<std::string, RawValue>>& rawVariables,
			std::vector<RawState>& rawStates,
			std::vector<RawObject>& rawObjects,
			SchemaValidation& validation);
	};
}
