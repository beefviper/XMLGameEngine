// generate.cpp
// XML Game Engine
// author: beefviper
// date: Oct 5, 2026

#include "generate.h"

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
#endif

#include <system_error>

namespace xge
{
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
	}

	bool canGenerate() noexcept
	{
		return true;
	}

	GeneratedProgram generateGame(const GenerateRequest& request)
	{
		const std::filesystem::path stylesheetFile = std::filesystem::absolute(request.generators / request.target / "generate.xsl");
		if (!std::filesystem::exists(stylesheetFile))
		{
			throw GenerateError("no generator for " + request.target + " (looked for " + stylesheetFile.string() + ")");
		}

		const std::filesystem::path gameFile = std::filesystem::absolute(request.gameFile);
		const std::filesystem::path output = std::filesystem::absolute(request.output);
		const bool outputWasThere = std::filesystem::exists(output);
		std::filesystem::create_directories(output);

		std::string messages;
		const MessageCollector collector(messages);

		exsltRegisterAll();

		const Document game(xmlReadFile(gameFile.string().c_str(), nullptr, XML_PARSE_NONET));
		if (!game)
		{
			throw GenerateError(withMessages("could not read " + gameFile.string(), messages));
		}

		const Stylesheet style(xsltParseStylesheetFile(xml(stylesheetFile.string())));
		if (!style)
		{
			throw GenerateError(withMessages("could not read the stylesheet " + stylesheetFile.string(), messages));
		}

		// The program's name and the game file it comes from, for the stylesheet.
		const std::string name = gameFile.stem().string();
		const std::string source = gameFile.filename().string();
		const char* parameters[] = { "name", name.c_str(), "source", source.c_str(), nullptr };

		const Context context(xsltNewTransformContext(style.get(), game.get()));
		if (!context || xsltQuoteUserParams(context.get(), parameters) != 0)
		{
			throw GenerateError(withMessages("could not start the stylesheet", messages));
		}

		Document manifest;
		{
			const WorkingDirectory inOutput(output);
			manifest.reset(xsltApplyStylesheetUser(style.get(), game.get(), nullptr, "manifest.xml", nullptr, context.get()));
		}

		if (!manifest || context->state == XSLT_STATE_ERROR || context->state == XSLT_STATE_STOPPED)
		{
			// Half a program is no use: a folder made for it goes again.
			if (!outputWasThere)
			{
				std::error_code ignored;
				std::filesystem::remove_all(output, ignored);
			}
			throw GenerateError(withMessages("could not generate " + source + " for " + request.target, messages));
		}

		GeneratedProgram program;
		xmlNode* root = xmlDocGetRootElement(manifest.get());
		for (xmlNode* node = root ? root->children : nullptr; node; node = node->next)
		{
			if (node->type != XML_ELEMENT_NODE)
			{
				continue;
			}

			const std::string element(reinterpret_cast<const char*>(node->name));
			const std::filesystem::path path = attribute(node, "path");

			if (element == "file")
			{
				program.files.push_back(path);
			}
			else if (element == "asset")
			{
				const std::filesystem::path from = request.dataFolder / path;
				const std::filesystem::path to = output / path;
				std::error_code failed;
				std::filesystem::create_directories(to.parent_path(), failed);
				std::filesystem::copy_file(from, to, std::filesystem::copy_options::overwrite_existing, failed);
				if (failed)
				{
					throw GenerateError("could not copy " + from.string() + " to " + to.string() + ": " + failed.message());
				}
				program.assets.push_back(path);
			}
		}

		return program;
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
