// cli.cpp
// XML Game Engine
// author: beefviper
// date: Sept 28, 2026

#include "cli.h"
#include "engine/engine.h"
#include "engine/folder.h"
#include "game/game.h"
#include "game/random.h"
#include "load/xml.h"

#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <exception>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <memory>
#include <optional>
#include <string_view>

namespace xge
{
	namespace
	{
		struct WindowName
		{
			std::string_view name;
			WindowBackend backend;
		};

		struct XmlName
		{
			std::string_view name;
			XmlBackend backend;
		};

		struct AudioName
		{
			std::string_view name;
			AudioBackend backend;
		};

		// The names accepted on the command line, lower case. The first one
		// in each table is the default.
		constexpr WindowName windowNames[] = {
			{ "sfml3",  WindowBackend::SFML3 },
			{ "raylib", WindowBackend::Raylib },
			{ "sdl2",   WindowBackend::SDL2 },
			{ "opengl", WindowBackend::OpenGL },
			{ "vulkan", WindowBackend::Vulkan },
			{ "direct3d9", WindowBackend::Direct3D9 },
			{ "direct3d11", WindowBackend::Direct3D11 },
			{ "direct3d12", WindowBackend::Direct3D12 },
			{ "none",   WindowBackend::None },
		};

		constexpr XmlName xmlNames[] = {
			{ "xerces",   XmlBackend::Xerces },
			{ "tinyxml2", XmlBackend::TinyXml2 },
			{ "pugixml",  XmlBackend::PugiXml },
			{ "rapidxml", XmlBackend::RapidXml },
		};

		constexpr AudioName audioNames[] = {
			{ "sfml3",  AudioBackend::SFML3 },
			{ "raylib", AudioBackend::Raylib },
			{ "sdl2",   AudioBackend::SDL2 },
			{ "none",   AudioBackend::None },
		};

		// What --generate can write a game out as: a platform and a language,
		// each a folder of stylesheets in games/generators/.
		constexpr std::string_view generateNames[] = {
			"engine-cpp",
			"windows-cpp",
		};

		// A whole number from the command line: digits only, and at least
		// `least`.
		unsigned long wholeNumber(const std::string& value, const std::string& option, unsigned long least)
		{
			const bool digits = !value.empty() && value.size() <= 9 && value.find_first_not_of("0123456789") == std::string::npos;
			if (!digits || std::stoul(value) < least)
			{
				throw CliError(option + " takes a whole number" + (least > 0 ? " of at least " + std::to_string(least) : std::string()) + ", not '" + value + "'");
			}
			return std::stoul(value);
		}

		std::string lowerCase(std::string text)
		{
			std::transform(text.begin(), text.end(), text.begin(),
				[](unsigned char c) { return static_cast<char>(std::tolower(c)); });
			return text;
		}

		// Whether this program was built with the library (the factories know).
		bool isBuilt(WindowBackend backend) { return WindowFactory::available(backend); }
		bool isBuilt(XmlBackend backend) { return XmlDocumentFactory::available(backend); }
		bool isBuilt(AudioBackend backend) { return AudioFactory::available(backend); }

		// The names of the libraries this program was built with, in the table's order.
		template <typename Table>
		std::string namesOf(const Table& table)
		{
			std::string names;
			for (const auto& entry : table)
			{
				if (!isBuilt(entry.backend))
				{
					continue;
				}
				if (!names.empty())
				{
					names += ", ";
				}
				names += entry.name;
			}
			return names;
		}

		template <typename Table>
		auto lookUp(const Table& table, const std::string& what, const std::string& value)
		{
			const std::string wanted = lowerCase(value);
			for (const auto& entry : table)
			{
				if (entry.name == wanted)
				{
					if (!isBuilt(entry.backend))
					{
						throw CliError("the " + std::string(entry.name) + " " + what + " is not built into this program (built with: " + namesOf(table) + ")");
					}
					return entry.backend;
				}
			}

			throw CliError("unknown " + what + " '" + value + "' (choose one of: " + namesOf(table) + ")");
		}

		// Which option an argument is, and the value stuck to it if any.
		struct OptionMatch
		{
			char key = 0; // 'g', 'w', 'x', 'a', 'G' (--generate), 'o', 'r' (--record) or 'h'
			std::optional<std::string> attachedValue;
		};

		// "--game" and friends: the whole word, value in the next argument.
		std::optional<char> longOptionKey(const std::string& arg)
		{
			if (arg == "--game")   return 'g';
			if (arg == "--window") return 'w';
			if (arg == "--xml")    return 'x';
			if (arg == "--audio")  return 'a';
			if (arg == "--generate") return 'G';
			if (arg == "--output") return 'o';
			if (arg == "--record") return 'r';
			if (arg == "--check")  return 'c';
			if (arg == "--frames") return 'f';
			if (arg == "--seed")   return 's';
			if (arg == "--keys")   return 'k';
			if (arg == "--dump")   return 'd';
			if (arg == "--dump-every") return 'e';
			if (arg == "--help")   return 'h';
			return std::nullopt;
		}

		bool takesValue(char key)
		{
			return key != 'h' && key != 'r' && key != 'c';
		}

		std::string optionName(char key)
		{
			switch (key)
			{
			case 'g': return "game";
			case 'w': return "window";
			case 'x': return "xml";
			case 'a': return "audio";
			case 'G': return "target";
			case 'o': return "folder";
			case 'f': return "number of frames";
			case 's': return "seed";
			case 'k': return "file";
			case 'd': return "file";
			case 'e': return "number of frames";
			}
			return "help";
		}
	}

	CliOptions parseCommandLine(const std::vector<std::string>& args)
	{
		CliOptions options;

		bool haveGame = false;
		bool haveWindow = false;
		bool haveXml = false;
		bool haveAudio = false;
		bool haveGenerate = false;
		bool haveOutput = false;
		bool haveFrames = false;

		// Several only for --check, which may come after them: worked out at the end.
		auto setGame = [&](const std::string& game)
		{
			if (!haveGame) { options.game = game; }
			options.games.push_back(game);
			haveGame = true;
		};

		auto setOption = [&](char key, const std::string& value)
		{
			switch (key)
			{
			case 'g':
				setGame(value);
				break;
			case 'w':
				if (haveWindow)
				{
					throw CliError("the window library was given more than once");
				}
				options.window = lookUp(windowNames, "window library", value);
				haveWindow = true;
				break;
			case 'x':
				if (haveXml)
				{
					throw CliError("the XML library was given more than once");
				}
				options.xml = lookUp(xmlNames, "XML library", value);
				haveXml = true;
				break;
			case 'a':
				if (haveAudio)
				{
					throw CliError("the sound library was given more than once");
				}
				options.audio = lookUp(audioNames, "sound library", value);
				haveAudio = true;
				break;
			case 'G':
			{
				if (haveGenerate)
				{
					throw CliError("--generate was given more than once");
				}
				const std::string target = lowerCase(value);
				if (std::find(std::begin(generateNames), std::end(generateNames), target) == std::end(generateNames))
				{
					std::string names;
					for (const auto& name : generateNames)
					{
						names += (names.empty() ? "" : ", ") + std::string(name);
					}
					throw CliError("unknown target '" + value + "' to generate (choose one of: " + names + ")");
				}
				options.generate = target;
				haveGenerate = true;
				break;
			}
			case 'o':
				if (haveOutput)
				{
					throw CliError("the output folder was given more than once");
				}
				options.output = value;
				haveOutput = true;
				break;
			case 'f':
				if (haveFrames)
				{
					throw CliError("--frames was given more than once");
				}
				options.frames = static_cast<int>(wholeNumber(value, "--frames", 1));
				haveFrames = true;
				break;
			case 's':
				if (options.seed)
				{
					throw CliError("--seed was given more than once");
				}
				options.seed = static_cast<unsigned int>(wholeNumber(value, "--seed", 0));
				break;
			case 'k':
				if (!options.keys.empty())
				{
					throw CliError("--keys was given more than once");
				}
				options.keys = value;
				break;
			case 'd':
				if (!options.dump.empty())
				{
					throw CliError("--dump was given more than once");
				}
				options.dump = value;
				break;
			case 'e':
				if (options.dumpEvery > 0)
				{
					throw CliError("--dump-every was given more than once");
				}
				options.dumpEvery = static_cast<int>(wholeNumber(value, "--dump-every", 1));
				break;
			}
		};

		for (std::size_t i = 0; i < args.size(); ++i)
		{
			const std::string& arg = args[i];

			if (arg.size() < 2 || arg[0] != '-')
			{
				// Not an option: the game. (A lone "-" lands here too and is
				// simply a file that will not be found.)
				setGame(arg);
				continue;
			}

			OptionMatch match;
			std::string shownAs = arg;

			if (arg[1] == '-')
			{
				const std::optional<char> key = longOptionKey(arg);
				if (!key)
				{
					std::string message = "unknown option '" + arg + "'";
					if (arg.find('=') != std::string::npos)
					{
						message += " (a long option takes its value after a space, like --game pong)";
					}
					throw CliError(message);
				}
				match.key = *key;
			}
			else
			{
				match.key = arg[1];
				if (match.key != 'g' && match.key != 'w' && match.key != 'x' && match.key != 'a' && match.key != 'o' && match.key != 'h')
				{
					throw CliError("unknown option '" + arg + "'");
				}

				if (arg.size() > 2)
				{
					if (!takesValue(match.key))
					{
						throw CliError("unknown option '" + arg + "'");
					}
					match.attachedValue = arg.substr(2);
					shownAs = arg.substr(0, 2);
				}
			}

			if (match.key == 'c')
			{
				if (options.check)
				{
					throw CliError("--check was given more than once");
				}
				options.check = true;
				continue;
			}
			if (match.key == 'r')
			{
				if (options.record)
				{
					throw CliError("--record was given more than once");
				}
				options.record = true;
				continue;
			}
			if (!takesValue(match.key))
			{
				options.showHelp = true;
				continue;
			}

			// The value: stuck to a short option, or the next argument. A
			// next argument that is itself an option is not taken as one.
			std::string value;
			if (match.attachedValue)
			{
				value = *match.attachedValue;
			}
			else if (i + 1 < args.size() && !(args[i + 1].size() > 1 && args[i + 1][0] == '-'))
			{
				value = args[++i];
			}

			if (value.empty())
			{
				throw CliError("option '" + shownAs + "' needs a " + optionName(match.key) + " name");
			}

			setOption(match.key, value);
		}

		if (options.games.size() > 1 && !options.check)
		{
			throw CliError("more than one game given ('" + options.games[0] + "' and '" + options.games[1] + "'); only --check takes several");
		}
		if (options.check && (options.record || haveGenerate || haveWindow || haveAudio))
		{
			throw CliError("--check only loads the games: it opens no window, plays no sound, and does not --record or --generate");
		}
		if (haveOutput && !haveGenerate)
		{
			throw CliError("an output folder is only for --generate");
		}
		if (options.record && haveGenerate)
		{
			throw CliError("--record plays the game, and --generate writes it out instead; give one of them");
		}
		if (options.seed && (options.check || haveGenerate))
		{
			throw CliError("--seed is for playing: --check and --generate draw no numbers");
		}
		if (!options.keys.empty())
		{
			if (options.check || haveGenerate || options.record)
			{
				throw CliError("--keys plays a recording back; it does not go with --check, --generate or --record");
			}
			if (options.seed)
			{
				throw CliError("--keys plays from the recording's own seed; leave out --seed");
			}
		}
		if (!options.dump.empty() && !haveFrames)
		{
			throw CliError("--dump writes what --frames played; give --frames too");
		}
		if (options.dumpEvery > 0 && options.dump.empty())
		{
			throw CliError("--dump-every says how often --dump writes; give --dump too");
		}
		if (haveFrames)
		{
			if (options.check || haveGenerate || options.record)
			{
				throw CliError("--frames plays the game; it does not go with --check, --generate or --record");
			}
			if (haveWindow && options.window != WindowBackend::None)
			{
				throw CliError("--frames plays with no window; leave out -w, or give -w none");
			}
			if (haveAudio && options.audio != AudioBackend::None)
			{
				throw CliError("--frames plays with no sound; leave out -a, or give -a none");
			}
			options.window = WindowBackend::None;
			options.audio = AudioBackend::None;
		}

		return options;
	}

	std::string findGameFile(const std::string& game, const std::filesystem::path& gamesDirectory)
	{
		// The search itself is shared with xgegui (engine/folder.h), so the two
		// programs look in the same places.
		if (const auto found = locateGameFile(game, gamesDirectory))
		{
			return found->string();
		}

		throw CliError("file not found: " + gameFileGiven(game).string());
	}

	std::string windowBackendName(WindowBackend backend)
	{
		for (const auto& entry : windowNames)
		{
			if (entry.backend == backend)
			{
				return std::string(entry.name);
			}
		}
		return "unknown";
	}

	std::string xmlBackendName(XmlBackend backend)
	{
		for (const auto& entry : xmlNames)
		{
			if (entry.backend == backend)
			{
				return std::string(entry.name);
			}
		}
		return "unknown";
	}

	std::string audioBackendName(AudioBackend backend)
	{
		for (const auto& entry : audioNames)
		{
			if (entry.backend == backend)
			{
				return std::string(entry.name);
			}
		}
		return "unknown";
	}

	std::vector<std::string> generateTargets()
	{
		return { std::begin(generateNames), std::end(generateNames) };
	}

	std::string usageText()
	{
		return
			"usage: xgecli [game] [options]\n"
			"\n"
			"  game                 a game name (pong) or file (pong.xml, path/to/pong.xml);\n"
			"                       looked for as given, then in the working directory,\n"
			"                       then in games/descriptions/ (in the working directory,\n"
			"                       next to the program, or one folder above it).\n"
			"                       Default: pong\n"
			"\n"
			"options:\n"
			"  -g, --game <game>    the game, same as giving it bare\n"
			"  -w, --window <name>  window library: " + namesOf(windowNames) + " (default " + WindowFactory::name(WindowFactory::defaultBackend()) + ")\n"
			"  -x, --xml <name>     XML library: " + namesOf(xmlNames) + " (default " + XmlDocumentFactory::name(XmlDocumentFactory::defaultBackend()) + ")\n"
			"  -a, --audio <name>   sound library: " + namesOf(audioNames) + " (default " + AudioFactory::name(AudioFactory::defaultBackend()) + ")\n"
			"  --generate <target>  write the game out as a program instead of playing it:\n"
			"                       engine-cpp (C++ on SFML 3, with a CMakeLists.txt: the\n"
			"                       engine's own code with the game's data built in), or\n"
			"                       windows-cpp (the game as a person would write it, on\n"
			"                       SFML 3: pong_min.xml for now)\n"
			"  -o, --output <dir>   where --generate writes it (default <game>-<target>)\n"
			"  --record             play, and write what was played into the game file as\n"
			"                       its <demo> when the window closes (the attract mode)\n"
			"  --check              load the games given (several, or none for every game in\n"
			"                       games/descriptions/) and print ok or what is wrong with\n"
			"                       each; exits 1 if any is wrong\n"
			"  --frames <n>         play n frames as fast as they go, with no window and no\n"
			"                       sound, then stop\n"
			"  --seed <n>           the random numbers start from n, so a game plays the\n"
			"                       same every time\n"
			"  --keys <file>        play back the <demo> in the file (one --record wrote, or a\n"
			"                       file that is only a <demo>) from its own seed\n"
			"  --dump <file>        with --frames, write the game's state into the file as\n"
			"                       XML at the end (games/xgedump.xsd is its schema)\n"
			"  --dump-every <n>     and every n frames on the way\n"
			"  -h, --help           show this text\n"
			"\n"
			"A short option can have its value attached (-gpong -wsdl2 -xtinyxml2 -anone); a\n"
			"long option needs a space (--game pong).\n";
	}

	std::vector<std::string> gameFilesIn(const std::filesystem::path& gamesDirectory)
	{
		std::vector<std::string> files;
		std::error_code error;
		for (const auto& entry : std::filesystem::directory_iterator(gamesDirectory, error))
		{
			if (entry.is_regular_file() && entry.path().extension() == ".xml") { files.push_back(entry.path().string()); }
		}
		std::sort(files.begin(), files.end());
		return files;
	}

	int checkGames(const std::vector<std::string>& files, XmlBackend xml, std::ostream& out)
	{
		int failed = 0;
		for (const std::string& file : files)
		{
			std::string problem;
			{
				// The libraries' "loading, parsed" lines are not the answer.
				struct Unheard : std::streambuf
				{
					int overflow(int c) override { return traits_type::not_eof(c); }
				} unheard;
				std::streambuf* const before = std::cout.rdbuf(&unheard);
				try
				{
					Game game{ readGame(file, xml) };
					Engine engine(game, WindowBackend::None, AudioBackend::None);
				}
				catch (const std::exception& error)
				{
					problem = error.what();
					if (problem.empty()) { problem = "it does not load"; }
				}
				catch (...)
				{
					problem = "it does not load";
				}
				std::cout.rdbuf(before);
			}
			out << std::filesystem::path(file).filename().string() << ": " << (problem.empty() ? "ok" : problem) << '\n';
			if (!problem.empty()) { ++failed; }
		}
		return failed;
	}

	int playFrames(const std::string& file, const CliOptions& options, std::ostream& out)
	{
		// The seed was fixed only for this game: what it was goes back after.
		const std::optional<unsigned int> seedBefore = fixedRandomSeed();
		if (options.seed) { fixRandomSeed(options.seed); }
		struct SeedBack
		{
			std::optional<unsigned int> seed;
			bool fixed;
			~SeedBack() { if (fixed) { fixRandomSeed(seed); } }
		} seedBack{ seedBefore, options.seed.has_value() };

		// The dump is opened first, so a file that cannot be written stops
		// the run before it plays, and is closed whole however the run ends.
		std::ofstream dump;
		if (!options.dump.empty())
		{
			dump.open(options.dump, std::ios::binary | std::ios::trunc);
			if (!dump)
			{
				out << "Error: cannot write the dump to " << options.dump << '\n';
				return EXIT_FAILURE;
			}
			const std::filesystem::path schema = std::filesystem::absolute("xgedump.xsd").lexically_normal();
			dump << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
				<< "<!-- " << std::filesystem::path(file).filename().string() << " as xgecli played it, at each moment it was written. -->\n"
				<< "<dump xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\" xsi:noNamespaceSchemaLocation=\"" << schema.generic_string() << "\">\n";
		}
		struct DumpEnd
		{
			std::ofstream& file;
			~DumpEnd() { if (file.is_open()) { file << "</dump>\n"; } }
		} dumpEnd{ dump };

		int played = 0;
		try
		{
			Game game{ readGame(file, options.xml) };
			Engine engine(game, WindowBackend::None, AudioBackend::None);
			if (!options.keys.empty())
			{
				engine.replay(readDemo(options.keys, options.xml));
			}
			const auto describe = [&]
			{
				if (dump.is_open()) { game.describe(dump, static_cast<unsigned long>(played), "  "); }
			};
			while (played < options.frames)
			{
				engine.step();
				++played;
				if (options.dumpEvery > 0 && played % options.dumpEvery == 0 && played < options.frames) { describe(); }
			}
			describe();
			out << "played " << played << " frames of " << std::filesystem::path(file).filename().string()
				<< "; the state is " << game.getCurrentState().name << '\n';
		}
		catch (const std::exception& error)
		{
			out << "Error: " << (played > 0 ? "in frame " + std::to_string(played + 1) + ": " : std::string()) << error.what() << '\n';
			return EXIT_FAILURE;
		}
		return EXIT_SUCCESS;
	}
}
