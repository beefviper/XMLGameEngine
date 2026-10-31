// test_generated_games.cpp
// XML Game Engine
// author: beefviper
// date: Oct 11, 2026
//
// Catch2 tests that a generated program plays its game as xgecli does: each
// game in games/ is generated as the build runs (tests.cmake), by the
// stylesheets in games/generators/windows-cpp, and its data compiled in here. The
// data must be what the engine reads from the file, every field of it; and
// played beside the game read from its file, the same keys pressed at the
// same frames and the same random numbers drawn, every object is compared
// every frame.

#include "engine/engine.h"
#include "game/game.h"
#include "load/expr.h"
#include "load/xml.h"
#include "generated_games.h"
#include "window/window.h"

#include <catch2/catch_test_macros.hpp>

#include <algorithm>
#include <map>
#include <memory>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

using namespace xge;

namespace
{
	// Just enough of a Window for Engine: nothing is drawn, and the keys are
	// pressed by the test (Engine::handleKey*).
	class FakeWindow : public Window
	{
	public:
		bool isOpen() const override { return true; }
		void close() override {}
		void init(std::vector<Object>&) override {}
		std::vector<std::pair<KeyCode, bool>> pollEvents() override { return {}; }
		void clear(const std::string&) override {}
		void draw(Object&) override {}
		void display() override {}
	};

	// Everything about a frame that playing shows: the state, and each
	// object's place, motion, whether it is in play, its look and variables.
	std::string snapshot(Game& game)
	{
		std::ostringstream out;
		out << "state " << game.getCurrentState().name << '\n';
		for (const Object& object : game.getCurrentObjects())
		{
			out << object.name << " at " << object.position.x << ',' << object.position.y
				<< " moving " << object.velocity.x << ',' << object.velocity.y
				<< (object.isVisible ? " shown" : " hidden")
				<< (object.collisionsOn ? " touching" : "")
				<< " look " << object.lookName() << " heading " << object.heading;
			for (const auto& [name, value] : object.variable)
			{
				out << ' ' << name << '=' << value;
			}
			out << '\n';
		}
		return out.str();
	}

	// The keys pressed: Space first (it starts every game), then each of the
	// others held for a while in turn, so the players move, jump and fire.
	const KeyCode script[] = {
		KeyCode::Space, KeyCode::Left, KeyCode::Space, KeyCode::Right, KeyCode::Up, KeyCode::A,
		KeyCode::Space, KeyCode::D, KeyCode::Down, KeyCode::W, KeyCode::S, KeyCode::Space,
	};

	constexpr int frames = 900;

	// The game played for `frames` frames, a snapshot a frame.
	std::vector<std::string> play(Game& game)
	{
		Engine engine(game, std::make_unique<FakeWindow>());
		std::vector<std::string> shots;
		for (int frame = 0; frame < frames; ++frame)
		{
			const KeyCode key = script[static_cast<std::size_t>(frame / 40) % std::size(script)];
			if (frame % 40 == 0) { engine.handleKeyPressed(key); }
			if (frame % 40 == 25) { engine.handleKeyReleased(key); }
			engine.step();
			shots.push_back(snapshot(game));
		}
		return shots;
	}

	// Every field of a game's data, a line each, so two can be compared and
	// the first place they differ shown.
	class Describe
	{
	public:
		std::ostringstream out;

		void value(const std::string& at, const RawValue& v)
		{
			out << at << " = " << static_cast<int>(v.kind) << " [" << v.text << "] [" << v.min << "] [" << v.max << "]\n";
			if (v.operations)
			{
				for (std::size_t i = 0; i < v.operations->size(); ++i)
				{
					const RawOperation& op = (*v.operations)[i];
					const std::string here = at + ".op" + std::to_string(i);
					out << here << " " << op.op << "\n";
					for (std::size_t j = 0; j < op.operands.size(); ++j)
					{
						value(here + "." + op.operands[j].role + std::to_string(j), op.operands[j].value);
					}
				}
			}
		}

		void text(const std::string& at, const std::string& t) { out << at << " = [" << t << "]\n"; }
		void flag(const std::string& at, bool b) { out << at << " = " << b << "\n"; }

		void vector2(const std::string& at, const RawVector2& v)
		{
			value(at + ".x", v.x);
			value(at + ".y", v.y);
		}

		void command(const std::string& at, const RawCommand& c)
		{
			out << at << " " << c.verb << " [" << c.object << "|" << c.variable << "|" << c.state << "|" << c.action << "|" << c.direction
				<< "|" << c.burn << "|" << c.sound << "|" << c.sprite << "|" << c.path << "|" << c.objClass << "|" << c.scene << "|" << c.way << "|" << c.into
				<< "|" << c.from << "|" << c.partner << "|" << c.retreat << "|" << c.button << "]\n";
			flag(at + ".byHops", c.byHops);
			value(at + ".amount", c.amount);
			value(at + ".ahead", c.ahead);
			value(at + ".seconds", c.seconds);
			value(at + ".distance", c.distance);
			value(at + ".positionX", c.positionX);
			value(at + ".positionY", c.positionY);
			value(at + ".velocityX", c.velocityX);
			value(at + ".velocityY", c.velocityY);
			value(at + ".accelerationX", c.accelerationX);
			value(at + ".accelerationY", c.accelerationY);
			for (const std::string& player : c.players) { out << at << ".player " << player << "\n"; }
		}

		void commands(const std::string& at, const std::vector<RawCommand>& list)
		{
			out << at << " count " << list.size() << "\n";
			for (std::size_t i = 0; i < list.size(); ++i) { command(at + "[" + std::to_string(i) + "]", list[i]); }
		}

		void timers(const std::string& at, const std::vector<RawTimer>& list)
		{
			out << at << " count " << list.size() << "\n";
			for (std::size_t i = 0; i < list.size(); ++i)
			{
				const std::string here = at + "[" + std::to_string(i) + "]";
				flag(here + ".repeat", list[i].repeat);
				text(here + ".sprite", list[i].sprite);
				value(here + ".interval", list[i].interval);
				commands(here + ".commands", list[i].commands);
			}
		}

		void sprite(const std::string& at, const RawSprite& s)
		{
			text(at + ".kind", s.kind);
			text(at + ".name", s.name);
			out << at << ".lines count " << s.lines.size() << "\n";
			for (std::size_t i = 0; i < s.lines.size(); ++i)
			{
				const std::string here = at + ".line" + std::to_string(i);
				vector2(here + ".from", s.lines[i].from);
				vector2(here + ".to", s.lines[i].to);
				text(here + ".color", s.lines[i].color);
				value(here + ".thickness", s.lines[i].thickness);
				flag(here + ".hasThickness", s.lines[i].hasThickness);
			}
			out << at << ".polygons count " << s.polygons.size() << "\n";
			for (std::size_t i = 0; i < s.polygons.size(); ++i)
			{
				const std::string here = at + ".polygon" + std::to_string(i);
				for (const RawVector2& point : s.polygons[i].points) { vector2(here + ".point", point); }
				text(here + ".color", s.polygons[i].color);
			}
			for (const std::string& row : s.bitmapRows) { text(at + ".row", row); }
			for (const auto& [character, color] : s.bitmapColors) { text(at + ".ink." + std::string(1, character), color); }
			value(at + ".scale", s.scale);
			flag(at + ".hasScale", s.hasScale);
			value(at + ".svgX", s.svgX);
			value(at + ".svgY", s.svgY);
			value(at + ".svgWidth", s.svgWidth);
			value(at + ".svgHeight", s.svgHeight);
			flag(at + ".hasSvgRegion", s.hasSvgRegion);
			for (const std::string& hide : s.svgHide) { text(at + ".hide", hide); }
			value(at + ".radius", s.radius);
			value(at + ".width", s.width);
			value(at + ".height", s.height);
			value(at + ".size", s.size);
			for (const RawTextPiece& piece : s.text)
			{
				text(at + ".text.kind", std::to_string(static_cast<int>(piece.kind)));
				text(at + ".text.words", piece.words);
				value(at + ".text.value", piece.value);
				for (const std::string& choice : piece.choices) { text(at + ".text.choice", choice); }
			}
			text(at + ".path", s.path);
			text(at + ".flip", s.flip);
			text(at + ".color", s.color);
			text(at + ".style", s.style);
			text(at + ".tetherFrom", s.tetherFrom);
			text(at + ".tetherTo", s.tetherTo);
			value(at + ".thickness", s.thickness);
			value(at + ".interval", s.interval);
			for (std::size_t i = 0; i < s.frames.size(); ++i) { sprite(at + ".frame" + std::to_string(i), s.frames[i]); }
		}

		void guards(const std::string& at, const std::vector<EdgeGuard>& list)
		{
			for (const EdgeGuard& g : list) { out << at << " " << g.first << " " << g.count << " [" << g.sprite << "] [" << g.unlessClass << "]\n"; }
		}

		void object(const std::string& at, const RawObject& o)
		{
			text(at + ".name", o.name);
			text(at + ".class", o.objClass);
			sprite(at + ".sprite", o.sprite);
			flag(at + ".hasAnimation", o.hasAnimation);
			value(at + ".animation.interval", o.animation.interval);
			for (std::size_t i = 0; i < o.animation.frames.size(); ++i) { sprite(at + ".frame" + std::to_string(i), o.animation.frames[i]); }
			for (std::size_t i = 0; i < o.looks.size(); ++i) { sprite(at + ".look" + std::to_string(i), o.looks[i]); }
			flag(at + ".isVisible", o.isVisible);
			flag(at + ".isDrawn", o.isDrawn);
			flag(at + ".exists", o.exists);
			text(at + ".most", std::to_string(o.most));
			vector2(at + ".position", o.rawPosition);
			vector2(at + ".velocity", o.rawVelocity);
			vector2(at + ".acceleration", o.rawAcceleration);
			flag(at + ".hasAcceleration", o.hasAcceleration);
			vector2(at + ".pivot", o.rawPivot);
			flag(at + ".hasPivot", o.hasPivot);
			value(at + ".heading", o.rawHeading);
			flag(at + ".hasHeading", o.hasHeading);
			value(at + ".drag", o.rawDrag);
			flag(at + ".hasDrag", o.hasDrag);
			value(at + ".friction", o.rawFriction);
			flag(at + ".hasFriction", o.hasFriction);
			value(at + ".mass", o.rawMass);
			flag(at + ".hasMass", o.hasMass);
			for (const auto& [name, strength] : o.rawPulls) { value(at + ".pull." + name, strength); }
			text(at + ".seekTarget", o.seekTarget);
			value(at + ".seek", o.rawSeek);
			value(at + ".light", o.rawLight);
			flag(at + ".hasLight", o.hasLight);
			text(at + ".dark", o.dark);
			flag(at + ".hasAltitude", o.hasAltitude);
			value(at + ".altitude.start", o.rawAltitude);
			value(at + ".altitude.rise", o.rawRise);
			value(at + ".altitude.fall", o.rawFall);
			value(at + ".altitude.bounce", o.rawAltitudeBounce);
			text(at + ".shadowOf", o.shadowOf);
			text(at + ".facing", o.facing);
			for (const auto& [pose, look] : o.poses) { text(at + ".pose." + pose, look); }
			for (const auto& [join, look] : o.joins) { text(at + ".join." + join, look); }
			text(at + ".mirror", o.mirror);
			flag(at + ".hasGrip", o.hasGrip);
			value(at + ".grip", o.rawGrip);
			timers(at + ".timers", o.timers);
			const RawCollisionData& c = o.rawCollisionData;
			flag(at + ".collisions.enabled", c.enabled);
			flag(at + ".collisions.lockstep", c.lockstep);
			out << at << ".collisions.type " << static_cast<int>(c.type) << "\n";
			text(at + ".collisions.solid", c.solid);
			text(at + ".collisions.hitbox", c.hitboxKind);
			value(at + ".collisions.hitbox.width", c.hitboxWidth);
			value(at + ".collisions.hitbox.height", c.hitboxHeight);
			value(at + ".collisions.hitbox.radius", c.hitboxRadius);
			value(at + ".collisions.hitbox.x", c.hitboxX);
			value(at + ".collisions.hitbox.y", c.hitboxY);
			for (const Edge edge : { Edge::Top, Edge::Bottom, Edge::Left, Edge::Right })
			{
				commands(at + "." + edgeName(edge), c.edges[edgeIndex(edge)].commands);
				guards(at + "." + edgeName(edge) + "Guards", c.edges[edgeIndex(edge)].guards);
			}
			for (std::size_t i = 0; i < c.others.size(); ++i)
			{
				const RawCollisionRule& r = c.others[i];
				const std::string here = at + ".rule" + std::to_string(i);
				out << here << " [" << r.filterClass << "|" << r.filterObject << "|" << r.unlessClass << "|" << r.whileSprite << "|" << r.side << "] join " << static_cast<int>(r.check.join) << "\n";
				conditions(here + ".check", r.check.parts);
				if (r.slower) { value(here + ".slower", *r.slower); }
				if (r.faster) { value(here + ".faster", *r.faster); }
				commands(here + ".commands", r.commands);
			}
			for (const auto& [name, list] : o.action) { commands(at + ".action." + name, list); }
			for (const auto& [name, v] : o.variable) { value(at + ".variable." + name, v); }
			text(at + ".group", o.groupName);
			for (const std::string& group : o.outerGroups) { text(at + ".outer", group); }
			text(at + ".maze", o.maze.map);
			for (const MazeWall& wall : o.maze.walls) { out << at << ".wall [" << wall.objClass << "|" << wall.sprite << "]\n"; }
			out << at << ".map [" << o.cell.map << "] " << o.cell.mapColumn << " " << o.cell.mapRow << " " << o.cell.mapColumns << " " << o.cell.mapRows << "\n";
			value(at + ".tileWidth", o.cell.tileWidth);
			value(at + ".tileHeight", o.cell.tileHeight);
			out << at << ".cell " << o.cell.column << " " << o.cell.row << "\n";
			out << at << ".cells " << o.cell.cells << " [" << o.cell.character << "]\n";
			for (const RawCheck& check : o.cell.checks) { commands(at + ".check." + check.kind + "." + check.objClass + "." + std::to_string(check.count) + (check.straight ? ".straight" : "") + "." + check.alike, check.commands); }
			for (const std::string& row : o.shapeRows) { text(at + ".shape", row); }
			text(at + ".chain", o.chain);
			for (const auto& [character, piece] : o.shapePieces) { out << at << ".shapePiece [" << character << "|" << piece << "]\n"; }
			sprite(at + ".cell.slot", o.cell.slot);
			value(at + ".cell.gapBefore", o.cell.gapBefore);
			value(at + ".cell.gapAbove", o.cell.gapAbove);
		}

		void conditions(const std::string& at, const std::vector<RawCondition>& list)
		{
			for (std::size_t i = 0; i < list.size(); ++i)
			{
				const RawCondition& c = list[i];
				const std::string here = at + ".condition" + std::to_string(i);
				out << here << " [" << c.filterClass << "|" << c.filterObject << "|" << c.variableName << "|" << c.total << "|" << c.distance << "] "
					<< static_cast<int>(c.test) << " join " << static_cast<int>(c.join) << " name " << c.name << "\n";
				value(here + ".threshold", c.threshold);
				conditions(here + ".parts", c.parts);
				commands(here + ".commands", c.commands);
			}
		}

		void game(const GameData& g)
		{
			out << "window [" << g.window.name << "] " << g.window.width << " " << g.window.height << " x" << g.window.scale << " [" << g.window.background << "] ["
				<< g.window.fullscreen << "] " << g.window.framerate << "\n";
			for (const auto& [name, v] : g.variables) { value("variable " + name, v); }
			for (const RawObject& o : g.objects) { object("object " + o.name, o); }
			for (const RawState& s : g.states)
			{
				const std::string at = "state " + s.name;
				for (const std::string& shown : s.show) { text(at + ".show", shown); }
				for (const std::string& shown : s.showStates) { text(at + ".showState", shown); }
				text(at + ".music", s.music);
				for (std::size_t moment = 0; moment < keyMoments.size(); ++moment)
				{
					for (const auto& [button, list] : *s.keyMaps()[moment]) { commands(at + ".input." + keyMoments[moment] + "." + button, list); }
				}
				for (const auto& [axis, list] : s.axes) { commands(at + ".axis." + axis, list); }
				conditions(at, s.conditions);
				conditions(at + ".computer", s.computer);
				timers(at + ".timers", s.timers);
				for (const RawObject& o : s.objects) { object(at + ".object " + o.name, o); }
				text(at + ".from", s.from);
				for (const RawScene& scene : s.scenes)
				{
					const std::string here = at + ".scene " + scene.name;
					text(here + ".from", scene.from);
					for (const auto& [way, to] : scene.ways) { text(here + ".way " + way, to); }
					for (const auto& [name, v] : scene.variables) { value(here + ".variable " + name, v); }
					for (const RawObject& o : scene.objects) { object(here + ".object " + o.name, o); }
					conditions(here, scene.conditions);
					timers(here + ".timers", scene.timers);
				}
			}
			for (const RawSound& s : g.sounds)
			{
				const std::string at = "sound " + s.name;
				text(at + ".wave", s.wave);
				flag(at + ".loop", s.loop);
				value(at + ".volume", s.volume);
				for (std::size_t i = 0; i < s.notes.size(); ++i)
				{
					const RawNote& n = s.notes[i];
					const std::string here = at + ".note" + std::to_string(i);
					out << here << " " << n.rest << " [" << n.wave << "|" << n.pitch << "|" << n.to << "] part " << n.part << "\n";
					value(here + ".length", n.length);
				}
			}
			for (const std::string& keep : g.keeps) { text("keep", keep); }
			for (const RawStyle& style : g.styles)
			{
				value("style " + style.name + ".size", style.size);
				text("style " + style.name + ".color", style.color);
				if (style.hasScale) { value("style " + style.name + ".scale", style.scale); }
				for (const auto& [character, color] : style.bitmapColors) { text("style " + style.name + ".ink." + std::string(1, character), color); }
			}
			flag("demo", g.demo.recorded);
			text("demo.seed", std::to_string(g.demo.seed));
			text("demo.frames", std::to_string(g.demo.frames));
			for (const DemoKey& key : g.demo.keys) { text("demo.key", key.button + (key.pressed ? " down " : " up ") + std::to_string(key.frame)); }
			for (const RawPath& p : g.paths)
			{
				const std::string at = "path " + p.name;
				value(at + ".speed", p.speed);
				flag(at + ".hasStart", p.hasStart);
				value(at + ".startX", p.startX);
				value(at + ".startY", p.startY);
				for (std::size_t i = 0; i < p.steps.size(); ++i)
				{
					const std::string here = at + ".step" + std::to_string(i);
					flag(here + ".home", p.steps[i].home);
					value(here + ".x", p.steps[i].x);
					value(here + ".y", p.steps[i].y);
					commands(here + ".commands", p.steps[i].commands);
				}
			}
		}
	};

	std::vector<std::string> describe(const GameData& game)
	{
		Describe d;
		d.game(game);
		std::vector<std::string> lines;
		std::istringstream in(d.out.str());
		for (std::string line; std::getline(in, line);) { lines.push_back(line); }
		return lines;
	}

	struct SeedReset
	{
		~SeedReset() { game_expr::fixRandomSeed(std::nullopt); }
	};
}

TEST_CASE("every generated game's data is what the engine reads from its file, field for field", "[generate][generated_games]")
{
	for (const GeneratedGame& each : generatedGames)
	{
		INFO(each.file);
		const std::vector<std::string> expected = describe(readGame(each.file));
		const std::vector<std::string> written = describe(each.describe());

		for (std::size_t line = 0; line < std::max(expected.size(), written.size()); ++line)
		{
			const std::string want = line < expected.size() ? expected[line] : "(nothing)";
			const std::string got = line < written.size() ? written[line] : "(nothing)";
			if (want != got)
			{
				INFO("line " << line << "\nfrom the file: " << want << "\ngenerated:     " << got);
				FAIL("the generated data differs from the file");
			}
		}
	}
}

TEST_CASE("every generated game plays frame for frame as the engine plays its file", "[generate][generated_games]")
{
	const SeedReset reset;
	for (const GeneratedGame& each : generatedGames)
	{
		INFO(each.file);

		game_expr::fixRandomSeed(20261011u);
		Game fromFile{ std::string(each.file) };
		const std::vector<std::string> expected = play(fromFile);
		CHECK(expected.front() != expected.back()); // the keys did play it

		game_expr::fixRandomSeed(20261011u);
		Game generated{ each.describe() };
		const std::vector<std::string> played = play(generated);

		REQUIRE(played.size() == expected.size());
		for (std::size_t frame = 0; frame < expected.size(); ++frame)
		{
			if (played[frame] != expected[frame])
			{
				INFO("frame " << frame << "\nfrom the file:\n" << expected[frame] << "\ngenerated:\n" << played[frame]);
				FAIL("the generated game played differently");
			}
		}
	}
}
