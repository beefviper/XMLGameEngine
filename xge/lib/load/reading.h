// reading.h
// XML Game Engine
// author: beefviper
// date: Nov 10, 2026

#pragma once

#include "game/command.h"
#include "xml/document.h"

#include <initializer_list>
#include <memory>
#include <optional>
#include <stdexcept>
#include <string>
#include <utility>

// What every reader of a game file's tags shares: finding a child, asking
// for an attribute, reading a value (a number, a <random>, an operation), and
// stopping with where it was when something is missing or wrong. load/xml.cpp
// reads the game with these, and each verb its own tag (verbs/, Verb::read).
// They need only the XmlNode every XML library is read through.

namespace xge
{
	std::string getAttribute(const XmlNode* node, const std::string& name);

	std::string trim(const std::string& text);

	// The first child of parent named `name`, or nullptr if parent is null
	// or none match. Schema elements have fixed names but a sequence still
	// lets some be left out (an object with no <actions>), so this looks a
	// child up by name rather than chaining getFirstChild()/getNextSibling()
	// calls that assume a particular position.
	std::unique_ptr<XmlNode> findChild(const XmlNode* parent, const std::string& name);

	// What fail() throws: what is wrong and where, and the line of the file
	// it is on once a reader that knows the line has said so (0 until then).
	class LoadError : public std::runtime_error
	{
	public:
		explicit LoadError(const std::string& message, int atLine = 0) : std::runtime_error(message), line(atLine) {}
		int line;
	};

	[[noreturn]] void fail(const std::string& where, const std::string& message);

	// Runs `work`, which is about what is written on `line` of the game file;
	// an error it throws that says no line yet gets this one, so the message
	// ends "(line 12)". The innermost that knows a line gives it, being
	// nearest the mistake. Line 0 (not known) adds nothing.
	template <typename Work>
	auto onLine(int line, Work&& work) -> decltype(work())
	{
		try
		{
			return work();
		}
		catch (const LoadError& error)
		{
			if (error.line != 0 || line == 0) { throw; }
			throw LoadError(error.what() + lineNote(line), line);
		}
		catch (const std::runtime_error& error)
		{
			// One from further down that already says its line (a merge's).
			const std::string message = error.what();
			if (line == 0 || message.find(" (line ") != std::string::npos) { throw; }
			throw LoadError(message + lineNote(line), line);
		}
	}

	// onLine for reading `node`: the line its tag starts on.
	template <typename Read>
	auto onLineOf(const XmlNode& node, Read&& read) -> decltype(read())
	{
		return onLine(node.line(), std::forward<Read>(read));
	}

	std::unique_ptr<XmlNode> requireChild(const XmlNode& parent, const std::string& name, const std::string& where);

	std::string requireAttribute(const XmlNode& node, const std::string& name, const std::string& where);

	// The text of a simple element such as <color>color.red</color>.
	std::string readText(const XmlNode& node);

	bool readBool(const XmlNode& node, const std::string& where);

	// <random><min>..</min><max>..</max></random>: each end an expression
	// or an operation. With an operation end, the two ends are kept as one
	// operation "random" (see RawValue::operations).
	RawValue readRandom(const XmlNode& tag, const std::string& here);

	// Whether text is a plain number (`2`, `-130`, `0.5`, `1e3`): digits, a
	// sign, a point and an exponent, nothing else, so that "inf" and "0x10"
	// are not.
	bool isNumber(const std::string& text);

	// Whether text is a name: a letter or underscore, then letters, digits,
	// underscores and (when dots is true) dots, like `window.width.center`.
	bool isName(const std::string& text, bool dots);

	// An operand of an operation is a name or a number and
	// nothing else: anything more is an operation of its own.
	std::string requireTerm(const std::string& text, const std::string& here);

	// An operand of an operation: <value>window.width.center</value>, or an
	// operation in place of the name, or a <random>.
	RawValue readFormulaOperand(const XmlNode& node, const std::string& where);

	// <subtract><value>a</value><value>b</value><value>c</value></subtract>:
	// the first operand, then the others taken from it one after another.
	RawOperation readFormulaOperation(const XmlNode& node, const OperationShape& shape, const std::string& where);

	// Whether a tag is an operation (<add>, <clamp>, ...; not <random>).
	bool isOperation(const std::string& tag);

	// An operation written straight in a value: <x><subtract><value>window.right
	// </value><value>4</value></subtract></x>, a value of kind Formula.
	RawValue readOperation(const XmlNode& tag, const std::string& where);

	// A number, written either as an expression (the text of the element) or
	// as one value tag inside it - see RawValue - and whether it is
	// update="live".
	RawValue readValue(const XmlNode& node, const std::string& where);

	RawValue readValueBody(const XmlNode& node, const std::string& where);

	RawValue readValueOf(const XmlNode& parent, const std::string& name, const std::string& where);

	// The one number a verb holds as its content, as text or a value tag,
	// beside the named parts it may also have (`parts`, with `expected`
	// saying them for the message): <chase object="ship">2<near>40</near></chase>.
	RawValue readLeadValue(const XmlNode& node, std::initializer_list<const char*> parts, const std::string& expected, const std::string& where);

	// Some of an <x>/<y> pair. A <group> gives what its members share and a
	// <member> only what differs, so either half may be left out of either;
	// an object's <velocity> may leave out a half that is 0.
	struct PartialVector2
	{
		std::optional<RawValue> x;
		std::optional<RawValue> y;
	};

	PartialVector2 readPartialVector2(const XmlNode& node, const std::string& where);

	// A whole number of at least 1 (a group's <columns> and <rows>): they
	// say how many cells there are and what they are called, so they are
	// settled as the file is read, not worked out like other values.
	int readCount(const XmlNode& node, const std::string& where);
}
