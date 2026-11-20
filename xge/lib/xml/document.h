// document.h
// XML Game Engine
// author: beefviper
// date: Sept 28, 2026

#pragma once

#include <cstddef>
#include <memory>
#include <ostream>
#include <string>
#include <utility>
#include <vector>

namespace xge
{
	// One parsed element, in no XML library's types, so load/xml.cpp names
	// none of them. A child is handed out as a node of its own: the reader
	// keeps several alive at once while it walks.
	class XmlNode
	{
	public:
		virtual ~XmlNode() = default;

		virtual std::string getName() const = 0;

		// "" when it is not there.
		virtual std::string getAttribute(const std::string& name) const = 0;

		// Every attribute it has, name and value, in the order written: what
		// load/merge needs to copy an element.
		virtual std::vector<std::pair<std::string, std::string>> getAttributes() const = 0;

		// Its text and CDATA joined, as written and not trimmed: <x>100</x>
		// gives "100", <x><random/></x> only its whitespace.
		virtual std::string getText() const = 0;

		// The first child or next sibling element of any tag, null for none;
		// the reader picks by name itself. Only elements are visited.
		virtual std::unique_ptr<XmlNode> getFirstChild() const = 0;
		virtual std::unique_ptr<XmlNode> getNextSibling() const = 0;

		// The line of the file its tag starts on, counting from 1; 0 when
		// that is not known. What a load error points at.
		virtual int line() const { return 0; }
	};

	// A parsed document. Only Xerces checks it against the schema as it
	// loads; for the others load/xml.cpp runs load/xsd.h's check.
	class XmlDocument
	{
	public:
		virtual ~XmlDocument() = default;

		virtual bool load(const std::string& filename) = 0;
		virtual std::string getErrorMessage() const = 0;
		virtual std::unique_ptr<XmlNode> getRootElement() const = 0;
	};

	// Where each line of a text starts, and the line (from 1) an offset into
	// it is on: for a library that knows where in the file an element is
	// but not its line.
	std::vector<std::size_t> lineStarts(const std::string& text);
	int lineAt(const std::vector<std::size_t>& starts, std::size_t offset) noexcept;

	// " (line 12)", as a message about a place in a file ends; "" for line 0.
	// Inline: a generated program has the reader's messages but not this
	// file's parsers.
	inline std::string lineNote(int line)
	{
		return line > 0 ? " (line " + std::to_string(line) + ")" : std::string{};
	}

	enum class XmlBackend
	{
		Xerces,
		TinyXml2,
		PugiXml,
		RapidXml
	};

	// How a document was checked against xgedef.xsd.
	enum class SchemaValidation
	{
		None,   // no schema to check against
		Weak,   // by load/xsd.h's own check: every backend but Xerces
		Strong  // by Xerces's XSD engine
	};

	std::ostream& operator<<(std::ostream& o, SchemaValidation validation);

	// The one place that knows every backend. A program is built with some
	// of them (options.cmake: Xerces alone by default, all for the tests).
	class XmlDocumentFactory
	{
	public:
		// Throws std::runtime_error for a backend that was not built into this
		// program (the message says which are).
		static std::unique_ptr<XmlDocument> create(XmlBackend backend = defaultBackend());

		// Whether this program was built with the backend.
		static bool available(XmlBackend backend);

		// The backends this program was built with, in XmlBackend's order.
		static std::vector<XmlBackend> availableBackends();

		// Xerces when it is built, otherwise the first one that is.
		static XmlBackend defaultBackend();

		// The backend's plain lower case name, as -x takes it: "xerces",
		// "tinyxml2", "pugixml", "rapidxml".
		static std::string name(XmlBackend backend);
	};
}
