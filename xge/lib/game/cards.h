// cards.h
// XML Game Engine
// author: beefviper
// date: Nov 1, 2026
//
// Cards: the members of a held class (a pack laid out as a group, a row
// for each suit and a column for each rank) and the holders they lie in,
// worked out once a game's objects are laid out. Called by the engine
// and by a generated program alike.

#pragma once

#include "game/object.h"
#include "picture/image.h"

#include <string>
#include <utility>
#include <vector>

namespace xge
{
	struct GameData;

	// The game's holders (<holds class>) and what they hold, worked out
	// once every object is laid out (game.cpp, before anything plays): each
	// member of a held class is a card, its suit its row and its rank its
	// column in its group (the face its first look, the back its second);
	// each holder's looks become which way up what is laid there turns, and
	// each state's <starts> where the cards begin and how many each holder
	// is dealt. Throws, naming what is wrong, when a held class has no
	// members, a member is not a cell of a group with two looks, a holder
	// names a look the members have not, or a start is one the deal cannot do.
	void settleHolders(GameData& data);
}
