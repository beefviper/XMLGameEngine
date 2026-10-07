// game_xml.cpp
// XML Game Engine
// author: beefviper
// date: Sept 21, 2020

#include "game_xml.h"

#include "xsd_lite.h"

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
		// What game_xml knows how to read is fixed by xgedef.xsd,
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
			return name == "circle" || name == "rectangle" || name == "text" || name == "image" || name == "bitmap" || name == "svg";
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
				// checked when it is drawn (bitmap.cpp), with the row named.
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
				sprite.flip = readPictureFlip(shape, here);
			}
			else if (kind == "svg")
			{
				// A drawing in an SVG file, made into a picture when the game
				// loads (svg.cpp): the file, optionally the part of it to take
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
			else if (kind == "text")
			{
				if (auto content = findChild(&shape, "content"))
				{
					sprite.content = readText(*content);
				}
				else if (auto number = findChild(&shape, "number"))
				{
					sprite.textIsNumber = true;
					sprite.number = readValue(*number, here);
				}
				else
				{
					fail(here, "needs a <content> (a label) or a <number>");
				}

				sprite.size = readValueOf(shape, "size", here);
			}
			else
			{
				sprite.path = readText(*requireChild(shape, "path", here));
				if (auto flip = findChild(&shape, "flip")) { sprite.flip = readText(*flip); }
			}

			if (kind != "image" && kind != "svg")
			{
				if (auto color = findChild(&shape, "color")) { sprite.color = readText(*color); }
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
			if (!first) { fail(here, "is empty; expected a shape or a <grid>"); }

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
			else if (first->getName() == "grid")
			{
				const std::string gridHere = here + " > <grid>";
				sprite.isGrid = true;
				sprite.columns = readValueOf(*first, "columns", gridHere);
				sprite.rows = readValueOf(*first, "rows", gridHere);

				if (auto padding = findChild(first.get(), "padding"))
				{
					sprite.hasPadding = true;
					sprite.padding = readVector2(*padding, gridHere);
				}

				std::unique_ptr<XmlNode> shape = first->getFirstChild();
				while (shape && !isShapeTag(shape->getName()))
				{
					if (shape->getName() == "line") { fail(gridHere, "cannot repeat a <line>; draw the lines in one sprite instead"); }
					shape = shape->getNextSibling();
				}
				if (!shape) { fail(gridHere, "needs a shape to repeat (<circle>, <rectangle>, <text>, <image>, <bitmap> or <svg>)"); }

				readShape(*shape, sprite, gridHere);
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

		bool isCommandTag(const std::string& name)
		{
			return name == "bounce" || name == "deflect" || name == "stick" || name == "wrap" || name == "carry" || name == "die"
				|| name == "reset" || name == "inc" || name == "dec" || name == "move" || name == "hop"
				|| name == "accelerate" || name == "turn" || name == "thrust" || name == "release" || name == "stop"
				|| name == "push" || name == "pop" || name == "fire" || name == "trigger" || name == "play"
				|| name == "jump" || name == "land" || name == "leap" || name == "climb" || name == "chase" || name == "aim" || name == "reverse" || name == "become" || name == "reveal" || name == "follow";
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

			const std::string& verb = command.verb;
			if (verb == "inc" || verb == "dec")
			{
				requireAttribute(node, "variable", where);

				// The amount is optional: a bare <inc variable="..." /> is 1.
				if (node.getFirstChild() || !readText(node).empty()) { command.amount = readValue(node, where); }
				else { command.amount = RawValue::expression("1"); }
			}
			if (verb == "push") { requireAttribute(node, "state", where); }
			if (verb == "fire") { requireAttribute(node, "object", where); }
			if (verb == "play") { requireAttribute(node, "sound", where); }
			if (verb == "become") { requireAttribute(node, "sprite", where); }
			if (verb == "reveal")
			{
				requireAttribute(node, "object", where);

				// How many is optional: a bare <reveal object="..." /> is one.
				if (node.getFirstChild() || !readText(node).empty()) { command.amount = readValue(node, where); }
				else { command.amount = RawValue::expression("1"); }
			}
			if (verb == "trigger") { requireAttribute(node, "object", where); requireAttribute(node, "action", where); }
			if (verb == "move" || verb == "hop" || verb == "accelerate" || verb == "turn")
			{
				requireAttribute(node, "direction", where);
				command.amount = readValue(node, where);
			}
			if (verb == "thrust" || verb == "deflect" || verb == "leap") { command.amount = readValue(node, where); }
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
		std::vector<RawTimer> readTimers(const XmlNode* timersNode, const std::string& where)
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

		// <collisions>: whether they are on, whether the objects move in lockstep
		// with the others of their grid or group, then the rules.
		RawCollisionData readCollisions(const XmlNode& collisions, const std::string& where)
		{
			const std::string collisionsHere = where + " > <collisions>";
			RawCollisionData collisionData;

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
					if (all || vertical || edge == "top") { appendCommands(collisionData.top, commands); }
					if (all || vertical || edge == "bottom") { appendCommands(collisionData.bottom, commands); }
					if (all || horizontal || edge == "left") { appendCommands(collisionData.left, commands); }
					if (all || horizontal || edge == "right") { appendCommands(collisionData.right, commands); }
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
					rule.commands = std::move(commands);
					collisionData.basic.push_back(std::move(rule));
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

		RawObject readObject(const XmlNode& object)
		{
			RawObject rawObject;
			rawObject.name = requireAttribute(object, "name", "<object>");
			rawObject.objClass = getAttribute(&object, "class");

			const std::string where = "object '" + rawObject.name + "'";

			std::optional<AnimationSpec> animation;
			if (auto node = findChild(&object, "animation")) { animation = readAnimation(*node, where); }
			chooseSprite(readSprites(object, where), animation, where, rawObject);
			rawObject.rawPosition = readVector2(*requireChild(object, "position", where), where);
			rawObject.rawVelocity = readVector2(*requireChild(object, "velocity", where), where);
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
			if (auto facing = findChild(&object, "facing")) { rawObject.facing = readFacing(*facing, where); }
			rawObject.timers = readTimers(findChild(&object, "timers").get(), where);
			if (auto hidden = findChild(&object, "hidden"))
			{
				if (readBool(*hidden, where)) { rawObject.isVisible = false; }
			}
			rawObject.rawCollisionData = readCollisions(*requireChild(object, "collisions", where), where);
			readActions(object, where, rawObject.action);
			readObjectVariables(object, where, rawObject.variable);

			return rawObject;
		}

		// Some of an <x>/<y> pair. A <group> gives what its members share and a
		// <member> only what differs, so either half may be left out of either.
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

		// What a member says for one thing, else what its group says, else an error.
		RawValue pickValue(const std::optional<RawValue>& own, const std::optional<RawValue>& shared,
			const std::string& where, const std::string& what)
		{
			if (own) { return *own; }
			if (shared) { return *shared; }
			fail(where, "has no " + what + ", and neither does its group");
		}

		// A <group> is read as one RawObject per <member>, in the order written.
		// The group gives what its members share (sprite, position, velocity,
		// collisions, actions, variables, all optional) and each member gives what
		// is its own, taking the rest from the group: a member's <sprite>,
		// <position> (or just its <x> or <y>) and <velocity> win over the group's.
		// What a member has after that must be complete, as an <object> is. A member
		// is called name="..." if it says so, otherwise the group's name, a dot and
		// its number counting from 1 (logrow3.2); the group's name still means
		// every member at once (see Object::groupName).
		void readGroup(const XmlNode& group, std::vector<RawObject>& out)
		{
			const std::string name = requireAttribute(group, "name", "<group>");
			const std::string where = "group '" + name + "'";

			std::vector<RawSprite> sprites;
			std::optional<AnimationSpec> animation;
			PartialVector2 position;
			PartialVector2 velocity;
			std::optional<RawCollisionData> collisions;
			std::optional<RawVector2> acceleration;
			bool hidden = false;
			std::string facing;

			for (std::unique_ptr<XmlNode> child = group.getFirstChild(); child != nullptr; child = child->getNextSibling())
			{
				const std::string tag = child->getName();

				if (tag == "hidden") { hidden = readBool(*child, where); }
				else if (tag == "sprite") { sprites.push_back(readSprite(*child, where)); }
				else if (tag == "animation") { animation = readAnimation(*child, where); }
				else if (tag == "position") { position = readPartialVector2(*child, where); }
				else if (tag == "velocity") { velocity = readPartialVector2(*child, where); }
				else if (tag == "acceleration") { acceleration = readVector2(*child, where); }
				else if (tag == "collisions") { collisions = readCollisions(*child, where); }
				else if (tag == "facing") { facing = readFacing(*child, where); }
				else if (tag != "actions" && tag != "variables" && tag != "timers" && tag != "member")
				{
					fail(where, "unknown <" + tag + ">; expected <sprite>, <animation>, <position>, <velocity>, <acceleration>, <facing>, <hidden>, <collisions>, <actions>, <variables>, <timers> or <member>");
				}
			}

			RawObject shared;
			shared.isVisible = !hidden;
			shared.facing = facing;
			if (acceleration)
			{
				shared.hasAcceleration = true;
				shared.rawAcceleration = *acceleration;
			}
			shared.timers = readTimers(findChild(&group, "timers").get(), where);
			readActions(group, where, shared.action);
			readObjectVariables(group, where, shared.variable);

			int count = 0;

			for (std::unique_ptr<XmlNode> member = findChild(&group, "member"); member != nullptr; member = member->getNextSibling())
			{
				if (member->getName() != "member") { continue; }

				++count;
				const std::string ownName = member->getAttribute("name");
				const std::string memberName = ownName.empty() ? name + "." + std::to_string(count) : ownName;
				const std::string here = "object '" + memberName + "' (a member of " + where + ")";

				std::vector<RawSprite> ownSprites;
				std::optional<AnimationSpec> ownAnimation;
				PartialVector2 ownPosition;
				PartialVector2 ownVelocity;

				for (std::unique_ptr<XmlNode> child = member->getFirstChild(); child != nullptr; child = child->getNextSibling())
				{
					const std::string tag = child->getName();

					if (tag == "sprite") { ownSprites.push_back(readSprite(*child, here)); }
					else if (tag == "animation") { ownAnimation = readAnimation(*child, here); }
					else if (tag == "position") { ownPosition = readPartialVector2(*child, here); }
					else if (tag == "velocity") { ownVelocity = readPartialVector2(*child, here); }
					else { fail(here, "unknown <" + tag + ">; a member can give <sprite>s, an <animation>, a <position> or a <velocity>"); }
				}

				RawObject rawObject = shared;
				rawObject.name = memberName;
				rawObject.objClass = getAttribute(&group, "class");
				rawObject.groupName = name;

				// A member that gives sprites of its own has those instead of the
				// group's, and its own animation instead of the group's.
				const std::vector<RawSprite>& usedSprites = ownSprites.empty() ? sprites : ownSprites;
				if (usedSprites.empty()) { fail(here, "has no <sprite>, and neither does its group"); }
				chooseSprite(usedSprites, ownAnimation ? ownAnimation : animation, here, rawObject);

				rawObject.rawPosition.x = pickValue(ownPosition.x, position.x, here, "<position><x>");
				rawObject.rawPosition.y = pickValue(ownPosition.y, position.y, here, "<position><y>");
				rawObject.rawVelocity.x = pickValue(ownVelocity.x, velocity.x, here, "<velocity><x>");
				rawObject.rawVelocity.y = pickValue(ownVelocity.y, velocity.y, here, "<velocity><y>");

				if (!collisions) { fail(where, "missing <collisions>"); }
				rawObject.rawCollisionData = *collisions;

				out.push_back(std::move(rawObject));
			}

			if (count == 0) { fail(where, "has no <member>"); }
		}

		// <sound name="..." wave="square">: an optional <volume>, then <note>s and
		// <rest>s in the order they play. The words (wave, pitch) are checked by
		// game_expr, which also works out the lengths.
		RawSound readSound(const XmlNode& sound)
		{
			RawSound raw;
			raw.name = requireAttribute(sound, "name", "<sounds>");
			raw.wave = sound.getAttribute("wave");

			const std::string where = "sound '" + raw.name + "'";

			for (std::unique_ptr<XmlNode> child = sound.getFirstChild(); child != nullptr; child = child->getNextSibling())
			{
				const std::string tag = child->getName();

				if (tag == "volume")
				{
					if (!raw.notes.empty()) { fail(where, "<volume> has to come before the <note>s and <rest>s"); }
					raw.volume = readValue(*child, where);
				}
				else if (tag == "note")
				{
					RawNote note;
					note.wave = child->getAttribute("wave");
					note.pitch = requireAttribute(*child, "pitch", where);
					note.to = child->getAttribute("to");
					note.length = readValue(*child, where);
					raw.notes.push_back(std::move(note));
				}
				else if (tag == "rest")
				{
					RawNote rest;
					rest.rest = true;
					rest.length = readValue(*child, where);
					raw.notes.push_back(std::move(rest));
				}
				else
				{
					fail(where, "unknown <" + tag + ">; expected a <volume>, then <note>s and <rest>s");
				}
			}

			if (raw.notes.empty()) { fail(where, "has no <note>"); }

			return raw;
		}

		using KeyBindings = std::map<std::string, std::vector<RawCommand>>;

		// The <input>s under a node (a state's <inputs>, or a <keys> set): a key,
		// or several keys separated by spaces (button="a left"), and the
		// commands each one runs. A key given twice keeps the later binding.
		void readInputs(const XmlNode& node, const std::string& where, KeyBindings& out)
		{
			for (std::unique_ptr<XmlNode> input = node.getFirstChild(); input != nullptr; input = input->getNextSibling())
			{
				const std::string buttons = requireAttribute(*input, "button", where);
				const std::vector<RawCommand> commands = readCommands(input->getFirstChild(), where + " > <input button=\"" + buttons + "\">");

				std::istringstream words(buttons);
				for (std::string button; words >> button;)
				{
					out[button] = commands;
				}
			}
		}

		RawState readState(const XmlNode& state, const std::map<std::string, KeyBindings>& keySets)
		{
			RawState rawState{};
			rawState.name = requireAttribute(state, "name", "<state>");

			const std::string where = "state '" + rawState.name + "'";

			// <shows>
			std::unique_ptr<XmlNode> shows = requireChild(state, "shows", where);
			for (std::unique_ptr<XmlNode> show = shows->getFirstChild(); show != nullptr; show = show->getNextSibling())
			{
				rawState.show.push_back(requireAttribute(*show, "object", where + " > <shows>"));
			}

			// <inputs>: the named <keys> sets it uses (keys="player menu"), in
			// order, each over the one before, then its own <input>s over those: a
			// state can use a set and still give one of its keys a job of its own.
			std::unique_ptr<XmlNode> inputs = requireChild(state, "inputs", where);
			std::istringstream setNames(inputs->getAttribute("keys"));
			for (std::string setName; setNames >> setName;)
			{
				const auto set = keySets.find(setName);
				if (set == keySets.end()) { fail(where + " > <inputs>", "keys=\"" + setName + "\" names no <keys> set"); }
				for (const auto& [button, commands] : set->second) { rawState.input[button] = commands; }
			}
			readInputs(*inputs, where + " > <inputs>", rawState.input);

			// <conditions> (optional - the schema allows a state with none): a
			// test, then the commands to run when it holds.
			if (std::unique_ptr<XmlNode> conditions = findChild(&state, "conditions"))
			{
				for (std::unique_ptr<XmlNode> condition = conditions->getFirstChild(); condition != nullptr; condition = condition->getNextSibling())
				{
					const std::string conditionHere = where + " > <condition>";

					RawCondition raw;
					raw.filterClass = condition->getAttribute("class");
					raw.filterObject = condition->getAttribute("object");
					raw.variableName = condition->getAttribute("variable");

					std::unique_ptr<XmlNode> test = condition->getFirstChild();
					if (!test) { fail(conditionHere, "needs an <atleast>, <atmost> or <remaining>"); }

					const std::string testName = test->getName();
					if (testName == "atleast") { raw.test = RawCondition::Test::AtLeast; }
					else if (testName == "atmost") { raw.test = RawCondition::Test::AtMost; }
					else if (testName == "remaining") { raw.test = RawCondition::Test::Remaining; }
					else { fail(conditionHere, "starts with <" + testName + ">; expected <atleast>, <atmost> or <remaining>"); }

					if (raw.test != RawCondition::Test::Remaining && raw.variableName.empty())
					{
						fail(conditionHere, "<" + testName + "> reads a variable, so the condition needs variable=\"...\"");
					}

					raw.threshold = readValue(*test, conditionHere);
					raw.commands = readCommands(test->getNextSibling(), conditionHere);
					rawState.conditions.push_back(std::move(raw));
				}
			}

			// <timers> (optional): counted while this is the current state.
			rawState.timers = readTimers(findChild(&state, "timers").get(), where);

			return rawState;
		}
	}

	void game_xml::init(const std::string& filename, XmlBackend backend, WindowDesc& windowDesc,
		std::vector<std::pair<std::string, RawValue>>& rawVariables, std::vector<RawState>& rawStates,
		std::vector<RawObject>& rawObjects, std::vector<RawSound>& rawSounds, std::vector<RawPath>& rawPaths,
		SchemaValidation& validation)
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
		// (xsi:noNamespaceSchemaLocation - see games/*.xml), so this reads
		// it directly off root rather than asking XmlDocument for it.
		// Xerces already ran its own real validation as part of load()
		// above whenever this is non-empty (see xml_xerces.cpp) - load()
		// would have failed already otherwise, so reaching here means it
		// passed. Every other backend can only check well-formedness on its
		// own (see xml_document.h), so this falls back to XsdLiteValidator
		// - this project's own interpreter for the subset of XSD this
		// schema actually uses (see xsd_lite.h) - against that same schema
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
			const std::string schemaPath = (std::filesystem::path(filename).parent_path() / schemaLocation).string();

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
		std::unique_ptr<XmlNode> objectsNode = requireChild(*root, "objects", where);
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

		// load objects
		for (std::unique_ptr<XmlNode> object = objectsNode->getFirstChild(); object != nullptr; object = object->getNextSibling())
		{
			if (object->getName() == "group") { readGroup(*object, rawObjects); }
			else { rawObjects.push_back(readObject(*object)); }
		}

		// load states: the named <keys> sets first, so any state can use any of them
		std::map<std::string, KeyBindings> keySets;
		for (std::unique_ptr<XmlNode> node = states->getFirstChild(); node != nullptr; node = node->getNextSibling())
		{
			if (node->getName() != "keys") { continue; }
			const std::string name = requireAttribute(*node, "name", "<states> > <keys>");
			if (keySets.count(name)) { fail("<states>", "two <keys> sets are called '" + name + "'"); }
			readInputs(*node, "<keys name=\"" + name + "\">", keySets[name]);
		}

		for (std::unique_ptr<XmlNode> state = states->getFirstChild(); state != nullptr; state = state->getNextSibling())
		{
			if (state->getName() == "keys") { continue; }
			rawStates.push_back(readState(*state, keySets));
		}
	}
}
