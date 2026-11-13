// pong_min.cpp
// XML Game Engine
// author: beefviper
// date: Oct 12, 2026
//
// Pong, from games/descriptions/pong_min.xml. Each part of the file is the code
// below that it names, in the file's order; nothing here moves anything itself.

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

		Object rectangle(const char* name, float across, float down)
		{
			Object object;
			object.name = name;
			object.ownerState = "playing";
			object.picture.kind = ShapeKind::Rectangle;
			object.picture.size = { across, down };
			object.picture.color = "color.white";
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
	}

	Pong::Pong()
	{
		window = { "Pong", windowWidth, windowHeight, 1, "color.black", "false", 60 };
		seedRandom();

		objects.push_back(rectangle("playing.ball", size, size));
		ball().variable["speed"] = 6;
		serve();
		startAt(ball(), ball().start.position, ball().start.velocity);

		// <group name="paddles">
		objects.push_back(rectangle("playing.paddle1", width, height));
		objects.push_back(rectangle("playing.paddle2", width, height));
		paddle1().groupName = paddle2().groupName = "playing.paddles";
		startAt(paddle1(), { margin, (windowHeight - height) / 2 }, { 0, 0 });
		startAt(paddle2(), { windowWidth - margin - width, (windowHeight - height) / 2 }, { 0, 0 });
	}

	// At ball.speed, within 30 degrees of straight left or right, to either side.
	void Pong::serve()
	{
		Object& self = ball();
		const float angle = randomBetween(-30, 30);
		const float side = randomBetween(-1, 1);
		self.variable["angle"] = self.start.variable["angle"] = angle;
		self.variable["side"] = self.start.variable["side"] = side;
		self.start.variable["speed"] = self.variable["speed"];

		const float speed = self.variable["speed"];
		self.start.position = { (windowWidth - size) / 2, (windowHeight - size) / 2 };
		self.start.velocity = { sign(side) * speed * cosine(angle), speed * sine(angle) };
	}

	// <inputs>: W and S move paddle1, Up and Down paddle2, while held.
	void Pong::key(KeyCode key, bool pressed)
	{
		switch (key)
		{
		case KeyCode::W:    holdMove(paddle1(), Direction::Up, step, pressed); break;
		case KeyCode::S:    holdMove(paddle1(), Direction::Down, step, pressed); break;
		case KeyCode::Up:   holdMove(paddle2(), Direction::Up, step, pressed); break;
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

	// The ball bounces off the top and bottom and is served again from
	// either side; the paddles stop at the top and bottom.
	void Pong::atEdge(Object& object, Edge edge)
	{
		if (!isBall(object))
		{
			if (vertical(edge)) { stick(object, window, edge); }
			return;
		}

		if (vertical(edge)) { bounceOffEdge(object, edge, 1); }
		else
		{
			serve();
			restart(object);
		}
	}

	bool Pong::sticksAt(const Object& object, Edge edge)
	{
		return !isBall(object) && vertical(edge);
	}

	// The ball's <collision> names nothing: it answers to anything it meets.
	bool Pong::canCollide(const Object& a, const Object& b)
	{
		return isBall(a) || isBall(b);
	}

	bool Pong::hasRules(const Object& object)
	{
		return isBall(object);
	}

	// <deflect>45</deflect>: off a paddle at up to 45 degrees, by where it hit.
	void Pong::touched(Object& a, Object& b, const Contact& contact)
	{
		if (isBall(a)) { deflect(a, b, touchOfFirst(contact).edge, 45); }
		if (isBall(b)) { deflect(b, a, touchOfSecond(contact).edge, 45); }
	}
}
