// xml.cpp
// XML Game Engine
// author: beefviper
// date: Sept 21, 2020

#include "load/xml.h"

#include "engine/folder.h"
#include "game/game.h"
#include "engine/keys.h"
#include "load/merge.h"
#include "load/spelling.h"
#include "load/xsd.h"

#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <filesystem>
#include <initializer_list>
#include <iostream>
#include <map>
#include <memory>
#include <optional>
#include <set>
#include <sstream>
#include <stdexcept>

namespace xge
{
	namespace
	{
		// What load/xml knows how to read is fixed by xgedef.xsd,
		// but not every file names that schema (and only Xerces really checks
		// against it), so every reader below also checks what it needs and
		// says where it was when something is missing or wrong.

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

		RawValue readFormula(const XmlNode& node, const std::string& where);

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

		RawOperation readFormulaOperation(const XmlNode& node, const OperationShape& shape, const std::string& where);

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
		// as one value tag inside it - see RawValue.
		RawValue readValue(const XmlNode& node, const std::string& where)
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

		RawVector2 readVector2(const XmlNode& node, const std::string& where)
		{
			const std::string here = where + " > <" + node.getName() + ">";
			RawVector2 vector;
			vector.x = readValueOf(node, "x", here);
			vector.y = readValueOf(node, "y", here);
			return vector;
		}

		bool isShapeTag(const std::string& name)
		{
			return name == "circle" || name == "rectangle" || name == "text" || name == "image" || name == "bitmap" || name == "svg" || name == "tether";
		}

		// The <flip> of a picture the engine draws itself (a <bitmap> or an
		// <svg>): "", "horizontal" or "vertical". Checked here, since only
		// Xerces would catch a wrong word from the schema.
		std::string readPictureFlip(const XmlNode& shape, const std::string& where)
		{
			const auto flip = findChild(&shape, "flip");
			if (!flip) { return {}; }

			const std::string text = readText(*flip);
			if (text != "horizontal" && text != "vertical")
			{
				fail(where, "<flip> is \"" + text + "\"; expected horizontal or vertical");
			}
			return text;
		}

		// The pieces of a <text>, in the order written: each <content>,
		// <number> and <letter>.
		std::vector<RawTextPiece> readTextPieces(const XmlNode& shape, const std::string& where)
		{
			std::vector<RawTextPiece> pieces;
			for (std::unique_ptr<XmlNode> child = shape.getFirstChild(); child != nullptr; child = child->getNextSibling())
			{
				const std::string tag = child->getName();
				RawTextPiece piece;
				if (tag == "content")
				{
					piece.words = readText(*child);
				}
				else if (tag == "number" || tag == "letter")
				{
					piece.kind = tag == "number" ? RawTextPiece::Kind::Number : RawTextPiece::Kind::Letter;
					piece.value = readValue(*child, where + " > <" + tag + ">");
				}
				else if (tag == "choose")
				{
					// <by> which, then its <word>s, the first 0.
					const std::string here = where + " > <choose>";
					piece.kind = RawTextPiece::Kind::Choice;
					piece.value = readValue(*requireChild(*child, "by", here), here);
					for (std::unique_ptr<XmlNode> word = child->getFirstChild(); word != nullptr; word = word->getNextSibling())
					{
						if (word->getName() == "word") { piece.choices.push_back(readText(*word)); }
					}
					if (piece.choices.empty()) { fail(here, "needs a <word> to choose from"); }
				}
				else
				{
					continue;
				}
				pieces.push_back(std::move(piece));
			}
			return pieces;
		}

		// A bitmap's <color char="w">s: a character and the color it draws,
		// each in place of an earlier one of the same character. One with no
		// char= is the color of the '*'s, read with the other shapes' <color>.
		void readBitmapColors(const XmlNode& shape, const std::string& where, std::vector<std::pair<char, std::string>>& colors)
		{
			bool plain = false;
			std::set<char> given;
			for (std::unique_ptr<XmlNode> node = shape.getFirstChild(); node != nullptr; node = node->getNextSibling())
			{
				if (node->getName() != "color") { continue; }
				const std::string character = node->getAttribute("char");
				if (character.empty())
				{
					if (plain) { fail(where, "has two <color>s with no char=; the one with none is the color of the '*'s"); }
					plain = true;
					continue;
				}
				if (character.size() != 1) { fail(where, "<color char=\"" + character + "\">: char= is one character"); }
				if (character == "." || character == "*")
				{
					fail(where, "<color char=\"" + character + "\">: '.' is always clear and '*' is the <color> with no char=; give another character");
				}
				if (!given.insert(character[0]).second) { fail(where, "has two <color char=\"" + character + "\">s"); }

				const auto same = std::find_if(colors.begin(), colors.end(), [&](const auto& each) { return each.first == character[0]; });
				if (same != colors.end()) { same->second = readText(*node); }
				else { colors.emplace_back(character[0], readText(*node)); }
			}
		}

		// The <color> of a shape: the one with no char= (a bitmap's others are
		// its other colors, readBitmapColors).
		std::unique_ptr<XmlNode> findPlainColor(const XmlNode& shape)
		{
			for (std::unique_ptr<XmlNode> node = shape.getFirstChild(); node != nullptr; node = node->getNextSibling())
			{
				if (node->getName() == "color" && node->getAttribute("char").empty()) { return node; }
			}
			return nullptr;
		}

		// One <circle>, <rectangle>, <text>, <image>, <bitmap> or <svg>, written into `sprite`.
		void readShape(const XmlNode& shape, RawSprite& sprite, const std::string& where)
		{
			const std::string kind = shape.getName();
			const std::string here = where + " > <" + kind + ">";

			sprite.kind = kind;

			if (kind == "circle")
			{
				sprite.radius = readValueOf(shape, "radius", here);
			}
			else if (kind == "rectangle")
			{
				sprite.width = readValueOf(shape, "width", here);
				sprite.height = readValueOf(shape, "height", here);
			}
			else if (kind == "bitmap")
			{
				// A picture in rows of text, '.' clear and '*' solid. Which
				// characters are allowed, and that the rows are one width, are
				// checked when it is drawn (picture/bitmap.cpp), with the row named.
				for (std::unique_ptr<XmlNode> row = shape.getFirstChild(); row != nullptr; row = row->getNextSibling())
				{
					if (row->getName() == "row") { sprite.bitmapRows.push_back(readText(*row)); }
				}
				if (sprite.bitmapRows.empty()) { fail(here, "needs at least one <row>"); }

				if (auto scale = findChild(&shape, "scale"))
				{
					sprite.hasScale = true;
					sprite.scale = readValue(*scale, here);
				}
				readBitmapColors(shape, here, sprite.bitmapColors);
				sprite.flip = readPictureFlip(shape, here);
				sprite.style = shape.getAttribute("style");
			}
			else if (kind == "svg")
			{
				// A drawing in an SVG file, made into a picture when the game
				// loads (picture/svg.cpp): the file, optionally the part of it to take
				// and how many pixels a unit of it is, and elements to leave out.
				sprite.path = readText(*requireChild(shape, "path", here));

				const auto x = findChild(&shape, "x");
				const auto y = findChild(&shape, "y");
				const auto width = findChild(&shape, "width");
				const auto height = findChild(&shape, "height");
				if (x || y || width || height)
				{
					if (!(x && y && width && height)) { fail(here, "takes a part of the drawing with all four of <x>, <y>, <width> and <height>, or none of them"); }

					sprite.hasSvgRegion = true;
					sprite.svgX = readValue(*x, here);
					sprite.svgY = readValue(*y, here);
					sprite.svgWidth = readValue(*width, here);
					sprite.svgHeight = readValue(*height, here);
				}

				if (auto scale = findChild(&shape, "scale"))
				{
					sprite.hasScale = true;
					sprite.scale = readValue(*scale, here);
				}

				for (std::unique_ptr<XmlNode> hide = shape.getFirstChild(); hide != nullptr; hide = hide->getNextSibling())
				{
					if (hide->getName() == "hide") { sprite.svgHide.push_back(readText(*hide)); }
				}
				sprite.flip = readPictureFlip(shape, here);
			}
			else if (kind == "tether")
			{
				// A line joining two objects' middles, wherever they go.
				sprite.tetherFrom = shape.getAttribute("from");
				sprite.tetherTo = shape.getAttribute("to");
				if (sprite.tetherFrom.empty() || sprite.tetherTo.empty()) { fail(here, "names the two objects it joins, from=\"...\" and to=\"...\""); }
				if (auto thickness = findChild(&shape, "thickness")) { sprite.thickness = readValue(*thickness, here); }
			}
			else if (kind == "text")
			{
				sprite.text = readTextPieces(shape, here);
				if (sprite.text.empty())
				{
					fail(here, "needs a <content> (a label), a <number>, a <letter> or a <choose>");
				}

				sprite.style = shape.getAttribute("style");
				if (auto size = findChild(&shape, "size")) { sprite.size = readValue(*size, here); }
				else if (sprite.style.empty()) { fail(here, "needs a <size>, or a style= whose <style> has one"); }
				if (auto font = findChild(&shape, "font")) { sprite.font = readText(*font); }
			}
			else
			{
				sprite.path = readText(*requireChild(shape, "path", here));
				if (auto flip = findChild(&shape, "flip")) { sprite.flip = readText(*flip); }
			}

			if (kind != "image" && kind != "svg")
			{
				if (auto color = findPlainColor(shape)) { sprite.color = readText(*color); }
			}
		}

		// One <line>: where it goes from and to, and optionally its color and
		// how many pixels thick it is.
		RawLine readLine(const XmlNode& line, const std::string& where)
		{
			const std::string here = where + " > <line>";
			RawLine rawLine;

			rawLine.from = readVector2(*requireChild(line, "from", here), here);
			rawLine.to = readVector2(*requireChild(line, "to", here), here);

			if (auto color = findChild(&line, "color")) { rawLine.color = readText(*color); }
			if (auto thickness = findChild(&line, "thickness"))
			{
				rawLine.hasThickness = true;
				rawLine.thickness = readValue(*thickness, here);
			}

			return rawLine;
		}

		RawSprite readSprite(const XmlNode& spriteNode, const std::string& where)
		{
			const std::string here = where + " > <sprite>";
			RawSprite sprite;
			sprite.name = spriteNode.getAttribute("name");

			std::unique_ptr<XmlNode> first = spriteNode.getFirstChild();
			if (!first) { fail(here, "is empty; expected a shape"); }

			if (first->getName() == "line")
			{
				// A drawing: every <line> of the sprite, drawn in the order written.
				sprite.kind = "line";

				for (std::unique_ptr<XmlNode> node = std::move(first); node != nullptr; node = node->getNextSibling())
				{
					if (node->getName() != "line") { fail(here, "a sprite of lines holds only <line>s, not <" + node->getName() + ">"); }
					sprite.lines.push_back(readLine(*node, here));
				}
			}
			else if (isShapeTag(first->getName()))
			{
				readShape(*first, sprite, here);
			}
			else
			{
				fail(here, "unknown shape <" + first->getName() + ">");
			}

			return sprite;
		}

		// A <sprite> that changes one a group already has, for a member, row,
		// column or cell of it. Of the same shape it need give only what
		// changes (a <color>, say) and keeps the rest; of another shape, or as
		// a drawing of lines, it is a sprite of its own and must be complete.
		RawSprite changeSprite(const XmlNode& spriteNode, const RawSprite& base, const std::string& where)
		{
			const std::unique_ptr<XmlNode> shape = spriteNode.getFirstChild();
			if (!shape || shape->getName() != base.kind || base.kind == "line") { return readSprite(spriteNode, where); }

			const std::string here = where + " > <sprite> > <" + base.kind + ">";
			RawSprite sprite = base;

			const auto change = [&](const char* tag, RawValue& value)
			{
				if (auto node = findChild(shape.get(), tag)) { value = readValue(*node, here); }
			};

			if (base.kind == "circle")
			{
				change("radius", sprite.radius);
			}
			else if (base.kind == "rectangle")
			{
				change("width", sprite.width);
				change("height", sprite.height);
			}
			else if (base.kind == "tether")
			{
				// The two it joins are named again; only the line's look carries over.
				sprite.tetherFrom = shape->getAttribute("from");
				sprite.tetherTo = shape->getAttribute("to");
				if (sprite.tetherFrom.empty() || sprite.tetherTo.empty()) { fail(here, "names the two objects it joins, from=\"...\" and to=\"...\""); }
				change("thickness", sprite.thickness);
			}
			else if (base.kind == "text")
			{
				// Pieces given are the whole text; none, and it is kept.
				if (std::vector<RawTextPiece> text = readTextPieces(*shape, here); !text.empty())
				{
					sprite.text = std::move(text);
				}
				change("size", sprite.size);
				if (auto font = findChild(shape.get(), "font")) { sprite.font = readText(*font); }
				if (const std::string style = shape->getAttribute("style"); !style.empty()) { sprite.style = style; }
			}
			else if (base.kind == "image")
			{
				if (auto path = findChild(shape.get(), "path")) { sprite.path = readText(*path); }
				if (auto flip = findChild(shape.get(), "flip")) { sprite.flip = readText(*flip); }
			}
			else if (base.kind == "bitmap" || base.kind == "svg")
			{
				// New <row>s are a new picture, all of it; the same for <hide>s.
				std::vector<std::string> rows;
				std::vector<std::string> hides;
				for (std::unique_ptr<XmlNode> node = shape->getFirstChild(); node != nullptr; node = node->getNextSibling())
				{
					if (node->getName() == "row") { rows.push_back(readText(*node)); }
					else if (node->getName() == "hide") { hides.push_back(readText(*node)); }
				}
				if (!rows.empty()) { sprite.bitmapRows = std::move(rows); }
				if (!hides.empty()) { sprite.svgHide = std::move(hides); }

				if (auto path = findChild(shape.get(), "path")) { sprite.path = readText(*path); }

				const auto x = findChild(shape.get(), "x");
				const auto y = findChild(shape.get(), "y");
				const auto width = findChild(shape.get(), "width");
				const auto height = findChild(shape.get(), "height");
				if (x || y || width || height)
				{
					// Changing the part taken may give only what changes; taking
					// a part of what was the whole drawing needs all four.
					if (!sprite.hasSvgRegion && !(x && y && width && height)) { fail(here, "takes a part of the drawing with all four of <x>, <y>, <width> and <height>, or none of them"); }

					sprite.hasSvgRegion = true;
					if (x) { sprite.svgX = readValue(*x, here); }
					if (y) { sprite.svgY = readValue(*y, here); }
					if (width) { sprite.svgWidth = readValue(*width, here); }
					if (height) { sprite.svgHeight = readValue(*height, here); }
				}

				if (auto scale = findChild(shape.get(), "scale"))
				{
					sprite.hasScale = true;
					sprite.scale = readValue(*scale, here);
				}
				if (base.kind == "bitmap")
				{
					readBitmapColors(*shape, here, sprite.bitmapColors);
					if (const std::string style = shape->getAttribute("style"); !style.empty()) { sprite.style = style; }
				}
				if (findChild(shape.get(), "flip")) { sprite.flip = readPictureFlip(*shape, here); }
			}

			if (base.kind != "image" && base.kind != "svg")
			{
				if (auto color = findPlainColor(*shape)) { sprite.color = readText(*color); }
			}

			return sprite;
		}

		// The group's sprites as a member, row, column or cell (`node`) has
		// them: each changed by the <sprite> of its name that node gives (one
		// with no name changes the group's one with no name), and any sprite
		// node names that the group has not, added. The names of those it gave
		// go in `given`.
		std::vector<RawSprite> changeSprites(const std::vector<RawSprite>& base, const XmlNode& node, const std::string& where,
			std::set<std::string>* given = nullptr)
		{
			std::vector<RawSprite> sprites = base;
			std::set<std::string> names;

			for (std::unique_ptr<XmlNode> child = node.getFirstChild(); child != nullptr; child = child->getNextSibling())
			{
				if (child->getName() != "sprite") { continue; }

				const std::string name = child->getAttribute("name");
				if (!names.insert(name).second) { fail(where, name.empty() ? "has two <sprite>s with no name" : "has two <sprite>s called \"" + name + "\""); }

				const auto found = std::find_if(sprites.begin(), sprites.end(), [&](const RawSprite& sprite) { return sprite.name == name; });
				if (found != sprites.end()) { *found = changeSprite(*child, *found, where); }
				else { sprites.push_back(readSprite(*child, where)); }
			}

			if (given) { given->insert(names.begin(), names.end()); }
			return sprites;
		}

		// Every <sprite> child of `parent`, in the order written.
		std::vector<RawSprite> readSprites(const XmlNode& parent, const std::string& where)
		{
			std::vector<RawSprite> sprites;

			for (std::unique_ptr<XmlNode> node = parent.getFirstChild(); node != nullptr; node = node->getNextSibling())
			{
				if (node->getName() == "sprite") { sprites.push_back(readSprite(*node, where)); }
			}

			return sprites;
		}

		// An <animation> as written (RawFrames). Which sprites its frames are
		// is settled by chooseSprite, once the sprites it can pick from are
		// known (a group's member can bring its own).
		using AnimationSpec = RawFrames;

		AnimationSpec readAnimation(const XmlNode& animation, const std::string& where)
		{
			AnimationSpec spec;
			spec.name = animation.getAttribute("name");
			const std::string here = where + " > <animation" + (spec.name.empty() ? std::string{} : " name=\"" + spec.name + "\"") + ">";

			spec.interval = readValueOf(animation, "interval", here);

			for (std::unique_ptr<XmlNode> node = animation.getFirstChild(); node != nullptr; node = node->getNextSibling())
			{
				if (node->getName() == "frame") { spec.frames.push_back(requireAttribute(*node, "sprite", here + " > <frame>")); }
				else if (node->getName() != "interval") { fail(here, "unknown <" + node->getName() + ">; expected an <interval> and then <frame>s"); }
			}

			return spec;
		}

		// All of `given` added to `animations`, each in place of the one of
		// its name (the unnamed one in place of the unnamed one).
		void mergeAnimations(std::vector<AnimationSpec>& animations, const std::vector<AnimationSpec>& given)
		{
			for (const AnimationSpec& animation : given)
			{
				const auto same = std::find_if(animations.begin(), animations.end(), [&](const AnimationSpec& each) { return each.name == animation.name; });
				if (same == animations.end()) { animations.push_back(animation); }
				else { *same = animation; }
			}
		}

		// Every <animation> of `node`, in the order written; two of one name
		// (or two with none) stop the load.
		std::vector<AnimationSpec> readAnimations(const XmlNode& node, const std::string& where)
		{
			std::vector<AnimationSpec> animations;
			for (std::unique_ptr<XmlNode> child = node.getFirstChild(); child != nullptr; child = child->getNextSibling())
			{
				if (child->getName() != "animation") { continue; }
				AnimationSpec animation = readAnimation(*child, where);
				if (std::any_of(animations.begin(), animations.end(), [&](const AnimationSpec& each) { return each.name == animation.name; }))
				{
					fail(where, animation.name.empty() ? std::string("has two <animation>s; it has one, or each is named as a look") : "two <animation>s are called \"" + animation.name + "\"");
				}
				animations.push_back(std::move(animation));
			}
			return animations;
		}

		// Settles what `object` looks like from the <sprite>s it has and its
		// <animation>s, if any. One sprite and no animation is the ordinary
		// case. With one unnamed animation, each <frame> names one of the
		// sprites, and the object is the first of them until it moves on. With
		// named animations, each is a look that moves (lookWithAnimations).
		// Anything that cannot be settled, such as several sprites and nothing
		// to say which is shown when, or a frame naming a sprite that is not
		// there, stops the load.
		void chooseSprite(const std::vector<RawSprite>& sprites, const std::vector<AnimationSpec>& animations,
			const std::string& where, RawObject& object)
		{
			if (sprites.empty()) { fail(where, "missing <sprite>"); }

			const bool named = std::any_of(animations.begin(), animations.end(), [](const AnimationSpec& each) { return !each.name.empty(); });
			if (named)
			{
				if (std::any_of(animations.begin(), animations.end(), [](const AnimationSpec& each) { return each.name.empty(); }))
				{
					fail(where, "has an <animation> with no name beside named ones; name it, as a look that moves, or leave the names off for one animation of the whole object");
				}
				lookWithAnimations(object, sprites, animations, where);
				return;
			}
			if (animations.size() > 1) { fail(where, "has two <animation>s; it has one, or each is named as a look"); }
			const std::optional<AnimationSpec> animation = animations.empty() ? std::nullopt : std::optional<AnimationSpec>(animations.front());

			for (std::size_t i = 0; i < sprites.size(); ++i)
			{
				if (sprites[i].name.empty()) { continue; }

				for (std::size_t j = i + 1; j < sprites.size(); ++j)
				{
					if (sprites[j].name == sprites[i].name) { fail(where, "two <sprite>s are called \"" + sprites[i].name + "\""); }
				}
			}

			if (!animation)
			{
				// Several sprites and no animation: the object's looks, which
				// <become> switches between; it starts as the first. Each needs a
				// name to be asked for by.
				if (sprites.size() > 1)
				{
					for (const RawSprite& sprite : sprites)
					{
						if (sprite.name.empty())
						{
							fail(where, "has " + std::to_string(sprites.size()) + " <sprite>s and no <animation>, so they are looks for <become>, and every one needs a name");
						}
					}
					object.looks = sprites;
				}

				object.sprite = sprites.front();
				return;
			}

			if (animation->frames.size() < 2) { fail(where, "<animation> needs at least two <frame>s"); }

			RawAnimation chosen;
			chosen.interval = animation->interval;

			std::vector<bool> used(sprites.size(), false);
			for (const std::string& name : animation->frames)
			{
				const auto found = std::find_if(sprites.begin(), sprites.end(), [&](const RawSprite& sprite) { return sprite.name == name; });
				if (found == sprites.end()) { fail(where, "<animation> > <frame sprite=\"" + name + "\">: no <sprite name=\"" + name + "\"> to show"); }

				used[static_cast<std::size_t>(found - sprites.begin())] = true;
				chosen.frames.push_back(*found);
			}

			for (std::size_t i = 0; i < sprites.size(); ++i)
			{
				if (!used[i])
				{
					fail(where, sprites[i].name.empty() ? "a <sprite> with no name is never shown by the <animation>"
						: "<sprite name=\"" + sprites[i].name + "\"> is never shown by the <animation>");
				}
			}

			object.hasAnimation = true;
			object.sprite = chosen.frames.front();
			object.animation = std::move(chosen);
		}

		// Some of an <x>/<y> pair. A <group> gives what its members share and a
		// <member> only what differs, so either half may be left out of either;
		// an object's <velocity> may leave out a half that is 0.
		struct PartialVector2
		{
			std::optional<RawValue> x;
			std::optional<RawValue> y;
		};

		PartialVector2 readPartialVector2(const XmlNode& node, const std::string& where)
		{
			const std::string here = where + " > <" + node.getName() + ">";
			PartialVector2 vector;
			if (auto x = findChild(&node, "x")) { vector.x = readValue(*x, here); }
			if (auto y = findChild(&node, "y")) { vector.y = readValue(*y, here); }
			return vector;
		}

		// An object's <pull>s and its <seek>, each naming what with object=.
		void readPullAndSeek(const XmlNode& object, RawObject& rawObject, const std::string& where)
		{
			for (std::unique_ptr<XmlNode> child = object.getFirstChild(); child != nullptr; child = child->getNextSibling())
			{
				const std::string tag = child->getName();
				if (tag != "pull" && tag != "seek") { continue; }
				requireAttribute(*child, "object", where + " > <" + tag + ">");
				if (tag == "pull")
				{
					rawObject.rawPulls.emplace_back(child->getAttribute("object"), readValue(*child, where + " > <pull>"));
				}
				else
				{
					rawObject.seekTarget = child->getAttribute("object");
					rawObject.rawSeek = readValue(*child, where + " > <seek>");
				}
			}
		}

		// An object's <light>, <dark>, <altitude> and <shadow>.
		void readSight(const XmlNode& object, RawObject& rawObject, const std::string& where)
		{
			if (auto light = findChild(&object, "light"))
			{
				rawObject.hasLight = true;
				rawObject.rawLight = readValue(*light, where + " > <light>");
			}
			if (auto dark = findChild(&object, "dark"))
			{
				rawObject.dark = readText(*dark);
				if (rawObject.dark != "unlit" && rawObject.dark != "unseen")
				{
					fail(where + " > <dark>", "is '" + rawObject.dark + "'; expected unlit (shown only while lit) or unseen (shown once it has been lit)");
				}
			}
			if (auto altitude = findChild(&object, "altitude"))
			{
				const std::string here = where + " > <altitude>";
				rawObject.hasAltitude = true;
				for (std::unique_ptr<XmlNode> child = altitude->getFirstChild(); child != nullptr; child = child->getNextSibling())
				{
					const std::string tag = child->getName();
					if (tag == "start") { rawObject.rawAltitude = readValue(*child, here); }
					else if (tag == "rise") { rawObject.rawRise = readValue(*child, here); }
					else if (tag == "fall") { rawObject.rawFall = readValue(*child, here); }
					else if (tag == "bounce") { rawObject.rawAltitudeBounce = readValue(*child, here); }
					else { fail(here, "unknown <" + tag + ">; expected <start>, <rise>, <fall> or <bounce>"); }
				}
			}
			if (auto shadow = findChild(&object, "shadow"))
			{
				requireAttribute(*shadow, "object", where + " > <shadow>");
				rawObject.shadowOf = shadow->getAttribute("object");
			}
		}

		// What a <hop> says besides its direction: one distance as its
		// content, or an <x> across and a <y> up or down; then how long it
		// takes, its <seconds>, and how high it goes, its <arc>. True for an
		// <x> and a <y>.
		bool readHop(const XmlNode& node, const std::string& where, RawCommand& command)
		{
			const std::string here = where + " > <hop>";
			if (auto seconds = findChild(&node, "seconds")) { command.seconds = readValue(*seconds, here); }
			if (auto arc = findChild(&node, "arc")) { command.arc = readValue(*arc, here); }

			if (!findChild(&node, "x") && !findChild(&node, "y"))
			{
				command.amount = readLeadValue(node, { "seconds", "arc" }, "its distance, or an <x> and a <y>, then <seconds> and <arc> if wanted", where);
				return false;
			}

			for (std::unique_ptr<XmlNode> child = node.getFirstChild(); child != nullptr; child = child->getNextSibling())
			{
				const std::string tag = child->getName();
				if (tag != "x" && tag != "y" && tag != "seconds" && tag != "arc")
				{
					fail(here, "unknown <" + tag + ">; expected its distance, or an <x> and a <y>, then <seconds> and <arc> if wanted");
				}
			}
			const PartialVector2 step = readPartialVector2(node, where);
			if (!step.x || !step.y || !readText(node).empty()) { fail(where, "<hop> gives one distance, or an <x> across and a <y> up or down, both"); }
			command.positionX = *step.x;
			command.positionY = *step.y;
			return true;
		}

		bool isCommandTag(const std::string& name)
		{
			return name == "bounce" || name == "deflect" || name == "knock" || name == "hole" || name == "hold" || name == "stick" || name == "wrap" || name == "ride" || name == "carry" || name == "drop" || name == "track" || name == "die"
				|| name == "reset" || name == "inc" || name == "dec" || name == "copy" || name == "rank" || name == "turns" || name == "pass" || name == "out" || name == "demo" || name == "move" || name == "hop"
				|| name == "accelerate" || name == "turn" || name == "thrust" || name == "release" || name == "stop"
				|| name == "push" || name == "pop" || name == "fire" || name == "trigger" || name == "play" || name == "silence"
				|| name == "jump" || name == "land" || name == "leap" || name == "climb" || name == "chase" || name == "go" || name == "hunt" || name == "wander" || name == "aim" || name == "reverse" || name == "become" || name == "reveal" || name == "follow" || name == "shift" || name == "enter" || name == "set" || name == "spawn" || name == "despawn";
		}

		RawCommand readCommand(const XmlNode& node, const std::string& where)
		{
			RawCommand command;
			command.verb = node.getName();

			if (!isCommandTag(command.verb)) { fail(where, "unknown command <" + command.verb + ">"); }

			command.object = node.getAttribute("object");
			command.variable = node.getAttribute("variable");
			command.state = node.getAttribute("state");
			command.action = node.getAttribute("action");
			command.direction = node.getAttribute("direction");
			command.burn = node.getAttribute("burn");
			command.sound = node.getAttribute("sound");
			command.sprite = node.getAttribute("sprite");
			command.style = node.getAttribute("style");
			command.path = node.getAttribute("path");
			command.objClass = node.getAttribute("class");
			command.scene = node.getAttribute("scene");
			command.way = node.getAttribute("way");
			command.into = node.getAttribute("into");
			command.from = node.getAttribute("from");

			const std::string& verb = command.verb;
			if (verb == "inc" || verb == "dec")
			{
				requireAttribute(node, "variable", where);

				// The amount is optional: a bare <inc variable="..." /> is 1.
				if (node.getFirstChild() || !readText(node).empty()) { command.amount = readValue(node, where); }
				else { command.amount = RawValue::expression("1"); }
			}
			if (verb == "set")
			{
				requireAttribute(node, "variable", where);
				command.amount = readValue(node, where);
			}
			if (verb == "spawn")
			{
				requireAttribute(node, "object", where);
				const std::string here = where + " > <spawn>";
				for (std::unique_ptr<XmlNode> child = node.getFirstChild(); child != nullptr; child = child->getNextSibling())
				{
					if (child->getName() != "position" && child->getName() != "velocity")
					{
						fail(here, "unknown <" + child->getName() + ">; expected <position> and <velocity>, each if wanted");
					}
				}
				if (auto position = findChild(&node, "position"))
				{
					const PartialVector2 given = readPartialVector2(*position, here);
					if (given.x) { command.positionX = *given.x; }
					if (given.y) { command.positionY = *given.y; }
				}
				if (auto velocity = findChild(&node, "velocity"))
				{
					const PartialVector2 given = readPartialVector2(*velocity, here);
					if (given.x) { command.velocityX = *given.x; }
					if (given.y) { command.velocityY = *given.y; }
				}
			}
			if (verb == "copy" || verb == "rank") { requireAttribute(node, "variable", where); requireAttribute(node, "into", where); }
			if (verb == "push") { requireAttribute(node, "state", where); }
			if (verb == "enter" && command.scene.empty() == command.way.empty())
			{
				fail(where, "<enter> names a scene=\"...\" or a way=\"...\", one of them");
			}
			if (verb == "reset" && (!command.object.empty()) + (!command.scene.empty()) + (!command.way.empty()) > 1)
			{
				fail(where, "<reset> puts back an object=, a scene= or a way=, one of them");
			}
			if (verb == "fire") { requireAttribute(node, "object", where); }
			if (verb == "play" || verb == "silence") { requireAttribute(node, "sound", where); }
			if (verb == "become" && command.sprite.empty() && command.style.empty())
			{
				fail(where, "<become> needs a sprite= (a look), a style= (a palette), or both");
			}
			if (verb == "reveal")
			{
				requireAttribute(node, "object", where);

				// How many is optional: a bare <reveal object="..." /> is one.
				if (node.getFirstChild() || !readText(node).empty()) { command.amount = readValue(node, where); }
				else { command.amount = RawValue::expression("1"); }
			}
			if (verb == "trigger") { requireAttribute(node, "action", where); }
			if (verb == "hop")
			{
				requireAttribute(node, "direction", where);
				if (readHop(node, where, command) && command.direction.find('-') == std::string::npos)
				{
					fail(where, "<hop direction=\"" + command.direction + "\"> is not diagonal; it gives one distance");
				}
			}
			else if (verb == "move" || verb == "accelerate" || verb == "turn")
			{
				requireAttribute(node, "direction", where);
				command.amount = readValue(node, where);
			}
			if (verb == "thrust" || verb == "deflect" || verb == "leap") { command.amount = readValue(node, where); }
			if (verb == "knock" && (node.getFirstChild() || !readText(node).empty())) { command.amount = readValue(node, where); }
			if (verb == "climb")
			{
				requireAttribute(node, "direction", where);
				requireAttribute(node, "class", where);
				command.amount = readValue(node, where);
			}
			if (verb == "jump")
			{
				// How far as its content, then how long and how high.
				requireAttribute(node, "direction", where);
				const std::string here = where + " > <jump>";
				command.amount = readLeadValue(node, { "seconds", "arc" }, "its distance, then <seconds> and <arc>, each if wanted", where);
				if (auto seconds = findChild(&node, "seconds")) { command.seconds = readValue(*seconds, here); }
				if (auto arc = findChild(&node, "arc")) { command.arc = readValue(*arc, here); }
			}
			if (verb == "aim")
			{
				// Optionally only when lined up, within so many pixels.
				requireAttribute(node, "object", where);
				if (auto lined = findChild(&node, "lined")) { command.distance = readValue(*lined, where + " > <aim>"); }
			}
			if (verb == "chase")
			{
				// Its speed as its content, then how near is near enough.
				requireAttribute(node, "object", where);
				const std::string here = where + " > <chase>";
				command.amount = readLeadValue(node, { "near" }, "its speed, then <near> if wanted", where);
				if (auto nearNode = findChild(&node, "near")) { command.distance = readValue(*nearNode, here); }
			}
			if (verb == "go")
			{
				requireAttribute(node, "direction", where);
				command.amount = readValue(node, where);
			}
			if (verb == "hold") { command.button = requireAttribute(node, "button", where); }
			if (verb == "hole")
			{
				// Its radius as its content, then how rough its edge is.
				const std::string here = where + " > <hole>";
				command.amount = readLeadValue(node, { "rough" }, "its radius, then <rough> if wanted", where);
				if (auto roughNode = findChild(&node, "rough")) { command.distance = readValue(*roughNode, here); }
			}
			if (verb == "wander")
			{
				command.amount = readValue(node, where);
			}
			if (verb == "turns")
			{
				// A <player> for each player, each with the state (if any) that
				// says it is their turn.
				const std::string here = where + " > <turns>";
				for (std::unique_ptr<XmlNode> child = node.getFirstChild(); child != nullptr; child = child->getNextSibling())
				{
					if (child->getName() != "player") { fail(here, "unknown <" + child->getName() + ">; expected a <player> for each player"); }
					command.players.push_back(child->getAttribute("state"));
				}
				if (command.players.empty()) { fail(here, "has no <player>; it needs one for each player"); }
			}
			if (verb == "hunt" && findChild(&node, "hop"))
			{
				// A hunt by hops: the one hop it makes toward what it hunts.
				requireAttribute(node, "object", where);
				const std::string here = where + " > <hunt>";
				std::unique_ptr<XmlNode> hop = node.getFirstChild();
				if (hop->getName() != "hop" || hop->getNextSibling() || !readText(node).empty())
				{
					fail(here, "a hunt by hops has its <hop> and nothing else (no speed, <ahead>, <with> or <retreat>: those are for a maze)");
				}
				if (!hop->getAttribute("direction").empty()) { fail(here + " > <hop>", "has a direction=; the hunt chooses the way"); }
				command.byHops = true;
				readHop(*hop, here, command);
			}
			else if (verb == "hunt")
			{
				// What it hunts, how fast, and how its target is found from that:
				// tiles ahead of it, with a partner, or backing off when near.
				requireAttribute(node, "object", where);
				const std::string here = where + " > <hunt>";
				for (std::unique_ptr<XmlNode> child = node.getFirstChild(); child != nullptr; child = child->getNextSibling())
				{
					const std::string tag = child->getName();
					if (tag == "with") { command.partner = requireAttribute(*child, "object", here + " > <with>"); }
					else if (tag == "retreat")
					{
						command.retreat = requireAttribute(*child, "object", here + " > <retreat>");
						command.distance = readValue(*child, here + " > <retreat>");
					}
				}
				command.amount = readLeadValue(node, { "ahead", "with", "retreat" }, "its speed, then <ahead>, <with> and <retreat>, each if wanted", where);
				if (auto ahead = findChild(&node, "ahead")) { command.ahead = readValue(*ahead, here); }
			}
			if (verb == "follow")
			{
				requireAttribute(node, "path", where);
				const std::string here = where + " > <follow>";
				for (std::unique_ptr<XmlNode> child = node.getFirstChild(); child != nullptr; child = child->getNextSibling())
				{
					if (child->getName() != "stagger" && child->getName() != "nearest") { fail(here, "unknown <" + child->getName() + ">; expected <stagger> and <nearest>"); }
				}
				if (auto stagger = findChild(&node, "stagger")) { command.seconds = readValue(*stagger, here); }
				if (auto nearest = findChild(&node, "nearest"))
				{
					if (node.getAttribute("object").empty()) { fail(here, "<nearest> picks among the objects object= names; this <follow> names none"); }
					command.amount = readValue(*nearest, here);
				}
			}
			if (verb == "shift")
			{
				requireAttribute(node, "object", where);
				const PartialVector2 by = readPartialVector2(node, where);
				if (!by.x && !by.y) { fail(where, "<shift> moves by an <x> across, a <y> down, or both; it has neither"); }
				if (by.x) { command.positionX = *by.x; }
				if (by.y) { command.positionY = *by.y; }
			}
			if (verb == "release")
			{
				requireAttribute(node, "object", where);

				// How many is optional: a bare <release object="..." /> is one.
				if (node.getFirstChild() || !readText(node).empty()) { command.amount = readValue(node, where); }
				else { command.amount = RawValue::expression("1"); }
			}

			return command;
		}

		// Every command from `first` on, in the order written.
		std::vector<RawCommand> readCommands(std::unique_ptr<XmlNode> first, const std::string& where)
		{
			std::vector<RawCommand> commands;

			for (std::unique_ptr<XmlNode> node = std::move(first); node != nullptr; node = node->getNextSibling())
			{
				commands.push_back(readCommand(*node, where));
			}

			return commands;
		}

		// <timers>: each <timer> an <every> or an <after>, then its commands.
		// An object's timer may follow one of its looks (sprite="blue"); a
		// state or a scene has no looks (`ofObject` false).
		std::vector<RawTimer> readTimers(const XmlNode* timersNode, const std::string& where, bool ofObject = true)
		{
			std::vector<RawTimer> timers;
			if (!timersNode) { return timers; }

			for (std::unique_ptr<XmlNode> timer = timersNode->getFirstChild(); timer != nullptr; timer = timer->getNextSibling())
			{
				const std::string here = where + " > <timer>";
				std::unique_ptr<XmlNode> when = timer->getFirstChild();
				if (!when || (when->getName() != "every" && when->getName() != "after"))
				{
					fail(here, "needs an <every> or an <after> first: how many seconds");
				}

				RawTimer raw;
				raw.name = timer->getAttribute("name");
				if (!raw.name.empty() && std::any_of(timers.begin(), timers.end(), [&](const RawTimer& other) { return other.name == raw.name; }))
				{
					fail(here, "has two timers called '" + raw.name + "'");
				}
				raw.sprite = timer->getAttribute("sprite");
				if (!raw.sprite.empty() && !ofObject)
				{
					fail(here, "sprite=\"" + raw.sprite + "\": only an object's timer follows a look; a state or a scene has none");
				}
				raw.repeat = when->getName() == "every";
				raw.interval = readValue(*when, here);
				raw.commands = readCommands(when->getNextSibling(), here);
				timers.push_back(std::move(raw));
			}

			return timers;
		}

		// A <path>: its <speed>, maybe a <start>, then its <step>s and <home />s
		// in the order flown.
		RawPath readPath(const XmlNode& node)
		{
			RawPath path;
			path.name = requireAttribute(node, "name", "<paths> > <path>");
			const std::string where = "path '" + path.name + "'";

			for (std::unique_ptr<XmlNode> child = node.getFirstChild(); child != nullptr; child = child->getNextSibling())
			{
				const std::string name = child->getName();
				if (name == "speed")
				{
					path.speed = readValue(*child, where + " > <speed>");
				}
				else if (name == "start")
				{
					const RawVector2 start = readVector2(*child, where + " > <start>");
					path.hasStart = true;
					path.startX = start.x;
					path.startY = start.y;
				}
				else if (name == "home")
				{
					RawPathStep step;
					step.home = true;
					path.steps.push_back(std::move(step));
				}
				else if (name == "step")
				{
					const std::string here = where + " > <step>";
					RawPathStep step;
					step.x = readValueOf(*child, "x", here);
					step.y = readValueOf(*child, "y", here);

					// The commands come after the <x> and the <y>.
					std::unique_ptr<XmlNode> first = child->getFirstChild();
					while (first && (first->getName() == "x" || first->getName() == "y"))
					{
						first = first->getNextSibling();
					}
					step.commands = readCommands(std::move(first), here);
					path.steps.push_back(std::move(step));
				}
				else
				{
					fail(where, "unknown <" + name + ">; expected <speed>, <start>, <step> and <home />");
				}
			}

			if (path.speed.kind == RawValue::Kind::Expression && path.speed.text.empty()) { fail(where, "missing <speed>"); }
			if (path.steps.empty()) { fail(where, "has no <step> or <home />"); }
			return path;
		}

		// What an object's looks and steps do as it moves, as far as `node`
		// says: its <poses>, <mirror> and <grip>.
		void readMoves(const XmlNode& node, RawObject& object, const std::string& where)
		{
			if (auto poses = findChild(&node, "poses"))
			{
				const std::string here = where + " > <poses>";
				object.poses.clear();
				for (std::unique_ptr<XmlNode> pose = poses->getFirstChild(); pose != nullptr; pose = pose->getNextSibling())
				{
					const std::string tag = pose->getName();
					if (tag != "standing" && tag != "walking" && tag != "skidding" && tag != "rising" && tag != "falling" && tag != "thrusting")
					{
						fail(here, "unknown <" + tag + ">; expected <standing>, then any of <walking>, <skidding>, <rising>, <falling> and <thrusting>");
					}
					if (object.poses.count(tag)) { fail(here, "has two <" + tag + ">s"); }
					object.poses[tag] = requireAttribute(*pose, "sprite", here + " > <" + tag + ">");
				}
				if (!object.poses.count("standing")) { fail(here, "missing <standing>: the look when it is not moving, which the others fall back to"); }
			}
			if (auto mirror = findChild(&node, "mirror"))
			{
				object.mirror = readText(*mirror);
				if (object.mirror != "left" && object.mirror != "right")
				{
					fail(where, "<mirror> is \"" + object.mirror + "\"; expected left or right, the way that shows its pictures flipped");
				}
			}
			if (auto grip = findChild(&node, "grip"))
			{
				object.hasGrip = true;
				object.rawGrip = readValue(*grip, where + " > <grip>");
			}
		}

		std::string readFacing(const XmlNode& node, const std::string& where)
		{
			const std::string facing = readText(node);
			if (facing != "up" && facing != "down" && facing != "left" && facing != "right")
			{
				fail(where, "<facing> is \"" + facing + "\"; expected up, down, left or right");
			}
			return facing;
		}

		// A value and its name, such as <variable name="score">0</variable>.
		void readVariables(const XmlNode* variablesNode, const std::string& where,
			std::vector<std::pair<std::string, RawValue>>& out)
		{
			if (!variablesNode) { return; }

			for (std::unique_ptr<XmlNode> variable = variablesNode->getFirstChild(); variable != nullptr; variable = variable->getNextSibling())
			{
				const std::string name = requireAttribute(*variable, "name", where);
				RawValue value = readValue(*variable, where + " > <variable name=\"" + name + "\">");

				// Declaring a name twice keeps the later value, in the earlier place.
				const auto existing = std::find_if(out.begin(), out.end(), [&](const auto& entry) { return entry.first == name; });
				if (existing != out.end()) { existing->second = std::move(value); }
				else { out.emplace_back(name, std::move(value)); }
			}
		}

		// What a <condition>, an <all> or an <any> tests (below, with <condition>s).
		std::unique_ptr<XmlNode> readCheck(const XmlNode& node, const std::string& here, RawCondition& raw);
		void readJoin(const XmlNode& join, const std::string& here, RawCondition& raw);

		// <collisions>: whether they are on, whether the objects move in lockstep
		// with the others of their group, then the rules.
		// A <collisions>, added to what `base` already has: a group inside
		// another keeps the outer group's rules and adds its own after them.
		RawCollisionData readCollisions(const XmlNode& collisions, const std::string& where, RawCollisionData base = {})
		{
			const std::string collisionsHere = where + " > <collisions>";
			RawCollisionData collisionData = std::move(base);

			// A <collisions> is on unless it says <enabled>false</enabled>.
			const auto enabled = findChild(&collisions, "enabled");
			collisionData.enabled = enabled ? readBool(*enabled, collisionsHere) : true;
			if (auto lockstep = findChild(&collisions, "lockstep"))
			{
				collisionData.lockstep = readBool(*lockstep, collisionsHere);
			}
			if (auto type = findChild(&collisions, "type"))
			{
				const std::string text = readText(*type);
				if (text == "box") { collisionData.type = CollisionType::Box; }
				else if (text == "pixel") { collisionData.type = CollisionType::Pixel; }
				else { fail(collisionsHere, "<type> is \"" + text + "\"; expected box or pixel"); }
			}

			// The characters of its bitmap that touch, each <solid char>; an
			// inner group's replace the outer's.
			std::string solid;
			for (std::unique_ptr<XmlNode> child = collisions.getFirstChild(); child != nullptr; child = child->getNextSibling())
			{
				if (child->getName() != "solid") { continue; }
				const std::string character = requireAttribute(*child, "char", collisionsHere + " > <solid>");
				if (character.size() != 1 || character == ".")
				{
					fail(collisionsHere + " > <solid>", "char=\"" + character + "\"; expected one character of the bitmap, '*' or one of its <color char>s");
				}
				solid += character;
			}
			if (!solid.empty()) { collisionData.solid = solid; }

			// The shape it touches with, if not its picture.
			if (auto hitbox = findChild(&collisions, "hitbox"))
			{
				const std::string here = collisionsHere + " > <hitbox>";
				std::unique_ptr<XmlNode> shape = hitbox->getFirstChild();
				if (!shape || (shape->getName() != "rectangle" && shape->getName() != "circle"))
				{
					fail(here, "expected a <rectangle> or a <circle>, then a <position> if wanted");
				}
				collisionData.hitboxKind = shape->getName();
				const std::string shapeHere = here + " > <" + shape->getName() + ">";
				for (std::unique_ptr<XmlNode> part = shape->getFirstChild(); part != nullptr; part = part->getNextSibling())
				{
					const std::string tag = part->getName();
					if (shape->getName() == "rectangle" ? (tag != "width" && tag != "height") : tag != "radius")
					{
						fail(shapeHere, "unknown <" + tag + ">; a hitbox's " + (shape->getName() == "rectangle" ? "rectangle has a <width> and a <height>" : "circle has a <radius>"));
					}
				}
				if (shape->getName() == "rectangle")
				{
					collisionData.hitboxWidth = readValueOf(*shape, "width", shapeHere);
					collisionData.hitboxHeight = readValueOf(*shape, "height", shapeHere);
				}
				else
				{
					collisionData.hitboxRadius = readValueOf(*shape, "radius", shapeHere);
				}
				collisionData.hitboxX = {};
				collisionData.hitboxY = {};
				for (std::unique_ptr<XmlNode> next = shape->getNextSibling(); next != nullptr; next = next->getNextSibling())
				{
					if (next->getName() != "position") { fail(here, "unknown <" + next->getName() + ">; after its shape a hitbox has a <position>"); }
					const PartialVector2 at = readPartialVector2(*next, here);
					if (at.x) { collisionData.hitboxX = *at.x; }
					if (at.y) { collisionData.hitboxY = *at.y; }
				}
			}

			for (std::unique_ptr<XmlNode> collision = findChild(&collisions, "collision"); collision != nullptr; collision = collision->getNextSibling())
			{
				if (collision->getName() != "collision") { continue; }

				const std::string ruleHere = collisionsHere + " > <collision>";
				const std::string edge = collision->getAttribute("edge");

				// A rule about another object may start with a speed filter,
				// <slower> and/or <faster>; the commands follow.
				std::optional<RawValue> slower;
				std::optional<RawValue> faster;
				std::unique_ptr<XmlNode> firstCommand = collision->getFirstChild();
				while (firstCommand && (firstCommand->getName() == "slower" || firstCommand->getName() == "faster"))
				{
					(firstCommand->getName() == "slower" ? slower : faster) = readValue(*firstCommand, ruleHere);
					firstCommand = firstCommand->getNextSibling();
				}
				if ((slower || faster) && !edge.empty())
				{
					fail(ruleHere, "<slower> and <faster> are about speed against another object; a rule about a screen edge has none");
				}

				// Then the checks that must all hold for it to run.
				RawCondition check;
				while (firstCommand && (firstCommand->getName() == "condition" || firstCommand->getName() == "all" || firstCommand->getName() == "any"))
				{
					const std::string name = firstCommand->getName();
					RawCondition part;
					if (name == "condition")
					{
						if (readCheck(*firstCommand, ruleHere + " > <condition>", part)) { fail(ruleHere + " > <condition>", "has its one test and nothing after it; the commands go after the checks"); }
					}
					else
					{
						readJoin(*firstCommand, ruleHere + " > <" + name + ">", part);
					}
					check.join = RawCondition::Join::All;
					check.parts.push_back(std::move(part));
					firstCommand = firstCommand->getNextSibling();
				}
				if (check.join != RawCondition::Join::None && !edge.empty())
				{
					fail(ruleHere, "a <condition> on a rule about a screen edge is not built yet; one in the state's <conditions> can test the same variable");
				}
				const std::string side = collision->getAttribute("side");
				if (!side.empty())
				{
					if (!edge.empty()) { fail(ruleHere, "side= is the side of the object another object touches; a rule about a screen edge says which with edge="); }
					if (side != "top" && side != "bottom" && side != "left" && side != "right" && side != "horizontal" && side != "vertical")
					{
						fail(ruleHere, "side=\"" + side + "\"; expected top, bottom, left, right, horizontal or vertical");
					}
				}

				std::vector<RawCommand> commands = readCommands(std::move(firstCommand), ruleHere);

				// With an edge the rule is about the screen edge. Without one it
				// is about another object: class and/or object narrow which, and
				// neither means anything at all.
				if (!edge.empty())
				{

					// Appends rather than overwrites, so more than one <collision
					// edge="..."> touching the same edge (e.g. an "all" rule plus a
					// specific "left" rule) both run instead of the later one
					// silently winning.
					// class= and object= pick the other object of a touch; an
					// edge is no object, so there is nothing for them to pick
					if (!collision->getAttribute("class").empty() || !collision->getAttribute("object").empty() || !collision->getAttribute("through").empty())
					{
						fail(ruleHere, "edge= with class=, object= or through=; a rule about a screen edge is about no other object"
							" (unless=\"class\" passes it over while touching that class)");
					}

					const std::string sprite = collision->getAttribute("sprite");
					const std::string unlessClass = collision->getAttribute("unless");
					if (!addEdgeRule(collisionData, edge, commands, sprite, unlessClass))
					{
						fail(ruleHere, "edge=\"" + edge + "\"; expected left, right, top, bottom, horizontal, vertical or all");
					}
				}
				else
				{
					RawCollisionRule rule;
					rule.filterClass = collision->getAttribute("class");
					rule.filterObject = collision->getAttribute("object");
					rule.unlessClass = collision->getAttribute("unless");
					rule.throughClass = collision->getAttribute("through");
					rule.whileSprite = collision->getAttribute("sprite");
					rule.slower = std::move(slower);
					rule.faster = std::move(faster);
					rule.side = side;
					rule.check = std::move(check);
					rule.commands = std::move(commands);
					collisionData.others.push_back(std::move(rule));
				}
			}

			return collisionData;
		}

		// <actions>: named things the object can do, run by a <trigger>. Nothing
		// is added if `object` has none.
		void readActions(const XmlNode& object, const std::string& where,
			std::map<std::string, std::vector<RawCommand>>& out)
		{
			if (std::unique_ptr<XmlNode> actions = findChild(&object, "actions"))
			{
				for (std::unique_ptr<XmlNode> action = actions->getFirstChild(); action != nullptr; action = action->getNextSibling())
				{
					const std::string actionName = requireAttribute(*action, "name", where + " > <actions>");
					out[actionName] = readCommands(action->getFirstChild(), where + " > <action name=\"" + actionName + "\">");
				}
			}
		}

		// <variables>: numbers the object owns, read elsewhere as name.variable.
		void readObjectVariables(const XmlNode& object, const std::string& where, std::map<std::string, RawValue>& out)
		{
			std::unique_ptr<XmlNode> variablesNode = findChild(&object, "variables");
			std::vector<std::pair<std::string, RawValue>> variables;
			readVariables(variablesNode.get(), where + " > <variables>", variables);
			for (auto& [name, value] : variables) { out[name] = std::move(value); }
		}

		// The value of what a game leaves out: 0.
		RawValue zeroValue()
		{
			RawValue value;
			value.text = "0";
			return value;
		}

		// <maze map="maze">, its <wall class="..." sprite="..." />s and its
		// <slow class="...">share</slow>s.
		RawMaze readMaze(const XmlNode& node, const std::string& where)
		{
			const std::string here = where + " > <maze>";
			RawMaze maze;
			maze.map = requireAttribute(node, "map", here);
			for (std::unique_ptr<XmlNode> child = node.getFirstChild(); child != nullptr; child = child->getNextSibling())
			{
				if (child->getName() == "slow")
				{
					const std::string there = here + " > <slow>";
					MazeSlow slow;
					slow.objClass = requireAttribute(*child, "class", there);
					const std::string text = readText(*child);
					if (!isNumber(text)) { fail(there, "is \"" + text + "\"; expected a share of the speed, a number above 0"); }
					slow.share = std::stof(text);
					if (!(slow.share > 0.0f)) { fail(there, "is " + text + "; a walker slowed to nothing would stop for good, so give a share above 0"); }
					maze.slows.push_back(std::move(slow));
					continue;
				}
				if (child->getName() != "wall") { fail(here, "unknown <" + child->getName() + ">; expected <wall>s and <slow>s"); }
				MazeWall wall;
				wall.objClass = requireAttribute(*child, "class", here + " > <wall>");
				wall.sprite = child->getAttribute("sprite");
				maze.walls.push_back(std::move(wall));
			}
			return maze;
		}

		// A <map> as what is on it is read (readMap): the map, and which
		// character each of its groups and objects picks, so that no two pick
		// the same one.
		struct MapLayout : RawMap
		{
			std::map<char, std::string>* picked = nullptr;
		};

		// The one character a group or object on a map picks (char="."),
		// noted as picked by `who`.
		char readMapCharacter(const XmlNode& node, const MapLayout& map, const std::string& who, const std::string& where)
		{
			const std::string character = requireAttribute(node, "char", where);
			if (character.size() != 1) { fail(where, "char=\"" + character + "\": a map's tiles are one character each"); }
			const auto [earlier, isNew] = map.picked->emplace(character[0], who);
			if (!isNew) { fail(where, "char=\"" + character + "\" is picked by " + earlier->second + " already"); }
			return character[0];
		}

		// An object on a map starts on the one tile of the character it picks.
		void placeObjectOnMap(const XmlNode& node, const MapLayout& map, RawObject& object, const std::string& where)
		{
			const char character = readMapCharacter(node, map, where, where);
			const auto tiles = tilesOf(map, character);
			const std::size_t found = tiles.size();
			if (found == 1) { placeOnMap(object, map, tiles.front().first, tiles.front().second); }
			if (found != 1)
			{
				fail(where, "picks '" + std::string(1, character) + "', which is on " + std::to_string(found) + " tiles of map '" + map.name
					+ "'; an object starts on one, and a group is what has a member on each");
			}
		}

		int readCount(const XmlNode& node, const std::string& where);

		// <exists>false</exists>: whether the object is in the game at the start.
		bool readExists(const XmlNode& object, const std::string& where)
		{
			const auto exists = findChild(&object, "exists");
			return !exists || readBool(*exists, where);
		}

		// An <object>; on a <map> (`map`), one that starts on the one tile of
		// the character it picks, with no <position> of its own.
		RawObject readObject(const XmlNode& object, const MapLayout* map = nullptr)
		{
			RawObject rawObject;
			rawObject.name = requireAttribute(object, "name", "<object>");
			rawObject.objClass = getAttribute(&object, "class");

			const std::string where = "object '" + rawObject.name + "'";

			std::vector<AnimationSpec> animations = readAnimations(object, where);
			std::vector<RawSprite> sprites = readSprites(object, where);

			// No <sprite>: an object that only holds numbers (a score, the
			// lives, the clock), never drawn and, unless it says, at 0, 0.
			const bool holder = sprites.empty() && animations.empty() && !map;
			if (holder)
			{
				rawObject.sprite = nothingSprite();
				rawObject.isDrawn = false;
				if (findChild(&object, "drawn")) { fail(where, "has no <sprite>, so it is never drawn; it has no <drawn>"); }
				if (!findChild(&object, "variables") && !findChild(&object, "timers")) { fail(where, "has no <sprite> and no <variables> or <timers>, so it is nothing"); }
			}
			else
			{
				chooseSprite(std::move(sprites), animations, where, rawObject);
			}

			if (map)
			{
				if (findChild(&object, "position")) { fail(where, "is on a <map>, where the tile of its char= is where it starts, so it has no <position>"); }
				placeObjectOnMap(object, *map, rawObject, where);
			}
			else
			{
				if (!object.getAttribute("char").empty()) { fail(where, "char=\"" + object.getAttribute("char") + "\" picks a character of a <map>'s rows; this object is on no <map>"); }
				if (auto position = findChild(&object, "position")) { rawObject.rawPosition = readVector2(*position, where); }
				else if (holder || !readExists(object, where)) { rawObject.rawPosition = { zeroValue(), zeroValue() }; }
				else { fail(where, "missing <position>"); }
			}
			// What a game leaves out is nothing: no <velocity> (or no half of
			// one) stands still, and no <collisions> touches nothing.
			if (auto velocity = findChild(&object, "velocity"))
			{
				const PartialVector2 given = readPartialVector2(*velocity, where);
				rawObject.rawVelocity.x = given.x.value_or(zeroValue());
				rawObject.rawVelocity.y = given.y.value_or(zeroValue());
			}
			else
			{
				rawObject.rawVelocity = { zeroValue(), zeroValue() };
			}
			if (auto acceleration = findChild(&object, "acceleration"))
			{
				rawObject.hasAcceleration = true;
				rawObject.rawAcceleration = readVector2(*acceleration, where);
			}
			if (auto heading = findChild(&object, "heading"))
			{
				rawObject.hasHeading = true;
				rawObject.rawHeading = readValue(*heading, where);
			}
			if (auto drag = findChild(&object, "drag"))
			{
				rawObject.hasDrag = true;
				rawObject.rawDrag = readValue(*drag, where);
			}
			if (auto mass = findChild(&object, "mass"))
			{
				rawObject.hasMass = true;
				rawObject.rawMass = readValue(*mass, where);
			}
			readPullAndSeek(object, rawObject, where);
			readSight(object, rawObject, where);
			if (auto facing = findChild(&object, "facing")) { rawObject.facing = readFacing(*facing, where); }
			readMoves(object, rawObject, where);
			if (auto maze = findChild(&object, "maze")) { rawObject.maze = readMaze(*maze, where); }
			rawObject.timers = readTimers(findChild(&object, "timers").get(), where);
			if (auto active = findChild(&object, "active"))
			{
				if (!readBool(*active, where)) { rawObject.isVisible = false; }
			}
			if (auto drawn = findChild(&object, "drawn")) { rawObject.isDrawn = readBool(*drawn, where); }
			rawObject.exists = readExists(object, where);
			if (auto most = findChild(&object, "most")) { rawObject.most = readCount(*most, where); }
			if (auto collisions = findChild(&object, "collisions")) { rawObject.rawCollisionData = readCollisions(*collisions, where); }
			readActions(object, where, rawObject.action);
			readObjectVariables(object, where, rawObject.variable);

			return rawObject;
		}

		// What a member says for one thing, else what its group says, else an error.
		RawValue pickValue(const std::optional<RawValue>& own, const std::optional<RawValue>& shared,
			const std::string& where, const std::string& what)
		{
			if (own) { return *own; }
			if (shared) { return *shared; }
			fail(where, "has no " + what + ", and neither does its group");
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

		// Which rows or columns a <row> or <column> picks, counting from 1:
		// one by number="2", several by a <number> each, every odd or even
		// one by <odd /> or <even />.
		std::vector<bool> readPick(const XmlNode& node, int count, const std::string& where)
		{
			std::vector<bool> picked(static_cast<std::size_t>(count) + 1, false);
			const auto pick = [&](const std::string& text)
			{
				if (text.empty() || text.size() > 4 || !std::all_of(text.begin(), text.end(), [](unsigned char c) { return std::isdigit(c) != 0; })
					|| std::stoi(text) < 1 || std::stoi(text) > count)
				{
					fail(where, "\"" + text + "\" is not one of 1 to " + std::to_string(count)
						+ (text.find_first_of(" \t") == std::string::npos ? "" : "; write several as a <number> each"));
				}
				picked[static_cast<std::size_t>(std::stoi(text))] = true;
			};

			const std::string attribute = node.getAttribute("number");
			bool any = !attribute.empty();
			if (any) { pick(attribute); }
			for (std::unique_ptr<XmlNode> child = node.getFirstChild(); child != nullptr; child = child->getNextSibling())
			{
				const std::string tag = child->getName();
				if (tag != "number" && tag != "odd" && tag != "even") { continue; }
				if (!attribute.empty()) { fail(where, "has number=\"" + attribute + "\" and a <" + tag + ">; use one or the other"); }
				any = true;
				if (tag == "number") { pick(readText(*child)); }
				else
				{
					for (int i = tag == "odd" ? 1 : 2; i <= count; i += 2) { picked[static_cast<std::size_t>(i)] = true; }
				}
			}
			if (!any) { fail(where, "picks nothing; give number=\"...\", a <number> for each, <odd /> or <even />"); }

			return picked;
		}

		// What a <member>, <row>, <column> or <cell> of a group says for itself.
		struct GroupPart
		{
			std::string where;
			std::unique_ptr<XmlNode> node;
			std::vector<bool> rows;                 // <row>: the rows it picks
			std::vector<bool> columns;              // <column>: the columns it picks
			int row{ 0 };                           // <cell>
			int column{ 0 };                        // <cell>
			std::set<std::string> sprites;          // the names of the sprites it changes
			std::vector<AnimationSpec> animations;
			PartialVector2 position;
			PartialVector2 velocity;
			PartialVector2 padding;
			std::map<std::string, RawValue> variables;
		};

		// The parts of a member, row, column or cell, as far as they are its
		// own; `allowed` is what it may give, said in the message for anything
		// else. Its sprites are applied to the group's later (changeSprites).
		GroupPart readGroupPart(std::unique_ptr<XmlNode> node, const std::string& where, const std::set<std::string>& allowed, const std::string& expected)
		{
			GroupPart part;
			part.where = where;

			for (std::unique_ptr<XmlNode> child = node->getFirstChild(); child != nullptr; child = child->getNextSibling())
			{
				const std::string tag = child->getName();
				if (!allowed.count(tag)) { fail(where, "unknown <" + tag + ">; " + expected); }

				if (tag == "sprite") { part.sprites.insert(child->getAttribute("name")); }
				else if (tag == "animation") { /* below, all at once */ }
				else if (tag == "position") { part.position = readPartialVector2(*child, where); }
				else if (tag == "velocity") { part.velocity = readPartialVector2(*child, where); }
				else if (tag == "padding") { part.padding = readPartialVector2(*child, where); }
			}
			readObjectVariables(*node, where, part.variables);
			part.animations = readAnimations(*node, where);

			part.node = std::move(node);
			return part;
		}

		// A <row> and a <column> (or two rows, or two columns) that both pick
		// a cell must not both change the same thing in it: which would win is
		// not something to guess. A <cell> may change anything they do.
		void checkNoClash(const GroupPart& one, const GroupPart& other, const std::string& cell)
		{
			const auto clash = [&](const std::string& what)
			{
				fail(cell, "is picked by " + one.where + " and by " + other.where + ", and both change its " + what
					+ "; change it in one of them, or in a <cell>");
			};

			for (const std::string& name : one.sprites)
			{
				if (other.sprites.count(name)) { clash(name.empty() ? std::string("<sprite>") : "<sprite name=\"" + name + "\">"); }
			}
			for (const AnimationSpec& animation : one.animations)
			{
				if (std::any_of(other.animations.begin(), other.animations.end(), [&](const AnimationSpec& each) { return each.name == animation.name; }))
				{
					clash(animation.name.empty() ? std::string("<animation>") : "<animation name=\"" + animation.name + "\">");
				}
			}
			if (one.velocity.x && other.velocity.x) { clash("<velocity><x>"); }
			if (one.velocity.y && other.velocity.y) { clash("<velocity><y>"); }
			if (one.padding.x && other.padding.x) { clash("<padding><x>"); }
			if (one.padding.y && other.padding.y) { clash("<padding><y>"); }
			for (const auto& [name, value] : one.variables)
			{
				if (other.variables.count(name)) { clash("<variable name=\"" + name + "\">"); }
			}
		}

		// What a <group> gives what is inside it, already merged with what the
		// groups around it give: a group inside another starts from all of this.
		struct GroupShared
		{
			std::vector<RawSprite> sprites;
			std::vector<AnimationSpec> animations;
			PartialVector2 position;
			PartialVector2 velocity;
			PartialVector2 padding;
			std::optional<RawValue> slope;
			int columns = 0;
			int rows = 0;
			// Visibility, facing, acceleration, collisions, actions, variables,
			// timers, class and the names of the groups around it.
			RawObject object;
			// The <map> it is on, if it is: its members are the tiles of the
			// character it picks.
			const MapLayout* map = nullptr;
		};

		// A <group> is read as one RawObject per member, which from then on is
		// an ordinary object that remembers its group (see Object::groupName).
		// The group gives what its members share (sprites, position, velocity,
		// collisions, actions, variables, all optional), and each member gives
		// what is its own and takes the rest from the group; after that it must
		// be as complete as an <object>.
		//
		// The members are either listed, each a <member> (logrow3.2: the
		// group's name and its number from 1, or name="..."), with its own
		// <sprite>s, <animation>, <position> and <velocity>; or laid out in
		// <columns> and <rows>, a cell to each place (bricks.5.3: column, then
		// row; or a <cell>'s name="..."), top row first and left to right. A
		// <row number="...">, <column number="..."> and <cell row="..."
		// column="..."> change what the cells they pick have: <sprite>s, an
		// <animation>, a <velocity> and <variables>, and for a row or column
		// the gap before it (<padding>). A sprite that changes one of the
		// group's need only give what changes (changeSprite).
		//
		// Or a group holds <group>s, which take from it the same way a member
		// does from its group: whatever an inner group leaves out it takes from
		// the outer, its sprites change the outer's by name, and its collision
		// rules, actions, variables and timers add to the outer's (an action or
		// variable of the same name replaces it). Its members are named after
		// it, and the outer group's name means all of them.
		void readGroup(const XmlNode& group, std::vector<RawObject>& out, const GroupShared& outer = {})
		{
			const std::string name = requireAttribute(group, "name", "<group>");
			const std::string where = "group '" + name + "'";

			GroupShared own = outer;
			RawObject& shared = own.object;
			bool inner = false;

			if (!group.getAttribute("char").empty() && !own.map)
			{
				fail(where, "char=\"" + group.getAttribute("char") + "\" picks a character of a <map>'s rows; this group is on no <map>");
			}

			for (std::unique_ptr<XmlNode> child = group.getFirstChild(); child != nullptr; child = child->getNextSibling())
			{
				const std::string tag = child->getName();
				if (own.map && (tag == "columns" || tag == "rows" || tag == "padding" || tag == "slope" || tag == "position" || tag == "member" || tag == "row" || tag == "column" || tag == "cell"))
				{
					fail(where, "is on map '" + own.map->name + "', whose rows say where its members are, so it has no <" + tag + ">");
				}

				if (tag == "active") { shared.isVisible = readBool(*child, where); }
				else if (tag == "drawn") { shared.isDrawn = readBool(*child, where); }
				else if (tag == "exists") { shared.exists = readBool(*child, where); }
				else if (tag == "most") { shared.most = readCount(*child, where); }
				else if (tag == "columns") { own.columns = readCount(*child, where); }
				else if (tag == "rows") { own.rows = readCount(*child, where); }
				else if (tag == "padding")
				{
					const PartialVector2 given = readPartialVector2(*child, where);
					if (given.x) { own.padding.x = given.x; }
					if (given.y) { own.padding.y = given.y; }
				}
				else if (tag == "slope") { own.slope = readValue(*child, where); }
				else if (tag == "animation") { /* below, all at once */ }
				else if (tag == "position" || tag == "velocity")
				{
					PartialVector2& into = tag == "position" ? own.position : own.velocity;
					const PartialVector2 given = readPartialVector2(*child, where);
					if (given.x) { into.x = given.x; }
					if (given.y) { into.y = given.y; }
				}
				else if (tag == "acceleration")
				{
					shared.hasAcceleration = true;
					shared.rawAcceleration = readVector2(*child, where);
				}
				else if (tag == "heading")
				{
					shared.hasHeading = true;
					shared.rawHeading = readValue(*child, where);
				}
				else if (tag == "drag")
				{
					shared.hasDrag = true;
					shared.rawDrag = readValue(*child, where);
				}
				else if (tag == "mass")
				{
					shared.hasMass = true;
					shared.rawMass = readValue(*child, where);
				}
				else if (tag == "pull" || tag == "seek" || tag == "light" || tag == "dark" || tag == "altitude" || tag == "shadow") { /* below, all at once */ }
				else if (tag == "collisions") { shared.rawCollisionData = readCollisions(*child, where, shared.rawCollisionData); }
				else if (tag == "facing") { shared.facing = readFacing(*child, where); }
				else if (tag == "poses" || tag == "mirror" || tag == "grip") { /* below, all at once */ }
				else if (tag == "maze") { shared.maze = readMaze(*child, where); }
				else if (tag == "group") { inner = true; }
				else if (tag != "sprite" && tag != "actions" && tag != "variables" && tag != "timers" && tag != "member" && tag != "row" && tag != "column" && tag != "cell")
				{
					fail(where, "unknown <" + tag + ">; expected <columns>, <rows>, <padding>, <slope>, <sprite>, <animation>, <position>, <velocity>, <acceleration>, <heading>, <drag>, <mass>, <pull>, <seek>, <light>, <dark>, <altitude>, <shadow>, <facing>, <poses>, <mirror>, <grip>, <maze>, <active>, <drawn>, <most>, <collisions>, <actions>, <variables>, <timers>, then <member>s, <group>s, or <row>s, <column>s and <cell>s");
				}
			}

			own.sprites = changeSprites(own.sprites, group, where);
			mergeAnimations(own.animations, readAnimations(group, where));
			for (RawTimer& timer : readTimers(findChild(&group, "timers").get(), where)) { shared.timers.push_back(std::move(timer)); }
			readActions(group, where, shared.action);
			readObjectVariables(group, where, shared.variable);
			readPullAndSeek(group, shared, where);
			readSight(group, shared, where);
			readMoves(group, shared, where);
			if (const std::string objClass = getAttribute(&group, "class"); !objClass.empty()) { shared.objClass = objClass; }

			if (inner)
			{
				if (!group.getAttribute("char").empty()) { fail(where, "has groups inside it, so it picks no character itself; give the char= to the groups inside"); }
				own.object.outerGroups.push_back(name);
				for (std::unique_ptr<XmlNode> child = group.getFirstChild(); child != nullptr; child = child->getNextSibling())
				{
					const std::string tag = child->getName();
					if (tag == "member" || tag == "row" || tag == "column" || tag == "cell")
					{
						fail(where, "has <group>s inside it, so it has no <" + tag + ">s of its own; give them to the groups inside");
					}
					if (tag == "group") { readGroup(*child, out, own); }
				}
				return;
			}

			const std::vector<RawSprite>& sprites = own.sprites;
			const std::vector<AnimationSpec>& animations = own.animations;
			const PartialVector2& position = own.position;
			const PartialVector2& velocity = own.velocity;
			const PartialVector2& padding = own.padding;
			const int columns = own.columns;
			const int rows = own.rows;

			if ((columns > 0) != (rows > 0)) { fail(where, "has <columns> or <rows> and not the other; a group laid out in cells needs both"); }

			shared.groupName = name;
			if (own.map)
			{
				own.position.x = own.map->position.x;
				own.position.y = own.map->position.y;
			}

			// One member or cell from the group and its own parts, the most
			// particular last.
			const auto make = [&](const std::string& memberName, const std::string& here, const std::vector<const GroupPart*>& parts) -> RawObject
			{
				RawObject rawObject = shared;
				rawObject.name = memberName;

				std::vector<RawSprite> used = sprites;
				std::vector<AnimationSpec> usedAnimations = animations;
				std::optional<RawValue> positionX = position.x;
				std::optional<RawValue> positionY = position.y;
				std::optional<RawValue> velocityX = velocity.x;
				std::optional<RawValue> velocityY = velocity.y;
				for (const GroupPart* part : parts)
				{
					used = changeSprites(used, *part->node, part->where);
					mergeAnimations(usedAnimations, part->animations);
					if (part->position.x) { positionX = part->position.x; }
					if (part->position.y) { positionY = part->position.y; }
					if (part->velocity.x) { velocityX = part->velocity.x; }
					if (part->velocity.y) { velocityY = part->velocity.y; }
					for (const auto& [variable, value] : part->variables) { rawObject.variable[variable] = value; }
				}

				if (used.empty()) { fail(here, "has no <sprite>, and neither does its group"); }
				chooseSprite(used, usedAnimations, here, rawObject);

				// One that does not exist at the start is placed by the <spawn>
				// that makes it, so its <position> may be left out (0, 0).
				if (!rawObject.exists)
				{
					if (!positionX) { positionX = zeroValue(); }
					if (!positionY) { positionY = zeroValue(); }
				}
				rawObject.rawPosition.x = pickValue(positionX, std::nullopt, here, "<position><x>");
				rawObject.rawPosition.y = pickValue(positionY, std::nullopt, here, "<position><y>");
				rawObject.rawVelocity.x = velocityX.value_or(zeroValue());
				rawObject.rawVelocity.y = velocityY.value_or(zeroValue());
				return rawObject;
			};

			// On a map: a member for each tile of its character, named under
			// the map by its number in the order the rows are read
			// (maze.dots.17), so two maps in one list each have their own.
			if (own.map)
			{
				const MapLayout& map = *own.map;
				const char character = readMapCharacter(group, map, where, where);
				int count = 0;
				for (const auto& [column, row] : tilesOf(map, character))
				{
					++count;
					const std::string memberName = map.name + "." + name + "." + std::to_string(count);
					const std::string here = "object '" + memberName + "' (a member of " + where + ")";
					RawObject member = make(memberName, here, {});
					placeOnMap(member, map, column, row);
					out.push_back(std::move(member));
				}
				if (count == 0) { fail(where, "picks '" + std::string(1, character) + "', which is on no tile of map '" + map.name + "'"); }
				return;
			}

			std::vector<std::unique_ptr<XmlNode>> children;
			for (std::unique_ptr<XmlNode> child = group.getFirstChild(); child != nullptr;)
			{
				std::unique_ptr<XmlNode> next = child->getNextSibling();
				children.push_back(std::move(child));
				child = std::move(next);
			}

			if (columns == 0)
			{
				int count = 0;
				for (std::unique_ptr<XmlNode>& child : children)
				{
					const std::string tag = child->getName();
					if (tag == "row" || tag == "column" || tag == "cell")
					{
						fail(where, "has a <" + tag + ">, but no <columns> and <rows> for it to pick from");
					}
					if (tag != "member") { continue; }

					++count;
					const std::string ownName = child->getAttribute("name");
					const std::string memberName = ownName.empty() ? name + "." + std::to_string(count) : ownName;
					const std::string here = "object '" + memberName + "' (a member of " + where + ")";

					const GroupPart member = readGroupPart(std::move(child), here, { "sprite", "animation", "position", "velocity" },
						"a member can give <sprite>s, an <animation>, a <position> or a <velocity>");
					out.push_back(make(memberName, here, { &member }));
				}

				if (count == 0) { fail(where, "has no <member>s, and no <columns> and <rows> to lay out cells in"); }
				return;
			}

			std::vector<GroupPart> lines;   // the <row>s and <column>s, in the order written
			std::vector<GroupPart> cells;
			for (std::unique_ptr<XmlNode>& child : children)
			{
				const std::string tag = child->getName();
				if (tag == "member") { fail(where, "is laid out in <columns> and <rows>, so it has no <member>s; a <cell> changes one place"); }

				if (tag == "row" || tag == "column")
				{
					const std::string number = child->getAttribute("number");
					const std::string here = where + " > <" + tag + (number.empty() ? std::string() : " number=\"" + number + "\"") + ">";
					std::vector<bool> picked = readPick(*child, tag == "row" ? rows : columns, here);
					GroupPart line = readGroupPart(std::move(child), here, { "number", "odd", "even", "sprite", "animation", "velocity", "padding", "variables" },
						"a <" + tag + "> can give <sprite>s, an <animation>, a <velocity>, a <padding> and <variables>");
					if (tag == "row") { line.rows = std::move(picked); }
					else
					{
						if (line.padding.y) { fail(here, "<padding> of a column gives only an <x>, the gap before it"); }
						line.columns = std::move(picked);
					}
					lines.push_back(std::move(line));
				}
				else if (tag == "cell")
				{
					const std::string rowText = requireAttribute(*child, "row", where + " > <cell>");
					const std::string columnText = requireAttribute(*child, "column", where + " > <cell>");
					const std::string here = where + " > <cell row=\"" + rowText + "\" column=\"" + columnText + "\">";

					const auto place = [&](const std::string& text, int count) -> int
					{
						if (text.empty() || text.size() > 4 || !std::all_of(text.begin(), text.end(), [](unsigned char c) { return std::isdigit(c) != 0; })
							|| std::stoi(text) < 1 || std::stoi(text) > count)
						{
							fail(here, "is not a place in the group; its rows and columns count from 1 to " + std::to_string(rows) + " and " + std::to_string(columns));
						}
						return std::stoi(text);
					};
					const int row = place(rowText, rows);
					const int column = place(columnText, columns);

					for (const GroupPart& other : cells)
					{
						if (other.row == row && other.column == column) { fail(here, "is the same place as an earlier <cell>; say all of it in one"); }
					}

					GroupPart cell = readGroupPart(std::move(child), here, { "sprite", "animation", "velocity", "variables" },
						"a <cell> can give <sprite>s, an <animation>, a <velocity> and <variables>");
					cell.row = row;
					cell.column = column;
					cells.push_back(std::move(cell));
				}
			}

			const auto picks = [](const GroupPart& line, int row, int column)
			{
				return line.rows.empty() ? line.columns[static_cast<std::size_t>(column)] : line.rows[static_cast<std::size_t>(row)];
			};

			for (int row = 1; row <= rows; ++row)
			{
				for (int column = 1; column <= columns; ++column)
				{
					const GroupPart* cell = nullptr;
					for (const GroupPart& candidate : cells)
					{
						if (candidate.row == row && candidate.column == column) { cell = &candidate; }
					}

					const std::string ownName = cell ? cell->node->getAttribute("name") : std::string{};
					const std::string cellName = ownName.empty() ? name + "." + std::to_string(column) + "." + std::to_string(row) : ownName;
					const std::string here = "object '" + cellName + "' (column " + std::to_string(column) + ", row " + std::to_string(row) + " of " + where + ")";

					// The rows and columns that pick it, then its own <cell>; and
					// the rows alone, which give its row's sprite and so its slot.
					std::vector<const GroupPart*> parts;
					std::vector<const GroupPart*> rowParts;
					const GroupPart* gapBefore = nullptr;
					const GroupPart* gapAbove = nullptr;
					for (const GroupPart& line : lines)
					{
						if (!picks(line, row, column)) { continue; }
						for (const GroupPart* earlier : parts) { checkNoClash(*earlier, line, here); }
						parts.push_back(&line);
						if (!line.rows.empty()) { rowParts.push_back(&line); }
						if (line.padding.x) { gapBefore = &line; }
						if (line.padding.y) { gapAbove = &line; }
					}
					if (cell) { parts.push_back(cell); }

					RawObject rawObject = make(cellName, here, parts);

					rawObject.cell.slot = make(cellName, here, rowParts).sprite;
					rawObject.cell.column = column;
					rawObject.cell.row = row;
					rawObject.cell.gapBefore = column == 1 ? RawValue::expression("0")
						: gapBefore ? *gapBefore->padding.x : padding.x ? *padding.x : RawValue::expression("0");
					rawObject.cell.gapAbove = row == 1 ? RawValue::expression("0")
						: gapAbove ? *gapAbove->padding.y : padding.y ? *padding.y : RawValue::expression("0");
					if (own.slope) { rawObject.cell.drop = *own.slope; }

					out.push_back(std::move(rawObject));
				}
			}
		}

		// A <map name="maze">: the size of a <tile>, where its top left is (its
		// <position>), the <row>s of characters, one to a tile, and then the
		// groups and objects on it. A group on it (or inside a group on it)
		// picks a character with char="." and has a member on each tile of it,
		// in the middle of the tile; an object picks one used on one tile only,
		// where it starts. A character nothing picks is an empty tile. The
		// map's name means everything on it. Rows are trimmed like any text, so
		// an empty tile at either end of a row needs a character, not a space.
		void readMap(const XmlNode& node, std::vector<RawObject>& out)
		{
			MapLayout map;
			map.name = requireAttribute(node, "name", "<map>");
			const std::string where = "map '" + map.name + "'";

			const std::unique_ptr<XmlNode> tile = requireChild(node, "tile", where);
			map.tileWidth = readValueOf(*tile, "width", where + " > <tile>");
			map.tileHeight = readValueOf(*tile, "height", where + " > <tile>");
			map.position = readVector2(*requireChild(node, "position", where), where);

			for (std::unique_ptr<XmlNode> child = node.getFirstChild(); child != nullptr; child = child->getNextSibling())
			{
				if (child->getName() != "row") { continue; }
				const std::string row = readText(*child);
				const std::string here = where + " > <row> " + std::to_string(map.rows.size() + 1);
				if (row.empty()) { fail(here, "is empty"); }
				if (!map.rows.empty() && row.size() != map.rows.front().size())
				{
					fail(here, "is " + std::to_string(row.size()) + " tiles wide, but row 1 is " + std::to_string(map.rows.front().size()) + "; every row is as wide as the map");
				}
				map.rows.push_back(row);
			}
			if (map.rows.empty()) { fail(where, "has no <row>s"); }

			std::map<char, std::string> picked;
			map.picked = &picked;

			GroupShared outer;
			outer.map = &map;
			bool any = false;
			for (std::unique_ptr<XmlNode> child = node.getFirstChild(); child != nullptr; child = child->getNextSibling())
			{
				const std::string tag = child->getName();
				if (tag == "group")
				{
					readGroup(*child, out, outer);
					any = true;
				}
				else if (tag == "object")
				{
					out.push_back(readObject(*child, &map));
					any = true;
				}
				else if (tag != "tile" && tag != "position" && tag != "row")
				{
					fail(where, "unknown <" + tag + ">; expected a <tile>, a <position>, <row>s, then <group>s and <object>s");
				}
			}
			if (!any) { fail(where, "has nothing on it; give it <group>s and <object>s that pick its characters"); }
		}

		// A <note> or <rest> of a sound, in the part given; a note with no
		// wave of its own takes the part's (`wave`, "" for the sound's).
		RawNote readNote(const XmlNode& node, const std::string& where, unsigned part, const std::string& wave)
		{
			RawNote note;
			note.part = part;
			if (node.getName() == "rest")
			{
				note.rest = true;
			}
			else
			{
				note.wave = node.getAttribute("wave");
				if (note.wave.empty()) { note.wave = wave; }
				note.pitch = requireAttribute(node, "pitch", where);
				note.to = node.getAttribute("to");
			}
			note.length = readValue(node, where);
			return note;
		}

		// <sound name="..." wave="square">: an optional <loop />, an optional
		// <volume>, then <note>s and <rest>s in the order they play, or the
		// <part>s that play together, each its own <note>s and <rest>s. The
		// words (wave, pitch) are checked by load/expr, which also works out
		// the lengths.
		RawSound readSound(const XmlNode& sound)
		{
			RawSound raw;
			raw.name = requireAttribute(sound, "name", "<sounds>");
			raw.wave = sound.getAttribute("wave");

			const std::string where = "sound '" + raw.name + "'";

			unsigned parts = 0;
			bool loose = false; // a <note> or <rest> not in a part
			for (std::unique_ptr<XmlNode> child = sound.getFirstChild(); child != nullptr; child = child->getNextSibling())
			{
				const std::string tag = child->getName();

				if (tag == "loop")
				{
					if (!raw.notes.empty() || parts > 0 || raw.volume.kind != RawValue::Kind::Expression || !raw.volume.text.empty()) { fail(where, "<loop /> has to come first"); }
					raw.loop = true;
				}
				else if (tag == "volume")
				{
					if (!raw.notes.empty() || parts > 0) { fail(where, "<volume> has to come before the <note>s and <rest>s"); }
					raw.volume = readValue(*child, where);
				}
				else if (tag == "note" || tag == "rest")
				{
					if (parts > 0) { fail(where, "has <part>s, so its <" + tag + ">s go inside them"); }
					loose = true;
					raw.notes.push_back(readNote(*child, where, 0, ""));
				}
				else if (tag == "part")
				{
					if (loose) { fail(where, "has <note>s and <rest>s of its own, so it cannot have <part>s as well"); }
					const std::string partWhere = where + " > <part> " + std::to_string(parts + 1);
					const std::string wave = child->getAttribute("wave");
					bool any = false;
					for (std::unique_ptr<XmlNode> step = child->getFirstChild(); step != nullptr; step = step->getNextSibling())
					{
						const std::string stepTag = step->getName();
						if (stepTag != "note" && stepTag != "rest") { fail(partWhere, "unknown <" + stepTag + ">; expected <note>s and <rest>s"); }
						raw.notes.push_back(readNote(*step, partWhere, parts, wave));
						any = true;
					}
					if (!any) { fail(partWhere, "has no <note>"); }
					++parts;
				}
				else
				{
					fail(where, "unknown <" + tag + ">; expected a <loop />, a <volume>, then <note>s and <rest>s, or <part>s");
				}
			}

			if (raw.notes.empty()) { fail(where, "has no <note>"); }

			return raw;
		}

		struct KeyBindings
		{
			std::map<std::string, std::vector<RawCommand>> buttons;
			std::map<std::string, std::vector<RawCommand>> axes;
		};

		// The names a node gives one way or the other: one by an attribute
		// (button="space"), several by an element each, written first
		// (<button>space</button><button>p</button>). `rest` is left at the
		// first child that is not one of them.
		std::vector<std::string> namesGiven(const XmlNode& node, const std::string& word, const std::string& where, std::unique_ptr<XmlNode>* rest = nullptr)
		{
			std::vector<std::string> names;
			const std::string attribute = node.getAttribute(word);
			if (!attribute.empty())
			{
				if (attribute.find_first_of(" \t\n") != std::string::npos)
				{
					fail(where, word + "=\"" + attribute + "\" names several; write each as a <" + word + "> of its own");
				}
				names.push_back(attribute);
			}

			std::unique_ptr<XmlNode> child = node.getFirstChild();
			for (; child != nullptr && child->getName() == word; child = child->getNextSibling())
			{
				if (!attribute.empty()) { fail(where, "has " + word + "=\"" + attribute + "\" and a <" + word + ">; use one or the other"); }
				const std::string name = readText(*child);
				if (name.empty()) { fail(where, "has an empty <" + word + ">"); }
				names.push_back(name);
			}
			if (rest) { *rest = std::move(child); }
			return names;
		}

		// The <input>s under a node (a state's <inputs>, or a <keys> set): a key
		// (button="a"), or several, a <button> each, or a control that points
		// (axis="mouse.x"), and the commands each one runs. One given twice
		// keeps the later binding.
		void readInputs(const XmlNode& node, const std::string& where, KeyBindings& out)
		{
			for (std::unique_ptr<XmlNode> input = node.getFirstChild(); input != nullptr; input = input->getNextSibling())
			{
				if (input->getName() == "keys") { continue; } // a state's <inputs> names its sets first
				if (input->getName() != "input") { fail(where, "has <" + input->getName() + ">, which is not an <input>"); }
				std::unique_ptr<XmlNode> commandsStart;
				const std::vector<std::string> buttons = namesGiven(*input, "button", where + " > <input>", &commandsStart);
				const std::string axis = input->getAttribute("axis");
				if (buttons.empty() == axis.empty())
				{
					fail(where, "an <input> names a button (button=\"...\", or a <button> each) or an axis=\"...\", one of the two");
				}
				if (!axis.empty())
				{
					out.axes[axis] = readCommands(input->getFirstChild(), where + " > <input axis=\"" + axis + "\">");
					continue;
				}

				std::string written;
				for (const std::string& button : buttons) { written += (written.empty() ? "" : " ") + button; }
				const std::vector<RawCommand> commands = readCommands(std::move(commandsStart), where + " > <input " + written + ">");
				for (const std::string& button : buttons) { out.buttons[button] = commands; }
			}
		}

		using ShowSets = std::map<std::string, std::vector<std::string>>;

		// The <show>s under a node (a state's <shows>, or a <shows> set), in
		// order: <show object="..."> names one object (or group), <show
		// set="..."> takes a named <shows> set's objects in its place, and, in
		// a state's own (`states` not null), <show state="..."> a state drawn
		// still underneath.
		void readShows(const XmlNode& node, const std::string& where, const ShowSets& showSets, std::vector<std::string>& out,
			std::vector<std::string>* states = nullptr)
		{
			for (std::unique_ptr<XmlNode> show = node.getFirstChild(); show != nullptr; show = show->getNextSibling())
			{
				const std::string object = show->getAttribute("object");
				const std::string setName = show->getAttribute("set");
				const std::string stateName = show->getAttribute("state");
				if (!stateName.empty())
				{
					if (!states) { fail(where, "<show state=\"" + stateName + "\" /> goes in a state's own <shows>, not in a set"); }
					if (!object.empty() || !setName.empty()) { fail(where, "a <show> names an object=\"...\", a set=\"...\" or a state=\"...\", one of them"); }
					states->push_back(stateName);
					continue;
				}
				if (object.empty() == setName.empty())
				{
					fail(where, "a <show> names an object=\"...\", a set=\"...\" or a state=\"...\", one of them");
				}
				if (!object.empty())
				{
					out.push_back(object);
					continue;
				}
				const auto set = showSets.find(setName);
				if (set == showSets.end()) { fail(where, "set=\"" + setName + "\" names no <shows> set before it"); }
				out.insert(out.end(), set->second.begin(), set->second.end());
			}
		}

		using ConditionSets = std::map<std::string, std::vector<RawCondition>>;

		// An <all> or an <any>, into `raw`: which, and its <condition>s, <all>s
		// and <any>s in order.
		void readJoin(const XmlNode& join, const std::string& here, RawCondition& raw)
		{
			raw.join = join.getName() == "all" ? RawCondition::Join::All : RawCondition::Join::Any;
			for (std::unique_ptr<XmlNode> part = join.getFirstChild(); part != nullptr; part = part->getNextSibling())
			{
				const std::string partName = part->getName();
				RawCondition check;
				if (partName == "condition")
				{
					const std::string checkHere = here + " > <condition>";
					if (readCheck(*part, checkHere, check)) { fail(checkHere, "has its one test and nothing after it; the commands go at the end of the <condition>"); }
				}
				else if (partName == "all" || partName == "any")
				{
					readJoin(*part, here + " > <" + partName + ">", check);
				}
				else
				{
					fail(here, "unknown <" + partName + ">; expected <condition>s, <all>s and <any>s");
				}
				raw.parts.push_back(std::move(check));
			}
			if (raw.parts.empty()) { fail(here, "has no <condition>"); }
		}

		// What a <condition> tests, into `raw`: its filters and
		// what it measures (attributes), then its test tag, or an <all> or
		// <any> of <condition>s. Gives back what follows (a condition's commands).
		std::unique_ptr<XmlNode> readCheck(const XmlNode& node, const std::string& here, RawCondition& raw)
		{
			raw.filterClass = node.getAttribute("class");
			raw.filterObject = node.getAttribute("object");
			raw.variableName = node.getAttribute("variable");
			raw.total = node.getAttribute("total");
			raw.distance = node.getAttribute("distance");

			std::unique_ptr<XmlNode> test = node.getFirstChild();
			if (!test) { fail(here, "needs an <atleast>, <atmost>, <above>, <below>, <equal> or <remaining>, or an <all> or <any>"); }

			const std::string testName = test->getName();

			// <all> / <any>: several checks, each a test of its own.
			if (testName == "all" || testName == "any")
			{
				if (!raw.filterClass.empty() || !raw.filterObject.empty() || !raw.variableName.empty() || !raw.total.empty() || !raw.distance.empty())
				{
					fail(here, "<" + testName + "> says what is tested in each of its <condition>s, so the condition has no class=, object=, variable=, total= or distance=");
				}
				readJoin(*test, here + " > <" + testName + ">", raw);
				return test->getNextSibling();
			}

			if (testName == "atleast") { raw.test = RawCondition::Test::AtLeast; }
			else if (testName == "atmost") { raw.test = RawCondition::Test::AtMost; }
			else if (testName == "above") { raw.test = RawCondition::Test::Above; }
			else if (testName == "below") { raw.test = RawCondition::Test::Below; }
			else if (testName == "equal") { raw.test = RawCondition::Test::Equal; }
			else if (testName == "remaining") { raw.test = RawCondition::Test::Remaining; }
			else { fail(here, "starts with <" + testName + ">; expected <atleast>, <atmost>, <above>, <below>, <equal> or <remaining>, or an <all> or <any>"); }

			const bool remaining = raw.test == RawCondition::Test::Remaining;
			if (!raw.total.empty() && raw.total != "count" && raw.total != "sum" && raw.total != "highest" && raw.total != "lowest")
			{
				fail(here, "total=\"" + raw.total + "\" is not a total; expected count, sum, highest or lowest");
			}
			if (raw.total == "count" && !raw.distance.empty()) { fail(here, "total=\"count\" counts the objects in play, so it measures no distance="); }
			if (remaining && (!raw.total.empty() || !raw.distance.empty())) { fail(here, "<remaining> counts what is in play, so it takes no total= or distance="); }
			if (!raw.distance.empty() && !raw.variableName.empty()) { fail(here, "distance=\"" + raw.distance + "\" measures how far, so it reads no variable="); }
			if (raw.total == "count" && !raw.variableName.empty()) { fail(here, "total=\"count\" counts the objects in play, so it reads no variable="); }
			if (!remaining && raw.variableName.empty() && raw.total != "count" && raw.distance.empty())
			{
				fail(here, "<" + testName + "> reads a variable, so it needs variable=\"...\"");
			}

			// The number compared with, which may name another variable
			// (<above>gun2.score</above>), read as the game runs.
			if (!test->getAttribute("object").empty() || !test->getAttribute("variable").empty())
			{
				fail(here, "<" + testName + "> takes its number as content, which may name a variable: <" + testName + ">"
					+ (test->getAttribute("object").empty() ? "self" : test->getAttribute("object")) + "." + test->getAttribute("variable") + "</" + testName + ">");
			}
			raw.threshold = readValue(*test, here);
			return test->getNextSibling();
		}

		// The <condition>s under a node (a state's <conditions>, or a
		// <conditions> set), in order: a test, then the commands to run when it
		// holds; or <condition set="..."> for a named set's, in its place.
		void readConditions(const XmlNode& node, const std::string& where, const ConditionSets& conditionSets, std::vector<RawCondition>& out)
		{
			for (std::unique_ptr<XmlNode> condition = node.getFirstChild(); condition != nullptr; condition = condition->getNextSibling())
			{
				const std::string conditionHere = where + " > <condition>";

				// <condition set="..."> takes a named <conditions> set's in its place
				if (!condition->getAttribute("set").empty())
				{
					const std::string setName = condition->getAttribute("set");
					if (condition->getFirstChild() || !condition->getAttribute("class").empty() || !condition->getAttribute("object").empty() || !condition->getAttribute("variable").empty() || !condition->getAttribute("name").empty())
					{
						fail(conditionHere, "set=\"" + setName + "\" takes a <conditions> set whole, so it has nothing else");
					}
					const auto set = conditionSets.find(setName);
					if (set == conditionSets.end()) { fail(conditionHere, "set=\"" + setName + "\" names no <conditions> set before it"); }
					out.insert(out.end(), set->second.begin(), set->second.end());
					continue;
				}

				RawCondition raw;
				raw.name = condition->getAttribute("name");
				if (!raw.name.empty() && std::any_of(out.begin(), out.end(), [&](const RawCondition& other) { return other.name == raw.name; }))
				{
					fail(conditionHere, "has two conditions called '" + raw.name + "'");
				}
				std::unique_ptr<XmlNode> after = readCheck(*condition, conditionHere, raw);
				raw.commands = readCommands(std::move(after), conditionHere);
				out.push_back(std::move(raw));
			}
		}

		// Objects laid out by load/merge, each read as if it had been written
		// out whole there: an <object>, a <group> or a <map>.
		void readAll(const std::vector<ElementPtr>& list, std::vector<RawObject>& out)
		{
			for (const ElementPtr& element : list)
			{
				const std::unique_ptr<XmlNode> node = viewOf(element);
				if (element->name == "group") { readGroup(*node, out); }
				else if (element->name == "map") { readMap(*node, out); }
				else { out.push_back(readObject(*node)); }
			}
		}

		// What a <spawn>, <fire> or <release> in those children of a state or
		// scene names (load/merge spawnedIn): a piece of the <kit> named so
		// comes into the list they run beside (addWaiting).
		std::set<std::string> spawnedBy(const XmlNode& node, const std::vector<std::string>& tags, const Kit& kit)
		{
			std::set<std::string> names;
			for (const std::string& tag : tags)
			{
				if (std::unique_ptr<XmlNode> child = findChild(&node, tag))
				{
					const std::set<std::string> more = spawnedIn(copyElement(*child), kit);
					names.insert(more.begin(), more.end());
				}
			}
			return names;
		}

		std::set<std::string> namesOf(const std::vector<ElementPtr>& list, std::set<std::string> names)
		{
			for (const ElementPtr& element : list)
			{
				const std::set<std::string> own = namesIn(element);
				names.insert(own.begin(), own.end());
			}
			return names;
		}

		// <scenes>: each <scene>, its objects laid out by load/merge: those of
		// the scene it is from=, changed by its own of the same names, then its
		// own and the <kit> pieces it takes. `seen` is every name of its state's
		// objects and the game's.
		void readScenes(const XmlNode& scenes, const std::string& stateWhere, const ConditionSets& conditionSets, const Kit& kit, const std::set<std::string>& seen, RawState& state)
		{
			if (findChild(&scenes, "objects"))
			{
				fail(stateWhere + " > <scenes>", "has an <objects>, a kit of its own: the pieces the scenes take go in the game's <kit>");
			}

			// Each scene so far, laid out, for a scene that is from= it.
			std::map<std::string, std::vector<ElementPtr>> lists;

			for (std::unique_ptr<XmlNode> node = scenes.getFirstChild(); node != nullptr; node = node->getNextSibling())
			{
				if (node->getName() != "scene") { continue; }

				RawScene scene;
				scene.name = requireAttribute(*node, "name", stateWhere + " > <scenes>");
				scene.from = node->getAttribute("from");
				const std::string where = stateWhere + " > scene '" + scene.name + "'";
				if (lists.count(scene.name)) { fail(stateWhere, "has two scenes called '" + scene.name + "'"); }
				if (scene.name.find('.') != std::string::npos) { fail(where, "a scene's name has no dots: they join the parts of a path (" + state.name + "." + scene.name + ")"); }

				std::vector<ElementPtr> list;
				if (!scene.from.empty())
				{
					const auto from = lists.find(scene.from);
					if (from == lists.end()) { fail(where, "from=\"" + scene.from + "\" names no scene before it"); }
					list = from->second;
				}

				if (std::unique_ptr<XmlNode> ways = findChild(node.get(), "ways"))
				{
					for (std::unique_ptr<XmlNode> way = ways->getFirstChild(); way != nullptr; way = way->getNextSibling())
					{
						if (way->getName() != "way") { continue; }
						const std::string wayWhere = where + " > <ways>";
						const std::string name = requireAttribute(*way, "name", wayWhere);
						const std::string to = requireAttribute(*way, "scene", wayWhere);
						const bool twice = std::any_of(scene.ways.begin(), scene.ways.end(), [&](const auto& each) { return each.first == name; });
						if (twice) { fail(wayWhere, "has two ways called '" + name + "'"); }
						scene.ways.emplace_back(name, to);
					}
				}

				readVariables(findChild(node.get(), "variables").get(), where + " > <variables>", scene.variables);

				if (std::unique_ptr<XmlNode> objects = findChild(node.get(), "objects"))
				{
					list = layOut(*objects, std::move(list), kit, where + " > <objects>");
				}

				if (std::unique_ptr<XmlNode> conditions = findChild(node.get(), "conditions"))
				{
					readConditions(*conditions, where, conditionSets, scene.conditions);
				}
				scene.timers = readTimers(findChild(node.get(), "timers").get(), where, false);

				addWaiting(list, spawnedBy(*node, { "conditions", "timers" }, kit), seen, kit, where);
				readAll(list, scene.objects);

				lists[scene.name] = std::move(list);
				state.scenes.push_back(std::move(scene));
			}
		}

		// What comes after a state's <inputs>: its <conditions> (optional - the
		// schema allows a state with none) and its <timers> (optional: counted
		// while this is the current state).
		RawState finishState(const XmlNode& state, const std::string& where, const ConditionSets& conditionSets, RawState rawState)
		{
			if (std::unique_ptr<XmlNode> conditions = findChild(&state, "conditions"))
			{
				readConditions(*conditions, where, conditionSets, rawState.conditions);
			}
			if (std::unique_ptr<XmlNode> computer = findChild(&state, "computer"))
			{
				readConditions(*computer, where + " > <computer>", conditionSets, rawState.computer);
			}
			rawState.timers = readTimers(findChild(&state, "timers").get(), where, false);
			return rawState;
		}

		// A <state>. Its objects are laid out by load/merge: those of the state
		// it is from= (`lists`, each state so far laid out), changed by its own
		// of the same names, then its own and the <kit> pieces it takes.
		RawState readState(const XmlNode& state, std::map<std::string, std::vector<ElementPtr>>& lists, const std::map<std::string, KeyBindings>& keySets,
			const ShowSets& showSets, const ConditionSets& conditionSets, const Kit& kit, const std::set<std::string>& gameNames)
		{
			RawState rawState{};
			rawState.name = requireAttribute(state, "name", "<state>");
			rawState.from = state.getAttribute("from");

			const std::string where = "state '" + rawState.name + "'";

			std::vector<ElementPtr> list;
			if (!rawState.from.empty())
			{
				const auto from = lists.find(rawState.from);
				if (from == lists.end()) { fail(where, "from=\"" + rawState.from + "\" names no state before it"); }
				list = from->second;
			}

			// <objects>: its own, in play in every scene of it
			if (std::unique_ptr<XmlNode> objects = findChild(&state, "objects"))
			{
				list = layOut(*objects, std::move(list), kit, where + " > <objects>");
			}

			// <shows> (optional): objects of the game's, named <shows> sets
			// taken whole, and states drawn still underneath
			if (std::unique_ptr<XmlNode> shows = findChild(&state, "shows"))
			{
				readShows(*shows, where + " > <shows>", showSets, rawState.show, &rawState.showStates);
			}

			// <music sound="..." /> (optional)
			if (std::unique_ptr<XmlNode> music = findChild(&state, "music"))
			{
				rawState.music = requireAttribute(*music, "sound", where + " > <music>");
			}

			// <inputs>: the named <keys> sets it uses (keys="player", or a <keys>
			// each), in order, each over the one before, then its own <input>s
			// over those: a state can use a set and still give one of its keys a
			// job of its own.
			std::unique_ptr<XmlNode> inputs = rawState.from.empty() ? requireChild(state, "inputs", where) : findChild(&state, "inputs");
			if (inputs)
			{
				for (const std::string& setName : namesGiven(*inputs, "keys", where + " > <inputs>"))
				{
					const auto set = keySets.find(setName);
					if (set == keySets.end()) { fail(where + " > <inputs>", "keys=\"" + setName + "\" names no <keys> set"); }
					for (const auto& [button, commands] : set->second.buttons) { rawState.input[button] = commands; }
					for (const auto& [axis, commands] : set->second.axes) { rawState.axes[axis] = commands; }
				}
				KeyBindings own;
				readInputs(*inputs, where + " > <inputs>", own);
				for (auto& [button, commands] : own.buttons) { rawState.input[button] = std::move(commands); }
				for (auto& [axis, commands] : own.axes) { rawState.axes[axis] = std::move(commands); }
			}
			rawState = finishState(state, where, conditionSets, std::move(rawState));

			// What its own inputs, conditions and timers spawn (a <kit> piece
			// comes into its objects), then its scenes, then its objects read.
			addWaiting(list, spawnedBy(state, { "inputs", "conditions", "computer", "timers" }, kit), gameNames, kit, where);

			if (std::unique_ptr<XmlNode> scenes = findChild(&state, "scenes"))
			{
				readScenes(*scenes, where, conditionSets, kit, namesOf(list, gameNames), rawState);
			}

			readAll(list, rawState.objects);
			lists[rawState.name] = std::move(list);
			return rawState;
		}
	}

	void game_xml::init(const std::string& filename, XmlBackend backend, WindowDesc& windowDesc,
		std::vector<std::pair<std::string, RawValue>>& rawVariables, std::vector<RawState>& rawStates,
		std::vector<RawObject>& rawObjects, std::vector<RawSound>& rawSounds, std::vector<RawPath>& rawPaths,
		std::vector<RawStyle>& rawStyles, std::vector<std::string>& rawKeeps, RawDemo& rawDemo, SchemaValidation& validation)
	{
		std::unique_ptr<XmlDocument> document = XmlDocumentFactory::create(backend);

		if (!document->load(filename))
		{
			// Thrown, not exit(): a front end that loads games one after another
			// (xgegui) tells the user and carries on.
			throw std::runtime_error("XML file failed to load: " + document->getErrorMessage());
		}

		// find key points in document
		std::unique_ptr<XmlNode> root = document->getRootElement();

		// A game file names its schema the same way regardless of backend
		// (xsi:noNamespaceSchemaLocation - see games/descriptions/*.xml), so this reads
		// it directly off root rather than asking XmlDocument for it.
		// Xerces already ran its own real validation as part of load()
		// above whenever this is non-empty (see xml/xerces.cpp) - load()
		// would have failed already otherwise, so reaching here means it
		// passed. Every other backend can only check well-formedness on its
		// own (see xml/document.h), so this falls back to XsdLiteValidator
		// - this project's own interpreter for the subset of XSD this
		// schema actually uses (see load/xsd.h) - against that same schema
		// file, resolved relative to filename the same way Xerces resolves
		// it relative to the document.
		const std::string schemaLocation = getAttribute(root.get(), "xsi:noNamespaceSchemaLocation");

		if (schemaLocation.empty())
		{
			validation = SchemaValidation::None;
		}
		else if (backend == XmlBackend::Xerces)
		{
			validation = SchemaValidation::Strong;
		}
		else
		{
			const std::string schemaPath = findSchema(filename, schemaLocation).string();

			XsdLiteValidator validator;

			if (!validator.loadSchema(schemaPath, backend) || !validator.validate(*root))
			{
				throw std::runtime_error("XML file failed to validate against the schema: " + validator.getErrorMessage());
			}

			validation = SchemaValidation::Weak;
		}

		const std::string where = "game";
		std::unique_ptr<XmlNode> window = requireChild(*root, "window", where);
		std::unique_ptr<XmlNode> variablesNode = requireChild(*root, "variables", where);
		std::unique_ptr<XmlNode> kitNode = findChild(root.get(), "kit");         // none: nothing is taken
		std::unique_ptr<XmlNode> objectsNode = findChild(root.get(), "objects"); // none: every object is a state's own
		std::unique_ptr<XmlNode> states = requireChild(*root, "states", where);

		// load window description: <window name="..."> and its settings. These
		// are plain numbers, not expressions - the expressions in the rest of
		// the file are worked out against the window's size.
		const std::string windowHere = "<window>";
		windowDesc.name = requireAttribute(*window, "name", windowHere);
		windowDesc.width = std::stof(readText(*requireChild(*window, "width", windowHere)));
		windowDesc.height = std::stof(readText(*requireChild(*window, "height", windowHere)));
		if (std::unique_ptr<XmlNode> scale = findChild(window.get(), "scale"))
		{
			windowDesc.scale = std::stoi(readText(*scale));
			if (windowDesc.scale < 1) { fail(windowHere, "<scale> is a whole number, 1 or more"); }
		}
		// Left out: black, a window, 60 frames a second.
		const auto background = findChild(window.get(), "background");
		const auto fullscreen = findChild(window.get(), "fullscreen");
		const auto framerate = findChild(window.get(), "framerate");
		windowDesc.background = background ? readText(*background) : "color.black";
		windowDesc.fullscreen = fullscreen && readBool(*fullscreen, windowHere) ? "true" : "false";
		windowDesc.framerate = framerate ? std::stoi(readText(*framerate)) : 60;

		// load variables
		readVariables(variablesNode.get(), "<variables>", rawVariables);

		// load sounds (optional: a game can be silent)
		if (std::unique_ptr<XmlNode> soundsNode = findChild(root.get(), "sounds"))
		{
			for (std::unique_ptr<XmlNode> sound = soundsNode->getFirstChild(); sound != nullptr; sound = sound->getNextSibling())
			{
				rawSounds.push_back(readSound(*sound));
			}
		}

		// load paths (optional: only a game whose objects fly them has any)
		if (std::unique_ptr<XmlNode> pathsNode = findChild(root.get(), "paths"))
		{
			for (std::unique_ptr<XmlNode> path = pathsNode->getFirstChild(); path != nullptr; path = path->getNextSibling())
			{
				rawPaths.push_back(readPath(*path));
			}
		}

		// load styles (optional: what a <text> or a <bitmap> can take by style="...")
		if (std::unique_ptr<XmlNode> stylesNode = findChild(root.get(), "styles"))
		{
			for (std::unique_ptr<XmlNode> style = stylesNode->getFirstChild(); style != nullptr; style = style->getNextSibling())
			{
				RawStyle raw;
				raw.name = requireAttribute(*style, "name", "<styles> > <style>");
				const std::string here = "<styles> > <style name=\"" + raw.name + "\">";
				if (std::any_of(rawStyles.begin(), rawStyles.end(), [&](const RawStyle& each) { return each.name == raw.name; }))
				{
					fail("<styles>", "two <style>s are called '" + raw.name + "'");
				}
				if (auto size = findChild(style.get(), "size")) { raw.size = readValue(*size, here); }
				if (auto scale = findChild(style.get(), "scale"))
				{
					raw.hasScale = true;
					raw.scale = readValue(*scale, here);
				}
				if (auto color = findPlainColor(*style)) { raw.color = readText(*color); }
				readBitmapColors(*style, here, raw.bitmapColors);
				if (auto font = findChild(style.get(), "font")) { raw.font = readText(*font); }
				rawStyles.push_back(std::move(raw));
			}
		}

		// load the kit: pieces written once, in play only where something
		// takes them (load/merge)
		Kit kit;
		if (kitNode)
		{
			for (std::unique_ptr<XmlNode> piece = kitNode->getFirstChild(); piece != nullptr; piece = piece->getNextSibling())
			{
				kit.add(copyElement(*piece), "<kit>");
			}
		}
		if (std::unique_ptr<XmlNode> pathsNode = findChild(root.get(), "paths"))
		{
			for (std::unique_ptr<XmlNode> path = pathsNode->getFirstChild(); path != nullptr; path = path->getNextSibling())
			{
				kit.addSet("path", path->getAttribute("name"), copyElement(*path));
			}
		}

		// load objects: the game's own, in play in every state that shows them
		std::vector<ElementPtr> gameList;
		if (objectsNode) { gameList = layOut(*objectsNode, {}, kit, "the game's <objects>"); }
		addWaiting(gameList, {}, {}, kit, "the game's <objects>");
		readAll(gameList, rawObjects);
		const std::set<std::string> gameNames = namesOf(gameList, {});

		// load keeps (optional: what a full reset passes over)
		if (std::unique_ptr<XmlNode> keepsNode = findChild(root.get(), "keeps"))
		{
			for (std::unique_ptr<XmlNode> keep = keepsNode->getFirstChild(); keep != nullptr; keep = keep->getNextSibling())
			{
				rawKeeps.push_back(requireAttribute(*keep, "object", "<keeps> > <keep>"));
			}
		}

		// load the demo (optional: a recorded game for the attract mode)
		if (std::unique_ptr<XmlNode> demoNode = findChild(root.get(), "demo"))
		{
			const std::string demoWhere = "<demo>";
			const auto whole = [&](const XmlNode& node, const std::string& here)
			{
				const std::string text = readText(node);
				if (text.empty() || text.find_first_not_of("0123456789") != std::string::npos)
				{
					fail(here, "\"" + text + "\" is not a whole number");
				}
				return static_cast<unsigned int>(std::stoul(text));
			};
			rawDemo.recorded = true;
			rawDemo.seed = whole(*requireChild(*demoNode, "seed", demoWhere), demoWhere + " > <seed>");
			rawDemo.frames = whole(*requireChild(*demoNode, "frames", demoWhere), demoWhere + " > <frames>");
			for (std::unique_ptr<XmlNode> child = demoNode->getFirstChild(); child != nullptr; child = child->getNextSibling())
			{
				const std::string tag = child->getName();
				if (tag != "press" && tag != "release") { continue; }
				const std::string here = demoWhere + " > <" + tag + ">";
				DemoKey key;
				key.button = requireAttribute(*child, "button", here);
				key.pressed = tag == "press";
				key.frame = whole(*child, here);
				if (keyCodeFromString(key.button) == KeyCode::Unknown)
				{
					fail(here, "there is no key named '" + key.button + "'" + didYouMean(key.button, keyNames()));
				}
				rawDemo.keys.push_back(std::move(key));
			}
		}

		// load states: the named <keys>, <shows> and <conditions> sets first, so
		// any state can use any of them (a <shows> or <conditions> set can take
		// a set written before it)
		std::map<std::string, KeyBindings> keySets;
		ShowSets showSets;
		ConditionSets conditionSets;
		for (std::unique_ptr<XmlNode> node = states->getFirstChild(); node != nullptr; node = node->getNextSibling())
		{
			if (node->getName() == "keys")
			{
				const std::string name = requireAttribute(*node, "name", "<states> > <keys>");
				if (keySets.count(name)) { fail("<states>", "two <keys> sets are called '" + name + "'"); }
				readInputs(*node, "<keys name=\"" + name + "\">", keySets[name]);
				kit.addSet("keys", name, copyElement(*node));
				if (!keySets[name].axes.empty())
				{
					fail("<keys name=\"" + name + "\">", "an <input axis=\"...\"> goes in a state's own <inputs>, not in a <keys> set");
				}
			}
			else if (node->getName() == "shows")
			{
				const std::string name = requireAttribute(*node, "name", "<states> > <shows>");
				if (showSets.count(name)) { fail("<states>", "two <shows> sets are called '" + name + "'"); }
				std::vector<std::string> objects;
				readShows(*node, "<shows name=\"" + name + "\">", showSets, objects);
				showSets[name] = std::move(objects);
			}
			else if (node->getName() == "conditions")
			{
				const std::string name = requireAttribute(*node, "name", "<states> > <conditions>");
				if (conditionSets.count(name)) { fail("<states>", "two <conditions> sets are called '" + name + "'"); }
				std::vector<RawCondition> conditions;
				readConditions(*node, "<conditions name=\"" + name + "\">", conditionSets, conditions);
				conditionSets[name] = std::move(conditions);
				kit.addSet("conditions", name, copyElement(*node));
			}
		}

		std::map<std::string, std::vector<ElementPtr>> stateLists;
		for (std::unique_ptr<XmlNode> state = states->getFirstChild(); state != nullptr; state = state->getNextSibling())
		{
			if (state->getName() != "state") { continue; }
			rawStates.push_back(readState(*state, stateLists, keySets, showSets, conditionSets, kit, gameNames));
		}
	}

	GameData readGame(const std::string& filename, XmlBackend backend)
	{
		GameData data;
		SchemaValidation validation = SchemaValidation::None;
		game_xml{}.init(filename, backend, data.window, data.variables, data.states, data.objects, data.sounds, data.paths, data.styles, data.keeps, data.demo, validation);

		std::ostringstream said;
		said << validation;
		data.validation = said.str();
		return data;
	}

	Game::Game(const std::string& file) :
		Game(readGame(file))
	{
	}

	std::string withDemo(const std::string& gameText, const RawDemo& demo)
	{
		std::ostringstream element;
		element << "  <demo>\n"
			<< "    <seed>" << demo.seed << "</seed>\n"
			<< "    <frames>" << demo.frames << "</frames>\n";
		for (const DemoKey& key : demo.keys)
		{
			const char* tag = key.pressed ? "press" : "release";
			element << "    <" << tag << " button=\"" << key.button << "\">" << key.frame << "</" << tag << ">\n";
		}
		element << "  </demo>\n";

		// The one it had goes, from the start of its line to the end of it.
		std::string text = gameText;
		const auto open = text.find("<demo>", text.rfind("</states>") == std::string::npos ? 0 : text.rfind("</states>"));
		if (open != std::string::npos)
		{
			const auto close = text.find("</demo>", open);
			if (close != std::string::npos)
			{
				const auto lineStart = text.rfind('\n', open);
				auto end = text.find('\n', close);
				end = end == std::string::npos ? text.size() : end + 1;
				text.erase(lineStart == std::string::npos ? 0 : lineStart + 1, end - (lineStart == std::string::npos ? 0 : lineStart + 1));
			}
		}

		const auto gameEnd = text.rfind("</game>");
		if (gameEnd == std::string::npos)
		{
			throw std::runtime_error("the game file has no </game> to write the demo before");
		}

		// Lines end as the file's do (a Windows checkout's end in \r\n).
		std::string lines = element.str();
		if (text.find("\r\n") != std::string::npos)
		{
			std::string crlf;
			for (const char c : lines)
			{
				if (c == '\n') { crlf += '\r'; }
				crlf += c;
			}
			lines = std::move(crlf);
		}

		const auto lineStart = text.rfind('\n', gameEnd);
		text.insert(lineStart == std::string::npos ? gameEnd : lineStart + 1, lines);
		return text;
	}
}
