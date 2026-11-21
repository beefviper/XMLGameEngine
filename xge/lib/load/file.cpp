// file.cpp
// XML Game Engine
// author: beefviper
// date: Nov 19, 2026

#include "load/xml.h"

#include "engine/folder.h"
#include "game/game.h"
#include "load/reading.h"
#include "load/xsd.h"

#include <filesystem>
#include <memory>
#include <sstream>
#include <stdexcept>
#include <string>

// A game read from its file: the file loaded with one of the XML libraries
// and checked against its schema, then read by load/xml.cpp. A generated
// program reads its game from a tree built into it instead, so it has
// neither the libraries nor this file.

namespace xge
{
	namespace
	{
		constexpr const char* engineSchema = "xgedef.xsd";

		// The game file loaded and checked against its schema.
		std::unique_ptr<XmlDocument> loadGameFile(const std::string& filename, XmlBackend backend, SchemaValidation& validation)
		{
			std::unique_ptr<XmlDocument> document = XmlDocumentFactory::create(backend);

			if (!document->load(filename))
			{
				// Thrown, not exit(): a front end that loads games one after another
				// (xgegui) tells the user and carries on.
				throw std::runtime_error("XML file failed to load: " + document->getErrorMessage());
			}

			// find key points in document
			std::unique_ptr<XmlNode> root = document->getRootElement();

			// Xerces checked the schema the file names as it loaded; every other
			// library is checked here by the engine's own checker (load/xsd.h),
			// against the same file. A game that names none is checked against
			// the xgedef.xsd beside it or above it, else the games folder's.
			const std::string schemaLocation = root ? root->getAttribute("xsi:noNamespaceSchemaLocation") : std::string{};

			if (!schemaLocation.empty() && backend == XmlBackend::Xerces)
			{
				validation = SchemaValidation::Strong;
			}
			else
			{
				std::filesystem::path found = findSchema(filename, schemaLocation.empty() ? std::string(engineSchema) : schemaLocation);
				if (schemaLocation.empty() && !std::filesystem::exists(found)) { found = engineSchema; }
				if (!std::filesystem::exists(found))
				{
					throw std::runtime_error("XML file names no schema, and there is no " + std::string(engineSchema) + " beside it, above it or in the working folder to check it against");
				}
				const std::string schemaPath = found.string();

				std::string why;
				if (!validateWithKeptSchema(*root, schemaPath, backend, why))
				{
					throw std::runtime_error("XML file failed to validate against the schema: " + why);
				}

				validation = SchemaValidation::Weak;
			}

			return document;
		}
	}

	GameData readGame(const std::string& filename, XmlBackend backend)
	{
		SchemaValidation validation = SchemaValidation::None;
		const std::unique_ptr<XmlDocument> document = loadGameFile(filename, backend, validation);
		GameData data = readGame(*document->getRootElement());

		std::ostringstream said;
		said << validation;
		data.validation = said.str();
		return data;
	}

	Game::Game(const std::string& file) :
		Game(readGame(file))
	{
	}

	WrittenDemo readDemo(const std::string& filename, XmlBackend backend)
	{
		std::unique_ptr<XmlDocument> document = XmlDocumentFactory::create(backend);
		if (!document->load(filename))
		{
			throw std::runtime_error("the keys file failed to load: " + document->getErrorMessage());
		}
		std::unique_ptr<XmlNode> root = document->getRootElement();
		if (root && root->getName() == "demo")
		{
			return readDemo(*root);
		}
		if (std::unique_ptr<XmlNode> demoNode = root ? findChild(root.get(), "demo") : nullptr)
		{
			return readDemo(*demoNode);
		}
		throw std::runtime_error(filename + " has no <demo>: a keys file is a <demo> as --record writes one, or a game file holding one");
	}

	std::string withDemo(const std::string& gameText, const WrittenDemo& demo)
	{
		std::ostringstream element;
		element << "  <demo>\n"
			<< "    <seed>" << demo.seed << "</seed>\n"
			<< "    <frames>" << demo.frames << "</frames>\n";
		for (const DemoKey& key : demo.keys)
		{
			const char* tag = key.pressed ? "press" : "release";
			element << "    <" << tag << " button=\"" << key.button << "\">" << key.frame << "</" << tag << ">\n";
		}
		element << "  </demo>\n";

		// The one it had goes, from the start of its line to the end of it.
		std::string text = gameText;
		const auto open = text.find("<demo>", text.rfind("</states>") == std::string::npos ? 0 : text.rfind("</states>"));
		if (open != std::string::npos)
		{
			const auto close = text.find("</demo>", open);
			if (close != std::string::npos)
			{
				const auto lineStart = text.rfind('\n', open);
				auto end = text.find('\n', close);
				end = end == std::string::npos ? text.size() : end + 1;
				text.erase(lineStart == std::string::npos ? 0 : lineStart + 1, end - (lineStart == std::string::npos ? 0 : lineStart + 1));
			}
		}

		const auto gameEnd = text.rfind("</game>");
		if (gameEnd == std::string::npos)
		{
			throw std::runtime_error("the game file has no </game> to write the demo before");
		}

		// Lines end as the file's do (a Windows checkout's end in \r\n).
		std::string lines = element.str();
		if (text.find("\r\n") != std::string::npos)
		{
			std::string crlf;
			for (const char c : lines)
			{
				if (c == '\n') { crlf += '\r'; }
				crlf += c;
			}
			lines = std::move(crlf);
		}

		const auto lineStart = text.rfind('\n', gameEnd);
		text.insert(lineStart == std::string::npos ? gameEnd : lineStart + 1, lines);
		return text;
	}}
