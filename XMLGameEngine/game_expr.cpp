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

	randomNumber<float> randomNumberFloat;
	randomRange<float> randomRangeFloat;
	shapeCircle<float> shapeCircleFloat;
	shapeRectangle<float> shapeRectangleFloat;

	float recWidth{};
	float recHeight{};

	std::string textString;
	float textSize;

	exprtk::symbol_table<float> symbolTable;
	symbolTable.add_function("random.number", randomNumberFloat);
	symbolTable.add_function("random.range", randomRangeFloat);
	symbolTable.add_function("shape.circle", shapeCircleFloat);
	symbolTable.add_function("shape.rectangle", shapeRectangleFloat);

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

	symbolTable.add_variable("recWidth", recWidth);
	symbolTable.add_variable("recHeight", recHeight);

	symbolTable.add_stringvar("textString", textString);
	symbolTable.add_variable("textSize", textSize);

	exprtk::expression<float> expression;
	expression.register_symbol_table(symbolTable);
	exprtk::parser<float> parser;

	auto evaluate_string = [&](std::string input_string) { parser.compile(input_string, expression); return expression.value(); };
	auto getParam = [&](xge::Object& object, std::string firstDelimiter, std::string secondDelimiter)
	{
		auto e_start = object.src.find(firstDelimiter) + 1;
		auto e_end = object.src.find(secondDelimiter);
		auto e_length = e_end - e_start;
		return object.src.substr(e_start, e_length);
	};
	auto getFirstParam = [&](xge::Object& object) { return getParam(object, "(", ","); };
	auto getSecondParam = [&](xge::Object& object) { return getParam(object, ",", ")"); };

	for (auto& object : objects)
	{
		auto& objectName = getSObject(object.name);

		object.position.x = evaluate_string(objectName.sposition.x);
		object.position.y = evaluate_string(objectName.sposition.y);
		object.velocity.x = evaluate_string(objectName.svelocity.x);
		object.velocity.y = evaluate_string(objectName.svelocity.y);

		object.collision = objectName.scollision == "true" ? true : false;

		// TODO: finish coming up with way to parse different types of assets: shape, file, data
		if (object.src.compare(0, 12, "shape.circle") == 0)
		{
			object.params.push_back(evaluate_string(object.src));
		}

		else if (object.src.compare(0, 15, "shape.rectangle") == 0)
		{
			evaluate_string("recWidth := " + getFirstParam(object));
			object.params.push_back(recWidth);

			evaluate_string("recHeight := " + getSecondParam(object));
			object.params.push_back(recHeight);
		}

		else if (object.src.compare(0, 4, "text") == 0)
		{
			evaluate_string("textString := " + getFirstParam(object));
			object.sparams.push_back(textString);

			evaluate_string("textSize := " + getSecondParam(object));
			object.params.push_back(textSize);
		}
	}
}
