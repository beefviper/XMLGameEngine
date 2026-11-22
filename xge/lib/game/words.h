// words.h
// XML Game Engine
// author: beefviper
// date: Nov 20, 2026

#pragma once

#include "game/name.h"

namespace xge::words
{
	// The names the engine itself gives: live numbers a holder, a hand or a
	// player shows (holder.cards, player.turn), made once.
	inline const Name turn{ "turn" };
	inline const Name worth{ "worth" };
	inline const Name high{ "high" };
	inline const Name cards{ "cards" };
	inline const Name held{ "held" };
	inline const Name points{ "points" };
	inline const Name deadwood{ "deadwood" };
	inline const Name tricks{ "tricks" };
	inline const Name lead{ "lead" };
	inline const Name winner{ "winner" };
	inline const Name pattern{ "pattern" };
	inline const Name strength{ "strength" };
	inline const Name odds{ "odds" };
	inline const Name calling{ "calling" };
	inline const Name called{ "called" };
	// The two a rule names its own: the one it is for and the one it meets.
	inline const Name self{ "self" };
	inline const Name other{ "other" };
}
