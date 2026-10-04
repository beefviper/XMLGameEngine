// xsd_lite.h
// XML Game Engine
// author: beefviper
// date: Sept 29, 2026

#pragma once

#include "xml_document.h"

#include <memory>
#include <string>

namespace xge
{
	// The "weak" validator (see game_xml.cpp): a hand-rolled interpreter for
	// the subset of XSD xgedef.xsd actually uses, and nothing
	// else:
	//
	//   * xs:element (name, type, minOccurs, maxOccurs), with either an inline
	//     xs:complexType or a type= naming one of the schema's own named types
	//     or a simple type;
	//   * xs:complexType (named or inline; mixed) whose content is xs:sequence
	//     and xs:choice of elements, nested, each with minOccurs/maxOccurs, and
	//     xs:group ref= to a named xs:group of the same (a named complexType
	//     may contain itself, through its elements, as a <formula>'s operands
	//     do; a group may not);
	//   * xs:attribute (name/type/use) on any complexType;
	//   * xs:simpleType > xs:restriction > xs:enumeration, and the built-ins
	//     xs:string, xs:boolean, xs:integer, xs:unsignedByte, xs:unsignedShort.
	//
	// No xs:import/include, xs:extension, xs:simpleContent, xs:all, keys...
	// Exists only because Xerces is the one backend able to run *real* schema
	// validation (see xml_xerces.cpp); when the game loads through one of the
	// other three, this runs instead, against the very same .xsd file, read
	// through the same abstract XmlNode/XmlDocument interface as everything
	// else here - an XSD file is just XML, so no dedicated XSD parser is
	// needed to walk it.
	//
	// Deliberately permissive about anything outside that subset: a type this
	// validator doesn't specifically recognize passes unchecked rather than
	// being rejected, so it can only ever report a real structural mismatch
	// (wrong/missing/extra element, unexpected text, a missing required
	// attribute, or a value that doesn't fit a type it *does* know) - never a
	// false failure on a schema construct it simply doesn't implement. It also
	// can't notice an attribute the schema doesn't declare, since XmlNode has
	// no way to list them. That gap is exactly what "weak" means in practice, as
	// opposed to Xerces's "strong": a pass here is real evidence the document
	// matches this subset of the schema, but there's no guarantee every
	// constraint the real schema expresses was checked.
	class XsdLiteValidator
	{
	public:
		XsdLiteValidator();
		~XsdLiteValidator();

		XsdLiteValidator(const XsdLiteValidator&) = delete;
		XsdLiteValidator& operator=(const XsdLiteValidator&) = delete;

		// Parses schemaFilename (any well-formed XML file, read with the
		// given backend) and builds this validator's in-memory model of it.
		// False + getErrorMessage() only if the file itself fails to parse,
		// or doesn't look like an xs:schema with a top-level xs:element at all
		// (xgedef.xsd is expected to always succeed here), or
		// names a type or group it doesn't define.
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
}
