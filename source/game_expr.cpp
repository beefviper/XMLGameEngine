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
		// custom funtions added to exprtk
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
		for (auto& object : rawObjects)
		{
			const auto evaluate_string = [&](const std::string& input_string)
			{
				if (!parser.compile(input_string, expression))
				{
					std::cout << "Error: " << parser.error().c_str()
						<< " in object named '" << object.name << "'" << '\n';
					exit(EXIT_FAILURE);
				}
				return expression.value();
			};

			object.position.x = evaluate_string(object.sposition.x);
			object.position.y = evaluate_string(object.sposition.y);
			object.velocity.x = evaluate_string(object.svelocity.x);
			object.velocity.y = evaluate_string(object.svelocity.y);

			object.position_original = object.position;
			object.velocity_original = object.velocity;

			tempSParams.clear();
			evaluate_string(object.src);
			object.spriteParams = tempSParams;

			auto process_collisionData = [&](const std::string& colData)
			{
				tempSParams.clear();
				if (colData != "")
				{
					evaluate_string(colData);
				}
				return tempSParams;
			};
			
			object.collisionData.enabled = object.rawCollisionData.enabled;
			object.collisionData.top = process_collisionData(object.rawCollisionData.top);
			object.collisionData.bottom = process_collisionData(object.rawCollisionData.bottom);
			object.collisionData.left = process_collisionData(object.rawCollisionData.left);
			object.collisionData.right = process_collisionData(object.rawCollisionData.right);
			object.collisionData.basic = process_collisionData(object.rawCollisionData.basic);
		}

		for (auto& object : rawObjects)
		{
			auto gridXmax = 1;
			auto gridYmax = 1;

			auto gridXpadding = 0;
			auto gridYpadding = 0;

			auto objWidth = 0;
			auto objHeight = 0;

			if (object.spriteParams.size() > 5)
			{
				if (object.spriteParams.at(3) == "grid" && object.spriteParams.at(0) == "circle")
				{
					gridXmax = std::stoi(object.spriteParams.at(4));
					gridYmax = std::stoi(object.spriteParams.at(5));

					gridXpadding = std::stoi(object.spriteParams.at(6));
					gridYpadding = std::stoi(object.spriteParams.at(7));

					objWidth = std::stoi(object.spriteParams.at(1));
					objHeight = std::stoi(object.spriteParams.at(1));
				}
				else if (object.spriteParams.at(4) == "grid" && object.spriteParams.at(0) == "rectangle")
				{
					gridXmax = std::stoi(object.spriteParams.at(5));
					gridYmax = std::stoi(object.spriteParams.at(6));

					gridXpadding = std::stoi(object.spriteParams.at(7));
					gridYpadding = std::stoi(object.spriteParams.at(8));

					objWidth = std::stoi(object.spriteParams.at(1));
					objHeight = std::stoi(object.spriteParams.at(2));
				}
			}

			auto totalObjects = gridXmax * gridYmax;
			for (auto gridX = 0; gridX < gridXmax; gridX++)
			{
				for (auto gridY = 0; gridY < gridYmax; gridY++)
				{
					Object newObject{};
					newObject.action = object.action;
					newObject.rawCollisionData = object.rawCollisionData;
					newObject.collisionData = object.collisionData;
					newObject.isVisible = object.isVisible;
					newObject.name = object.name;
					newObject.position = object.position;
					newObject.position_original = object.position_original;
					newObject.sposition = object.sposition;
					newObject.spriteParams = object.spriteParams;
					newObject.src = object.src;
					newObject.svelocity = object.svelocity;
					newObject.velocity = object.velocity;
					newObject.velocity_original = object.velocity_original;
					newObject.renderTexture = std::make_unique<sf::RenderTexture>();
					newObject.sprite = std::make_unique<sf::Sprite>();

					newObject.position.x = object.position.x + ((objWidth + gridXpadding) * gridX);
					newObject.position.y = object.position.y + ((objHeight + gridYpadding) * gridY);
					newObject.position_original = newObject.position;

					objects.push_back(std::move(newObject));
				}
			}
		}
	}
}
