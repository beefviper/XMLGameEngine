// game_expr.cpp
// XML Game Engine
// author: beefviper
// date: Sept 21, 2020

#include "game_expr.h"

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

		// TODO: loop through objects add their variables to the symbolTable
		// ^ might not be needed, update: might be needed?

		// register symbol table with expression
		expression.register_symbol_table(symbolTable);

		int groupNum = 1;

		// evaluate strings in objects
		for (auto& rawObject : rawObjects)
		{
			std::vector<std::string> tempSpriteParams = processData(rawObject, rawObject.src);
			const GridData gridData = setGridXY(tempSpriteParams);

			for (auto gridX = 0; gridX < gridData.max.x; gridX++)
			{
				for (auto gridY = 0; gridY < gridData.max.y; gridY++)
				{
					Object object{};

					object.spriteParams = tempSpriteParams;

					object.name = rawObject.name;
					object.objClass = rawObject.objClass;
					object.src = rawObject.src;

					if (rawObject.objClass == "projectile")
					{
						rawObject.isVisible = false;
					}

					object.isVisible = rawObject.isVisible;

					object.position.x = static_cast<float>(gridX);
					object.position.y = static_cast<float>(gridY);

					object.positionOriginal.x = evaluateString(rawObject, rawObject.rawPosition.x);
					object.positionOriginal.y = evaluateString(rawObject, rawObject.rawPosition.y);

					object.velocity.x = evaluateString(rawObject, rawObject.rawVelocity.x);
					object.velocity.y = evaluateString(rawObject, rawObject.rawVelocity.y);

					object.velocityOriginal = object.velocity;

					object.collisionData.enabled = rawObject.rawCollisionData.enabled;
					object.collisionData.group = rawObject.rawCollisionData.group ? groupNum : 0;

					object.collisionData.top = processData(rawObject, rawObject.rawCollisionData.top);
					object.collisionData.bottom = processData(rawObject, rawObject.rawCollisionData.bottom);
					object.collisionData.left = processData(rawObject, rawObject.rawCollisionData.left);
					object.collisionData.right = processData(rawObject, rawObject.rawCollisionData.right);
					object.collisionData.basic = processData(rawObject, rawObject.rawCollisionData.basic);

					//object.action = rawObject.action;
					for (auto& rawAction : rawObject.action)
					{
						object.action[rawAction.first] = processData(rawObject, rawAction.second);
					}

					for (auto& rawVariable : rawObject.variable)
					{
						object.variable[rawVariable.first] = evaluateString(rawObject, rawVariable.second);
					}

					object.renderTexture = std::make_unique<sf::RenderTexture>();
					object.sprite = std::make_unique<sf::Sprite>();

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
				state.input[rawAction.first] = processData(rawState, rawAction.second);
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

	xge::GridData game_expr::setGridXY(std::vector<std::string>& spriteParams)
	{
		GridData gridData;

		// TODO: calulate final position in here
		if (spriteParams.size() > 5 && spriteParams.at(4) == "grid")
		{
			gridData.max.x = std::stoi(spriteParams.at(5));
			gridData.max.y = std::stoi(spriteParams.at(6));
		}

		return gridData;
	}
}
