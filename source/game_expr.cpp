// game_expr.cpp
// XML Game Engine
// author: beefviper
// date: Sept 21, 2020

#include "game_expr.h"

namespace xge
{
	void game_expr::init(WindowDesc& windowDesc, std::map<std::string, float>& variables, std::vector<State>& states,
		std::vector<RawObject>& rawObjects, std::vector<Object>& objects)
	{
		// custom functions added to exprtk
		randomNumber<float> randomNumberFloat{};
		randomRange<float> randomRangeFloat{};
		shapeCircle<float> shapeCircleFloat{};
		shapeRectangle<float> shapeRectangleFloat{};
		text<float> textFloat{};
		image<float> imageFloat{};
		collide<float> collideFloat{};
		inc<float> incFloat{};
		grid<float> gridFloat{};

		// add functions to symbol table
		symbolTable.add_function("random.number", randomNumberFloat);
		symbolTable.add_function("random.range", randomRangeFloat);
		symbolTable.add_function("shape.circle", shapeCircleFloat);
		symbolTable.add_function("shape.rectangle", shapeRectangleFloat);
		symbolTable.add_function("text", textFloat);
		symbolTable.add_function("image", imageFloat);
		symbolTable.add_function("collide", collideFloat);
		symbolTable.add_function("inc", incFloat);
		symbolTable.add_function("grid", gridFloat);

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

		// register symbol table with expression
		expression.register_symbol_table(symbolTable);

		// evaluate strings in objects
		for (auto& rawObject : rawObjects)
		{
			std::vector<std::string> tempSpriteParams = processData(rawObject, rawObject.src);
			GridData gridData = setGridData(tempSpriteParams);

			for (auto gridX = 0; gridX < gridData.max.x; gridX++)
			{
				for (auto gridY = 0; gridY < gridData.max.y; gridY++)
				{
					Object object{};

					object.spriteParams = tempSpriteParams;

					object.name = rawObject.name;
					object.src = rawObject.src;
					object.isVisible = rawObject.isVisible;

					object.position.x = evaluateString(rawObject, rawObject.rawPosition.x) + ((gridData.obj.x + gridData.padding.x) * gridX);
					object.position.y = evaluateString(rawObject, rawObject.rawPosition.y) + ((gridData.obj.y + gridData.padding.y) * gridY);

					object.velocity.x = evaluateString(rawObject, rawObject.rawVelocity.x);
					object.velocity.y = evaluateString(rawObject, rawObject.rawVelocity.y);

					object.positionOriginal = object.position;
					object.velocityOriginal = object.velocity;

					object.collisionData.enabled = rawObject.rawCollisionData.enabled;
					object.collisionData.top = processData(rawObject, rawObject.rawCollisionData.top);
					object.collisionData.bottom = processData(rawObject, rawObject.rawCollisionData.bottom);
					object.collisionData.left = processData(rawObject, rawObject.rawCollisionData.left);
					object.collisionData.right = processData(rawObject, rawObject.rawCollisionData.right);
					object.collisionData.basic = processData(rawObject, rawObject.rawCollisionData.basic);

					object.action = rawObject.action;

					object.renderTexture = std::make_unique<sf::RenderTexture>();
					object.sprite = std::make_unique<sf::Sprite>();

					objects.push_back(std::move(object));
				}
			}
		}
	}

	float game_expr::evaluateString(const RawObject& rawObject, const std::string& input_string)
	{
		if (!parser.compile(input_string, expression))
		{
			std::cout << "Error: " << parser.error().c_str()
				<< " in object named '" << rawObject.name << "'" << '\n';
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

	xge::GridData game_expr::setGridData(std::vector<std::string>& tempSpriteParams)
	{
		GridData gridData;

		if (tempSpriteParams.size() > 5)
		{
			gridData.max.x = std::stoi(tempSpriteParams.at(5));
			gridData.max.y = std::stoi(tempSpriteParams.at(6));

			gridData.padding.x = std::stoi(tempSpriteParams.at(7));
			gridData.padding.y = std::stoi(tempSpriteParams.at(8));

			if (tempSpriteParams.at(4) == "grid" && tempSpriteParams.at(0) == "circle")
			{
				gridData.obj.x = std::stoi(tempSpriteParams.at(1)) * 2;
				gridData.obj.y = std::stoi(tempSpriteParams.at(1)) * 2;
			}
			else if (tempSpriteParams.at(4) == "grid" && tempSpriteParams.at(0) == "rectangle")
			{
				gridData.obj.x = std::stoi(tempSpriteParams.at(1));
				gridData.obj.y = std::stoi(tempSpriteParams.at(2));
			}
		}

		return gridData;
	}
}
