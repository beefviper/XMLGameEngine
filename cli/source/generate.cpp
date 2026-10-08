// generate.cpp
// XML Game Engine
// author: beefviper
// date: Oct 5, 2026

#include "generate.h"

#if XGE_WITH_XSLT
#include "svg.h"

#include <libexslt/exslt.h>
#include <libxml/parser.h>
#include <libxml/tree.h>
#include <libxslt/transform.h>
#include <libxslt/variables.h>
#include <libxslt/xslt.h>
#include <libxslt/xsltInternals.h>
#include <libxslt/xsltutils.h>

#include <algorithm>
#include <array>
#include <cstdarg>
#include <cstdint>
#include <cstdio>
#include <fstream>
#include <locale>
#include <memory>
#include <sstream>
#include <stdexcept>
#include <utility>
#include <vector>
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

		// A picture as a PNG file, the plainest kind: 8 bits a channel with
		// alpha, and its pixels stored, not compressed (a generated game's
		// drawings are small). What an <svg> sprite is drawn into, so the
		// generated program loads it like any picture.
		void writePng(const Bitmap& picture, const std::filesystem::path& file)
		{
			std::array<std::uint32_t, 256> crcTable{};
			for (std::uint32_t n = 0; n < 256; ++n)
			{
				std::uint32_t c = n;
				for (int k = 0; k < 8; ++k)
				{
					c = (c & 1u) ? 0xEDB88320u ^ (c >> 1) : c >> 1;
				}
				crcTable[n] = c;
			}

			std::vector<std::uint8_t> out{ 0x89, 'P', 'N', 'G', '\r', '\n', 0x1A, '\n' };
			const auto put32 = [](std::vector<std::uint8_t>& bytes, std::uint32_t value)
			{
				for (int shift = 24; shift >= 0; shift -= 8)
				{
					bytes.push_back(static_cast<std::uint8_t>(value >> shift));
				}
			};
			const auto chunk = [&](const char* type, const std::vector<std::uint8_t>& data)
			{
				put32(out, static_cast<std::uint32_t>(data.size()));
				std::vector<std::uint8_t> body(type, type + 4);
				body.insert(body.end(), data.begin(), data.end());
				std::uint32_t crc = 0xFFFFFFFFu;
				for (const std::uint8_t byte : body)
				{
					crc = crcTable[(crc ^ byte) & 0xFFu] ^ (crc >> 8);
				}
				out.insert(out.end(), body.begin(), body.end());
				put32(out, crc ^ 0xFFFFFFFFu);
			};

			std::vector<std::uint8_t> header;
			put32(header, static_cast<std::uint32_t>(picture.width));
			put32(header, static_cast<std::uint32_t>(picture.height));
			header.insert(header.end(), { 8, 6, 0, 0, 0 });
			chunk("IHDR", header);

			// Each row starts with its filter (0, none), then its pixels.
			std::vector<std::uint8_t> raw;
			const std::size_t rowBytes = static_cast<std::size_t>(picture.width) * 4;
			for (std::size_t row = 0; row < static_cast<std::size_t>(picture.height); ++row)
			{
				raw.push_back(0);
				raw.insert(raw.end(), picture.rgba.begin() + static_cast<std::ptrdiff_t>(row * rowBytes),
					picture.rgba.begin() + static_cast<std::ptrdiff_t>((row + 1) * rowBytes));
			}

			// A zlib stream of stored deflate blocks, then its Adler-32.
			std::vector<std::uint8_t> zlib{ 0x78, 0x01 };
			std::size_t at = 0;
			do
			{
				const std::size_t length = std::min<std::size_t>(raw.size() - at, 65535);
				const bool last = at + length == raw.size();
				zlib.push_back(last ? 1 : 0);
				zlib.push_back(static_cast<std::uint8_t>(length & 0xFFu));
				zlib.push_back(static_cast<std::uint8_t>(length >> 8));
				zlib.push_back(static_cast<std::uint8_t>(~length & 0xFFu));
				zlib.push_back(static_cast<std::uint8_t>((~length >> 8) & 0xFFu));
				zlib.insert(zlib.end(), raw.begin() + static_cast<std::ptrdiff_t>(at), raw.begin() + static_cast<std::ptrdiff_t>(at + length));
				at += length;
			} while (at < raw.size());
			std::uint32_t a = 1;
			std::uint32_t b = 0;
			for (const std::uint8_t byte : raw)
			{
				a = (a + byte) % 65521u;
				b = (b + a) % 65521u;
			}
			put32(zlib, (b << 16) | a);
			chunk("IDAT", zlib);
			chunk("IEND", {});

			std::ofstream stream(file, std::ios::binary);
			stream.write(reinterpret_cast<const char*>(out.data()), static_cast<std::streamsize>(out.size()));
			if (!stream)
			{
				throw GenerateError("could not write " + file.string());
			}
		}

		// A number written in the manifest, read the same whatever the locale.
		float number(const std::string& text)
		{
			std::istringstream in(text);
			in.imbue(std::locale::classic());
			float value{};
			if (!(in >> value))
			{
				throw GenerateError("the generator wrote \"" + text + "\" where a number belongs");
			}
			return value;
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

		Document game(xmlReadFile(gameFile.string().c_str(), nullptr, XML_PARSE_NONET));
		if (!game)
		{
			throw GenerateError(withMessages("could not read " + gameFile.string(), messages));
		}

		// A target may first put the game in a form it writes more simply
		// (windows-cpp: a member of a group shown on a screen of its own
		// becomes an object), with prepare.xsl beside generate.xsl.
		const std::filesystem::path prepareFile = std::filesystem::absolute(request.generators / request.target / "prepare.xsl");
		if (std::filesystem::exists(prepareFile))
		{
			const Stylesheet prepare(xsltParseStylesheetFile(xml(prepareFile.string())));
			if (!prepare)
			{
				throw GenerateError(withMessages("could not read the stylesheet " + prepareFile.string(), messages));
			}
			Document prepared(xsltApplyStylesheet(prepare.get(), game.get(), nullptr));
			if (!prepared)
			{
				throw GenerateError(withMessages("could not prepare " + gameFile.filename().string() + " for " + request.target, messages));
			}
			game = std::move(prepared);
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

			// A file copied in: a game's asset from the data folder, or one of the
			// target's modules (physics.h) from its modules/ folder.
			const auto copyIn = [&](const std::filesystem::path& from)
			{
				const std::filesystem::path to = output / path;
				std::error_code failed;
				std::filesystem::create_directories(to.parent_path(), failed);
				std::filesystem::copy_file(from, to, std::filesystem::copy_options::overwrite_existing, failed);
				if (failed)
				{
					throw GenerateError("could not copy " + from.string() + " to " + to.string() + ": " + failed.message());
				}
			};

			if (element == "file")
			{
				program.files.push_back(path);
			}
			else if (element == "module")
			{
				copyIn(request.generators / request.target / "modules" / path);
				program.files.push_back(path);
			}
			else if (element == "asset")
			{
				copyIn(request.dataFolder / path);
				program.assets.push_back(path);
			}
			else if (element == "picture")
			{
				// An <svg> sprite, drawn now as the engine draws it when a game
				// loads, so the program has a plain picture to open.
				SvgRegion region;
				if (!attribute(node, "width").empty())
				{
					region = { number(attribute(node, "x")), number(attribute(node, "y")),
						number(attribute(node, "width")), number(attribute(node, "height")) };
				}
				const std::string scale = attribute(node, "scale");
				std::vector<std::string> hide;
				for (xmlNode* child = node->children; child; child = child->next)
				{
					if (child->type == XML_ELEMENT_NODE)
					{
						hide.push_back(attribute(child, "id"));
					}
				}

				const std::filesystem::path to = output / path;
				std::error_code failed;
				std::filesystem::create_directories(to.parent_path(), failed);
				try
				{
					const std::filesystem::path svg = request.dataFolder / attribute(node, "svg");
					writePng(rasterizeSvg(svg.string(), region, scale.empty() ? 1.0f : number(scale), hide), to);
				}
				catch (const std::exception& error)
				{
					throw GenerateError("could not draw " + attribute(node, "svg") + " for " + path.string() + ": " + error.what());
				}
				program.drawn.push_back(path);
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
