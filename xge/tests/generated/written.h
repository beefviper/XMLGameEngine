// written.h
// XML Game Engine
// author: beefviper
// date: Nov 13, 2026
//
// A game windows-cpp writes, as test_written_games.cpp plays it: whatever
// its class, the keys go in, a frame is played and its objects come out,
// which is all a written game offers (its key(), frame(), objects and
// window).

#pragma once

#include "engine/keys.h"
#include "game/object.h"

#include <memory>
#include <vector>

struct WrittenPlay
{
	virtual ~WrittenPlay() = default;
	virtual void key(xge::KeyCode key, bool pressed) = 0;
	virtual void frame() = 0;
	virtual std::vector<xge::Object>& objects() = 0;
	virtual const xge::WindowDesc& window() const = 0;
};

template <typename Game>
struct WrittenAs : WrittenPlay
{
	Game game;
	void key(xge::KeyCode key, bool pressed) override { game.key(key, pressed); }
	void frame() override { game.frame(); }
	std::vector<xge::Object>& objects() override { return game.objects; }
	const xge::WindowDesc& window() const override { return game.window; }
};

template <typename Game>
std::unique_ptr<WrittenPlay> written()
{
	return std::make_unique<WrittenAs<Game>>();
}

struct WrittenGame
{
	const char* file;
	std::unique_ptr<WrittenPlay> (*make)();
};
