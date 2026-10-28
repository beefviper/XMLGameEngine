// command.cpp
// XML Game Engine
// author: beefviper
// date: Sept 27, 2026

#include "game/command.h"
#include "play/geometry.h"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <iostream>
#include <stdexcept>

namespace xge
{
	namespace
	{
		Direction directionFromName(const std::string& name, const std::string& verb)
		{
			if (name == "up")    { return Direction::Up; }
			if (name == "down")  { return Direction::Down; }
			if (name == "left")  { return Direction::Left; }
			if (name == "right") { return Direction::Right; }

			throw std::runtime_error("<" + verb + "> has direction=\"" + name + "\"; expected up, down, left or right");
		}
	}

	Command makeCommand(const RawCommand& raw, const ValueEvaluator& evaluate)
	{
		const std::string& verb = raw.verb;

		if (verb == "bounce") { return CmdBounce{}; }
		if (verb == "stick")  { return CmdStick{}; }
		if (verb == "die")    { return CmdDie{}; }
		if (verb == "stop")   { return CmdStop{}; }
		if (verb == "wrap")   { return CmdWrap{}; }
		if (verb == "ride")   { return CmdRide{}; }
		if (verb == "carry")  { return CmdCarry{}; }
		if (verb == "drop")   { return CmdDrop{}; }
		if (verb == "track")  { return CmdTrack{ raw.path }; }
		if (verb == "land")   { return CmdLand{}; }
		if (verb == "reverse") { return CmdReverse{}; }

		// <reset/> puts the object in the rule (or, in a state's input or
		// condition, the whole game) back; <reset object="name"/> that one object.
		if (verb == "reset")
		{
			if (!raw.scene.empty() || !raw.way.empty()) { return CmdResetScene{ raw.scene, raw.way }; }
			if (raw.object.empty()) { return CmdReset{}; }
			return CmdResetObject{ raw.object };
		}

		if (verb == "move") { return CmdMove{ directionFromName(raw.direction, verb), evaluate(raw.amount) }; }
		// How long a <hop> takes: no <seconds>, none at all.
		const auto hopSeconds = [&]()
		{
			if (raw.seconds.kind == RawValue::Kind::Expression && raw.seconds.text.empty()) { return 0.0f; }
			const float seconds = evaluate(raw.seconds);
			if (!(seconds > 0.0f)) { throw std::runtime_error("<hop> has <seconds> of " + formatDisplayNumber(seconds) + "; expected more than 0"); }
			return seconds;
		};
		// How high a timed <hop> or <jump> goes: no <arc>, not at all.
		const auto arcOf = [&](const char* what, float seconds)
		{
			if (raw.arc.kind == RawValue::Kind::Expression && raw.arc.text.empty()) { return 0.0f; }
			if (!(seconds > 0.0f)) { throw std::runtime_error(std::string("<") + what + "> has an <arc> but no <seconds>: only a hop that takes time leaves the ground"); }
			const float arc = evaluate(raw.arc);
			if (arc < 0.0f) { throw std::runtime_error(std::string("<") + what + "> has an <arc> of " + formatDisplayNumber(arc) + "; expected 0 or more"); }
			return arc;
		};

		if (verb == "hop")
		{
			// up-left and the rest: one of up/down, then one of left/right.
			const std::string& name = raw.direction;
			const auto dash = name.find('-');
			if (dash == std::string::npos)
			{
				CmdHop hop{ directionFromName(name, verb), evaluate(raw.amount), {}, hopSeconds() };
				hop.arc = arcOf("hop", hop.seconds);
				hop.step = stepToward(hop.direction, hop.distance);
				return hop;
			}

			const Direction upDown = directionFromName(name.substr(0, dash), verb);
			const Direction leftRight = directionFromName(name.substr(dash + 1), verb);
			if ((upDown != Direction::Up && upDown != Direction::Down) || (leftRight != Direction::Left && leftRight != Direction::Right))
			{
				throw std::runtime_error("<hop> has direction=\"" + name + "\"; a diagonal is up-left, up-right, down-left or down-right");
			}
			const bool split = !raw.positionX.text.empty() || raw.positionX.kind != RawValue::Kind::Expression;
			const float distance = split ? 0.0f : evaluate(raw.amount);
			const float across = split ? evaluate(raw.positionX) : distance;
			const float down = split ? evaluate(raw.positionY) : distance;
			CmdHop hop{ upDown, distance, {}, hopSeconds() };
			hop.arc = arcOf("hop", hop.seconds);
			hop.step = { leftRight == Direction::Left ? -across : across, upDown == Direction::Up ? -down : down };
			return hop;
		}

		if (verb == "jump")
		{
			CmdJump jump{ directionFromName(raw.direction, verb), evaluate(raw.amount) };
			const bool noSeconds = raw.seconds.kind == RawValue::Kind::Expression && raw.seconds.text.empty();
			if (!noSeconds) { jump.seconds = evaluate(raw.seconds); }
			if (!(jump.seconds > 0.0f)) { throw std::runtime_error("<jump> has <seconds> of " + std::to_string(jump.seconds) + "; expected more than 0"); }
			jump.arc = arcOf("jump", jump.seconds);
			return jump;
		}

		if (verb == "leap")
		{
			const float height = evaluate(raw.amount);
			if (!(height > 0.0f)) { throw std::runtime_error("<leap> has a height of " + formatDisplayNumber(height) + "; expected more than 0"); }
			return CmdLeap{ height };
		}

		if (verb == "climb")
		{
			const Direction direction = directionFromName(raw.direction, verb);
			if (direction != Direction::Up && direction != Direction::Down)
			{
				throw std::runtime_error("<climb> has direction=\"" + raw.direction + "\"; expected up or down");
			}
			return CmdClimb{ direction, evaluate(raw.amount), raw.objClass };
		}

		if (verb == "chase")
		{
			const float speed = evaluate(raw.amount);
			const bool noNear = raw.distance.kind == RawValue::Kind::Expression && raw.distance.text.empty();
			const float stopWithin = noNear ? 0.0f : evaluate(raw.distance);
			if (speed < 0.0f) { throw std::runtime_error("<chase object=\"" + raw.object + "\">: its speed is below 0"); }
			if (stopWithin < 0.0f) { throw std::runtime_error("<chase object=\"" + raw.object + "\">: the <near> is below 0"); }
			return CmdChase{ raw.object, speed, stopWithin };
		}
		if (verb == "aim")
		{
			const bool lined = !(raw.distance.kind == RawValue::Kind::Expression && raw.distance.text.empty());
			const float within = lined ? evaluate(raw.distance) : 0.0f;
			if (within < 0.0f) { throw std::runtime_error("<aim object=\"" + raw.object + "\">: the <lined> is below 0"); }
			return CmdAim{ raw.object, lined, within };
		}
		if (verb == "hold")
		{
			const KeyCode button = keyCodeFromString(raw.button);
			if (button == KeyCode::Unknown)
			{
				throw std::runtime_error("<hold button=\"" + raw.button + "\">: there is no key named '" + raw.button + "'");
			}
			return CmdHold{ button };
		}

		// The speed of a <go>, <hunt> or <wander>, which may not be below 0.
		const auto speedOf = [&](const char* what)
		{
			const float speed = evaluate(raw.amount);
			if (speed < 0.0f) { throw std::runtime_error(std::string("<") + what + ">: the speed is below 0"); }
			return speed;
		};

		if (verb == "go") { return CmdGo{ directionFromName(raw.direction, verb), speedOf("go") }; }
		if (verb == "wander") { return CmdWander{ speedOf("wander") }; }
		if (verb == "hunt" && raw.byHops)
		{
			const bool diagonal = !raw.positionX.text.empty() || raw.positionX.kind != RawValue::Kind::Expression;
			const float distance = diagonal ? 0.0f : evaluate(raw.amount);
			const float seconds = hopSeconds();
			return CmdHuntHop{ raw.object, diagonal ? evaluate(raw.positionX) : distance, diagonal ? evaluate(raw.positionY) : distance, diagonal, seconds, arcOf("hop", seconds) };
		}
		if (verb == "hunt")
		{
			CmdHunt hunt{ raw.object, speedOf("hunt"), 0.0f, raw.partner, raw.retreat, 0.0f };
			const bool noAhead = raw.ahead.kind == RawValue::Kind::Expression && raw.ahead.text.empty();
			if (!noAhead) { hunt.ahead = evaluate(raw.ahead); }
			if (!raw.retreat.empty())
			{
				hunt.retreatWithin = evaluate(raw.distance);
				if (hunt.retreatWithin < 0.0f) { throw std::runtime_error("<hunt object=\"" + raw.object + "\">: the <retreat> is below 0 tiles"); }
			}
			return hunt;
		}

		if (verb == "accelerate")
		{
			return CmdAccelerate{ directionFromName(raw.direction, verb), evaluate(raw.amount), raw.burn };
		}

		if (verb == "turn")
		{
			const Direction direction = directionFromName(raw.direction, verb);
			if (direction != Direction::Left && direction != Direction::Right)
			{
				throw std::runtime_error("<turn> has direction=\"" + raw.direction + "\"; expected left or right");
			}
			return CmdTurn{ direction, evaluate(raw.amount) };
		}

		if (verb == "thrust") { return CmdThrust{ evaluate(raw.amount), raw.burn }; }

		if (verb == "deflect")
		{
			const float maxAngle = evaluate(raw.amount);
			if (!(maxAngle >= 0.0f && maxAngle < 90.0f))
			{
				throw std::runtime_error("<deflect> has an angle of " + formatDisplayNumber(maxAngle) + "; expected 0 up to (not including) 90 degrees");
			}
			return CmdDeflect{ maxAngle };
		}

		if (verb == "knock")
		{
			const bool none = raw.amount.kind == RawValue::Kind::Expression && raw.amount.text.empty();
			const float bounce = none ? 1.0f : evaluate(raw.amount);
			if (!(bounce >= 0.0f && bounce <= 1.0f))
			{
				throw std::runtime_error("<knock> has a bounce of " + formatDisplayNumber(bounce) + "; expected 0 up to 1");
			}
			return CmdKnock{ bounce };
		}

		if (verb == "hole")
		{
			const float radius = evaluate(raw.amount);
			const bool smooth = raw.distance.kind == RawValue::Kind::Expression && raw.distance.text.empty();
			const float rough = smooth ? 0.0f : evaluate(raw.distance);
			if (!(radius > 0.0f)) { throw std::runtime_error("<hole> has a radius of " + formatDisplayNumber(radius) + "; expected above 0"); }
			if (!(rough >= 0.0f && rough <= 1.0f)) { throw std::runtime_error("<hole> has a <rough> of " + formatDisplayNumber(rough) + "; expected 0 up to 1"); }
			return CmdHole{ radius, rough };
		}

		if (verb == "release")
		{
			const int count = static_cast<int>(std::lround(evaluate(raw.amount)));
			if (count < 1) { throw std::runtime_error("<release> of " + raw.object + " has a count under 1"); }
			return CmdRelease{ raw.object, count };
		}

		// How much an <inc> or <dec> changes by: its amount, or 1 when it has none.
		const auto amountOf = [&](const RawCommand& command)
		{
			const bool none = command.amount.kind == RawValue::Kind::Expression && command.amount.text.empty();
			return none ? 1.0f : evaluate(command.amount);
		};

		if (verb == "inc") { return CmdIncrement{ raw.variable, amountOf(raw) }; }
		if (verb == "dec") { return CmdDecrement{ raw.variable, amountOf(raw) }; }
		if (verb == "copy") { return CmdCopy{ raw.variable, raw.into }; }
		if (verb == "rank") { return CmdRank{ raw.variable, raw.into }; }
		if (verb == "turns")
		{
			if (raw.players.empty()) { throw std::runtime_error("<turns> has no <player>; it needs one for each player"); }
			return CmdTurns{ raw.players };
		}
		if (verb == "pass") { return CmdPass{}; }
		if (verb == "out") { return CmdOut{}; }
		if (verb == "demo") { return CmdDemo{}; }

		if (verb == "push") { return CmdPushState{ raw.state }; }
		if (verb == "enter") { return CmdEnterScene{ raw.scene, raw.way }; }
		if (verb == "pop")  { return CmdPopState{ raw.state }; }

		if (verb == "play")    { return CmdPlay{ raw.sound }; }
		if (verb == "silence") { return CmdSilence{ raw.sound }; }
		if (verb == "become")  { return CmdBecome{ raw.sprite, raw.object, raw.from, raw.style }; }
		if (verb == "reveal")
		{
			const int count = static_cast<int>(std::lround(evaluate(raw.amount)));
			if (count < 1) { throw std::runtime_error("<reveal> of " + raw.object + " has a count under 1"); }
			return CmdReveal{ raw.object, count };
		}
		if (verb == "fire")    { return CmdFire{ raw.object }; }
		if (verb == "trigger") { return CmdTriggerAction{ raw.object, raw.action }; }
		if (verb == "follow")
		{
			const bool none = raw.seconds.kind == RawValue::Kind::Expression && raw.seconds.text.empty();
			const float stagger = none ? 0.0f : evaluate(raw.seconds);
			if (stagger < 0.0f) { throw std::runtime_error("<follow path=\"" + raw.path + "\">: the <stagger> is below 0"); }
			return CmdFollow{ raw.path, raw.object, stagger };
		}

		if (verb == "set") { return CmdSet{ raw.variable, evaluate(raw.amount) }; }
		if (verb == "despawn") { return CmdDespawn{ raw.object }; }

		if (verb == "spawn")
		{
			CmdSpawn spawn;
			spawn.target = raw.object;
			// A <centered> half holds where the middle goes; left empty, the
			// middle of the window.
			const auto half = [&](const RawValue& value, std::optional<float>& into, bool* centered, const char* middle)
			{
				if (value.kind == RawValue::Kind::Expression && value.text.empty()) { return; }
				if (value.kind == RawValue::Kind::Centered)
				{
					if (!centered)
					{
						throw std::runtime_error("<spawn object=\"" + raw.object + "\">: <centered> is for its <position>, not its <velocity>");
					}
					*centered = true;
					into = evaluate(RawValue::expression(value.text.empty() ? middle : value.text));
					return;
				}
				into = evaluate(value);
			};
			half(raw.positionX, spawn.x, &spawn.centerX, "window.width.center");
			half(raw.positionY, spawn.y, &spawn.centerY, "window.height.center");
			half(raw.velocityX, spawn.velocityX, nullptr, "");
			half(raw.velocityY, spawn.velocityY, nullptr, "");
			return spawn;
		}

		throw std::runtime_error("unknown command <" + verb + ">");
	}

	const Command& shapeOf(const Command& command) noexcept
	{
		if (const auto* live = std::get_if<CmdLive>(&command))
		{
			return live->loaded->command;
		}
		return command;
	}

	std::vector<Command> makeCommands(const std::vector<RawCommand>& raw, const ValueEvaluator& evaluate)
	{
		std::vector<Command> commands;
		commands.reserve(raw.size());

		for (const RawCommand& rawCommand : raw)
		{
			commands.push_back(makeCommand(rawCommand, evaluate));
		}

		return commands;
	}

	ShapeKind shapeKindFromTag(const std::string& tag) noexcept
	{
		if (tag == "circle")    { return ShapeKind::Circle; }
		if (tag == "rectangle") { return ShapeKind::Rectangle; }
		if (tag == "text")      { return ShapeKind::Text; }
		if (tag == "image")     { return ShapeKind::Image; }
		if (tag == "line")      { return ShapeKind::Line; }
		return ShapeKind::Unknown;
	}

	std::vector<std::string> circleSprite(float radius, const std::string& color)
	{
		return { "circle", std::to_string(radius), std::to_string(0), color };
	}

	std::vector<std::string> rectangleSprite(float width, float height, const std::string& color)
	{
		return { "rectangle", std::to_string(width), std::to_string(height), color };
	}

	Vector2f measureShapeSize(const std::vector<std::string>& spriteParams, ShapeKind shapeKind) noexcept
	{
		// spriteParams is always well-formed here - it was built moments
		// earlier from the object's <circle> or <rectangle> sprite (see
		// load/expr.cpp), never handed in from outside, so the indices
		// below are guaranteed present whenever shapeKind says they should
		// be - same assumption SFMLWindow::buildCircle/buildRectangle (and
		// their Raylib/SDL2 equivalents) already make.
		switch (shapeKind)
		{
		case ShapeKind::Circle:
		{
			const float radius = std::stof(spriteParams.at(1));
			return { radius * 2.0f, radius * 2.0f };
		}
		case ShapeKind::Rectangle:
		case ShapeKind::Line: // {"line", width, height}: the drawing's size, known when it was drawn
			return { std::stof(spriteParams.at(1)), std::stof(spriteParams.at(2)) };
		case ShapeKind::Text:
		case ShapeKind::Image:
		case ShapeKind::Unknown:
		default:
			return {};
		}
	}

	std::optional<std::pair<std::string, std::string>> parseVariableReference(const std::string& expression)
	{
		const auto first = expression.find_first_not_of(" \t\r\n");
		if (first == std::string::npos) { return std::nullopt; }
		const auto last = expression.find_last_not_of(" \t\r\n");
		const std::string reference = expression.substr(first, last - first + 1);

		// The variable is after the last dot; the owner, before it, may be a
		// path (playing.room1.tally.kills).
		const auto dot = reference.rfind('.');
		if (dot == std::string::npos || dot == 0 || dot == reference.size() - 1)
		{
			return std::nullopt;
		}

		const auto isIdentChar = [](char c) { return std::isalnum(static_cast<unsigned char>(c)) || c == '_'; };

		for (std::size_t i = 0; i < reference.size(); ++i)
		{
			const char c = reference[i];
			if (c == '.' ? (i == 0 || reference[i - 1] == '.') : !isIdentChar(c)) { return std::nullopt; }
		}

		return std::make_pair(reference.substr(0, dot), reference.substr(dot + 1));
	}

	std::string formatDisplayNumber(float value)
	{
		if (value == std::floor(value))
		{
			return std::to_string(static_cast<long long>(value));
		}
		return std::to_string(value);
	}

	std::ostream& operator<<(std::ostream& o, const Command& command)
	{
		std::visit(overload{
			[&](const CmdBounce&) { o << "bounce"; },
			[&](const CmdStick&) { o << "stick"; },
			[&](const CmdReset&) { o << "reset"; },
			[&](const CmdDie&) { o << "die"; },
			[&](const CmdDespawn& d) { o << "despawn" << (d.target.empty() ? "" : "(" + d.target + ")"); },
			[&](const CmdStop&) { o << "stop"; },
			[&](const CmdWrap&) { o << "wrap"; },
			[&](const CmdRide&) { o << "ride"; },
			[&](const CmdCarry&) { o << "carry"; },
			[&](const CmdDrop&) { o << "drop"; },
			[&](const CmdTrack& t) { o << "track" << (t.path.empty() ? "" : "(" + t.path + ")"); },
			[&](const CmdDeflect& d) { o << "deflect(" << d.maxAngle << ")"; },
			[&](const CmdKnock& k) { o << "knock(" << k.bounce << ")"; },
			[&](const CmdHold& h) { o << "hold(" << keyCodeToString(h.button) << ")"; },
			[&](const CmdHole& h) { o << "hole(" << h.radius << ", " << h.rough << ")"; },
			[&](const CmdReverse&) { o << "reverse"; },
			[&](const CmdMove& m)
			{
				const char* direction = (m.direction == Direction::Up) ? "up"
					: (m.direction == Direction::Down) ? "down"
					: (m.direction == Direction::Left) ? "left"
					: "right";
				o << "move." << direction << "(" << m.step << ")";
			},
			[&](const CmdHop& h)
			{
				const char* direction = (h.direction == Direction::Up) ? "up"
					: (h.direction == Direction::Down) ? "down"
					: (h.direction == Direction::Left) ? "left"
					: "right";
				if (h.step.x != 0.0f && h.step.y != 0.0f) { o << "hop(" << h.step.x << ", " << h.step.y; }
				else { o << "hop." << direction << "(" << h.distance; }
				o << (h.seconds > 0.0f ? ", " + formatDisplayNumber(h.seconds) + "s" : std::string{})
					<< (h.arc > 0.0f ? ", arc " + formatDisplayNumber(h.arc) : std::string{}) << ")";
			},
			[&](const CmdHuntHop& h)
			{
				o << "hunt(" << h.target << ", by " << (h.diagonal ? "diagonal " : "") << "hops of " << h.across << ", " << h.down
					<< (h.seconds > 0.0f ? ", " + formatDisplayNumber(h.seconds) + "s" : std::string{})
					<< (h.arc > 0.0f ? ", arc " + formatDisplayNumber(h.arc) : std::string{}) << ")";
			},
			[&](const CmdJump& j)
			{
				const char* direction = (j.direction == Direction::Up) ? "up"
					: (j.direction == Direction::Down) ? "down"
					: (j.direction == Direction::Left) ? "left"
					: "right";
				o << "jump." << direction << "(" << j.distance << ", " << j.seconds << "s"
					<< (j.arc > 0.0f ? ", arc " + formatDisplayNumber(j.arc) : std::string{}) << ")";
			},
			[&](const CmdLand&) { o << "land()"; },
			[&](const CmdLeap& l) { o << "leap(" << l.height << ")"; },
			[&](const CmdClimb& c) { o << "climb." << (c.direction == Direction::Up ? "up" : "down") << "(" << c.step << ", " << c.ladderClass << ")"; },
			[&](const CmdChase& c) { o << "chase(" << c.target << ", " << c.speed << (c.stopWithin > 0.0f ? ", near " + formatDisplayNumber(c.stopWithin) : std::string{}) << ")"; },
			[&](const CmdGo& g)
			{
				const char* direction = (g.direction == Direction::Up) ? "up"
					: (g.direction == Direction::Down) ? "down"
					: (g.direction == Direction::Left) ? "left"
					: "right";
				o << "go." << direction << "(" << g.speed << ")";
			},
			[&](const CmdHunt& h)
			{
				o << "hunt(" << h.target << ", " << h.speed;
				if (h.ahead != 0.0f) { o << ", ahead " << h.ahead; }
				if (!h.partner.empty()) { o << ", with " << h.partner; }
				if (!h.retreatTo.empty()) { o << ", retreat " << h.retreatTo << " within " << h.retreatWithin; }
				o << ")";
			},
			[&](const CmdWander& w) { o << "wander(" << w.speed << ")"; },
			[&](const CmdAim& a) { o << "aim(" << a.target << (a.lined ? ", lined " + std::to_string(a.within) : std::string()) << ")"; },
			[&](const CmdAccelerate& a)
			{
				const char* direction = (a.direction == Direction::Up) ? "up"
					: (a.direction == Direction::Down) ? "down"
					: (a.direction == Direction::Left) ? "left"
					: "right";
				o << "accelerate." << direction << "(" << a.amount << (a.burn.empty() ? "" : ", burn " + a.burn) << ")";
			},
			[&](const CmdTurn& t) { o << "turn." << (t.direction == Direction::Left ? "left" : "right") << "(" << t.rate << ")"; },
			[&](const CmdThrust& t) { o << "thrust(" << t.amount << (t.burn.empty() ? "" : ", burn " + t.burn) << ")"; },
			[&](const CmdRelease& r) { o << "release(" << r.target << ", " << r.count << ")"; },
			[&](const CmdIncrement& c) { o << "inc(" << c.target << (c.amount == 1.0f ? "" : ", " + formatDisplayNumber(c.amount)) << ")"; },
			[&](const CmdDecrement& c) { o << "dec(" << c.target << (c.amount == 1.0f ? "" : ", " + formatDisplayNumber(c.amount)) << ")"; },
			[&](const CmdCopy& c) { o << "copy(" << c.from << ", " << c.into << ")"; },
			[&](const CmdRank& c) { o << "rank(" << c.entry << ", " << c.table << ")"; },
			[&](const CmdTurns& c)
			{
				o << "turns(";
				for (std::size_t i = 0; i < c.players.size(); ++i) { o << (i ? ", " : "") << (c.players[i].empty() ? "-" : c.players[i]); }
				o << ")";
			},
			[&](const CmdPass&) { o << "pass()"; },
			[&](const CmdOut&) { o << "out()"; },
			[&](const CmdDemo&) { o << "demo()"; },
			[&](const CmdPushState& s) { o << "state(" << s.name << ")"; },
			[&](const CmdPopState& s) { o << "state()"; if (!s.name.empty()) { o << " state(" << s.name << ")"; } },
			[&](const CmdFire& f) { o << "fire(" << f.projectileName << ")"; },
			[&](const CmdTriggerAction& a) { o << "action(" << a.object << "," << a.action << ")"; },
			[&](const CmdResetObject& r) { o << "reset(" << r.target << ")"; },
			[&](const CmdPlay& p) { o << "play(" << p.sound << ")"; },
			[&](const CmdSilence& s) { o << "silence(" << s.sound << ")"; },
			[&](const CmdBecome& b) { o << "become(" << b.sprite << (b.target.empty() ? "" : ", " + b.target) << (b.from.empty() ? "" : ", from " + b.from) << (b.style.empty() ? "" : ", style " + b.style) << ")"; },
			[&](const CmdReveal& r) { o << "reveal(" << r.target << ", " << r.count << ")"; },
			[&](const CmdFollow& f) { o << "follow(" << f.path << (f.target.empty() ? "" : ", " + f.target) << ")"; },
			[&](const CmdEnterScene& e) { o << "enter(" << (e.way.empty() ? e.scene : "way " + e.way) << ")"; },
			[&](const CmdResetScene& r) { o << "reset(" << (r.way.empty() ? "scene " + r.scene : "way " + r.way) << ")"; },
			[&](const CmdSet& s) { o << "set(" << s.target << ", " << s.value << ")"; },
			[&](const CmdSpawn& s)
			{
				const auto half = [&](const std::optional<float>& value, bool centered)
				{
					if (!value) { o << "-"; return; }
					o << (centered ? "centered " : "") << *value;
				};
				o << "spawn(" << s.target << ", ";
				half(s.x, s.centerX);
				o << " ";
				half(s.y, s.centerY);
				o << ", ";
				half(s.velocityX, false);
				o << " ";
				half(s.velocityY, false);
				o << ")";
			},
			[&](const CmdLive& l) { o << "live " << l.loaded->command; },
		}, command);

		return o;
	}

	const OperationShape* operationShape(const std::string& tag)
	{
		static const OperationShape shapes[] = {
			{ "add", "augend", "addend", '+' },
			{ "subtract", "minuend", "subtrahend", '-' },
			{ "multiply", "multiplicand", "multiplier", '*' },
			{ "divide", "dividend", "divisor", '/' },
			{ "smallest", "of", "of", 0, "min" },
			{ "largest", "of", "of", 0, "max" },
			{ "absolute", "of", "of", 0, "abs", 1, 1 },
			{ "clamp", "of", "atleast", 0, "clamp", 3, 3, "atmost" },
			// A <random> with a <formula> end (RawValue::operations).
			{ "random", "min", "max", 0, "random", 2, 2 },
		};

		for (const OperationShape& shape : shapes)
		{
			if (tag == shape.tag) { return &shape; }
		}

		return nullptr;
	}

	namespace
	{
		std::string operationText(const RawOperation& operation)
		{
			const OperationShape* shape = operationShape(operation.op);
			const bool infix = shape && shape->symbol != 0;
			const std::string between = infix ? " " + std::string(1, shape->symbol) + " " : ", ";

			std::string text = infix ? "(" : (shape ? shape->function : operation.op) + std::string("(");
			for (std::size_t i = 0; i < operation.operands.size(); ++i)
			{
				if (i > 0) { text += between; }
				text += valueText(operation.operands[i].value);
			}
			return text + ")";
		}
	}

	std::string valueText(const RawValue& value)
	{
		switch (value.kind)
		{
		case RawValue::Kind::Random:
			if (value.operations && !value.operations->empty()) { return operationText(value.operations->front()); }
			return "random(" + value.min + ", " + value.max + ")";

		case RawValue::Kind::Centered:
			return "centered(" + value.text + ")";

		case RawValue::Kind::Formula:
			return value.operations && !value.operations->empty() ? operationText(value.operations->front()) : "";

		case RawValue::Kind::Expression:
			break;
		}

		return value.text;
	}

	std::vector<const std::string*> RawValue::expressions() const
	{
		if (kind == Kind::Random && !operations) { return { &min, &max }; }

		if (kind == Kind::Formula || kind == Kind::Random)
		{
			std::vector<const std::string*> all;
			if (operations)
			{
				for (const RawOperation& operation : *operations)
				{
					for (const RawOperand& operand : operation.operands)
					{
						for (const std::string* expression : operand.value.expressions()) { all.push_back(expression); }
					}
				}
			}
			return all;
		}

		return { &text };
	}

	bool RawValue::drawsRandom() const
	{
		if (kind == Kind::Random) { return true; }
		if (!operations) { return false; }
		for (const RawOperation& operation : *operations)
		{
			for (const RawOperand& operand : operation.operands)
			{
				if (operand.value.drawsRandom()) { return true; }
			}
		}
		return false;
	}

	std::ostream& operator<<(std::ostream& o, const RawValue& value)
	{
		return o << valueText(value);
	}

	std::ostream& operator<<(std::ostream& o, const RawCommand& command)
	{
		o << command.verb;

		if (command.verb == "thrust" || command.verb == "deflect")
		{
			o << "(" << command.amount << (command.burn.empty() ? "" : ", burn " + command.burn) << ")";
		}
		else if (command.verb == "hold")
		{
			o << "(" << command.button << ")";
		}
		else if (command.verb == "hole")
		{
			o << "(" << command.amount << ", " << command.distance << ")";
		}
		else if (command.verb == "release")
		{
			o << "(" << command.object << ", " << command.amount << ")";
		}
		else if (command.verb == "jump")
		{
			o << "." << command.direction << "(" << command.amount << ", " << command.seconds << ")";
		}
		else if (command.verb == "leap" || (command.verb == "knock" && (command.amount.kind != RawValue::Kind::Expression || !command.amount.text.empty())))
		{
			o << "(" << command.amount << ")";
		}
		else if (command.verb == "climb")
		{
			o << "." << command.direction << "(" << command.amount << ", " << command.objClass << ")";
		}
		else if (command.verb == "hop" || (command.verb == "hunt" && command.byHops))
		{
			o << (command.byHops ? "(" + command.object + ", by hops of " : "." + command.direction + "(") << command.amount << ", " << command.positionX << " " << command.positionY
				<< ", " << command.seconds << ")";
		}
		else if (command.verb == "move" || command.verb == "accelerate" || command.verb == "turn" || command.verb == "go")
		{
			o << "." << command.direction << "(" << command.amount << (command.burn.empty() ? "" : ", burn " + command.burn) << ")";
		}
		else if (command.verb == "inc" || command.verb == "dec")
		{
			o << "(" << command.variable;
			if (!(command.amount.kind == RawValue::Kind::Expression && (command.amount.text == "1" || command.amount.text.empty()))) { o << ", " << command.amount; }
			o << ")";
		}
		else if (command.verb == "push")
		{
			o << "(" << command.state << ")";
		}
		else if (command.verb == "enter" || (command.verb == "reset" && !command.scene.empty()))
		{
			o << "(scene " << command.scene << ")";
		}
		else if (command.verb == "chase")
		{
			o << "(" << command.object << ", " << command.amount << ", " << command.distance << ")";
		}
		else if (command.verb == "hunt")
		{
			o << "(" << command.object << ", " << command.amount << ", ahead " << command.ahead << ", with " << command.partner
				<< ", retreat " << command.retreat << " " << command.distance << ")";
		}
		else if (command.verb == "wander")
		{
			o << "(" << command.amount << ")";
		}
		else if (command.verb == "fire" || command.verb == "aim" || (command.verb == "reset" && !command.object.empty()) || (command.verb == "despawn" && !command.object.empty()))
		{
			o << "(" << command.object << ")";
		}
		else if (command.verb == "play" || command.verb == "silence")
		{
			o << "(" << command.sound << ")";
		}
		else if (command.verb == "trigger")
		{
			o << "(" << command.object << "," << command.action << ")";
		}
		else if (command.verb == "follow")
		{
			o << "(" << command.path << (command.object.empty() ? "" : ", " + command.object) << ")";
		}
		else if (command.verb == "track" && !command.path.empty())
		{
			o << "(" << command.path << ")";
		}
		else if (command.verb == "set")
		{
			o << "(" << command.variable << ", " << command.amount << ")";
		}
		else if (command.verb == "spawn")
		{
			o << "(" << command.object << ", " << command.positionX << " " << command.positionY << ", " << command.velocityX << " " << command.velocityY << ")";
		}

		return o;
	}

	std::ostream& operator<<(std::ostream& o, const std::vector<RawCommand>& commands)
	{
		for (std::size_t i = 0; i < commands.size(); ++i)
		{
			o << (i ? ";" : "") << commands[i];
		}

		return o;
	}
}
