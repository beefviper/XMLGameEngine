// reading.cpp
// XML Game Engine
// author: beefviper
// date: Nov 10, 2026

#include "load/reading.h"

#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <set>
#include <stdexcept>
#include <vector>

namespace xge
{
	std::string trim(const std::string& text)
	{
		const auto first = text.find_first_not_of(" \t\r\n");
		if (first == std::string::npos) { return {}; }
		const auto last = text.find_last_not_of(" \t\r\n");
		return text.substr(first, last - first + 1);
	}

	std::unique_ptr<XmlNode> findChild(const XmlNode* parent, const std::string& name)
	{
		std::unique_ptr<XmlNode> child = parent ? parent->getFirstChild() : nullptr;

		while (child != nullptr && child->getName() != name)
		{
			child = child->getNextSibling();
		}

		return child;
	}

	Children::Children(const XmlNode& parent)
	{
		std::unique_ptr<XmlNode> child = parent.getFirstChild();
		while (child != nullptr)
		{
			std::unique_ptr<XmlNode> next = child->getNextSibling();
			std::string name = child->getName();
			all.push_back({ std::move(name), std::move(child) });
			child = std::move(next);
		}
	}

	const XmlNode* Children::find(std::string_view name) const
	{
		for (const Child& child : all)
		{
			if (child.name == name) { return child.node.get(); }
		}
		return nullptr;
	}

	[[noreturn]] void fail(const std::string& where, const std::string& message)
	{
		throw LoadError(where + ": " + message);
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

	// The text of a simple element such as <color>red</color>.
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


	// <window><right /></window>: one of the window's measures, as the name the
	// engine gives it, which starts with an @ so no name in a game is it.
	// <window><x><center /></x></window>: its middle across (in a <y>, down).
	static std::string windowMeasure(const XmlNode& tag, const std::string& where)
	{
		static const std::set<std::string> measures{ "width", "height", "right", "bottom" };
		const std::string here = where + " > <window>";
		const char* const wanted = "holds one of the window's measures: <width />, <height />, <right />, <bottom />, or its middle, <x><center /></x> or <y><center /></y>";
		const std::unique_ptr<XmlNode> measure = tag.getFirstChild();
		if (!measure || measure->getNextSibling() || !readText(tag).empty() || !readText(*measure).empty()) { fail(here, wanted); }
		const std::string name = measure->getName();
		if (measures.count(name) && !measure->getFirstChild()) { return "@window." + name; }
		if (name == "x" || name == "y")
		{
			const std::unique_ptr<XmlNode> center = measure->getFirstChild();
			if (center && center->getName() == "center" && !center->getNextSibling() && !center->getFirstChild())
			{
				return name == "x" ? "@window.centerx" : "@window.centery";
			}
		}
		fail(here, wanted);
	}

	bool isPronoun(const std::string& word)
	{
		return word == "self" || word == "other" || word == "others";
	}

	// <self>score</self>: a variable of the object running it (<other>: the
	// one it touched, <others>: each of the rest), as the engine names it.
	static std::string pronounVariable(const XmlNode& tag, const std::string& where)
	{
		const std::string text = readText(tag);
		if (tag.getFirstChild() || !isName(text, true))
		{
			fail(where + " > <" + tag.getName() + ">", "names one of its variables (or a live number such as x)");
		}
		return "@" + tag.getName() + "." + text;
	}

	// A tag that is one name the engine gives: a window measure, how many
	// people play (<people />) or a pronoun's variable. Empty for any other tag.
	static std::string engineName(const XmlNode& tag, const std::string& where)
	{
		if (tag.getName() == "window") { return windowMeasure(tag, where); }
		if (tag.getName() == "people")
		{
			if (tag.getFirstChild() || !readText(tag).empty()) { fail(where + " > <people>", "is empty as a value: <people />, how many people play"); }
			return "@people";
		}
		if (isPronoun(tag.getName())) { return pronounVariable(tag, where); }
		return {};
	}

	std::string readWho(const XmlNode& node, const std::string& where)
	{
		const std::string who = node.getAttribute("who");
		if (who.empty()) { return {}; }
		if (!isPronoun(who)) { fail(where, "<" + node.getName() + "> who=\"" + who + "\": who= is self, other or others"); }
		return "@" + who;
	}

	// Text written as a value: a game's names never start with an @, which
	// marks the engine's own (see windowMeasure).
	static std::string writtenText(const std::string& text, const std::string& here)
	{
		if (!text.empty() && text[0] == '@') { fail(here, "\"" + text + "\" is not a name or a number"); }
		return text;
	}

	// <random><min>..</min><max>..</max></random>: each end an expression
	// or an operation. With an operation end, the two ends are kept as one
	// operation "random" (see WrittenValue::operations).
	WrittenValue readRandom(const XmlNode& tag, const std::string& here)
	{
		WrittenValue value;
		value.kind = WrittenValue::Kind::Random;
		const std::string there = here + " > <random>";
		for (std::unique_ptr<XmlNode> child = tag.getFirstChild(); child; child = child->getNextSibling())
		{
			if (child->getName() != "min" && child->getName() != "max") { fail(there, "unknown <" + child->getName() + ">; expected <min> and <max>"); }
		}

		WrittenOperation ends;
		ends.op = "random";
		bool worked = false;
		for (const char* end : { "min", "max" })
		{
			const std::unique_ptr<XmlNode> node = requireChild(tag, end, there);
			const std::string text = readText(*node);
			const std::unique_ptr<XmlNode> inside = node->getFirstChild();
			WrittenOperand operand;
			operand.role = end;
			if (const std::string named = inside ? engineName(*inside, there + " > <" + end + ">") : std::string(); !named.empty())
			{
				if (!text.empty() || inside->getNextSibling()) { fail(there + " > <" + end + ">", "holds a <" + inside->getName() + "> and something more; an end is a number, a name or an operation"); }
				operand.value = WrittenValue::expression(named);
			}
			else if (inside)
			{
				const std::string at = there + " > <" + end + ">";
				if (!isOperation(inside->getName())) { fail(at, "holds <" + inside->getName() + ">; an end is a number, a name or an operation (" + operationTags() + ")"); }
				if (!text.empty()) { fail(at, "holds both the text \"" + text + "\" and an <" + inside->getName() + ">; use one or the other"); }
				if (inside->getNextSibling()) { fail(at, "holds more than one tag"); }
				operand.value = readOperation(*inside, at);
				worked = true;
			}
			else
			{
				if (text.empty()) { fail(there, "a <min> or <max> with nothing in it; each is a number, a name or an operation"); }
				operand.value = WrittenValue::expression(writtenText(text, there + " > <" + end + ">"));
			}
			ends.operands.push_back(std::move(operand));
		}

		if (worked)
		{
			value.operations = std::make_shared<std::vector<WrittenOperation>>(1, std::move(ends));
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
	// underscores and (when dots is true) dots, like `ship.score`.
	bool isName(const std::string& text, bool dots)
	{
		if (text.empty() || !(std::isalpha(static_cast<unsigned char>(text[0])) || text[0] == '_')) { return false; }

		return std::all_of(text.begin(), text.end(), [dots](char c)
		{
			return std::isalnum(static_cast<unsigned char>(c)) || c == '_' || (dots && c == '.');
		});
	}

	// An operand of an operation is a name or a number and
	// nothing else: anything more is an operation of its own.
	std::string requireTerm(const std::string& text, const std::string& here)
	{
		if (!isNumber(text) && !isName(text, true))
		{
			fail(here, "\"" + text + "\" is not a name or a number; an operand is one or the other, so write anything more than that as an operation of its own");
		}
		return text;
	}


	// An operand of an operation: <value>ship.score</value>, a <window> measure, an
	// operation in place of the name, or a <random>.
	WrittenValue readFormulaOperand(const XmlNode& node, const std::string& where)
	{
		const std::string text = readText(node);
		const std::string here = where + " > <" + node.getName() + ">";
		std::unique_ptr<XmlNode> tag = node.getFirstChild();

		if (!tag)
		{
			if (text.empty()) { fail(here, "has no value"); }
			return WrittenValue::expression(requireTerm(text, here));
		}

		if (!text.empty()) { fail(here, "holds both the text \"" + text + "\" and a <" + tag->getName() + "> tag; use one or the other"); }
		if (tag->getNextSibling()) { fail(here, "holds more than one tag"); }

		if (tag->getName() == "random") { return readRandom(*tag, here); }
		if (const std::string named = engineName(*tag, here); !named.empty()) { return WrittenValue::expression(named); }

		const OperationShape* shape = operationShape(tag->getName());
		if (!shape) { fail(here, "has <" + tag->getName() + ">, which is not an operation; expected a name, a number, <random>, <window>, <self>, <other>, <others>, " + operationTags()); }

		WrittenValue value;
		value.kind = WrittenValue::Kind::Formula;
		value.operations = std::make_shared<std::vector<WrittenOperation>>(1, readFormulaOperation(*tag, *shape, here));
		return value;
	}

	// <subtract><value>a</value><value>b</value><value>c</value></subtract>:
	// the first operand, then the others taken from it one after another.
	WrittenOperation readFormulaOperation(const XmlNode& node, const OperationShape& shape, const std::string& where)
	{
		const std::string here = where + " > <" + shape.tag + ">";


		WrittenOperation operation;
		operation.op = shape.tag;

		// What it holds, in words, for a mistake: "two or more <value>", "one
		// <value>", "two <value>", "a <value>, an <atleast> and an <atmost>".
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

			WrittenOperand operand;
			operand.role = expected;
			operand.value = readFormulaOperand(*child, here);
			operation.operands.push_back(std::move(operand));
		}

		if (operation.operands.size() < shape.fewest) { fail(here, "needs " + wanted); }

		return operation;
	}

	bool isOperation(const std::string& tag)
	{
		return tag != "random" && operationShape(tag) != nullptr;
	}

	WrittenValue readOperation(const XmlNode& tag, const std::string& where)
	{
		const OperationShape* shape = isOperation(tag.getName()) ? operationShape(tag.getName()) : nullptr;
		if (!shape) { fail(where, "has <" + tag.getName() + ">, which is not an operation; expected " + operationTags()); }

		WrittenValue value;
		value.kind = WrittenValue::Kind::Formula;
		value.operations = std::make_shared<std::vector<WrittenOperation>>(1, readFormulaOperation(tag, *shape, where));
		return value;
	}


	// A number, written either as an expression (the text of the element) or
	// as one value tag inside it - see WrittenValue - and whether it is
	// update="live".
	static WrittenValue readValueBody(const XmlNode& node, const std::string& where)
	{
		const std::string text = readText(node);
		const std::string here = where + " > <" + node.getName() + ">";
		std::unique_ptr<XmlNode> tag = node.getFirstChild();

		if (!tag)
		{
			if (text.empty()) { fail(here, "has no value"); }
			return WrittenValue::expression(writtenText(text, here));
		}

		if (!text.empty()) { fail(here, "holds both the text \"" + text + "\" and a <" + tag->getName() + "> tag; use one or the other"); }
		if (tag->getNextSibling()) { fail(here, "holds more than one value tag"); }

		const std::string tagName = tag->getName();

		if (tagName == "random") { return readRandom(*tag, here); }
		if (const std::string named = engineName(*tag, here); !named.empty()) { return WrittenValue::expression(named); }
		if (isOperation(tagName)) { return readOperation(*tag, here); }
		if (tagName == "centered")
		{
			WrittenValue value = WrittenValue::expression(writtenText(readText(*tag), here + " > <centered>"));
			value.kind = WrittenValue::Kind::Centered;
			// <centered><offset>-60</offset></centered>: that far from the window's
			// middle; <centered><divide>...</divide></centered>: the middle at what
			// the operation gives (its steps kept in `operations`).
			if (std::unique_ptr<XmlNode> inside = tag->getFirstChild())
			{
				const std::string centered = here + " > <centered>";
				if (inside->getNextSibling()) { fail(centered, "holds where the middle goes (a name, a number or an operation), one <offset> from the window's middle, or nothing for the window's middle"); }
				if (isOperation(inside->getName()))
				{
					if (!value.text.empty()) { fail(centered, "holds both the text \"" + value.text + "\" and an <" + inside->getName() + ">; use one or the other"); }
					value.operations = readOperation(*inside, centered).operations;
					return value;
				}
				if (const std::string named = engineName(*inside, centered); !named.empty())
				{
					if (!value.text.empty()) { fail(centered, "holds both the text \"" + value.text + "\" and a <" + inside->getName() + ">; use one or the other"); }
					value.text = named;
					return value;
				}
				if (inside->getName() != "offset" || inside->getFirstChild()) { fail(centered, "holds where the middle goes (a name, a number or an operation), one <offset> from the window's middle, or nothing for the window's middle"); }
				if (!value.text.empty()) { fail(centered, "holds both the text \"" + value.text + "\" and an <offset>; use one or the other"); }
				value.offset = writtenText(readText(*inside), centered + " > <offset>");
				if (value.offset.empty()) { fail(centered + " > <offset>", "has no value"); }
			}
			return value;
		}
		if (tagName == "left" || tagName == "right" || tagName == "top" || tagName == "bottom")
		{
			// <right><offset>12</offset></right>: the object's right side on
			// the window's, then 12 in from it.
			const std::string edgeHere = here + " > <" + tagName + ">";
			const bool across = tagName == "left" || tagName == "right";
			if (node.getName() != (across ? "x" : "y")) { fail(edgeHere, std::string("is a side of the window ") + (across ? "across, so it goes in an <x>" : "up and down, so it goes in a <y>")); }
			if (!readText(*tag).empty()) { fail(edgeHere, "holds only an <offset>, how far in from the window's side the object goes"); }
			WrittenValue value;
			value.kind = WrittenValue::Kind::Centered;
			value.edge = (tagName == "left" || tagName == "top") ? WrittenValue::Edge::Start : WrittenValue::Edge::End;
			if (std::unique_ptr<XmlNode> offset = tag->getFirstChild())
			{
				if (offset->getName() != "offset" || offset->getNextSibling() || offset->getFirstChild()) { fail(edgeHere, "holds only an <offset>, how far in from the window's side the object goes"); }
				value.offset = writtenText(readText(*offset), edgeHere + " > <offset>");
				if (value.offset.empty()) { fail(edgeHere + " > <offset>", "has no value"); }
			}
			return value;
		}

		fail(here, "unknown value tag <" + tagName + ">; a value is a number, a name, <random>, <window>, <self>, <other>, <others>, <centered>, a side (<left>, <right>, <top>, <bottom>) or an operation: " + operationTags());
	}

	WrittenValue readValue(const XmlNode& node, const std::string& where)
	{
		return onLineOf(node, [&]
		{
			WrittenValue value = readValueBody(node, where);
			value.live = node.getAttribute("update") == "live";
			return value;
		});
	}

	WrittenValue readValueOf(const XmlNode& parent, const std::string& name, const std::string& where)
	{
		return readValue(*requireChild(parent, name, where), where);
	}

	// The one number a verb holds as its content, as text or a value tag,
	// beside the named parts it may also have (`parts`, with `expected`
	// saying them for the message): <chase object="ship">2<near>40</near></chase>.
	WrittenValue readLeadValue(const XmlNode& node, std::initializer_list<const char*> parts, const std::string& expected, const std::string& where)
	{
		const std::string here = where + " > <" + node.getName() + ">";
		std::string valueTagName;
		for (std::unique_ptr<XmlNode> child = node.getFirstChild(); child != nullptr; child = child->getNextSibling())
		{
			const std::string tag = child->getName();
			if (tag == "random" || tag == "window" || isPronoun(tag) || isOperation(tag))
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
		if (!valueTag) { return WrittenValue::expression(writtenText(text, here)); }
		if (valueTag->getName() == "random") { return readRandom(*valueTag, here); }
		if (const std::string named = engineName(*valueTag, here); !named.empty()) { return WrittenValue::expression(named); }
		return readOperation(*valueTag, here);
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
