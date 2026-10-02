// game_xml.cpp
// XML Game Engine
// author: beefviper
// date: Sept 21, 2020

#include "game_xml.h"

#include "xsd_lite.h"

#include <algorithm>
#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <map>
#include <optional>
#include <stdexcept>

namespace xge
{
	namespace
	{
		// What game_xml knows how to read is fixed by assets/xmlgameengine.xsd,
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

			if (tagName == "random")
			{
				RawValue value;
				value.kind = RawValue::Kind::Random;
				value.min = requireAttribute(*tag, "min", here);
				value.max = requireAttribute(*tag, "max", here);
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
			return name == "circle" || name == "rectangle" || name == "text" || name == "image" || name == "bitmap";
		}

		// One <circle>, <rectangle>, <text>, <image> or <bitmap>, written into `sprite`.
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

			if (kind != "image")
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
				if (!shape) { fail(gridHere, "needs a shape to repeat (<circle>, <rectangle>, <text>, <image> or <bitmap>)"); }

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
				if (sprites.size() > 1)
				{
					fail(where, "has " + std::to_string(sprites.size()) + " <sprite>s but no <animation> to show them one after the other");
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
			return name == "bounce" || name == "stick" || name == "wrap" || name == "carry" || name == "die"
				|| name == "reset" || name == "inc" || name == "dec" || name == "move" || name == "hop"
				|| name == "accelerate" || name == "turn" || name == "thrust" || name == "release" || name == "stop"
				|| name == "push" || name == "pop" || name == "fire" || name == "trigger";
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
			if (verb == "trigger") { requireAttribute(node, "object", where); requireAttribute(node, "action", where); }
			if (verb == "move" || verb == "hop" || verb == "accelerate" || verb == "turn")
			{
				requireAttribute(node, "direction", where);
				command.amount = readValue(node, where);
			}
			if (verb == "thrust") { command.amount = readValue(node, where); }
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
			bool hidden = false;

			for (std::unique_ptr<XmlNode> child = group.getFirstChild(); child != nullptr; child = child->getNextSibling())
			{
				const std::string tag = child->getName();

				if (tag == "hidden") { hidden = readBool(*child, where); }
				else if (tag == "sprite") { sprites.push_back(readSprite(*child, where)); }
				else if (tag == "animation") { animation = readAnimation(*child, where); }
				else if (tag == "position") { position = readPartialVector2(*child, where); }
				else if (tag == "velocity") { velocity = readPartialVector2(*child, where); }
				else if (tag == "collisions") { collisions = readCollisions(*child, where); }
				else if (tag != "actions" && tag != "variables" && tag != "member")
				{
					fail(where, "unknown <" + tag + ">; expected <sprite>, <animation>, <position>, <velocity>, <hidden>, <collisions>, <actions>, <variables> or <member>");
				}
			}

			RawObject shared;
			shared.isVisible = !hidden;
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

		RawState readState(const XmlNode& state)
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

			// <inputs>: a key and the commands it runs
			std::unique_ptr<XmlNode> inputs = requireChild(state, "inputs", where);
			for (std::unique_ptr<XmlNode> input = inputs->getFirstChild(); input != nullptr; input = input->getNextSibling())
			{
				const std::string button = requireAttribute(*input, "button", where + " > <inputs>");
				rawState.input[button] = readCommands(input->getFirstChild(), where + " > <input button=\"" + button + "\">");
			}

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

			return rawState;
		}
	}

	void game_xml::init(const std::string& filename, XmlBackend backend, WindowDesc& windowDesc,
		std::vector<std::pair<std::string, RawValue>>& rawVariables, std::vector<RawState>& rawStates,
		std::vector<RawObject>& rawObjects, SchemaValidation& validation)
	{
		std::unique_ptr<XmlDocument> document = XmlDocumentFactory::create(backend);

		if (!document->load(filename))
		{
			// Thrown, not exit(): a front end that loads games one after another
			// (XGEGUI) tells the user and carries on.
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

		// load objects
		for (std::unique_ptr<XmlNode> object = objectsNode->getFirstChild(); object != nullptr; object = object->getNextSibling())
		{
			if (object->getName() == "group") { readGroup(*object, rawObjects); }
			else { rawObjects.push_back(readObject(*object)); }
		}

		// load states
		for (std::unique_ptr<XmlNode> state = states->getFirstChild(); state != nullptr; state = state->getNextSibling())
		{
			rawStates.push_back(readState(*state));
		}
	}
}
