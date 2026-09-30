// game_expr.cpp
// XML Game Engine
// author: beefviper
// date: Sept 21, 2020

#include "game_expr.h"

#include "keycode.h"

#include <algorithm>
#include <cctype>

namespace xge
{
	void game_expr::init(const WindowDesc& windowDesc, std::map<std::string, float>& variables,
		std::vector<RawState>& rawStates, std::vector<State>& states,
		std::vector<RawObject>& rawObjects, std::vector<Object>& objects)
	{
		generator.seed(seed());

		// custom functions added to exprtk
		randomNumber<float> randomNumberFloat{};
		randomRange<float> randomRangeFloat{};
		shapeCircle<float> shapeCircleFloat{};
		shapeRectangle<float> shapeRectangleFloat{};
		text<float> textFloat{};
		image<float> imageFloat{};
		inc<float> incFloat{};
		grid<float> gridFloat{};
		bounce<float> bounceFloat{};
		stick<float> stickFloat{};
		reset<float> resetFloat{};
		die<float> dieFloat{};
		moveUp<float> moveUpFloat{};
		moveDown<float> moveDownFloat{};
		moveLeft<float> moveLeftFloat{};
		moveRight<float> moveRightFloat{};
		state<float> stateFloat{};
		action<float> actionFloat{};
		fire<float> fireFloat{};

		// add functions to symbol table
		symbolTable.add_function("random.number", randomNumberFloat);
		symbolTable.add_function("random.range", randomRangeFloat);
		symbolTable.add_function("shape.circle", shapeCircleFloat);
		symbolTable.add_function("shape.rectangle", shapeRectangleFloat);
		symbolTable.add_function("text", textFloat);
		symbolTable.add_function("image", imageFloat);
		symbolTable.add_function("inc", incFloat);
		symbolTable.add_function("grid", gridFloat);
		symbolTable.add_function("bounce", bounceFloat);
		symbolTable.add_function("stick", stickFloat);
		symbolTable.add_function("reset", resetFloat);
		symbolTable.add_function("die", dieFloat);
		symbolTable.add_function("move.up", moveUpFloat);
		symbolTable.add_function("move.down", moveDownFloat);
		symbolTable.add_function("move.left", moveLeftFloat);
		symbolTable.add_function("move.right", moveRightFloat);
		symbolTable.add_function("state", stateFloat);
		symbolTable.add_function("action", actionFloat);
		symbolTable.add_function("fire", fireFloat);

		// add constants to symbol table
		symbolTable.add_constant("window.top", 0);
		symbolTable.add_constant("window.bottom", windowDesc.height);
		symbolTable.add_constant("window.left", 0);
		symbolTable.add_constant("window.right", windowDesc.width);
		symbolTable.add_constant("window.width.center", windowDesc.width / 2);
		symbolTable.add_constant("window.height.center", windowDesc.height / 2);

		// add variables from XML file to symbol table
		for (auto& variable : variables)
		{
			symbolTable.add_constant(variable.first, variable.second);
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

		// register symbol table with expression
		expression.register_symbol_table(symbolTable);

		// Every object's size as name.width / name.height, so an expression can
		// place an object by its own size (or another's) the same way it reads
		// paddle1.score. Worked out before anything is placed so the order
		// objects appear in the file does not matter; a text's or image's is
		// {0,0} for now (see objectSizes). An object's own <variable> named
		// width or height takes the name instead.
		for (auto& rawObject : rawObjects)
		{
			if (objectSizes.count(rawObject.name))
			{
				continue; // every cell of a grid() shares its object's name
			}

			const std::vector<std::string> params = processData(rawObject, rawObject.src);
			const ShapeKind kind = shapeKindFromTag(params.empty() ? std::string{} : params.at(0));
			objectShapeKinds[rawObject.name] = kind;
			objectSizes[rawObject.name] = measureShapeSize(params, kind);
		}
		for (auto& [name, size] : objectSizes)
		{
			if (!objectVariables.count(name + ".width")) { symbolTable.add_variable(name + ".width", size.x); }
			if (!objectVariables.count(name + ".height")) { symbolTable.add_variable(name + ".height", size.y); }
		}

		int groupNum = 1;

		// evaluate strings in objects
		for (auto& rawObject : rawObjects)
		{
			std::vector<std::string> tempSpriteParams = processData(rawObject, rawObject.src);
			const GridData gridData = setGridXY(tempSpriteParams);
			const ShapeKind rawObjectShapeKind = shapeKindFromTag(tempSpriteParams.empty() ? std::string{} : tempSpriteParams.at(0));

			// Every grid cell shares the same footprint (one shape.circle()/
			// shape.rectangle() call covers the whole grid() - see games/
			// breakout.xml, games/spaceinvaders.xml), so this is computed
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
				std::cout << "warning: grid(): '" << rawObject.name << "' is a "
					<< (rawObjectShapeKind == ShapeKind::Text ? "text" : "image")
					<< " object - its real footprint can't be known without a Window "
					<< "backend (see measureShapeSize, command.cpp), so grid spacing "
					<< "here will collapse to just the padding\n";
			}

			for (auto gridX = 0; gridX < gridData.max.x; gridX++)
			{
				for (auto gridY = 0; gridY < gridData.max.y; gridY++)
				{
					Object object{};

					object.spriteParams = tempSpriteParams;
					object.shapeKind = rawObjectShapeKind;

					if (object.shapeKind == ShapeKind::Text)
					{
						if (auto binding = parseTextVariableBinding(rawObject.src))
						{
							object.boundVariableOwner = binding->first;
							object.boundVariableName = binding->second;
						}
					}

					object.name = rawObject.name;
					object.objClass = rawObject.objClass;
					object.src = rawObject.src;

					if (rawObject.objClass == "projectile")
					{
						rawObject.isVisible = false;
					}

					object.isVisible = rawObject.isVisible;

					object.positionOriginal.x = evaluateString(rawObject, rawObject.rawPosition.x);
					object.positionOriginal.y = evaluateString(rawObject, rawObject.rawPosition.y);

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

					object.velocity.x = evaluateString(rawObject, rawObject.rawVelocity.x);
					object.velocity.y = evaluateString(rawObject, rawObject.rawVelocity.y);

					object.velocityOriginal = object.velocity;

					object.collisionData.enabled = rawObject.rawCollisionData.enabled;
					object.collisionData.group = rawObject.rawCollisionData.group ? groupNum : 0;

					object.collisionData.top = processCommands(rawObject, rawObject.rawCollisionData.top);
					object.collisionData.bottom = processCommands(rawObject, rawObject.rawCollisionData.bottom);
					object.collisionData.left = processCommands(rawObject, rawObject.rawCollisionData.left);
					object.collisionData.right = processCommands(rawObject, rawObject.rawCollisionData.right);

					for (auto& rawRule : rawObject.rawCollisionData.basic)
					{
						CollisionRule rule;
						rule.filterClass = rawRule.filterClass;
						rule.filterObject = rawRule.filterObject;
						rule.commands = processCommands(rawObject, rawRule.action);
						object.collisionData.basic.push_back(std::move(rule));
					}

					//object.action = rawObject.action;
					for (auto& rawAction : rawObject.action)
					{
						object.action[rawAction.first] = processCommands(rawObject, rawAction.second);
					}

					for (auto& rawVariable : rawObject.variable)
					{
						const float value = evaluateString(rawObject, rawVariable.second);
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

			if (rawObject.rawCollisionData.group) {
				groupNum++;
			}
		}

		for (auto& rawState : rawStates)
		{
			State state{};

			state.name = rawState.name;
			state.show = rawState.show;

			for (auto& rawAction : rawState.input)
			{
				state.input[keyCodeFromString(rawAction.first)] = processCommands(rawState, rawAction.second);
			}

			for (auto& rawCondition : rawState.conditions)
			{
				Condition condition;
				condition.filterClass = rawCondition.filterClass;
				condition.filterObject = rawCondition.filterObject;
				condition.variableName = rawCondition.variableName;
				condition.value = rawCondition.value;
				condition.commands = processCommands(rawState, rawCondition.action);
				state.conditions.push_back(std::move(condition));
			}

			states.push_back(state);
		}
	}

	// TODO: instead of passing an object or state, just pass the .name (std::string)
	// remove copies of evaluateString() and ProcessData()

	float game_expr::evaluateString(const RawObject& rawObject, const std::string& input_string)
	{
		if (!parser.compile(input_string, expression))
		{
			std::cout << "Error: " << parser.error().c_str()
				<< " in object named '" << rawObject.name << "'" << '\n';
			std::cout << "Failed to parse: \"" << input_string << "\"\n";
			exit(EXIT_FAILURE);
		}
		return expression.value();
	}

	std::vector<std::string> game_expr::processData(const RawObject& rawObject, const std::string& input_string)
	{
		tempSParams.clear();
		if (input_string != "")
		{
			evaluateString(rawObject, input_string);
		}
		return tempSParams;
	}

	std::vector<Command> game_expr::processCommands(const RawObject& rawObject, const std::string& input_string)
	{
		return parseCommands(processData(rawObject, input_string));
	}

	float game_expr::evaluateString(const RawState& rawState, const std::string& input_string)
	{
		if (!parser.compile(input_string, expression))
		{
			std::cout << "Error: " << parser.error().c_str()
				<< " in object named '" << rawState.name << "'" << '\n';
			std::cout << "Failed to parse: \"" << input_string << "\"\n";
			exit(EXIT_FAILURE);
		}
		return expression.value();
	}

	std::vector<std::string> game_expr::processData(const RawState& rawState, const std::string& input_string)
	{
		tempSParams.clear();
		if (input_string != "")
		{
			evaluateString(rawState, input_string);
		}
		return tempSParams;
	}

	std::vector<Command> game_expr::processCommands(const RawState& rawState, const std::string& input_string)
	{
		return parseCommands(processData(rawState, input_string));
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
		for (const std::string* positionExpression : { &rawObject.rawPosition.x, &rawObject.rawPosition.y })
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

	xge::GridData game_expr::setGridXY(std::vector<std::string>& spriteParams)
	{
		GridData gridData;

		if (spriteParams.size() > 5 && spriteParams.at(4) == "grid")
		{
			gridData.max.x = std::stoi(spriteParams.at(5));
			gridData.max.y = std::stoi(spriteParams.at(6));
			gridData.padding.x = std::stoi(spriteParams.at(7));
			gridData.padding.y = std::stoi(spriteParams.at(8));
		}

		return gridData;
	}
}
