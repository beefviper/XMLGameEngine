// test_sound.cpp
// XML Game Engine
// author: beefviper
// date: Oct 3, 2026
//
// Catch2 tests for the sound system: pitch names, the synthesizer
// (audio/sound.h), reading <sounds> and <play> from a game file, the checks made
// when it loads, Pong asking for its sounds when the ball bounces and
// scores, the eight games that were silent now having sounds, and Engine
// handing what was asked for to its Audio (audio/audio.h), and a WAV file
// read into the same samples (audio/wave.h). No test opens a sound device:
// Engine is given a recording Audio instead.

#include "scratch.h"
#include "file_text.h"
#include "engine/engine.h"
#include "game/game.h"
#include "load/xml.h"
#include "audio/sound.h"
#include "audio/wave.h"
#include "window/null.h"

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_string.hpp>

#include <algorithm>
#include <cmath>
#include <cstdint>
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
			"<framerate>60</framerate></window>"
			"<variables><variable name=\"beat\">0.25</variable></variables>"
			"<states><objects><object name=\"o\"><sprite><circle><radius>5</radius></circle></sprite>"
			"<position><x>0</x><y>0</y></position><velocity><x>0</x><y>0</y></velocity>"
			"<collisions><off /></collisions></object></objects>"
			"<state name=\"playing\"><shows><show object=\"o\" /></shows>"
			"<inputs><input button=\"space\">" + onSpace + "</input></inputs></state></states>"
			+ sounds + "</game>";
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
		"<note pitch=\"C5\">beat</note><rest><divide><value>beat</value><value>2</value></divide></rest><note pitch=\"C4\" to=\"C6\" wave=\"noise\">0.1</note></sound>"
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
	Engine engine(game, std::make_unique<NullWindow>(), std::make_unique<RecordingAudio>(log));

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
	Engine engine(game, std::make_unique<NullWindow>());

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

namespace
{
	// The game's sounds, the same through every XML library, and each one
	// played somewhere: a sound nothing plays is a sound that was forgotten.
	void hasItsSounds(const std::string& name, const std::vector<std::string>& expected)
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

		std::ifstream file(path);
		const std::string text = fileText(file);
		for (const std::string& sound : expected)
		{
			INFO(sound);
			CHECK((text.find("<play sound=\"" + sound + "\" />") != std::string::npos || text.find("<music sound=\"" + sound + "\" />") != std::string::npos));
		}
	}
}

TEST_CASE("Breakout and Frogger have their sounds, and each is played somewhere", "[sound][games]")
{
	hasItsSounds("breakout", { "wall", "paddle", "red", "orange", "green", "yellow", "lost" });
	hasItsSounds("frogger", { "hop", "squash", "splash", "home", "start", "win", "gameover" });
}

TEST_CASE("Space Race and Freeway have their sounds, and each is played somewhere", "[sound][games]")
{
	hasItsSounds("spacerace", { "score", "crash", "end" });
	hasItsSounds("freeway", { "hop", "hit", "cross", "start", "end" });
}

TEST_CASE("Depth Charge and Astrosmash have their sounds, and each is played somewhere", "[sound][games]")
{
	hasItsSounds("depthcharge", { "sonar", "drop", "sink", "boom", "start", "gameover" });
	hasItsSounds("astrosmash", { "pop", "thud", "boom", "shoot", "start", "spare", "gameover" });
}

TEST_CASE("Lunar Lander and Asteroids have their sounds, and each is played somewhere", "[sound][games]")
{
	hasItsSounds("lunarlander", { "thrust", "land", "crash", "start" });
	hasItsSounds("asteroids", { "beat", "thrust", "shoot", "bangbig", "bangmedium", "bangsmall", "boom", "saucer", "extra", "gameover" });
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
	Engine engine(game, std::make_unique<NullWindow>(), std::make_unique<RecordingAudio>(log));

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
	engine.handleKeyPressed(KeyCode::Pause);
	engine.handleKeyReleased(KeyCode::Pause);
	engine.step();
	REQUIRE(game.getCurrentState().name == "paused");
	CHECK(game.loopingSounds().empty());
	CHECK(log.stopped == std::vector<std::string>{ "thrust" });
	engine.handleKeyPressed(KeyCode::Pause);
	engine.handleKeyReleased(KeyCode::Pause);
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
	Engine engine(game, std::make_unique<NullWindow>(), std::make_unique<RecordingAudio>(log));

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
	Game game{ "descriptions/pacman.xml" };
	AudioLog log;
	Engine engine(game, std::make_unique<NullWindow>(), std::make_unique<RecordingAudio>(log));

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
	engine.handleKeyPressed(KeyCode::Pause);
	engine.handleKeyReleased(KeyCode::Pause);
	engine.step();
	REQUIRE(game.getCurrentState().name == "paused");
	CHECK(log.stopped == std::vector<std::string>{ "siren" });
	engine.handleKeyPressed(KeyCode::Pause);
	engine.handleKeyReleased(KeyCode::Pause);
	engine.step();
	REQUIRE(game.getCurrentState().name == "playing");
	CHECK(log.looped == std::vector<std::string>{ "siren", "siren" });
	CHECK(log.played.size() == 1); // coming back to play is not beginning it

	// The last life lost: the siren stops and the loss plays, once.
	game.getObject("status").variable["lives"] = 0.0f;
	engine.step();
	REQUIRE(game.getCurrentState().name == "over");
	CHECK(log.stopped == std::vector<std::string>{ "siren", "siren" });
	CHECK(log.played == std::vector<std::string>{ "start", "lose" });
	engine.step();
	CHECK(log.played.size() == 2);
}

namespace
{
	// A WAV file's bytes, written by hand: a header, a format chunk, then the
	// samples as given (already laid out frame by frame, channel by channel).
	struct WaveFile
	{
		std::uint16_t format{ 1 };
		std::uint16_t channels{ 1 };
		std::uint32_t rate{ kSoundSampleRate };
		std::uint16_t bits{ 16 };
		std::vector<int> samples;
		bool extra{ false }; // a "LIST" chunk of odd size before the data, to be passed over

		void write(const std::filesystem::path& path) const
		{
			std::vector<unsigned char> data;
			for (const int sample : samples)
			{
				if (bits == 8) { data.push_back(static_cast<unsigned char>(sample)); }
				else
				{
					const auto value = static_cast<std::uint16_t>(static_cast<std::int16_t>(sample));
					data.push_back(static_cast<unsigned char>(value & 0xFF));
					data.push_back(static_cast<unsigned char>(value >> 8));
				}
			}

			std::vector<unsigned char> out;
			const auto tag = [&](const char* text) { out.insert(out.end(), text, text + 4); };
			const auto number = [&](std::uint32_t value, int count)
			{
				for (int i = 0; i < count; ++i) { out.push_back(static_cast<unsigned char>((value >> (8 * i)) & 0xFF)); }
			};
			tag("RIFF");
			number(0, 4); // the whole size, filled in below
			tag("WAVE");
			tag("fmt ");
			number(16, 4);
			number(format, 2);
			number(channels, 2);
			number(rate, 4);
			number(rate * channels * bits / 8, 4);
			number(static_cast<std::uint32_t>(channels * bits / 8), 2);
			number(bits, 2);
			if (extra)
			{
				tag("LIST");
				number(3, 4);
				out.insert(out.end(), { 'a', 'b', 'c', 0 }); // three bytes and a pad
			}
			tag("data");
			number(static_cast<std::uint32_t>(data.size()), 4);
			out.insert(out.end(), data.begin(), data.end());
			const auto size = static_cast<std::uint32_t>(out.size() - 8);
			for (int i = 0; i < 4; ++i) { out[4 + static_cast<std::size_t>(i)] = static_cast<unsigned char>((size >> (8 * i)) & 0xFF); }

			std::ofstream file(path, std::ios::binary);
			file.write(reinterpret_cast<const char*>(out.data()), static_cast<std::streamsize>(out.size()));
		}
	};

	std::vector<std::int16_t> readBack(const WaveFile& wave)
	{
		const ScratchFile file{ scratchName("descriptions/wave.wav") };
		wave.write(file.path);
		return readWave(file.path);
	}

	std::string refusal(const WaveFile& wave)
	{
		const ScratchFile file{ scratchName("descriptions/wave.wav") };
		wave.write(file.path);
		try
		{
			(void)readWave(file.path);
		}
		catch (const std::runtime_error& error)
		{
			return error.what();
		}
		return "";
	}
}

TEST_CASE("a 16-bit one-channel WAV at the engine's rate reads back as its own samples", "[sound][wave]")
{
	WaveFile wave;
	wave.samples = { 0, 1000, -1000, 32767, -32768, 7 };
	wave.extra = true;
	CHECK(readBack(wave) == std::vector<std::int16_t>{ 0, 1000, -1000, 32767, -32768, 7 });
}

TEST_CASE("an 8-bit WAV's samples are unsigned, 128 the middle", "[sound][wave]")
{
	WaveFile wave;
	wave.bits = 8;
	wave.samples = { 128, 255, 0, 192 };
	CHECK(readBack(wave) == std::vector<std::int16_t>{ 0, 127 * 256, -32768, 64 * 256 });
}

TEST_CASE("a two-channel WAV is averaged into one", "[sound][wave]")
{
	WaveFile wave;
	wave.channels = 2;
	wave.samples = { 1000, 3000, -500, 500, 32767, 32767 };
	CHECK(readBack(wave) == std::vector<std::int16_t>{ 2000, 0, 32767 });
}

TEST_CASE("a WAV at another rate is resampled along straight lines to the engine's", "[sound][wave]")
{
	// Half the rate: twice as many samples, each new one half way between.
	WaveFile wave;
	wave.rate = kSoundSampleRate / 2;
	wave.samples = { 0, 1000, 3000 };
	const std::vector<std::int16_t> doubled = readBack(wave);
	REQUIRE(doubled.size() == 6);
	CHECK(doubled[0] == 0);
	CHECK(doubled[1] == 500);
	CHECK(doubled[2] == 1000);
	CHECK(doubled[3] == 2000);
	CHECK(doubled[4] == 3000);

	// A common other rate keeps the sound's length.
	WaveFile slow;
	slow.rate = 22050;
	slow.samples.assign(22050, 100);
	const std::vector<std::int16_t> second = readBack(slow);
	CHECK(second.size() == kSoundSampleRate);
	CHECK(std::all_of(second.begin(), second.end(), [](std::int16_t sample) { return sample == 100; }));
}

TEST_CASE("a WAV file that cannot be read is refused, naming the file and why", "[sound][wave]")
{
	SECTION("missing")
	{
		const std::string missing = scratchName("descriptions/missing.wav");
		CHECK_THROWS_WITH(readWave(missing), ContainsSubstring("missing_scratch") && ContainsSubstring("cannot be opened"));
	}
	SECTION("not a WAV")
	{
		const ScratchFile file{ scratchName("descriptions/notwave.wav") };
		std::ofstream(file.path) << "<game />";
		CHECK_THROWS_WITH(readWave(file.path), ContainsSubstring("notwave_scratch") && ContainsSubstring("is not a WAV file"));
	}
	SECTION("compressed")
	{
		WaveFile wave;
		wave.format = 2; // ADPCM
		wave.samples = { 0, 0 };
		CHECK_THAT(refusal(wave), ContainsSubstring("wave_scratch") && ContainsSubstring("is compressed"));
	}
	SECTION("another size of sample")
	{
		WaveFile wave;
		wave.bits = 24;
		CHECK_THAT(refusal(wave), ContainsSubstring("24-bit samples"));
	}
	SECTION("more channels")
	{
		WaveFile wave;
		wave.channels = 6;
		CHECK_THAT(refusal(wave), ContainsSubstring("6 channels"));
	}
	SECTION("too long")
	{
		WaveFile wave;
		wave.rate = 8000;
		wave.bits = 8;
		wave.samples.assign(static_cast<std::size_t>(8000 * (kMaxSoundFileSeconds + 1)), 128);
		CHECK_THAT(refusal(wave), ContainsSubstring("longer than 60 seconds"));
	}
}

TEST_CASE("a sound with a <path> plays its recording, as loud as made unless given a volume", "[sound][wave]")
{
	const ScratchFile recording{ scratchName("descriptions/roar.wav") };
	WaveFile wave;
	wave.samples = { 0, 10000, -10000, 20000 };
	wave.write(recording.path);
	const std::string path = recording.path.generic_string();

	const std::vector<SoundDesc> sounds = loadSounds(gameWithSounds(
		"<sounds><sound name=\"roar\"><path>" + path + "</path></sound>"
		"<sound name=\"hum\"><loop /><volume>0.5</volume><path>" + path + "</path></sound></sounds>",
		"<play sound=\"roar\" />"));
	REQUIRE(sounds.size() == 2);

	CHECK(sounds[0].path == path);
	CHECK(sounds[0].notes.empty());
	CHECK(sounds[0].volume == 1.0f);
	CHECK(samplesOf(sounds[0]) == std::vector<std::int16_t>{ 0, 10000, -10000, 20000 });

	CHECK(sounds[1].loop);
	CHECK(sounds[1].volume == 0.5f);
	CHECK(samplesOf(sounds[1]) == std::vector<std::int16_t>{ 0, 5000, -5000, 10000 });
}

TEST_CASE("a recording that is missing or unreadable, or a <path> with notes, stops the load", "[sound][wave]")
{
	const auto refused = [](const std::string& sound)
	{
		try
		{
			(void)loadSounds(gameWithSounds("<sounds>" + sound + "</sounds>"));
		}
		catch (const std::exception& error)
		{
			return std::string(error.what());
		}
		return std::string();
	};

	const std::string missing = scratchName("descriptions/gone.wav");
	CHECK_THAT(refused("<sound name=\"roar\"><path>" + missing + "</path></sound>"),
		ContainsSubstring("sound 'roar'") && ContainsSubstring("gone_scratch") && ContainsSubstring("cannot be opened"));

	const ScratchFile text{ scratchName("descriptions/text.wav") };
	std::ofstream(text.path) << "not a sound";
	CHECK_THAT(refused("<sound name=\"roar\"><path>" + text.path.generic_string() + "</path></sound>"),
		ContainsSubstring("sound 'roar'") && ContainsSubstring("is not a WAV file"));

	CHECK_THAT(refused("<sound name=\"roar\"><note pitch=\"C4\">0.1</note><path>a.wav</path></sound>"), ContainsSubstring("cannot have a <path> as well"));
	CHECK_THAT(refused("<sound name=\"roar\"><path>a.wav</path><note pitch=\"C4\">0.1</note></sound>"), ContainsSubstring("cannot have <note>s as well"));
	CHECK_THAT(refused("<sound name=\"roar\" wave=\"sine\"><path>a.wav</path></sound>"), ContainsSubstring("has no wave="));
}
