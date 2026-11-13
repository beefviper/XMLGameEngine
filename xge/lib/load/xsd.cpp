// xsd.cpp
// XML Game Engine
// author: beefviper
// date: Sept 29, 2026

#include "load/xsd.h"

#include "xml/document.h"

#include <algorithm>
#include <cctype>
#include <charconv>
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <map>
#include <optional>
#include <set>
#include <vector>

namespace xge
{
	namespace
	{
		bool isXmlSpace(char c) { return c == ' ' || c == '\t' || c == '\n' || c == '\r'; }

		// XSD's whiteSpace="collapse": tabs and line ends become spaces, runs
		// of spaces become one, and none is left at either end. Every
		// built-in type but xs:string (preserve) and xs:normalizedString
		// (replace) does this to a value before reading it.
		std::string collapsed(const std::string& value)
		{
			std::string out;
			bool space = false;
			for (const char c : value)
			{
				if (isXmlSpace(c)) { space = !out.empty(); continue; }
				if (space) { out += ' '; space = false; }
				out += c;
			}
			return out;
		}

		// whiteSpace="replace": each tab and line end becomes a space.
		std::string replaced(std::string value)
		{
			for (char& c : value) { if (isXmlSpace(c)) { c = ' '; } }
			return value;
		}

		// How many characters (not bytes) a UTF-8 string holds, as the
		// length facets count them.
		std::size_t characters(const std::string& value)
		{
			return static_cast<std::size_t>(std::count_if(value.begin(), value.end(), [](char c) { return (static_cast<unsigned char>(c) & 0xC0) != 0x80; }));
		}

		bool allDigits(const std::string& text, std::size_t from, std::size_t to)
		{
			if (from >= to) { return false; }
			for (std::size_t i = from; i < to; ++i)
			{
				if (!std::isdigit(static_cast<unsigned char>(text[i]))) { return false; }
			}
			return true;
		}

		// xs:integer's lexical form: an optional sign, then digits, of any length.
		bool integerLexical(const std::string& value)
		{
			const std::size_t start = !value.empty() && (value[0] == '+' || value[0] == '-') ? 1 : 0;
			return allDigits(value, start, value.size());
		}

		// Two xs:integer values compared exactly, whatever their size.
		int compareIntegers(const std::string& a, const std::string& b)
		{
			const auto split = [](const std::string& v)
			{
				const bool negative = !v.empty() && v[0] == '-';
				std::size_t at = !v.empty() && (v[0] == '+' || v[0] == '-') ? 1 : 0;
				while (at + 1 < v.size() && v[at] == '0') { ++at; }
				std::string digits = v.substr(at);
				return std::pair<bool, std::string>{ negative && digits != "0", digits };
			};
			const auto [aNegative, aDigits] = split(a);
			const auto [bNegative, bDigits] = split(b);
			if (aNegative != bNegative) { return aNegative ? -1 : 1; }
			int magnitude = aDigits.size() != bDigits.size() ? (aDigits.size() < bDigits.size() ? -1 : 1) : aDigits.compare(bDigits);
			magnitude = magnitude < 0 ? -1 : magnitude > 0 ? 1 : 0;
			return aNegative ? -magnitude : magnitude;
		}

		// xs:decimal: an optional sign, digits with at most one point among
		// or around them, at least one digit; no exponent.
		bool decimalLexical(const std::string& value)
		{
			std::size_t at = !value.empty() && (value[0] == '+' || value[0] == '-') ? 1 : 0;
			const std::size_t point = value.find('.', at);
			if (point == std::string::npos) { return allDigits(value, at, value.size()); }
			const bool before = point > at && allDigits(value, at, point);
			const bool after = point + 1 < value.size() && allDigits(value, point + 1, value.size());
			return (before || after) && (point == at || before) && (point + 1 == value.size() || after);
		}

		// xs:float and xs:double: a decimal with an optional exponent, or INF, -INF, NaN.
		bool floatLexical(const std::string& value)
		{
			if (value == "INF" || value == "-INF" || value == "NaN") { return true; }
			const std::size_t e = value.find_first_of("eE");
			if (e == std::string::npos) { return decimalLexical(value); }
			return decimalLexical(value.substr(0, e)) && integerLexical(value.substr(e + 1));
		}

		// The integer types by their bounds ("" for none), as XSD sets them.
		struct IntegerBounds
		{
			const char* name;
			const char* low;
			const char* high;
		};
		constexpr IntegerBounds integerTypes[] = {
			{ "integer", "", "" },
			{ "nonNegativeInteger", "0", "" },
			{ "positiveInteger", "1", "" },
			{ "nonPositiveInteger", "", "0" },
			{ "negativeInteger", "", "-1" },
			{ "long", "-9223372036854775808", "9223372036854775807" },
			{ "int", "-2147483648", "2147483647" },
			{ "short", "-32768", "32767" },
			{ "byte", "-128", "127" },
			{ "unsignedLong", "0", "18446744073709551615" },
			{ "unsignedInt", "0", "4294967295" },
			{ "unsignedShort", "0", "65535" },
			{ "unsignedByte", "0", "255" },
		};

		const IntegerBounds* integerType(const std::string& name)
		{
			for (const IntegerBounds& type : integerTypes)
			{
				if (name == type.name) { return &type; }
			}
			return nullptr;
		}

		// The built-in types this validator reads; any other stops the schema loading.
		bool isBuiltin(const std::string& name)
		{
			return name == "string" || name == "normalizedString" || name == "token" || name == "NMTOKEN" || name == "boolean"
				|| name == "decimal" || name == "float" || name == "double" || integerType(name) != nullptr;
		}

		// A value of a built-in type, once its white space has been dealt with.
		bool builtinMatches(const std::string& type, const std::string& value)
		{
			if (type == "string" || type == "normalizedString") { return true; }
			// A token is collapsed already, so anything left is one.
			if (type == "token") { return true; }
			if (type == "boolean") { return value == "true" || value == "false" || value == "1" || value == "0"; }
			if (type == "decimal") { return decimalLexical(value); }
			if (type == "float" || type == "double") { return floatLexical(value); }
			// One name, nothing between: letters, digits and . - _ : (a key, a
			// set); any character past ASCII is taken as a letter.
			if (type == "NMTOKEN")
			{
				return !value.empty() && std::all_of(value.begin(), value.end(), [](unsigned char c)
					{ return std::isalnum(c) != 0 || c == '.' || c == '-' || c == '_' || c == ':' || c >= 0x80; });
			}
			if (const IntegerBounds* bounds = integerType(type))
			{
				if (!integerLexical(value)) { return false; }
				if (*bounds->low && compareIntegers(value, bounds->low) < 0) { return false; }
				if (*bounds->high && compareIntegers(value, bounds->high) > 0) { return false; }
				return true;
			}
			return false;
		}

		// A number as a facet compares it (the value has passed its type already).
		long double numberOf(const std::string& value)
		{
			if (value == "INF") { return HUGE_VALL; }
			if (value == "-INF") { return -HUGE_VALL; }
			return std::strtold(value.c_str(), nullptr);
		}

		// What each schema construct may carry, and so what this validator
		// reads; anything else stops the schema loading (checkSchema).
		const std::map<std::string, std::set<std::string>>& schemaAttributes()
		{
			static const std::map<std::string, std::set<std::string>> known = {
				{ "schema", { "attributeFormDefault", "elementFormDefault" } },
				{ "annotation", {} },
				{ "documentation", { "source", "xml:lang" } },
				{ "appinfo", { "source" } },
				{ "element", { "name", "type", "minOccurs", "maxOccurs" } },
				{ "attribute", { "name", "type", "use" } },
				{ "complexType", { "name", "mixed" } },
				{ "simpleType", { "name" } },
				{ "simpleContent", {} },
				{ "complexContent", { "mixed" } },
				{ "extension", { "base" } },
				{ "restriction", { "base" } },
				{ "sequence", { "minOccurs", "maxOccurs" } },
				{ "choice", { "minOccurs", "maxOccurs" } },
				{ "all", { "minOccurs", "maxOccurs" } },
				{ "group", { "name", "ref", "minOccurs", "maxOccurs" } },
				{ "attributeGroup", { "name", "ref" } },
				{ "enumeration", { "value" } },
				{ "length", { "value" } },
				{ "minLength", { "value" } },
				{ "maxLength", { "value" } },
				{ "minInclusive", { "value" } },
				{ "maxInclusive", { "value" } },
				{ "minExclusive", { "value" } },
				{ "maxExclusive", { "value" } },
			};
			return known;
		}

		// Strips any "prefix:" off a qualified name (e.g. "xs:element" ->
		// "element") - every backend's getName()/getAttribute() hands back
		// the raw qualified name as written (see xml/document.h), and this
		// schema only ever uses one prefix ("xs"), but comparing against
		// the local name rather than assuming that exact prefix string
		// costs nothing and is one less thing to break if it ever changes.
		std::string localName(const std::string& qualifiedName)
		{
			const std::size_t colon = qualifiedName.find(':');
			return colon == std::string::npos ? qualifiedName : qualifiedName.substr(colon + 1);
		}

		std::string trim(const std::string& text)
		{
			const auto first = text.find_first_not_of(" \t\r\n");
			if (first == std::string::npos) { return {}; }
			const auto last = text.find_last_not_of(" \t\r\n");
			return text.substr(first, last - first + 1);
		}

		// minOccurs/maxOccurs as written: absent is 1, "unbounded" is -1.
		int occurs(const XmlNode& node, const char* attribute)
		{
			const std::string value = node.getAttribute(attribute);
			if (value.empty()) { return 1; }
			if (value == "unbounded") { return -1; }
			int number = 0;
			std::from_chars(value.data(), value.data() + value.size(), number);
			return number;
		}
	}

	// ------------------------------------------------------------ the model
	struct XsdLiteValidator::Model
	{
		struct Attribute
		{
			std::string name;
			std::string type;
			bool required = false;
		};

		// A restriction of a type: its base (a built-in or another of the
		// schema's) and the facets that narrow it.
		struct SimpleType
		{
			std::string base;
			std::vector<std::string> enumeration; // empty: any value the base allows
			std::optional<std::string> minInclusive, maxInclusive, minExclusive, maxExclusive;
			std::optional<std::size_t> length, minLength, maxLength;
		};

		struct ComplexType;

		// One item of a content model: an element, or a sequence/choice of
		// other particles, and how many times it may repeat.
		struct Particle
		{
			enum class Kind { Element, Sequence, Choice, All };

			Kind kind = Kind::Element;
			int minOccurs = 1;
			int maxOccurs = 1;

			// Element:
			std::string name;
			std::shared_ptr<ComplexType> complexType; // null: a simple element (text only) ...
			std::string simpleType;                   // ... of this type
			bool anyType = false;                     // no type at all: anything goes (xs:anyType)

			// Sequence / Choice / All:
			std::vector<Particle> children;
		};

		struct ComplexType
		{
			bool mixed = false;
			std::string textType; // xs:simpleContent: its text is of this type, and it holds no elements
			std::vector<Attribute> attributes;
			std::unique_ptr<Particle> content; // null: no child elements allowed
		};

		Particle root;
		std::map<std::string, SimpleType> simpleTypes;

		// The top-level named pieces of the schema, kept as XML until something
		// refers to them (so the order they are written in does not matter),
		// and what has been built from them.
		std::map<std::string, std::unique_ptr<XmlNode>> complexTypeNodes;
		std::map<std::string, std::unique_ptr<XmlNode>> groupNodes;
		std::map<std::string, std::unique_ptr<XmlNode>> attributeGroupNodes;
		std::map<std::string, std::shared_ptr<ComplexType>> complexTypes;
		std::map<std::string, Particle> groups;
		std::set<std::string> inProgress;
		// Named types reached while a group is being built, made once the
		// root is: a group may hold an element of a type that holds the group
		// again (a <trigger>'s <otherwise> holds commands, <trigger> among them).
		std::vector<std::shared_ptr<ComplexType>> deferred;
		std::vector<std::string> deferredNames;

		// Types that contain themselves point at each other through shared
		// pointers; emptying every named type lets them all go.
		~Model()
		{
			for (auto& [name, type] : complexTypes) { type->content.reset(); }
		}

		// -------------------------------------------------------- building
		bool build(const XmlNode& schemaRoot, std::string& error);
		bool checkSchema(const XmlNode& node, std::string& error) const;
		bool buildSimpleType(const XmlNode& node, const std::string& name, std::string& error);
		bool knownType(const std::string& name) const;
		std::string inlineType(const XmlNode& holder, std::string& error);
		int inlineTypes = 0;
		std::string rootBuiltin(const std::string& type) const;
		bool valueMatches(const std::string& type, const std::string& value) const;
		bool buildElement(const XmlNode& node, Particle& out, std::string& error);
		bool buildComplexType(const XmlNode& node, ComplexType& out, std::string& error);
		bool extendComplexType(const XmlNode& extension, ComplexType& out, std::string& error);
		bool buildContent(const XmlNode& node, Particle& out, std::string& error);
		bool resolveComplexType(const std::string& name, std::shared_ptr<ComplexType>& out, std::string& error);
		bool resolveGroup(const std::string& name, Particle& out, std::string& error);

		// ------------------------------------------------------ validating
		using Children = std::vector<std::unique_ptr<XmlNode>>;

		// value against a declared type (e.g. "xs:boolean", or the name of one
		// of the schema's own xs:simpleTypes) - see the class comment in
		// load/xsd.h for how unrecognized types are handled.
		bool typeMatches(const std::string& type, const std::string& value) const;

		bool validateElement(const Particle& declaration, const XmlNode& actual, const std::string& path, std::string& error) const;
		bool validateAttributes(const ComplexType& type, const XmlNode& actual, const std::string& path, std::string& error) const;

		// What matching a content model against an element's children found:
		// the children matched so far up to `position`, and, when it did not
		// match, either a real error inside a child that was there
		// (`hardError`), or just what it was still waiting for (`expected`).
		// Optional parts are tried and put back, so `position` says little about
		// where a mismatch really was; `failPosition` and `failExpected` keep the
		// furthest child at which something the content model wanted was not
		// there, which is what a message should point at.
		struct MatchState
		{
			std::size_t position = 0;
			bool hardError = false;
			std::string error;
			std::string expected;
			std::size_t failPosition = 0;
			std::string failExpected;

			void noteFailure(std::size_t at, const std::string& wanted)
			{
				if (at >= failPosition) { failPosition = at; failExpected = wanted; }
			}
		};

		bool matchParticle(const Particle& particle, const Children& children, MatchState& state, const std::string& path) const;

		// The names of the elements a particle could start with, for "expected
		// one of ..." in a message.
		static void startingNames(const Particle& particle, std::vector<std::string>& names);
		bool matchOnce(const Particle& particle, const Children& children, MatchState& state, const std::string& path, int occurrence) const;
	};

	// Every construct of the schema is one this validator reads, with only
	// the attributes it reads, and holds no text but in its documentation:
	// anything else would be passed over, and games then judged on a
	// schema other than the one written.
	bool XsdLiteValidator::Model::checkSchema(const XmlNode& node, std::string& error) const
	{
		const std::string kind = localName(node.getName());
		const auto known = schemaAttributes().find(kind);
		if (known == schemaAttributes().end())
		{
			error = "unsupported schema construct xs:" + kind + lineNote(node.line());
			return false;
		}
		if (kind == "documentation" || kind == "appinfo") { return true; }
		for (const auto& [name, value] : node.getAttributes())
		{
			if (name == "xmlns" || name.rfind("xmlns:", 0) == 0) { continue; }
			if (!known->second.count(name))
			{
				error = "xs:" + kind + " has " + name + "=, which this validator does not read" + lineNote(node.line());
				return false;
			}
			if (name == "minOccurs" || name == "maxOccurs")
			{
				if (!(integerLexical(value) && value[0] != '-' && value[0] != '+') && !(name == "maxOccurs" && value == "unbounded"))
				{
					error = "xs:" + kind + " " + name + "=\"" + value + "\" is not a number of times" + lineNote(node.line());
					return false;
				}
			}
			if (name == "use" && value != "required" && value != "optional")
			{
				error = "xs:attribute use=\"" + value + "\" is not read; only required and optional" + lineNote(node.line());
				return false;
			}
			if (name == "mixed" && value != "true" && value != "false")
			{
				error = "xs:" + kind + " mixed=\"" + value + "\" is not true or false" + lineNote(node.line());
				return false;
			}
		}
		if (!trim(node.getText()).empty())
		{
			error = "xs:" + kind + " holds the text \"" + trim(node.getText()) + "\"; only xs:documentation and xs:appinfo hold text" + lineNote(node.line());
			return false;
		}
		const std::string minOccurs = node.getAttribute("minOccurs");
		const std::string maxOccurs = node.getAttribute("maxOccurs");
		if (!minOccurs.empty() && !maxOccurs.empty() && maxOccurs != "unbounded" && compareIntegers(minOccurs, maxOccurs) > 0)
		{
			error = "xs:" + kind + " minOccurs is above its maxOccurs" + lineNote(node.line());
			return false;
		}
		for (std::unique_ptr<XmlNode> child = node.getFirstChild(); child; child = child->getNextSibling())
		{
			if (!checkSchema(*child, error)) { return false; }
		}
		return true;
	}

	bool XsdLiteValidator::Model::knownType(const std::string& name) const
	{
		return isBuiltin(name) || simpleTypes.count(name) != 0;
	}

	// The simple type an xs:attribute or xs:element writes inside itself,
	// under a name of its own that no schema can write.
	std::string XsdLiteValidator::Model::inlineType(const XmlNode& holder, std::string& error)
	{
		for (std::unique_ptr<XmlNode> child = holder.getFirstChild(); child; child = child->getNextSibling())
		{
			if (localName(child->getName()) != "simpleType") { continue; }
			const std::string name = "#" + std::to_string(++inlineTypes);
			if (!buildSimpleType(*child, name, error)) { return {}; }
			return name;
		}
		return {};
	}

	bool XsdLiteValidator::Model::buildSimpleType(const XmlNode& node, const std::string& name, std::string& error)
	{
		if (name.empty())
		{
			error = "xs:simpleType with no 'name' attribute";
			return false;
		}
		if (simpleTypes.count(name) || complexTypeNodes.count(name))
		{
			error = "the schema has two types named '" + name + "'";
			return false;
		}

		SimpleType simpleType;
		bool restricted = false;

		for (std::unique_ptr<XmlNode> restriction = node.getFirstChild(); restriction; restriction = restriction->getNextSibling())
		{
			const std::string kind = localName(restriction->getName());
			if (kind == "annotation") { continue; }
			if (kind != "restriction" || restricted)
			{
				error = "unsupported schema construct xs:" + kind + " in xs:simpleType '" + name + "'";
				return false;
			}
			restricted = true;

			simpleType.base = localName(restriction->getAttribute("base"));
			if (simpleType.base.empty())
			{
				error = "xs:simpleType '" + name + "' restricts no base=";
				return false;
			}

			for (std::unique_ptr<XmlNode> facet = restriction->getFirstChild(); facet; facet = facet->getNextSibling())
			{
				const std::string facetKind = localName(facet->getName());
				const std::string value = facet->getAttribute("value");
				const auto size = [&](std::optional<std::size_t>& into)
				{
					if (!integerLexical(value) || value[0] == '-') { return false; }
					std::size_t number = 0;
					std::from_chars(value.data(), value.data() + value.size(), number);
					into = number;
					return true;
				};
				bool ok = true;
				if (facetKind == "annotation") { continue; }
				else if (facetKind == "enumeration") { simpleType.enumeration.push_back(value); }
				else if (facetKind == "minInclusive") { simpleType.minInclusive = value; }
				else if (facetKind == "maxInclusive") { simpleType.maxInclusive = value; }
				else if (facetKind == "minExclusive") { simpleType.minExclusive = value; }
				else if (facetKind == "maxExclusive") { simpleType.maxExclusive = value; }
				else if (facetKind == "length") { ok = size(simpleType.length); }
				else if (facetKind == "minLength") { ok = size(simpleType.minLength); }
				else if (facetKind == "maxLength") { ok = size(simpleType.maxLength); }
				else
				{
					error = "unsupported schema construct xs:" + facetKind + " in xs:simpleType '" + name + "'";
					return false;
				}
				if (!ok)
				{
					error = "xs:" + facetKind + " of xs:simpleType '" + name + "' is \"" + value + "\"; expected a number of characters";
					return false;
				}
			}
		}

		if (!restricted)
		{
			error = "xs:simpleType '" + name + "' has no xs:restriction";
			return false;
		}

		simpleTypes[name] = std::move(simpleType);
		return true;
	}

	// An xs:sequence or xs:choice, or an xs:group ref, as a Particle.
	bool XsdLiteValidator::Model::buildContent(const XmlNode& node, Particle& out, std::string& error)
	{
		const std::string kind = localName(node.getName());

		if (kind == "group")
		{
			const std::string ref = localName(node.getAttribute("ref"));
			Particle group;
			if (!resolveGroup(ref, group, error)) { return false; }

			// A reference is a sequence of the one thing the group is, repeated
			// as the reference says.
			out.kind = Particle::Kind::Sequence;
			out.minOccurs = occurs(node, "minOccurs");
			out.maxOccurs = occurs(node, "maxOccurs");
			out.children.push_back(std::move(group));
			return true;
		}

		if (kind != "sequence" && kind != "choice" && kind != "all")
		{
			error = "unsupported schema construct xs:" + kind;
			return false;
		}

		out.kind = kind == "sequence" ? Particle::Kind::Sequence : kind == "choice" ? Particle::Kind::Choice : Particle::Kind::All;
		out.minOccurs = occurs(node, "minOccurs");
		out.maxOccurs = occurs(node, "maxOccurs");

		for (std::unique_ptr<XmlNode> child = node.getFirstChild(); child; child = child->getNextSibling())
		{
			const std::string childKind = localName(child->getName());
			Particle particle;

			if (childKind == "element")
			{
				if (!buildElement(*child, particle, error)) { return false; }
			}
			else if (childKind == "sequence" || childKind == "choice" || childKind == "group")
			{
				if (!buildContent(*child, particle, error)) { return false; }
			}
			else if (childKind == "annotation")
			{
				continue;
			}
			else
			{
				error = "unsupported schema construct xs:" + childKind + " in xs:" + kind;
				return false;
			}
			if (out.kind == Particle::Kind::All && (particle.kind != Particle::Kind::Element || particle.maxOccurs != 1))
			{
				error = "an xs:all holds elements that each appear at most once";
				return false;
			}

			out.children.push_back(std::move(particle));
		}

		return true;
	}

	bool XsdLiteValidator::Model::buildComplexType(const XmlNode& node, ComplexType& out, std::string& error)
	{
		out.mixed = node.getAttribute("mixed") == "true";

		for (std::unique_ptr<XmlNode> child = node.getFirstChild(); child; child = child->getNextSibling())
		{
			const std::string kind = localName(child->getName());

			if (kind == "sequence" || kind == "choice" || kind == "all" || kind == "group")
			{
				auto content = std::make_unique<Particle>();
				if (!buildContent(*child, *content, error)) { return false; }
				out.content = std::move(content);
			}
			else if (kind == "complexContent")
			{
				// A type built on another: the base's attributes and content,
				// then the extension's own attributes and content after it.
				if (child->getAttribute("mixed") == "true") { out.mixed = true; }
				for (std::unique_ptr<XmlNode> extension = child->getFirstChild(); extension; extension = extension->getNextSibling())
				{
					const std::string extensionKind = localName(extension->getName());
					if (extensionKind == "annotation") { continue; }
					if (extensionKind != "extension")
					{
						error = "unsupported schema construct xs:" + extensionKind + " in xs:complexContent";
						return false;
					}
					if (!extendComplexType(*extension, out, error)) { return false; }
				}
			}
			else if (kind == "annotation")
			{
				continue;
			}
			else if (kind == "simpleContent")
			{
				// Text with attributes (a bitmap's <color char="w">): the
				// text is of the extension's base type, and the extension's
				// attributes are the element's.
				for (std::unique_ptr<XmlNode> extension = child->getFirstChild(); extension; extension = extension->getNextSibling())
				{
					const std::string extensionKind = localName(extension->getName());
					if (extensionKind == "annotation") { continue; }
					if (extensionKind != "extension")
					{
						error = "unsupported schema construct xs:" + extensionKind + " in xs:simpleContent";
						return false;
					}
					const std::string base = localName(extension->getAttribute("base"));
					if (!knownType(base))
					{
						error = "xs:simpleContent extends '" + base + "', which is no simple type of the schema or one this validator reads";
						return false;
					}
					if (!buildComplexType(*extension, out, error)) { return false; }
					out.textType = base;
				}
			}
			else if (kind == "attribute")
			{
				Attribute attribute;
				attribute.name = child->getAttribute("name");
				attribute.type = localName(child->getAttribute("type"));
				attribute.required = (child->getAttribute("use") == "required");

				if (attribute.name.empty())
				{
					error = "xs:attribute with no 'name' attribute";
					return false;
				}
				if (attribute.type.empty())
				{
					attribute.type = inlineType(*child, error);
					if (!error.empty()) { return false; }
					// No type at all is xs:anySimpleType: any text.
					if (attribute.type.empty()) { attribute.type = "string"; }
				}
				if (!knownType(attribute.type))
				{
					error = "xs:attribute '" + attribute.name + "' is of type '" + attribute.type + "', neither a type of the schema nor a built-in this validator reads";
					return false;
				}
				if (std::any_of(out.attributes.begin(), out.attributes.end(), [&](const Attribute& other) { return other.name == attribute.name; }))
				{
					error = "a type declares the attribute '" + attribute.name + "' twice";
					return false;
				}

				out.attributes.push_back(std::move(attribute));
			}
			else if (kind == "attributeGroup")
			{
				// Attributes several types share, written once: its xs:attributes are this type's.
				const std::string name = child->getAttribute("ref");
				const auto group = attributeGroupNodes.find(name);
				if (group == attributeGroupNodes.end())
				{
					error = "xs:attributeGroup ref='" + name + "' names no attribute group";
					return false;
				}
				const bool mixed = out.mixed;
				if (!buildComplexType(*group->second, out, error)) { return false; }
				out.mixed = mixed;
			}
			else
			{
				error = "unsupported schema construct xs:" + kind + " in xs:complexType";
				return false;
			}
		}

		return true;
	}

	bool XsdLiteValidator::Model::extendComplexType(const XmlNode& extension, ComplexType& out, std::string& error)
	{
		const std::string base = localName(extension.getAttribute("base"));
		const auto baseNode = complexTypeNodes.find(base);
		if (baseNode == complexTypeNodes.end())
		{
			error = "xs:extension base='" + base + "' names no complex type of the schema";
			return false;
		}
		if (!inProgress.insert("extension:" + base).second)
		{
			error = "the complex type '" + base + "' extends itself";
			return false;
		}
		ComplexType from;
		const bool built = buildComplexType(*baseNode->second, from, error);
		inProgress.erase("extension:" + base);
		if (!built) { return false; }

		ComplexType own;
		if (!buildComplexType(extension, own, error)) { return false; }

		out.mixed = out.mixed || from.mixed;
		out.attributes = std::move(from.attributes);
		out.attributes.insert(out.attributes.end(), own.attributes.begin(), own.attributes.end());
		if (from.content && own.content)
		{
			auto both = std::make_unique<Particle>();
			both->kind = Particle::Kind::Sequence;
			both->children.push_back(std::move(*from.content));
			both->children.push_back(std::move(*own.content));
			out.content = std::move(both);
		}
		else
		{
			out.content = from.content ? std::move(from.content) : std::move(own.content);
		}
		return true;
	}

	bool XsdLiteValidator::Model::buildElement(const XmlNode& node, Particle& out, std::string& error)
	{
		out.kind = Particle::Kind::Element;
		out.name = node.getAttribute("name");

		if (out.name.empty())
		{
			error = "xs:element with no 'name' attribute";
			return false;
		}

		out.minOccurs = occurs(node, "minOccurs");
		out.maxOccurs = occurs(node, "maxOccurs");

		const std::string type = localName(node.getAttribute("type"));

		if (!type.empty())
		{
			if (complexTypeNodes.count(type))
			{
				return resolveComplexType(type, out.complexType, error);
			}

			if (type == "anyType")
			{
				out.anyType = true;
				return true;
			}
			if (!knownType(type))
			{
				error = "xs:element '" + out.name + "' is of type '" + type + "', neither a type of the schema nor a built-in this validator reads";
				return false;
			}
			out.simpleType = type; // xs:string, xs:boolean, ... or one of simpleTypes
			return true;
		}

		for (std::unique_ptr<XmlNode> child = node.getFirstChild(); child; child = child->getNextSibling())
		{
			const std::string kind = localName(child->getName());
			if (kind == "complexType")
			{
				out.complexType = std::make_shared<ComplexType>();
				return buildComplexType(*child, *out.complexType, error);
			}
			if (kind == "simpleType")
			{
				out.simpleType = inlineType(node, error);
				return error.empty();
			}
			if (kind != "annotation")
			{
				error = "unsupported schema construct xs:" + kind + " in xs:element '" + out.name + "'";
				return false;
			}
		}

		// No type at all: xs:anyType, which takes anything.
		out.anyType = true;
		return true;
	}

	bool XsdLiteValidator::Model::resolveComplexType(const std::string& name, std::shared_ptr<ComplexType>& out, std::string& error)
	{
		if (auto built = complexTypes.find(name); built != complexTypes.end())
		{
			out = built->second;
			return true;
		}

		const auto node = complexTypeNodes.find(name);
		if (node == complexTypeNodes.end())
		{
			error = "the schema refers to a type '" + name + "' it does not define";
			return false;
		}

		// A type may contain itself, through an element (or elements of other
		// types) that name it: it is recorded before it is built, so that the
		// reference inside it finds the type being made. (An operation's
		// operands hold operations whose operands are operations.) The cycle
		// is broken again when the model goes (~Model).
		auto type = std::make_shared<ComplexType>();
		complexTypes[name] = type;

		if (!inProgress.empty())
		{
			deferred.push_back(type);
			deferredNames.push_back(name);
			out = type;
			return true;
		}

		if (!buildComplexType(*node->second, *type, error))
		{
			complexTypes.erase(name);
			return false;
		}

		out = type;
		return true;
	}

	bool XsdLiteValidator::Model::resolveGroup(const std::string& name, Particle& out, std::string& error)
	{
		if (auto built = groups.find(name); built != groups.end())
		{
			out = built->second;
			return true;
		}

		const auto node = groupNodes.find(name);
		if (node == groupNodes.end())
		{
			error = "the schema refers to a group '" + name + "' it does not define";
			return false;
		}

		if (!inProgress.insert("group:" + name).second)
		{
			error = "the schema group '" + name + "' contains itself, which this validator does not support";
			return false;
		}

		bool ok = false;
		for (std::unique_ptr<XmlNode> child = node->second->getFirstChild(); child; child = child->getNextSibling())
		{
			const std::string kind = localName(child->getName());
			if (kind == "sequence" || kind == "choice")
			{
				Particle group;
				ok = buildContent(*child, group, error);
				if (ok) { groups[name] = group; out = std::move(group); }
				break;
			}
		}

		inProgress.erase("group:" + name);
		if (!ok && error.empty()) { error = "the schema group '" + name + "' has no xs:sequence or xs:choice"; }
		return ok;
	}

	bool XsdLiteValidator::Model::build(const XmlNode& schemaRoot, std::string& error)
	{
		std::unique_ptr<XmlNode> topElement;

		if (!checkSchema(schemaRoot, error)) { return false; }

		// Collect the named pieces first; they can then refer to each other in
		// any order.
		for (std::unique_ptr<XmlNode> top = schemaRoot.getFirstChild(); top; )
		{
			const std::string kind = localName(top->getName());
			std::unique_ptr<XmlNode> next = top->getNextSibling();

			const std::string name = top->getAttribute("name");
			const auto twice = [&](const char* what)
			{
				error = "the schema has two " + std::string(what) + " named '" + name + "'" + lineNote(top->line());
				return false;
			};
			if (kind == "simpleType")
			{
				if (!buildSimpleType(*top, name, error)) { return false; }
			}
			else if (kind == "complexType")
			{
				if (name.empty()) { error = "a top-level xs:complexType with no name" + lineNote(top->line()); return false; }
				if (complexTypeNodes.count(name) || simpleTypes.count(name)) { return twice("types"); }
				complexTypeNodes[name] = std::move(top);
			}
			else if (kind == "group")
			{
				if (groupNodes.count(name)) { return twice("groups"); }
				groupNodes[name] = std::move(top);
			}
			else if (kind == "attributeGroup")
			{
				if (attributeGroupNodes.count(name)) { return twice("attribute groups"); }
				attributeGroupNodes[name] = std::move(top);
			}
			else if (kind == "element")
			{
				// A game file has one root; a second top-level element would be
				// another document this validator never looks at.
				if (topElement) { error = "the schema has more than one top-level xs:element" + lineNote(top->line()); return false; }
				topElement = std::move(top);
			}
			else if (kind != "annotation")
			{
				error = "unsupported schema construct xs:" + kind + " at the top of the schema" + lineNote(top->line());
				return false;
			}

			top = std::move(next);
		}

		if (!topElement)
		{
			error = "the schema has no top-level xs:element";
			return false;
		}

		for (const auto& [name, simpleType] : simpleTypes)
		{
			if (!knownType(simpleType.base))
			{
				error = "xs:simpleType '" + name + "' restricts '" + simpleType.base + "', neither a type of the schema nor a built-in this validator reads";
				return false;
			}
		}

		if (!buildElement(*topElement, root, error)) { return false; }

		while (!deferred.empty())
		{
			const std::shared_ptr<ComplexType> type = deferred.back();
			const std::string name = deferredNames.back();
			deferred.pop_back();
			deferredNames.pop_back();
			if (!buildComplexType(*complexTypeNodes.at(name), *type, error)) { return false; }
		}
		return true;
	}

	// ----------------------------------------------------------- validating
	void XsdLiteValidator::Model::startingNames(const Particle& particle, std::vector<std::string>& names)
	{
		if (particle.kind == Particle::Kind::Element)
		{
			names.push_back("<" + particle.name + ">");
			return;
		}

		for (const Particle& part : particle.children)
		{
			startingNames(part, names);

			// In a sequence, only the first part that has to be there can start it.
			if (particle.kind == Particle::Kind::Sequence && part.minOccurs > 0) { break; }
		}
	}

	// The built-in a type comes down to, through the schema's restrictions.
	std::string XsdLiteValidator::Model::rootBuiltin(const std::string& type) const
	{
		std::string at = localName(type);
		for (int depth = 0; depth < 64; ++depth)
		{
			const auto simple = simpleTypes.find(at);
			if (simple == simpleTypes.end()) { return at; }
			at = simple->second.base;
		}
		return at;
	}

	bool XsdLiteValidator::Model::typeMatches(const std::string& type, const std::string& value) const
	{
		// White space first, by the built-in the type comes down to.
		const std::string builtin = rootBuiltin(type);
		const std::string normal = builtin == "string" ? value : builtin == "normalizedString" ? replaced(value) : collapsed(value);
		return valueMatches(localName(type), normal);
	}

	bool XsdLiteValidator::Model::valueMatches(const std::string& type, const std::string& value) const
	{
		const auto simple = simpleTypes.find(type);
		if (simple == simpleTypes.end()) { return builtinMatches(type, value); }

		const SimpleType& restriction = simple->second;
		if (!valueMatches(restriction.base, value)) { return false; }

		const auto& enumeration = restriction.enumeration;
		if (!enumeration.empty() && std::find(enumeration.begin(), enumeration.end(), value) == enumeration.end()) { return false; }

		const std::size_t size = characters(value);
		if (restriction.length && size != *restriction.length) { return false; }
		if (restriction.minLength && size < *restriction.minLength) { return false; }
		if (restriction.maxLength && size > *restriction.maxLength) { return false; }

		if (restriction.minInclusive || restriction.maxInclusive || restriction.minExclusive || restriction.maxExclusive)
		{
			const long double number = numberOf(value);
			if (std::isnan(number)) { return false; }
			if (restriction.minInclusive && number < numberOf(*restriction.minInclusive)) { return false; }
			if (restriction.maxInclusive && number > numberOf(*restriction.maxInclusive)) { return false; }
			if (restriction.minExclusive && number <= numberOf(*restriction.minExclusive)) { return false; }
			if (restriction.maxExclusive && number >= numberOf(*restriction.maxExclusive)) { return false; }
		}
		return true;
	}

	namespace
	{
		// The attributes any element may carry: namespace declarations and the
		// schema instance's own (xsi:noNamespaceSchemaLocation).
		bool alwaysAllowed(const std::string& name)
		{
			return name == "xmlns" || name.rfind("xmlns:", 0) == 0 || name.rfind("xsi:", 0) == 0;
		}
	}

	bool XsdLiteValidator::Model::validateAttributes(const ComplexType& type, const XmlNode& actual, const std::string& path, std::string& error) const
	{
		const std::vector<std::pair<std::string, std::string>> present = actual.getAttributes();

		for (const Attribute& attribute : type.attributes)
		{
			const auto found = std::find_if(present.begin(), present.end(), [&](const auto& each) { return each.first == attribute.name; });

			if (found == present.end())
			{
				if (attribute.required)
				{
					error = path + ": missing required attribute '" + attribute.name + "'" + lineNote(actual.line());
					return false;
				}

				continue;
			}

			if (!typeMatches(attribute.type, found->second))
			{
				error = path + ": attribute '" + attribute.name + "' = \"" + found->second + "\" is not a valid " + (isBuiltin(attribute.type) ? "xs:" : "") + attribute.type + lineNote(actual.line());
				return false;
			}
		}

		for (const auto& [name, value] : present)
		{
			if (alwaysAllowed(name)) { continue; }
			if (std::none_of(type.attributes.begin(), type.attributes.end(), [&](const Attribute& attribute) { return attribute.name == name; }))
			{
				error = path + ": attribute '" + name + "' is not declared for <" + localName(actual.getName()) + ">" + lineNote(actual.line());
				return false;
			}
		}

		return true;
	}

	bool XsdLiteValidator::Model::validateElement(const Particle& declaration, const XmlNode& actual, const std::string& path, std::string& error) const
	{
		Children children;
		for (std::unique_ptr<XmlNode> child = actual.getFirstChild(); child; )
		{
			std::unique_ptr<XmlNode> next = child->getNextSibling();
			children.push_back(std::move(child));
			child = std::move(next);
		}

		if (declaration.anyType) { return true; }

		const std::string rawText = actual.getText();
		const std::string text = trim(rawText);

		// Text only, of a simple type: of a simple element, or one of a
		// complex type with simple content (and attributes).
		const auto simpleText = [&](const std::string& simpleType)
		{
			if (!children.empty())
			{
				error = path + ": unexpected element <" + children.front()->getName() + "> inside a text-only element" + lineNote(children.front()->line());
				return false;
			}

			if (!typeMatches(simpleType, rawText))
			{
				error = path + ": \"" + text + "\" is not a valid " + (isBuiltin(simpleType) ? "xs:" : "") + simpleType + lineNote(actual.line());
				return false;
			}

			return true;
		};

		if (!declaration.complexType)
		{
			for (const auto& [name, value] : actual.getAttributes())
			{
				if (alwaysAllowed(name)) { continue; }
				error = path + ": attribute '" + name + "' is not declared for <" + localName(actual.getName()) + ">, which holds only text" + lineNote(actual.line());
				return false;
			}
			return simpleText(declaration.simpleType);
		}

		const ComplexType& type = *declaration.complexType;

		if (!validateAttributes(type, actual, path, error)) { return false; }

		if (!type.textType.empty()) { return simpleText(type.textType); }

		if (!type.mixed && !text.empty())
		{
			error = path + ": unexpected text \"" + text + "\"" + lineNote(actual.line());
			return false;
		}

		if (!type.content)
		{
			if (!children.empty())
			{
				error = path + ": unexpected element <" + children.front()->getName() + ">" + lineNote(children.front()->line());
				return false;
			}

			return true;
		}

		MatchState state;
		const bool matched = matchParticle(*type.content, children, state, path);

		if (state.hardError)
		{
			error = state.error;
			return false;
		}

		if (matched && state.position == children.size()) { return true; }

		// Where it went wrong: for a match that fell short, the furthest child
		// something was still wanted at (see MatchState); for one that matched
		// but left children over, the first one left over.
		const std::size_t at = matched ? state.position : state.failPosition;
		const std::string& expected = matched ? state.expected : state.failExpected;

		if (at < children.size())
		{
			error = path + ": unexpected element <" + children[at]->getName() + ">";
			if (!expected.empty()) { error += ", expected " + expected; }
			error += lineNote(children[at]->line());
		}
		else
		{
			error = path + ": " + (expected.empty() ? std::string("incomplete") : "expected " + expected + " but the element ends") + lineNote(actual.line());
		}

		return false;
	}

	// One pass of `particle`: an element (must be the next child), a sequence
	// (each part in turn), or a choice (the first alternative that matches).
	bool XsdLiteValidator::Model::matchOnce(const Particle& particle, const Children& children, MatchState& state, const std::string& path, int occurrence) const
	{
		if (particle.kind == Particle::Kind::Element)
		{
			if (state.position >= children.size() || localName(children[state.position]->getName()) != particle.name)
			{
				state.expected = "<" + particle.name + ">";
				state.noteFailure(state.position, state.expected);
				return false;
			}

			std::string elementPath = path + "/" + particle.name;
			if (particle.maxOccurs != 1) { elementPath += "[" + std::to_string(occurrence + 1) + "]"; }

			std::string error;
			if (!validateElement(particle, *children[state.position], elementPath, error))
			{
				state.hardError = true;
				state.error = error;
				return false;
			}

			++state.position;
			return true;
		}

		if (particle.kind == Particle::Kind::Sequence)
		{
			for (const Particle& part : particle.children)
			{
				if (!matchParticle(part, children, state, path)) { return false; }
			}

			return true;
		}

		if (particle.kind == Particle::Kind::All)
		{
			// Each of its elements once at most, in any order.
			std::vector<bool> used(particle.children.size(), false);
			while (state.position < children.size())
			{
				const std::string name = localName(children[state.position]->getName());
				std::size_t found = particle.children.size();
				for (std::size_t i = 0; i < particle.children.size(); ++i)
				{
					if (!used[i] && particle.children[i].name == name) { found = i; break; }
				}
				if (found == particle.children.size()) { break; }
				if (!matchOnce(particle.children[found], children, state, path, 0)) { return false; }
				used[found] = true;
			}
			for (std::size_t i = 0; i < particle.children.size(); ++i)
			{
				if (!used[i] && particle.children[i].minOccurs > 0)
				{
					state.expected = "<" + particle.children[i].name + ">";
					state.noteFailure(state.position, state.expected);
					return false;
				}
			}
			return true;
		}

		// Choice: the first alternative that matches, preferring one that
		// takes something over one that matches by taking nothing.
		const std::size_t start = state.position;
		bool matchedEmpty = false;

		for (const Particle& alternative : particle.children)
		{
			state.position = start;
			if (matchParticle(alternative, children, state, path))
			{
				if (state.position > start) { return true; }
				matchedEmpty = true;
			}
			else if (state.hardError)
			{
				return false;
			}
		}

		state.position = start;

		if (!matchedEmpty)
		{
			std::vector<std::string> names;
			startingNames(particle, names);

			state.expected = "one of";
			for (std::size_t i = 0; i < names.size() && i < 6; ++i) { state.expected += (i ? ", " : " ") + names[i]; }
			if (names.size() > 6) { state.expected += ", ..."; }
			state.noteFailure(start, state.expected);
		}

		return matchedEmpty;
	}

	// `particle`, repeated as often as its minOccurs/maxOccurs allow.
	bool XsdLiteValidator::Model::matchParticle(const Particle& particle, const Children& children, MatchState& state, const std::string& path) const
	{
		int count = 0;

		while (particle.maxOccurs == -1 || count < particle.maxOccurs)
		{
			const std::size_t before = state.position;

			if (!matchOnce(particle, children, state, path, count))
			{
				if (state.hardError) { return false; }

				// Whatever was tried and did not fit is put back; if it was
				// only an optional repeat, that is fine.
				state.position = before;
				break;
			}

			++count;
			if (state.position == before) { break; } // matched, but took nothing
		}

		if (count < particle.minOccurs)
		{
			return false;
		}

		state.expected.clear();
		return true;
	}

	// ------------------------------------------------------------ the class
	XsdLiteValidator::XsdLiteValidator() = default;
	XsdLiteValidator::~XsdLiteValidator() = default;

	bool XsdLiteValidator::loadSchema(const std::string& schemaFilename, XmlBackend backend)
	{
		std::unique_ptr<XmlDocument> schemaDocument = XmlDocumentFactory::create(backend);

		if (!schemaDocument->load(schemaFilename))
		{
			errorMessage = "failed to parse schema file '" + schemaFilename + "': " + schemaDocument->getErrorMessage();
			return false;
		}

		std::unique_ptr<XmlNode> schemaRoot = schemaDocument->getRootElement();

		if (!schemaRoot || localName(schemaRoot->getName()) != "schema")
		{
			errorMessage = "'" + schemaFilename + "' is not an xs:schema document";
			return false;
		}

		model = std::make_unique<Model>();

		if (!model->build(*schemaRoot, errorMessage))
		{
			errorMessage = "'" + schemaFilename + "': " + errorMessage;
			model.reset();
			return false;
		}

		return true;
	}

	bool XsdLiteValidator::validate(const XmlNode& root) const
	{
		errorMessage.clear();

		if (!model)
		{
			errorMessage = "no schema loaded";
			return false;
		}

		if (localName(root.getName()) != model->root.name)
		{
			errorMessage = "expected root element <" + model->root.name + ">, found <" + root.getName() + ">";
			return false;
		}

		return model->validateElement(model->root, root, model->root.name, errorMessage);
	}

	std::string XsdLiteValidator::getErrorMessage() const
	{
		return errorMessage;
	}
}
