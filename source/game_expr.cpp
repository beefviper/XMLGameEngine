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
			const auto evaluate_string = [&](const std::string& input_string)
			{
				if (!parser.compile(input_string, expression))
				{
					std::cout << "Error: " << parser.error().c_str()
						<< " in object named '" << rawObject.name << "'" << '\n';
					exit(EXIT_FAILURE);
				}
				return expression.value();
			};

			auto process_collisionData = [&](const std::string& colData)
			{
				tempSParams.clear();
				if (colData != "")
				{
					evaluate_string(colData);
				}
				return tempSParams;
			};

			auto gridXmax = 1;
			auto gridYmax = 1;

			auto gridXpadding = 0;
			auto gridYpadding = 0;

			auto objWidth = 0;
			auto objHeight = 0;

			tempSParams.clear();
			evaluate_string(rawObject.src);
			rawObject.spriteParams = tempSParams;

			if (rawObject.spriteParams.size() > 5)
			{
				if (rawObject.spriteParams.at(3) == "grid" && rawObject.spriteParams.at(0) == "circle")
				{
					gridXmax = std::stoi(rawObject.spriteParams.at(4));
					gridYmax = std::stoi(rawObject.spriteParams.at(5));

					gridXpadding = std::stoi(rawObject.spriteParams.at(6));
					gridYpadding = std::stoi(rawObject.spriteParams.at(7));

					objWidth = std::stoi(rawObject.spriteParams.at(1));
					objHeight = std::stoi(rawObject.spriteParams.at(1));
				}
				else if (rawObject.spriteParams.at(4) == "grid" && rawObject.spriteParams.at(0) == "rectangle")
				{
					gridXmax = std::stoi(rawObject.spriteParams.at(5));
					gridYmax = std::stoi(rawObject.spriteParams.at(6));

					gridXpadding = std::stoi(rawObject.spriteParams.at(7));
					gridYpadding = std::stoi(rawObject.spriteParams.at(8));

					objWidth = std::stoi(rawObject.spriteParams.at(1));
					objHeight = std::stoi(rawObject.spriteParams.at(2));
				}
			}

			for (auto gridX = 0; gridX < gridXmax; gridX++)
			{
				for (auto gridY = 0; gridY < gridYmax; gridY++)
				{
					Object object{};

					object.position.x = evaluate_string(rawObject.sposition.x) + ((objWidth + gridXpadding) * gridX);
					object.position.y = evaluate_string(rawObject.sposition.y) + ((objHeight + gridYpadding) * gridY);

					object.velocity.x = evaluate_string(rawObject.svelocity.x);
					object.velocity.y = evaluate_string(rawObject.svelocity.y);

					object.position_original = object.position;
					object.velocity_original = object.velocity;

					object.collisionData.enabled = rawObject.rawCollisionData.enabled;
					object.collisionData.top = process_collisionData(rawObject.rawCollisionData.top);
					object.collisionData.bottom = process_collisionData(rawObject.rawCollisionData.bottom);
					object.collisionData.left = process_collisionData(rawObject.rawCollisionData.left);
					object.collisionData.right = process_collisionData(rawObject.rawCollisionData.right);
					object.collisionData.basic = process_collisionData(rawObject.rawCollisionData.basic);

					object.action = rawObject.action;
					object.isVisible = rawObject.isVisible;
					object.name = rawObject.name;
					object.spriteParams = rawObject.spriteParams;
					object.src = rawObject.src;
					object.renderTexture = std::make_unique<sf::RenderTexture>();
					object.sprite = std::make_unique<sf::Sprite>();

					objects.push_back(std::move(object));
				}
			}
		}
	}
}
