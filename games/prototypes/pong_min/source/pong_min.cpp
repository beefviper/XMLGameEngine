// pong_min.cpp
// XML Game Engine
// author: beefviper
// date: Oct 12, 2026
//
// Pong, from games/descriptions/pong_min.xml. Each part of the file is the code below
// that it names, in the file's order; nothing here moves anything itself.

#include "pong_min.h"

#include "random.h"
#include "verbs.h"

#include <cmath>

namespace pong_min
{
	using namespace xge;

	namespace
	{
		// The names an expression knows.
		const float pi = 3.14159265358979323846f;
		float sgn(float value) { return value > 0.0f ? 1.0f : (value < 0.0f ? -1.0f : 0.0f); }

		// <window>
		const float windowWidth = 1280.0f;
		const float windowHeight = 720.0f;
		const float windowLeft = 0.0f;
		const float windowRight = windowWidth;
		const float windowWidthCenter = windowWidth / 2;
		const float windowHeightCenter = windowHeight / 2;

		// <variables>
		const float margin = 30.0f;
		const float ballRadius = 10.0f;
		const float width = 30.0f;
		const float height = 150.0f;
		const float step = 5.0f;
	}

	Pong::Pong()
	{
		window = { "Pong", windowWidth, windowHeight, "color.black", "false", 60 };

		seedRandom();

		// <state name="playing"> > <object name="ball">, named by its path
		objects.push_back(makeObject("playing.ball", circleSprite(ballRadius, "color.white"), true));
		ball().variable["speed"] = 6.0f;
		drawBallStart();
		startAt(ball(), ball().positionOriginal, ball().velocityOriginal);

		// <state name="playing"> > <object name="paddle1">, named by its path
		objects.push_back(makeObject("playing.paddle1", rectangleSprite(width, height, "color.white"), true));
		startAt(paddle1(), { windowLeft + margin, windowHeightCenter - height / 2 }, { 0, 0 });

		// <state name="playing"> > <object name="paddle2">, named by its path
		objects.push_back(makeObject("playing.paddle2", rectangleSprite(width, height, "color.white"), true));
		startAt(paddle2(), { windowRight - margin - width, windowHeightCenter - height / 2 }, { 0, 0 });
	}

	void Pong::drawBallStart()
	{
		Object& self = ball();
		self.variable["angle"] = self.variableOriginal["angle"] = randomBetween(-30, 30);
		self.variable["side"] = self.variableOriginal["side"] = randomBetween(-1, 1);
		self.variableOriginal["speed"] = self.variable["speed"];

		const float speed = self.variable["speed"];
		const float angle = self.variable["angle"];
		const float side = self.variable["side"];
		self.positionOriginal = { windowWidthCenter - ballRadius, windowHeightCenter - ballRadius };
		// ball.angle * pi / 180 as exprtk works it out: it folds the two
		// constants first, angle * (pi / 180), and the last bit can differ.
		self.velocityOriginal = { sgn(side) * speed * std::cos(angle * (pi / 180)), speed * std::sin(angle * (pi / 180)) };
	}

	// <state name="playing"> > <inputs>
	void Pong::key(KeyCode key, bool pressed)
	{
		switch (key)
		{
		case KeyCode::W:    paddle1Up(pressed); break;
		case KeyCode::S:    paddle1Down(pressed); break;
		case KeyCode::Up:   paddle2Up(pressed); break;
		case KeyCode::Down: paddle2Down(pressed); break;
		default: break;
		}
	}

	void Pong::frame()
	{
		playOneFrame(objects, window, *this);
	}

	// <state name="playing"> > <objects>: all three, its own.
	bool Pong::inPlay(const Object& object)
	{
		return object.isVisible;
	}

	// <collision edge="...">
	void Pong::atEdge(Object& object, Edge edge)
	{
		const bool vertical = edge == Edge::Top || edge == Edge::Bottom;

		if (&object == &ball())
		{
			if (vertical) { bounce(object, edge); }
			else { drawBallStart(); restart(object); }
		}
		else if (vertical)
		{
			stick(object, window, edge);
		}
	}

	// The paddles' <collision edge="vertical"><stick />.
	bool Pong::sticksAt(const Object& object, Edge edge)
	{
		return &object != &objects[Ball] && (edge == Edge::Top || edge == Edge::Bottom);
	}

	// The ball's <collision> names no object or class: it answers to anything.
	bool Pong::canCollide(const Object& a, const Object& b)
	{
		return &a == &objects[Ball] || &b == &objects[Ball];
	}

	// The ball's <collision><deflect>45</deflect>.
	void Pong::touched(Object& a, Object& b, Edge edgeOfB)
	{
		if (&a == &ball()) { deflect(a, b, opposite(edgeOfB), 45); }
		if (&b == &ball()) { deflect(b, a, edgeOfB, 45); }
	}

	// <object name="paddle1"> > <actions>, and paddle2's.
	void Pong::paddle1Up(bool pressed) { holdMove(paddle1(), Direction::Up, step, pressed); }
	void Pong::paddle1Down(bool pressed) { holdMove(paddle1(), Direction::Down, step, pressed); }
	void Pong::paddle2Up(bool pressed) { holdMove(paddle2(), Direction::Up, step, pressed); }
	void Pong::paddle2Down(bool pressed) { holdMove(paddle2(), Direction::Down, step, pressed); }
}
