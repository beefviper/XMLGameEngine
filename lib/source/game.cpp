// game.cpp
// XML Game Engine
// author: beefviper
// date: Sept 18, 2020

#include "game.h"

#include "command_executor.h"

#include <algorithm>
#include <cstddef>
#include <stdexcept>
#include <utility>
#include <variant>

namespace xge
{
	Game::Game(const std::string& game, XmlBackend xmlBackend) :
		filename(game)
	{
		xml.init(filename, xmlBackend, windowDesc, rawVariables, rawStates, rawObjects, rawSounds, rawPaths, xmlValidation);
		expr.init(windowDesc, rawVariables, variables, rawStates, states, rawObjects, objects, rawSounds, sounds, rawPaths, paths);
		expr.finishLoading();

		for (const auto& state : states)
		{
			stateTimers[state.name] = state.timers;
		}
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
				expr.setObjectSize(object.baseName, object.size);
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
					[&](const Object& other) { return other.baseName == dependency && other.sizeKnown; });
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
				[&](const RawObject& raw) { return raw.name == object.baseName; });
			if (rawObject == rawObjects.end())
			{
				continue;
			}

			const std::string where = "object '" + rawObject->name + "'";
			object.positionOriginal.x = expr.evaluate(rawObject->rawPosition.x, where);
			object.positionOriginal.y = expr.evaluate(rawObject->rawPosition.y, where);
			object.positionOriginal = object.positionOriginal + object.gridOffset;
			object.position = object.positionOriginal;
			object.positionSizesUsed = sizes;
			object.positionResolved = true;
		}
	}

	void Game::updateObjects(void)
	{
		auto& currentObjects = getCurrentObjects();

		// A picture that changes with time (an <animation>) counts this frame
		// whether or not the object is moving; only what is shown and in play
		// does, so a pause or a menu holds it where it was.
		for (auto& object : currentObjects)
		{
			if (isShown(object)) { object.advanceAnimation(); }
		}

		// Timers go off before anything moves, so a bomb dropped this frame
		// starts falling this frame.
		updateTimers();

		applyPaths();

		applyClimbing();

		applyAcceleration();

		// Screen-edge checks: independent per object, order doesn't matter.
		for (auto& object : currentObjects)
		{
			if (isShown(object) && object.collisionData.enabled && isMoving(object))
			{
				checkEdge(object, Edge::Top);
				checkEdge(object, Edge::Bottom);
				checkEdge(object, Edge::Left);
				checkEdge(object, Edge::Right);
			}
		}

		// Movement and object-against-object collisions are one job, so that
		// a fast or small object cannot jump over a thin one between frames.
		moveObjects();

		for (auto& object : currentObjects)
		{
			// The edge checks above run before the move, and only for an object
			// that is moving, so on their own they let a stick()ed object end the
			// frame poking out past the wall (and stay there once it stopped
			// moving).
			if (isShown(object) && object.collisionData.enabled)
			{
				keepStuckObjectInBounds(object);
			}
		}

		checkConditions();
	}

	void Game::printGame(void)
	{
		std::cout << "schema validation: " << xmlValidation << "\n\n";

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
		if (std::find(soundRequests.begin(), soundRequests.end(), name) == soundRequests.end())
		{
			soundRequests.push_back(name);
		}
	}

	std::vector<std::string> Game::takeSoundRequests(void)
	{
		return std::exchange(soundRequests, {});
	}

	WindowDesc& Game::getWindowDesc(void) noexcept
	{
		return windowDesc;
	}

	bool Game::isShown(const Object& object) noexcept
	{
		bool result = false;

		for (auto& shown : currentState.top().show)
		{
			if (shown == object.name || shown == object.baseName || (!object.groupName.empty() && shown == object.groupName))
			{
				result = true;
			}
		}
		return result && object.isVisible;
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

	// An exact name (aliens.3.2) finds that one object; the name from the XML
	// (aliens, or the name of a <group>) finds the first object made from it,
	// which for anything that is not a grid or a group is the object itself.
	Object* Game::tryGetObject(const std::string& name) noexcept
	{
		const auto exact = std::find_if(std::begin(objects), std::end(objects), [&](const Object& obj) { return obj.name == name; });
		if (exact != std::end(objects))
		{
			return &(*exact);
		}

		const auto first = std::find_if(std::begin(objects), std::end(objects), [&](const Object& obj) { return obj.baseName == name || (!obj.groupName.empty() && obj.groupName == name); });
		return (first == std::end(objects)) ? nullptr : &(*first);
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

		const auto raw = std::find_if(rawObjects.begin(), rawObjects.end(), [&](const RawObject& candidate) { return candidate.name == object.baseName; });
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
			refreshBoundTexts(object.baseName, name, drawn);
		}

		object.velocityOriginal = { expr.evaluate(raw->rawVelocity.x, where), expr.evaluate(raw->rawVelocity.y, where) };

		// A position waiting on a size not measured yet is worked out when it is.
		if (object.positionResolved)
		{
			object.positionOriginal = Vector2f{ expr.evaluate(raw->rawPosition.x, where), expr.evaluate(raw->rawPosition.y, where) } + object.gridOffset;
		}
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

	void Game::changeVariable(const std::string& target, float delta, const char* verb)
	{
		const auto dot = target.find('.');

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
		// number is bound to it (Object::boundVariableOwner/boundVariableName,
		// set from a <text> sprite whose <number> is paddle1.score - see
		// parseVariableReference).
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
		refreshBoundTexts(ownerName, variableName, variableIt->second);
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

				if (tickTimer(objects[i].timers[t], "object '" + objects[i].baseName + "' > <timer>"))
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
		for (std::size_t t = 0; t < found->second.size() && stateChanges == changes; ++t)
		{
			if (tickTimer(found->second[t], "state '" + stateName + "' > <timer>"))
			{
				const std::vector<Command> commands = found->second[t].commands;
				for (const auto& command : commands)
				{
					executor.executeTimer(command, nullptr);
					if (stateChanges != changes) { break; }
				}
			}
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
			if (object.boundVariableOwner == ownerName && object.boundVariableName == variableName)
			{
				object.spriteParams.at(1) = formatDisplayNumber(value);
				object.visualDirty = true;
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
		refreshBoundTexts(objectName, variableName, value);
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
			object.activeThrustAhead = 0.0f;
			object.activeThrustAheadBurn.clear();
			object.showHeading();

			// An animation starts again from its first picture.
			object.restartAnimation();

			// Nor should a reset object carry on being carried, or jump; it
			// faces the way it started, and its timers start over.
			object.carry = {};
			object.hopPending = {};
			object.hopped = false;
			object.jumpStep = {};
			object.jumpFramesLeft = 0;
			object.grounded = false;
			object.leaping = false;
			object.climbing = false;
			object.activeClimb = {};
			object.facing = object.facingOriginal;
			object.followPath.clear();
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

	void Game::resetObject(const std::string& name)
	{
		Object* object = tryGetObject(name);
		if (!object)
		{
			std::cout << "warning: <reset object=\"" << name << "\" />: no such object\n";
			return;
		}

		// A name from the XML (aliens) means every object made from it - the
		// whole grid, or the whole group - and an exact one (aliens.3.2) just
		// that one.
		for (auto& candidate : objects)
		{
			if (candidate.name == name || candidate.baseName == name || (!candidate.groupName.empty() && candidate.groupName == name))
			{
				drawStartAgain(candidate);
				resetObjectState(candidate);
			}
		}

		// Refresh every text display bound to one of this object's variables
		// (same notify pattern as incrementText above), so e.g. a HUD showing
		// paddle1.score visibly drops back to 0 immediately, not just internally.
		for (auto& other : objects)
		{
			if (other.boundVariableOwner != name)
			{
				continue;
			}

			const auto valueIt = object->variable.find(other.boundVariableName);
			if (valueIt != object->variable.end())
			{
				const std::string text = formatDisplayNumber(valueIt->second);
				if (other.spriteParams.at(1) != text)
				{
					other.spriteParams.at(1) = text;
					other.visualDirty = true;
				}
			}
		}
	}

	void Game::become(const std::string& target, const std::string& sprite)
	{
		for (auto& object : objects)
		{
			if (object.name == target || object.baseName == target || (!object.groupName.empty() && object.groupName == target))
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

			const bool named = object.name == target || object.baseName == target || (!object.groupName.empty() && object.groupName == target);
			if (!named || object.isVisible) { continue; }

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
			const bool named = object.name == target || object.baseName == target || (!object.groupName.empty() && object.groupName == target);
			if (!named || !object.isVisible || object.isFollowing()) { continue; }

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
				const float distance = std::hypot(left.x, left.y);
				if (distance < 0.001f)
				{
					++object.followLeg;
					object.followLegStarted = false;
					continue;
				}

				const Vector2f step = distance <= path.speed ? left : left * (path.speed / distance);
				object.velocity = step;
				if (!leg.home) { object.followLeft -= step; }
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
		for (auto& object : objects)
		{
			drawStartAgain(object);
			resetObjectState(object);
		}

		for (const auto& state : states)
		{
			stateTimers[state.name] = state.timers;
		}

		// Refresh every text display bound to another object's variable, now
		// that all of them are back to their starting values (same notify
		// pattern as resetObject/incrementText above, just over every bound
		// display instead of one object's worth).
		for (auto& object : objects)
		{
			if (object.boundVariableOwner.empty())
			{
				continue;
			}

			Object* owner = tryGetObject(object.boundVariableOwner);
			if (!owner)
			{
				continue;
			}

			const auto valueIt = owner->variable.find(object.boundVariableName);
			if (valueIt != owner->variable.end())
			{
				const std::string text = formatDisplayNumber(valueIt->second);
				if (object.spriteParams.at(1) != text)
				{
					object.spriteParams.at(1) = text;
					object.visualDirty = true;
				}
			}
		}

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

	void Game::updateLockstepObjects(const Object& object) noexcept
	{
		const int lockstepNum = object.collisionData.lockstep;

		// Every member gets exactly the same treatment. This used to shift the
		// members stored before the touching one by three steps and the rest
		// by one (and repeated that on every right-hand bounce), which pushed
		// the last column of a <grid> further from the others each time.
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
		std::vector<Command>& collisionCommandsFor(Object& object, Edge edge)
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

		// Shared by a collision rule's class/object filter and a state
		// condition's: empty filterClass/filterObject match anything; either or
		// both narrow it to a specific class and/or one specific named object.
		bool matchesClassOrObjectFilter(const std::string& filterClass, const std::string& filterObject, const Object& candidate)
		{
			if (!filterClass.empty() && filterClass != candidate.objClass) { return false; }
			if (!filterObject.empty() && filterObject != candidate.name && filterObject != candidate.baseName
				&& (candidate.groupName.empty() || filterObject != candidate.groupName)) { return false; }
			return true;
		}

		bool collisionRuleMatches(const CollisionRule& rule, const Object& other)
		{
			return matchesClassOrObjectFilter(rule.filterClass, rule.filterObject, other);
		}
	}

	void Game::checkEdge(Object& object, Edge edge)
	{
		if (!CollisionDetector::touchesScreenEdge(object, windowDesc, edge))
		{
			return;
		}

		CommandExecutor executor(*this);
		for (const auto& command : collisionCommandsFor(object, edge))
		{
			executor.executeScreenEdgeCollision(command, object, edge);
		}
	}

	// Re-applies just the stick() rules, after the frame's move, whether or not
	// the object is still moving - see the call in updateObjects. Other verbs
	// (bounce, reset, inc, ...) stay where checkEdge runs them, before the
	// move, so their timing is unchanged.
	void Game::keepStuckObjectInBounds(Object& object)
	{
		CommandExecutor executor(*this);
		for (const Edge edge : { Edge::Top, Edge::Bottom, Edge::Left, Edge::Right })
		{
			const auto& commands = collisionCommandsFor(object, edge);
			const bool sticks = std::any_of(commands.begin(), commands.end(),
				[](const Command& command) { return std::holds_alternative<CmdStick>(command); });

			if (sticks && CollisionDetector::touchesScreenEdge(object, windowDesc, edge))
			{
				executor.executeScreenEdgeCollision(Command{ CmdStick{} }, object, edge);
			}
		}
	}

	// Whether a and b could do anything about touching: both are in play, they
	// are not in lockstep with each other (the invader block never collides with
	// itself), and at least one has a rule that answers to the other.
	bool Game::canCollide(const Object& a, const Object& b) noexcept
	{
		if (!a.collisionData.enabled || !b.collisionData.enabled)
		{
			return false;
		}

		// Something in the middle of a jump is over everything.
		if (a.isAirborne() || b.isAirborne())
		{
			return false;
		}

		if (a.collisionData.lockstep != 0 && a.collisionData.lockstep == b.collisionData.lockstep)
		{
			return false;
		}

		const auto answers = [](const Object& self, const Object& other)
		{
			return std::any_of(self.collisionData.basic.begin(), self.collisionData.basic.end(),
				[&](const CollisionRule& rule) { return collisionRuleMatches(rule, other); });
		};

		return answers(a, b) || answers(b, a);
	}

	Vector2f Game::motionOf(const Object& object) noexcept
	{
		return object.velocity + object.carry;
	}

	bool Game::isMoving(const Object& object) noexcept
	{
		const Vector2f motion = motionOf(object);
		return motion.x != 0 || motion.y != 0 || object.hopped;
	}

	// A constant pull (<acceleration>, gravity being one with only a y) and
	// held thrust (<accelerate>) both change an object's velocity, once a
	// frame, before it is moved. Thrust that burns a variable takes 1 off it
	// every frame it is on, and does nothing while the variable is at 0 or
	// below: a ship out of fuel still falls.
	void Game::applyAcceleration(void)
	{
		for (auto& object : objects)
		{
			// On a ladder nothing pulls it (Game::applyClimbing moves it).
			if (!isShown(object) || object.climbing) { continue; }

			Vector2f change = object.acceleration;

			// Burns one unit of `burn` (an object's own variable), if it names
			// one, and says whether there was any left to burn. A thruster with
			// no fuel does nothing.
			const auto burnFuel = [&](const std::string& burn, const char* verb)
			{
				if (burn.empty()) { return true; }
				const auto fuel = object.variable.find(burn);
				if (fuel == object.variable.end() || fuel->second <= 0.0f) { return false; }
				changeVariable(object.name + "." + burn, -1.0f, verb);
				return true;
			};

			for (std::size_t i = 0; i < object.activeThrust.size(); ++i)
			{
				const float amount = object.activeThrust[i];
				if (amount == 0.0f) { continue; }

				if (!burnFuel(object.activeThrustBurn[i], "accelerate")) { continue; }

				switch (static_cast<Direction>(i))
				{
				case Direction::Up:    change.y -= amount; break;
				case Direction::Down:  change.y += amount; break;
				case Direction::Left:  change.x -= amount; break;
				case Direction::Right: change.x += amount; break;
				}
			}

			// Turning, and thrust along the way the object then faces. A heading
			// is degrees clockwise from straight up.
			if (object.hasHeading)
			{
				const float turn = object.activeTurn[static_cast<std::size_t>(Direction::Right)]
					- object.activeTurn[static_cast<std::size_t>(Direction::Left)];
				if (turn != 0.0f)
				{
					object.heading = std::fmod(object.heading + turn, 360.0f);
					if (object.heading < 0.0f) { object.heading += 360.0f; }
					object.showHeading();
				}

				if (object.activeThrustAhead != 0.0f && burnFuel(object.activeThrustAheadBurn, "thrust"))
				{
					const float radians = object.heading * 3.14159265358979323846f / 180.0f;
					change.x += std::sin(radians) * object.activeThrustAhead;
					change.y -= std::cos(radians) * object.activeThrustAhead;
				}
			}

			object.velocity += change;

			// Drag: what is left of the speed after this frame.
			if (object.drag != 0.0f)
			{
				object.velocity *= 1.0f - object.drag;
			}
		}
	}

	// Climbing (a <climb> in an action, held): an object at a ladder - an
	// object of its climbClass with the climber's middle over it and its feet
	// between the ladder's top and bottom - that is on the ground and asked to
	// go up or down it, and can, gets on it, lined up with its middle. On it,
	// it goes the step held each frame (none held: it stays put), its feet
	// kept between the two ends, with no pull and no landing, and its own
	// velocity at 0 so no key walks it off. Reaching either end it gets off,
	// standing there, and the keys held take it on as before.
	void Game::applyClimbing(void)
	{
		for (auto& object : objects)
		{
			if (object.climbClass.empty() || !isShown(object)) { continue; }

			const Vector2f size = object.size;
			const float middle = object.position.x + size.x / 2.0f;
			const float feet = object.position.y + size.y;

			const auto ladder = std::find_if(objects.begin(), objects.end(), [&](const Object& other)
				{
					return &other != &object && other.objClass == object.climbClass && isShown(other)
						&& middle >= other.position.x && middle <= other.position.x + other.size.x
						&& feet >= other.position.y - 0.5f && feet <= other.position.y + other.size.y + 0.5f;
				});

			if (ladder == objects.end())
			{
				object.climbing = false;
				continue;
			}

			const float top = ladder->position.y;
			const float bottom = ladder->position.y + ladder->size.y;
			const float step = object.activeClimb[1] - object.activeClimb[0];

			if (!object.climbing)
			{
				const bool canGo = (step < 0.0f && feet > top + 0.5f) || (step > 0.0f && feet < bottom - 0.5f);
				if (!canGo || !object.grounded) { continue; }

				object.climbing = true;
				object.leaping = false;
				object.position.x = ladder->position.x + ladder->size.x / 2.0f - size.x / 2.0f;
			}

			const float newFeet = std::clamp(feet + step, top, bottom);
			object.position.y = newFeet - size.y;
			object.velocity = {};

			if (step != 0.0f && (newFeet <= top || newFeet >= bottom))
			{
				object.climbing = false;
				object.velocity.x = object.activeMoveStep[static_cast<std::size_t>(Direction::Right)]
					- object.activeMoveStep[static_cast<std::size_t>(Direction::Left)];
			}
		}
	}

	// Makes the hops queued by hop.*() actions. A hop is a jump, not a slide:
	// the object is simply somewhere else, one step away, before this frame's
	// collisions are worked out, so it is judged by where it lands and not by
	// what it would have brushed past on the way (a step is never more than
	// the object's own lane). A hop that would take it out of the window is
	// refused. Whether or not it went, an object that asked to hop counts as
	// moving for the rest of the frame - it is what makes the collisions at
	// its new spot run even when nothing else there is moving.
	void Game::applyHops(void)
	{
		for (auto& object : objects)
		{
			object.hopped = false;

			// Whatever last frame's collisions said, this frame's start over:
			// what it rides, and whether it stands on something.
			object.carry = {};
			object.grounded = false;

			// A jump under way goes on a step; the frame it lands, it counts as
			// having moved, so it meets whatever it landed on.
			if (object.isAirborne())
			{
				if (isShown(object))
				{
					const Vector2f target = object.position + object.jumpStep;
					const bool inside = target.x >= 0 && target.y >= 0
						&& target.x + object.size.x <= windowDesc.width
						&& target.y + object.size.y <= windowDesc.height;
					if (inside) { object.position = target; }
				}
				--object.jumpFramesLeft;
				object.hopped = true;
			}

			if (object.hopPending.x == 0 && object.hopPending.y == 0)
			{
				continue;
			}

			const Vector2f target = object.position + object.hopPending;
			object.hopPending = {};

			if (!isShown(object)) { continue; }

			const bool inside = target.x >= 0 && target.y >= 0
				&& target.x + object.size.x <= windowDesc.width
				&& target.y + object.size.y <= windowDesc.height;

			if (inside)
			{
				object.position = target;
			}
			object.hopped = true;
		}
	}

	// Plays one frame of movement. Every object that is shown moves by its
	// velocity (and by whatever it is carried at, see Object::carry), but not
	// in one jump: each pair of objects that has a rule for touching is swept
	// along its own path (CollisionDetector::sweep), the earliest touch
	// anywhere is found, everything is moved up to that moment, that pair's
	// rules run, and the rest of the frame carries on from there with
	// whatever velocities the rules left behind - so a ball that hits a
	// brick a third of the way through its move spends the other two thirds
	// heading back the way it came. Each object is swept by itself; a lockstep set
	// has no bounding box of its own, so when most of the invaders are gone
	// only the ones left are tested.
	void Game::moveObjects(void)
	{
		auto& all = getCurrentObjects();

		applyHops();

		struct Pair
		{
			Object* first;
			Object* second;
			bool handled;
		};

		std::vector<Pair> pairs;
		for (std::size_t i = 0; i < all.size(); ++i)
		{
			if (!isShown(all[i]))
			{
				continue;
			}

			for (std::size_t j = i + 1; j < all.size(); ++j)
			{
				if (isShown(all[j]) && canCollide(all[i], all[j]))
				{
					pairs.push_back({ &all[i], &all[j], false });
				}
			}
		}

		const auto advance = [&](float fraction)
		{
			for (auto& object : all)
			{
				if (isShown(object)) // TODO: at some point you might wanna collide with invisible objects
				{
					object.position += motionOf(object) * fraction;
				}
			}
		};

		// Each pair reacts at most once a frame, which also bounds the loop;
		// the count is a second guard.
		float played = 0.0f;
		for (std::size_t reactions = 0; reactions <= pairs.size() && played < 1.0f; ++reactions)
		{
			const float left = 1.0f - played;

			Pair* next = nullptr;
			CollisionDetector::SweepHit nextHit;

			for (auto& pair : pairs)
			{
				Object& a = *pair.first;
				Object& b = *pair.second;

				// Anything already handled, or taken out of play by an earlier
				// reaction this frame (a bullet that just died), sits out.
				if (pair.handled || !isShown(a) || !isShown(b) || !canCollide(a, b) || (!isMoving(a) && !isMoving(b)))
				{
					continue;
				}

				const auto hit = CollisionDetector::sweep(a, motionOf(a) * left, b, motionOf(b) * left);
				if (hit && (!next || hit->time < nextHit.time))
				{
					next = &pair;
					nextHit = *hit;
				}
			}

			if (!next)
			{
				break;
			}

			const float step = nextHit.time * left;
			advance(step);
			played += step;
			next->handled = true;
			applyObjectCollision(*next->first, *next->second, nextHit.edgeOfSecond);
		}

		advance(1.0f - played);

		for (auto& object : all)
		{
			object.hopped = false;
		}
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
	void Game::applyObjectCollision(Object& a, Object& b, Edge edgeOfB)
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
		// wasteful.
		const State& state = currentState.top();

		for (auto& condition : state.conditions)
		{
			// "No more than N of them left": everything that matches, still
			// in play. Nothing to look at per object, so this is its own case.
			if (condition.remaining)
			{
				const auto stillIn = std::count_if(objects.begin(), objects.end(), [&](const Object& object)
					{
						return object.isVisible && matchesClassOrObjectFilter(condition.filterClass, condition.filterObject, object);
					});

				if (static_cast<float>(stillIn) <= *condition.remaining)
				{
					runConditionCommands(condition.commands);
					return; // the state may have just changed - stop for this frame
				}

				continue;
			}

			for (auto& object : objects)
			{
				if (!matchesClassOrObjectFilter(condition.filterClass, condition.filterObject, object))
				{
					continue;
				}

				auto variableIt = object.variable.find(condition.variableName);
				if (variableIt == object.variable.end())
				{
					continue;
				}

				// Reached the threshold going up (value), or fallen to it
				// going down (atmost).
				const bool met = condition.atMost
					? variableIt->second <= *condition.atMost
					: variableIt->second >= condition.value;
				if (!met)
				{
					continue;
				}

				runConditionCommands(condition.commands);
				return; // the state may have just changed - stop for this frame
			}
		}
	}

	// A copy of the commands, not the state's own list: the list belongs to the
	// current state, and a <pop /> among them frees that state while the rest
	// of the list is still to run.
	void Game::runConditionCommands(std::vector<Command> commands)
	{
		CommandExecutor executor(*this);
		for (const auto& command : commands)
		{
			executor.executeCondition(command);
		}
	}
}
