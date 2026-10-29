// test_sound.cpp
// XML Game Engine
// author: beefviper
// date: Oct 3, 2026
//
// Catch2 tests for the sound system: pitch names, the synthesizer
// (audio/sound.h), reading <sounds> and <play> from a game file, the checks made
// when it loads, Pong asking for its sounds when the ball bounces and
// scores, the eight games that were silent now having sounds, and Engine
// handing what was asked for to its Audio (audio/audio.h). No
// test opens a sound device: Engine is given a recording Audio instead.

#include "scratch.h"
#include "file_text.h"
#include "engine/engine.h"
#include "game/game.h"
#include "load/xml.h"
#include "audio/sound.h"

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_string.hpp>

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <memory>
#include <set>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

using namespace xge;
using Catch::Approx;
using Catch::Matchers::ContainsSubstring;

namespace
{
	// Just enough of a Window for Engine's constructor (see test_engine_input.cpp).
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

	// What an Audio was asked to do, kept outside it so a test can read it
	// after Engine has destroyed the Audio.
	struct AudioLog
	{
		std::vector<std::string> loaded;
		std::vector<std::string> played;
		std::vector<std::string> looped;
		std::vector<std::string> stopped;
		int stops{ 0 };
	};

	class RecordingAudio : public Audio
	{
	public:
		explicit RecordingAudio(AudioLog& audioLog) : log(audioLog) {}

		void load(const std::vector<SoundDesc>& sounds) override
		{
			log.loaded.clear();
			for (const auto& sound : sounds) { log.loaded.push_back(sound.name); }
		}
		void play(const std::string& name) override { log.played.push_back(name); }
		void loop(const std::string& name) override { log.looped.push_back(name); }
		void stop(const std::string& name) override { log.stopped.push_back(name); }
		void stopAll() override { ++log.stops; }

	private:
		AudioLog& log;
	};

	// Removes a scratch game file when it goes out of scope, pass or fail.
	struct ScratchFile
	{
		std::filesystem::path path;

		explicit ScratchFile(std::filesystem::path where) : path(std::move(where)) {}
		ScratchFile(const ScratchFile&) = delete;
		ScratchFile& operator=(const ScratchFile&) = delete;
		~ScratchFile()
		{
			std::error_code ignored;
			std::filesystem::remove(path, ignored);
		}
	};

	// A one-object game with the given <sounds> and the given commands on
	// Space; no schema is named, so only what load/xml and load/expr check
	// is checked.
	std::string gameWithSounds(const std::string& sounds, const std::string& onSpace = "<pop />")
	{
		return "<game>"
			"<window name=\"test\"><width>800</width><height>600</height><background>color.black</background>"
			"<fullscreen>false</fullscreen><framerate>60</framerate></window>"
			"<variables><variable name=\"beat\">0.25</variable></variables>"
			+ sounds +
			"<objects><object name=\"o\"><sprite><circle><radius>5</radius></circle></sprite>"
			"<position><x>0</x><y>0</y></position><velocity><x>0</x><y>0</y></velocity>"
			"<collisions><enabled>false</enabled></collisions></object></objects>"
			"<states><state name=\"playing\"><shows><show object=\"o\" /></shows>"
			"<inputs><input button=\"space\">" + onSpace + "</input></inputs></state></states>"
			"</game>";
	}

	// Loads the game, or rethrows what loading it threw.
	std::vector<SoundDesc> loadSounds(const std::string& xml)
	{
		ScratchFile scratch{ std::filesystem::temp_directory_path() / scratchName("xge_test_sound.xml") };
		{
			std::ofstream out(scratch.path);
			out << xml;
		}
		Game game{ scratch.path.string() };
		return game.getSounds();
	}

	// How many times the samples cross zero going up, a second: the pitch of a
	// plain wave.
	double risingCrossingsPerSecond(const std::vector<std::int16_t>& samples, std::size_t from, std::size_t to)
	{
		int crossings = 0;
		for (std::size_t i = from + 1; i < to; ++i)
		{
			if (samples[i - 1] < 0 && samples[i] >= 0) { ++crossings; }
		}
		return crossings * static_cast<double>(kSoundSampleRate) / static_cast<double>(to - from);
	}

	SoundDesc oneNote(Waveform wave, float startHz, float endHz, float seconds, float volume = 0.5f)
	{
		SoundDesc sound;
		sound.name = "test";
		sound.volume = volume;
		sound.notes.push_back(SoundNote{ false, wave, startHz, endHz, seconds });
		return sound;
	}

	// Pong, in play, with nothing moving but what the test moves.
	struct PongInPlay
	{
		Game game{ "descriptions/pong.xml" };
		Object& ball;
		Object& paddle1;

		PongInPlay() :
			ball(game.getObject("ball")),
			paddle1(game.getObject("paddle1"))
		{
			game.setCurrentState("playing");
			for (Object& object : game.getCurrentObjects())
			{
				object.size = measureShapeSize(object.picture);
			}

			ball.velocity = { 0, 0 };
		}

		// Every sound asked for over that many frames, in order.
		std::vector<std::string> run(int frames)
		{
			std::vector<std::string> asked;
			for (int i = 0; i < frames; ++i)
			{
				game.updateObjects();
				for (auto& name : game.takeSoundRequests()) { asked.push_back(name); }
			}
			return asked;
		}
	};
}

TEST_CASE("pitches: note names with octaves, sharps and flats, or hertz", "[sound]")
{
	CHECK(*pitchFromName("A4") == Approx(440.0f));
	CHECK(*pitchFromName("A5") == Approx(880.0f));
	CHECK(*pitchFromName("C4") == Approx(261.6256f).epsilon(0.0001));
	CHECK(*pitchFromName("C#4") == Approx(*pitchFromName("Db4")));
	CHECK(*pitchFromName("Eb3") == Approx(*pitchFromName("D#3")));
	CHECK(*pitchFromName("B3") == Approx(*pitchFromName("Cb4")));
	CHECK(*pitchFromName("C-1") == Approx(8.1758f).epsilon(0.001));
	CHECK(*pitchFromName("a4") == Approx(440.0f));
	CHECK(*pitchFromName("440") == Approx(440.0f));
	CHECK(*pitchFromName("97.5") == Approx(97.5f));

	CHECK_FALSE(pitchFromName(""));
	CHECK_FALSE(pitchFromName("H4"));
	CHECK_FALSE(pitchFromName("C"));
	CHECK_FALSE(pitchFromName("C#"));
	CHECK_FALSE(pitchFromName("C4x"));
	CHECK_FALSE(pitchFromName("0"));
	CHECK_FALSE(pitchFromName("440Hz"));
	CHECK_FALSE(pitchFromName("-5"));
}

TEST_CASE("waves are named the way a game file names them", "[sound]")
{
	for (const Waveform wave : { Waveform::Square, Waveform::Triangle, Waveform::Sawtooth, Waveform::Sine, Waveform::Noise })
	{
		CHECK(waveformFromName(waveformName(wave)) == wave);
	}
	CHECK_FALSE(waveformFromName("pulse"));
	CHECK_FALSE(waveformFromName("Square"));
}

TEST_CASE("a note lasts as long as it says, at its pitch and volume", "[sound]")
{
	const auto samples = synthesize(oneNote(Waveform::Square, 440.0f, 440.0f, 0.5f));

	REQUIRE(samples.size() == kSoundSampleRate / 2);
	CHECK(risingCrossingsPerSecond(samples, 0, samples.size()) == Approx(440.0).margin(4.0));

	// Square: full volume (0.5 of the loudest) in the middle, faded at both ends
	// so it neither starts nor stops with a click.
	const auto loudest = *std::max_element(samples.begin(), samples.end());
	CHECK(loudest == Approx(0.5 * 32767).margin(2));
	CHECK(samples.front() == 0);
	CHECK(std::abs(samples.back()) < 200);
}

TEST_CASE("every wave makes sound, and each is a different shape", "[sound]")
{
	std::vector<std::vector<std::int16_t>> made;
	for (const Waveform wave : { Waveform::Square, Waveform::Triangle, Waveform::Sawtooth, Waveform::Sine, Waveform::Noise })
	{
		made.push_back(synthesize(oneNote(wave, 220.0f, 220.0f, 0.1f)));
		const auto& samples = made.back();
		CHECK(*std::max_element(samples.begin(), samples.end()) > 10000);
		CHECK(*std::min_element(samples.begin(), samples.end()) < -10000);
	}

	for (std::size_t a = 0; a < made.size(); ++a)
	{
		for (std::size_t b = a + 1; b < made.size(); ++b)
		{
			CHECK(made[a] != made[b]);
		}
	}
}

TEST_CASE("a slide moves from one pitch to the other", "[sound]")
{
	const auto samples = synthesize(oneNote(Waveform::Square, 220.0f, 880.0f, 1.0f));
	const std::size_t tenth = kSoundSampleRate / 10;

	CHECK(risingCrossingsPerSecond(samples, 0, tenth) < 300.0);
	CHECK(risingCrossingsPerSecond(samples, samples.size() - tenth, samples.size()) > 750.0);
}

TEST_CASE("notes and rests play one after the other", "[sound]")
{
	SoundDesc sound;
	sound.volume = 1.0f;
	sound.notes.push_back(SoundNote{ false, Waveform::Square, 440.0f, 440.0f, 0.1f });
	sound.notes.push_back(SoundNote{ true, Waveform::Square, 0.0f, 0.0f, 0.2f });
	sound.notes.push_back(SoundNote{ false, Waveform::Triangle, 880.0f, 880.0f, 0.1f });

	const auto samples = synthesize(sound);
	const std::size_t tenth = kSoundSampleRate / 10;

	REQUIRE(samples.size() == 4 * tenth);
	CHECK(std::all_of(samples.begin() + tenth, samples.begin() + 3 * tenth, [](std::int16_t s) { return s == 0; }));
	CHECK(risingCrossingsPerSecond(samples, 0, tenth) == Approx(440.0).margin(20.0));
	CHECK(risingCrossingsPerSecond(samples, 3 * tenth, 4 * tenth) == Approx(880.0).margin(20.0));
}

TEST_CASE("noise is the same every time it is made", "[sound]")
{
	const SoundDesc hiss = oneNote(Waveform::Noise, 4000.0f, 4000.0f, 0.2f);
	CHECK(synthesize(hiss) == synthesize(hiss));
}

TEST_CASE("a game file's sounds: volume, wave, notes, rests and slides", "[sound][xml]")
{
	const auto sounds = loadSounds(gameWithSounds(
		"<sounds>"
		"<sound name=\"blip\"><note pitch=\"A4\">0.05</note></sound>"
		"<sound name=\"tune\" wave=\"triangle\"><volume>0.6</volume>"
		"<note pitch=\"C5\">beat</note><rest>beat / 2</rest><note pitch=\"C4\" to=\"C6\" wave=\"noise\">0.1</note></sound>"
		"</sounds>",
		"<play sound=\"blip\" />"));

	REQUIRE(sounds.size() == 2);

	CHECK(sounds[0].name == "blip");
	CHECK(sounds[0].volume == Approx(0.3f));
	REQUIRE(sounds[0].notes.size() == 1);
	CHECK(sounds[0].notes[0].wave == Waveform::Square);
	CHECK(sounds[0].notes[0].startHz == Approx(440.0f));
	CHECK(sounds[0].notes[0].endHz == Approx(440.0f));
	CHECK(sounds[0].notes[0].seconds == Approx(0.05f));

	CHECK(sounds[1].volume == Approx(0.6f));
	REQUIRE(sounds[1].notes.size() == 3);
	CHECK(sounds[1].notes[0].wave == Waveform::Triangle);
	CHECK(sounds[1].notes[0].seconds == Approx(0.25f));
	CHECK(sounds[1].notes[1].rest);
	CHECK(sounds[1].notes[1].seconds == Approx(0.125f));
	CHECK(sounds[1].notes[2].wave == Waveform::Noise);
	CHECK(sounds[1].notes[2].startHz == Approx(*pitchFromName("C4")));
	CHECK(sounds[1].notes[2].endHz == Approx(*pitchFromName("C6")));
}

TEST_CASE("a game needs no sounds", "[sound][xml]")
{
	CHECK(loadSounds(gameWithSounds("")).empty());
}

TEST_CASE("a sound the game cannot use is turned away when it loads, saying where", "[sound][xml][errors]")
{
	const auto fails = [](const std::string& sounds, const std::string& onSpace, const std::string& expected)
	{
		INFO(sounds << onSpace);
		CHECK_THROWS_WITH(loadSounds(gameWithSounds(sounds, onSpace)), ContainsSubstring(expected));
	};

	const std::string blip = "<sounds><sound name=\"blip\"><note pitch=\"A4\">0.05</note></sound></sounds>";

	fails(blip, "<play sound=\"bleep\" />", "<play sound=\"bleep\" /> names no sound of the game");
	fails(blip, "<play />", "<play> needs sound=");
	fails(blip, "<silence sound=\"bleep\" />", "<silence sound=\"bleep\" /> names no sound of the game");
	fails(blip, "<silence />", "<silence> needs sound=");
	fails("<sounds><sound name=\"s\"><volume>0.5</volume><loop /><note pitch=\"A4\">0.1</note></sound></sounds>", "<pop />", "<loop /> has to come first");
	fails("<sounds><sound name=\"s\" wave=\"organ\"><note pitch=\"A4\">0.1</note></sound></sounds>", "<pop />", "wave=\"organ\"");
	fails("<sounds><sound name=\"s\"><note pitch=\"A4\" wave=\"pulse\">0.1</note></sound></sounds>", "<pop />", "wave=\"pulse\"");
	fails("<sounds><sound name=\"s\"><note pitch=\"H2\">0.1</note></sound></sounds>", "<pop />", "pitch=\"H2\" is not a pitch");
	fails("<sounds><sound name=\"s\"><note pitch=\"A4\" to=\"up\">0.1</note></sound></sounds>", "<pop />", "to=\"up\" is not a pitch");
	fails("<sounds><sound name=\"s\"><note pitch=\"A4\">0</note></sound></sounds>", "<pop />", "sound 's' > <note> 1: lasts 0");
	fails("<sounds><sound name=\"s\"><note pitch=\"A4\">0.1</note><rest>60</rest></sound></sounds>", "<pop />", "<rest> 2: lasts 60");
	fails("<sounds><sound name=\"s\"><note>0.1</note></sound></sounds>", "<pop />", "<note> needs pitch=");
	fails("<sounds><sound name=\"s\"><volume>2</volume><note pitch=\"A4\">0.1</note></sound></sounds>", "<pop />", "<volume> is 2");
	fails("<sounds><sound name=\"s\"><note pitch=\"A4\">0.1</note><volume>1</volume></sound></sounds>", "<pop />", "<volume> has to come before");
	fails("<sounds><sound name=\"s\"></sound></sounds>", "<pop />", "has no <note>");
	fails("<sounds><sound name=\"s\"><beep /></sound></sounds>", "<pop />", "unknown <beep>");
	fails("<sounds><sound name=\"s\"><note pitch=\"A4\">0.1</note></sound><sound name=\"s\"><note pitch=\"A5\">0.1</note></sound></sounds>",
		"<pop />", "already a sound of that name");
}

TEST_CASE("Pong's sounds load with every XML library, checked against the schema", "[sound][pong]")
{
	for (const XmlBackend backend : { XmlBackend::Xerces, XmlBackend::TinyXml2, XmlBackend::PugiXml, XmlBackend::RapidXml })
	{
		Game game{ readGame("descriptions/pong.xml", backend) };

		std::vector<std::string> names;
		for (const auto& sound : game.getSounds()) { names.push_back(sound.name); }
		CHECK(names == std::vector<std::string>{ "paddle", "wall", "score" });
	}
}

TEST_CASE("Pong: the ball off the top wall asks for the wall sound, once", "[sound][pong]")
{
	PongInPlay pong;
	pong.ball.position = { 600, 4 };
	pong.ball.velocity = { 0, -3 };

	CHECK(pong.run(10) == std::vector<std::string>{ "wall" });
	CHECK(pong.ball.velocity.y > 0);
}

TEST_CASE("Pong: the ball off the bottom wall asks for the wall sound too", "[sound][pong]")
{
	PongInPlay pong;
	pong.ball.position = { 600, 720 - 16 - 2 };
	pong.ball.velocity = { 0, 3 };

	CHECK(pong.run(10) == std::vector<std::string>{ "wall" });
	CHECK(pong.ball.velocity.y < 0);
}

TEST_CASE("Pong: the ball off a paddle asks for the paddle sound", "[sound][pong]")
{
	PongInPlay pong;
	pong.ball.position = { pong.paddle1.position.x + 20 + 10, pong.paddle1.position.y + 50 };
	pong.ball.velocity = { -4, 0 };

	CHECK(pong.run(10) == std::vector<std::string>{ "paddle" });
	CHECK(pong.ball.velocity.x > 0);
}

TEST_CASE("Pong: a point scored asks for the score sound", "[sound][pong]")
{
	PongInPlay pong;
	pong.ball.position = { 600, 300 };
	pong.ball.velocity = { 7, 0 };

	// Clear of both paddles, out through the right: a point to player one.
	pong.ball.position.y = 30;
	const auto asked = pong.run(120);

	CHECK(asked == std::vector<std::string>{ "score" });
	CHECK(pong.game.getObject("paddle1").variable.at("score") == 1);
}

TEST_CASE("a sound asked for twice in a frame is played once", "[sound]")
{
	Game game{ "descriptions/pong.xml" };
	game.requestSound("wall");
	game.requestSound("paddle");
	game.requestSound("wall");

	CHECK(game.takeSoundRequests() == std::vector<std::string>{ "wall", "paddle" });
	CHECK(game.takeSoundRequests().empty());
}

TEST_CASE("Engine loads the game's sounds into its Audio and plays what each frame asks for", "[sound][engine]")
{
	Game game{ "descriptions/pong.xml" };
	AudioLog log;
	Engine engine(game, std::make_unique<FakeWindow>(), std::make_unique<RecordingAudio>(log));

	CHECK(log.loaded == std::vector<std::string>{ "paddle", "wall", "score" });

	// Space on the main menu starts the game; the ball off the top wall asks for its blip.
	engine.handleKeyPressed(KeyCode::Space);
	engine.handleKeyReleased(KeyCode::Space);
	engine.step();
	Object& ball = game.getObject("ball");
	ball.position = { 600, -1 };
	ball.velocity = { 0, -3 };
	CHECK(log.played.empty()); // asked for, played on the frame
	engine.step();
	CHECK(log.played == std::vector<std::string>{ "wall" });

	engine.silence();
	CHECK(log.stops == 1);
}

TEST_CASE("Engine without an Audio is silent, and replaceAudio hands the sounds over", "[sound][engine]")
{
	Game game{ "descriptions/pong.xml" };
	Engine engine(game, std::make_unique<FakeWindow>());

	REQUIRE(engine.currentAudio() != nullptr);
	CHECK(dynamic_cast<NullAudio*>(engine.currentAudio()) != nullptr);

	AudioLog log;
	engine.replaceAudio([&] { return std::make_unique<RecordingAudio>(log); });
	CHECK(log.loaded.size() == 3);

	game.requestSound("score");
	engine.step();
	CHECK(log.played == std::vector<std::string>{ "score" });

	// A library that will not start leaves the game silent, and says why.
	CHECK_THROWS_WITH(engine.replaceAudio([]() -> std::unique_ptr<Audio> { throw std::runtime_error("no sound card"); }),
		ContainsSubstring("no sound card"));
	CHECK(dynamic_cast<NullAudio*>(engine.currentAudio()) != nullptr);
	engine.step();
}

TEST_CASE("the silent backend needs no sound device", "[sound][engine]")
{
	auto audio = AudioFactory::create(AudioBackend::None);
	audio->load(Game{ "descriptions/pong.xml" }.getSounds());
	audio->play("wall");
	audio->stopAll();
	CHECK(dynamic_cast<NullAudio*>(audio.get()) != nullptr);
}

TEST_CASE("Breakout, Frogger, Space Race, Freeway, Depth Charge, Astrosmash, Lunar Lander and Asteroids have their sounds, and each is played somewhere", "[sound][games]")
{
	const std::vector<std::pair<std::string, std::vector<std::string>>> games = {
		{ "breakout", { "wall", "paddle", "red", "orange", "green", "yellow", "lost" } },
		{ "frogger", { "hop", "squash", "splash", "home", "start", "win", "gameover" } },
		{ "spacerace", { "score", "crash", "end" } },
		{ "freeway", { "hop", "hit", "cross", "start", "end" } },
		{ "depthcharge", { "sonar", "drop", "sink", "boom", "start", "gameover" } },
		{ "astrosmash", { "pop", "thud", "boom", "shoot", "start", "spare", "gameover" } },
		{ "lunarlander", { "thrust", "land", "crash", "start" } },
		{ "asteroids", { "beat", "thrust", "shoot", "bangbig", "bangmedium", "bangsmall", "boom", "saucer", "extra", "gameover" } },
	};

	for (const auto& [name, expected] : games)
	{
		INFO(name);
		const std::string path = "descriptions/" + name + ".xml";

		for (const XmlBackend backend : { XmlBackend::Xerces, XmlBackend::TinyXml2, XmlBackend::PugiXml, XmlBackend::RapidXml })
		{
			Game game{ readGame(path, backend) };
			std::vector<std::string> names;
			for (const auto& sound : game.getSounds()) { names.push_back(sound.name); }
			CHECK(names == expected);
		}

		// A sound nothing plays is a sound that was forgotten.
		std::ifstream file(path);
		const std::string text = fileText(file);
		for (const std::string& sound : expected)
		{
			INFO(sound);
			CHECK((text.find("<play sound=\"" + sound + "\" />") != std::string::npos || text.find("<music sound=\"" + sound + "\" />") != std::string::npos));
		}
	}
}

TEST_CASE("a sound that loops is read from the file, and a silence beside it", "[sound][xml]")
{
	const std::vector<SoundDesc> sounds = loadSounds(gameWithSounds(
		"<sounds><sound name=\"hum\" wave=\"noise\"><loop /><volume>0.2</volume><note pitch=\"150\">1</note></sound>"
		"<sound name=\"blip\"><note pitch=\"A4\">0.05</note></sound></sounds>",
		"<silence sound=\"hum\" />"));

	REQUIRE(sounds.size() == 2);
	CHECK(sounds[0].loop);
	CHECK(sounds[0].volume == Approx(0.2f));
	CHECK_FALSE(sounds[1].loop);
}

TEST_CASE("a sound that loops stays on until it is silenced, and is asked for once", "[sound]")
{
	Game game{ "descriptions/lunarlander.xml" };

	CHECK(game.soundLoops("thrust"));
	CHECK_FALSE(game.soundLoops("land"));
	CHECK_FALSE(game.soundLoops("nothing"));

	game.requestSound("thrust");
	game.requestSound("thrust");
	game.requestSound("land");
	CHECK(game.loopingSounds() == std::set<std::string>{ "thrust" });
	CHECK(game.takeSoundRequests() == std::vector<std::string>{ "land" });
	CHECK(game.loopingSounds() == std::set<std::string>{ "thrust" }); // still on next frame

	game.silenceSound("thrust");
	CHECK(game.loopingSounds().empty());
	CHECK(game.takeSilenceRequests() == std::vector<std::string>{ "thrust" });
	CHECK(game.takeSilenceRequests().empty());

	// A new game starts quiet.
	game.requestSound("thrust");
	game.resetAll();
	CHECK(game.loopingSounds().empty());
}

TEST_CASE("Lunar Lander: the engine hums while thrust is held, and stops when let go, paused or out of fuel", "[sound][engine][lunarlander]")
{
	Game game{ "descriptions/lunarlander.xml" };
	AudioLog log;
	Engine engine(game, std::make_unique<FakeWindow>(), std::make_unique<RecordingAudio>(log));

	engine.handleKeyPressed(KeyCode::Space);
	engine.handleKeyReleased(KeyCode::Space);
	engine.step();
	REQUIRE(game.getCurrentState().name == "playing");

	// Held: started once, and left playing frame after frame.
	engine.handleKeyPressed(KeyCode::Up);
	engine.step();
	engine.step();
	engine.step();
	CHECK(log.looped == std::vector<std::string>{ "thrust" });
	CHECK(log.stopped.empty());

	// Up and W both thrust: letting go of one while the other is down keeps it going.
	engine.handleKeyPressed(KeyCode::W);
	engine.handleKeyReleased(KeyCode::Up);
	engine.step();
	CHECK(log.stopped.empty());
	engine.handleKeyReleased(KeyCode::W);
	engine.step();
	CHECK(log.stopped == std::vector<std::string>{ "thrust" });

	// Paused, a held key drives nothing, so it is quiet; back in play it hums again.
	log.looped.clear();
	log.stopped.clear();
	engine.handleKeyPressed(KeyCode::Up);
	engine.step();
	CHECK(log.looped == std::vector<std::string>{ "thrust" });
	log.looped.clear();
	engine.handleKeyPressed(KeyCode::P);
	engine.handleKeyReleased(KeyCode::P);
	engine.step();
	REQUIRE(game.getCurrentState().name == "paused");
	CHECK(game.loopingSounds().empty());
	CHECK(log.stopped == std::vector<std::string>{ "thrust" });
	engine.handleKeyPressed(KeyCode::P);
	engine.handleKeyReleased(KeyCode::P);
	engine.step();
	REQUIRE(game.getCurrentState().name == "playing");
	CHECK(log.looped == std::vector<std::string>{ "thrust" });

	// Out of fuel, nothing pushes, and nothing hums.
	log.stopped.clear();
	game.getObject("lander").variable["fuel"] = 0.0f;
	engine.step();
	CHECK(game.loopingSounds().empty());
	CHECK(log.stopped == std::vector<std::string>{ "thrust" });

	// A front end pausing silences everything; the next frame plays again what is still on.
	log.looped.clear();
	game.getObject("lander").variable["fuel"] = 100.0f;
	game.requestSound("thrust");
	engine.step();
	CHECK(log.looped == std::vector<std::string>{ "thrust" });
	engine.silence();
	engine.step();
	CHECK(log.looped == std::vector<std::string>{ "thrust", "thrust" });
}

TEST_CASE("Asteroids: the heartbeat plays in play, and the ship's engine hums while thrust is held", "[sound][engine][asteroids]")
{
	Game game{ "descriptions/asteroids.xml" };
	AudioLog log;
	Engine engine(game, std::make_unique<FakeWindow>(), std::make_unique<RecordingAudio>(log));

	engine.handleKeyPressed(KeyCode::Space);
	engine.handleKeyReleased(KeyCode::Space);
	engine.step();
	REQUIRE(game.getCurrentState().name == "playing");
	CHECK(log.looped == std::vector<std::string>{ "beat" });
	log.looped.clear();

	engine.handleKeyPressed(KeyCode::Up);
	engine.step();
	CHECK(log.looped == std::vector<std::string>{ "thrust" });
	engine.handleKeyReleased(KeyCode::Up);
	engine.step();
	CHECK(log.stopped == std::vector<std::string>{ "thrust" });
}

TEST_CASE("a sound's parts are read in order, each note with its part's wave unless it gives its own", "[sound][xml][music]")
{
	const std::vector<SoundDesc> sounds = loadSounds(gameWithSounds(
		"<sounds><sound name=\"tune\" wave=\"sine\">"
		"<part wave=\"square\"><note pitch=\"C5\">0.1</note><note pitch=\"E5\" wave=\"sawtooth\">0.1</note></part>"
		"<part><rest>0.05</rest><note pitch=\"C3\">beat</note></part>"
		"</sound></sounds>"));

	REQUIRE(sounds.size() == 1);
	const std::vector<SoundNote>& notes = sounds[0].notes;
	REQUIRE(notes.size() == 4);
	CHECK(notes[0].part == 0);
	CHECK(notes[0].wave == Waveform::Square);
	CHECK(notes[1].wave == Waveform::Sawtooth);
	CHECK(notes[2].part == 1);
	CHECK(notes[2].rest);
	CHECK(notes[3].part == 1);
	CHECK(notes[3].wave == Waveform::Sine); // the part gives none: the sound's
	CHECK(notes[3].seconds == Approx(0.25f));
}

TEST_CASE("a sound's parts play at the same time, added together, as long as the longest", "[sound][music]")
{
	SoundDesc tune;
	tune.volume = 0.4f;
	tune.notes = { SoundNote{ false, Waveform::Square, 440.0f, 440.0f, 0.1f, 0 },
		SoundNote{ false, Waveform::Triangle, 110.0f, 110.0f, 0.2f, 1 } };

	SoundDesc melody = tune;
	melody.notes = { tune.notes[0] };
	SoundDesc bass = tune;
	bass.notes = { tune.notes[1] };
	bass.notes[0].part = 0;

	const std::vector<std::int16_t> both = synthesize(tune);
	const std::vector<std::int16_t> high = synthesize(melody);
	const std::vector<std::int16_t> low = synthesize(bass);
	REQUIRE(both.size() == low.size()); // the longer part
	REQUIRE(high.size() < low.size());

	bool same = true;
	for (std::size_t i = 0; i < both.size(); ++i)
	{
		const int sum = (i < high.size() ? high[i] : 0) + low[i];
		if (std::abs(both[i] - sum) > 1) { same = false; }
	}
	CHECK(same);

	// Loud parts are cut off at the loudest a sample can be, not wrapped.
	tune.volume = 1.0f;
	tune.notes[1].wave = Waveform::Square;
	tune.notes[1].startHz = tune.notes[1].endHz = 440.0f;
	const std::vector<std::int16_t> loud = synthesize(tune);
	CHECK(*std::max_element(loud.begin(), loud.end()) == 32767);
	CHECK(*std::min_element(loud.begin(), loud.end()) == -32767);
}

TEST_CASE("parts the game cannot use are turned away when it loads, saying where", "[sound][xml][music][errors]")
{
	const auto fails = [](const std::string& sounds, const std::string& expected)
	{
		INFO(sounds);
		CHECK_THROWS_WITH(loadSounds(gameWithSounds(sounds)), ContainsSubstring(expected));
	};
	fails("<sounds><sound name=\"s\"><note pitch=\"A4\">0.1</note><part><note pitch=\"A4\">0.1</note></part></sound></sounds>", "cannot have <part>s as well");
	fails("<sounds><sound name=\"s\"><part><note pitch=\"A4\">0.1</note></part><note pitch=\"A4\">0.1</note></sound></sounds>", "go inside them");
	fails("<sounds><sound name=\"s\"><part></part></sound></sounds>", "sound 's' > <part> 1: has no <note>");
	fails("<sounds><sound name=\"s\"><part><note pitch=\"A4\">0.1</note></part><part><loop /></part></sound></sounds>", "<part> 2: unknown <loop>");
	fails("<sounds><sound name=\"s\"><part wave=\"organ\"><note pitch=\"A4\">0.1</note></part></sound></sounds>", "wave=\"organ\"");
	fails("<sounds><sound name=\"s\"><part><note pitch=\"A4\">0.1</note></part><volume>1</volume></sound></sounds>", "<volume> has to come before");
}

TEST_CASE("a state's music names a sound of the game", "[sound][xml][music][errors]")
{
	const std::string blip = "<sounds><sound name=\"blip\"><note pitch=\"A4\">0.05</note></sound></sounds>";
	std::string game = gameWithSounds(blip);
	game.replace(game.find("<inputs>"), 0, "<music sound=\"blip\" />");
	CHECK(loadSounds(game).size() == 1);
	game.replace(game.find("blip\" />"), 4, "bleep");
	CHECK_THROWS_WITH(loadSounds(game), ContainsSubstring("state 'playing': <music sound=\"bleep\" /> names no sound of the game"));
}

TEST_CASE("Pac-Man's music: the opening tune plays over READY!, the siren loops while play is current, and the loss plays once", "[sound][engine][music][pacman]")
{
	for (const XmlBackend backend : { XmlBackend::Xerces, XmlBackend::TinyXml2, XmlBackend::PugiXml, XmlBackend::RapidXml })
	{
		CHECK_NOTHROW(Game{ readGame("descriptions/pacman.xml", backend) });
	}

	Game game{ "descriptions/pacman.xml" };
	AudioLog log;
	Engine engine(game, std::make_unique<FakeWindow>(), std::make_unique<RecordingAudio>(log));

	engine.step();
	CHECK(log.looped.empty()); // the title is quiet

	// Space: play, with READY! over it and the opening tune; no siren yet.
	engine.handleKeyPressed(KeyCode::Space);
	engine.handleKeyReleased(KeyCode::Space);
	engine.step();
	REQUIRE(game.getCurrentState().name == "ready");
	CHECK(log.played == std::vector<std::string>{ "start" });
	CHECK(log.looped.empty());

	// READY! goes, and the siren starts, once.
	for (int i = 0; i < 300 && game.getCurrentState().name == "ready"; ++i) { engine.step(); }
	REQUIRE(game.getCurrentState().name == "playing");
	CHECK(log.looped == std::vector<std::string>{ "siren" });
	engine.step();
	CHECK(log.looped.size() == 1);

	// Paused: quiet; back: it starts again.
	engine.handleKeyPressed(KeyCode::P);
	engine.handleKeyReleased(KeyCode::P);
	engine.step();
	REQUIRE(game.getCurrentState().name == "paused");
	CHECK(log.stopped == std::vector<std::string>{ "siren" });
	engine.handleKeyPressed(KeyCode::P);
	engine.handleKeyReleased(KeyCode::P);
	engine.step();
	REQUIRE(game.getCurrentState().name == "playing");
	CHECK(log.looped == std::vector<std::string>{ "siren", "siren" });
	CHECK(log.played.size() == 1); // coming back to play is not beginning it

	// The last life lost: the siren stops and the loss plays, once.
	game.getObject("status").variable["lives"] = 0.0f;
	engine.step();
	REQUIRE(game.getCurrentState().name == "gameover");
	CHECK(log.stopped == std::vector<std::string>{ "siren", "siren" });
	CHECK(log.played == std::vector<std::string>{ "start", "lose" });
	engine.step();
	CHECK(log.played.size() == 2);
}
