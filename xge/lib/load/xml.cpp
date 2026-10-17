// xml.cpp
// XML Game Engine
// author: beefviper
// date: Sept 21, 2020

#include "load/xml.h"

#include "engine/folder.h"
#include "game/game.h"
#include "engine/keys.h"
#include "load/spelling.h"
#include "load/xsd.h"

#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <filesystem>
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

		RawValue readRandom(const XmlNode& tag, const std::string& here)
		{
			RawValue value;
			value.kind = RawValue::Kind::Random;
			value.min = requireAttribute(tag, "min", here);
			value.max = requireAttribute(tag, "max", here);
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

		// An operand of an equation or formula is a name or a number and
		// nothing else: anything more is an operation of its own.
		std::string requireTerm(const std::string& text, const std::string& here)
		{
			if (!isNumber(text) && !isName(text, true))
			{
				fail(here, "\"" + text + "\" is not a name or a number; an operand is one or the other, so write anything more than that as an operation of its own");
			}
			return text;
		}

		// One step of an <equation>: <divide name="half" dividend="title.width"
		// divisor="2" />. The operands are attributes, a name or a number each;
		// a name is an earlier step's, or one the game's expressions can use.
		RawOperation readEquationStep(const XmlNode& node, const OperationShape& shape, const std::string& where)
		{
			const std::string here = where + " > <" + shape.tag + ">";

			if (node.getFirstChild()) { fail(here, "is a step of an <equation>, which takes its operands as attributes (" + std::string(shape.first) + "=\"...\" " + shape.rest + "=\"...\"); operands written as elements belong in a <formula>"); }

			RawOperation step;
			step.op = shape.tag;
			step.name = trim(node.getAttribute("name"));

			if (!step.name.empty() && !isName(step.name, false)) { fail(here, "name=\"" + step.name + "\" is not a name; a step's name is letters, digits and underscores, with no dots"); }

			for (const char* role : { shape.first, shape.rest })
			{
				RawOperand operand;
				operand.role = role;
				operand.value = RawValue::expression(requireTerm(trim(requireAttribute(node, role, here)), here + " " + role));
				step.operands.push_back(std::move(operand));
			}

			return step;
		}

		RawValue readEquation(const XmlNode& node, const std::string& where)
		{
			const std::string here = where + " > <equation>";
			auto steps = std::make_shared<std::vector<RawOperation>>();
			std::set<std::string> names;

			for (std::unique_ptr<XmlNode> child = node.getFirstChild(); child; child = child->getNextSibling())
			{
				const OperationShape* shape = operationShape(child->getName());
				if (!shape) { fail(here, "has <" + child->getName() + ">, which is not a step; a step is <add>, <subtract>, <multiply> or <divide>"); }

				RawOperation step = readEquationStep(*child, *shape, here);
				if (!step.name.empty() && !names.insert(step.name).second) { fail(here, "two steps are called \"" + step.name + "\""); }
				steps->push_back(std::move(step));
			}

			if (steps->empty()) { fail(here, "has no steps"); }

			RawValue value;
			value.kind = RawValue::Kind::Equation;
			value.operations = std::move(steps);
			return value;
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
			if (!shape) { fail(here, "has <" + tag->getName() + ">, which is not an operation; expected a name, a number, <random>, <add>, <subtract>, <multiply> or <divide>"); }

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

			if (!node.getAttribute("name").empty()) { fail(here, "has a name; only a step of an <equation> is named, and an operation nested in a <formula> is used where it stands"); }

			RawOperation operation;
			operation.op = shape.tag;

			for (std::unique_ptr<XmlNode> child = node.getFirstChild(); child; child = child->getNextSibling())
			{
				const char* expected = operation.operands.empty() ? shape.first : shape.rest;
				if (child->getName() != expected)
				{
					fail(here, "has <" + child->getName() + "> where <" + expected + "> belongs: " + shape.first + " first, then one or more " + shape.rest);
				}

				RawOperand operand;
				operand.role = expected;
				operand.value = readFormulaOperand(*child, here);
				operation.operands.push_back(std::move(operand));
			}

			if (operation.operands.size() < 2) { fail(here, "needs a <" + std::string(shape.first) + "> and at least one <" + shape.rest + ">"); }

			return operation;
		}

		RawValue readFormula(const XmlNode& node, const std::string& where)
		{
			const std::string here = where + " > <formula>";
			std::unique_ptr<XmlNode> tag = node.getFirstChild();

			if (!tag) { fail(here, "has no operation"); }
			if (tag->getNextSibling()) { fail(here, "holds more than one operation; a formula is one, with the others inside it"); }

			const OperationShape* shape = operationShape(tag->getName());
			if (!shape) { fail(here, "has <" + tag->getName() + ">, which is not an operation; expected <add>, <subtract>, <multiply> or <divide>"); }

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
			if (tagName == "equation") { return readEquation(*tag, here); }
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
					fail(here, "needs a <content> (a label), a <number> or a <letter>");
				}

				sprite.style = shape.getAttribute("style");
				if (auto size = findChild(&shape, "size")) { sprite.size = readValue(*size, here); }
				else if (sprite.style.empty()) { fail(here, "needs a <size>, or a style= whose <style> has one"); }
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
			else if (base.kind == "text")
			{
				// Pieces given are the whole text; none, and it is kept.
				if (std::vector<RawTextPiece> text = readTextPieces(*shape, here); !text.empty())
				{
					sprite.text = std::move(text);
				}
				change("size", sprite.size);
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
				if (base.kind == "bitmap") { readBitmapColors(*shape, here, sprite.bitmapColors); }
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

		// An <animation> as written: how long each picture is shown, and the
		// names of the sprites it shows, in order. Which sprites those are is
		// settled by chooseSprite, once the sprites it can pick from are known
		// (a group's member can bring its own).
		struct AnimationSpec
		{
			RawValue interval;
			std::vector<std::string> frames;
		};

		AnimationSpec readAnimation(const XmlNode& animation, const std::string& where)
		{
			const std::string here = where + " > <animation>";
			AnimationSpec spec;

			spec.interval = readValueOf(animation, "interval", here);

			for (std::unique_ptr<XmlNode> node = animation.getFirstChild(); node != nullptr; node = node->getNextSibling())
			{
				if (node->getName() == "frame") { spec.frames.push_back(requireAttribute(*node, "sprite", here + " > <frame>")); }
				else if (node->getName() != "interval") { fail(here, "unknown <" + node->getName() + ">; expected an <interval> and then <frame>s"); }
			}

			return spec;
		}

		// Settles what `object` looks like from the <sprite>s it has and its
		// <animation>, if any. One sprite and no animation is the ordinary case.
		// With an animation, each <frame> names one of the sprites, and the
		// object is the first of them until it moves on. Anything that cannot
		// be settled, such as several sprites and nothing to say which is shown
		// when, or a frame naming a sprite that is not there, stops the load.
		void chooseSprite(const std::vector<RawSprite>& sprites, const std::optional<AnimationSpec>& animation,
			const std::string& where, RawObject& object)
		{
			if (sprites.empty()) { fail(where, "missing <sprite>"); }

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

		bool isCommandTag(const std::string& name)
		{
			return name == "bounce" || name == "deflect" || name == "knock" || name == "stick" || name == "wrap" || name == "ride" || name == "carry" || name == "drop" || name == "track" || name == "die"
				|| name == "reset" || name == "inc" || name == "dec" || name == "copy" || name == "rank" || name == "turns" || name == "pass" || name == "out" || name == "demo" || name == "move" || name == "hop"
				|| name == "accelerate" || name == "turn" || name == "thrust" || name == "release" || name == "stop"
				|| name == "push" || name == "pop" || name == "fire" || name == "trigger" || name == "play" || name == "silence"
				|| name == "jump" || name == "land" || name == "leap" || name == "climb" || name == "chase" || name == "go" || name == "hunt" || name == "wander" || name == "aim" || name == "reverse" || name == "become" || name == "reveal" || name == "follow" || name == "enter" || name == "set" || name == "spawn";
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
			if (verb == "become") { requireAttribute(node, "sprite", where); }
			if (verb == "reveal")
			{
				requireAttribute(node, "object", where);

				// How many is optional: a bare <reveal object="..." /> is one.
				if (node.getFirstChild() || !readText(node).empty()) { command.amount = readValue(node, where); }
				else { command.amount = RawValue::expression("1"); }
			}
			if (verb == "trigger") { requireAttribute(node, "action", where); }
			if (verb == "hop" && (findChild(&node, "x") || findChild(&node, "y")))
			{
				// A diagonal one's step across and up or down, each its own.
				requireAttribute(node, "direction", where);
				const PartialVector2 step = readPartialVector2(node, where);
				if (!step.x || !step.y) { fail(where, "<hop> gives one distance, or an <x> across and a <y> up or down, both"); }
				if (command.direction.find('-') == std::string::npos) { fail(where, "<hop direction=\"" + command.direction + "\"> is not diagonal; it gives one distance"); }
				command.positionX = *step.x;
				command.positionY = *step.y;
			}
			else if (verb == "move" || verb == "hop" || verb == "accelerate" || verb == "turn")
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
				// A distance and a time: two values, so two elements.
				requireAttribute(node, "direction", where);
				const std::string here = where + " > <jump>";
				for (std::unique_ptr<XmlNode> child = node.getFirstChild(); child != nullptr; child = child->getNextSibling())
				{
					if (child->getName() != "distance" && child->getName() != "seconds")
					{
						fail(here, "unknown <" + child->getName() + ">; expected <distance> and <seconds>");
					}
				}
				command.amount = readValueOf(node, "distance", here);
				if (auto seconds = findChild(&node, "seconds")) { command.seconds = readValue(*seconds, here); }
			}
			if (verb == "aim") { requireAttribute(node, "object", where); }
			if (verb == "chase")
			{
				// A speed and how near is near enough: two values, so two elements.
				requireAttribute(node, "object", where);
				const std::string here = where + " > <chase>";
				for (std::unique_ptr<XmlNode> child = node.getFirstChild(); child != nullptr; child = child->getNextSibling())
				{
					if (child->getName() != "speed" && child->getName() != "near")
					{
						fail(here, "unknown <" + child->getName() + ">; expected <speed> and <near>");
					}
				}
				command.amount = readValueOf(node, "speed", here);
				if (auto nearNode = findChild(&node, "near")) { command.distance = readValue(*nearNode, here); }
			}
			if (verb == "go")
			{
				requireAttribute(node, "direction", where);
				command.amount = readValue(node, where);
			}
			if (verb == "wander")
			{
				const std::string here = where + " > <wander>";
				for (std::unique_ptr<XmlNode> child = node.getFirstChild(); child != nullptr; child = child->getNextSibling())
				{
					if (child->getName() != "speed") { fail(here, "unknown <" + child->getName() + ">; expected <speed>"); }
				}
				command.amount = readValueOf(node, "speed", here);
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
			if (verb == "hunt")
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
					else if (tag != "speed" && tag != "ahead")
					{
						fail(here, "unknown <" + tag + ">; expected <speed>, then <ahead>, <with> and <retreat>, each if wanted");
					}
				}
				command.amount = readValueOf(node, "speed", here);
				if (auto ahead = findChild(&node, "ahead")) { command.ahead = readValue(*ahead, here); }
			}
			if (verb == "follow")
			{
				requireAttribute(node, "path", where);
				const std::string here = where + " > <follow>";
				for (std::unique_ptr<XmlNode> child = node.getFirstChild(); child != nullptr; child = child->getNextSibling())
				{
					if (child->getName() != "stagger") { fail(here, "unknown <" + child->getName() + ">; expected <stagger>"); }
				}
				if (auto stagger = findChild(&node, "stagger")) { command.seconds = readValue(*stagger, here); }
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

		std::string readFacing(const XmlNode& node, const std::string& where)
		{
			const std::string facing = readText(node);
			if (facing != "up" && facing != "down" && facing != "left" && facing != "right")
			{
				fail(where, "<facing> is \"" + facing + "\"; expected up, down, left or right");
			}
			return facing;
		}

		void appendCommands(std::vector<RawCommand>& existing, const std::vector<RawCommand>& more)
		{
			existing.insert(existing.end(), more.begin(), more.end());
		}

		// An edge rule's commands appended to one edge's list, noting where
		// they are when the rule holds to a look (sprite=) or is passed over
		// while touching a class (unless=).
		void appendEdgeRule(std::vector<RawCommand>& existing, std::vector<EdgeGuard>& guards,
			const std::vector<RawCommand>& more, const std::string& sprite, const std::string& unlessClass)
		{
			if ((!sprite.empty() || !unlessClass.empty()) && !more.empty())
			{
				guards.push_back(EdgeGuard{ existing.size(), more.size(), sprite, unlessClass });
			}
			appendCommands(existing, more);
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

		// What a <check>, an <all> or an <any> tests (below, with <condition>s).
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

			collisionData.enabled = readBool(*requireChild(collisions, "enabled", collisionsHere), collisionsHere);
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
				while (firstCommand && (firstCommand->getName() == "check" || firstCommand->getName() == "all" || firstCommand->getName() == "any"))
				{
					const std::string name = firstCommand->getName();
					RawCondition part;
					if (name == "check")
					{
						if (readCheck(*firstCommand, ruleHere + " > <check>", part)) { fail(ruleHere + " > <check>", "has its one test and nothing after it; the commands go after the checks"); }
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
					fail(ruleHere, "a <check> on a rule about a screen edge is not built yet; a <condition> of the state can test the same variable");
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
					const bool all = edge == "all";
					const bool horizontal = edge == "horizontal";
					const bool vertical = edge == "vertical";

					if (!all && !horizontal && !vertical && edge != "top" && edge != "bottom" && edge != "left" && edge != "right")
					{
						fail(ruleHere, "edge=\"" + edge + "\"; expected left, right, top, bottom, horizontal, vertical or all");
					}

					// Appends rather than overwrites, so more than one <collision
					// edge="..."> touching the same edge (e.g. an "all" rule plus a
					// specific "left" rule) both run instead of the later one
					// silently winning.
					// class= and object= pick the other object of a touch; an
					// edge is no object, so there is nothing for them to pick
					if (!collision->getAttribute("class").empty() || !collision->getAttribute("object").empty())
					{
						fail(ruleHere, "edge= with class= or object=; a rule about a screen edge is about no other object"
							" (unless=\"class\" passes it over while touching that class)");
					}

					const std::string sprite = collision->getAttribute("sprite");
					const std::string unlessClass = collision->getAttribute("unless");
					if (all || vertical || edge == "top") { appendEdgeRule(collisionData.top, collisionData.topGuards, commands, sprite, unlessClass); }
					if (all || vertical || edge == "bottom") { appendEdgeRule(collisionData.bottom, collisionData.bottomGuards, commands, sprite, unlessClass); }
					if (all || horizontal || edge == "left") { appendEdgeRule(collisionData.left, collisionData.leftGuards, commands, sprite, unlessClass); }
					if (all || horizontal || edge == "right") { appendEdgeRule(collisionData.right, collisionData.rightGuards, commands, sprite, unlessClass); }
				}
				else
				{
					RawCollisionRule rule;
					rule.filterClass = collision->getAttribute("class");
					rule.filterObject = collision->getAttribute("object");
					rule.unlessClass = collision->getAttribute("unless");
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

		// <maze map="maze"> and its <wall class="..." sprite="..." />s.
		RawMaze readMaze(const XmlNode& node, const std::string& where)
		{
			const std::string here = where + " > <maze>";
			RawMaze maze;
			maze.map = requireAttribute(node, "map", here);
			for (std::unique_ptr<XmlNode> child = node.getFirstChild(); child != nullptr; child = child->getNextSibling())
			{
				if (child->getName() != "wall") { fail(here, "unknown <" + child->getName() + ">; expected <wall>s"); }
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

		// An <object>; on a <map> (`map`), one that starts on the one tile of
		// the character it picks, with no <position> of its own.
		RawObject readObject(const XmlNode& object, const MapLayout* map = nullptr)
		{
			RawObject rawObject;
			rawObject.name = requireAttribute(object, "name", "<object>");
			rawObject.objClass = getAttribute(&object, "class");

			const std::string where = "object '" + rawObject.name + "'";

			std::optional<AnimationSpec> animation;
			if (auto node = findChild(&object, "animation")) { animation = readAnimation(*node, where); }
			std::vector<RawSprite> sprites = readSprites(object, where);

			// No <sprite>: an object that only holds numbers (a score, the
			// lives, the clock), never drawn and, unless it says, at 0, 0.
			const bool holder = sprites.empty() && !animation && !map;
			if (holder)
			{
				rawObject.sprite = nothingSprite();
				rawObject.isDrawn = false;
				if (findChild(&object, "drawn")) { fail(where, "has no <sprite>, so it is never drawn; it has no <drawn>"); }
				if (!findChild(&object, "variables") && !findChild(&object, "timers")) { fail(where, "has no <sprite> and no <variables> or <timers>, so it is nothing"); }
			}
			else
			{
				chooseSprite(std::move(sprites), animation, where, rawObject);
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
				else if (holder) { rawObject.rawPosition = { zeroValue(), zeroValue() }; }
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
			if (auto facing = findChild(&object, "facing")) { rawObject.facing = readFacing(*facing, where); }
			if (auto maze = findChild(&object, "maze")) { rawObject.maze = readMaze(*maze, where); }
			rawObject.timers = readTimers(findChild(&object, "timers").get(), where);
			if (auto hidden = findChild(&object, "hidden"))
			{
				if (readBool(*hidden, where)) { rawObject.isVisible = false; }
			}
			if (auto drawn = findChild(&object, "drawn")) { rawObject.isDrawn = readBool(*drawn, where); }
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

		// Which rows or columns a <row number="..."> or <column number="...">
		// picks: numbers counting from 1 (number="2", number="2 4"), or every
		// odd or even one.
		std::vector<bool> readPick(const XmlNode& node, int count, const std::string& where)
		{
			const std::string text = requireAttribute(node, "number", where);
			std::vector<bool> picked(static_cast<std::size_t>(count) + 1, false);

			std::istringstream words(text);
			std::string word;
			bool any = false;
			while (words >> word)
			{
				any = true;
				if (word == "odd" || word == "even")
				{
					for (int i = word == "odd" ? 1 : 2; i <= count; i += 2) { picked[static_cast<std::size_t>(i)] = true; }
				}
				else if (!word.empty() && word.size() <= 4 && std::all_of(word.begin(), word.end(), [](unsigned char c) { return std::isdigit(c) != 0; })
					&& std::stoi(word) >= 1 && std::stoi(word) <= count)
				{
					picked[static_cast<std::size_t>(std::stoi(word))] = true;
				}
				else
				{
					fail(where, "number=\"" + text + "\": \"" + word + "\" is not one of 1 to " + std::to_string(count) + ", odd or even");
				}
			}
			if (!any) { fail(where, "number=\"\" picks nothing; give numbers from 1 to " + std::to_string(count) + ", odd or even"); }

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
			std::optional<AnimationSpec> animation;
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
				else if (tag == "animation") { part.animation = readAnimation(*child, where); }
				else if (tag == "position") { part.position = readPartialVector2(*child, where); }
				else if (tag == "velocity") { part.velocity = readPartialVector2(*child, where); }
				else if (tag == "padding") { part.padding = readPartialVector2(*child, where); }
			}
			readObjectVariables(*node, where, part.variables);

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
			if (one.animation && other.animation) { clash("<animation>"); }
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
			std::optional<AnimationSpec> animation;
			PartialVector2 position;
			PartialVector2 velocity;
			PartialVector2 padding;
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
				if (own.map && (tag == "columns" || tag == "rows" || tag == "padding" || tag == "position" || tag == "member" || tag == "row" || tag == "column" || tag == "cell"))
				{
					fail(where, "is on map '" + own.map->name + "', whose rows say where its members are, so it has no <" + tag + ">");
				}

				if (tag == "hidden") { shared.isVisible = !readBool(*child, where); }
				else if (tag == "drawn") { shared.isDrawn = readBool(*child, where); }
				else if (tag == "columns") { own.columns = readCount(*child, where); }
				else if (tag == "rows") { own.rows = readCount(*child, where); }
				else if (tag == "padding")
				{
					const PartialVector2 given = readPartialVector2(*child, where);
					if (given.x) { own.padding.x = given.x; }
					if (given.y) { own.padding.y = given.y; }
				}
				else if (tag == "animation") { own.animation = readAnimation(*child, where); }
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
				else if (tag == "pull" || tag == "seek") { /* below, all at once */ }
				else if (tag == "collisions") { shared.rawCollisionData = readCollisions(*child, where, shared.rawCollisionData); }
				else if (tag == "facing") { shared.facing = readFacing(*child, where); }
				else if (tag == "maze") { shared.maze = readMaze(*child, where); }
				else if (tag == "group") { inner = true; }
				else if (tag != "sprite" && tag != "actions" && tag != "variables" && tag != "timers" && tag != "member" && tag != "row" && tag != "column" && tag != "cell")
				{
					fail(where, "unknown <" + tag + ">; expected <columns>, <rows>, <padding>, <sprite>, <animation>, <position>, <velocity>, <acceleration>, <drag>, <mass>, <pull>, <seek>, <facing>, <maze>, <hidden>, <drawn>, <collisions>, <actions>, <variables>, <timers>, then <member>s, <group>s, or <row>s, <column>s and <cell>s");
				}
			}

			own.sprites = changeSprites(own.sprites, group, where);
			for (RawTimer& timer : readTimers(findChild(&group, "timers").get(), where)) { shared.timers.push_back(std::move(timer)); }
			readActions(group, where, shared.action);
			readObjectVariables(group, where, shared.variable);
			readPullAndSeek(group, shared, where);
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
			const std::optional<AnimationSpec>& animation = own.animation;
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
				std::optional<AnimationSpec> usedAnimation = animation;
				std::optional<RawValue> positionX = position.x;
				std::optional<RawValue> positionY = position.y;
				std::optional<RawValue> velocityX = velocity.x;
				std::optional<RawValue> velocityY = velocity.y;
				for (const GroupPart* part : parts)
				{
					used = changeSprites(used, *part->node, part->where);
					if (part->animation) { usedAnimation = part->animation; }
					if (part->position.x) { positionX = part->position.x; }
					if (part->position.y) { positionY = part->position.y; }
					if (part->velocity.x) { velocityX = part->velocity.x; }
					if (part->velocity.y) { velocityY = part->velocity.y; }
					for (const auto& [variable, value] : part->variables) { rawObject.variable[variable] = value; }
				}

				if (used.empty()) { fail(here, "has no <sprite>, and neither does its group"); }
				chooseSprite(used, usedAnimation, here, rawObject);

				rawObject.rawPosition.x = pickValue(positionX, std::nullopt, here, "<position><x>");
				rawObject.rawPosition.y = pickValue(positionY, std::nullopt, here, "<position><y>");
				rawObject.rawVelocity.x = velocityX.value_or(zeroValue());
				rawObject.rawVelocity.y = velocityY.value_or(zeroValue());
				return rawObject;
			};

			// On a map: a member for each tile of its character, named by its
			// number in the order the rows are read (dots.17).
			if (own.map)
			{
				const MapLayout& map = *own.map;
				const char character = readMapCharacter(group, map, where, where);
				int count = 0;
				for (const auto& [column, row] : tilesOf(map, character))
				{
					++count;
					const std::string memberName = name + "." + std::to_string(count);
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
					const std::string here = where + " > <" + tag + " number=\"" + child->getAttribute("number") + "\">";
					std::vector<bool> picked = readPick(*child, tag == "row" ? rows : columns, here);
					GroupPart line = readGroupPart(std::move(child), here, { "sprite", "animation", "velocity", "padding", "variables" },
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

		// The <input>s under a node (a state's <inputs>, or a <keys> set): a key,
		// or several keys separated by spaces (button="a left"), or a control
		// that points (axis="mouse.x"), and the commands each one runs. One
		// given twice keeps the later binding.
		void readInputs(const XmlNode& node, const std::string& where, KeyBindings& out)
		{
			for (std::unique_ptr<XmlNode> input = node.getFirstChild(); input != nullptr; input = input->getNextSibling())
			{
				const std::string buttons = input->getAttribute("button");
				const std::string axis = input->getAttribute("axis");
				if (buttons.empty() == axis.empty())
				{
					fail(where, "an <input> names a button=\"...\" or an axis=\"...\", one of the two");
				}
				if (!axis.empty())
				{
					out.axes[axis] = readCommands(input->getFirstChild(), where + " > <input axis=\"" + axis + "\">");
					continue;
				}

				const std::vector<RawCommand> commands = readCommands(input->getFirstChild(), where + " > <input button=\"" + buttons + "\">");

				std::istringstream words(buttons);
				for (std::string button; words >> button;)
				{
					out.buttons[button] = commands;
				}
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

		// An <all> or an <any>, into `raw`: which, and its <check>s, <all>s
		// and <any>s in order.
		void readJoin(const XmlNode& join, const std::string& here, RawCondition& raw)
		{
			raw.join = join.getName() == "all" ? RawCondition::Join::All : RawCondition::Join::Any;
			for (std::unique_ptr<XmlNode> part = join.getFirstChild(); part != nullptr; part = part->getNextSibling())
			{
				const std::string partName = part->getName();
				RawCondition check;
				if (partName == "check")
				{
					const std::string checkHere = here + " > <check>";
					if (readCheck(*part, checkHere, check)) { fail(checkHere, "has its one test and nothing after it; the commands go at the end of the <condition>"); }
				}
				else if (partName == "all" || partName == "any")
				{
					readJoin(*part, here + " > <" + partName + ">", check);
				}
				else
				{
					fail(here, "unknown <" + partName + ">; expected <check>s, <all>s and <any>s");
				}
				raw.parts.push_back(std::move(check));
			}
			if (raw.parts.empty()) { fail(here, "has no <check>"); }
		}

		// What a <condition> or a <check> tests, into `raw`: its filters and
		// what it measures (attributes), then its test tag, or an <all> or
		// <any> of <check>s. Gives back what follows (a condition's commands).
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
					fail(here, "<" + testName + "> says what is tested in each of its <check>s, so the condition has no class=, object=, variable=, total= or distance=");
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
			if (!raw.total.empty() && !raw.distance.empty()) { fail(here, "has total= and distance=; it tests one thing"); }
			if (remaining && (!raw.total.empty() || !raw.distance.empty())) { fail(here, "<remaining> counts what is in play, so it takes no total= or distance="); }
			if (!raw.distance.empty() && !raw.variableName.empty()) { fail(here, "distance=\"" + raw.distance + "\" measures how far, so it reads no variable="); }
			if (raw.total == "count" && !raw.variableName.empty()) { fail(here, "total=\"count\" counts the objects in play, so it reads no variable="); }
			if (!remaining && raw.variableName.empty() && raw.total != "count" && raw.distance.empty())
			{
				fail(here, "<" + testName + "> reads a variable, so it needs variable=\"...\"");
			}

			// Against another variable (<above object="gun2" variable="score" />),
			// or a number.
			raw.otherObject = test->getAttribute("object");
			raw.otherVariable = test->getAttribute("variable");
			if (!raw.otherVariable.empty() || !raw.otherObject.empty())
			{
				const std::string tag = "<" + testName + ">";
				if (remaining) { fail(here, "<remaining> counts objects, so it takes a number, not a variable"); }
				if (raw.otherVariable.empty()) { fail(here, tag + " names object=\"" + raw.otherObject + "\" but no variable= of it to compare with"); }
				if (raw.otherObject.empty() && (!raw.total.empty() || !raw.distance.empty()))
				{
					fail(here, tag + " compares a total or a distance, which has no variables of its own, so it needs object= as well as variable=");
				}
				if (test->getFirstChild() || !readText(*test).empty()) { fail(here, tag + " compares with variable=\"" + raw.otherVariable + "\", so it has no number as well"); }
			}
			else
			{
				raw.threshold = readValue(*test, here);
			}
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

		// The <object>s, <group>s and <map>s of an <objects>, in order: the
		// game's, a state's, or a kit's.
		void readObjects(const XmlNode& objects, std::vector<RawObject>& out)
		{
			for (std::unique_ptr<XmlNode> object = objects.getFirstChild(); object != nullptr; object = object->getNextSibling())
			{
				if (object->getName() == "group") { readGroup(*object, out); }
				else if (object->getName() == "map") { readMap(*object, out); }
				else { out.push_back(readObject(*object)); }
			}
		}

		// Every name the objects can be called by: their own, and their groups'.
		std::set<std::string> namesOf(const std::vector<RawObject>& objects)
		{
			std::set<std::string> names;
			for (const RawObject& object : objects)
			{
				names.insert(object.name);
				if (!object.groupName.empty()) { names.insert(object.groupName); }
				names.insert(object.outerGroups.begin(), object.outerGroups.end());
			}
			return names;
		}

		// What a piece a scene takes changes: whole <sprite>s by name, either
		// half of the <position> and the <velocity>, <hidden>, <drawn> and
		// <variables>.
		RawSceneChange readSceneChange(const XmlNode& node, const std::string& where)
		{
			RawSceneChange change;
			for (std::unique_ptr<XmlNode> child = node.getFirstChild(); child != nullptr; child = child->getNextSibling())
			{
				const std::string tag = child->getName();
				if (tag == "sprite") { change.sprites.push_back(readSprite(*child, where)); }
				else if (tag == "position" || tag == "velocity")
				{
					const PartialVector2 given = readPartialVector2(*child, where);
					if (tag == "position") { change.positionX = given.x; change.positionY = given.y; }
					else { change.velocityX = given.x; change.velocityY = given.y; }
				}
				else if (tag == "hidden") { change.hidden = readBool(*child, where); }
				else if (tag == "drawn") { change.drawn = readBool(*child, where); }
				else if (tag != "variables")
				{
					fail(where, "takes a piece, and a piece taken can change only its <sprite>s, <position>, <velocity>, <hidden>, <drawn> and <variables>, not its <" + tag
						+ ">; give the kit another piece for that");
				}
			}
			readObjectVariables(node, where, change.variables);
			return change;
		}

		// <scenes>: the kit (an <objects> of pieces), then each <scene>. An
		// object or group in a scene named after a piece of the kit, or of an
		// object the scene it is from= has, takes it; anything else is the
		// scene's own and must be complete.
		void readScenes(const XmlNode& scenes, const std::string& stateWhere, const ConditionSets& conditionSets, RawState& state)
		{
			if (std::unique_ptr<XmlNode> kit = findChild(&scenes, "objects")) { readObjects(*kit, state.kit); }
			const std::set<std::string> kitNames = namesOf(state.kit);

			// What each scene so far can be taken from: its own names and those it took.
			std::map<std::string, std::set<std::string>> sceneNames;

			for (std::unique_ptr<XmlNode> node = scenes.getFirstChild(); node != nullptr; node = node->getNextSibling())
			{
				if (node->getName() != "scene") { continue; }

				RawScene scene;
				scene.name = requireAttribute(*node, "name", stateWhere + " > <scenes>");
				scene.from = node->getAttribute("from");
				const std::string where = stateWhere + " > scene '" + scene.name + "'";
				if (sceneNames.count(scene.name)) { fail(stateWhere, "has two scenes called '" + scene.name + "'"); }
				if (scene.name.find('.') != std::string::npos) { fail(where, "a scene's name has no dots: they join the parts of a path (" + state.name + "." + scene.name + ")"); }

				std::set<std::string> names;
				if (!scene.from.empty())
				{
					const auto from = sceneNames.find(scene.from);
					if (from == sceneNames.end()) { fail(where, "from=\"" + scene.from + "\" names no scene before it"); }
					names = from->second;
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
					for (std::unique_ptr<XmlNode> child = objects->getFirstChild(); child != nullptr; child = child->getNextSibling())
					{
						const std::string name = requireAttribute(*child, "name", where + " > <objects>");
						RawSceneEntry entry;
						if (kitNames.count(name) || names.count(name))
						{
							// A piece is taken by a name written in the file (an object, a
							// group, a named member), the one the generated program sees too.
							if (name.find('.') != std::string::npos)
							{
								fail(where, "takes '" + name + "', a numbered member of a group: take the whole group, or give the member a name= in the kit and take that");
							}
							entry.take = name;
							entry.change = readSceneChange(*child, where + " > <" + child->getName() + " name=\"" + name + "\">");
							std::vector<RawObject> taken;
							for (const RawObject& piece : state.kit)
							{
								if (piece.name == name || piece.inGroup(name)) { taken.push_back(piece); }
							}
							const std::set<std::string> more = namesOf(taken);
							names.insert(more.begin(), more.end());
						}
						else
						{
							if (child->getName() == "group") { readGroup(*child, entry.objects); }
							else if (child->getName() == "map") { readMap(*child, entry.objects); }
							else { entry.objects.push_back(readObject(*child)); }
							const std::set<std::string> own = namesOf(entry.objects);
							names.insert(own.begin(), own.end());
						}
						scene.entries.push_back(std::move(entry));
					}
				}

				if (std::unique_ptr<XmlNode> conditions = findChild(node.get(), "conditions"))
				{
					readConditions(*conditions, where, conditionSets, scene.conditions);
				}
				scene.timers = readTimers(findChild(node.get(), "timers").get(), where, false);

				sceneNames[scene.name] = std::move(names);
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
			rawState.timers = readTimers(findChild(&state, "timers").get(), where, false);
			return rawState;
		}

		// Every name the objects of a state can be called by, with those of
		// the state it is a copy of, and so on.
		void stateNames(const RawState& state, const std::vector<RawState>& earlier, std::set<std::string>& names)
		{
			const std::set<std::string> own = namesOf(state.objects);
			names.insert(own.begin(), own.end());
			if (state.from.empty()) { return; }
			const auto from = std::find_if(earlier.begin(), earlier.end(), [&](const RawState& each) { return each.name == state.from; });
			if (from != earlier.end()) { stateNames(*from, earlier, names); }
		}

		RawState readState(const XmlNode& state, const std::vector<RawState>& earlier, const std::map<std::string, KeyBindings>& keySets, const ShowSets& showSets, const ConditionSets& conditionSets)
		{
			RawState rawState{};
			rawState.name = requireAttribute(state, "name", "<state>");
			rawState.from = state.getAttribute("from");

			const std::string where = "state '" + rawState.name + "'";

			// from="...": what the earlier state's objects are called, so that
			// an <object> of one of those names is a change to it
			std::set<std::string> copied;
			if (!rawState.from.empty())
			{
				const auto from = std::find_if(earlier.begin(), earlier.end(), [&](const RawState& each) { return each.name == rawState.from; });
				if (from == earlier.end()) { fail(where, "from=\"" + rawState.from + "\" names no state before it"); }
				stateNames(*from, earlier, copied);
			}

			// <objects>: its own, in play in every scene of it; in a copy, an
			// object or group the copy has is what it changes in it
			if (std::unique_ptr<XmlNode> objects = findChild(&state, "objects"))
			{
				for (std::unique_ptr<XmlNode> object = objects->getFirstChild(); object != nullptr; object = object->getNextSibling())
				{
					const std::string name = object->getAttribute("name");
					if (copied.count(name) && object->getName() != "map")
					{
						RawSceneEntry entry;
						entry.take = name;
						entry.change = readSceneChange(*object, where + " > <" + object->getName() + " name=\"" + name + "\">");
						rawState.changes.push_back(std::move(entry));
					}
					else if (object->getName() == "group") { readGroup(*object, rawState.objects); }
					else if (object->getName() == "map") { readMap(*object, rawState.objects); }
					else { rawState.objects.push_back(readObject(*object)); }
				}
			}

			// <scenes>: the kit, then the scenes
			if (std::unique_ptr<XmlNode> scenes = findChild(&state, "scenes"))
			{
				readScenes(*scenes, where, conditionSets, rawState);
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

			// <inputs>: the named <keys> sets it uses (keys="player menu"), in
			// order, each over the one before, then its own <input>s over those: a
			// state can use a set and still give one of its keys a job of its own.
			std::unique_ptr<XmlNode> inputs = rawState.from.empty() ? requireChild(state, "inputs", where) : findChild(&state, "inputs");
			if (!inputs) { return finishState(state, where, conditionSets, std::move(rawState)); }
			std::istringstream setNames(inputs->getAttribute("keys"));
			for (std::string setName; setNames >> setName;)
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

			return finishState(state, where, conditionSets, std::move(rawState));
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
		std::unique_ptr<XmlNode> objectsNode = findChild(root.get(), "objects"); // none: every object is a state's own
		std::unique_ptr<XmlNode> states = requireChild(*root, "states", where);

		// load window description: <window name="..."> and its settings. These
		// are plain numbers, not expressions - the expressions in the rest of
		// the file are worked out against the window's size.
		const std::string windowHere = "<window>";
		windowDesc.name = requireAttribute(*window, "name", windowHere);
		windowDesc.width = std::stof(readText(*requireChild(*window, "width", windowHere)));
		windowDesc.height = std::stof(readText(*requireChild(*window, "height", windowHere)));
		windowDesc.background = readText(*requireChild(*window, "background", windowHere));
		windowDesc.fullscreen = readBool(*requireChild(*window, "fullscreen", windowHere), windowHere) ? "true" : "false";
		windowDesc.framerate = std::stoi(readText(*requireChild(*window, "framerate", windowHere)));

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

		// load styles (optional: what a <text> can take by style="...")
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
				if (auto color = findChild(style.get(), "color")) { raw.color = readText(*color); }
				rawStyles.push_back(std::move(raw));
			}
		}

		// load objects
		if (objectsNode) { readObjects(*objectsNode, rawObjects); }

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
			}
		}

		for (std::unique_ptr<XmlNode> state = states->getFirstChild(); state != nullptr; state = state->getNextSibling())
		{
			if (state->getName() != "state") { continue; }
			rawStates.push_back(readState(*state, rawStates, keySets, showSets, conditionSets));
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
