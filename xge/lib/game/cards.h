// cards.h
// XML Game Engine
// author: beefviper
// date: Nov 1, 2026
//
// A <deck>: the cards a game plays with, made from its suits and ranks.
// What a game file says of it (RawDeck), and the card objects it makes
// (addDeck), each with a face laid out as a bitmap from the suit's pip and
// the rank's label in a small built-in pixel font, and a back, or cut
// from a picture file of them all (a <sheet>). Read by
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
		int width{ 63 };
		int height{ 93 };
		int scale{ 1 };
		std::string faceColor{ "color.white" };
		std::string faceEdge{ "color.grey" };

		// <back>: its color and the color of its edge and pattern, and the
		// <pattern> (rows, '*' the edge's color) tiled inside its frame;
		// none, a lattice of diamonds.
		std::string backColor{ "color.darkblue" };
		std::string backEdge{ "color.white" };
		std::vector<std::string> pattern;

		// <sheet>: a picture file holding every face, a row for each suit
		// and a column for each rank in the order written, and under them a
		// row of backs; `back` is which of them, from 1. The faces are cut
		// from it and made the <face>'s size (times its scale), and the
		// suits' and ranks' rows, the face's colors and <back> go unused.
		// Empty: the faces are drawn from the rows.
		std::string sheet;
		int back{ 1 };

		// <color char="y">s: more characters the ranks' pictures use.
		std::vector<std::pair<char, std::string>> colors;

		// <suit name="...">: its color, its pip (bitmap rows, '*' in the
		// suit's color), the small pip of the corner index (<index>; none:
		// the pip), the ace's big one (<large>; none: the pip made bigger)
		// and any more characters' colors.
		struct Suit
		{
			std::string name;
			std::string color;
			std::vector<std::string> rows;
			std::vector<std::string> index;
			std::vector<std::string> large;
			std::vector<std::pair<char, std::string>> colors;
		};
		std::vector<Suit> suits;

		// <rank>s, lowest first: the name, and how its face is drawn: the
		// corner's <index> (rows; none: the name in the built-in font), the
		// <pips> laid out on a grid of rows ('*' a pip, 'v' one turned
		// round, '@' the large pip), or a <picture> (rows; with <half />
		// the top half, the bottom the same turned round). Neither: the
		// pip in the middle.
		struct Rank
		{
			std::string name;
			std::vector<std::string> index;
			std::vector<std::string> pips;
			std::vector<std::string> picture;
			bool half{ false };
			// <worth>s: what the card counts for in a pile's points, the
			// lowest first and any higher it may count for (an ace's 1 or
			// 11); none: its number in the order written.
			std::vector<float> worths;
		};
		std::vector<Rank> ranks;
	};

	// The deck's cards, appended: named after the deck and numbered from 1
	// (cards.1, cards.2, ...), suit by suit, each rank in turn, each pack in
	// turn; each with looks `face` and `back` and variables `suit` and
	// `rank`. Throws when a rank's label has a character the built-in font
	// has not, a picture a character no color names, or the face is too
	// small for its corners and pips; with a sheet, when it cannot be read,
	// does not split evenly into the suits' rows (and one of backs) and the
	// ranks' columns, or has no such back.
	void addDeck(std::vector<RawObject>& out, const RawDeck& deck);

	// The characters the rank labels may use.
	const std::string& cardFontCharacters();
}
