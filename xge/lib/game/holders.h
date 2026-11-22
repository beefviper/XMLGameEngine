// holders.h
// XML Game Engine
// author: beefviper
// date: Nov 20, 2026

#pragma once

#include "engine/keys.h"
#include "game/command.h"
#include "game/name.h"
#include "game/object.h"

#include <cstddef>
#include <optional>
#include <string>
#include <vector>

namespace xge
{
	class Game;

	// Holders of items (cards) and the hands that move them
	// (game/holders.cpp): a hand's verbs, each false when refused; moving
	// items among the holders; the deal at the start of a game; where every
	// held item is drawn. The game holds one (Game::holders).
	class Holders
	{
	public:
		explicit Holders(Game& owner) noexcept : game(owner) {}

		// A hand's verbs, each false when refused. <pick />: takes what the
		// hand points at, or from a holder with <picked> sends as that says
		// (then false: nothing held).
		bool pickItems(Object& hand);
		bool layItems(Object& hand, Name holder);
		bool dropItems(Object& hand);
		bool callValue(Object& hand, Name holder);
		bool hopHand(Object& hand, Direction direction);
		void trackHand(Object& hand, bool horizontal, float at);
		// Members of a held class, holder to holder: the top `count` of
		// `from` (or all; a group's name: every member, wherever it is)
		// into `into`, showing `sprite` if given; a holder's items at
		// random; a holder's items turned to a look of theirs. False when
		// refused (nothing to send, no such holder or look).
		bool sendHeld(Name from, Name into, int count, bool all, Name sprite);
		bool orderHeld(Name holder);
		bool turnHeld(Object& holder, Name sprite);
		// What a holder holds, the bottom first (none: no such holder); one
		// of them, by its number, sent into another (false: not there).
		std::vector<Object*> heldIn(Name holder);
		bool sendItem(int number, Name from, Name into);

		// The deal at the start of a state, and where every held item is drawn.
		void sendStarts();
		void layOutItems();
		std::vector<Object*> shownHolders();
		Object* itemAt(int number);
		void turnItem(Object& item, bool up);
		void putInHolder(Object& holder, int number);
		void refreshHolder(Object& holder);
		// A held item's variable set while it is held (a die rolled): its
		// worth follows a new `worth`, and its holder counts and scores again.
		void itemChanged(Object& item, Name variable);
		void scoreHolder(Object& holder);
		void scoreDueHolders();
		// Marks each scored holder whose `odds` nothing reads: no condition,
		// shown text or expression names them (Object::Holder::oddsRead).
		void markOddsRead();
		// `called`: a variable whose value on `below` counts as `value` (0: its own).
		static bool acceptsBy(const std::vector<HolderRule::VarRule>& rules, const Object& next, const Object* below, const std::string& called = {}, float value = 0.0f);

		// The computer's play with a hand. Seconds before it next moves or
		// plays, and how long it waits each time: half a second.
		std::optional<KeyCode> thinkHeld(const CmdThink& think, Object& hand, float seconds);
		float itemWait{};
		static constexpr float kItemPace = 0.5f;

		// For tests: the next <starts> puts these items on top of the starting
		// holder after shuffling, the first on top, before the holders are dealt.
		std::vector<int> stackedStart;

	private:
		Game& game;

		bool takeItems(Object& hand);
		bool sendPicked(Object& hand, Name from = {}, Name faces = {});
		void layHeld(Object& hand, Object& holder);
		bool isCovered(const Object& item, const Object& own);   // overlapped by a later-drawn item of another shown holder
		Object* holderNamed(Name name);
		int takeTop(Object& holder);
		void showTop(Object& holder);
		// Shows the held member that look of its two, face (the first) or back.
		void showHeld(Object& item, Name look);
		Vector2f itemPlace(const Object& holder, std::size_t index);
		bool holderTakes(Object& holder, const std::vector<int>& items, const Object* from = nullptr);
		static bool follows(const HolderRule& rule, const Object& below, const Object& next);
		static bool isWild(const HolderRule& rule, const Object& item);
		void refreshCall(Object& holder);
		float itemPoints(const Object& holder, const Object& item);
		using MeldValues = std::vector<std::vector<std::vector<std::vector<float>>>>;
		MeldValues meldValues(const std::vector<const Object*>& items, const Patterns& set);
		static bool meldFits(const std::vector<std::size_t>& group, const Patterns::Pattern& pattern, const std::vector<std::vector<std::vector<float>>>& values, bool growing);
		float meldDeadwood(const Object& holder, const std::vector<int>& numbers, int best, std::vector<std::vector<int>>* melds);
		void layOutMelds(Object& holder);
		const Object* trickWinning(const Object& holder);
		const Object* trickHolder();
		bool isTrump(const HolderRule& rule, const Object& item);
		bool beatsInTrick(const HolderRule& rule, const Object& item, const Object& best);
		float reckonTricks(const Object& own, const HolderRule& trick);
		int playToTrick(Object& trick, const Object& hand, const Object& own, bool avoid);
		bool scoresDue{ false };
		// Where each item's object is in `objects`, by its number less one
		// (looked over again when a copy taken away moved them).
		std::vector<std::size_t> itemIndex;
	};
}
