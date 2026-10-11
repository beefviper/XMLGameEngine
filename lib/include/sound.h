// sound.h
// XML Game Engine
// author: beefviper
// date: Oct 3, 2026
//
// A game file's <sounds> as the loader reads them. What a sound is once it is
// worked out, and how it is made into samples, is synth.h, which has nothing
// of the engine in it so that a generated game can have it too.

#pragma once

#include "command.h"
#include "synth.h"

#include <string>
#include <vector>

namespace xge
{
	// --- What the XML says, before any of it is evaluated (see game_xml.cpp,
	// and game_expr.cpp, which turns these into SoundDescs).

	// One <note> or <rest> as written: <note pitch="C5" to="C6" wave="noise">0.1</note>.
	// wave, pitch and to are left as the words in the file; length is a value
	// (an expression or a value tag), in seconds.
	struct RawNote
	{
		bool rest{ false };
		std::string wave;   // "" means the sound's own
		std::string pitch;
		std::string to;     // "" means no slide
		RawValue length;
	};

	// One <sound name="..." wave="square"> as written; volume is empty text
	// when the sound gives no <volume>.
	struct RawSound
	{
		std::string name;
		std::string wave;   // "" means square
		RawValue volume;
		std::vector<RawNote> notes;
	};
}
