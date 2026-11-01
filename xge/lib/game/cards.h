// cards.h
// XML Game Engine
// author: beefviper
// date: Nov 1, 2026
//
// A <deck>: the cards a game plays with, made from its suits and ranks.
// What a game file says of it (RawDeck), and the card objects it makes
// (addDeck), each with a face laid out as a bitmap from the suit's pip and
// the rank's label in a small built-in pixel font, and a back. Read by
// load/xml, written out by a generated program; both make the cards here.

#pragma once

#include "game/object.h"

#include <string>
#include <utility>
#include <vector>

namespace xge
{
	struct RawDeck
	{
		std::string name;
		std::string pile;    // pile=: where the cards start, shuffled
		int copies{ 1 };     // <copies>: packs shuffled together

		// <face>: its size in the bitmap's pixels, the scale, the color of
		// the face and of its edge.
		int width{ 17 };
		int height{ 24 };
		int scale{ 4 };
		std::string faceColor{ "color.white" };
		std::string faceEdge{ "color.grey" };

		// <back>: its color and the color of its edge and pattern.
		std::string backColor{ "color.darkblue" };
		std::string backEdge{ "color.white" };

		// <suit name="...">: its color, its pip (bitmap rows, '*' in the
		// suit's color) and any more characters' colors.
		struct Suit
		{
			std::string name;
			std::string color;
			std::vector<std::string> rows;
			std::vector<std::pair<char, std::string>> colors;
		};
		std::vector<Suit> suits;

		// <rank>s: the labels, lowest first.
		std::vector<std::string> ranks;
	};

	// The deck's cards, appended: named after the deck and numbered from 1
	// (cards.1, cards.2, ...), suit by suit, each rank in turn, each pack in
	// turn; each with looks `face` and `back` and variables `suit` and
	// `rank`. Throws when a rank's label has a character the built-in font
	// has not, or the face is too small for its pips.
	void addDeck(std::vector<RawObject>& out, const RawDeck& deck);

	// The characters the rank labels may use.
	const std::string& cardFontCharacters();
}
