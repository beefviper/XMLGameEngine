// xml_document.h
// XML Game Engine
// author: beefviper
// date: Sept 28, 2026

#pragma once

#include <memory>
#include <ostream>
#include <string>
#include <vector>

namespace xge
{
	// What game_xml needs from one parsed XML element - a tag name,
	// attribute lookup, its own text, and unfiltered first-child/next-sibling
	// traversal -
	// expressed with no particular XML library's types, so game_xml.cpp
	// never has to name Xerces/TinyXML2/PugiXML/RapidXML directly - same
	// shape as Window/Object in window.h for the graphics backends.
	//
	// getFirstChild()/getNextSibling() return the first/next element
	// regardless of its tag name (this schema's own elements aren't
	// guaranteed to stay in a fixed order - see xgedef.xsd),
	// so game_xml.cpp's own findChild() walks siblings itself, comparing
	// getName() against the child name it wants, rather than assuming a
	// fixed position.
	//
	// getFirstChild()/getNextSibling() return a fresh, independently-owned
	// node (nullptr if there isn't one) rather than a raw pointer into some
	// shared cache: game_xml.cpp keeps several of these alive at once while
	// it walks a subtree (e.g. xc_sprite, xc_pos, xc_vel, xc_collisions all
	// derived from the same object element and read from out of order), so
	// plain unique_ptr ownership is simpler than a backend having to keep
	// every node it ever handed out alive in a cache of its own.
	class XmlNode
	{
	public:
		virtual ~XmlNode() = default;

		virtual std::string getName() const = 0;

		// "" if the attribute isn't present - never fails, matching every
		// backend's own C++ library convention (XPath-less flat attributes,
		// no distinction needed here between "empty value" and "missing").
		virtual std::string getAttribute(const std::string& name) const = 0;

		// The element's own text: every text and CDATA child joined together,
		// exactly as written - not trimmed, so an element that only holds
		// child elements and the whitespace between them gives back that
		// whitespace. "" if it has none. (<x>100</x> gives "100"; the text
		// of <x><random/></x> is just its whitespace.)
		virtual std::string getText() const = 0;

		// The first child element regardless of its tag name, or the next
		// sibling element regardless of its tag name - nullptr if there is
		// none. Deliberately unfiltered by name (unlike a typical library's
		// own name-filtered traversal) - see the class comment above for why
		// (game_xml.cpp's own findChild() does the name filtering instead).
		// Only elements are visited: the text, comments and processing
		// instructions between them are not, in any backend.
		virtual std::unique_ptr<XmlNode> getFirstChild() const = 0;
		virtual std::unique_ptr<XmlNode> getNextSibling() const = 0;
	};

	// What game_xml needs from a whole parsed document - load it, find out
	// whether that worked, and get the root element to start walking from.
	// A backend that can't validate against an XSD schema (only Xerces can,
	// among the four below) just treats "load" as "well-formed XML parsed
	// successfully" - see xml_xerces.cpp vs xml_tinyxml2.cpp/xml_pugixml.cpp/
	// xml_rapidxml.cpp. game_xml.cpp falls back to its own XsdLiteValidator
	// (xsd_lite.h) for those three instead - see game_xml.cpp.
	class XmlDocument
	{
	public:
		virtual ~XmlDocument() = default;

		virtual bool load(const std::string& filename) = 0;
		virtual std::string getErrorMessage() const = 0;
		virtual std::unique_ptr<XmlNode> getRootElement() const = 0;
	};

	enum class XmlBackend
	{
		Xerces,
		TinyXml2,
		PugiXml,
		RapidXml
	};

	// How strongly a document's contents were checked against
	// xgedef.xsd, for game_xml.cpp to report back through
	// Game::printGame() - see game_xml.cpp for how each value gets decided,
	// and xsd_lite.h for what "Weak" actually checks.
	enum class SchemaValidation
	{
		None,   // no xsi:noNamespaceSchemaLocation on the document at all - nothing to validate against
		Weak,   // checked by this project's own XsdLiteValidator (see xsd_lite.h) - every backend but Xerces
		Strong  // checked by Xerces's real XSD engine (see xml_xerces.cpp)
	};

	std::ostream& operator<<(std::ostream& o, SchemaValidation validation);

	// Builds the concrete XmlDocument for the given backend - the one place
	// that knows about all four (see WindowFactory in window.h for the exact
	// same pattern on the graphics side). Adding a fifth backend only needs a
	// branch added here.
	//
	// As for windows (window.h), a program is built with some of the XML
	// libraries, not necessarily all of them (options.cmake: Xerces alone by
	// default, every one when the tests are built), and available() says which.
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
