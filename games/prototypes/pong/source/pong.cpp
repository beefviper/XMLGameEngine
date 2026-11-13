// pong.cpp
// XML Game Engine
// author: beefviper
// date: Nov 13, 2026
//
// Pong, from games/descriptions/pong.xml. Each part of the file is the code
// below that it names, in the file's order; nothing here moves anything
// itself.

#include "pong.h"

#include "game/random.h"
#include "play/numbers.h"
#include "play/verbs.h"

#include <fstream>
#include <utility>

namespace pong
{
	using namespace xge;

	namespace
	{
		// <window>
		const float windowWidth = 1280;
		const float windowHeight = 720;

		// <variables>
		const float margin = 30;
		const float width = 20;
		const float height = 120;
		const float step = 5;

		// <styles>
		struct Style
		{
			int size;
			const char* color;
		};
		const Style banner{ 160, "color.white" };
		const Style prompt{ 48, "color.lightgrey" };
		const Style hud{ 128, "color.white" };

		// What the computer says on the title, for each level.
		const char* const easy = "a d: the computer plays easy";
		const char* const smart = "a d: the computer plays smart";

		Object rectangle(const std::string& name, const std::string& state, float across, float down, const char* color = "color.white")
		{
			Object object;
			object.name = name;
			object.ownerState = state;
			object.picture.kind = ShapeKind::Rectangle;
			object.picture.size = { across, down };
			object.picture.color = color;
			object.collisionsOn = true;
			return object;
		}

		// A <text>: placed once the window has measured it (place()).
		Object text(const std::string& name, const std::string& state, const std::string& words, Style style)
		{
			Object object;
			object.name = name;
			object.ownerState = state;
			object.picture.kind = ShapeKind::Text;
			object.picture.text = words;
			object.picture.textSize = style.size;
			object.picture.color = style.color;
			return object;
		}

		// Where it starts, and where a reset puts it back.
		void startAt(Object& object, Vector2f position, Vector2f velocity = { 0, 0 })
		{
			object.position = position;
			object.velocity = velocity;
			object.start = object;
		}

		// A text put where its measured size puts it, now and at every reset.
		void placeAt(Object& object, Vector2f position)
		{
			object.position = object.start.position = position;
		}

		// Its middle on the window's middle, across and down (and so far
		// below it).
		float middleX(const Object& object) { return windowWidth / 2 - object.size.x / 2; }
		float middleY(const Object& object, float below = 0) { return windowHeight / 2 - object.size.y / 2 + below; }

		// A <note> of the square wave the arcade's tones were.
		SoundDesc tone(const char* name, float hertz, float seconds)
		{
			SoundDesc sound;
			sound.name = name;
			SoundNote note;
			note.startHz = note.endHz = hertz;
			note.seconds = seconds;
			sound.notes.push_back(note);
			return sound;
		}

		bool vertical(Edge edge) { return edge == Edge::Top || edge == Edge::Bottom; }
	}

	Pong::Pong()
	{
		window = { "Pong", windowWidth, windowHeight, 1, "color.black", "false", 60 };
		seedRandom();

		// the arcade's three tones: a paddle hit, the wall an octave lower,
		// and a long one for a point
		sounds = { tone("paddle", 459, 0.1f), tone("wall", 226, 0.02f), tone("score", 490, 0.26f) };

		// who plays the right paddle: 1 the computer, 2 a second player; and
		// how well the computer plays
		objects.push_back(text("computer", "", easy, prompt));
		computer().variable["players"] = 1;
		computer().variable["level"] = 1;
		startAt(computer(), {});

		// <state name="title">
		objects.push_back(text("title.title", "title", "PONG", banner));
		objects.push_back(text("title.start", "title", "spacebar to play the computer, 2 for two players", prompt));
		startAt(objects[1], {});
		startAt(objects[2], {});

		// <state name="playing">: the net, a dash in every other place down
		// the middle
		for (int row = 1; row <= 18; ++row)
		{
			Object dash = rectangle("playing.net.1." + std::to_string(row), "playing", 6, 20, "color.lightgrey");
			dash.groupName = "playing.net";
			dash.collisionsOn = false;
			startAt(dash, { windowWidth / 2 - 6.0f / 2, 10 + static_cast<float>(row - 1) * (20 + 20) });
			objects.push_back(dash);
		}

		// <group name="paddles" class="paddle">
		objects.push_back(rectangle("playing.paddle1", "playing", width, height));
		objects.push_back(rectangle("playing.paddle2", "playing", width, height));
		for (Object* paddle : { &paddle1(), &paddle2() })
		{
			paddle->groupName = "playing.paddles";
			paddle->objClass = "paddle";
			paddle->variable["score"] = 0;
		}
		startAt(paddle1(), { margin, windowHeight / 2 - height / 2 });
		startAt(paddle2(), { windowWidth - margin - width, windowHeight / 2 - height / 2 });

		objects.push_back(text("playing.score1", "playing", "0", hud));
		objects.push_back(text("playing.score2", "playing", "0", hud));
		startAt(score1(), {});
		startAt(score2(), { windowWidth / 2 + 80, margin });

		// served from the middle at ball.speed, within 30 degrees of
		// straight left or right, to either side
		objects.push_back(rectangle("playing.ball", "playing", 16, 16));
		ball().variable["size"] = 16;
		ball().variable["speed"] = 9;
		ball().variable["hits"] = 0;
		ballStart();
		startAt(ball(), ball().start.position, ball().start.velocity);

		// <state name="paused"> and <state name="over">
		objects.push_back(text("paused.paused", "paused", "PAUSED", banner));
		objects.push_back(text("over.gameover", "over", "GAME OVER", banner));
		objects.push_back(text("over.continue", "over", "spacebar to play again", prompt));
		for (std::size_t i = 26; i < objects.size(); ++i) { startAt(objects[i], {}); }
	}

	// The ball's <random>s drawn, then where it starts and the way it goes.
	void Pong::ballStart()
	{
		Object& self = ball();
		self.variable["angle"] = self.start.variable["angle"] = randomBetween(-30, 30);
		self.variable["side"] = self.start.variable["side"] = randomBetween(-1, 1);

		const float size = self.variable["size"];
		const float speed = self.variable["speed"];
		const float angle = self.variable["angle"];
		const float side = self.variable["side"];
		self.start.position = { windowWidth / 2 - size / 2, windowHeight / 2 - size / 2 };
		self.start.velocity = { sign(side) * speed * cosine(angle), speed * sine(angle) };
	}

	void Pong::place()
	{
		placeAt(computer(), { middleX(computer()), middleY(computer(), 190) });
		placeAt(objects[1], { middleX(objects[1]), margin * 4 });
		placeAt(objects[2], { middleX(objects[2]), middleY(objects[2], 120) });
		placeAt(score1(), { windowWidth / 2 - 80 - score1().size.x, margin });
		placeAt(objects[26], { middleX(objects[26]), middleY(objects[26]) });
		placeAt(objects[27], { middleX(objects[27]), middleY(objects[27]) });
		placeAt(objects[28], { middleX(objects[28]), middleY(objects[28], 120) });
	}

	const char* Pong::nameOf(State state)
	{
		switch (state)
		{
		case State::Title: return "title";
		case State::Playing: return "playing";
		case State::Paused: return "paused";
		case State::Over: return "over";
		}
		return "";
	}

	void Pong::push(State state)
	{
		states.push_back(state);
		++stateChanges;
	}

	void Pong::pop()
	{
		if (states.size() > 1) { states.pop_back(); ++stateChanges; }
	}

	// <inputs>, a state's at a time: what a key does in the state on top
	// when it goes down, and what it lets go of when it comes up.
	void Pong::key(KeyCode key, bool pressed)
	{
		if (pressed)
		{
			keysDown.insert(key);
			pressedIn[key] = top();
			keyIn(top(), key, true);
		}
		else
		{
			keysDown.erase(key);
			const auto was = pressedIn.find(key);
			if (was != pressedIn.end())
			{
				const State state = was->second;
				pressedIn.erase(was);
				keyIn(state, key, false);
			}
		}
		keysToState();
	}

	void Pong::keyIn(State state, KeyCode key, bool pressed)
	{
		switch (state)
		{
		case State::Title: titleKey(key, pressed); break;
		case State::Playing: playingKey(key, pressed); break;
		case State::Paused: pausedKey(key, pressed); break;
		case State::Over: overKey(key, pressed); break;
		}
	}

	// Space plays the computer, 2 a second player; A and D choose how well
	// the computer plays.
	void Pong::titleKey(KeyCode key, bool pressed)
	{
		if (!pressed) { return; }
		switch (key)
		{
		case KeyCode::Space: computer().variable["players"] = 1; push(State::Playing); break;
		case KeyCode::Num2:
		case KeyCode::Numpad2: computer().variable["players"] = 2; push(State::Playing); break;
		case KeyCode::A: computer().variable["level"] = 1; showLevel(); break;
		case KeyCode::D: computer().variable["level"] = 2; showLevel(); break;
		default: break;
		}
	}

	// W and S the left paddle, Up and Down the right for a second player,
	// F11 and F12 the right for the computer; Pause pauses.
	void Pong::playingKey(KeyCode key, bool pressed)
	{
		switch (key)
		{
		case KeyCode::W: holdMove(paddle1(), Direction::Up, step, pressed); break;
		case KeyCode::S: holdMove(paddle1(), Direction::Down, step, pressed); break;
		case KeyCode::Up: secondPlayer(Direction::Up, pressed); break;
		case KeyCode::Down: secondPlayer(Direction::Down, pressed); break;
		case KeyCode::F11: holdMove(paddle2(), Direction::Up, step, pressed); break;
		case KeyCode::F12: holdMove(paddle2(), Direction::Down, step, pressed); break;
		case KeyCode::Pause:
		case KeyCode::Pad1Start: if (pressed) { push(State::Paused); } break;
		default: break;
		}
	}

	void Pong::pausedKey(KeyCode key, bool pressed)
	{
		if (pressed && (key == KeyCode::Pause || key == KeyCode::Pad1Start)) { pop(); }
	}

	void Pong::overKey(KeyCode key, bool pressed)
	{
		if (pressed && key == KeyCode::Space) { newGame(); }
	}

	// The second player's keys do nothing while the computer plays.
	void Pong::secondPlayer(Direction direction, bool pressed)
	{
		if (computer().variable["players"] == 2) { holdMove(paddle2(), direction, step, pressed); }
	}

	// A paddle's own key still down when play comes back moves it again.
	void Pong::holdAgain(KeyCode key)
	{
		switch (key)
		{
		case KeyCode::W: holdStep(paddle1(), Direction::Up, step); break;
		case KeyCode::S: holdStep(paddle1(), Direction::Down, step); break;
		case KeyCode::F11: holdStep(paddle2(), Direction::Up, step); break;
		case KeyCode::F12: holdStep(paddle2(), Direction::Down, step); break;
		default: break;
		}
	}

	// The state changed: what the keys held were doing stops, and those
	// still down that the new state moves with move again.
	void Pong::keysToState()
	{
		if (keysFollowed == stateChanges) { return; }
		keysFollowed = stateChanges;

		const std::map<KeyCode, State> held = std::move(pressedIn);
		pressedIn.clear();
		for (const auto& [key, state] : held) { keyIn(state, key, false); }

		if (top() != State::Playing) { return; }
		for (const KeyCode key : keysDown)
		{
			switch (key)
			{
			case KeyCode::W: case KeyCode::S: case KeyCode::Up: case KeyCode::Down: case KeyCode::F11: case KeyCode::F12:
				pressedIn[key] = State::Playing;
				holdAgain(key);
				break;
			default: break;
			}
		}
	}

	// <computer>: when the computer plays the right paddle it holds F11 or
	// F12 to keep the paddle's middle within 8 of where it wants it. Easy
	// wants it under the ball, always; smart under the ball once it comes
	// over the net, and in the middle until then.
	void Pong::computerPlays()
	{
		std::set<KeyCode> wanted;
		if (top() == State::Playing && computer().variable["players"] == 1)
		{
			const float level = computer().variable["level"];
			const bool coming = ball().velocity.x > 0 && ball().position.x > windowWidth / 2;
			const float under = ball().position.y + ball().variable["size"] / 2 - height / 2;
			const float middle = windowHeight / 2 - height / 2;
			const float aim = level == 1 || coming ? under : middle;
			if (level == 1 || level == 2)
			{
				if (paddle2().position.y < aim - 8) { wanted.insert(KeyCode::F12); }
				if (paddle2().position.y > aim + 8) { wanted.insert(KeyCode::F11); }
			}
		}

		for (const KeyCode held : std::set<KeyCode>(computerHolds))
		{
			if (!wanted.count(held)) { key(held, false); }
		}
		for (const KeyCode want : wanted)
		{
			if (!computerHolds.count(want) && !keysDown.count(want)) { key(want, true); }
		}
		computerHolds = wanted;
	}

	void Pong::frame()
	{
		computerPlays();
		playOneFrame(objects, window, *this);
		keysToState();
	}

	std::vector<std::string> Pong::takeSounds()
	{
		return std::exchange(requested, {});
	}

	// The state on top shows what it owns; the title shows the computer's
	// level too.
	bool Pong::inPlay(const Object& object)
	{
		if (!object.isVisible || !object.exists) { return false; }
		return object.ownerState == nameOf(top()) || (top() == State::Title && &object == &objects[0]);
	}

	bool Pong::still(const Object& object) const
	{
		return (top() == State::Paused || top() == State::Over)
			&& object.isVisible && object.exists && object.ownerState == "playing";
	}

	// The ball bounces off the top and the bottom; off either side it gives
	// the other player a point and is served again. The paddles stop at the
	// top and the bottom.
	void Pong::atEdge(Object& object, Edge edge)
	{
		if (isBall(object))
		{
			if (vertical(edge)) { bounceOffEdge(object, edge, 1); play("wall"); }
			if (edge == Edge::Left) { point(paddle2()); }
			if (edge == Edge::Right) { point(paddle1()); }
		}
		if (isPaddle(object) && vertical(edge)) { stick(object, window, edge); }
	}

	void Pong::point(Object& paddle)
	{
		paddle.variable["score"] += 1;
		showScores();
		play("score");

		// its <random>s and its hits drawn anew
		ballStart();
		ball().backToStart();
		ball().activeMoveStep = {};
	}

	bool Pong::sticksAt(const Object& object, Edge edge)
	{
		return isPaddle(object) && vertical(edge);
	}

	// The ball's rules are about the paddles.
	bool Pong::canCollide(const Object& a, const Object& b)
	{
		return (isBall(a) && isPaddle(b)) || (isPaddle(a) && isBall(b));
	}

	bool Pong::hasRules(const Object& object)
	{
		return isBall(object);
	}

	void Pong::touched(Object& a, Object& b, const Contact& contact)
	{
		if (isBall(a) && isPaddle(b)) { ballMeetsPaddle(a, b, touchOfFirst(contact)); }
		if (isBall(b) && isPaddle(a)) { ballMeetsPaddle(b, a, touchOfSecond(contact)); }
	}

	// Off a paddle at up to 45 degrees, steeper the further from its middle;
	// the fourth hit and the twelfth make it faster.
	void Pong::ballMeetsPaddle(Object& self, Object& paddle, const Touch& touch)
	{
		deflect(self, paddle, touch.edge, 45);
		self.variable["hits"] += 1;
		play("paddle");

		const float hits = self.variable["hits"];
		if (hits == 4 || hits == 12)
		{
			self.velocity.x = self.velocity.x * 1.3f;
			self.velocity.y = self.velocity.y * 1.3f;
		}
	}

	// The first to eleven wins.
	void Pong::conditions()
	{
		if (top() == State::Playing && (paddle1().variable["score"] >= 11 || paddle2().variable["score"] >= 11))
		{
			push(State::Over);
		}
	}

	// The scores in big digits, as whole numbers.
	void Pong::showScores()
	{
		const auto show = [](Object& text, float number)
		{
			const std::string shown = std::to_string(static_cast<long>(number));
			if (text.picture.text != shown) { text.picture.text = shown; text.visualDirty = true; }
		};
		show(score1(), paddle1().variable["score"]);
		show(score2(), paddle2().variable["score"]);
	}

	void Pong::showLevel()
	{
		const char* const words = computer().variable["level"] == 2 ? smart : easy;
		if (computer().picture.text != words) { computer().picture.text = words; computer().visualDirty = true; }
	}

	// Everything back as it started, the ball served anew, but who plays
	// and how well; then the title again.
	void Pong::newGame()
	{
		ballStart();
		for (std::size_t i = 1; i < objects.size(); ++i)
		{
			objects[i].backToStart();
			objects[i].activeMoveStep = {};
		}
		showScores();
		states = { State::Title };
		++stateChanges;
		writeKept();
	}

	// One line a number: players, then level.
	void Pong::keepIn(const std::string& file)
	{
		keptFile = file;
		std::ifstream in(file);
		float players = 0;
		float level = 0;
		if (in >> players >> level && (players == 1 || players == 2) && (level == 1 || level == 2))
		{
			computer().variable["players"] = computer().start.variable["players"] = players;
			computer().variable["level"] = computer().start.variable["level"] = level;
			showLevel();
		}
	}

	void Pong::writeKept() const
	{
		if (keptFile.empty()) { return; }
		std::ofstream out(keptFile);
		out << objects[0].variable.at("players") << '\n' << objects[0].variable.at("level") << '\n';
	}
}
