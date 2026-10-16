// game.cpp
// XML Game Engine
// author: beefviper
// date: Sept 18, 2020

#include "game.h"

#include "command_executor.h"
#include "geometry.h"
#include "scenes.h"
#include "spelling.h"
#include "verbs.h"

#include <algorithm>
#include <cstddef>
#include <iomanip>
#include <limits>
#include <regex>
#include <sstream>
#include <stdexcept>
#include <utility>
#include <variant>

namespace xge
{
	Game::Game(GameData data)
	{
		// What states and scenes own, among the game's objects and named in full.
		layOutScenes(data);
		windowDesc = std::move(data.window);
		validation = std::move(data.validation);
		rawVariables = std::move(data.variables);
		rawStates = std::move(data.states);
		rawObjects = std::move(data.objects);
		rawSounds = std::move(data.sounds);
		rawPaths = std::move(data.paths);
		const std::vector<std::string> keeps = std::move(data.keeps);

		expr.init(windowDesc, rawVariables, variables, rawStates, states, rawObjects, objects, rawSounds, sounds, rawPaths, paths);
		expr.finishLoading();
		keep(keeps);
		refreshAllBoundTexts();

		for (const auto& state : states)
		{
			stateTimers[state.name] = state.timers;
		}
		startScenes();
		// Every object's own visual is built later, by Engine, once a real
		// Window (and therefore a real backend to build against) exists -
		// see Window::init() in window.h.
	}

	void Game::resolveSizeDependentPositions(void)
	{
		// What every backend-measured object now measures, in the expressions'
		// own name.width / name.height variables.
		for (const auto& object : objects)
		{
			if (object.sizeKnown && game_expr::sizeNeedsBackend(object.shapeKind))
			{
				expr.setObjectSize(object.name, object.size);
			}
		}

		for (auto& object : objects)
		{
			if (!object.positionUsesSize)
			{
				continue;
			}

			// Every object this position uses needs a measured size before it
			// can be worked out, and only needs doing again if one has changed.
			std::vector<Vector2f> sizes;
			bool allMeasured = true;
			for (const auto& dependency : object.sizeDependencies)
			{
				const auto measured = std::find_if(objects.begin(), objects.end(),
					[&](const Object& other) { return other.name == dependency && other.sizeKnown; });
				if (measured == objects.end())
				{
					allMeasured = false;
					break;
				}
				sizes.push_back(measured->size);
			}

			if (!allMeasured || (object.positionResolved && sizes == object.positionSizesUsed))
			{
				continue;
			}

			const auto rawObject = std::find_if(rawObjects.begin(), rawObjects.end(),
				[&](const RawObject& raw) { return raw.name == object.name; });
			if (rawObject == rawObjects.end())
			{
				continue;
			}

			const std::string where = "object '" + rawObject->name + "'";
			object.positionOriginal.x = expr.evaluate(rawObject->rawPosition.x, where);
			object.positionOriginal.y = expr.evaluate(rawObject->rawPosition.y, where);
			object.positionOriginal = object.positionOriginal + object.cellOffset;
			object.position = object.positionOriginal;
			object.positionSizesUsed = sizes;
			object.positionResolved = true;
		}
	}

	void Game::updateObjects(void)
	{
		playOneFrame(getCurrentObjects(), windowDesc, *this);
	}

	void Game::printGame(void)
	{
		std::cout << "schema validation: " << validation << "\n\n";

		std::cout << windowDesc << '\n';

		for (auto& variable : variables)
		{
			std::cout << "variable: name=" << variable.first << ", value=" << variable.second << '\n';
		}
		std::cout << '\n';

		for (auto& rawObject : rawObjects)
		{
			std::cout << rawObject << '\n';
		}

		for (auto& object : objects)
		{
			std::cout << object << '\n';
		}

		for (auto& state : states)
		{
			std::cout << state << '\n';
		}

		for (const auto& sound : sounds)
		{
			std::cout << sound << '\n';
		}
	}

	const std::vector<SoundDesc>& Game::getSounds(void) const noexcept
	{
		return sounds;
	}

	void Game::requestSound(const std::string& name)
	{
		// Within a frame the last word wins: a loop silenced and asked for
		// again (one of two keys let go, the other still down) plays on.
		if (soundLoops(name))
		{
			loops.insert(name);
			silenceRequests.erase(std::remove(silenceRequests.begin(), silenceRequests.end(), name), silenceRequests.end());
			return;
		}

		if (std::find(soundRequests.begin(), soundRequests.end(), name) == soundRequests.end())
		{
			soundRequests.push_back(name);
		}
	}

	std::vector<std::string> Game::takeSoundRequests(void)
	{
		return std::exchange(soundRequests, {});
	}

	const std::set<std::string>& Game::loopingSounds(void) const noexcept
	{
		return loops;
	}

	void Game::silenceSound(const std::string& name)
	{
		loops.erase(name);
		soundRequests.erase(std::remove(soundRequests.begin(), soundRequests.end(), name), soundRequests.end());
		if (std::find(silenceRequests.begin(), silenceRequests.end(), name) == silenceRequests.end())
		{
			silenceRequests.push_back(name);
		}
	}

	std::vector<std::string> Game::takeSilenceRequests(void)
	{
		return std::exchange(silenceRequests, {});
	}

	bool Game::soundLoops(const std::string& name) const noexcept
	{
		return std::any_of(sounds.begin(), sounds.end(), [&](const SoundDesc& sound) { return sound.loop && sound.name == name; });
	}

	WindowDesc& Game::getWindowDesc(void) noexcept
	{
		return windowDesc;
	}

	const Object* Game::nearestInPlay(const Object& from, const std::string& name)
	{
		const auto middleOf = [](const Object& object) { return object.position + object.size * 0.5f; };
		const Vector2f here = middleOf(from);

		const Object* nearest = nullptr;
		float nearestDistance = 0.0f;
		for (const auto& other : objects)
		{
			const bool named = other.name == name || other.inGroup(name);
			if (&other == &from || !named || !other.isVisible || !isShown(other)) { continue; }

			const Vector2f apart = middleOf(other) - here;
			const float distance = std::hypot(apart.x, apart.y);
			if (!nearest || distance < nearestDistance)
			{
				nearest = &other;
				nearestDistance = distance;
			}
		}
		return nearest;
	}

	bool Game::isShown(const Object& object) noexcept
	{
		return object.isVisible && !currentState.empty() && runsIn(object, currentState.top());
	}

	bool Game::runsIn(const Object& object, const State& state) const
	{
		if (!object.ownerState.empty() && object.ownerState == state.name)
		{
			if (object.ownerScene.empty()) { return true; }
			const auto current = currentScenes.find(state.name);
			if (current != currentScenes.end() && current->second == object.ownerScene) { return true; }
		}

		return std::any_of(state.show.begin(), state.show.end(), [&](const std::string& shown) { return shown == object.name || object.inGroup(shown); });
	}

	bool Game::isStill(const Object& object) noexcept
	{
		if (currentState.empty()) { return false; }
		const State& top = currentState.top();
		if (top.showStates.empty() || !object.isVisible || runsIn(object, top)) { return false; }

		for (const std::string& name : top.showStates)
		{
			const auto state = std::find_if(states.begin(), states.end(), [&](const State& candidate) { return candidate.name == name; });
			if (state != states.end() && runsIn(object, *state)) { return true; }
		}
		return false;
	}

	bool Game::inScope(const Object& object) const
	{
		if (object.ownerState.empty()) { return true; }
		if (currentState.empty() || object.ownerState != currentState.top().name) { return false; }
		if (object.ownerScene.empty()) { return true; }
		const auto current = currentScenes.find(object.ownerState);
		return current != currentScenes.end() && current->second == object.ownerScene;
	}

	bool Game::isAway(const Object& object) const
	{
		if (object.ownerScene.empty()) { return false; }
		const auto current = currentScenes.find(object.ownerState);
		return current == currentScenes.end() || current->second != object.ownerScene;
	}

	std::string Game::currentScene(const std::string& state) const
	{
		const auto current = currentScenes.find(state);
		return current == currentScenes.end() ? std::string{} : current->second;
	}

	void Game::startScenes(void)
	{
		currentScenes.clear();
		sceneTimers.clear();
		for (const State& state : states)
		{
			if (!state.scenes.empty()) { currentScenes[state.name] = state.scenes.front().name; }
			for (const Scene& scene : state.scenes) { sceneTimers[scene.name] = scene.timers; }
		}
	}

	void Game::enterScene(const std::string& scene)
	{
		const std::string state = scene.substr(0, scene.find('.'));
		if (!sceneTimers.count(scene))
		{
			// The file is checked for this when it loads; this is for a caller in C++.
			throw std::out_of_range("no scene named '" + scene + "'");
		}

		std::string& current = currentScenes[state];
		if (current == scene) { return; }
		current = scene;
		++sceneChanges;

		// What is carried by something that is in every scene (the state's
		// own, or the game's) belongs to the scene it is carried into.
		for (const Object& carrier : objects)
		{
			if (carrier.carrying.empty() || !carrier.ownerScene.empty()) { continue; }
			Object* carried = tryGetObject(carrier.carrying);
			if (carried && carried->ownerState == state && !carried->ownerScene.empty()) { carried->ownerScene = scene; }
		}
	}

	std::string Game::fullName(const std::string& name) const
	{
		const auto calls = [&](const Object& object, const std::string& wanted) { return object.name == wanted || object.inGroup(wanted); };
		if (std::any_of(objects.begin(), objects.end(), [&](const Object& object) { return calls(object, name); })) { return name; }

		const std::string end = "." + name;
		const auto endsWith = [&](const std::string& text) { return text.size() > end.size() && text.compare(text.size() - end.size(), end.size(), end) == 0; };
		// The shortest path wins, as when the game loads (playing.rock, the
		// rock of every room, over playing.room1.rock); two as short: neither.
		const auto dots = [](const std::string& text) { return std::count(text.begin(), text.end(), '.'); };
		std::string found;
		bool tied = false;
		for (const Object& object : objects)
		{
			std::vector<std::string> callings{ object.name };
			if (!object.groupName.empty()) { callings.push_back(object.groupName); }
			callings.insert(callings.end(), object.outerGroups.begin(), object.outerGroups.end());
			for (const std::string& calling : callings)
			{
				if (!endsWith(calling) || calling == found) { continue; }
				if (found.empty() || dots(calling) < dots(found)) { found = calling; tied = false; }
				else if (dots(calling) == dots(found)) { tied = true; }
			}
		}
		return found.empty() || tied ? name : found;
	}

	Object& Game::getObject(const std::string& name)
	{
		Object* object = tryGetObject(name);
		if (!object)
		{
			throw std::out_of_range("no object named '" + name + "'");
		}
		return *object;
	}

	// An object's own name (aliens.3.2) finds that object; the name of a
	// <group> (aliens) finds its first member.
	Object* Game::tryGetObject(const std::string& name) noexcept
	{
		const auto exact = std::find_if(std::begin(objects), std::end(objects), [&](const Object& obj) { return obj.name == name; });
		if (exact != std::end(objects))
		{
			return &(*exact);
		}

		const auto first = std::find_if(std::begin(objects), std::end(objects), [&](const Object& obj) { return obj.inGroup(name); });
		if (first != std::end(objects))
		{
			return &(*first);
		}

		// The end of one path: robots for playing.room1.robots.
		const std::string full = fullName(name);
		return full == name ? nullptr : tryGetObject(full);
	}

	float Game::getVariable(const std::string& name)
	{
		auto result = std::find_if(std::begin(variables), std::end(variables), [&](std::pair<const std::string, float>& var) { return var.first == name; });

		if (result == variables.end())
		{
			return 0;
		}
		else
		{
			return result->second;
		}
	}

	State Game::getCurrentState(void)
	{
		return currentState.top();
	}

	std::vector<Object>& Game::getCurrentObjects(void) noexcept
	{
		return objects;
	}

	void Game::setCurrentState(const int& index)
	{
		currentState.push(states.at(static_cast<std::size_t>(index)));
		++stateChanges;
	}

	void Game::setCurrentState(const std::string& name)
	{
		pushState(name);
	}

	void Game::pushState(const std::string& name)
	{
		auto result = std::find_if(std::begin(states), std::end(states), [&](State& state) { return state.name == name; });
		if (result == std::end(states))
		{
			// The file is checked for this when it loads (see
			// game_expr::checkReferences); this is for a caller in C++.
			throw std::out_of_range("no state named '" + name + "'");
		}

		currentState.push(*result);
		++stateChanges;
	}

	void Game::popState(void) noexcept
	{
		// The state the game started in is never popped: with no state at
		// all there would be nothing to show and no keys to read.
		if (currentState.size() <= 1)
		{
			return;
		}

		currentState.pop();
		++stateChanges;
	}

	void Game::drawStartAgain(Object& object)
	{
		if (!object.startDrawsRandom)
		{
			return;
		}

		const auto raw = std::find_if(rawObjects.begin(), rawObjects.end(), [&](const RawObject& candidate) { return candidate.name == object.name; });
		if (raw == rawObjects.end())
		{
			return;
		}

		const std::string where = "object '" + raw->name + "'";
		for (const auto& [name, value] : raw->variable)
		{
			if (!value.drawsRandom()) { continue; }
			const float drawn = expr.evaluate(value, where);
			object.variable[name] = drawn;
			object.variableOriginal[name] = drawn;
			refreshBoundTexts(object.name, name, drawn);
		}

		// The position, then the velocity, the order they are drawn in when the
		// game loads (game_expr::init), so a start drawn again takes the same
		// numbers as the first did, and a generated program, which draws them
		// in that order, draws the same ones. A position waiting on a size not
		// measured yet is worked out when it is.
		if (object.positionResolved)
		{
			object.positionOriginal = Vector2f{ expr.evaluate(raw->rawPosition.x, where), expr.evaluate(raw->rawPosition.y, where) } + object.cellOffset;
		}

		object.velocityOriginal = { expr.evaluate(raw->rawVelocity.x, where), expr.evaluate(raw->rawVelocity.y, where) };
	}

	void Game::popState(const std::string& name)
	{
		if (name.empty())
		{
			popState();
			return;
		}

		auto result = std::find_if(std::begin(states), std::end(states), [&](State& state) { return state.name == name; });
		if (result == std::end(states))
		{
			throw std::out_of_range("no state named '" + name + "'");
		}

		if (!currentState.empty())
		{
			currentState.pop();
		}
		currentState.push(*result);
		++stateChanges;
	}

	unsigned long Game::stateChangeCount(void) const noexcept
	{
		return stateChanges;
	}

	const std::vector<State>& Game::getStates(void) const noexcept
	{
		return states;
	}

	void Game::setObjectParam(const std::string& name, const std::string& param, const float& value)
	{
		auto result = std::find_if(std::begin(objects), std::end(objects), [&](Object& obj) { return obj.name == name; });
		if (result != std::end(objects) && param == "velocity")
		{
			result->velocity.y = value;
		}
	}

	void Game::incrementText(const std::string& target, float amount)
	{
		changeVariable(target, amount, "inc");
	}

	void Game::decrementText(const std::string& target, float amount)
	{
		changeVariable(target, -amount, "dec");
	}

	void Game::copyVariable(const std::string& from, const std::string& into)
	{
		// Both are owner.variable, the variable after the last dot.
		const auto find = [&](const std::string& target) -> std::pair<Object*, float*>
		{
			const auto dot = target.rfind('.');
			Object* owner = dot == std::string::npos ? nullptr : tryGetObject(target.substr(0, dot));
			if (!owner) { return { nullptr, nullptr }; }
			auto variable = owner->variable.find(target.substr(dot + 1));
			return { owner, variable == owner->variable.end() ? nullptr : &variable->second };
		};

		const auto [source, value] = find(from);
		const auto [owner, variable] = find(into);
		if (!value || !variable)
		{
			std::cout << "warning: <copy variable=\"" << from << "\" into=\"" << into << "\" />: no such variable\n";
			return;
		}

		*variable = *value;
		refreshBoundTexts(owner->name, into.substr(into.rfind('.') + 1), *variable);
	}

	void Game::startTurns(const std::vector<std::string>& playerStates)
	{
		turnsState = currentState.empty() ? std::string{} : currentState.top().name;
		Turn start;
		putTurnAway(start);
		turns.assign(playerStates.size(), start);
		for (std::size_t i = 0; i < playerStates.size(); ++i)
		{
			turns[i].state = playerStates[i];
		}
		turn = 0;
		beginTurn();
	}

	void Game::passTurn(bool out)
	{
		if (turns.empty())
		{
			return;
		}
		if (out)
		{
			turns[turn].out = true;
		}

		std::size_t next = turn;
		bool anyone = false;
		for (std::size_t k = 1; k <= turns.size(); ++k)
		{
			next = (turn + k) % turns.size();
			if (!turns[next].out)
			{
				anyone = true;
				break;
			}
		}
		if (!anyone)
		{
			turns.clear(); // everyone is out: what follows <out /> runs
			return;
		}
		if (next == turn)
		{
			return; // the only one still in plays on
		}

		putTurnAway(turns[turn]);
		turn = next;
		takeTurnUp(turns[turn]);
		turnHandedOver = out;
		beginTurn();
	}

	bool Game::takeTurnStop(void) noexcept
	{
		return std::exchange(turnHandedOver, false);
	}

	void Game::putTurnAway(Turn& into) const
	{
		into.objects = objects;
		into.stateTimers = stateTimers;
		into.currentScenes = currentScenes;
		into.sceneTimers = sceneTimers;
	}

	void Game::takeTurnUp(const Turn& from)
	{
		for (std::size_t i = 0; i < objects.size() && i < from.objects.size(); ++i)
		{
			if (objects[i].kept) { continue; }
			objects[i] = from.objects[i];
			objects[i].visualDirty = true;
		}
		stateTimers = from.stateTimers;
		currentScenes = from.currentScenes;
		sceneTimers = from.sceneTimers;
		++sceneChanges;
	}

	void Game::beginTurn(void)
	{
		// What was playing stops; the turn is played in the state the turns
		// started in, with the player's own state over it.
		loops.clear();
		while (currentState.size() > 1 && currentState.top().name != turnsState)
		{
			currentState.pop();
		}
		const std::string& state = turns[turn].state;
		if (!state.empty())
		{
			const auto found = std::find_if(states.begin(), states.end(), [&](const State& each) { return each.name == state; });
			if (found != states.end()) { stateTimers[state] = found->timers; }
			pushState(state);
		}
		++stateChanges;
		refreshAllBoundTexts();
	}

	bool Game::keepsAnything(void) const noexcept
	{
		return std::any_of(objects.begin(), objects.end(), [](const Object& object) { return object.kept; });
	}

	void Game::saveKept(std::ostream& out) const
	{
		out << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
			<< "<!-- What the game keeps from one game to the next (its <keeps>), written by the engine. -->\n"
			<< "<kept>\n";
		for (const auto& object : objects)
		{
			if (!object.kept) { continue; }
			out << "  <object name=\"" << object.name << "\"";
			if (!object.lookName().empty()) { out << " sprite=\"" << object.lookName() << "\""; }
			out << ">\n";
			for (const auto& [name, value] : object.variable)
			{
				std::ostringstream number;
				number << std::setprecision(std::numeric_limits<float>::max_digits10) << value;
				out << "    <variable name=\"" << name << "\">" << number.str() << "</variable>\n";
			}
			out << "  </object>\n";
		}
		out << "</kept>\n";
	}

	void Game::loadKept(std::istream& in)
	{
		// The file is the engine's own, written by saveKept, one tag a line,
		// so it is read a line at a time, without an XML library (a generated
		// program has none).
		static const std::regex objectLine{ R"line(^\s*<object name="([^"]*)"(?: sprite="([^"]*)")?>\s*$)line" };
		static const std::regex variableLine{ R"line(^\s*<variable name="([^"]*)">([^<]*)</variable>\s*$)line" };

		Object* object = nullptr;
		std::string line;
		while (std::getline(in, line))
		{
			std::smatch match;
			if (std::regex_match(line, match, objectLine))
			{
				object = tryGetObject(match[1].str());
				if (object && !object->kept) { object = nullptr; }
				if (object && match[2].matched)
				{
					const auto look = std::find_if(object->looks.begin(), object->looks.end(),
						[&](const Object::Look& each) { return each.name == match[2].str(); });
					if (look != object->looks.end()) { object->showLook(static_cast<std::size_t>(look - object->looks.begin())); }
				}
			}
			else if (object && std::regex_match(line, match, variableLine))
			{
				const auto variable = object->variable.find(match[1].str());
				if (variable == object->variable.end()) { continue; }
				try
				{
					variable->second = std::stof(match[2].str());
				}
				catch (const std::exception&)
				{
					std::cout << "warning: kept " << object->name << "." << variable->first << " is \"" << match[2].str() << "\", which is no number; left as it was\n";
				}
			}
		}
		refreshAllBoundTexts();
	}

	void Game::rank(const std::string& entry, const std::string& table)
	{
		const auto dot = entry.rfind('.');
		Object* from = dot == std::string::npos ? nullptr : tryGetObject(entry.substr(0, dot));
		const std::string by = dot == std::string::npos ? std::string{} : entry.substr(dot + 1);
		if (!from || !from->variable.count(by))
		{
			std::cout << "warning: <rank variable=\"" << entry << "\" into=\"" << table << "\" />: no such variable\n";
			return;
		}

		// The places, the best first, in the order the group's members are written.
		const std::string group = fullName(table);
		std::vector<Object*> places;
		for (auto& object : objects)
		{
			if (object.inGroup(group)) { places.push_back(&object); }
		}

		// Its place is the first one it beats; an equal score stays above it.
		const float score = from->variable.at(by);
		const auto place = std::find_if(places.begin(), places.end(),
			[&](const Object* each) { return each->variable.count(by) && each->variable.at(by) < score; });
		if (place == places.end())
		{
			return;
		}

		for (auto below = places.end() - 1; below != place; --below)
		{
			(*below)->variable = (*(below - 1))->variable;
		}
		for (auto& [name, value] : (*place)->variable)
		{
			if (const auto given = from->variable.find(name); given != from->variable.end())
			{
				value = given->second;
			}
		}

		for (Object* each : places)
		{
			for (const auto& [name, value] : each->variable)
			{
				refreshBoundTexts(each->name, name, value);
			}
		}
	}

	void Game::keep(const std::vector<std::string>& names)
	{
		// A name is an object or a group (every member of it), written out in
		// full the way a command's are.
		for (const std::string& given : names)
		{
			const std::string name = fullName(given);
			bool found = false;
			for (auto& object : objects)
			{
				if (object.name == name || object.inGroup(name))
				{
					object.kept = true;
					found = true;
				}
			}
			if (!found)
			{
				std::vector<std::string> known;
				for (const auto& object : objects) { known.push_back(object.name); }
				throw std::runtime_error("<keeps> > <keep object=\"" + given + "\" />: names no object of the game" + didYouMean(given, known));
			}
		}
	}

	void Game::changeVariable(const std::string& target, float delta, const char* verb)
	{
		// A text object counts its own number; anything else is
		// owner.variable, the variable after the last dot (the owner may be a
		// path: playing.room1.tally.kills).
		const bool whole = std::any_of(objects.begin(), objects.end(), [&](const Object& object) { return object.name == target; });
		const auto dot = whole ? std::string::npos : target.rfind('.');

		if (dot == std::string::npos)
		{
			// Older pattern (e.g. <inc variable="score1" /> for a text object that just
			// displays its own counter, with nothing else deriving its number):
			// the named object displays and owns its own number - change its
			// displayed value directly.
			Object* object = tryGetObject(target);
			if (!object)
			{
				std::cout << "warning: <" << verb << " variable=\"" << target << "\" />: no such object\n";
				return;
			}
			const float newValue = std::stof(object->spriteParams.at(1)) + delta;
			object->spriteParams.at(1) = formatDisplayNumber(newValue);
			object->visualDirty = true;
			return;
		}

		// Usual pattern (e.g. <inc variable="paddle1.score" />): change another object's
		// own named <variable>, then refresh every text object whose displayed
		// number is bound to it (Object::textPieces, set from a <text> sprite
		// whose <number> is paddle1.score - see parseVariableReference).
		const std::string ownerName = target.substr(0, dot);
		const std::string variableName = target.substr(dot + 1);

		Object* owner = tryGetObject(ownerName);
		if (!owner)
		{
			std::cout << "warning: <" << verb << " variable=\"" << target << "\" />: no object named '" << ownerName << "'\n";
			return;
		}

		auto variableIt = owner->variable.find(variableName);
		if (variableIt == owner->variable.end())
		{
			std::cout << "warning: <" << verb << " variable=\"" << target << "\" />: '" << ownerName << "' has no variable named '" << variableName << "'\n";
			return;
		}

		variableIt->second += delta;
		refreshBoundTexts(owner->name, variableName, variableIt->second);
	}

	int Game::framesFor(float seconds) const noexcept
	{
		const int framerate = windowDesc.framerate > 0 ? windowDesc.framerate : 60;
		return std::max(1, static_cast<int>(std::lround(seconds * static_cast<float>(framerate))));
	}

	bool Game::tickTimer(Timer& timer, const std::string& where)
	{
		if (timer.done)
		{
			return false;
		}

		if (timer.framesLeft < 0)
		{
			timer.framesLeft = framesFor(expr.evaluate(timer.interval, where));
		}

		if (--timer.framesLeft > 0)
		{
			return false;
		}

		// Gone off: a repeating timer starts over, with its interval worked out
		// again on the next frame, after its commands have run (so they can
		// change it, and a <random> waits a new time); a once-only one is
		// finished.
		if (timer.repeat) { timer.framesLeft = -1; }
		else { timer.done = true; }
		return true;
	}

	void Game::updateTimers(void)
	{
		CommandExecutor executor(*this);

		// An object's timers, while it is shown and in play. By index: a timer
		// can fire a projectile or release a pool, which changes other objects
		// but never how many there are.
		for (std::size_t i = 0; i < objects.size(); ++i)
		{
			for (std::size_t t = 0; t < objects[i].timers.size(); ++t)
			{
				if (!isShown(objects[i]))
				{
					break;
				}

				// One that follows a look counts only while the object shows it.
				const std::string& sprite = objects[i].timers[t].sprite;
				if (!sprite.empty() && objects[i].lookName() != sprite)
				{
					continue;
				}

				if (tickTimer(objects[i].timers[t], "object '" + objects[i].name + "' > <timer>"))
				{
					// A copy: a command may reset the object, and its timers with it.
					const std::vector<Command> commands = objects[i].timers[t].commands;
					for (const auto& command : commands)
					{
						executor.executeTimer(command, &objects[i]);
					}
				}
			}
		}

		// The current state's timers. A command can change the state, which
		// ends this state's turn for the frame.
		const std::string stateName = currentState.top().name;
		auto found = stateTimers.find(stateName);
		if (found == stateTimers.end())
		{
			return;
		}

		const unsigned long changes = stateChanges;
		const unsigned long scenes = sceneChanges;
		const auto unchanged = [&] { return stateChanges == changes && sceneChanges == scenes; };
		const auto count = [&](std::vector<Timer>& timers, const std::string& where)
		{
			for (std::size_t t = 0; t < timers.size() && unchanged(); ++t)
			{
				if (tickTimer(timers[t], where))
				{
					const std::vector<Command> commands = timers[t].commands;
					takeTurnStop();
					for (const auto& command : commands)
					{
						executor.executeTimer(command, nullptr);
						if (takeTurnStop() || !unchanged()) { break; }
					}
				}
			}
		};
		count(found->second, "state '" + stateName + "' > <timer>");

		// Then its current scene's.
		const std::string scene = currentScene(stateName);
		const auto sceneFound = sceneTimers.find(scene);
		if (unchanged() && sceneFound != sceneTimers.end())
		{
			count(sceneFound->second, "scene '" + scene + "' > <timer>");
		}
	}

	void Game::refreshBoundTexts(const std::string& ownerName, const std::string& variableName, float value)
	{
		// Expressions read the variable too (a timer's <every>, worked out again
		// each round), so they see the new value.
		const auto expressionVariable = expr.objectVariables.find(ownerName + "." + variableName);
		if (expressionVariable != expr.objectVariables.end())
		{
			expressionVariable->second = value;
		}

		for (auto& object : objects)
		{
			if (object.showsVariable(ownerName, variableName))
			{
				relabel(object);
			}
		}
	}

	bool Game::setVariable(const std::string& objectName, const std::string& variableName, float value)
	{
		Object* owner = tryGetObject(objectName);
		if (!owner)
		{
			return false;
		}

		auto variableIt = owner->variable.find(variableName);
		if (variableIt == owner->variable.end())
		{
			return false;
		}

		variableIt->second = value;
		refreshBoundTexts(owner->name, variableName, value);
		return true;
	}

	namespace
	{
		// Shared by Game::resetObject (one named object) and Game::resetAll
		// (every object): puts position/velocity/every <variable> back to
		// their construction-time snapshots (positionOriginal/velocityOriginal/
		// variableOriginal). Doesn't touch bound text displays - callers do
		// that afterward, once they know which objects actually changed.
		void resetObjectState(Object& object) noexcept
		{
			object.position = object.positionOriginal;
			object.velocity = object.velocityOriginal;

			// Whatever has died, or been fired, since the game began is back
			// as it started (a bullet in flight is put away, an alien is back).
			object.isVisible = object.isVisibleOriginal;
			object.collisionData.enabled = object.collisionEnabledOriginal;

			// Also drop any directions CommandExecutor::applyActionVelocity
			// currently has recorded as held: velocity above is reset to rest,
			// so a stale held direction (a key never released across this
			// reset) shouldn't be able to resurrect part of the old velocity
			// the next time some other direction's key event recomputes it.
			object.activeMoveStep = {};

			// The same goes for thrust still recorded as held, and a <stop />
			// that took the acceleration away gives it back.
			object.acceleration = object.accelerationOriginal;
			object.activeThrust = {};
			object.activeThrustBurn = {};

			// Facing the way it started, and no turn or thrust along it still held.
			object.heading = object.headingOriginal;
			object.activeTurn = {};
			object.activeThrustAheadBy.clear();
			object.showHeading();

			// An animation starts again from its first picture.
			object.restartAnimation();

			// Nor should a reset object go on riding, or jump; it
			// faces the way it started, and its timers start over.
			object.riding = {};
			object.carrying.clear();
			object.dropped.clear();
			object.hopPending = {};
			object.hopped = false;
			object.jumpStep = {};
			object.jumpFramesLeft = 0;
			object.grounded = false;
			object.leaping = false;
			object.climbing = false;
			object.activeClimb = {};
			object.hasAim = false;
			object.facing = object.facingOriginal;
			object.followPath.clear();
			object.walk = {};
			object.ownerScene = object.ownerSceneOriginal;
			if (!object.looks.empty()) { object.showLook(0); }
			for (auto& timer : object.timers)
			{
				timer.framesLeft = -1;
				timer.done = false;
			}

			for (auto& [variableName, originalValue] : object.variableOriginal)
			{
				object.variable[variableName] = originalValue;
			}
		}
	}

	void Game::resetObject(const std::string& given)
	{
		const std::string name = fullName(given);
		Object* object = tryGetObject(name);
		if (!object)
		{
			std::cout << "warning: <reset object=\"" << name << "\" />: no such object\n";
			return;
		}

		// A group's name (aliens) means every member of it, and an object's
		// own name (aliens.3.2) just that one.
		for (auto& candidate : objects)
		{
			if (candidate.name == name || candidate.inGroup(name))
			{
				drawStartAgain(candidate);
				resetObjectState(candidate);
			}
		}

		// Back where it started, it is no longer carried.
		for (auto& carrier : objects)
		{
			const Object* carried = carrier.carrying.empty() ? nullptr : tryGetObject(carrier.carrying);
			if (carried && (carried->name == name || carried->inGroup(name)))
			{
				carrier.carrying.clear();
			}
		}

		// Refresh every text display bound to one of this object's variables
		// (same notify pattern as incrementText above), so e.g. a HUD showing
		// paddle1.score visibly drops back to 0 immediately, not just internally.
		const std::string owner = object->name;
		for (auto& other : objects)
		{
			if (std::any_of(other.textPieces.begin(), other.textPieces.end(), [&](const TextPiece& piece) { return piece.owner == owner; }))
			{
				relabel(other);
			}
		}
	}

	void Game::become(const std::string& target, const std::string& sprite, const std::string& from)
	{
		for (auto& object : objects)
		{
			if ((object.name == target || object.inGroup(target)) && (from.empty() || object.lookName() == from))
			{
				showLookNamed(object, sprite);
			}
		}
	}

	void Game::reveal(const std::string& target, int count)
	{
		for (auto& object : objects)
		{
			if (count <= 0) { break; }

			const bool named = object.name == target || object.inGroup(target);
			if (!named || object.isVisible || isAway(object)) { continue; }

			// Back in play is a new start: a <random> in it is drawn again.
			drawStartAgain(object);
			object.position = object.positionOriginal;
			object.velocity = object.velocityOriginal;
			object.isVisible = true;
			object.collisionData.enabled = object.collisionEnabledOriginal;
			object.followPath.clear();
			--count;
		}
	}

	const std::map<std::string, Path>& Game::getPaths(void) const noexcept
	{
		return paths;
	}

	void Game::follow(Object& object, const std::string& path, int wait)
	{
		const Path& chosen = paths.at(path);
		if (object.isFollowing())
		{
			return;
		}

		object.followPath = path;
		object.followLeg = 0;
		object.followLeft = {};
		object.followLegStarted = false;
		object.followWait = std::max(0, wait);
		object.velocity = {};
		if (chosen.hasStart)
		{
			object.position = chosen.start;
		}
	}

	void Game::follow(const std::string& target, const std::string& path, float stagger)
	{
		const int framerate = windowDesc.framerate > 0 ? windowDesc.framerate : 60;
		int setOff = 0;
		for (auto& object : objects)
		{
			const bool named = object.name == target || object.inGroup(target);
			if (!named || !object.isVisible || object.isFollowing() || isAway(object)) { continue; }

			follow(object, path, static_cast<int>(std::lround(static_cast<float>(setOff) * stagger * static_cast<float>(framerate))));
			++setOff;
		}
	}

	void Game::applyPaths(void)
	{
		CommandExecutor executor(*this);

		for (auto& object : objects)
		{
			if (!object.isFollowing() || !isShown(object))
			{
				continue;
			}

			if (object.followWait > 0)
			{
				--object.followWait;
				object.velocity = {};
				continue;
			}

			const Path& path = paths.at(object.followPath);
			bool moving = false;
			while (object.isFollowing() && object.followLeg < path.steps.size())
			{
				const PathStep& leg = path.steps[object.followLeg];
				if (!object.followLegStarted)
				{
					object.followLegStarted = true;
					object.followLeft = leg.by;

					// A copy: a command may reset the object, which ends the path.
					const std::vector<Command> commands = leg.commands;
					for (const auto& command : commands)
					{
						executor.executeTimer(command, &object);
					}
					if (!object.isFollowing()) { break; }
				}

				// What is left of this leg: the rest of a step, or the way home
				// from wherever it has got to.
				const Vector2f left = leg.home ? object.positionOriginal - object.position : object.followLeft;
				const std::optional<Vector2f> step = pathStep(left, path.speed);
				if (!step)
				{
					++object.followLeg;
					object.followLegStarted = false;
					continue;
				}

				object.velocity = *step;
				if (!leg.home) { object.followLeft -= *step; }
				moving = true;
				break;
			}

			// The end of the path: it comes to rest where it is.
			if (object.isFollowing() && !moving)
			{
				object.velocity = {};
				object.followPath.clear();
			}
		}
	}

	void Game::resetAll()
	{
		// A new game starts quiet: whatever loop the last one left on (music,
		// a thrust held into the game over) stops.
		loops.clear();
		++resets;
		turns.clear();
		turnHandedOver = false;

		for (auto& object : objects)
		{
			if (object.kept) { continue; }
			drawStartAgain(object);
			resetObjectState(object);
		}

		for (const auto& state : states)
		{
			stateTimers[state.name] = state.timers;
		}
		startScenes();

		// Refresh every text display bound to another object's variable, now
		// that all of them are back to their starting values (same notify
		// pattern as resetObject/incrementText above, just over every bound
		// display instead of one object's worth).
		refreshAllBoundTexts();

		// Collapse the whole state stack. mainmenu -> playing -> gameover ->
		// mainmenu -> ... is push-only (see pushState/setCurrentState) - only
		// settings and paused ever pop back off - so left alone it grows for as
		// long as the game keeps getting played. Clear it and start fresh from
		// the very first state, exactly like Engine's constructor does once at
		// startup.
		while (!currentState.empty())
		{
			currentState.pop();
		}
		setCurrentState(0);
	}

	void Game::refreshAllBoundTexts(void)
	{
		for (auto& object : objects)
		{
			if (!object.textPieces.empty())
			{
				relabel(object);
			}
		}
	}

	void Game::relabel(Object& object)
	{
		if (object.shapeKind != ShapeKind::Text || object.spriteParams.size() < 2)
		{
			return;
		}

		for (TextPiece& piece : object.textPieces)
		{
			if (piece.owner.empty())
			{
				continue;
			}
			const Object* owner = tryGetObject(piece.owner);
			if (!owner)
			{
				continue;
			}
			const auto value = owner->variable.find(piece.variable);
			if (value != owner->variable.end())
			{
				piece.shown = showPiece(piece.kind, value->second);
			}
		}

		const std::string text = joinText(object.textPieces);
		if (object.spriteParams.at(1) != text)
		{
			object.spriteParams.at(1) = text;
			object.visualDirty = true;
		}
	}

	std::string Game::wayLeadsTo(const std::string& way) const
	{
		if (!currentState.empty())
		{
			const State& state = currentState.top();
			const std::string current = currentScene(state.name);
			for (const Scene& scene : state.scenes)
			{
				if (scene.name != current) { continue; }
				const auto found = scene.ways.find(way);
				if (found != scene.ways.end()) { return found->second; }
			}
		}
		std::cout << "warning: way=\"" << way << "\": the current scene has no way of that name\n";
		return {};
	}

	void Game::enter(const CmdEnterScene& command)
	{
		const std::string scene = command.way.empty() ? command.scene : wayLeadsTo(command.way);
		if (!scene.empty()) { enterScene(scene); }
	}

	void Game::reset(const CmdResetScene& command)
	{
		const std::string scene = command.way.empty() ? command.scene : wayLeadsTo(command.way);
		if (!scene.empty()) { resetScene(scene); }
	}

	void Game::resetScene(const std::string& scene)
	{
		const auto owner = std::find_if(states.begin(), states.end(), [&](const State& state)
			{
				return std::any_of(state.scenes.begin(), state.scenes.end(), [&](const Scene& each) { return each.name == scene; });
			});
		if (owner == states.end())
		{
			throw std::out_of_range("no scene named '" + scene + "'");
		}

		for (auto& object : objects)
		{
			if (object.ownerSceneOriginal != scene) { continue; }
			drawStartAgain(object);
			resetObjectState(object);

			// Back where it started, it is no longer carried.
			for (auto& carrier : objects)
			{
				if (carrier.carrying == object.name) { carrier.carrying.clear(); }
			}
		}

		for (const Scene& each : owner->scenes)
		{
			if (each.name == scene) { sceneTimers[scene] = each.timers; }
		}
		refreshAllBoundTexts();
	}

	void Game::updateLockstepObjects(const Object& object) noexcept
	{
		const int lockstepNum = object.collisionData.lockstep;

		// Every member gets exactly the same treatment. This used to shift the
		// members stored before the touching one by three steps and the rest
		// by one (and repeated that on every right-hand bounce), which pushed
		// the last column of a block further from the others each time.
		for (auto& obj : objects)
		{
			if (obj.collisionData.lockstep == lockstepNum)
			{
				obj.velocity.x *= -1;
				obj.position.x += obj.velocity.x;
			}
		}
	}

	namespace
	{
		// A reference, unlike a pointer, can't be null - which is what keeps
		// this out of the same "what if the switch fell through" trap as the
		// raw-pointer version this replaced (VS flagged that one, C6011,
		// even though Edge's four enumerators are all handled below).
		const std::vector<Command>& collisionCommandsFor(const Object& object, Edge edge)
		{
			switch (edge)
			{
			case Edge::Left:   return object.collisionData.left;
			case Edge::Right:  return object.collisionData.right;
			case Edge::Top:    return object.collisionData.top;
			case Edge::Bottom: return object.collisionData.bottom;
			}

			// Unreachable: Edge only ever has the four values above.
			return object.collisionData.top;
		}

		// The edge's rules with sprite= (see EdgeGuard).
		const std::vector<EdgeGuard>& collisionGuardsFor(const Object& object, Edge edge)
		{
			switch (edge)
			{
			case Edge::Left:   return object.collisionData.leftGuards;
			case Edge::Right:  return object.collisionData.rightGuards;
			case Edge::Top:    return object.collisionData.topGuards;
			case Edge::Bottom: return object.collisionData.bottomGuards;
			}

			// Unreachable: Edge only ever has the four values above.
			return object.collisionData.topGuards;
		}

		// Shared by a collision rule's class/object filter and a state
		// condition's: empty filterClass/filterObject match anything; either or
		// both narrow it to a specific class and/or one specific named object.
		bool matchesClassOrObjectFilter(const std::string& filterClass, const std::string& filterObject, const Object& candidate)
		{
			if (!filterClass.empty() && filterClass != candidate.objClass) { return false; }
			if (!filterObject.empty() && filterObject != candidate.name && !candidate.inGroup(filterObject)) { return false; }
			return true;
		}

		// The name is the object's, or one of its groups', under the path of
		// the scene or state it belongs to: not the name its state gives that
		// piece in every scene.
		bool namedByPath(const Object& object, const std::string& name)
		{
			const std::string& owner = object.ownerScene.empty() ? object.ownerState : object.ownerScene;
			return !name.empty() && !owner.empty() && (name == object.name || object.inGroup(name))
				&& name.compare(0, owner.size() + 1, owner + ".") == 0;
		}

		bool collisionRuleMatches(const CollisionRule& rule, const Object& other)
		{
			return matchesClassOrObjectFilter(rule.filterClass, rule.filterObject, other);
		}
	}

	// The object is at or past this edge (playOneFrame looked): the edge's
	// rules, in the order written.
	void Game::atEdge(Object& object, Edge edge)
	{
		CommandExecutor executor(*this);
		const std::vector<Command>& commands = collisionCommandsFor(object, edge);
		const std::vector<EdgeGuard>& guards = collisionGuardsFor(object, edge);
		for (std::size_t i = 0; i < commands.size();)
		{
			// a rule with sprite= or unless= is looked at as it comes, after
			// the rules before it have run (one of them may have changed it)
			const auto guard = std::find_if(guards.begin(), guards.end(), [&](const EdgeGuard& g) { return g.first == i; });
			if (guard != guards.end() && isEdgeRulePassedOver(object, *guard))
			{
				i += guard->count;
				continue;
			}
			executor.executeScreenEdgeCollision(commands[i], object, edge);
			++i;
		}
	}

	bool Game::isEdgeRulePassedOver(const Object& object, const EdgeGuard& guard)
	{
		return (!guard.sprite.empty() && object.lookName() != guard.sprite)
			|| (!guard.unlessClass.empty() && isTouchingClass(object, object, guard.unlessClass));
	}

	// Whether the edge has a stick() rule that holds right now (one in a rule
	// with sprite= or unless= only while it holds): playOneFrame puts the
	// object back inside after the frame's move, whether or not it is still
	// moving. Other verbs (bounce, reset, inc, ...) run only from atEdge,
	// before the move.
	bool Game::sticksAt(const Object& object, Edge edge)
	{
		const auto& commands = collisionCommandsFor(object, edge);
		const auto& guards = collisionGuardsFor(object, edge);
		for (std::size_t i = 0; i < commands.size(); ++i)
		{
			const bool passedOver = std::any_of(guards.begin(), guards.end(), [&](const EdgeGuard& guard)
				{ return i >= guard.first && i < guard.first + guard.count && isEdgeRulePassedOver(object, guard); });
			if (!passedOver && std::holds_alternative<CmdStick>(commands[i]))
			{
				return true;
			}
		}
		return false;
	}

	// Whether a rule of either answers to the other (playOneFrame has checked
	// they can touch at all).
	bool Game::canCollide(const Object& a, const Object& b)
	{
		const auto answers = [](const Object& self, const Object& other)
		{
			return std::any_of(self.collisionData.basic.begin(), self.collisionData.basic.end(),
				[&](const CollisionRule& rule) { return collisionRuleMatches(rule, other); });
		};

		return answers(a, b) || answers(b, a);
	}

	// A thruster burns one unit of the object's own `variable`, if it has any
	// left.
	bool Game::burn(Object& object, const std::string& variable, const char* verb)
	{
		const auto fuel = object.variable.find(variable);
		if (fuel == object.variable.end() || fuel->second <= 0.0f) { return false; }
		changeVariable(object.name + "." + variable, -1.0f, verb);
		return true;
	}

	// Whether `object` is touching, right now, any other object in play that is
	// of class `objClass` - `excluding` (the one it is in the middle of a
	// collision with) is left out, and so is `object` itself.
	bool Game::isTouchingClass(const Object& object, const Object& excluding, const std::string& objClass)
	{
		return std::any_of(objects.begin(), objects.end(), [&](const Object& other)
			{
				return &other != &object && &other != &excluding
					&& !other.isAirborne()
					&& other.objClass == objClass
					&& isShown(other) && other.collisionData.enabled
					&& CollisionDetector::overlap(object, other).has_value();
			});
	}

	// edgeOfB is the edge of b that was touched, which is already b's own side
	// of it; a's own touched edge is the opposite one.
	void Game::touched(Object& a, Object& b, Edge edgeOfB)
	{
		CommandExecutor executor(*this);

		// One side's rules for touching the other. A rule with unless= is
		// passed over while `self` is also touching something of that class.
		const auto react = [&](Object& self, Object& other, Edge selfEdge)
		{
			// How fast it is going at the moment of the touch, for the rules
			// with slower/faster; taken once, so that a rule that stops it does
			// not change which of the rules after it run.
			const Vector2f motion = motionOf(self);
			const float speed = std::sqrt(motion.x * motion.x + motion.y * motion.y);

			for (const auto& rule : self.collisionData.basic)
			{
				if (!collisionRuleMatches(rule, other)) { continue; }
				if (!rule.whileSprite.empty() && self.lookName() != rule.whileSprite) { continue; }
				if (!rule.unlessClass.empty() && isTouchingClass(self, other, rule.unlessClass)) { continue; }
				if (rule.slower && !(speed < *rule.slower)) { continue; }
				if (rule.faster && !(speed >= *rule.faster)) { continue; }

				for (const auto& command : rule.commands)
				{
					executor.executeObjectCollision(command, self, other, selfEdge);
				}
			}
		};

		react(a, b, opposite(edgeOfB));
		react(b, a, edgeOfB);
	}

	void Game::checkConditions()
	{
		// currentState.top() directly, rather than getCurrentState() (which
		// returns a copy of the whole State) - this runs every frame, so
		// copying every command list in it just to read .conditions would be
		// wasteful. The state's own conditions, then its current scene's.
		const State& state = currentState.top();
		if (checkConditionList(state.conditions)) { return; }

		const std::string scene = currentScene(state.name);
		const auto found = std::find_if(state.scenes.begin(), state.scenes.end(), [&](const Scene& each) { return each.name == scene; });
		if (found != state.scenes.end()) { checkConditionList(found->conditions); }
	}

	bool Game::checkConditionList(const std::vector<Condition>& conditions)
	{
		for (auto& condition : conditions)
		{
			// A class, or a name every scene has (robots, for playing.robots),
			// is what is in this state and scene (and the game's own); a name
			// by its path (room2.robots, playing.room2.robots.3) is that
			// thing, wherever it is.
			const auto considers = [&](const Object& object)
			{
				return matchesClassOrObjectFilter(condition.filterClass, condition.filterObject, object)
					&& (inScope(object) || namedByPath(object, condition.filterObject));
			};

			// "No more than N of them left": everything that matches, still
			// in play. Nothing to look at per object, so this is its own case.
			if (condition.remaining)
			{
				const auto stillIn = std::count_if(objects.begin(), objects.end(), [&](const Object& object)
					{
						return object.isVisible && considers(object);
					});

				if (static_cast<float>(stillIn) <= *condition.remaining)
				{
					runConditionCommands(condition.commands);
					return true; // the state may have just changed - stop for this frame
				}

				continue;
			}

			for (auto& object : objects)
			{
				if (!considers(object))
				{
					continue;
				}

				auto variableIt = object.variable.find(condition.variableName);
				if (variableIt == object.variable.end())
				{
					continue;
				}

				// Against a number, or another variable as it is now.
				float against = condition.value;
				if (!condition.otherVariable.empty())
				{
					const Object* other = condition.otherObject.empty() ? &object : tryGetObject(condition.otherObject);
					if (!other)
					{
						continue;
					}
					const auto otherIt = other->variable.find(condition.otherVariable);
					if (otherIt == other->variable.end())
					{
						continue;
					}
					against = otherIt->second;
				}

				if (!passes(condition.compare, variableIt->second, against))
				{
					continue;
				}

				runConditionCommands(condition.commands);
				return true; // the state may have just changed - stop for this frame
			}
		}
		return false;
	}

	// A copy of the commands, not the state's own list: the list belongs to the
	// current state, and a <pop /> among them frees that state while the rest
	// of the list is still to run.
	void Game::runConditionCommands(std::vector<Command> commands)
	{
		CommandExecutor executor(*this);
		takeTurnStop();
		for (const auto& command : commands)
		{
			executor.executeCondition(command);
			if (takeTurnStop()) { break; }
		}
	}
}
