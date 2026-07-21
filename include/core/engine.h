// engine.h
// XML Game Engine
// author: beefviper
// date: 20-Jul-26

#pragma once

#include "core/game.h"

namespace xge
{

class Engine
{
public:
	Engine(xge::Game& game);

	void run();

private:
	Game& game;
	bool isRunning = false;

	void input();
	void update();
	void render();
};

} // namespace xge
