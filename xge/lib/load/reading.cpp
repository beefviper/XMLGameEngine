// reading.cpp
// XML Game Engine
// author: beefviper
// date: Nov 10, 2026

#include "load/reading.h"

#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <stdexcept>
#include <vector>

namespace xge
{
	std::string getAttribute(const XmlNode* node, const std::string& name)
	{
		return node ? node->getAttribute(name) : std::string{};
	}

	std::string trim(const std::string& text)
	{
		const auto first = text.find_first_not_of(" \t\r\n");
		if (first == std::string::npos) { return {}; }
		const auto last = text.find_last_not_of(" \t\r\n");
		return text.substr(first, last - first + 1);
	}

	// The first child of parent named `name`, or nullptr if parent is null
	// or none match. Schema elements have fixed names but a sequence still
	// lets some be left out (an object with no <actions>), so this looks a
	// child up by name rather than chaining getFirstChild()/getNextSibling()
	// calls that assume a particular position.
	std::unique_ptr<XmlNode> findChild(const XmlNode* parent, const std::string& name)
	{
		std::unique_ptr<XmlNode> child = parent ? parent->getFirstChild() : nullptr;

		while (child != nullptr && child->getName() != name)
		{
			child = child->getNextSibling();
		}

		return child;
	}

	[[noreturn]] void fail(const std::string& where, const std::string& message)
	{
		throw std::runtime_error(where + ": " + message);
	}

	std::unique_ptr<XmlNode> requireChild(const XmlNode& parent, const std::string& name, const std::string& where)
	{
		std::unique_ptr<XmlNode> child = findChild(&parent, name);
		if (!child) { fail(where, "missing <" + name + ">"); }
		return child;
	}

	std::string requireAttribute(const XmlNode& node, const std::string& name, const std::string& where)
	{
		const std::string value = node.getAttribute(name);
		if (value.empty()) { fail(where, "<" + node.getName() + "> needs " + name + "=\"...\""); }
		return value;
	}

	// The text of a simple element such as <color>color.red</color>.
	std::string readText(const XmlNode& node)
	{
		return trim(node.getText());
	}

	bool readBool(const XmlNode& node, const std::string& where)
	{
		const std::string text = readText(node);
		if (text == "true" || text == "1") { return true; }
		if (text == "false" || text == "0") { return false; }
		fail(where, "<" + node.getName() + "> is \"" + text + "\"; expected true or false");
	}


	// <random><min>..</min><max>..</max></random>: each end an expression
	// or a <formula>. With a formula end, the two ends are kept as one
	// operation "random" (see RawValue::operations).
	RawValue readRandom(const XmlNode& tag, const std::string& here)
	{
		RawValue value;
		value.kind = RawValue::Kind::Random;
		const std::string there = here + " > <random>";
		for (std::unique_ptr<XmlNode> child = tag.getFirstChild(); child; child = child->getNextSibling())
		{
			if (child->getName() != "min" && child->getName() != "max") { fail(there, "unknown <" + child->getName() + ">; expected <min> and <max>"); }
		}

		RawOperation ends;
		ends.op = "random";
		bool worked = false;
		for (const char* end : { "min", "max" })
		{
			const std::unique_ptr<XmlNode> node = requireChild(tag, end, there);
			const std::string text = readText(*node);
			const std::unique_ptr<XmlNode> inside = node->getFirstChild();
			RawOperand operand;
			operand.role = end;
			if (inside)
			{
				const std::string at = there + " > <" + end + ">";
				if (inside->getName() != "formula") { fail(at, "holds <" + inside->getName() + ">; an end is a number, an expression or a <formula>"); }
				if (!text.empty()) { fail(at, "holds both the text \"" + text + "\" and a <formula>; use one or the other"); }
				if (inside->getNextSibling()) { fail(at, "holds more than one tag"); }
				operand.value = readFormula(*inside, at);
				worked = true;
			}
			else
			{
				if (text.empty()) { fail(there, "a <min> or <max> with nothing in it; each is a number, an expression or a <formula>"); }
				operand.value = RawValue::expression(text);
			}
			ends.operands.push_back(std::move(operand));
		}

		if (worked)
		{
			value.operations = std::make_shared<std::vector<RawOperation>>(1, std::move(ends));
		}
		else
		{
			value.min = ends.operands[0].value.text;
			value.max = ends.operands[1].value.text;
		}
		return value;
	}

	// Whether text is a plain number (`2`, `-130`, `0.5`, `1e3`): digits, a
	// sign, a point and an exponent, nothing else, so that "inf" and "0x10"
	// are not.
	bool isNumber(const std::string& text)
	{
		if (text.empty() || text.find_first_not_of("0123456789+-.eE") != std::string::npos) { return false; }

		char* end = nullptr;
		std::strtod(text.c_str(), &end);
		return end == text.c_str() + text.size();
	}

	// Whether text is a name: a letter or underscore, then letters, digits,
	// underscores and (when dots is true) dots, like `window.width.center`.
	bool isName(const std::string& text, bool dots)
	{
		if (text.empty() || !(std::isalpha(static_cast<unsigned char>(text[0])) || text[0] == '_')) { return false; }

		return std::all_of(text.begin(), text.end(), [dots](char c)
		{
			return std::isalnum(static_cast<unsigned char>(c)) || c == '_' || (dots && c == '.');
		});
	}

	// An operand of a formula is a name or a number and
	// nothing else: anything more is an operation of its own.
	std::string requireTerm(const std::string& text, const std::string& here)
	{
		if (!isNumber(text) && !isName(text, true))
		{
			fail(here, "\"" + text + "\" is not a name or a number; an operand is one or the other, so write anything more than that as an operation of its own");
		}
		return text;
	}


	// An operand of a <formula> operation: <minuend>window.width.center
	// </minuend>, or an operation in place of the name, or a <random>.
	RawValue readFormulaOperand(const XmlNode& node, const std::string& where)
	{
		const std::string text = readText(node);
		const std::string here = where + " > <" + node.getName() + ">";
		std::unique_ptr<XmlNode> tag = node.getFirstChild();

		if (!tag)
		{
			if (text.empty()) { fail(here, "has no value"); }
			return RawValue::expression(requireTerm(text, here));
		}

		if (!text.empty()) { fail(here, "holds both the text \"" + text + "\" and a <" + tag->getName() + "> tag; use one or the other"); }
		if (tag->getNextSibling()) { fail(here, "holds more than one tag"); }

		if (tag->getName() == "random") { return readRandom(*tag, here); }

		const OperationShape* shape = operationShape(tag->getName());
		if (!shape) { fail(here, "has <" + tag->getName() + ">, which is not an operation; expected a name, a number, <random>, " + operationTags()); }

		RawValue value;
		value.kind = RawValue::Kind::Formula;
		value.operations = std::make_shared<std::vector<RawOperation>>(1, readFormulaOperation(*tag, *shape, here));
		return value;
	}

	// <subtract><minuend>..</minuend><subtrahend>..</subtrahend>
	// <subtrahend>..</subtrahend></subtract>: the first operand, then one or
	// more of the second, taken one after another.
	RawOperation readFormulaOperation(const XmlNode& node, const OperationShape& shape, const std::string& where)
	{
		const std::string here = where + " > <" + shape.tag + ">";


		RawOperation operation;
		operation.op = shape.tag;

		// What it holds, in words, for a mistake: "a <minuend>, then one
		// or more <subtrahend>", "one <value>", "two <value>", "a <value>, an <atleast> and an <atmost>".
		std::string wanted;
		if (shape.compares) { wanted = std::string("a <") + shape.first + ">, then one of <above>, <below>, <atleast>, <atmost> or <equal>"; }
		else if (shape.most == 1) { wanted = std::string("one <") + shape.first + ">"; }
		else if (shape.most == 2 && std::string(shape.first) == shape.rest) { wanted = std::string("two <") + shape.first + ">"; }
		else if (shape.most == 2) { wanted = std::string("a <") + shape.first + "> and a <" + shape.rest + ">, in that order"; }
		else if (shape.last) { wanted = std::string("a <") + shape.first + ">, an <" + shape.rest + "> and an <" + shape.last + ">, in that order"; }
		else if (std::string(shape.first) == shape.rest) { wanted = std::string("two or more <") + shape.first + ">"; }
		else { wanted = std::string("a <") + shape.first + ">, then one or more <" + shape.rest + ">"; }

		for (std::unique_ptr<XmlNode> child = node.getFirstChild(); child; child = child->getNextSibling())
		{
			std::string expected = shape.roleAt(operation.operands.size());
			if (shape.most > 0 && operation.operands.size() == shape.most)
			{
				fail(here, "has <" + child->getName() + "> after all it holds: it holds " + wanted);
			}
			// A <compare>'s test names its second operand.
			if (shape.compares && operation.operands.size() == 1)
			{
				if (!isComparisonWord(child->getName())) { fail(here, "has <" + child->getName() + "> where its test belongs: it holds " + wanted); }
				expected = child->getName();
			}
			if (child->getName() != expected)
			{
				fail(here, "has <" + child->getName() + "> where <" + expected + "> belongs: it holds " + wanted);
			}

			RawOperand operand;
			operand.role = expected;
			operand.value = readFormulaOperand(*child, here);
			operation.operands.push_back(std::move(operand));
		}

		if (operation.operands.size() < shape.fewest) { fail(here, "needs " + wanted); }

		return operation;
	}

	RawValue readFormula(const XmlNode& node, const std::string& where)
	{
		const std::string here = where + " > <formula>";
		std::unique_ptr<XmlNode> tag = node.getFirstChild();

		if (!tag) { fail(here, "has no operation"); }
		if (tag->getNextSibling()) { fail(here, "holds more than one operation; a formula is one, with the others inside it"); }

		const OperationShape* shape = tag->getName() == "random" ? nullptr : operationShape(tag->getName());
		if (!shape) { fail(here, "has <" + tag->getName() + ">, which is not an operation; expected " + operationTags()); }

		RawValue value;
		value.kind = RawValue::Kind::Formula;
		value.operations = std::make_shared<std::vector<RawOperation>>(1, readFormulaOperation(*tag, *shape, here));
		return value;
	}


	// A number, written either as an expression (the text of the element) or
	// as one value tag inside it - see RawValue - and whether it is
	// update="live".
	RawValue readValue(const XmlNode& node, const std::string& where)
	{
		RawValue value = readValueBody(node, where);
		value.live = node.getAttribute("update") == "live";
		return value;
	}

	RawValue readValueBody(const XmlNode& node, const std::string& where)
	{
		const std::string text = readText(node);
		const std::string here = where + " > <" + node.getName() + ">";
		std::unique_ptr<XmlNode> tag = node.getFirstChild();

		if (!tag)
		{
			if (text.empty()) { fail(here, "has no value"); }
			return RawValue::expression(text);
		}

		if (!text.empty()) { fail(here, "holds both the text \"" + text + "\" and a <" + tag->getName() + "> tag; use one or the other"); }
		if (tag->getNextSibling()) { fail(here, "holds more than one value tag"); }

		const std::string tagName = tag->getName();

		if (tagName == "random") { return readRandom(*tag, here); }
		if (tagName == "formula") { return readFormula(*tag, here); }
		if (tagName == "centered")
		{
			if (tag->getFirstChild()) { fail(here + " > <centered>", "holds a tag; it holds where the middle goes, as text, or nothing for the window's middle"); }
			RawValue value = RawValue::expression(readText(*tag));
			value.kind = RawValue::Kind::Centered;
			return value;
		}

		fail(here, "unknown value tag <" + tagName + ">");
	}

	RawValue readValueOf(const XmlNode& parent, const std::string& name, const std::string& where)
	{
		return readValue(*requireChild(parent, name, where), where);
	}

	// The one number a verb holds as its content, as text or a value tag,
	// beside the named parts it may also have (`parts`, with `expected`
	// saying them for the message): <chase object="ship">2<near>40</near></chase>.
	RawValue readLeadValue(const XmlNode& node, std::initializer_list<const char*> parts, const std::string& expected, const std::string& where)
	{
		const std::string here = where + " > <" + node.getName() + ">";
		std::string valueTagName;
		for (std::unique_ptr<XmlNode> child = node.getFirstChild(); child != nullptr; child = child->getNextSibling())
		{
			const std::string tag = child->getName();
			if (tag == "random" || tag == "formula")
			{
				if (!valueTagName.empty()) { fail(here, "holds more than one value tag"); }
				valueTagName = tag;
			}
			else if (std::none_of(parts.begin(), parts.end(), [&](const char* part) { return tag == part; }))
			{
				fail(here, "unknown <" + tag + ">; expected " + expected);
			}
		}
		const std::unique_ptr<XmlNode> valueTag = valueTagName.empty() ? nullptr : findChild(&node, valueTagName);

		const std::string text = readText(node);
		if (valueTag && !text.empty()) { fail(here, "holds both the text \"" + text + "\" and a <" + valueTag->getName() + "> tag; use one or the other"); }
		if (!valueTag && text.empty()) { fail(here, "has no value; its number goes between its tags"); }
		if (!valueTag) { return RawValue::expression(text); }
		if (valueTag->getName() == "random") { return readRandom(*valueTag, here); }
		return readFormula(*valueTag, here);
	}

	PartialVector2 readPartialVector2(const XmlNode& node, const std::string& where)
	{
		const std::string here = where + " > <" + node.getName() + ">";
		PartialVector2 vector;
		if (auto x = findChild(&node, "x")) { vector.x = readValue(*x, here); }
		if (auto y = findChild(&node, "y")) { vector.y = readValue(*y, here); }
		return vector;
	}


	// A whole number of at least 1 (a group's <columns> and <rows>): they
	// say how many cells there are and what they are called, so they are
	// settled as the file is read, not worked out like other values.
	int readCount(const XmlNode& node, const std::string& where)
	{
		const std::string text = readText(node);
		const std::string here = where + " > <" + node.getName() + ">";
		if (text.empty() || text.size() > 4 || !std::all_of(text.begin(), text.end(), [](unsigned char c) { return std::isdigit(c) != 0; }) || std::stoi(text) < 1)
		{
			fail(here, "is \"" + text + "\"; expected a whole number from 1");
		}
		return std::stoi(text);
	}
}
