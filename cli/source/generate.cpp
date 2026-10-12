// generate.cpp
// XML Game Engine
// author: beefviper
// date: Oct 5, 2026

#include "generate.h"

#include "game.h"
#include "game_xml.h"

#include <cctype>
#include <ctime>
#include <system_error>

#if XGE_WITH_XSLT
#include <libexslt/exslt.h>
#include <libxml/parser.h>
#include <libxml/tree.h>
#include <libxslt/transform.h>
#include <libxslt/variables.h>
#include <libxslt/xslt.h>
#include <libxslt/xsltInternals.h>
#include <libxslt/xsltutils.h>

#include <algorithm>
#include <cstdarg>
#include <cstdio>
#include <memory>
#include <set>
#endif

namespace xge
{
	std::string gameFunctionName(const std::filesystem::path& gameFile)
	{
		// Words split at anything that is not a letter or digit, run together
		// in camel case, never starting with a digit; as camel in text.xsl.
		std::string text;
		bool upper = false;
		for (const char each : gameFile.stem().string())
		{
			if (std::isalnum(static_cast<unsigned char>(each)) == 0)
			{
				upper = !text.empty();
				continue;
			}
			text += upper ? static_cast<char>(std::toupper(static_cast<unsigned char>(each))) : each;
			upper = false;
		}
		if (text.empty() || std::isdigit(static_cast<unsigned char>(text.front())) != 0)
		{
			text = "n" + text;
		}
		return text + "Game";
	}

#if XGE_WITH_XSLT
	namespace
	{
		// libxml2 and libxslt report errors (and an <xsl:message>) through a
		// printf-style function; this one keeps the text, so it can go in the
		// exception rather than straight to the console.
#if defined(__GNUC__)
		__attribute__((format(printf, 2, 3)))
#endif
		void collectMessage(void* context, const char* format, ...)
		{
			auto* messages = static_cast<std::string*>(context);

			va_list arguments;
			va_start(arguments, format);
			char buffer[1024];
			const int length = std::vsnprintf(buffer, sizeof buffer, format, arguments);
			va_end(arguments);

			if (length > 0)
			{
				messages->append(buffer, std::min(static_cast<std::size_t>(length), sizeof buffer - 1));
			}
		}

		// Sends the libraries' messages to `messages` for as long as it lives.
		class MessageCollector
		{
		public:
			explicit MessageCollector(std::string& messages)
			{
				xmlSetGenericErrorFunc(&messages, collectMessage);
				xsltSetGenericErrorFunc(&messages, collectMessage);
			}

			~MessageCollector()
			{
				xmlSetGenericErrorFunc(nullptr, nullptr);
				xsltSetGenericErrorFunc(nullptr, nullptr);
			}

			MessageCollector(const MessageCollector&) = delete;
			MessageCollector& operator=(const MessageCollector&) = delete;
		};

		// The stylesheet writes its files relative to the name given for its
		// own output, and libxslt resolves that as a URI, which a Windows path
		// is not. So the output folder is made the working directory while it
		// runs, and the names are plain relative ones.
		class WorkingDirectory
		{
		public:
			explicit WorkingDirectory(const std::filesystem::path& folder) :
				previous(std::filesystem::current_path())
			{
				std::filesystem::current_path(folder);
			}

			~WorkingDirectory()
			{
				std::error_code ignored;
				std::filesystem::current_path(previous, ignored);
			}

			WorkingDirectory(const WorkingDirectory&) = delete;
			WorkingDirectory& operator=(const WorkingDirectory&) = delete;

		private:
			std::filesystem::path previous;
		};

		struct FreeDocument { void operator()(xmlDoc* document) const { xmlFreeDoc(document); } };
		struct FreeStylesheet { void operator()(xsltStylesheet* style) const { xsltFreeStylesheet(style); } };
		struct FreeContext { void operator()(xsltTransformContext* context) const { xsltFreeTransformContext(context); } };

		using Document = std::unique_ptr<xmlDoc, FreeDocument>;
		using Stylesheet = std::unique_ptr<xsltStylesheet, FreeStylesheet>;
		using Context = std::unique_ptr<xsltTransformContext, FreeContext>;

		const xmlChar* xml(const std::string& text)
		{
			return reinterpret_cast<const xmlChar*>(text.c_str());
		}

		std::string attribute(xmlNode* node, const char* name)
		{
			xmlChar* value = xmlGetProp(node, reinterpret_cast<const xmlChar*>(name));
			if (!value)
			{
				return {};
			}
			std::string text(reinterpret_cast<const char*>(value));
			xmlFree(value);
			return text;
		}

		std::string withMessages(const std::string& what, const std::string& messages)
		{
			if (messages.empty())
			{
				return what;
			}
			std::string text = what + ":\n" + messages;
			while (!text.empty() && (text.back() == '\n' || text.back() == ' '))
			{
				text.pop_back();
			}
			return text;
		}

		// The day the program is written, as the header block says it.
		std::string today()
		{
			const std::time_t now = std::time(nullptr);
			std::tm local{};
#if defined(_WIN32)
			localtime_s(&local, &now);
#else
			localtime_r(&now, &local);
#endif
			static const char* months[] = { "Jan", "Feb", "Mar", "Apr", "May", "June", "July", "Aug", "Sept", "Oct", "Nov", "Dec" };
			return std::string(months[local.tm_mon]) + " " + std::to_string(local.tm_mday) + ", " + std::to_string(local.tm_year + 1900);
		}

		void copyFile(const std::filesystem::path& from, const std::filesystem::path& to)
		{
			std::error_code failed;
			std::filesystem::create_directories(to.parent_path(), failed);
			std::filesystem::copy_file(from, to, std::filesystem::copy_options::overwrite_existing, failed);
			if (failed)
			{
				throw GenerateError("could not copy " + from.string() + " to " + to.string() + ": " + failed.message());
			}
		}

		// Runs the stylesheet on the game and copies in what its list names.
		GeneratedProgram transform(const GenerateRequest& request, const std::filesystem::path& gameFile, const std::filesystem::path& output)
		{
			const std::filesystem::path targetFolder = request.generators / request.target;

			std::string messages;
			const MessageCollector collector(messages);

			exsltRegisterAll();

			Document game(xmlReadFile(gameFile.string().c_str(), nullptr, XML_PARSE_NONET));
			if (!game)
			{
				throw GenerateError(withMessages("could not read " + gameFile.string(), messages));
			}

			const std::filesystem::path stylesheetFile = std::filesystem::absolute(targetFolder / "generate.xsl");
			const Stylesheet style(xsltParseStylesheetFile(xml(stylesheetFile.string())));
			if (!style)
			{
				throw GenerateError(withMessages("could not read the stylesheet " + stylesheetFile.string(), messages));
			}

			// The program's name, the game file it comes from, the function
			// its data is and the date, for the stylesheet.
			const std::string name = gameFile.stem().string();
			const std::string source = gameFile.filename().string();
			const std::string function = gameFunctionName(gameFile);
			const std::string date = today();
			const char* parameters[] = { "name", name.c_str(), "source", source.c_str(), "function", function.c_str(), "date", date.c_str(), nullptr };

			const Context context(xsltNewTransformContext(style.get(), game.get()));
			if (!context || xsltQuoteUserParams(context.get(), parameters) != 0)
			{
				throw GenerateError(withMessages("could not start the stylesheet", messages));
			}

			// A program is laid out in source/ and engine/, which exsl:document
			// does not make for the files it writes there.
			std::filesystem::create_directories(output / "source");

			Document manifest;
			{
				const WorkingDirectory inOutput(output);
				manifest.reset(xsltApplyStylesheetUser(style.get(), game.get(), nullptr, "manifest.xml", nullptr, context.get()));
			}

			if (!manifest || context->state == XSLT_STATE_ERROR || context->state == XSLT_STATE_STOPPED)
			{
				throw GenerateError(withMessages("could not generate " + source + " for " + request.target, messages));
			}

			GeneratedProgram program;
			std::set<std::filesystem::path> copied;
			xmlNode* root = xmlDocGetRootElement(manifest.get());
			for (xmlNode* node = root ? root->children : nullptr; node; node = node->next)
			{
				if (node->type != XML_ELEMENT_NODE)
				{
					continue;
				}

				const std::string element(reinterpret_cast<const char*>(node->name));
				const std::filesystem::path path = attribute(node, "path");
				if (path.empty() || !copied.insert(path).second)
				{
					continue;
				}

				if (element == "file")
				{
					program.files.push_back(path);
				}
				else if (element == "module")
				{
					// one of the target's own files, from its modules/ folder
					copyFile(targetFolder / "modules" / attribute(node, "from"), output / path);
					program.files.push_back(path);
				}
				else if (element == "engine")
				{
					// one of the engine's files, from lib/ in the data folder, unchanged
					copyFile(request.dataFolder / attribute(node, "from"), output / path);
					program.files.push_back(path);
				}
				else if (element == "asset")
				{
					copyFile(request.dataFolder / path, output / path);
					program.assets.push_back(path);
				}
			}

			return program;
		}
	}

	bool canGenerate() noexcept
	{
		return true;
	}

	GeneratedProgram generateGame(const GenerateRequest& request)
	{
		const std::filesystem::path stylesheetFile = request.generators / request.target / "generate.xsl";
		if (request.target.empty() || !std::filesystem::exists(stylesheetFile))
		{
			throw GenerateError("no generator for " + request.target + " (looked for " + stylesheetFile.string() + ")");
		}

		const std::filesystem::path gameFile = std::filesystem::absolute(request.gameFile);

		// Read as xgecli reads it, and worked out once, so a game the engine
		// would not load is refused here rather than by the program; the
		// stylesheet can then take the file as the engine does.
		try
		{
			const Game check{ readGame(gameFile.string()) };
		}
		catch (const std::exception& error)
		{
			throw GenerateError("could not generate " + gameFile.filename().string() + ": " + error.what());
		}

		const std::filesystem::path output = std::filesystem::absolute(request.output);
		const bool outputWasThere = std::filesystem::exists(output);
		try
		{
			std::filesystem::create_directories(output);
			return transform(request, gameFile, output);
		}
		catch (...)
		{
			// Half a program is no use: a folder made for it goes again.
			if (!outputWasThere)
			{
				std::error_code ignored;
				std::filesystem::remove_all(output, ignored);
			}
			throw;
		}
	}
#else
	bool canGenerate() noexcept
	{
		return false;
	}

	GeneratedProgram generateGame(const GenerateRequest&)
	{
		throw GenerateError("this xgecli was built without libxslt, which --generate needs (vcpkg install libxslt, or libxslt1-dev on Linux, then configure again)");
	}
#endif
}
