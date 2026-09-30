// game.cpp
// XML Game Engine
// author: beefviper
// date: Sept 18, 2020

#include "game.h"

#include "command_executor.h"

#include <algorithm>
#include <cstddef>
#include <stdexcept>
#include <variant>

namespace xge
{
	Game::Game(const std::string& game, XmlBackend xmlBackend) :
		filename(game)
	{
		xml.init(filename, xmlBackend, windowDesc, variables, rawStates, rawObjects, xmlValidation);
		expr.init(windowDesc, variables, rawStates, states, rawObjects, objects);
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

			object.positionOriginal.x = expr.evaluateString(*rawObject, rawObject->rawPosition.x);
			object.positionOriginal.y = expr.evaluateString(*rawObject, rawObject->rawPosition.y);
			object.positionOriginal = object.positionOriginal + object.gridOffset;
			object.position = object.positionOriginal;
			object.positionSizesUsed = sizes;
			object.positionResolved = true;
		}
	}

	void Game::updateObjects(void)
	{
		auto& currentObjects = getCurrentObjects();

		// Screen-edge checks: independent per object, order doesn't matter.
		for (auto& object : currentObjects)
		{
			if (isShown(object) && object.collisionData.enabled && (object.velocity.x != 0 || object.velocity.y != 0))
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
			if (shown == object.name || shown == object.baseName)
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
	// (aliens) finds the first object made from it, which for anything that is
	// not a grid is the object itself.
	Object* Game::tryGetObject(const std::string& name) noexcept
	{
		const auto exact = std::find_if(std::begin(objects), std::end(objects), [&](const Object& obj) { return obj.name == name; });
		if (exact != std::end(objects))
		{
			return &(*exact);
		}

		const auto first = std::find_if(std::begin(objects), std::end(objects), [&](const Object& obj) { return obj.baseName == name; });
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
		currentState.push(states.at(index));
		++stateChanges;
	}

	void Game::setCurrentState(const std::string& name)
	{
		auto result = std::find_if(std::begin(states), std::end(states), [&](State& state) { return state.name == name; });
		currentState.push(*result);
		++stateChanges;
	}

	void Game::pushState(std::string name) {
		auto result = std::find_if(std::begin(states), std::end(states), [&](State& state) { return state.name == name; });
		currentState.push(*result);
		++stateChanges;
	}

	void Game::popState(void) noexcept
	{
		currentState.pop();
		++stateChanges;
	}

	unsigned long Game::stateChangeCount(void) const noexcept
	{
		return stateChanges;
	}

	void Game::setObjectParam(const std::string& name, const std::string& param, const float& value)
	{
		auto result = std::find_if(std::begin(objects), std::end(objects), [&](Object& obj) { return obj.name == name; });
		if (param == "velocity")
		{
			result->velocity.y = value;
		}
	}

	void Game::incrementText(const std::string& target)
	{
		const auto dot = target.find('.');

		if (dot == std::string::npos)
		{
			// Legacy pattern (e.g. inc('score1') for a text object that just
			// displays its own counter, with nothing else deriving its number):
			// the named object displays and owns its own number - bump its
			// displayed value directly.
			Object* object = tryGetObject(target);
			if (!object)
			{
				std::cout << "warning: inc('" << target << "'): no such object\n";
				return;
			}
			const float newValue = std::stof(object->spriteParams.at(1)) + 1;
			object->spriteParams.at(1) = formatDisplayNumber(newValue);
			object->visualDirty = true;
			return;
		}

		// New pattern (e.g. inc('paddle1.score')): increment another object's
		// own named <variable>, then refresh every text object whose displayed
		// number is bound to it (Object::boundVariableOwner/boundVariableName,
		// set from a sprite like text(paddle1.score,128,...) - see
		// parseTextVariableBinding).
		const std::string ownerName = target.substr(0, dot);
		const std::string variableName = target.substr(dot + 1);

		Object* owner = tryGetObject(ownerName);
		if (!owner)
		{
			std::cout << "warning: inc('" << target << "'): no object named '" << ownerName << "'\n";
			return;
		}

		auto variableIt = owner->variable.find(variableName);
		if (variableIt == owner->variable.end())
		{
			std::cout << "warning: inc('" << target << "'): '" << ownerName << "' has no variable named '" << variableName << "'\n";
			return;
		}

		variableIt->second += 1;
		const float newValue = variableIt->second;

		for (auto& object : objects)
		{
			if (object.boundVariableOwner == ownerName && object.boundVariableName == variableName)
			{
				object.spriteParams.at(1) = formatDisplayNumber(newValue);
				object.visualDirty = true;
			}
		}
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

			// Also drop any directions CommandExecutor::applyActionVelocity
			// currently has recorded as held: velocity above is reset to rest,
			// so a stale held direction (a key never released across this
			// reset) shouldn't be able to resurrect part of the old velocity
			// the next time some other direction's key event recomputes it.
			object.activeMoveStep = {};

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
			std::cout << "warning: reset('" << name << "'): no such object\n";
			return;
		}

		// A name from the XML (aliens) means every object made from it - the
		// whole grid - and an exact one (aliens.3.2) just that one.
		for (auto& candidate : objects)
		{
			if (candidate.name == name || candidate.baseName == name)
			{
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
				other.spriteParams.at(1) = formatDisplayNumber(valueIt->second);
				other.visualDirty = true;
			}
		}
	}

	void Game::resetAll()
	{
		for (auto& object : objects)
		{
			resetObjectState(object);
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
				object.spriteParams.at(1) = formatDisplayNumber(valueIt->second);
				object.visualDirty = true;
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

	void Game::updateGroupOfObjects(const Object& object) noexcept
	{
		const int groupNum = object.collisionData.group;

		// Every member gets exactly the same treatment. This used to shift the
		// members stored before the touching one by three steps and the rest
		// by one (and repeated that on every right-hand bounce), which pushed
		// the last column of a grid() further from the others each time.
		for (auto& obj : objects)
		{
			if (obj.collisionData.group == groupNum)
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
			if (!filterObject.empty() && filterObject != candidate.name && filterObject != candidate.baseName) { return false; }
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
	// are not members of the same group (the invader block never collides with
	// itself), and at least one has a rule that answers to the other.
	bool Game::canCollide(const Object& a, const Object& b) noexcept
	{
		if (!a.collisionData.enabled || !b.collisionData.enabled)
		{
			return false;
		}

		if (a.collisionData.group != 0 && a.collisionData.group == b.collisionData.group)
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

	// Plays one frame of movement. Every object that is shown moves by its
	// velocity, but not in one jump: each pair of objects that has a rule for
	// touching is swept along its own path (CollisionDetector::sweep), the
	// earliest touch anywhere is found, everything is moved up to that moment,
	// that pair's rules run, and the rest of the frame carries on from there
	// with whatever velocities the rules left behind - so a ball that hits a
	// brick a third of the way through its move spends the other two thirds
	// heading back the way it came. Each object is swept by itself; a group
	// has no bounding box of its own, so when most of the invaders are gone
	// only the ones left are tested.
	void Game::moveObjects(void)
	{
		auto& all = getCurrentObjects();

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

		const auto isMoving = [](const Object& object) { return object.velocity.x != 0 || object.velocity.y != 0; };
		const auto advance = [&](float fraction)
		{
			for (auto& object : all)
			{
				if (isShown(object)) // TODO: at some point you might wanna collide with invisible objects
				{
					object.position += object.velocity * fraction;
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

				const auto hit = CollisionDetector::sweep(a, a.velocity * left, b, b.velocity * left);
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
	}

	// edgeOfB is the edge of b that was touched, which is already b's own side
	// of it; a's own touched edge is the opposite one.
	void Game::applyObjectCollision(Object& a, Object& b, Edge edgeOfB)
	{
		CommandExecutor executor(*this);
		for (const auto& rule : a.collisionData.basic)
		{
			if (!collisionRuleMatches(rule, b)) { continue; }
			for (const auto& command : rule.commands)
			{
				executor.executeObjectCollision(command, a, opposite(edgeOfB));
			}
		}
		for (const auto& rule : b.collisionData.basic)
		{
			if (!collisionRuleMatches(rule, a)) { continue; }
			for (const auto& command : rule.commands)
			{
				executor.executeObjectCollision(command, b, edgeOfB);
			}
		}
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
			for (auto& object : objects)
			{
				if (!matchesClassOrObjectFilter(condition.filterClass, condition.filterObject, object))
				{
					continue;
				}

				auto variableIt = object.variable.find(condition.variableName);
				if (variableIt == object.variable.end() || variableIt->second < condition.value)
				{
					continue;
				}

				CommandExecutor executor(*this);
				for (const auto& command : condition.commands)
				{
					executor.executeCondition(command);
				}
				return; // the state may have just changed - stop for this frame
			}
		}
	}
}
