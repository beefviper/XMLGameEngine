// test_equations.cpp
// XML Game Engine
// author: beefviper
// date: Oct 4, 2026
//
// Catch2 tests for the two ways to write arithmetic as tags instead of as text:
// an <equation> (steps, each one operation on two names or numbers, a step able
// to name its answer for the ones after it) and a <formula> (one operation whose
// operands are themselves numbers, operations included). Both are value tags,
// like <random>, so the text form (exprtk) still works beside them, and all
// three must give the same number. Then what each mistake says, the printed
// form, a divisor of 0 (a load error, and a warning instead once the game is
// running), both schema checkers against the same good and bad files, and the two
// shipped games that are written with them (Pong's title with an <equation>,
// Breakout's with <formula>s), against real xge::Games.
//
// Small games are written to a scratch file and loaded by a real xge::Game;
// none of it needs a window.

#include "command.h"
#include "game.h"
#include "xml_document.h"
#include "xsd_lite.h"

#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_string.hpp>

#include <algorithm>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <memory>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

using namespace xge;
using Catch::Matchers::ContainsSubstring;

namespace
{
	// Removes a scratch game file when it goes out of scope, pass or fail.
	struct ScratchFile
	{
		std::filesystem::path path;

		// Moved, never copied: a copy left behind by returning one from a function
		// (which Debug builds do not elide) would delete the file it was written to.
		explicit ScratchFile(std::filesystem::path where) : path(std::move(where)) {}
		ScratchFile(const ScratchFile&) = delete;
		ScratchFile& operator=(const ScratchFile&) = delete;
		ScratchFile(ScratchFile&& other) noexcept : path(std::move(other.path)) { other.path.clear(); }
		~ScratchFile()
		{
			if (path.empty()) { return; }
			std::error_code ignored;
			std::filesystem::remove(path, ignored);
		}
	};

	ScratchFile writeScratch(const std::string& name, const std::string& xml)
	{
		ScratchFile scratch{ std::filesystem::temp_directory_path() / name };
		std::ofstream out(scratch.path);
		out << xml;
		return scratch;
	}

	// A game with one object, `o`, whose position is x and y (each the inner
	// XML of its element, so a value tag can be given), and the given global
	// variables. The window is 800 by 600, so window.width.center is 400.
	std::string gameXml(const std::string& x, const std::string& y = "0", const std::string& variables = {})
	{
		return "<game>"
			"<window name=\"test\"><width>800</width><height>600</height><background>color.black</background>"
			"<fullscreen>false</fullscreen><framerate>60</framerate></window>"
			"<variables>" + variables + "</variables>"
			"<objects><object name=\"o\"><sprite><circle><radius>5</radius></circle></sprite>"
			"<position><x>" + x + "</x><y>" + y + "</y></position>"
			"<velocity><x>0</x><y>0</y></velocity>"
			"<collisions><enabled>false</enabled></collisions></object></objects>"
			"<states><state name=\"playing\"><shows><show object=\"o\" /></shows>"
			"<inputs><input button=\"space\"><pop /></input></inputs></state></states>"
			"</game>";
	}

	struct Loaded
	{
		ScratchFile file;
		Game game;

		explicit Loaded(const std::string& xml) :
			file(writeScratch("xge_equations_game.xml", xml)),
			game(file.path.string())
		{
		}

		float x() { return game.getObject("o").position.x; }
		float y() { return game.getObject("o").position.y; }
	};

	float xOf(const std::string& x, const std::string& variables = {})
	{
		return Loaded(gameXml(x, "0", variables)).x();
	}

	// The same arithmetic in the three spellings, for the tests that want to
	// see them agree. Window width is 800, w is 100 and xoffset is 7.
	const std::string kVariables = "<variable name=\"w\">100</variable><variable name=\"xoffset\">7</variable>";

	const std::string kText = "window.width.center - w / 2 - xoffset";

	const std::string kEquation =
		"<equation>"
		"<divide name=\"half\" dividend=\"w\" divisor=\"2\" />"
		"<subtract name=\"centered\" minuend=\"window.width.center\" subtrahend=\"half\" />"
		"<subtract minuend=\"centered\" subtrahend=\"xoffset\" />"
		"</equation>";

	const std::string kFormula =
		"<formula><subtract>"
		"<minuend>window.width.center</minuend>"
		"<subtrahend><divide><dividend>w</dividend><divisor>2</divisor></divide></subtrahend>"
		"<subtrahend>xoffset</subtrahend>"
		"</subtract></formula>";
}

// --------------------------------------------------------------- the forms
TEST_CASE("text, an <equation> and a <formula> give the same number", "[equations]")
{
	CHECK(xOf(kText, kVariables) == 343.0f);
	CHECK(xOf(kEquation, kVariables) == 343.0f);
	CHECK(xOf(kFormula, kVariables) == 343.0f);
}

TEST_CASE("each operation works in both forms", "[equations]")
{
	struct Case
	{
		const char* tag;
		const char* first;
		const char* rest;
		float expected;
	};

	// 12 and 4 give 16, 8, 48 and 3.
	const Case cases[] = {
		{ "add", "augend", "addend", 16.0f },
		{ "subtract", "minuend", "subtrahend", 8.0f },
		{ "multiply", "multiplicand", "multiplier", 48.0f },
		{ "divide", "dividend", "divisor", 3.0f },
	};

	for (const Case& c : cases)
	{
		DYNAMIC_SECTION(c.tag)
		{
			const std::string tag = c.tag;
			const std::string first = c.first;
			const std::string rest = c.rest;

			CHECK(xOf("<equation><" + tag + " " + first + "=\"12\" " + rest + "=\"4\" /></equation>") == c.expected);
			CHECK(xOf("<formula><" + tag + "><" + first + ">12</" + first + "><" + rest + ">4</" + rest + "></" + tag + "></formula>") == c.expected);
		}
	}
}

TEST_CASE("a formula takes the first operand, then each of the others in turn", "[equations]")
{
	const auto formula = [](const std::string& tag, const std::string& first, const std::string& rest, const std::vector<std::string>& numbers)
	{
		std::string xml = "<formula><" + tag + "><" + first + ">" + numbers.front() + "</" + first + ">";
		for (std::size_t i = 1; i < numbers.size(); ++i) { xml += "<" + rest + ">" + numbers[i] + "</" + rest + ">"; }
		return xml + "</" + tag + "></formula>";
	};

	CHECK(xOf(formula("subtract", "minuend", "subtrahend", { "100", "10", "20", "30" })) == 40.0f);
	CHECK(xOf(formula("divide", "dividend", "divisor", { "1000", "10", "5" })) == 20.0f);
	CHECK(xOf(formula("add", "augend", "addend", { "1", "2", "3" })) == 6.0f);
	CHECK(xOf(formula("multiply", "multiplicand", "multiplier", { "2", "3", "4" })) == 24.0f);
}

TEST_CASE("operations nest as deep as they are written, and a <random> is an operand", "[equations]")
{
	// (((6 + 4) * 3) - (20 / 5)) + 1 = 27
	CHECK(xOf(
		"<formula><add>"
		"<augend><subtract>"
		"<minuend><multiply><multiplicand><add><augend>6</augend><addend>4</addend></add></multiplicand><multiplier>3</multiplier></multiply></minuend>"
		"<subtrahend><divide><dividend>20</dividend><divisor>5</divisor></divide></subtrahend>"
		"</subtract></augend>"
		"<addend>1</addend>"
		"</add></formula>") == 27.0f);

	CHECK(xOf("<formula><add><augend><random min=\"5\" max=\"5\" /></augend><addend>1</addend></add></formula>") == 6.0f);
}

TEST_CASE("an equation's steps use the names of the steps before them, and the last step is the answer", "[equations]")
{
	// a = 2 + 3, b = a * a, then b - a
	CHECK(xOf(
		"<equation>"
		"<add name=\"a\" augend=\"2\" addend=\"3\" />"
		"<multiply name=\"b\" multiplicand=\"a\" multiplier=\"a\" />"
		"<subtract minuend=\"b\" subtrahend=\"a\" />"
		"</equation>") == 20.0f);

	// A named last step is still the answer.
	CHECK(xOf("<equation><add name=\"only\" augend=\"1\" addend=\"1\" /></equation>") == 2.0f);
}

TEST_CASE("a step's name is its own, and goes before a variable of the same name", "[equations]")
{
	// The variable w is 100; the step called w is 3 + 4 for the steps after it.
	CHECK(xOf(
		"<equation><add name=\"w\" augend=\"3\" addend=\"4\" /><multiply multiplicand=\"w\" multiplier=\"2\" /></equation>",
		kVariables) == 14.0f);
}

TEST_CASE("variables can hold an equation or a formula, and an expression can use them", "[equations]")
{
	Loaded loaded(gameXml(
		"r * 2 + 1", "s",
		kVariables +
		"<variable name=\"r\">" + kFormula + "</variable>"
		"<variable name=\"s\">" + kEquation + "</variable>"));

	CHECK(loaded.game.getVariable("r") == 343.0f);
	CHECK(loaded.game.getVariable("s") == 343.0f);
	CHECK(loaded.x() == 687.0f);
	CHECK(loaded.y() == 343.0f);
}

TEST_CASE("an operation follows the order its operands are written in", "[equations]")
{
	// Not commutative: 10 - 4 is not 4 - 10, and 10 / 4 is not 4 / 10.
	CHECK(xOf("<equation><subtract minuend=\"10\" subtrahend=\"4\" /></equation>") == 6.0f);
	CHECK(xOf("<equation><subtract minuend=\"4\" subtrahend=\"10\" /></equation>") == -6.0f);
	CHECK(xOf("<formula><divide><dividend>10</dividend><divisor>4</divisor></divide></formula>") == 2.5f);
	CHECK(xOf("<formula><divide><dividend>4</dividend><divisor>10</divisor></divide></formula>") == 0.4f);
}

TEST_CASE("a negative number and a decimal are numbers", "[equations]")
{
	CHECK(xOf("<equation><add augend=\"-110\" addend=\"0.5\" /></equation>") == -109.5f);
	CHECK(xOf("<formula><add><augend>-110</augend><addend>1e2</addend></add></formula>") == -10.0f);
}

// --------------------------------------------------------------- the print
TEST_CASE("a value prints as infix with its brackets", "[equations]")
{
	const auto operand = [](const std::string& text) { RawOperand o; o.value = RawValue::expression(text); return o; };

	RawOperation divide;
	divide.op = "divide";
	divide.operands = { operand("title.width"), operand("2") };

	RawValue inner;
	inner.kind = RawValue::Kind::Formula;
	inner.operations = std::make_shared<std::vector<RawOperation>>(1, divide);

	RawOperation subtract;
	subtract.op = "subtract";
	subtract.operands = { operand("window.width.center") };
	RawOperand nested;
	nested.value = inner;
	subtract.operands.push_back(nested);

	RawValue formula;
	formula.kind = RawValue::Kind::Formula;
	formula.operations = std::make_shared<std::vector<RawOperation>>(1, subtract);

	CHECK(valueText(formula) == "(window.width.center - (title.width / 2))");

	divide.name = "half";
	subtract.operands = { operand("window.width.center"), operand("half") };
	RawValue equation;
	equation.kind = RawValue::Kind::Equation;
	equation.operations = std::make_shared<std::vector<RawOperation>>(std::vector<RawOperation>{ divide, subtract });

	CHECK(valueText(equation) == "equation{ half = (title.width / 2); (window.width.center - half) }");
	CHECK(valueText(RawValue::expression("a + b")) == "a + b");
}

// --------------------------------------------------------------- mistakes
TEST_CASE("an equation that is wrong says which step and what", "[equations][errors]")
{
	struct Mistake
	{
		const char* what;
		std::string value;
		const char* says;
		const char* where = "object 'o' > <position> > <x>";
	};

	const Mistake mistakes[] = {
		{ "no steps", "<equation></equation>", "has no steps" },
		{ "a tag that is not a step", "<equation><modulo dividend=\"7\" divisor=\"2\" /></equation>", "<modulo>, which is not a step" },
		{ "an operand that is an expression", "<equation><add augend=\"a + b\" addend=\"1\" /></equation>", "\"a + b\" is not a name or a number" },
		{ "a missing operand", "<equation><subtract minuend=\"1\" /></equation>", "<subtract> needs subtrahend=" },
		{ "operands written as elements", "<equation><add><augend>1</augend><addend>2</addend></add></equation>", "takes its operands as attributes" },
		{ "two steps with one name", "<equation><add name=\"a\" augend=\"1\" addend=\"1\" /><add name=\"a\" augend=\"2\" addend=\"2\" /></equation>", "two steps are called \"a\"" },
		{ "a name with a dot", "<equation><add name=\"a.b\" augend=\"1\" addend=\"1\" /></equation>", "is not a name" },
		{ "a name used before its step", "<equation><add augend=\"later\" addend=\"1\" /><add name=\"later\" augend=\"1\" addend=\"1\" /></equation>", "later", "object 'o'" },
		{ "text beside the tag", "5 <equation><add augend=\"1\" addend=\"1\" /></equation>", "both the text \"5\" and a <equation> tag" },
	};

	for (const Mistake& mistake : mistakes)
	{
		DYNAMIC_SECTION(mistake.what)
		{
			REQUIRE_THROWS_WITH(Loaded(gameXml(mistake.value)), ContainsSubstring(mistake.where) && ContainsSubstring(mistake.says));
		}
	}
}

TEST_CASE("a formula that is wrong says which operation and what", "[equations][errors]")
{
	struct Mistake
	{
		const char* what;
		std::string value;
		const char* says;
	};

	const Mistake mistakes[] = {
		{ "no operation", "<formula></formula>", "has no operation" },
		{ "two operations", "<formula><add><augend>1</augend><addend>1</addend></add><add><augend>1</augend><addend>1</addend></add></formula>", "more than one operation" },
		{ "a tag that is not an operation", "<formula><modulo><dividend>7</dividend><divisor>2</divisor></modulo></formula>", "<modulo>, which is not an operation" },
		{ "operands in the wrong order", "<formula><subtract><subtrahend>1</subtrahend><minuend>2</minuend></subtract></formula>", "<subtrahend> where <minuend> belongs" },
		{ "one operand", "<formula><subtract><minuend>1</minuend></subtract></formula>", "needs a <minuend> and at least one <subtrahend>" },
		{ "the first operand twice", "<formula><subtract><minuend>1</minuend><minuend>2</minuend></subtract></formula>", "<minuend> where <subtrahend> belongs" },
		{ "an expression as an operand", "<formula><add><augend>a + b</augend><addend>1</addend></add></formula>", "\"a + b\" is not a name or a number" },
		{ "an operand with text and a tag", "<formula><add><augend>2 <random min=\"1\" max=\"2\" /></augend><addend>1</addend></add></formula>", "both the text \"2\" and a <random> tag" },
		{ "an operand with a tag that is not an operation", "<formula><add><augend><dice /></augend><addend>1</addend></add></formula>", "<dice>, which is not an operation" },
		{ "an operand with nothing in it", "<formula><add><augend></augend><addend>1</addend></add></formula>", "has no value" },
		{ "a named operation", "<formula><add name=\"a\"><augend>1</augend><addend>1</addend></add></formula>", "has a name" },
		{ "an equation inside a formula", "<formula><add><augend><equation><add augend=\"1\" addend=\"1\" /></equation></augend><addend>1</addend></add></formula>", "<equation>, which is not an operation" },
	};

	for (const Mistake& mistake : mistakes)
	{
		DYNAMIC_SECTION(mistake.what)
		{
			REQUIRE_THROWS_WITH(Loaded(gameXml(mistake.value)), ContainsSubstring("object 'o' > <position> > <x>") && ContainsSubstring(mistake.says));
		}
	}
}

// ------------------------------------------------------------------ schema
namespace
{
	std::string readFile(const std::string& path)
	{
		std::ifstream in(path);
		REQUIRE(in.good());
		std::string text((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
		text.erase(std::remove(text.begin(), text.end(), '\r'), text.end());
		return text;
	}

	struct Verdict
	{
		bool weakAccepts;
		std::string weakMessage;
		bool strongAccepts;
	};

	// What the weak validator (through TinyXML2) and Xerces's real validation each
	// make of a document. The scratch file sits beside the games so that its
	// relative schema path resolves.
	Verdict judge(const std::string& xml)
	{
		const ScratchFile scratch{ "games/equations_scratch.xml" };
		{
			std::ofstream out(scratch.path);
			out << xml;
		}

		Verdict verdict{};

		auto weak = XmlDocumentFactory::create(XmlBackend::TinyXml2);
		REQUIRE(weak->load(scratch.path.string()));
		XsdLiteValidator validator;
		REQUIRE(validator.loadSchema("xgedef.xsd", XmlBackend::TinyXml2));
		verdict.weakAccepts = validator.validate(*weak->getRootElement());
		verdict.weakMessage = validator.getErrorMessage();

		auto strong = XmlDocumentFactory::create(XmlBackend::Xerces);
		verdict.strongAccepts = strong->load(scratch.path.string());

		return verdict;
	}

	// pong.xml with its title's <equation> replaced by `value` (the whole
	// <x>...</x> element's inner text).
	std::string pongTitleX(const std::string& value)
	{
		std::string xml = readFile("games/pong.xml");
		const std::string open = "<x>\n          <equation>";
		const std::string close = "</equation>\n        </x>";
		const auto from = xml.find(open);
		const auto to = xml.find(close);
		REQUIRE(from != std::string::npos);
		REQUIRE(to != std::string::npos);
		xml.replace(from, to + close.size() - from, "<x>" + value + "</x>");
		return xml;
	}
}

TEST_CASE("both schema checkers accept every spelling", "[equations][schema]")
{
	for (const std::string& value : { std::string("window.width.center - title.width / 2"), kEquation, kFormula })
	{
		// kEquation and kFormula use the variables w and xoffset, which pong does not have; the
		// schema does not look at names, only at shape.
		const Verdict verdict = judge(pongTitleX(value));
		CHECK(verdict.weakAccepts);
		CHECK(verdict.strongAccepts);
	}

	for (const char* file : { "games/pong.xml", "games/breakout.xml" })
	{
		const Verdict verdict = judge(readFile(file));
		CHECK(verdict.weakAccepts);
		CHECK(verdict.strongAccepts);
	}
}

TEST_CASE("both schema checkers turn away the same mistakes in an equation or formula", "[equations][schema]")
{
	struct Mistake
	{
		const char* what;
		std::string value;
		const char* weakSays;
	};

	const Mistake mistakes[] = {
		{ "a step with no divisor", "<equation><divide dividend=\"1\" /></equation>", "missing required attribute 'divisor'" },
		{ "a tag that is not a step", "<equation><modulo dividend=\"1\" divisor=\"2\" /></equation>", "unexpected element <modulo>" },
		{ "an empty equation", "<equation></equation>", "expected one of <add>, <subtract>, <multiply>, <divide>" },
		{ "operands out of order", "<formula><subtract><subtrahend>1</subtrahend><minuend>2</minuend></subtract></formula>", "expected <minuend>" },
		{ "a formula with no operation", "<formula></formula>", "expected one of <add>, <subtract>, <multiply>, <divide>" },
		{ "two operations in a formula", "<formula><add><augend>1</augend><addend>1</addend></add><add><augend>1</augend><addend>1</addend></add></formula>", "unexpected element <add>" },
		{ "a formula with one operand", "<formula><subtract><minuend>1</minuend></subtract></formula>", "expected <subtrahend>" },
		{ "an unknown tag deep inside", "<formula><add><augend><add><augend>1</augend><addend><dice /></addend></add></augend><addend>1</addend></add></formula>", "unexpected element <dice>" },
	};

	for (const Mistake& mistake : mistakes)
	{
		DYNAMIC_SECTION(mistake.what)
		{
			const Verdict verdict = judge(pongTitleX(mistake.value));

			CHECK_FALSE(verdict.weakAccepts);
			CHECK_THAT(verdict.weakMessage, ContainsSubstring(mistake.weakSays));
			CHECK_FALSE(verdict.strongAccepts);
		}
	}
}

// ---------------------------------------------------------- the two games
namespace
{
	void measure(Object& object, float width, float height)
	{
		object.size = { width, height };
		object.sizeKnown = true;
	}
}

TEST_CASE("Pong's title is placed by an <equation> of its own size", "[equations][games]")
{
	Game game{ "games/pong.xml" };
	Object& title = game.getObject("title");

	// Unknown until the text is measured, like the same position written as text.
	CHECK(title.positionUsesSize);
	CHECK_FALSE(title.positionResolved);

	measure(title, 600.0f, 200.0f);
	game.resolveSizeDependentPositions();

	CHECK(title.positionResolved);
	CHECK(title.position.x == 640.0f - 300.0f); // 1280 wide window, window.width.center - title.width / 2
	CHECK(title.position.y == 30.0f);

	// A different size gives a different place, as it does for text.
	measure(title, 400.0f, 200.0f);
	game.resolveSizeDependentPositions();
	CHECK(title.position.x == 640.0f - 200.0f);
}

TEST_CASE("Breakout's title is placed by <formula>s of its own size", "[equations][games]")
{
	Game game{ "games/breakout.xml" };
	Object& title = game.getObject("title");

	CHECK(title.positionUsesSize);
	CHECK_FALSE(title.positionResolved);

	measure(title, 600.0f, 200.0f);
	game.resolveSizeDependentPositions();

	CHECK(title.positionResolved);
	CHECK(title.position.x == 640.0f - 300.0f);          // window.width.center - title.width / 2
	CHECK(title.position.y == 360.0f - 100.0f - 110.0f); // window.height.center - title.height / 2 + (-110)
}

// -------------------------------------------------------- a divisor of 0
namespace
{
	// One object with the given inner XML after its collisions (variables and
	// timers), and a state that shows it.
	std::string gameWithObject(const std::string& name, const std::string& sprite, const std::string& x, const std::string& more = {})
	{
		return "<game>"
			"<window name=\"test\"><width>800</width><height>600</height><background>color.black</background>"
			"<fullscreen>false</fullscreen><framerate>60</framerate></window>"
			"<variables><variable name=\"zero\">0</variable></variables>"
			"<objects><object name=\"" + name + "\"><sprite>" + sprite + "</sprite>"
			"<position><x>" + x + "</x><y>0</y></position>"
			"<velocity><x>0</x><y>0</y></velocity>"
			"<collisions><enabled>false</enabled></collisions>" + more + "</object></objects>"
			"<states><state name=\"playing\"><shows><show object=\"" + name + "\" /></shows>"
			"<inputs><input button=\"space\"><pop /></input></inputs></state></states>"
			"</game>";
	}

	// Collects what is written to std::cerr while it lives.
	struct CaptureStderr
	{
		std::ostringstream text;
		std::streambuf* before;

		CaptureStderr() : before(std::cerr.rdbuf(text.rdbuf())) {}
		~CaptureStderr() { std::cerr.rdbuf(before); }
	};

	std::size_t countOf(const std::string& text, const std::string& part)
	{
		std::size_t count = 0;
		for (auto at = text.find(part); at != std::string::npos; at = text.find(part, at + part.size())) { ++count; }
		return count;
	}
}

TEST_CASE("a divisor of 0 stops the load, and says which divide", "[equations][errors][zero]")
{
	struct Mistake
	{
		const char* what;
		std::string value;
		const char* where;
	};

	const Mistake mistakes[] = {
		{ "a number, in an equation", "<equation><divide dividend=\"1\" divisor=\"0\" /></equation>", "<equation> > <divide>" },
		{ "a variable that is 0, in an equation", "<equation><divide dividend=\"1\" divisor=\"zero\" /></equation>", "<equation> > <divide>" },
		{ "a step that works out to 0", "<equation><subtract name=\"gone\" minuend=\"3\" subtrahend=\"3\" /><divide dividend=\"1\" divisor=\"gone\" /></equation>", "<equation> > <divide>" },
		{ "a number, in a formula", "<formula><divide><dividend>1</dividend><divisor>0</divisor></divide></formula>", "<formula> > <divide>" },
		{ "a variable that is 0, in a formula", "<formula><divide><dividend>1</dividend><divisor>zero</divisor></divide></formula>", "<formula> > <divide>" },
		{ "a later divisor of a chain", "<formula><divide><dividend>100</dividend><divisor>5</divisor><divisor>0</divisor></divide></formula>", "<formula> > <divide>" },
		{ "an operation that works out to 0, nested", "<formula><divide><dividend>1</dividend><divisor><subtract><minuend>2</minuend><subtrahend>2</subtrahend></subtract></divisor></divide></formula>", "<formula> > <divide>" },
	};

	for (const Mistake& mistake : mistakes)
	{
		DYNAMIC_SECTION(mistake.what)
		{
			REQUIRE_THROWS_WITH(Loaded(gameXml(mistake.value, "0", "<variable name=\"zero\">0</variable>")),
				ContainsSubstring("object 'o'") && ContainsSubstring(mistake.where) && ContainsSubstring("the divisor is 0"));
		}
	}
}

TEST_CASE("only a divisor matters: 0 as a dividend, or in other operations, is fine", "[equations][zero]")
{
	CHECK(xOf("<equation><divide dividend=\"0\" divisor=\"5\" /></equation>") == 0.0f);
	CHECK(xOf("<formula><divide><dividend>0</dividend><divisor>5</divisor></divide></formula>") == 0.0f);
	CHECK(xOf("<equation><multiply multiplicand=\"7\" multiplier=\"0\" /></equation>") == 0.0f);
	CHECK(xOf("<equation><add augend=\"0\" addend=\"0\" /></equation>") == 0.0f);
	CHECK(xOf("<equation><subtract minuend=\"4\" subtrahend=\"0\" /></equation>") == 4.0f);
}

TEST_CASE("the exprtk text is left alone: it divides by 0 as it always did", "[equations][zero]")
{
	// Not an error, as before; the tags are where the engine looks.
	Loaded loaded(gameXml("1 / zero", "0", "<variable name=\"zero\">0</variable>"));
	CHECK(std::isinf(loaded.x()));
}

TEST_CASE("a position finished once a text is measured warns, once, instead of ending the game", "[equations][zero]")
{
	// The text's width is unknown until a window measures it, so nothing is
	// divided at load; a text measured as 0 wide is the divisor of 0.
	const std::string xml = gameWithObject("t", "<text><content>hi</content><size>20</size></text>",
		"<equation><divide dividend=\"100\" divisor=\"t.width\" /></equation>");

	Loaded loaded(xml);
	Object& text = loaded.game.getObject("t");
	loaded.game.setCurrentState(0);
	CHECK(text.positionUsesSize);

	CaptureStderr captured;

	measure(text, 0.0f, 10.0f);
	REQUIRE_NOTHROW(loaded.game.resolveSizeDependentPositions());
	CHECK(text.positionResolved);
	CHECK(text.position.x == 0.0f);

	// The size changes, so it is worked out again: still 0 wide, so the same place, no second warning.
	measure(text, 0.0f, 12.0f);
	REQUIRE_NOTHROW(loaded.game.resolveSizeDependentPositions());
	CHECK(text.position.x == 0.0f);

	CHECK(countOf(captured.text.str(), "the divisor is 0; using 0 for the answer") == 1);
	CHECK_THAT(captured.text.str(), ContainsSubstring("object 't'") && ContainsSubstring("<divide>"));

	// Measured as a width that works, it is placed properly.
	measure(text, 50.0f, 12.0f);
	loaded.game.resolveSizeDependentPositions();
	CHECK(text.position.x == 2.0f);
}

TEST_CASE("a timer whose interval reaches a divisor of 0 keeps going, with one warning", "[equations][zero]")
{
	// Every 1 / rate seconds; each time it goes off it takes 1 off rate, so the
	// second interval divides by 0.
	const std::string xml = gameWithObject("c", "<circle><radius>5</radius></circle>", "0",
		"<variables><variable name=\"rate\">1</variable><variable name=\"n\">0</variable></variables>"
		"<timers><timer>"
		"<every><formula><divide><dividend>1</dividend><divisor>c.rate</divisor></divide></formula></every>"
		"<dec variable=\"c.rate\" /><inc variable=\"c.n\" />"
		"</timer></timers>");

	Loaded loaded(xml);
	loaded.game.setCurrentState(0);

	CaptureStderr captured;

	for (int i = 0; i < 59; ++i) { REQUIRE_NOTHROW(loaded.game.updateObjects()); }
	CHECK(loaded.game.getObject("c").variable.at("n") == 0.0f); // a second at 60 frames a second

	for (int i = 0; i < 10; ++i) { REQUIRE_NOTHROW(loaded.game.updateObjects()); }

	CHECK(loaded.game.getObject("c").variable.at("rate") < 1.0f);
	CHECK(loaded.game.getObject("c").variable.at("n") >= 2.0f);
	CHECK(countOf(captured.text.str(), "the divisor is 0") == 1);
}

TEST_CASE("a divisor that is 0 only because it is not known yet is not refused at load", "[equations][zero]")
{
	// b is built after a, so b.w reads 0 while a is being built; and the width of
	// a text reads 0 until a window has measured it. Neither is a mistake.
	const std::string xml =
		"<game>"
		"<window name=\"test\"><width>800</width><height>600</height><background>color.black</background>"
		"<fullscreen>false</fullscreen><framerate>60</framerate></window>"
		"<variables></variables>"
		"<objects>"
		"<object name=\"a\"><sprite><circle><radius>5</radius></circle></sprite>"
		"<position><x>0</x><y>0</y></position><velocity><x>0</x><y>0</y></velocity>"
		"<collisions><enabled>false</enabled></collisions>"
		"<variables><variable name=\"v\"><formula><divide><dividend>1</dividend><divisor>b.w</divisor></divide></formula></variable></variables></object>"
		"<object name=\"b\"><sprite><text><content>hi</content><size>20</size></text></sprite>"
		"<position><x><equation><divide dividend=\"100\" divisor=\"b.width\" /></equation></x><y>0</y></position>"
		"<velocity><x>0</x><y>0</y></velocity><collisions><enabled>false</enabled></collisions>"
		"<variables><variable name=\"w\">4</variable></variables></object>"
		"</objects>"
		"<states><state name=\"playing\"><shows><show object=\"a\" /><show object=\"b\" /></shows>"
		"<inputs><input button=\"space\"><pop /></input></inputs></state></states>"
		"</game>";

	CaptureStderr captured;
	REQUIRE_NOTHROW(Loaded(xml));
	CHECK(captured.text.str().find("divisor is 0") == std::string::npos); // and nothing is said
}
