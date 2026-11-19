// xml.h
// XML Game Engine
// author: beefviper
// date: Sept 18, 2020

#pragma once

#include "picture/image.h"

#include "game/data.h"
#include "game/object.h"
#include "audio/sound.h"
#include "game/states.h"
#include "xml/document.h"

#include <string>
#include <utility>
#include <vector>

namespace xge
{
	// The game under `file`, its root <game> element, as Game's raw data:
	// the values, sprites, sounds and commands as the file says them, none
	// of it worked out yet (load/expr's job). The element has the shape
	// the schema says: load/file.cpp checks a file before it gets here, and
	// a generated program's tree was checked when it was made. Read through
	// XmlNode only (xml/document.h), never a particular XML library. A tag
	// the file gets wrong in a way no schema can say (two states of one
	// name, a char= on no tile) throws, saying where.
	GameData readGame(const XmlNode& file);

	// A <demo> element: its seed, its frames, and each key pressed and let
	// go with the frame it happened in.
	RawDemo readDemo(const XmlNode& demo);

	// The text of a game file with `demo` written into it as its <demo>, in
	// place of the one it had, or as the last thing in <game>: what
	// xgecli --record does when it closes.
	std::string withDemo(const std::string& gameText, const RawDemo& demo);

	// The <demo> in that file, read with that XML library: the file's root,
	// or a child of the root (a game file --record wrote into). What
	// xgecli --keys replays. Throws when there is none or it is wrong.
	RawDemo readDemo(const std::string& filename, XmlBackend backend = XmlDocumentFactory::defaultBackend());

	// The game in that file, read with that XML library and checked against
	// the schema it names, else the engine's own (load/file.cpp), as Game is
	// made from it. GameData::validation says how it was checked: Strong when
	// Xerces checked it as it loaded, Weak when the engine's own checker did
	// (load/xsd.h).
	GameData readGame(const std::string& filename, XmlBackend backend = XmlDocumentFactory::defaultBackend());
}
