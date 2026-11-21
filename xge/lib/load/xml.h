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
	// The game under its root <game> element, as written: none of it worked
	// out yet (load/expr's job). The tree already passed the schema; what no
	// schema can say (two states of one name, a char= on no tile) throws,
	// saying where.
	GameData readGame(const XmlNode& file);

	// A <demo> element: its seed, its frames, and each key pressed and let
	// go with the frame it happened in.
	WrittenDemo readDemo(const XmlNode& demo);

	// The text of a game file with `demo` written into it as its <demo>, in
	// place of the one it had, or as the last thing in <game>: what
	// xgecli --record does when it closes.
	std::string withDemo(const std::string& gameText, const WrittenDemo& demo);

	// The <demo> in that file, read with that XML library: the file's root,
	// or a child of the root (a game file --record wrote into). What
	// xgecli --keys replays. Throws when there is none or it is wrong.
	WrittenDemo readDemo(const std::string& filename, XmlBackend backend = XmlDocumentFactory::defaultBackend());

	// The game in that file, read with that XML library and checked against
	// the schema it names, else the engine's own (load/file.cpp), as Game is
	// made from it. GameData::validation says how it was checked: Strong when
	// Xerces checked it as it loaded, Weak when the engine's own checker did
	// (load/xsd.h).
	GameData readGame(const std::string& filename, XmlBackend backend = XmlDocumentFactory::defaultBackend());
}
