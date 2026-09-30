// game_expr.cpp
// XML Game Engine
// author: beefviper
// date: Sept 21, 2020

#include "game_expr.h"

#include "keycode.h"

#include <algorithm>
#include <cctype>
#include <stdexcept>

namespace xge
{
	void game_expr::init(const WindowDesc& windowDesc,
		const std::vector<std::pair<std::string, RawValue>>& rawVariables, std::map<std::string, float>& variables,
		std::vector<RawState>& rawStates, std::vector<State>& states,
		std::vector<RawObject>& rawObjects, std::vector<Object>& objects)
	{
		generator.seed(seed());

		// add constants to symbol table
		symbolTable.add_constant("window.top", 0);
		symbolTable.add_constant("window.bottom", windowDesc.height);
		symbolTable.add_constant("window.left", 0);
		symbolTable.add_constant("window.right", windowDesc.width);
		symbolTable.add_constant("window.width.center", windowDesc.width / 2);
		symbolTable.add_constant("window.height.center", windowDesc.height / 2);

		// register symbol table with expression
		expression.register_symbol_table(symbolTable);

		// add variables from XML file to symbol table, in the order the file
		// declares them so that one can use the ones before it
		for (const auto& [name, rawValue] : rawVariables)
		{
			const float value = evaluate(rawValue, "variable '" + name + "'");
			variables[name] = value;
			symbolTable.add_constant(name, value);
		}

		// Pre-register every object's own <variable> entries, keyed
		// "ownerName.variableName", so any expression evaluated below can refer
		// to another object's variable regardless of which object appears first
		// in the XML (e.g. score1's sprite reading paddle1.score, even though
		// paddle1 is declared after score1). Values start at 0 here; the object
		// loop further down fills in the real evaluated value once it reaches
		// that object - see the objectVariables[...] assignment there.
		for (auto& rawObject : rawObjects)
		{
			for (auto& rawVariable : rawObject.variable)
			{
				objectVariables[rawObject.name + "." + rawVariable.first] = 0.0f;
			}
		}
		for (auto& objectVariable : objectVariables)
		{
			symbolTable.add_variable(objectVariable.first, objectVariable.second);
		}

		// Every object's size as name.width / name.height, so an expression can
		// place an object by its own size (or another's) the same way it reads
		// paddle1.score. Worked out before anything is placed so the order
		// objects appear in the file does not matter; a text's or image's is
		// {0,0} for now (see objectSizes). An object's own <variable> named
		// width or height takes the name instead.
		//
		// The sprite of each object is worked out here once, and a shape (unlike
		// a text showing a variable, whose number may have changed by then) is
		// kept as it came out, so that a <random> in it gives the size used here
		// and the size drawn the same number.
		std::map<std::string, std::vector<std::string>> firstPassSpriteParams;
		for (auto& rawObject : rawObjects)
		{
			if (objectSizes.count(rawObject.name))
			{
				continue; // every cell of a <grid> comes from this one object
			}

			const std::vector<std::string> params = buildSpriteParams(rawObject.sprite, "object '" + rawObject.name + "'");
			const ShapeKind kind = shapeKindFromTag(params.empty() ? std::string{} : params.at(0));
			objectShapeKinds[rawObject.name] = kind;
			objectSizes[rawObject.name] = measureShapeSize(params, kind);
			firstPassSpriteParams[rawObject.name] = params;
		}
		for (auto& [name, size] : objectSizes)
		{
			if (!objectVariables.count(name + ".width")) { symbolTable.add_variable(name + ".width", size.x); }
			if (!objectVariables.count(name + ".height")) { symbolTable.add_variable(name + ".height", size.y); }
		}

		int lockstepNum = 1;

		// evaluate strings in objects
		for (auto& rawObject : rawObjects)
		{
			const std::string where = "object '" + rawObject.name + "'";

			// Only a text showing a number needs working out again here, for the
			// value it has by now (see above).
			std::vector<std::string> tempSpriteParams = (rawObject.sprite.kind == "text" && rawObject.sprite.textIsNumber)
				? buildSpriteParams(rawObject.sprite, where)
				: firstPassSpriteParams.at(rawObject.name);
			const GridData gridData = gridDataOf(rawObject.sprite, where);
			const ShapeKind rawObjectShapeKind = shapeKindFromTag(tempSpriteParams.empty() ? std::string{} : tempSpriteParams.at(0));

			// Every grid cell shares the same footprint (one <circle> or
			// <rectangle> covers the whole <grid> - see games/breakout.xml,
			// games/spaceinvaders.xml), so this is computed
			// once per rawObject rather than per cell.
			const Vector2f gridObjSize = measureShapeSize(tempSpriteParams, rawObjectShapeKind);

			// A position that uses the size of a text or image (its own, or
			// another object's) can only be finished once a backend has measured
			// it - see Object::positionUsesSize. Sizes of shapes are exact
			// already (objectSizes above).
			const std::vector<std::string> positionSizeDependencies = sizeDependenciesOf(rawObject);

			if ((gridData.max.x > 1 || gridData.max.y > 1)
				&& (rawObjectShapeKind == ShapeKind::Text || rawObjectShapeKind == ShapeKind::Image))
			{
				std::cout << "warning: <grid>: '" << rawObject.name << "' is a "
					<< (rawObjectShapeKind == ShapeKind::Text ? "text" : "image")
					<< " object - its real footprint can't be known without a Window "
					<< "backend (see measureShapeSize, command.cpp), so grid spacing "
					<< "here will collapse to just the padding\n";
			}

			const bool isGrid = gridData.max.x > 1 || gridData.max.y > 1;

			for (auto gridX = 0; gridX < gridData.max.x; gridX++)
			{
				for (auto gridY = 0; gridY < gridData.max.y; gridY++)
				{
					Object object{};

					object.spriteParams = tempSpriteParams;
					object.shapeKind = rawObjectShapeKind;

					if (object.shapeKind == ShapeKind::Text && rawObject.sprite.textIsNumber
						&& rawObject.sprite.number.kind == RawValue::Kind::Expression)
					{
						if (auto binding = parseVariableReference(rawObject.sprite.number.text))
						{
							object.boundVariableOwner = binding->first;
							object.boundVariableName = binding->second;
						}
					}

					// A grid gets one name per cell (aliens.3.2 - column, row,
					// from 1) so each cell can be found, hit and removed on
					// its own; a plain object keeps the name it was given.
					object.baseName = rawObject.name;
					object.name = isGrid
						? rawObject.name + "." + std::to_string(gridX + 1) + "." + std::to_string(gridY + 1)
						: rawObject.name;
					object.objClass = rawObject.objClass;

					if (rawObject.objClass == "projectile")
					{
						rawObject.isVisible = false;
					}

					object.isVisible = rawObject.isVisible;

					object.positionOriginal.x = evaluate(rawObject.rawPosition.x, where);
					object.positionOriginal.y = evaluate(rawObject.rawPosition.y, where);

					// Finalizes this grid cell's real screen position right
					// here - Game is now done with position the moment its
					// own constructor returns, with no Window/backend needed
					// (see main.cpp). A non-grid object always has
					// gridData.max == {1,1}, so gridX == gridY == 0 and this
					// reduces to plain object.position = object.positionOriginal,
					// same as it always has. positionOriginal is then bumped
					// to match, exactly as before, so Game::resetObject/
					// resetAll restores each grid cell (e.g. each brick) to
					// its own slot, not the shared base corner.
					object.gridOffset.x = (gridObjSize.x + static_cast<float>(gridData.padding.x)) * static_cast<float>(gridX);
					object.gridOffset.y = (gridObjSize.y + static_cast<float>(gridData.padding.y)) * static_cast<float>(gridY);
					object.position = object.positionOriginal + object.gridOffset;
					object.positionOriginal = object.position;

					object.sizeDependencies = positionSizeDependencies;
					object.positionUsesSize = !positionSizeDependencies.empty();
					object.positionResolved = !object.positionUsesSize;

					object.velocity.x = evaluate(rawObject.rawVelocity.x, where);
					object.velocity.y = evaluate(rawObject.rawVelocity.y, where);

					object.velocityOriginal = object.velocity;

					object.collisionData.enabled = rawObject.rawCollisionData.enabled;
					object.isVisibleOriginal = object.isVisible;
					object.collisionEnabledOriginal = object.collisionData.enabled;
					object.collisionData.lockstep = rawObject.rawCollisionData.lockstep ? lockstepNum : 0;

					object.collisionData.top = processCommands(rawObject.rawCollisionData.top, where);
					object.collisionData.bottom = processCommands(rawObject.rawCollisionData.bottom, where);
					object.collisionData.left = processCommands(rawObject.rawCollisionData.left, where);
					object.collisionData.right = processCommands(rawObject.rawCollisionData.right, where);

					for (auto& rawRule : rawObject.rawCollisionData.basic)
					{
						CollisionRule rule;
						rule.filterClass = rawRule.filterClass;
						rule.filterObject = rawRule.filterObject;
						rule.unlessClass = rawRule.unlessClass;
						rule.commands = processCommands(rawRule.commands, where);
						object.collisionData.basic.push_back(std::move(rule));
					}

					//object.action = rawObject.action;
					for (auto& rawAction : rawObject.action)
					{
						object.action[rawAction.first] = processCommands(rawAction.second, where);
					}

					for (auto& rawVariable : rawObject.variable)
					{
						const float value = evaluate(rawVariable.second, where);
						object.variable[rawVariable.first] = value;
						object.variableOriginal[rawVariable.first] = value;

						// Keep the cross-object symbol table entry (registered above,
						// before any expression compiled) up to date with the real
						// value now that it's known.
						objectVariables[rawObject.name + "." + rawVariable.first] = value;
					}

					// object's visual is built later by whichever Window backend is
					// running, once one exists (see Window::init() in window.h) -
					// visualDirty starts true (Object's own default), so nothing
					// needs to happen here.
					objects.push_back(std::move(object));
				}
			}

			if (rawObject.rawCollisionData.lockstep) {
				lockstepNum++;
			}
		}

		for (auto& rawState : rawStates)
		{
			State state{};
			const std::string where = "state '" + rawState.name + "'";

			state.name = rawState.name;
			state.show = rawState.show;

			for (auto& rawAction : rawState.input)
			{
				state.input[keyCodeFromString(rawAction.first)] = processCommands(rawAction.second, where);
			}

			for (auto& rawCondition : rawState.conditions)
			{
				Condition condition;
				condition.filterClass = rawCondition.filterClass;
				condition.filterObject = rawCondition.filterObject;
				condition.variableName = rawCondition.variableName;

				const float threshold = evaluate(rawCondition.threshold, where);
				switch (rawCondition.test)
				{
				case RawCondition::Test::AtLeast:   condition.value = threshold; break;
				case RawCondition::Test::AtMost:    condition.atMost = threshold; break;
				case RawCondition::Test::Remaining: condition.remaining = threshold; break;
				}

				if (condition.remaining && std::none_of(objects.begin(), objects.end(), [&](const Object& object)
					{
						return (rawCondition.filterClass.empty() || rawCondition.filterClass == object.objClass)
							&& (rawCondition.filterObject.empty() || rawCondition.filterObject == object.name || rawCondition.filterObject == object.baseName);
					}))
				{
					std::cout << "warning: state '" << rawState.name << "': a condition with remaining= matches no object at all, so it would fire at once\n";
				}
				condition.commands = processCommands(rawCondition.commands, where);
				state.conditions.push_back(std::move(condition));
			}

			states.push_back(state);
		}
	}

	float game_expr::evaluate(const RawValue& value, const std::string& where)
	{
		if (value.kind == RawValue::Kind::Random)
		{
			const float min = evaluateExpression(value.min, where);
			const float max = evaluateExpression(value.max, where);
			return randomNumberRange(min, max);
		}

		return evaluateExpression(value.text, where);
	}

	float game_expr::evaluateExpression(const std::string& text, const std::string& where)
	{
		if (!parser.compile(text, expression))
		{
			throw std::runtime_error(where + ": cannot read \"" + text + "\": " + parser.error().c_str());
		}
		return expression.value();
	}

	std::vector<Command> game_expr::processCommands(const std::vector<RawCommand>& raw, const std::string& where)
	{
		try
		{
			return makeCommands(raw, [&](const RawValue& value) { return evaluate(value, where); });
		}
		catch (const std::runtime_error& error)
		{
			throw std::runtime_error(where + ": " + error.what());
		}
	}

	std::vector<std::string> game_expr::buildSpriteParams(const RawSprite& sprite, const std::string& where)
	{
		const std::string color = sprite.color.empty() ? "color.white" : sprite.color;
		std::vector<std::string> params;

		if (sprite.kind == "circle")
		{
			params = { "circle", std::to_string(evaluate(sprite.radius, where)), std::to_string(0), color };
		}
		else if (sprite.kind == "rectangle")
		{
			params = { "rectangle", std::to_string(evaluate(sprite.width, where)), std::to_string(evaluate(sprite.height, where)), color };
		}
		else if (sprite.kind == "text")
		{
			// A fixed label, or a number - shown as the whole number it is, with
			// no decimal point, the way someone typing the label would.
			const std::string label = sprite.textIsNumber ? formatDisplayNumber(evaluate(sprite.number, where)) : sprite.content;
			params = { "text", label, std::to_string(evaluate(sprite.size, where)), color };
		}
		else if (sprite.kind == "image")
		{
			params = { "image", sprite.path, sprite.flip.empty() ? "color.white" : "flip." + sprite.flip };
		}
		else
		{
			throw std::runtime_error(where + ": unknown shape '" + sprite.kind + "'");
		}

		if (sprite.isGrid)
		{
			params.push_back("grid");
			params.push_back(std::to_string(evaluate(sprite.columns, where)));
			params.push_back(std::to_string(evaluate(sprite.rows, where)));
			params.push_back(sprite.hasPadding ? std::to_string(evaluate(sprite.padding.x, where)) : std::to_string(0));
			params.push_back(sprite.hasPadding ? std::to_string(evaluate(sprite.padding.y, where)) : std::to_string(0));
		}

		return params;
	}

	xge::GridData game_expr::gridDataOf(const RawSprite& sprite, const std::string& where)
	{
		GridData gridData;

		if (sprite.isGrid)
		{
			gridData.max.x = static_cast<int>(evaluate(sprite.columns, where));
			gridData.max.y = static_cast<int>(evaluate(sprite.rows, where));

			if (sprite.hasPadding)
			{
				gridData.padding.x = static_cast<int>(evaluate(sprite.padding.x, where));
				gridData.padding.y = static_cast<int>(evaluate(sprite.padding.y, where));
			}
		}

		return gridData;
	}

	void game_expr::setObjectSize(const std::string& name, const Vector2f& size)
	{
		if (auto it = objectSizes.find(name); it != objectSizes.end())
		{
			it->second = size;
		}
	}

	std::vector<std::string> game_expr::sizeDependenciesOf(const RawObject& rawObject) const
	{
		std::vector<std::string> dependencies;

		// Every identifier in the position expressions, e.g. "title.width" or
		// "window.width.center"; the ones that end in .width / .height and name
		// an object whose size needs a backend are what this is looking for.
		std::vector<const std::string*> positionExpressions = rawObject.rawPosition.x.expressions();
		for (const std::string* expression : rawObject.rawPosition.y.expressions())
		{
			positionExpressions.push_back(expression);
		}

		for (const std::string* positionExpression : positionExpressions)
		{
			std::size_t i = 0;
			while (i < positionExpression->size())
			{
				const auto isIdentifierChar = [](char c) { return std::isalnum(static_cast<unsigned char>(c)) || c == '_' || c == '.'; };
				if (!isIdentifierChar((*positionExpression)[i]))
				{
					++i;
					continue;
				}

				std::size_t end = i;
				while (end < positionExpression->size() && isIdentifierChar((*positionExpression)[end])) { ++end; }
				const std::string token = positionExpression->substr(i, end - i);
				i = end;

				for (const std::string suffix : { ".width", ".height" })
				{
					if (token.size() <= suffix.size() || token.compare(token.size() - suffix.size(), suffix.size(), suffix) != 0)
					{
						continue;
					}

					const std::string name = token.substr(0, token.size() - suffix.size());
					const auto kind = objectShapeKinds.find(name);
					if (kind != objectShapeKinds.end() && sizeNeedsBackend(kind->second)
						&& !objectVariables.count(token)
						&& std::find(dependencies.begin(), dependencies.end(), name) == dependencies.end())
					{
						dependencies.push_back(name);
					}
				}
			}
		}

		return dependencies;
	}
}
