// test_schema_checker.cpp
// XML Game Engine
// author: beefviper
// date: Nov 13, 2026
//
// Catch2 tests that keep the engine's own schema checker (XsdLiteValidator,
// load/xsd.h) in step with Xerces. Each case is a small schema and documents
// against it, each marked good or bad; Xerces's real validation and the light
// checker, reading the schema and the document through each of the other three
// XML libraries, must all give that verdict. Then schemas the light checker
// must refuse to load, naming what it does not read.

#include "scratch.h"
#include "load/xsd.h"
#include "xml/document.h"

#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_string.hpp>

#include <algorithm>
#include <cctype>
#include <filesystem>
#include <fstream>
#include <functional>
#include <iterator>
#include <optional>
#include <random>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

using namespace xge;
using Catch::Matchers::ContainsSubstring;

namespace
{
	// Removes a scratch file when it goes out of scope, pass or fail.
	struct ScratchFile
	{
		std::filesystem::path path;

		explicit ScratchFile(std::filesystem::path where) : path(std::move(where)) {}
		ScratchFile(const ScratchFile&) = delete;
		ScratchFile& operator=(const ScratchFile&) = delete;
		~ScratchFile()
		{
			std::error_code ignored;
			std::filesystem::remove(path, ignored);
		}
	};

	struct Document
	{
		std::string body; // the root element's attributes after its name, then its content
		bool good;
	};

	const char* const schemaStart = "<xs:schema xmlns:xs=\"http://www.w3.org/2001/XMLSchema\">\n";
	const char* const schemaEnd = "</xs:schema>\n";

	// Writes the schema and each document (root <r>, pointing at the schema
	// the way a game does), and checks every validator gives each its verdict.
	void agree(const std::string& schemaBody, const std::vector<Document>& documents)
	{
		const ScratchFile schema{ scratchName("descriptions/checker.xsd") };
		std::ofstream(schema.path) << schemaStart << schemaBody << schemaEnd;
		const std::string location = schema.path.filename().string();

		for (const Document& document : documents)
		{
			const ScratchFile file{ scratchName("descriptions/checker.xml") };
			std::ofstream(file.path) << "<r xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\" xsi:noNamespaceSchemaLocation=\""
				<< location << "\"" << document.body << "</r>\n";

			INFO("document: <r" << document.body << "</r>");
			if (XmlDocumentFactory::available(XmlBackend::Xerces))
			{
				auto strong = XmlDocumentFactory::create(XmlBackend::Xerces);
				INFO("Xerces");
				CHECK(strong->load(file.path.string()) == document.good);
			}
			for (const XmlBackend backend : { XmlBackend::TinyXml2, XmlBackend::PugiXml, XmlBackend::RapidXml })
			{
				if (!XmlDocumentFactory::available(backend)) { continue; }
				INFO("light checker through " << XmlDocumentFactory::name(backend));
				XsdLiteValidator validator;
				REQUIRE(validator.loadSchema(schema.path.string(), backend));
				auto weak = XmlDocumentFactory::create(backend);
				REQUIRE(weak->load(file.path.string()));
				const bool accepts = validator.validate(*weak->getRootElement());
				INFO(validator.getErrorMessage());
				CHECK(accepts == document.good);
			}
		}
	}

	// One element of each built-in type under <r>, so a value is judged by it.
	std::string builtinsSchema(const std::vector<std::string>& types)
	{
		std::string choice = "<xs:element name=\"r\"><xs:complexType><xs:choice>";
		for (const std::string& type : types) { choice += "<xs:element name=\"" + type + "\" type=\"xs:" + type + "\" />"; }
		return choice + "</xs:choice></xs:complexType></xs:element>\n";
	}
}

TEST_CASE("the light schema checker refuses an attribute the schema does not declare, as Xerces does", "[schema][checker]")
{
	agree(
		"<xs:element name=\"r\"><xs:complexType><xs:sequence>"
		"<xs:element name=\"box\" maxOccurs=\"unbounded\"><xs:complexType>"
		"<xs:attribute name=\"name\" type=\"xs:NMTOKEN\" />"
		"<xs:attribute name=\"size\" type=\"xs:int\" use=\"required\" />"
		"</xs:complexType></xs:element>"
		"</xs:sequence></xs:complexType></xs:element>\n",
		{
			{ "><box name=\"a\" size=\"3\" /><box size=\"4\" />", true },
			{ "><box name=\"a\" size=\"3\" colour=\"red\" />", false },
			{ " extra=\"1\"><box size=\"3\" />", false },
			{ "><box name=\"a\" />", false },
			// Present but empty is still checked, and an int's spaces are collapsed.
			{ "><box size=\"\" />", false },
			{ "><box name=\"\" size=\"3\" />", false },
			{ "><box size=\" 12 \" />", true },
			{ "><box size=\"2147483648\" />", false },
		});
}

TEST_CASE("the light schema checker refuses an attribute on an element that holds only a value", "[schema][checker]")
{
	agree(
		"<xs:element name=\"r\"><xs:complexType><xs:sequence>"
		"<xs:element name=\"n\" type=\"xs:integer\" />"
		"</xs:sequence></xs:complexType></xs:element>\n",
		{
			{ "><n>3</n>", true },
			{ "><n unit=\"px\">3</n>", false },
			{ "><n><m /></n>", false },
		});
}

TEST_CASE("the light schema checker reads an attribute's own inline type, and a text with attributes", "[schema][checker]")
{
	agree(
		"<xs:element name=\"r\"><xs:complexType><xs:sequence>"
		"<xs:element name=\"v\" maxOccurs=\"unbounded\"><xs:complexType><xs:simpleContent>"
		"<xs:extension base=\"xs:float\">"
		"<xs:attribute name=\"total\"><xs:simpleType><xs:restriction base=\"xs:token\">"
		"<xs:enumeration value=\"all\" /><xs:enumeration value=\"any\" />"
		"</xs:restriction></xs:simpleType></xs:attribute>"
		"</xs:extension>"
		"</xs:simpleContent></xs:complexType></xs:element>"
		"</xs:sequence></xs:complexType></xs:element>\n",
		{
			{ "><v total=\"all\">1.5</v><v>-INF</v><v> 1e3 </v>", true },
			{ "><v total=\"some\">1.5</v>", false },
			{ "><v total=\" any \">2</v>", true },
			{ "><v>fast</v>", false },
			{ "><v>1.5</v><v other=\"x\">2</v>", false },
			{ "><v></v>", false },
		});
}

TEST_CASE("the light schema checker reads each built-in type as XSD defines it", "[schema][checker]")
{
	struct Value
	{
		const char* type;
		const char* text;
		bool good;
	};

	const Value values[] = {
		{ "string", "  any text  ", true },
		{ "string", "", true },
		{ "normalizedString", "a\tb", true },
		{ "token", "  two  words ", true },
		{ "NMTOKEN", "a-b.c_d:e", true },
		{ "NMTOKEN", " ab ", true },
		{ "NMTOKEN", "a b", false },
		{ "NMTOKEN", "", false },
		{ "NMTOKEN", "a+b", false },
		{ "boolean", "true", true },
		{ "boolean", "0", true },
		{ "boolean", " false ", true },
		{ "boolean", "TRUE", false },
		{ "boolean", "yes", false },
		{ "decimal", "1.5", true },
		{ "decimal", "-.5", true },
		{ "decimal", "+3.", true },
		{ "decimal", "007", true },
		{ "decimal", "1e3", false },
		{ "decimal", "1.2.3", false },
		{ "decimal", "", false },
		{ "float", "1e3", true },
		{ "float", "-INF", true },
		{ "float", "NaN", true },
		{ "float", ".5E-2", true },
		{ "float", "3", true },
		{ "float", "inf", false },
		{ "float", "1e", false },
		{ "float", "e3", false },
		{ "double", "-0.0", true },
		{ "double", "1.5x", false },
		{ "integer", "123456789012345678901234567890", true },
		{ "integer", "+0", true },
		{ "integer", "1.0", false },
		{ "integer", "- 1", false },
		{ "long", "9223372036854775807", true },
		{ "long", "9223372036854775808", false },
		{ "int", "-2147483648", true },
		{ "int", "2147483648", false },
		{ "short", "32767", true },
		{ "short", "32768", false },
		{ "byte", "-128", true },
		{ "byte", "-129", false },
		{ "unsignedLong", "18446744073709551615", true },
		{ "unsignedLong", "18446744073709551616", false },
		{ "unsignedInt", "4294967295", true },
		{ "unsignedInt", "4294967296", false },
		{ "unsignedShort", "65535", true },
		{ "unsignedShort", "65536", false },
		{ "unsignedByte", "0255", true },
		{ "unsignedByte", "256", false },
		{ "unsignedByte", "-1", false },
		{ "positiveInteger", "1", true },
		{ "positiveInteger", "0", false },
		{ "nonNegativeInteger", "0", true },
		{ "nonNegativeInteger", "-1", false },
		{ "nonPositiveInteger", "0", true },
		{ "nonPositiveInteger", "1", false },
		{ "negativeInteger", "-1", true },
		{ "negativeInteger", "0", false },
	};

	std::vector<std::string> types;
	for (const Value& value : values)
	{
		if (std::find(types.begin(), types.end(), value.type) == types.end()) { types.push_back(value.type); }
	}

	std::vector<Document> documents;
	for (const Value& value : values)
	{
		documents.push_back({ std::string("><") + value.type + ">" + value.text + "</" + value.type + ">", value.good });
	}
	agree(builtinsSchema(types), documents);

	// XSD gives a decimal at least one digit; Xerces lets a lone "." through,
	// so only the light checker is held to the specification here.
	const ScratchFile schema{ scratchName("descriptions/decimal.xsd") };
	std::ofstream(schema.path) << schemaStart << builtinsSchema({ "decimal" }) << schemaEnd;
	const ScratchFile file{ scratchName("descriptions/decimal.xml") };
	std::ofstream(file.path) << "<r><decimal>.</decimal></r>\n";
	XsdLiteValidator validator;
	REQUIRE(validator.loadSchema(schema.path.string(), XmlBackend::TinyXml2));
	auto document = XmlDocumentFactory::create(XmlBackend::TinyXml2);
	REQUIRE(document->load(file.path.string()));
	CHECK_FALSE(validator.validate(*document->getRootElement()));
}

TEST_CASE("the light schema checker keeps a string's spaces and collapses a token's before comparing", "[schema][checker]")
{
	agree(
		"<xs:simpleType name=\"exactType\"><xs:restriction base=\"xs:string\"><xs:enumeration value=\"left\" /></xs:restriction></xs:simpleType>\n"
		"<xs:simpleType name=\"wordType\"><xs:restriction base=\"xs:token\"><xs:enumeration value=\"two words\" /></xs:restriction></xs:simpleType>\n"
		"<xs:element name=\"r\"><xs:complexType><xs:sequence>"
		"<xs:element name=\"exact\" type=\"exactType\" minOccurs=\"0\" />"
		"<xs:element name=\"word\" type=\"wordType\" minOccurs=\"0\" />"
		"</xs:sequence></xs:complexType></xs:element>\n",
		{
			{ "><exact>left</exact>", true },
			{ "><exact> left </exact>", false },
			{ "><word>two words</word>", true },
			{ "><word>  two \n words </word>", true },
			{ "><word>twowords</word>", false },
		});
}

TEST_CASE("the light schema checker reads lengths in characters, bounds, and a type restricting another", "[schema][checker]")
{
	agree(
		"<xs:simpleType name=\"codeType\"><xs:restriction base=\"xs:string\"><xs:length value=\"3\" /></xs:restriction></xs:simpleType>\n"
		"<xs:simpleType name=\"wordType\"><xs:restriction base=\"xs:NMTOKEN\"><xs:minLength value=\"2\" /><xs:maxLength value=\"4\" /></xs:restriction></xs:simpleType>\n"
		"<xs:simpleType name=\"shareType\"><xs:restriction base=\"xs:decimal\"><xs:minExclusive value=\"0\" /><xs:maxInclusive value=\"1\" /></xs:restriction></xs:simpleType>\n"
		"<xs:simpleType name=\"countType\"><xs:restriction base=\"xs:integer\"><xs:minInclusive value=\"0\" /></xs:restriction></xs:simpleType>\n"
		"<xs:simpleType name=\"digitType\"><xs:restriction base=\"countType\"><xs:maxExclusive value=\"10\" /></xs:restriction></xs:simpleType>\n"
		"<xs:element name=\"r\"><xs:complexType><xs:choice>"
		"<xs:element name=\"code\" type=\"codeType\" />"
		"<xs:element name=\"word\" type=\"wordType\" />"
		"<xs:element name=\"share\" type=\"shareType\" />"
		"<xs:element name=\"digit\" type=\"digitType\" />"
		"</xs:choice></xs:complexType></xs:element>\n",
		{
			{ "><code>abc</code>", true },
			{ "><code>ab</code>", false },
			{ "><code>abcd</code>", false },
			{ "><code>h\xC3\xA9\xC3\xA9</code>", true },
			{ "><word>ab</word>", true },
			{ "><word>abcd</word>", true },
			{ "><word>a</word>", false },
			{ "><word>abcde</word>", false },
			{ "><share>0.5</share>", true },
			{ "><share>1</share>", true },
			{ "><share>0</share>", false },
			{ "><share>1.01</share>", false },
			{ "><digit>0</digit>", true },
			{ "><digit>9</digit>", true },
			{ "><digit>10</digit>", false },
			{ "><digit>-1</digit>", false },
			{ "><digit>1.5</digit>", false },
		});
}

TEST_CASE("the light schema checker lets an element with no type hold anything, and an empty type nothing", "[schema][checker]")
{
	agree(
		"<xs:element name=\"r\"><xs:complexType><xs:sequence>"
		"<xs:element name=\"note\" minOccurs=\"0\" />"
		"<xs:element name=\"flag\" minOccurs=\"0\"><xs:complexType /></xs:element>"
		"</xs:sequence></xs:complexType></xs:element>\n",
		{
			{ "><note a=\"b\"><x>1</x>text</note>", true },
			{ "><note />", true },
			{ "><flag />", true },
			{ "><flag>x</flag>", false },
			{ "><flag><x /></flag>", false },
			{ "><flag a=\"1\" />", false },
		});
}

TEST_CASE("the light schema checker lets text in among a mixed type's elements and nowhere else", "[schema][checker]")
{
	agree(
		"<xs:element name=\"r\"><xs:complexType><xs:sequence>"
		"<xs:element name=\"line\" minOccurs=\"0\"><xs:complexType mixed=\"true\"><xs:sequence>"
		"<xs:element name=\"b\" minOccurs=\"0\" maxOccurs=\"unbounded\"><xs:complexType /></xs:element>"
		"</xs:sequence></xs:complexType></xs:element>"
		"<xs:element name=\"list\" minOccurs=\"0\"><xs:complexType><xs:sequence>"
		"<xs:element name=\"b\" minOccurs=\"0\" maxOccurs=\"unbounded\"><xs:complexType /></xs:element>"
		"</xs:sequence></xs:complexType></xs:element>"
		"</xs:sequence></xs:complexType></xs:element>\n",
		{
			{ "><line>one <b /> two <b /></line>", true },
			{ "><list>\n  <b />\n  <b />\n</list>", true },
			{ "><list>one <b /></list>", false },
			{ "> stray <list />", false },
		});
}

TEST_CASE("the light schema checker refuses a schema that is wrong, or that it does not read, naming what", "[schema][checker]")
{
	struct Refused
	{
		const char* what;
		std::string body;
		const char* says;
	};

	const std::string root = "<xs:element name=\"r\"><xs:complexType><xs:sequence>";
	const std::string rootEnd = "</xs:sequence></xs:complexType></xs:element>\n";

	const Refused refused[] = {
		{ "text inside the schema", root + "<xs:element name=\"a\" />oops" + rootEnd, "holds the text \"oops\"" },
		{ "text at the top of the schema", "stray " + root + rootEnd, "holds the text \"stray\"" },
		{ "a default value", root + "<xs:element name=\"a\" type=\"xs:string\" default=\"x\" />" + rootEnd, "default=" },
		{ "an element taken by ref=", root + "<xs:element ref=\"a\" />" + rootEnd, "ref=" },
		{ "a prohibited attribute", "<xs:element name=\"r\"><xs:complexType><xs:attribute name=\"a\" use=\"prohibited\" /></xs:complexType></xs:element>\n", "use=\"prohibited\"" },
		{ "a type the schema does not define", root + "<xs:element name=\"a\" type=\"colourType\" />" + rootEnd, "colourType" },
		{ "a built-in type it does not read", root + "<xs:element name=\"a\" type=\"xs:date\" />" + rootEnd, "type 'date', neither a type of the schema nor a built-in" },
		{ "an attribute of a type it does not read", "<xs:element name=\"r\"><xs:complexType><xs:attribute name=\"a\" type=\"xs:anyURI\" /></xs:complexType></xs:element>\n", "anyURI" },
		{ "a restriction of a type it does not read", "<xs:simpleType name=\"t\"><xs:restriction base=\"xs:QName\" /></xs:simpleType>\n" + root + rootEnd, "restricts 'QName'" },
		{ "two types of one name", "<xs:simpleType name=\"t\"><xs:restriction base=\"xs:string\" /></xs:simpleType><xs:simpleType name=\"t\"><xs:restriction base=\"xs:string\" /></xs:simpleType>\n" + root + rootEnd, "two types named 't'" },
		{ "a pattern facet", "<xs:simpleType name=\"t\"><xs:restriction base=\"xs:string\"><xs:pattern value=\"a*\" /></xs:restriction></xs:simpleType>\n" + root + rootEnd, "xs:pattern" },
		{ "an attribute wildcard", "<xs:element name=\"r\"><xs:complexType><xs:anyAttribute /></xs:complexType></xs:element>\n", "xs:anyAttribute" },
		{ "more at least than at most", root + "<xs:element name=\"a\" minOccurs=\"2\" maxOccurs=\"1\" />" + rootEnd, "minOccurs is above its maxOccurs" },
		{ "a count that is not one", root + "<xs:element name=\"a\" maxOccurs=\"many\" />" + rootEnd, "is not a number of times" },
		{ "a second top-level element", root + rootEnd + "<xs:element name=\"s\" />\n", "more than one top-level xs:element" },
		{ "a construct it does not read at the top", "<xs:notation name=\"n\" public=\"p\" />\n" + root + rootEnd, "xs:notation" },
		{ "a length that is not a number", "<xs:simpleType name=\"t\"><xs:restriction base=\"xs:string\"><xs:length value=\"x\" /></xs:restriction></xs:simpleType>\n" + root + rootEnd, "number of characters" },
	};

	for (const Refused& schema : refused)
	{
		DYNAMIC_SECTION(schema.what)
		{
			const ScratchFile file{ scratchName("descriptions/refused.xsd") };
			std::ofstream(file.path) << schemaStart << schema.body << schemaEnd;
			for (const XmlBackend backend : { XmlBackend::TinyXml2, XmlBackend::PugiXml, XmlBackend::RapidXml })
			{
				if (!XmlDocumentFactory::available(backend)) { continue; }
				INFO("through " << XmlDocumentFactory::name(backend));
				XsdLiteValidator validator;
				CHECK_FALSE(validator.loadSchema(file.path.string(), backend));
				CHECK_THAT(validator.getErrorMessage(), ContainsSubstring(schema.says));
			}
		}
	}
}

TEST_CASE("the light schema checker reads xgedef.xsd and xgedump.xsd through every XML library", "[schema][checker]")
{
	for (const std::string& schema : { std::string("xgedef.xsd"), std::string("xgedump.xsd") })
	{
		if (!std::filesystem::exists(schema)) { continue; }
		for (const XmlBackend backend : { XmlBackend::TinyXml2, XmlBackend::PugiXml, XmlBackend::RapidXml })
		{
			if (!XmlDocumentFactory::available(backend)) { continue; }
			INFO(schema << " through " << XmlDocumentFactory::name(backend));
			XsdLiteValidator validator;
			CHECK(validator.loadSchema(schema, backend));
			INFO(validator.getErrorMessage());
		}
	}
}

namespace
{
	// A start tag in a game's text: where it is and what it is called.
	struct Tag
	{
		std::size_t start; // the '<'
		std::size_t end; // one past the '>'
		std::string name;
		bool empty; // written <name ... />
	};

	std::vector<Tag> tagsOf(const std::string& text)
	{
		std::vector<Tag> tags;
		for (std::size_t at = text.find('<'); at != std::string::npos; at = text.find('<', at + 1))
		{
			if (text.compare(at, 4, "<!--") == 0)
			{
				at = text.find("-->", at);
				if (at == std::string::npos) { break; }
				continue;
			}
			if (at + 1 >= text.size() || !std::isalpha(static_cast<unsigned char>(text[at + 1]))) { continue; }
			const std::size_t close = text.find('>', at);
			if (close == std::string::npos) { break; }
			std::size_t nameEnd = at + 1;
			while (nameEnd < close && !std::isspace(static_cast<unsigned char>(text[nameEnd])) && text[nameEnd] != '/') { ++nameEnd; }
			tags.push_back({ at, close + 1, text.substr(at + 1, nameEnd - at - 1), text[close - 1] == '/' });
		}
		return tags;
	}

	// One change to a game: what was done, and the text it makes.
	struct Mutant
	{
		std::string what;
		std::string text;
	};

	using Mutate = std::optional<Mutant> (*)(const std::string&, const std::vector<Tag>&, std::mt19937&);

	const Tag* pickTag(const std::vector<Tag>& tags, std::mt19937& pick, bool emptyOnly)
	{
		std::vector<const Tag*> fit;
		// The root (the first tag) is left alone: every change is inside it.
		for (std::size_t i = 1; i < tags.size(); ++i) { if (!emptyOnly || tags[i].empty) { fit.push_back(&tags[i]); } }
		if (fit.empty()) { return nullptr; }
		return fit[pick() % fit.size()];
	}

	std::string lineAt(const std::string& text, std::size_t at)
	{
		return std::to_string(1 + std::count(text.begin(), text.begin() + static_cast<std::ptrdiff_t>(at), '\n'));
	}

	std::optional<Mutant> addAttribute(const std::string& text, const std::vector<Tag>& tags, std::mt19937& pick)
	{
		const Tag* tag = pickTag(tags, pick, false);
		if (!tag) { return std::nullopt; }
		std::string out = text;
		out.insert(tag->start + 1 + tag->name.size(), " bogus=\"1\"");
		return Mutant{ "bogus= on <" + tag->name + "> at line " + lineAt(text, tag->start), out };
	}

	std::optional<Mutant> removeElement(const std::string& text, const std::vector<Tag>& tags, std::mt19937& pick)
	{
		const Tag* tag = pickTag(tags, pick, true);
		if (!tag) { return std::nullopt; }
		std::string out = text;
		out.erase(tag->start, tag->end - tag->start);
		return Mutant{ "<" + tag->name + "> at line " + lineAt(text, tag->start) + " taken out", out };
	}

	std::optional<Mutant> doubleElement(const std::string& text, const std::vector<Tag>& tags, std::mt19937& pick)
	{
		const Tag* tag = pickTag(tags, pick, true);
		if (!tag) { return std::nullopt; }
		std::string out = text;
		out.insert(tag->end, text.substr(tag->start, tag->end - tag->start));
		return Mutant{ "<" + tag->name + "> at line " + lineAt(text, tag->start) + " written twice", out };
	}

	std::optional<Mutant> renameElement(const std::string& text, const std::vector<Tag>& tags, std::mt19937& pick)
	{
		const Tag* tag = pickTag(tags, pick, true);
		const Tag* other = pickTag(tags, pick, false);
		if (!tag || !other || other->name == tag->name) { return std::nullopt; }
		std::string out = text;
		out.replace(tag->start + 1, tag->name.size(), other->name);
		return Mutant{ "<" + tag->name + "> at line " + lineAt(text, tag->start) + " renamed <" + other->name + ">", out };
	}

	std::optional<Mutant> moveElement(const std::string& text, const std::vector<Tag>& tags, std::mt19937& pick)
	{
		const Tag* tag = pickTag(tags, pick, true);
		const Tag* after = pickTag(tags, pick, false);
		if (!tag || !after || after == tag) { return std::nullopt; }
		const std::string moved = text.substr(tag->start, tag->end - tag->start);
		std::string out = text;
		// Put in first when it goes later, so the cut is still where it was.
		if (after->end > tag->start)
		{
			out.insert(after->end, moved);
			out.erase(tag->start, moved.size());
		}
		else
		{
			out.erase(tag->start, moved.size());
			out.insert(after->end, moved);
		}
		return Mutant{ "<" + tag->name + "> at line " + lineAt(text, tag->start) + " moved after <" + after->name + "> at line " + lineAt(text, after->start), out };
	}

	std::optional<Mutant> changeAttribute(const std::string& text, const std::vector<Tag>& tags, std::mt19937& pick)
	{
		const Tag* tag = pickTag(tags, pick, false);
		if (!tag) { return std::nullopt; }
		const std::size_t quote = text.find('"', tag->start);
		if (quote == std::string::npos || quote >= tag->end) { return std::nullopt; }
		const std::size_t closing = text.find('"', quote + 1);
		static const char* const values[] = { "two words", "", "-1", "0", "1.5", "x" };
		const std::string value = values[pick() % std::size(values)];
		std::string out = text;
		out.replace(quote + 1, closing - quote - 1, value);
		return Mutant{ "first attribute of <" + tag->name + "> at line " + lineAt(text, tag->start) + " set to \"" + value + "\"", out };
	}

	std::optional<Mutant> changeText(const std::string& text, const std::vector<Tag>& tags, std::mt19937& pick)
	{
		// A tag holding only text, <x>text</x>.
		std::vector<const Tag*> leaves;
		for (const Tag& tag : tags)
		{
			if (tag.empty) { continue; }
			const std::size_t next = text.find('<', tag.end);
			if (next != std::string::npos && text.compare(next, 2 + tag.name.size(), "</" + tag.name) == 0) { leaves.push_back(&tag); }
		}
		if (leaves.empty()) { return std::nullopt; }
		const Tag* leaf = leaves[pick() % leaves.size()];
		static const char* const values[] = { "two words", "", "-1", "0", "1.5", "nonsense", "70000" };
		const std::string value = values[pick() % std::size(values)];
		std::string out = text;
		out.replace(leaf->end, text.find('<', leaf->end) - leaf->end, value);
		return Mutant{ "text of <" + leaf->name + "> at line " + lineAt(text, leaf->start) + " set to \"" + value + "\"", out };
	}

	std::vector<std::string> shippedGames()
	{
		std::vector<std::string> games;
		for (const auto& entry : std::filesystem::directory_iterator("descriptions"))
		{
			const std::string file = entry.path().generic_string();
			if (entry.path().extension() == ".xml" && file.find("_scratch") == std::string::npos) { games.push_back(file); }
		}
		std::sort(games.begin(), games.end());
		return games;
	}

	// Every shipped game, changed a few times by `mutate` (the same changes
	// every run): the light checker and Xerces must give each the same verdict.
	void sweep(Mutate mutate, unsigned seed)
	{
		REQUIRE(XmlDocumentFactory::available(XmlBackend::Xerces));
		XsdLiteValidator validator;
		REQUIRE(validator.loadSchema("xgedef.xsd", XmlBackend::TinyXml2));

		const ScratchFile file{ scratchName("descriptions/sweep.xml") };
		int refused = 0;
		int judged = 0;
		for (const std::string& game : shippedGames())
		{
			std::ifstream in(game);
			std::ostringstream read;
			read << in.rdbuf();
			const std::string text = read.str();
			const std::vector<Tag> tags = tagsOf(text);
			std::mt19937 pick(seed + static_cast<unsigned>(std::hash<std::string>{}(game) & 0xFFFFu));

			for (int round = 0; round < 3; ++round)
			{
				const std::optional<Mutant> mutant = mutate(text, tags, pick);
				if (!mutant) { continue; }
				std::ofstream(file.path, std::ios::binary) << mutant->text;

				auto strong = XmlDocumentFactory::create(XmlBackend::Xerces);
				const bool strongAccepts = strong->load(file.path.string());
				auto weak = XmlDocumentFactory::create(XmlBackend::TinyXml2);
				if (!weak->load(file.path.string())) { continue; } // no longer well formed: nothing to compare
				const bool weakAccepts = validator.validate(*weak->getRootElement());

				INFO(game << ": " << mutant->what);
				INFO("light checker: " << (weakAccepts ? std::string("accepts") : validator.getErrorMessage()));
				CHECK(weakAccepts == strongAccepts);
				++judged;
				if (!strongAccepts) { ++refused; }
			}
		}
		// The sweep means something only if it made real mistakes.
		CHECK(judged > 0);
		CHECK(refused > 0);
	}
}

TEST_CASE("the light schema checker and Xerces agree on every game given an undeclared attribute", "[schema][checker][sweep]")
{
	sweep(addAttribute, 1);
}

TEST_CASE("the light schema checker and Xerces agree on every game with an element taken out", "[schema][checker][sweep]")
{
	sweep(removeElement, 2);
}

TEST_CASE("the light schema checker and Xerces agree on every game with an element written twice", "[schema][checker][sweep]")
{
	sweep(doubleElement, 3);
}

TEST_CASE("the light schema checker and Xerces agree on every game with an element renamed", "[schema][checker][sweep]")
{
	sweep(renameElement, 4);
}

TEST_CASE("the light schema checker and Xerces agree on every game with an element moved", "[schema][checker][sweep]")
{
	sweep(moveElement, 5);
}

TEST_CASE("the light schema checker and Xerces agree on every game with an attribute changed", "[schema][checker][sweep]")
{
	sweep(changeAttribute, 6);
}

TEST_CASE("the light schema checker and Xerces agree on every game with a value changed", "[schema][checker][sweep]")
{
	sweep(changeText, 7);
}
