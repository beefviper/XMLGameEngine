// game_expr.cpp
// XML Game Engine
// author: beefviper
// date: Sept 21, 2020

#include "game_expr.h"

namespace xge
{
	void game_expr::init(WindowDesc& windowDesc, std::map<std::string, float>& variables, std::vector<State>& states, std::vector<Object>& objects)
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

		// add functions to symbol table
		symbolTable.add_function("random.number", randomNumberFloat);
		symbolTable.add_function("random.range", randomRangeFloat);
		symbolTable.add_function("shape.circle", shapeCircleFloat);
		symbolTable.add_function("shape.rectangle", shapeRectangleFloat);
		symbolTable.add_function("text", textFloat);
		symbolTable.add_function("image", imageFloat);
		symbolTable.add_function("collide", collideFloat);
		symbolTable.add_function("inc", incFloat);

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
		for (auto& object : objects)
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
			object.velocity_original = object.position;

			evaluate_string(object.src);
			object.spriteParams = tempSParams;

			if (object.collisionData.top != "")
			{
				tempSParams.clear();
				evaluate_string(object.collisionData.top);
				object.collisionDataEx.top = tempSParams;
			}

			if (object.collisionData.bottom != "")
			{
				tempSParams.clear();
				evaluate_string(object.collisionData.bottom);
				object.collisionDataEx.bottom = tempSParams;
			}

			if (object.collisionData.left != "")
			{
				tempSParams.clear();
				evaluate_string(object.collisionData.left);
				object.collisionDataEx.left = tempSParams;
			}

			if (object.collisionData.right != "")
			{
				tempSParams.clear();
				evaluate_string(object.collisionData.right);
				object.collisionDataEx.right = tempSParams;
			}

			if (object.collisionData.basic != "")
			{
				tempSParams.clear();
				evaluate_string(object.collisionData.basic);
				object.collisionDataEx.basic = tempSParams;
			}
		}
	}
}
