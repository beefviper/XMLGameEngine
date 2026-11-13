// pong_min.cpp
// XML Game Engine
// author: beefviper
// date: Oct 12, 2026
//
// Pong, from pong_min.xml.
//
// Each part of the file is the code below that it names, in the file's order;
// nothing here moves anything itself.

#include "pong_min.h"

#include "game/random.h"
#include "play/numbers.h"
#include "play/verbs.h"

namespace pong_min
{
	using namespace xge;

	namespace
	{
		// <window>
		const float windowWidth = 1280;
		const float windowHeight = 720;

		// <variables>
		const float margin = 30;
		const float size = 16;
		const float width = 20;
		const float height = 120;
		const float step = 5;

		// A <rectangle>, white unless it says, that touches what it meets.
		Object rectangle(const char* name, float across, float down, const char* color = "color.white")
		{
			Object object;
			object.name = name;
			object.ownerState = "playing";
			object.picture.kind = ShapeKind::Rectangle;
			object.picture.size = { across, down };
			object.picture.color = color;
			object.collisionsOn = true;
			return object;
		}

		// Where it starts, and where a reset puts it back.
		void startAt(Object& object, Vector2f position, Vector2f velocity)
		{
			object.position = position;
			object.velocity = velocity;
			object.start = object;
		}

		bool vertical(Edge edge) { return edge == Edge::Top || edge == Edge::Bottom; }
		bool horizontal(Edge edge) { return edge == Edge::Left || edge == Edge::Right; }
	}

	Pong::Pong()
	{
		window = { "Pong", windowWidth, windowHeight, 1, "color.black", "false", 60 };
		seedRandom();

		objects.push_back(rectangle("playing.ball", size, size));
		ball().variable["speed"] = 6;
		ballStart();
		startAt(ball(), ball().start.position, ball().start.velocity);

		// <group name="paddles">
		objects.push_back(rectangle("playing.paddle1", width, height));
		objects.push_back(rectangle("playing.paddle2", width, height));
		paddle1().groupName = paddle2().groupName = "playing.paddles";
		startAt(paddle1(), { margin, windowHeight / 2 - height / 2 }, { 0, 0 });
		startAt(paddle2(), { windowWidth - margin - width, windowHeight / 2 - height / 2 }, { 0, 0 });
	}

	// Where the ball starts, and starts again after a reset: its
	// <random>s drawn, then its place and its way from them.
	void Pong::ballStart()
	{
		Object& self = ball();
		self.variable["angle"] = self.start.variable["angle"] = randomBetween(-30, 30);
		self.variable["side"] = self.start.variable["side"] = randomBetween(-1, 1);

		const float speed = self.variable["speed"];
		const float angle = self.variable["angle"];
		const float side = self.variable["side"];
		self.start.position = { windowWidth / 2 - size / 2, windowHeight / 2 - size / 2 };
		self.start.velocity = { sign(side) * speed * cosine(angle), speed * sine(angle) };
	}

	// <inputs>
	void Pong::key(KeyCode key, bool pressed)
	{
		switch (key)
		{
		case KeyCode::W: holdMove(paddle1(), Direction::Up, step, pressed); break;
		case KeyCode::S: holdMove(paddle1(), Direction::Down, step, pressed); break;
		case KeyCode::Up: holdMove(paddle2(), Direction::Up, step, pressed); break;
		case KeyCode::Down: holdMove(paddle2(), Direction::Down, step, pressed); break;
		default: break;
		}
	}

	void Pong::frame()
	{
		playOneFrame(objects, window, *this);
	}

	// The one state shows everything it owns.
	bool Pong::inPlay(const Object& object)
	{
		return object.isVisible;
	}

	// At the window's edges: each one's <collision edge>s.
	void Pong::atEdge(Object& object, Edge edge)
	{
		if (isBall(object))
		{
			if (vertical(edge)) { bounceOffEdge(object, edge, 1); }
			if (horizontal(edge)) { ballStart(); restart(object); }
		}
		if (inPaddles(object))
		{
			if (vertical(edge)) { stick(object, window, edge); }
		}
	}

	// What stops at an edge rather than going past it: a <stick />.
	bool Pong::sticksAt(const Object& object, Edge edge)
	{
		return inPaddles(object) && vertical(edge);
	}

	// Which pairs answer to meeting: a <collision> naming nothing answers to
	// anything its object meets.
	bool Pong::canCollide(const Object& a, const Object& b)
	{
		return isBall(a) || isBall(b);
	}

	bool Pong::hasRules(const Object& object)
	{
		return isBall(object);
	}

	// When they meet: each one's <collision>s, done to it.
	void Pong::touched(Object& a, Object& b, const Contact& contact)
	{
		if (isBall(a)) { deflect(a, b, touchOfFirst(contact).edge, 45); }
		if (isBall(b)) { deflect(b, a, touchOfSecond(contact).edge, 45); }
	}
}
