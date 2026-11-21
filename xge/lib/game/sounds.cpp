// sounds.cpp
// XML Game Engine
// author: beefviper
// date: Nov 20, 2026

#include "game/sounds.h"
#include "game/game.h"

#include <algorithm>
#include <utility>

namespace xge
{
	void Sounds::request(Name name)
	{
		// Within a frame the last word wins: a loop silenced and asked for
		// again (one of two keys let go, the other still down) plays on.
		if (loops(name))
		{
			looped.insert(name);
			silenced.erase(std::remove(silenced.begin(), silenced.end(), name), silenced.end());
			return;
		}

		if (std::find(requested.begin(), requested.end(), name) == requested.end())
		{
			requested.push_back(name);
		}
	}

	std::vector<std::string> Sounds::takeRequests(void)
	{
		return std::exchange(requested, {});
	}

	const std::set<std::string>& Sounds::looping(void) const noexcept
	{
		return looped;
	}

	std::set<std::string> Sounds::playing(void) const
	{
		std::set<std::string> on = looped;
		if (game.hasState() && !game.getCurrentState().music.empty() && loops(game.getCurrentState().music))
		{
			on.insert(game.getCurrentState().music);
		}
		return on;
	}

	void Sounds::silence(Name name)
	{
		looped.erase(name);
		requested.erase(std::remove(requested.begin(), requested.end(), name), requested.end());
		if (std::find(silenced.begin(), silenced.end(), name) == silenced.end())
		{
			silenced.push_back(name);
		}
	}

	std::vector<std::string> Sounds::takeSilenced(void)
	{
		return std::exchange(silenced, {});
	}

	bool Sounds::loops(Name name) const noexcept
	{
		const std::vector<SoundDesc>& sounds = game.getSounds();
		return std::any_of(sounds.begin(), sounds.end(), [&](const SoundDesc& sound) { return sound.loop && sound.name == name; });
	}
}
