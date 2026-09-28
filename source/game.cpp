// game.cpp
// XML Game Engine
// author: beefviper
// date: Sept 18, 2020

#include "game.h"

#include "command_executor.h"

#include <cstddef>

namespace xge
{
	Game::Game(const std::string& game) :
		filename(game)
	{
		xml.init(filename, windowDesc, variables, rawStates, rawObjects);
		expr.init(windowDesc, variables, rawStates, states, rawObjects, objects);
		sfml.init(objects);
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

		// Object-object checks: every unordered pair is looked at once. A
		// simple bounding-box test is enough for two rectangles; when either
		// side is a circle (the ball, a bullet) checkObjectCollision looks
		// closer to make sure it's actually touching, not just its bounding
		// box overlapping.
		for (std::size_t i = 0; i < currentObjects.size(); ++i)
		{
			if (!isShown(currentObjects[i]))
			{
				continue;
			}

			for (std::size_t j = i + 1; j < currentObjects.size(); ++j)
			{
				if (!isShown(currentObjects[j]))
				{
					continue;
				}

				checkObjectCollision(currentObjects[i], currentObjects[j]);
			}
		}

		// TODO: currently only have position and velocity, will probably need acceleration too
		for (auto& object : currentObjects)
		{
			if (isShown(object)) // TODO: at some point you might wanna collide with invisible objects
			{
				object.position.x += object.velocity.x;
				object.position.y += object.velocity.y;

				object.sprite->setPosition(object.position);
			}
		}

		checkConditions();
	}

	void Game::printGame(void)
	{
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
			if (shown == object.name)
			{
				result = true;
			}
		}
		return result && object.isVisible;
	}

	Object& Game::getObject(const std::string& name)
	{
		auto result = std::find_if(std::begin(objects), std::end(objects), [&](Object& obj) { return obj.name == name; });
		return *result;
	}

	Object* Game::tryGetObject(const std::string& name) noexcept
	{
		auto result = std::find_if(std::begin(objects), std::end(objects), [&](Object& obj) { return obj.name == name; });
		return (result == std::end(objects)) ? nullptr : &(*result);
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
	}

	void Game::setCurrentState(const std::string& name)
	{
		auto result = std::find_if(std::begin(states), std::end(states), [&](State& state) { return state.name == name; });
		currentState.push(*result);
	}

	void Game::pushState(std::string name) {
		auto result = std::find_if(std::begin(states), std::end(states), [&](State& state) { return state.name == name; });
		currentState.push(*result);
	}

	void Game::popState(void) noexcept
	{
		currentState.pop();
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
			sfml.updateTextIncrementValue(*object);
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
				sfml.setDisplayedNumber(object, newValue);
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

		resetObjectState(*object);

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
				sfml.setDisplayedNumber(other, valueIt->second);
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
				sfml.setDisplayedNumber(object, valueIt->second);
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

	void Game::updateGroupOfObjects(const Object& object, std::string side) noexcept
	{
		const int groupNum = object.collisionData.group;

		bool foundObject = false;

		for (auto& obj : objects)
		{
			if (obj.collisionData.group == groupNum)
			{
				// TODO: fix logic, no need to set a variable, and then test it in the next block
				// get rid of varialbe foundObject, combine if, if, and else
				if (obj.position.x == object.position.x && obj.position.y == object.position.y)
				{
					foundObject = true;
				}

				if (foundObject == false)
				{
					obj.velocity.x *= -1;
					if (side == "right") { obj.position.x += obj.velocity.x * 3; }
					if (side == "left") { obj.position.x += obj.velocity.x; }
				}
				else
				{
					obj.velocity.x *= -1;
					if (side == "right") { obj.position.x += obj.velocity.x; }
					if (side == "left") { obj.position.x += obj.velocity.x; }
				}
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
			if (!filterObject.empty() && filterObject != candidate.name) { return false; }
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

	void Game::checkObjectCollision(Object& a, Object& b)
	{
		if (!a.collisionData.enabled || !b.collisionData.enabled)
		{
			return;
		}

		// Objects rigidly moving together as a group (e.g. the invader block
		// in spaceinvaders) never collide with each other.
		if (a.collisionData.group != 0 && a.collisionData.group == b.collisionData.group)
		{
			return;
		}

		// At least one side has to have moved for there to be anything new to
		// detect - two stationary objects can't have just started touching.
		const bool aMoved = (a.velocity.x != 0 || a.velocity.y != 0);
		const bool bMoved = (b.velocity.x != 0 || b.velocity.y != 0);
		if (!aMoved && !bMoved)
		{
			return;
		}

		// A plain bounding-box test is exact for two rectangles; when either
		// side is a circle, look closer. Either way this comes back as
		// "the edge of b that was touched" (see the comment on CollisionDetector).
		std::optional<Edge> edgeOfB;
		if (a.shapeKind == ShapeKind::Circle)
		{
			edgeOfB = CollisionDetector::circleRectangle(a, b);
		}
		else if (b.shapeKind == ShapeKind::Circle)
		{
			const auto edgeOfA = CollisionDetector::circleRectangle(b, a);
			edgeOfB = edgeOfA ? std::optional<Edge>(opposite(*edgeOfA)) : std::nullopt;
		}
		else
		{
			edgeOfB = CollisionDetector::rectangleRectangle(a, b);
		}

		if (!edgeOfB)
		{
			return;
		}

		// edgeOfB is self-relative for b already; a's own self-relative
		// touched edge is the opposite side.
		CommandExecutor executor(*this);
		for (const auto& rule : a.collisionData.basic)
		{
			if (!collisionRuleMatches(rule, b)) { continue; }
			for (const auto& command : rule.commands)
			{
				executor.executeObjectCollision(command, a, opposite(*edgeOfB));
			}
		}
		for (const auto& rule : b.collisionData.basic)
		{
			if (!collisionRuleMatches(rule, a)) { continue; }
			for (const auto& command : rule.commands)
			{
				executor.executeObjectCollision(command, b, *edgeOfB);
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
