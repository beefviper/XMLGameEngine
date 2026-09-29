// xsd_lite.h
// XML Game Engine
// author: beefviper
// date: Sept 29, 2026

#pragma once

#include "xml_document.h"

#include <string>
#include <vector>

namespace xge
{
	// The "weak" validator (see game_xml.cpp): a hand-rolled interpreter for
	// the small subset of XSD assets/xmlgameengine.xsd actually uses -
	// xs:schema > xs:element > xs:complexType > (xs:sequence of nested
	// xs:element, each with an optional minOccurs/maxOccurs) plus
	// xs:attribute (name/type/use) on any complexType - and nothing else (no
	// xs:choice, xs:import/include, xs:restriction, keys...). Exists only
	// because Xerces is the one backend able to run *real* schema validation
	// (see xml_xerces.cpp); when the game loads through one of the other
	// three, this runs instead, against the very same .xsd file, read
	// through the same abstract XmlNode/XmlDocument interface as everything
	// else here - an XSD file is just XML, so no dedicated XSD parser is
	// needed to walk it.
	//
	// Deliberately permissive about anything outside that subset: an
	// xs:attribute type this validator doesn't specifically recognize
	// (typeMatches) passes unchecked rather than being rejected, so it can
	// only ever report a real structural mismatch (wrong/missing/extra
	// element, a missing required attribute, or a value that doesn't fit a
	// type it *does* know) - never a false failure on a schema construct it
	// simply doesn't implement. That gap is exactly what "weak" means in
	// practice, as opposed to Xerces's "strong": a pass here is real
	// evidence the document matches this subset of the schema, but there's
	// no guarantee every constraint the real schema expresses was checked.
	class XsdLiteValidator
	{
	public:
		// Parses schemaFilename (any well-formed XML file, read with the
		// given backend) and builds this validator's in-memory model of it.
		// False + getErrorMessage() only if the file itself fails to parse,
		// or doesn't look like xs:schema > xs:element > ... at all -
		// assets/xmlgameengine.xsd is expected to always succeed here.
		bool loadSchema(const std::string& schemaFilename, XmlBackend backend);

		// Validates root - the root element of an already-parsed game
		// document - against the schema loaded above. False +
		// getErrorMessage() on the first mismatch found.
		bool validate(const XmlNode& root) const;

		std::string getErrorMessage() const;

	private:
		struct Attribute
		{
			std::string name;
			std::string type;
			bool required = false;
		};

		// One xs:element - its own required/optional attributes plus,
		// recursively, its own xs:sequence of nested elements (in schema
		// order - see validateSequence). minOccurs/maxOccurs default to 1
		// each per the XSD spec; maxOccurs of -1 means "unbounded".
		struct Element
		{
			std::string name;
			int minOccurs = 1;
			int maxOccurs = 1;
			std::vector<Attribute> attributes;
			std::vector<Element> children;
		};

		// Strips any "prefix:" off a qualified name (e.g. "xs:element" ->
		// "element") - every backend's getName()/getAttribute() hands back
		// the raw qualified name as written (see xml_document.h), and this
		// schema only ever uses one prefix ("xs"), but comparing against
		// the local name rather than assuming that exact prefix string
		// costs nothing and is one less thing to break if it ever changes.
		static std::string localName(const std::string& qualifiedName);

		static bool parseElement(const XmlNode& xsdElement, Element& outElement, std::string& error);
		static bool parseComplexType(const XmlNode& complexType, Element& outElement, std::string& error);

		// value against an xs:attribute's declared type (e.g. "xs:boolean")
		// - see the class comment for how unrecognized types are handled.
		static bool typeMatches(const std::string& type, const std::string& value);

		bool validateAttributes(const Element& element, const XmlNode& actual, const std::string& path) const;

		// Walks actual's children (starting at cursor, which this function
		// takes ownership of) against expected, in order - xs:sequence
		// semantics: each expected element must appear, consecutively, at
		// least minOccurs and at most maxOccurs times before the next
		// expected element can start, and nothing may be left over once
		// every expected element has been matched.
		bool validateSequence(const std::vector<Element>& expected, std::unique_ptr<XmlNode> cursor, const std::string& path) const;

		Element rootElement;
		mutable std::string errorMessage;
	};
}
