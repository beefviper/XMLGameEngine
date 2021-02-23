// game_expr.cpp
// XML Game Engine
// author: beefviper
// date: Sept 21, 2020

#include "game.h"
#include "game_expr.h"

void xge::Game::initEXPR(void)
{
	// TODO: Write code to parse the rest of the text fields
	//		-- exprtk lib will parse text variables and expressions

	randomNumber<float> randomNumberFloat{};
	randomRange<float> randomRangeFloat{};
	shapeCircle<float> shapeCircleFloat{};
	shapeRectangle<float> shapeRectangleFloat{};
	text<float> textFloat{};

	exprtk::symbol_table<float> symbolTable;
	symbolTable.add_function("random.number", randomNumberFloat);
	symbolTable.add_function("random.range", randomRangeFloat);
	symbolTable.add_function("shape.circle", shapeCircleFloat);
	symbolTable.add_function("shape.rectangle", shapeRectangleFloat);
	symbolTable.add_function("text", textFloat);

	symbolTable.add_constant("window.top", 0);
	symbolTable.add_constant("window.bottom", windowDesc.height);
	symbolTable.add_constant("window.left", 0);
	symbolTable.add_constant("window.right", windowDesc.width);
	symbolTable.add_constant("window.width.center", windowDesc.width / 2);
	symbolTable.add_constant("window.height.center", windowDesc.height / 2);
	for (auto& variable : variables)
	{
		symbolTable.add_constant(variable.first, variable.second);
	}

	exprtk::expression<float> expression;
	expression.register_symbol_table(symbolTable);
	exprtk::parser<float> parser;

	for (auto& object : objects)
	{
		auto evaluate_string = [&](const std::string& input_string)
		{
			parser.compile(input_string, expression);

			if (!parser.compile(input_string, expression))
			{
				std::cout << "Error: " << parser.error().c_str()
					<< " in object named '" << object.name << "'" << '\n';
				exit(EXIT_FAILURE); 
			}

			return expression.value();
		};
		
		auto& objectName = getSObject(object.name);

		object.position.x = evaluate_string(objectName.sposition.x);
		object.position.y = evaluate_string(objectName.sposition.y);
		object.velocity.x = evaluate_string(objectName.svelocity.x);
		object.velocity.y = evaluate_string(objectName.svelocity.y);

		evaluate_string(object.src);
		object.sparams = tempSParams;
		object.params = tempParams;
	}
}
