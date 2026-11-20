// xsd.h
// XML Game Engine
// author: beefviper
// date: Sept 29, 2026

#pragma once

#include "xml/document.h"

#include <cstddef>
#include <memory>
#include <string>

namespace xge
{
	// The "weak" validator (see load/xml.cpp): a hand-rolled interpreter for
	// the subset of XSD that xgedef.xsd and xgedump.xsd use, held to what XSD
	// says and Xerces does within it:
	//
	//   * xs:element (name, type, minOccurs, maxOccurs), with an inline
	//     xs:complexType or xs:simpleType, a type= naming one of the schema's
	//     types or a built-in, or no type at all (xs:anyType: anything goes);
	//   * xs:complexType (named or inline; mixed) whose content is xs:sequence
	//     and xs:choice of elements, nested, each with minOccurs/maxOccurs,
	//     xs:all of elements each once at most, and xs:group ref= to a named
	//     xs:group of the same (a named complexType may contain itself,
	//     through its elements, as an operation's operands do; a group may
	//     not);
	//   * xs:complexContent > xs:extension base= (the base's attributes and
	//     content, then its own), and xs:simpleContent > xs:extension (text of
	//     a simple type, with attributes);
	//   * xs:attribute (name, type or an inline xs:simpleType, use required or
	//     optional) and xs:attributeGroup ref=; an attribute a document's
	//     element has that its type does not declare is refused (xmlns and
	//     xsi: ones excepted), as is any attribute on a text-only element;
	//   * xs:annotation anywhere, read past;
	//   * xs:simpleType > xs:restriction of a built-in or of another simple
	//     type, with xs:enumeration, xs:length, xs:minLength, xs:maxLength (in
	//     characters) and xs:min/maxInclusive, xs:min/maxExclusive;
	//   * the built-ins string, normalizedString, token, NMTOKEN, boolean,
	//     decimal, float, double, and integer with its thirteen kinds (long,
	//     int, unsignedByte, positiveInteger...), bounds checked at any size;
	//     white space is kept, replaced or collapsed as the type's built-in
	//     says before a value is judged.
	//
	// A schema it cannot read wholly stops loading, naming what: a construct,
	// a type or facet outside that list, an attribute on a schema part it does
	// not read (default=, ref= on an element...), text outside
	// xs:documentation, two types of one name, a second top-level element.
	// So a game is never refused for something this validator merely does not
	// know. Not checked: XSD's schema-wide rules (Unique Particle Attribution,
	// Element Declarations Consistent), which Xerces skips too without full
	// checking. test_schema_checker.cpp keeps it in step with Xerces.
	//
	// Only Xerces checks a schema itself; with the other libraries this
	// checks the same .xsd, read as XML through XmlNode like everything else.
	class XsdLiteValidator
	{
	public:
		XsdLiteValidator();
		~XsdLiteValidator();

		XsdLiteValidator(const XsdLiteValidator&) = delete;
		XsdLiteValidator& operator=(const XsdLiteValidator&) = delete;

		// Parses schemaFilename (any well-formed XML file, read with the
		// given backend) and builds this validator's in-memory model of it.
		// False + getErrorMessage() if the file fails to parse, is not an
		// xs:schema with one top-level xs:element, or holds anything this
		// validator does not read or a schema may not say (above).
		bool loadSchema(const std::string& schemaFilename, XmlBackend backend);

		// Validates root - the root element of an already-parsed game
		// document - against the schema loaded above. False +
		// getErrorMessage() on the first mismatch found.
		bool validate(const XmlNode& root) const;

		std::string getErrorMessage() const;

	private:
		struct Model;

		std::unique_ptr<Model> model;
		mutable std::string errorMessage;
	};

	// Checks `root` against the schema at `schemaPath`, read with `backend`.
	// The schema is parsed once for the program, as it outlives every game
	// that names it, and again only when its file changes. False, with `why`,
	// if the schema cannot be read or `root` does not match it.
	bool validateWithKeptSchema(const XmlNode& root, const std::string& schemaPath, XmlBackend backend, std::string& why);

	// How many times a schema file has been parsed, for a test.
	std::size_t schemaReads() noexcept;
}
