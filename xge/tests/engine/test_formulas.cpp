// test_formulas.cpp
// XML Game Engine
// author: beefviper
// date: Oct 4, 2026
//
// Catch2 tests for arithmetic written as tags instead of as text: a ,
// one operation whose operands are themselves numbers, operations included. It
// is a value tag, like <random>, so the text form (load/calc: + - * / and
// brackets) still works beside it, and both must give the same number. Then what each mistake says, the
// printed form, a divisor of 0 (a load error, and a warning instead once the
// game is running), both schema checkers against the same good and bad files,
// and the two shipped games written with it (Pong's and Breakout's titles),
// against real xge::Games.
//
// Small games are written to a scratch file and loaded by a real xge::Game;
// none of it needs a window.

#include "scratch.h"
#include "file_text.h"
#include "game/command.h"
#include "engine/folder.h"
#include "game/game.h"
#include "xml/document.h"
#include "load/xsd.h"

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
		ScratchFile scratch{ std::filesystem::temp_directory_path() / scratchName(name) };
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
			"<framerate>60</framerate></window>"
			"<variables>" + variables + "</variables>"
			"<states><objects><object name=\"o\"><sprite><circle><radius>5</radius></circle></sprite>"
			"<position><x>" + x + "</x><y>" + y + "</y></position>"
			"<velocity><x>0</x><y>0</y></velocity>"
			"<collisions><off /></collisions></object></objects>"
			"<state name=\"playing\"><shows><show object=\"o\" /></shows>"
			"<inputs><input button=\"space\"><pop /></input></inputs></state></states>"
			"</game>";
	}

	struct Loaded
	{
		ScratchFile file;
		Game game;

		explicit Loaded(const std::string& xml) :
			file(writeScratch("xge_formulas_game.xml", xml)),
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

	// The same arithmetic as text (refused) and as an operation, for the tests
	// that compare them. Window width is 800, w is 100 and xoffset is 7.
	const std::string kVariables = "<variable name=\"w\">100</variable><variable name=\"xoffset\">7</variable>";

	const std::string kText = "window.width.center - w / 2 - xoffset";

	const std::string kFormula =
		"<subtract>"
		"<value>window.width.center</value>"
		"<value><divide><value>w</value><value>2</value></divide></value>"
		"<value>xoffset</value>"
		"</subtract>";
}

// --------------------------------------------------------------- the forms
TEST_CASE("a value's text is one number or one name: arithmetic in it is refused, naming the operation to write", "[formulas][errors]")
{
	CHECK(xOf(kFormula, kVariables) == 343.0f);
	CHECK(xOf("-7") == -7.0f);
	CHECK(xOf("xoffset", kVariables) == 7.0f);
	CHECK_THROWS_WITH(Loaded(gameXml("w / 2", "0", kVariables)), ContainsSubstring("\"w / 2\": a value's text is one number or one name, with no arithmetic hidden in it; write it as <divide><value>w</value><value>2</value></divide>"));
	CHECK_THROWS_WITH(Loaded(gameXml(kText, "0", kVariables)), ContainsSubstring("write the arithmetic as operations (<add>, <subtract>"));
	CHECK_THROWS_WITH(Loaded(gameXml(" ", "0")), ContainsSubstring("has no value"));
}

TEST_CASE("each operation works", "[formulas]")
{
	struct Case
	{
		const char* tag;
		float expected;
	};

	// 12 and 4 give 16, 8, 48 and 3.
	const Case cases[] = {
		{ "add", 16.0f },
		{ "subtract", 8.0f },
		{ "multiply", 48.0f },
		{ "divide", 3.0f },
	};

	for (const Case& c : cases)
	{
		DYNAMIC_SECTION(c.tag)
		{
			const std::string tag = c.tag;
			CHECK(xOf("<" + tag + "><value>12</value><value>4</value></" + tag + ">") == c.expected);
		}
	}
}

TEST_CASE("an operation takes the first operand, then each of the others in turn", "[formulas]")
{
	const auto operation = [](const std::string& tag, const std::vector<std::string>& numbers)
	{
		std::string xml = "<" + tag + ">";
		for (const std::string& number : numbers) { xml += "<value>" + number + "</value>"; }
		return xml + "</" + tag + ">";
	};

	CHECK(xOf(operation("subtract", { "100", "10", "20", "30" })) == 40.0f);
	CHECK(xOf(operation("divide", { "1000", "10", "5" })) == 20.0f);
	CHECK(xOf(operation("add", { "1", "2", "3" })) == 6.0f);
	CHECK(xOf(operation("multiply", { "2", "3", "4" })) == 24.0f);
}

TEST_CASE("operations nest as deep as they are written, and a <random> is an operand", "[formulas]")
{
	// (((6 + 4) * 3) - (20 / 5)) + 1 = 27
	CHECK(xOf(
		"<add>"
		"<value><subtract>"
		"<value><multiply><value><add><value>6</value><value>4</value></add></value><value>3</value></multiply></value>"
		"<value><divide><value>20</value><value>5</value></divide></value>"
		"</subtract></value>"
		"<value>1</value>"
		"</add>") == 27.0f);

	CHECK(xOf("<add><value><random><min>5</min><max>5</max></random></value><value>1</value></add>") == 6.0f);
}

TEST_CASE("smallest, largest, absolute and clamp", "[formulas]")
{
	CHECK(xOf("<smallest><value>12</value><value>4</value><value>9</value></smallest>") == 4.0f);
	CHECK(xOf("<largest><value>12</value><value>4</value><value>w</value></largest>", kVariables) == 100.0f);
	CHECK(xOf("<absolute><value><subtract><value>4</value><value>w</value></subtract></value></absolute>", kVariables) == 96.0f);
	CHECK(xOf("<clamp><value>w</value><atleast>10</atleast><atmost>50</atmost></clamp>", kVariables) == 50.0f);
	CHECK(xOf("<clamp><value>-3</value><atleast>10</atleast><atmost>50</atmost></clamp>") == 10.0f);
	CHECK(xOf("<clamp><value>xoffset</value><atleast>0</atleast><atmost>50</atmost></clamp>", kVariables) == 7.0f);

	// Too many operands, too few, or in the wrong order.
	CHECK_THROWS_WITH(Loaded(gameXml("<absolute><value>1</value><value>2</value></absolute>")), ContainsSubstring("after all it holds: it holds one <value>"));
	CHECK_THROWS_WITH(Loaded(gameXml("<smallest><value>1</value></smallest>")), ContainsSubstring("needs two or more <value>"));
	CHECK_THROWS_WITH(Loaded(gameXml("<clamp><value>1</value><atmost>2</atmost><atleast>0</atleast></clamp>")), ContainsSubstring("<atmost> where <atleast> belongs"));
}

TEST_CASE("sign, ceiling, sine and cosine, the last two of degrees", "[formulas]")
{
	CHECK(xOf("<sign><value>-7.5</value></sign>") == -1.0f);
	CHECK(xOf("<sign><value>0</value></sign>") == 0.0f);
	CHECK(xOf("<sign><value>w</value></sign>", kVariables) == 1.0f);
	CHECK(xOf("<ceiling><value>2.25</value></ceiling>") == 3.0f);
	CHECK(xOf("<ceiling><value>-2.25</value></ceiling>") == -2.0f);
	CHECK(xOf("<ceiling><value>4</value></ceiling>") == 4.0f);
	CHECK(std::fabs(xOf("<sine><value>30</value></sine>") - 0.5f) < 1e-6f);
	CHECK(std::fabs(xOf("<cosine><value>60</value></cosine>") - 0.5f) < 1e-6f);
	CHECK(std::fabs(xOf("<sine><value>-90</value></sine>") + 1.0f) < 1e-6f);
	CHECK(xOf("<cosine><value>0</value></cosine>") == 1.0f);

	CHECK_THROWS_WITH(Loaded(gameXml("<sine><value>1</value><value>2</value></sine>")), ContainsSubstring("after all it holds: it holds one <value>"));
	CHECK_THROWS_WITH(Loaded(gameXml("<tangent><value>1</value></tangent>")),
		ContainsSubstring("an operation: <add>, <subtract>, <multiply>, <divide>, <remainder>, <smallest>, <largest>, <absolute>, <negate>, <sign>, <floor>, <ceiling>, <round>, <sine>, <cosine>, <clamp> or <compare>"));
}

TEST_CASE("negate, floor, round and remainder", "[formulas]")
{
	CHECK(xOf("<negate><value>w</value></negate>", kVariables) == -100.0f);
	CHECK(xOf("<floor><value>2.75</value></floor>") == 2.0f);
	CHECK(xOf("<floor><value>-2.25</value></floor>") == -3.0f);
	CHECK(xOf("<round><value>2.5</value></round>") == 3.0f);
	CHECK(xOf("<round><value>2.49</value></round>") == 2.0f);

	// What is left, from 0 up to the divisor, so a count wraps: -1 of 4 leaves 3.
	CHECK(xOf("<remainder><value>10</value><value>4</value></remainder>") == 2.0f);
	CHECK(xOf("<remainder><value>-1</value><value>4</value></remainder>") == 3.0f);
	CHECK(xOf("<remainder><value>7.5</value><value>2</value></remainder>") == 1.5f);
	CHECK_THROWS_WITH(Loaded(gameXml("<remainder><value>1</value><value>0</value></remainder>")), ContainsSubstring("<remainder>: the divisor is 0"));
	CHECK_THROWS_WITH(Loaded(gameXml("<remainder><value>1</value><value>2</value><value>3</value></remainder>")),
		ContainsSubstring("it holds two <value>"));
}

TEST_CASE("a <compare> is 1 when its test holds and 0 when not", "[formulas]")
{
	struct Case
	{
		const char* tag;
		float lower;  // 3 against 4
		float same;   // 4 against 4
		float higher; // 5 against 4
	};

	const Case cases[] = {
		{ "above", 0.0f, 0.0f, 1.0f },
		{ "below", 1.0f, 0.0f, 0.0f },
		{ "atleast", 0.0f, 1.0f, 1.0f },
		{ "atmost", 1.0f, 1.0f, 0.0f },
		{ "equal", 0.0f, 1.0f, 0.0f },
	};

	for (const Case& c : cases)
	{
		DYNAMIC_SECTION(c.tag)
		{
			const std::string tag = c.tag;
			const auto compare = [&](const std::string& first) { return xOf("<compare><value>" + first + "</value><" + tag + ">4</" + tag + "></compare>"); };
			CHECK(compare("3") == c.lower);
			CHECK(compare("4") == c.same);
			CHECK(compare("5") == c.higher);
		}
	}

	// Either side can be an operation of its own.
	CHECK(xOf("<compare><value><add><value>w</value><value>1</value></add></value><above>w</above></compare>", kVariables) == 1.0f);

	CHECK_THROWS_WITH(Loaded(gameXml("<compare><value>1</value></compare>")), ContainsSubstring("needs a <value>, then one of <above>, <below>, <atleast>, <atmost> or <equal>"));
	CHECK_THROWS_WITH(Loaded(gameXml("<compare><value>1</value><value>2</value></compare>")), ContainsSubstring("has <value> where its test belongs"));
	CHECK_THROWS_WITH(Loaded(gameXml("<atleast><value>1</value><value>2</value></atleast>")), ContainsSubstring("unknown value tag <atleast>"));
}

TEST_CASE("a value's text has no functions: they are operation tags", "[formulas][errors]")
{
	CHECK_THROWS_WITH(Loaded(gameXml("max(1, 2)")), ContainsSubstring("a value's text has no functions; write it as an operation (<smallest>, <largest>"));
	CHECK_THROWS_WITH(Loaded(gameXml("2 * pi")), ContainsSubstring("write it as <multiply><value>2</value><value>pi</value></multiply>"));
	CHECK_THROWS_WITH(Loaded(gameXml("pi")), ContainsSubstring("'pi' names nothing"));
	CHECK_THROWS_WITH(Loaded(gameXml("w >= 2", "0", kVariables)), ContainsSubstring("write the arithmetic as operations"));
}

TEST_CASE("a <random>'s ends can be operations", "[formulas]")
{
	CHECK(xOf("<random><min><multiply><value>w</value><value>2</value></multiply></min><max>200</max></random>", kVariables) == 200.0f);
	CHECK(xOf("<random><min>xoffset</min><max><smallest><value>w</value><value>7</value></smallest></max></random>", kVariables) == 7.0f);
	CHECK_THROWS_WITH(Loaded(gameXml("<random><min><dice /></min><max>2</max></random>")), ContainsSubstring("an end is a number, a name or an operation"));
}

TEST_CASE("variables can hold an operation, and an expression can use them", "[formulas]")
{
	Loaded loaded(gameXml(
		"<add><value><multiply><value>r</value><value>2</value></multiply></value><value>1</value></add>", "s",
		kVariables +
		"<variable name=\"r\">" + kFormula + "</variable>"
		"<variable name=\"s\">r</variable>"));

	CHECK(loaded.game.getVariable("r") == 343.0f);
	CHECK(loaded.game.getVariable("s") == 343.0f);
	CHECK(loaded.x() == 687.0f);
	CHECK(loaded.y() == 343.0f);
}

TEST_CASE("an operation follows the order its operands are written in", "[formulas]")
{
	// Not commutative: 10 - 4 is not 4 - 10, and 10 / 4 is not 4 / 10.
	CHECK(xOf("<subtract><value>10</value><value>4</value></subtract>") == 6.0f);
	CHECK(xOf("<subtract><value>4</value><value>10</value></subtract>") == -6.0f);
	CHECK(xOf("<divide><value>10</value><value>4</value></divide>") == 2.5f);
	CHECK(xOf("<divide><value>4</value><value>10</value></divide>") == 0.4f);
}

TEST_CASE("a negative number and a decimal are numbers", "[formulas]")
{
	CHECK(xOf("<add><value>-110</value><value>0.5</value></add>") == -109.5f);
	CHECK(xOf("<add><value>-110</value><value>1e2</value></add>") == -10.0f);
}

// --------------------------------------------------------------- the print
TEST_CASE("a value prints as infix with its brackets", "[formulas]")
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

	RawOperation compare;
	compare.op = "compare";
	RawOperand of = operand("mode.waves");
	of.role = "value";
	RawOperand test = operand("4");
	test.role = "atleast";
	compare.operands = { of, test };
	RawValue compared;
	compared.kind = RawValue::Kind::Formula;
	compared.operations = std::make_shared<std::vector<RawOperation>>(1, compare);
	CHECK(valueText(compared) == "(mode.waves >= 4)");
	CHECK(valueText(RawValue::expression("a + b")) == "a + b");
}

// --------------------------------------------------------------- mistakes
TEST_CASE("an operation that is wrong says which operation and what", "[formulas][errors]")
{
	struct Mistake
	{
		const char* what;
		std::string value;
		const char* says;
	};

	const Mistake mistakes[] = {
		{ "nothing", "", "has no value" },
		{ "two operations", "<add><value>1</value><value>1</value></add><add><value>1</value><value>1</value></add>", "holds more than one value tag" },
		{ "a tag that is not an operation", "<modulo><value>7</value><value>2</value></modulo>", "unknown value tag <modulo>" },
		{ "one operand", "<subtract><value>1</value></subtract>", "needs two or more <value>" },
		{ "an expression as an operand", "<add><value>a + b</value><value>1</value></add>", "\"a + b\" is not a name or a number" },
		{ "an operand with text and a tag", "<add><value>2 <random><min>1</min><max>2</max></random></value><value>1</value></add>", "both the text \"2\" and a <random> tag" },
		{ "an operand with a tag that is not an operation", "<add><value><dice /></value><value>1</value></add>", "<dice>, which is not an operation" },
		{ "an operand with nothing in it", "<add><value></value><value>1</value></add>", "has no value" },
		{ "text beside the tag", "5 <add><value>1</value><value>1</value></add>", "both the text \"5\" and a <add> tag" },
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
		std::string text = fileText(in);
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
		const ScratchFile scratch{ scratchName("descriptions/formulas_scratch.xml") };
		{
			std::ofstream out(scratch.path);
			out << xml;
		}

		Verdict verdict{};

		auto weak = XmlDocumentFactory::create(XmlBackend::TinyXml2);
		REQUIRE(weak->load(scratch.path.string()));
		XsdLiteValidator validator;
		REQUIRE(validator.loadSchema(findSchema("descriptions/pong.xml", "../xgedef.xsd").string(), XmlBackend::TinyXml2));
		verdict.weakAccepts = validator.validate(*weak->getRootElement());
		verdict.weakMessage = validator.getErrorMessage();

		auto strong = XmlDocumentFactory::create(XmlBackend::Xerces);
		verdict.strongAccepts = strong->load(scratch.path.string());

		return verdict;
	}

	// pong.xml with its title's <x> replaced by `value` (the whole
	// <x>...</x> element's inner text). The title is the main menu's own, so
	// it is written inside that state.
	std::string pongTitleX(const std::string& value)
	{
		std::string xml = readFile("descriptions/pong.xml");
		const std::string open = "<x>\n              ";
		const std::string close = "\n            </x>";
		const auto from = xml.find(open);
		const auto to = xml.find(close);
		REQUIRE(from != std::string::npos);
		REQUIRE(to != std::string::npos);
		xml.replace(from, to + close.size() - from, "<x>" + value + "</x>");
		return xml;
	}
}

TEST_CASE("both schema checkers accept every spelling", "[formulas][schema]")
{
	const std::string picked = "<clamp><value><largest><value>1</value><value>2</value><value><absolute><value>-3</value></absolute></value></largest></value>"
		"<atleast><smallest><value>0</value><value>1</value></smallest></atleast><atmost>9</atmost></clamp>";
	const std::string drawn = "<random><min><add><value>1</value><value>2</value></add></min><max>9</max></random>";
	for (const std::string& value : { std::string("window.width.center - title.width / 2"), kFormula, picked, drawn })
	{
		// kFormula uses the variables w and xoffset, which pong does not have; the
		// schema does not look at names, only at shape.
		const Verdict verdict = judge(pongTitleX(value));
		CHECK(verdict.weakAccepts);
		CHECK(verdict.strongAccepts);
	}

	for (const char* file : { "descriptions/pong.xml", "descriptions/breakout.xml" })
	{
		const Verdict verdict = judge(readFile(file));
		CHECK(verdict.weakAccepts);
		CHECK(verdict.strongAccepts);
	}
}

TEST_CASE("both schema checkers turn away the same mistakes in an operation", "[formulas][schema]")
{
	struct Mistake
	{
		const char* what;
		std::string value;
		const char* weakSays;
	};

	const Mistake mistakes[] = {
		{ "the old <equation>", "<equation><divide dividend=\"1\" divisor=\"2\" /></equation>", "unexpected element <equation>" },
		{ "a tag that is not an operation", "<modulo><value>1</value><value>2</value></modulo>", "unexpected element <modulo>" },
		{ "two operations", "<add><value>1</value><value>1</value></add><add><value>1</value><value>1</value></add>", "unexpected element <add>" },
		{ "a subtract of one value", "<subtract><value>1</value></subtract>", "expected <value>" },
		{ "an absolute of two", "<absolute><value>1</value><value>2</value></absolute>", "unexpected element <value>" },
		{ "a clamp out of order", "<clamp><value>1</value><atmost>2</atmost><atleast>0</atleast></clamp>", "expected <atleast>" },
		{ "an unknown tag deep inside", "<add><value><add><value>1</value><value><dice /></value></add></value><value>1</value></add>", "unexpected element <dice>" },
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
	}
}

TEST_CASE("Pong's title is placed by an operation on its own size", "[formulas][games]")
{
	Game game{ "descriptions/pong.xml" };
	Object& title = game.getObject("title");

	// Known when the game loads, from the text as drawn.
	CHECK(title.positionUsesSize);
	REQUIRE(title.size.x > 0.0f);
	CHECK(title.position.x == 640.0f - title.size.x / 2.0f); // 1280 wide window, window.width.center - title.width / 2
	CHECK(title.position.y == 120.0f);

	// A different size gives a different place, as it does for text.
	measure(title, 400.0f, 200.0f);
	game.resolveSizeDependentPositions();
	CHECK(title.position.x == 640.0f - 200.0f);
}

TEST_CASE("Breakout's title is placed by operations on its own size", "[formulas][games]")
{
	Game game{ "descriptions/breakout.xml" };
	Object& title = game.getObject("title");

	CHECK(title.positionUsesSize);

	measure(title, 600.0f, 200.0f);
	game.resolveSizeDependentPositions();

	CHECK(title.position.x == 320.0f - 300.0f);          // 640 wide: window.width.center - title.width / 2
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
			"<framerate>60</framerate></window>"
			"<variables><variable name=\"zero\">0</variable></variables>"
			"<states><objects><object name=\"" + name + "\"><sprite>" + sprite + "</sprite>"
			"<position><x>" + x + "</x><y>0</y></position>"
			"<velocity><x>0</x><y>0</y></velocity>"
			"<collisions><off /></collisions>" + more + "</object></objects>"
			"<state name=\"playing\"><shows><show object=\"" + name + "\" /></shows>"
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

TEST_CASE("a divisor of 0 stops the load, and says which divide", "[formulas][errors][zero]")
{
	struct Mistake
	{
		const char* what;
		std::string value;
		const char* where;
	};

	const Mistake mistakes[] = {
		{ "a number", "<divide><value>1</value><value>0</value></divide>", " > <divide>" },
		{ "a variable that is 0", "<divide><value>1</value><value>zero</value></divide>", " > <divide>" },
		{ "a later divisor of a chain", "<divide><value>100</value><value>5</value><value>0</value></divide>", " > <divide>" },
		{ "an operation that works out to 0, nested", "<divide><value>1</value><value><subtract><value>2</value><value>2</value></subtract></value></divide>", " > <divide>" },
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

TEST_CASE("only a divisor matters: 0 as a dividend, or in other operations, is fine", "[formulas][zero]")
{
	CHECK(xOf("<divide><value>0</value><value>5</value></divide>") == 0.0f);
	CHECK(xOf("<multiply><value>7</value><value>0</value></multiply>") == 0.0f);
	CHECK(xOf("<add><value>0</value><value>0</value></add>") == 0.0f);
	CHECK(xOf("<subtract><value>4</value><value>0</value></subtract>") == 4.0f);
}

TEST_CASE("a number too big to be one stops the load", "[formulas][errors][zero]")
{
	REQUIRE_THROWS_WITH(Loaded(gameXml("<multiply><value>1e30</value><value>1e30</value></multiply>", "0")),
		ContainsSubstring(": comes out as infinity"));
}

TEST_CASE("a position worked out again for a text's new size warns, once, instead of ending the game", "[formulas][zero]")
{
	// The text has a width when the game loads; one that comes to be 0 wide
	// is the divisor of 0.
	const std::string xml = gameWithObject("t", "<text><content>hi</content><size>20</size></text>",
		"<divide><value>100</value><value>t.width</value></divide>");

	Loaded loaded(xml);
	Object& text = loaded.game.getObject("t");
	loaded.game.setCurrentState(0);
	CHECK(text.positionUsesSize);

	CaptureStderr captured;

	measure(text, 0.0f, 10.0f);
	REQUIRE_NOTHROW(loaded.game.resolveSizeDependentPositions());
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

TEST_CASE("a timer whose interval reaches a divisor of 0 keeps going, with one warning", "[formulas][zero]")
{
	// Every 1 / rate seconds; each time it goes off it takes 1 off rate, so the
	// second interval divides by 0.
	const std::string xml = gameWithObject("c", "<circle><radius>5</radius></circle>", "0",
		"<variables><variable name=\"rate\">1</variable><variable name=\"n\">0</variable></variables>"
		"<timers><timer>"
		"<every><divide><value>1</value><value>c.rate</value></divide></every>"
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

TEST_CASE("a divisor that is 0 only because it is not known yet is not refused at load", "[formulas][zero]")
{
	// b is built after a, so b.w reads 0 while a is being built; and the width of
	// a text reads 0 until a window has measured it. Neither is a mistake.
	const std::string xml =
		"<game>"
		"<window name=\"test\"><width>800</width><height>600</height><background>color.black</background>"
		"<framerate>60</framerate></window>"
		"<variables></variables>"
		"<states><objects>"
		"<object name=\"a\"><sprite><circle><radius>5</radius></circle></sprite>"
		"<position><x>0</x><y>0</y></position><velocity><x>0</x><y>0</y></velocity>"
		"<collisions><off /></collisions>"
		"<variables><variable name=\"v\"><divide><value>1</value><value>b.w</value></divide></variable></variables></object>"
		"<object name=\"b\"><sprite><text><content>hi</content><size>20</size></text></sprite>"
		"<position><x><divide><value>100</value><value>b.width</value></divide></x><y>0</y></position>"
		"<velocity><x>0</x><y>0</y></velocity><collisions><off /></collisions>"
		"<variables><variable name=\"w\">4</variable></variables></object>"
		"</objects>"
		"<state name=\"playing\"><shows><show object=\"a\" /><show object=\"b\" /></shows>"
		"<inputs><input button=\"space\"><pop /></input></inputs></state></states>"
		"</game>";

	CaptureStderr captured;
	REQUIRE_NOTHROW(Loaded(xml));
	CHECK(captured.text.str().find("divisor is 0") == std::string::npos); // and nothing is said
}
