// cli.cpp
// XML Game Engine
// author: beefviper
// date: Sept 28, 2026

#include "cli.h"
#include "data_folder.h"

#include <algorithm>
#include <cctype>
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

		std::string lowerCase(std::string text)
		{
			std::transform(text.begin(), text.end(), text.begin(),
				[](unsigned char c) { return static_cast<char>(std::tolower(c)); });
			return text;
		}

		template <typename Table>
		std::string namesOf(const Table& table)
		{
			std::string names;
			for (const auto& entry : table)
			{
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
					return entry.backend;
				}
			}

			throw CliError("unknown " + what + " '" + value + "' (choose one of: " + namesOf(table) + ")");
		}

		// Which option an argument is, and the value stuck to it if any.
		struct OptionMatch
		{
			char key = 0; // 'g', 'w', 'x', 'a' or 'h'
			std::optional<std::string> attachedValue;
		};

		// "--game" and friends: the whole word, value in the next argument.
		std::optional<char> longOptionKey(const std::string& arg)
		{
			if (arg == "--game")   return 'g';
			if (arg == "--window") return 'w';
			if (arg == "--xml")    return 'x';
			if (arg == "--audio")  return 'a';
			if (arg == "--help")   return 'h';
			return std::nullopt;
		}

		bool takesValue(char key)
		{
			return key != 'h';
		}

		std::string optionName(char key)
		{
			switch (key)
			{
			case 'g': return "game";
			case 'w': return "window";
			case 'x': return "xml";
			case 'a': return "audio";
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

		auto setGame = [&](const std::string& game)
		{
			if (haveGame)
			{
				throw CliError("more than one game given ('" + options.game + "' and '" + game + "')");
			}
			options.game = game;
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
				if (match.key != 'g' && match.key != 'w' && match.key != 'x' && match.key != 'a' && match.key != 'h')
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

		return options;
	}

	std::string findGameFile(const std::string& game, const std::filesystem::path& gamesDirectory)
	{
		// The search itself is shared with XGEGUI (data_folder.h), so the two
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

	std::string usageText()
	{
		return
			"usage: XGECLI [game] [options]\n"
			"\n"
			"  game                 a game name (pong) or file (pong.xml, path/to/pong.xml);\n"
			"                       looked for as given, then in the working directory,\n"
			"                       then in the games directory (games/ in the working\n"
			"                       directory, next to the program, or one folder above\n"
			"                       it). Default: pong\n"
			"\n"
			"options:\n"
			"  -g, --game <game>    the game, same as giving it bare\n"
			"  -w, --window <name>  window library: " + namesOf(windowNames) + " (default sfml3)\n"
			"  -x, --xml <name>     XML library: " + namesOf(xmlNames) + " (default xerces)\n"
			"  -a, --audio <name>   sound library: " + namesOf(audioNames) + " (default sfml3)\n"
			"  -h, --help           show this text\n"
			"\n"
			"A short option can have its value attached (-gpong -wsdl2 -xtinyxml2 -anone); a\n"
			"long option needs a space (--game pong).\n";
	}
}
