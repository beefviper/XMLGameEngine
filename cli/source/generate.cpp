// generate.cpp
// XML Game Engine
// author: beefviper
// date: Oct 5, 2026

#include "generate.h"

#include "game.h"
#include "game_xml.h"

#include <algorithm>
#include <cctype>
#include <charconv>
#include <cstddef>
#include <ctime>
#include <fstream>
#include <map>
#include <set>
#include <sstream>
#include <system_error>
#include <utility>

namespace xge
{
	namespace
	{
		using Lines = std::vector<std::string>;

		std::string tabs(int count)
		{
			return std::string(static_cast<std::size_t>(std::max(count, 0)), '\t');
		}

		// A string as a C++ literal. Anything not plain ASCII is written as
		// octal escapes of its bytes, three digits each so a digit after one is
		// not taken into it, and the file reads the same in any code page.
		std::string literal(const std::string& text)
		{
			std::string quoted = "\"";
			for (const char each : text)
			{
				const auto byte = static_cast<unsigned char>(each);
				switch (each)
				{
				case '"': quoted += "\\\""; break;
				case '\\': quoted += "\\\\"; break;
				case '\n': quoted += "\\n"; break;
				case '\t': quoted += "\\t"; break;
				case '\r': quoted += "\\r"; break;
				default:
					if (byte < 0x20 || byte >= 0x7F)
					{
						const char digits[] = { '\\', static_cast<char>('0' + ((byte >> 6) & 7)),
							static_cast<char>('0' + ((byte >> 3) & 7)), static_cast<char>('0' + (byte & 7)) };
						quoted.append(digits, sizeof digits);
					}
					else
					{
						quoted += each;
					}
				}
			}
			return quoted + "\"";
		}

		std::string number(float value)
		{
			char buffer[64];
			const auto written = std::to_chars(buffer, buffer + sizeof buffer, value);
			std::string text(buffer, written.ptr);
			if (text.find_first_of(".e") == std::string::npos)
			{
				text += ".0";
			}
			return text + "f";
		}

		std::string number(long long value)
		{
			return std::to_string(value);
		}

		std::string boolean(bool value)
		{
			return value ? "true" : "false";
		}

		// What a name in the game becomes in a C++ name: words split at
		// anything that is not a letter or digit, run together in camel case
		// (pong_min is pongMin), and never starting with a digit.
		std::string camel(const std::string& name)
		{
			std::string text;
			bool upper = false;
			for (const char each : name)
			{
				if (std::isalnum(static_cast<unsigned char>(each)) == 0)
				{
					upper = !text.empty();
					continue;
				}
				text += upper ? static_cast<char>(std::toupper(static_cast<unsigned char>(each))) : each;
				upper = false;
			}
			if (text.empty() || std::isdigit(static_cast<unsigned char>(text.front())) != 0)
			{
				text = "n" + text;
			}
			return text;
		}

		bool oneLine(const std::string& text)
		{
			return text.find('\n') == std::string::npos;
		}

		// Writes a game's data as C++, keeping track of which of the small
		// helpers at the top of the file (value(), verb(), make<>()...) the
		// data uses, so only those are written.
		class DataWriter
		{
		public:
			std::set<std::string> used;

			// A braced list of items already written for `indent` + 1: on one
			// line when it is short, else one item to a line.
			static std::string list(const Lines& items, int indent, bool eachOnALine = false)
			{
				if (items.empty())
				{
					return "{}";
				}
				std::size_t length = 0;
				bool simple = !eachOnALine;
				for (const std::string& item : items)
				{
					length += item.size() + 2;
					simple = simple && oneLine(item);
				}
				if (simple && length <= 90)
				{
					std::string text = "{ ";
					for (std::size_t i = 0; i < items.size(); ++i)
					{
						text += (i == 0 ? "" : ", ") + items[i];
					}
					return text + " }";
				}
				std::string text = "{\n";
				for (const std::string& item : items)
				{
					text += tabs(indent + 1) + item + ",\n";
				}
				return text + tabs(indent) + "}";
			}

			// `head` then a lambda that fills in a `type` called `name` with
			// `statements` (written for `indent` + 1), then `)`: make<RawLine>(...)
			// or verb("move", ...).
			static std::string filled(const std::string& head, const std::string& type, const std::string& name, const Lines& statements, int indent)
			{
				const std::string opening = head + "[](" + type + "& " + name + ")";
				std::size_t length = opening.size();
				bool simple = true;
				for (const std::string& statement : statements)
				{
					length += statement.size() + 1;
					simple = simple && oneLine(statement);
				}
				if (simple && length <= 100)
				{
					std::string text = opening + " {";
					for (const std::string& statement : statements)
					{
						text += " " + statement;
					}
					return text + " })";
				}
				std::string text = opening + "\n" + tabs(indent) + "{\n";
				for (const std::string& statement : statements)
				{
					text += tabs(indent + 1) + statement + "\n";
				}
				return text + tabs(indent) + "})";
			}

			std::string made(const std::string& type, const std::string& name, const Lines& statements, int indent)
			{
				if (statements.empty())
				{
					return type + "{}";
				}
				used.insert("make");
				return filled("make<" + type + ">(", type, name, statements, indent);
			}

			static void assign(Lines& out, const std::string& target, const std::string& written, const std::string& otherwise)
			{
				if (written != otherwise)
				{
					out.push_back(target + " = " + written + ";");
				}
			}

			static Lines strings(const std::vector<std::string>& texts)
			{
				Lines items;
				for (const std::string& text : texts)
				{
					items.push_back(literal(text));
				}
				return items;
			}

			std::string value(const RawValue& raw, int indent)
			{
				switch (raw.kind)
				{
				case RawValue::Kind::Expression:
					used.insert("value");
					return "value(" + literal(raw.text) + ")";
				case RawValue::Kind::Random:
					used.insert("random");
					return "random(" + literal(raw.min) + ", " + literal(raw.max) + ")";
				case RawValue::Kind::Equation:
				case RawValue::Kind::Formula:
					break;
				}

				used.insert("arithmetic");
				Lines operations;
				if (raw.operations)
				{
					for (const RawOperation& each : *raw.operations)
					{
						operations.push_back(operation(each, indent + 1));
					}
				}
				const char* kind = raw.kind == RawValue::Kind::Equation ? "RawValue::Kind::Equation" : "RawValue::Kind::Formula";
				return std::string("arithmetic(") + kind + ", " + list(operations, indent) + ")";
			}

			std::string operation(const RawOperation& raw, int indent)
			{
				used.insert("operation");
				Lines operands;
				for (const RawOperand& each : raw.operands)
				{
					operands.push_back("{ " + literal(each.role) + ", " + value(each.value, indent + 1) + " }");
				}
				return "operation(" + literal(raw.op) + ", " + literal(raw.name) + ", " + list(operands, indent) + ")";
			}

			std::string value(const std::optional<RawValue>& raw, int indent)
			{
				return raw ? value(*raw, indent) : "std::nullopt";
			}

			std::string vector2(const RawVector2& raw, int indent)
			{
				return "{ " + value(raw.x, indent) + ", " + value(raw.y, indent) + " }";
			}

			std::string command(const RawCommand& raw, int indent)
			{
				const RawCommand none;
				Lines out;
				const std::string at = "command";
				const int inside = indent + 1;
				assign(out, at + ".object", literal(raw.object), literal(none.object));
				assign(out, at + ".variable", literal(raw.variable), literal(none.variable));
				assign(out, at + ".state", literal(raw.state), literal(none.state));
				assign(out, at + ".action", literal(raw.action), literal(none.action));
				assign(out, at + ".direction", literal(raw.direction), literal(none.direction));
				assign(out, at + ".burn", literal(raw.burn), literal(none.burn));
				assign(out, at + ".sound", literal(raw.sound), literal(none.sound));
				assign(out, at + ".sprite", literal(raw.sprite), literal(none.sprite));
				assign(out, at + ".path", literal(raw.path), literal(none.path));
				assign(out, at + ".objClass", literal(raw.objClass), literal(none.objClass));
				assign(out, at + ".amount", value(raw.amount, inside), value(none.amount, inside));
				assign(out, at + ".seconds", value(raw.seconds, inside), value(none.seconds, inside));
				assign(out, at + ".distance", value(raw.distance, inside), value(none.distance, inside));

				if (out.empty())
				{
					used.insert("verb");
					return "verb(" + literal(raw.verb) + ")";
				}
				used.insert("verbWith");
				return filled("verb(" + literal(raw.verb) + ", ", "RawCommand", "command", out, indent);
			}

			std::string commands(const std::vector<RawCommand>& raw, int indent)
			{
				Lines items;
				for (const RawCommand& each : raw)
				{
					items.push_back(command(each, indent + 1));
				}
				return list(items, indent);
			}

			// A map of names to commands or values (an object's <action>s and
			// <variable>s, a state's <input>s), as an assignment for each name.
			template <typename Map, typename Write>
			void map(Lines& out, const std::string& target, const Map& raw, const Map& reference, int indent, Write write)
			{
				Lines mine;
				Lines theirs;
				for (const auto& [name, each] : raw)
				{
					mine.push_back(target + "[" + literal(name) + "] = " + write(each, indent) + ";");
				}
				for (const auto& [name, each] : reference)
				{
					theirs.push_back(target + "[" + literal(name) + "] = " + write(each, indent) + ";");
				}
				if (mine == theirs)
				{
					return;
				}
				if (!reference.empty())
				{
					out.push_back(target + ".clear();");
				}
				out.insert(out.end(), mine.begin(), mine.end());
			}

			std::string guard(const EdgeGuard& raw)
			{
				return "EdgeGuard{ " + number(static_cast<long long>(raw.first)) + ", " + number(static_cast<long long>(raw.count)) + ", "
					+ literal(raw.sprite) + ", " + literal(raw.unlessClass) + " }";
			}

			std::string guards(const std::vector<EdgeGuard>& raw, int indent)
			{
				Lines items;
				for (const EdgeGuard& each : raw)
				{
					items.push_back(guard(each));
				}
				return list(items, indent);
			}

			std::string rule(const RawCollisionRule& raw, int indent)
			{
				const RawCollisionRule none;
				Lines out;
				const int inside = indent + 1;
				assign(out, "rule.filterClass", literal(raw.filterClass), literal(none.filterClass));
				assign(out, "rule.filterObject", literal(raw.filterObject), literal(none.filterObject));
				assign(out, "rule.unlessClass", literal(raw.unlessClass), literal(none.unlessClass));
				assign(out, "rule.slower", value(raw.slower, inside), value(none.slower, inside));
				assign(out, "rule.faster", value(raw.faster, inside), value(none.faster, inside));
				assign(out, "rule.whileSprite", literal(raw.whileSprite), literal(none.whileSprite));
				assign(out, "rule.commands", commands(raw.commands, inside), commands(none.commands, inside));
				return made("RawCollisionRule", "rule", out, indent);
			}

			void collisions(Lines& out, const RawCollisionData& raw, const RawCollisionData& reference, const std::string& at, int indent)
			{
				assign(out, at + ".enabled", boolean(raw.enabled), boolean(reference.enabled));
				assign(out, at + ".lockstep", boolean(raw.lockstep), boolean(reference.lockstep));
				const auto type = [](CollisionType kind) { return std::string(kind == CollisionType::Pixel ? "CollisionType::Pixel" : "CollisionType::Box"); };
				assign(out, at + ".type", type(raw.type), type(reference.type));
				assign(out, at + ".top", commands(raw.top, indent), commands(reference.top, indent));
				assign(out, at + ".bottom", commands(raw.bottom, indent), commands(reference.bottom, indent));
				assign(out, at + ".left", commands(raw.left, indent), commands(reference.left, indent));
				assign(out, at + ".right", commands(raw.right, indent), commands(reference.right, indent));
				assign(out, at + ".topGuards", guards(raw.topGuards, indent), guards(reference.topGuards, indent));
				assign(out, at + ".bottomGuards", guards(raw.bottomGuards, indent), guards(reference.bottomGuards, indent));
				assign(out, at + ".leftGuards", guards(raw.leftGuards, indent), guards(reference.leftGuards, indent));
				assign(out, at + ".rightGuards", guards(raw.rightGuards, indent), guards(reference.rightGuards, indent));

				const auto rules = [&](const std::vector<RawCollisionRule>& each)
				{
					Lines items;
					for (const RawCollisionRule& one : each)
					{
						items.push_back(rule(one, indent + 1));
					}
					return list(items, indent);
				};
				assign(out, at + ".basic", rules(raw.basic), rules(reference.basic));
			}

			std::string line(const RawLine& raw, int indent)
			{
				const RawLine none;
				Lines out;
				const int inside = indent + 1;
				assign(out, "line.from", vector2(raw.from, inside), vector2(none.from, inside));
				assign(out, "line.to", vector2(raw.to, inside), vector2(none.to, inside));
				assign(out, "line.color", literal(raw.color), literal(none.color));
				assign(out, "line.thickness", value(raw.thickness, inside), value(none.thickness, inside));
				assign(out, "line.hasThickness", boolean(raw.hasThickness), boolean(none.hasThickness));
				return made("RawLine", "line", out, indent);
			}

			void sprite(Lines& out, const RawSprite& raw, const RawSprite& reference, const std::string& at, int indent)
			{
				assign(out, at + ".kind", literal(raw.kind), literal(reference.kind));
				assign(out, at + ".name", literal(raw.name), literal(reference.name));

				const auto lines = [&](const std::vector<RawLine>& each)
				{
					Lines items;
					for (const RawLine& one : each)
					{
						items.push_back(line(one, indent + 1));
					}
					return list(items, indent);
				};
				assign(out, at + ".lines", lines(raw.lines), lines(reference.lines));

				// A picture's rows one under another, so they look like it.
				assign(out, at + ".bitmapRows", list(strings(raw.bitmapRows), indent, true), list(strings(reference.bitmapRows), indent, true));
				assign(out, at + ".scale", value(raw.scale, indent), value(reference.scale, indent));
				assign(out, at + ".hasScale", boolean(raw.hasScale), boolean(reference.hasScale));
				assign(out, at + ".svgX", value(raw.svgX, indent), value(reference.svgX, indent));
				assign(out, at + ".svgY", value(raw.svgY, indent), value(reference.svgY, indent));
				assign(out, at + ".svgWidth", value(raw.svgWidth, indent), value(reference.svgWidth, indent));
				assign(out, at + ".svgHeight", value(raw.svgHeight, indent), value(reference.svgHeight, indent));
				assign(out, at + ".hasSvgRegion", boolean(raw.hasSvgRegion), boolean(reference.hasSvgRegion));
				assign(out, at + ".svgHide", list(strings(raw.svgHide), indent), list(strings(reference.svgHide), indent));
				assign(out, at + ".radius", value(raw.radius, indent), value(reference.radius, indent));
				assign(out, at + ".width", value(raw.width, indent), value(reference.width, indent));
				assign(out, at + ".height", value(raw.height, indent), value(reference.height, indent));
				assign(out, at + ".size", value(raw.size, indent), value(reference.size, indent));
				assign(out, at + ".content", literal(raw.content), literal(reference.content));
				assign(out, at + ".textIsNumber", boolean(raw.textIsNumber), boolean(reference.textIsNumber));
				assign(out, at + ".number", value(raw.number, indent), value(reference.number, indent));
				assign(out, at + ".path", literal(raw.path), literal(reference.path));
				assign(out, at + ".flip", literal(raw.flip), literal(reference.flip));
				assign(out, at + ".color", literal(raw.color), literal(reference.color));
			}

			std::string sprites(const std::vector<RawSprite>& raw, int indent)
			{
				Lines items;
				for (const RawSprite& each : raw)
				{
					Lines out;
					sprite(out, each, RawSprite{}, "sprite", indent + 2);
					items.push_back(made("RawSprite", "sprite", out, indent + 1));
				}
				return list(items, indent);
			}

			std::string timer(const RawTimer& raw, int indent)
			{
				const RawTimer none;
				Lines out;
				const int inside = indent + 1;
				assign(out, "timer.repeat", boolean(raw.repeat), boolean(none.repeat));
				assign(out, "timer.interval", value(raw.interval, inside), value(none.interval, inside));
				assign(out, "timer.commands", commands(raw.commands, inside), commands(none.commands, inside));
				return made("RawTimer", "timer", out, indent);
			}

			std::string timers(const std::vector<RawTimer>& raw, int indent)
			{
				Lines items;
				for (const RawTimer& each : raw)
				{
					items.push_back(timer(each, indent + 1));
				}
				return list(items, indent);
			}

			void object(Lines& out, const RawObject& raw, const RawObject& reference, const std::string& at, int indent)
			{
				assign(out, at + ".name", literal(raw.name), literal(reference.name));
				assign(out, at + ".objClass", literal(raw.objClass), literal(reference.objClass));
				assign(out, at + ".groupName", literal(raw.groupName), literal(reference.groupName));
				assign(out, at + ".cell.column", number(static_cast<long long>(raw.cell.column)), number(static_cast<long long>(reference.cell.column)));
				assign(out, at + ".cell.row", number(static_cast<long long>(raw.cell.row)), number(static_cast<long long>(reference.cell.row)));
				sprite(out, raw.cell.slot, reference.cell.slot, at + ".cell.slot", indent);
				assign(out, at + ".cell.gapBefore", value(raw.cell.gapBefore, indent), value(reference.cell.gapBefore, indent));
				assign(out, at + ".cell.gapAbove", value(raw.cell.gapAbove, indent), value(reference.cell.gapAbove, indent));
				sprite(out, raw.sprite, reference.sprite, at + ".sprite", indent);
				assign(out, at + ".hasAnimation", boolean(raw.hasAnimation), boolean(reference.hasAnimation));
				assign(out, at + ".animation.interval", value(raw.animation.interval, indent), value(reference.animation.interval, indent));
				assign(out, at + ".animation.frames", sprites(raw.animation.frames, indent), sprites(reference.animation.frames, indent));
				assign(out, at + ".looks", sprites(raw.looks, indent), sprites(reference.looks, indent));
				assign(out, at + ".isVisible", boolean(raw.isVisible), boolean(reference.isVisible));
				assign(out, at + ".rawPosition", vector2(raw.rawPosition, indent), vector2(reference.rawPosition, indent));
				assign(out, at + ".rawVelocity", vector2(raw.rawVelocity, indent), vector2(reference.rawVelocity, indent));
				assign(out, at + ".rawAcceleration", vector2(raw.rawAcceleration, indent), vector2(reference.rawAcceleration, indent));
				assign(out, at + ".hasAcceleration", boolean(raw.hasAcceleration), boolean(reference.hasAcceleration));
				assign(out, at + ".rawHeading", value(raw.rawHeading, indent), value(reference.rawHeading, indent));
				assign(out, at + ".hasHeading", boolean(raw.hasHeading), boolean(reference.hasHeading));
				assign(out, at + ".rawDrag", value(raw.rawDrag, indent), value(reference.rawDrag, indent));
				assign(out, at + ".hasDrag", boolean(raw.hasDrag), boolean(reference.hasDrag));
				assign(out, at + ".facing", literal(raw.facing), literal(reference.facing));
				map(out, at + ".variable", raw.variable, reference.variable, indent,
					[&](const RawValue& each, int level) { return value(each, level); });
				assign(out, at + ".timers", timers(raw.timers, indent), timers(reference.timers, indent));
				collisions(out, raw.rawCollisionData, reference.rawCollisionData, at + ".rawCollisionData", indent);
				map(out, at + ".action", raw.action, reference.action, indent,
					[&](const std::vector<RawCommand>& each, int level) { return commands(each, level); });
			}

			std::string condition(const RawCondition& raw, int indent)
			{
				const RawCondition none;
				Lines out;
				const int inside = indent + 1;
				const auto test = [](RawCondition::Test kind)
				{
					switch (kind)
					{
					case RawCondition::Test::AtMost: return std::string("RawCondition::Test::AtMost");
					case RawCondition::Test::Remaining: return std::string("RawCondition::Test::Remaining");
					case RawCondition::Test::AtLeast: break;
					}
					return std::string("RawCondition::Test::AtLeast");
				};
				assign(out, "condition.filterClass", literal(raw.filterClass), literal(none.filterClass));
				assign(out, "condition.filterObject", literal(raw.filterObject), literal(none.filterObject));
				assign(out, "condition.variableName", literal(raw.variableName), literal(none.variableName));
				assign(out, "condition.test", test(raw.test), test(none.test));
				assign(out, "condition.threshold", value(raw.threshold, inside), value(none.threshold, inside));
				assign(out, "condition.commands", commands(raw.commands, inside), commands(none.commands, inside));
				return made("RawCondition", "condition", out, indent);
			}

			void state(Lines& out, const RawState& raw, int indent)
			{
				const RawState none;
				assign(out, "state.name", literal(raw.name), literal(none.name));
				assign(out, "state.show", list(strings(raw.show), indent), list(strings(none.show), indent));
				map(out, "state.input", raw.input, none.input, indent,
					[&](const std::vector<RawCommand>& each, int level) { return commands(each, level); });
				Lines items;
				for (const RawCondition& each : raw.conditions)
				{
					items.push_back(condition(each, indent + 1));
				}
				assign(out, "state.conditions", list(items, indent), "{}");
				assign(out, "state.timers", timers(raw.timers, indent), timers(none.timers, indent));
			}

			std::string note(const RawNote& raw, int indent)
			{
				const RawNote none;
				Lines out;
				const int inside = indent + 1;
				assign(out, "note.rest", boolean(raw.rest), boolean(none.rest));
				assign(out, "note.wave", literal(raw.wave), literal(none.wave));
				assign(out, "note.pitch", literal(raw.pitch), literal(none.pitch));
				assign(out, "note.to", literal(raw.to), literal(none.to));
				assign(out, "note.length", value(raw.length, inside), value(none.length, inside));
				return made("RawNote", "note", out, indent);
			}

			std::string sound(const RawSound& raw, int indent)
			{
				const RawSound none;
				Lines out;
				const int inside = indent + 1;
				assign(out, "sound.name", literal(raw.name), literal(none.name));
				assign(out, "sound.wave", literal(raw.wave), literal(none.wave));
				assign(out, "sound.volume", value(raw.volume, inside), value(none.volume, inside));
				Lines items;
				for (const RawNote& each : raw.notes)
				{
					items.push_back(note(each, inside + 1));
				}
				assign(out, "sound.notes", list(items, inside), "{}");
				return made("RawSound", "sound", out, indent);
			}

			std::string step(const RawPathStep& raw, int indent)
			{
				const RawPathStep none;
				Lines out;
				const int inside = indent + 1;
				assign(out, "step.home", boolean(raw.home), boolean(none.home));
				assign(out, "step.x", value(raw.x, inside), value(none.x, inside));
				assign(out, "step.y", value(raw.y, inside), value(none.y, inside));
				assign(out, "step.commands", commands(raw.commands, inside), commands(none.commands, inside));
				return made("RawPathStep", "step", out, indent);
			}

			std::string path(const RawPath& raw, int indent)
			{
				const RawPath none;
				Lines out;
				const int inside = indent + 1;
				assign(out, "path.name", literal(raw.name), literal(none.name));
				assign(out, "path.speed", value(raw.speed, inside), value(none.speed, inside));
				assign(out, "path.hasStart", boolean(raw.hasStart), boolean(none.hasStart));
				assign(out, "path.startX", value(raw.startX, inside), value(none.startX, inside));
				assign(out, "path.startY", value(raw.startY, inside), value(none.startY, inside));
				Lines items;
				for (const RawPathStep& each : raw.steps)
				{
					items.push_back(step(each, inside + 1));
				}
				assign(out, "path.steps", list(items, inside), "{}");
				return made("RawPath", "path", out, indent);
			}
		};

		void writeLines(std::ostream& out, const Lines& lines, int indent)
		{
			for (const std::string& line : lines)
			{
				out << tabs(indent) << line << '\n';
			}
		}

		// The helpers a game's data is written with, each only if it is used.
		void writeHelpers(std::ostream& out, const std::set<std::string>& used)
		{
			const auto uses = [&](const char* name) { return used.count(name) != 0; };

			if (uses("value"))
			{
				out << "\t// A number, or an expression worked out when the game loads.\n"
					"\tRawValue value(std::string text)\n"
					"\t{\n"
					"\t\treturn RawValue::expression(std::move(text));\n"
					"\t}\n\n";
			}
			if (uses("random"))
			{
				out << "\t// <random min=\"...\" max=\"...\" />\n"
					"\tRawValue random(std::string min, std::string max)\n"
					"\t{\n"
					"\t\tRawValue drawn;\n"
					"\t\tdrawn.kind = RawValue::Kind::Random;\n"
					"\t\tdrawn.min = std::move(min);\n"
					"\t\tdrawn.max = std::move(max);\n"
					"\t\treturn drawn;\n"
					"\t}\n\n";
			}
			if (uses("arithmetic"))
			{
				out << "\t// An <equation>'s steps, or a <formula>'s one operation.\n"
					"\tRawValue arithmetic(RawValue::Kind kind, std::vector<RawOperation> operations)\n"
					"\t{\n"
					"\t\tRawValue worked;\n"
					"\t\tworked.kind = kind;\n"
					"\t\tworked.operations = std::make_shared<const std::vector<RawOperation>>(std::move(operations));\n"
					"\t\treturn worked;\n"
					"\t}\n\n";
			}
			if (uses("operation"))
			{
				out << "\t// One operation of an <equation> or a <formula>: <subtract>, its name, its operands.\n"
					"\tRawOperation operation(std::string op, std::string name, std::vector<RawOperand> operands)\n"
					"\t{\n"
					"\t\treturn RawOperation{ std::move(op), std::move(name), std::move(operands) };\n"
					"\t}\n\n";
			}
			if (uses("verb"))
			{
				out << "\t// A verb tag on its own: <bounce />.\n"
					"\tRawCommand verb(std::string name)\n"
					"\t{\n"
					"\t\tRawCommand command;\n"
					"\t\tcommand.verb = std::move(name);\n"
					"\t\treturn command;\n"
					"\t}\n\n";
			}
			if (uses("verbWith"))
			{
				out << "\t// A verb tag and what it says: <move direction=\"up\">8</move>.\n"
					"\ttemplate <typename Fill>\n"
					"\tRawCommand verb(std::string name, Fill fill)\n"
					"\t{\n"
					"\t\tRawCommand command;\n"
					"\t\tcommand.verb = std::move(name);\n"
					"\t\tfill(command);\n"
					"\t\treturn command;\n"
					"\t}\n\n";
			}
			if (uses("make"))
			{
				out << "\t// A part of the game, its fields filled in by `fill`.\n"
					"\ttemplate <typename Part, typename Fill>\n"
					"\tPart make(Fill fill)\n"
					"\t{\n"
					"\t\tPart part{};\n"
					"\t\tfill(part);\n"
					"\t\treturn part;\n"
					"\t}\n\n";
			}
		}

		// The day the program is written, as the header block says it.
		std::string today()
		{
			const std::time_t now = std::time(nullptr);
			std::tm local{};
#if defined(_WIN32)
			localtime_s(&local, &now);
#else
			localtime_r(&now, &local);
#endif
			static const char* months[] = { "Jan", "Feb", "Mar", "Apr", "May", "June", "July", "Aug", "Sept", "Oct", "Nov", "Dec" };
			return std::string(months[local.tm_mon]) + " " + std::to_string(local.tm_mday) + ", " + std::to_string(local.tm_year + 1900);
		}

		std::string header(const std::string& file)
		{
			return "// " + file + "\n// XML Game Engine\n// author: beefviper\n// date: " + today() + "\n";
		}
	}

	const std::vector<std::filesystem::path>& engineFiles()
	{
		// Everything Game and Engine are made of, and SFML's window and audio;
		// not game_xml (the game is C++ already), the other libraries, or the
		// factories that choose among them (window.cpp, audio.cpp,
		// engine_backends.cpp).
		static const std::vector<std::filesystem::path> files = {
			"lib/include/types.h",
			"lib/include/geometry.h", "lib/source/geometry.cpp",
			"lib/include/color.h", "lib/source/color.cpp",
			"lib/include/bitmap.h", "lib/source/bitmap.cpp",
			"lib/include/svg.h", "lib/source/svg.cpp",
			"lib/include/spelling.h", "lib/source/spelling.cpp",
			"lib/include/keycode.h", "lib/source/keycode.cpp",
			"lib/include/command.h", "lib/source/command.cpp",
			"lib/include/object.h", "lib/source/object.cpp",
			"lib/include/states.h", "lib/source/states.cpp",
			"lib/include/sound.h", "lib/source/sound.cpp",
			"lib/include/game_data.h",
			"lib/include/game_expr.h", "lib/source/game_expr.cpp",
			"lib/include/collision_detector.h", "lib/source/collision_detector.cpp",
			"lib/include/command_executor.h", "lib/source/command_executor.cpp",
			"lib/include/game.h", "lib/source/game.cpp",
			"lib/include/audio.h",
			"lib/include/audio_sfml.h", "lib/source/audio_sfml.cpp",
			"lib/include/builtin_font.h", "lib/source/builtin_font.cpp",
			"lib/include/window.h",
			"lib/include/window_sfml.h", "lib/source/window_sfml.cpp",
			"lib/include/engine.h", "lib/source/engine.cpp",
		};
		return files;
	}

	std::string gameFunctionName(const std::filesystem::path& gameFile)
	{
		return camel(gameFile.stem().string()) + "Game";
	}

	void writeGameData(std::ostream& out, const GameData& game, const std::string& function, const std::string& from)
	{
		DataWriter writer;
		std::ostringstream body;
		std::set<std::string> names{ function, "value", "random", "arithmetic", "operation", "verb", "make", "objects", "states", "sounds", "paths" };
		const auto nameFor = [&](const std::string& wanted)
		{
			std::string name = wanted;
			for (int n = 2; names.count(name) != 0; ++n)
			{
				name = wanted + std::to_string(n);
			}
			names.insert(name);
			return name;
		};

		// The objects, in the order the file has them (the order touches are
		// met in): a function for each object, and one for each <group>,
		// whose members it makes one after another, each from the one before.
		Lines objectCalls;
		for (std::size_t i = 0; i < game.objects.size();)
		{
			const RawObject& first = game.objects[i];
			std::size_t end = i + 1;
			while (!first.groupName.empty() && end < game.objects.size() && game.objects[end].groupName == first.groupName)
			{
				++end;
			}

			if (first.groupName.empty())
			{
				const std::string name = nameFor(camel(first.name) + "Object");
				Lines lines;
				writer.object(lines, first, RawObject{}, "object", 2);
				body << "\t// <object name=\"" << first.name << "\">\n"
					<< "\tRawObject " << name << "()\n\t{\n\t\tRawObject object;\n";
				writeLines(body, lines, 2);
				body << "\t\treturn object;\n\t}\n\n";
				objectCalls.push_back("all.push_back(" + name + "());");
			}
			else
			{
				const std::string name = nameFor(camel(first.groupName) + "Group");
				body << "\t// <group name=\"" << first.groupName << "\">: its " << (end - i)
					<< " members, read as an object each; each written as what it changes of the one before\n"
					<< "\tstd::vector<RawObject> " << name << "()\n\t{\n\t\tstd::vector<RawObject> members;\n\t\tRawObject member;\n";
				RawObject previous;
				for (std::size_t m = i; m < end; ++m)
				{
					Lines lines;
					writer.object(lines, game.objects[m], previous, "member", 2);
					writeLines(body, lines, 2);
					body << "\t\tmembers.push_back(member);\n";
					previous = game.objects[m];
				}
				body << "\t\treturn members;\n\t}\n\n";
				objectCalls.push_back("for (RawObject& member : " + name + "())\n\t{\n\t\tall.push_back(std::move(member));\n\t}");
			}
			i = end;
		}

		body << "\t// Every object and every member of a group, in the file's order\n"
			<< "\tstd::vector<RawObject> objects()\n\t{\n\t\tstd::vector<RawObject> all;\n";
		for (const std::string& call : objectCalls)
		{
			std::string indented = call;
			for (std::size_t at = indented.find('\n'); at != std::string::npos; at = indented.find('\n', at + 1))
			{
				indented.insert(at + 1, "\t");
			}
			body << "\t\t" << indented << '\n';
		}
		body << "\t\treturn all;\n\t}\n\n";

		Lines stateCalls;
		for (const RawState& each : game.states)
		{
			const std::string name = nameFor(camel(each.name) + "State");
			Lines lines;
			writer.state(lines, each, 2);
			body << "\t// <state name=\"" << each.name << "\">\n"
				<< "\tRawState " << name << "()\n\t{\n\t\tRawState state;\n";
			writeLines(body, lines, 2);
			body << "\t\treturn state;\n\t}\n\n";
			stateCalls.push_back(name + "()");
		}

		Lines sounds;
		for (const RawSound& each : game.sounds)
		{
			sounds.push_back(writer.sound(each, 3));
		}
		Lines paths;
		for (const RawPath& each : game.paths)
		{
			paths.push_back(writer.path(each, 3));
		}
		Lines variables;
		for (const auto& [name, raw] : game.variables)
		{
			variables.push_back("{ " + literal(name) + ", " + writer.value(raw, 3) + " }");
		}

		if (!sounds.empty())
		{
			body << "\t// <sounds>\n\tstd::vector<RawSound> sounds()\n\t{\n\t\treturn " << DataWriter::list(sounds, 2, true) << ";\n\t}\n\n";
		}
		if (!paths.empty())
		{
			body << "\t// <paths>\n\tstd::vector<RawPath> paths()\n\t{\n\t\treturn " << DataWriter::list(paths, 2, true) << ";\n\t}\n\n";
		}

		const std::string name = std::filesystem::path(from).stem().string();
		out << header(name + ".cpp")
			<< "//\n"
			<< "// " << game.window.name << ", as written in " << from << ", read the way the engine reads it\n"
			<< "// and written out as the engine's own data (game_data.h): " << function << "()\n"
			<< "// is the game file, with no XML left to read. Generated by xgecli --generate\n"
			<< "// windows-cpp; change the game file and generate it again rather than this.\n\n"
			<< "#include \"game_data.h\"\n\n";
		if (writer.used.count("arithmetic") != 0)
		{
			out << "#include <memory>\n";
		}
		out << "#include <string>\n#include <utility>\n#include <vector>\n\n"
			<< "namespace\n{\n\tusing namespace xge;\n\n";
		writeHelpers(out, writer.used);
		std::string written = body.str();
		if (written.size() >= 2 && written.compare(written.size() - 2, 2, "\n\n") == 0)
		{
			written.pop_back();
		}
		out << written << "}\n\n";

		out << "xge::GameData " << function << "()\n{\n\txge::GameData game;\n";
		const WindowDesc none;
		Lines window;
		DataWriter::assign(window, "game.window.name", literal(game.window.name), literal(none.name));
		DataWriter::assign(window, "game.window.width", number(game.window.width), number(none.width));
		DataWriter::assign(window, "game.window.height", number(game.window.height), number(none.height));
		DataWriter::assign(window, "game.window.background", literal(game.window.background), literal(none.background));
		DataWriter::assign(window, "game.window.fullscreen", literal(game.window.fullscreen), literal(none.fullscreen));
		DataWriter::assign(window, "game.window.framerate", number(static_cast<long long>(game.window.framerate)), number(static_cast<long long>(none.framerate)));
		writeLines(out, window, 1);
		if (!variables.empty())
		{
			out << "\tgame.variables = " << DataWriter::list(variables, 1, true) << ";\n";
		}
		out << "\tgame.objects = objects();\n";
		out << "\tgame.states = " << DataWriter::list(stateCalls, 1) << ";\n";
		if (!sounds.empty())
		{
			out << "\tgame.sounds = sounds();\n";
		}
		if (!paths.empty())
		{
			out << "\tgame.paths = paths();\n";
		}
		out << "\tgame.validation = " << literal("none (written into the program from " + from + ")") << ";\n"
			<< "\treturn game;\n}\n";
	}

	namespace
	{
		// The game's assets: the font, if it shows any text, and every picture
		// and drawing its sprites name, each once.
		std::vector<std::filesystem::path> assetsOf(const GameData& game)
		{
			std::set<std::string> found;
			bool text = false;
			const auto look = [&](const RawSprite& sprite)
			{
				text = text || sprite.kind == "text";
				if (!sprite.path.empty())
				{
					found.insert(sprite.path);
				}
			};
			for (const RawObject& object : game.objects)
			{
				look(object.sprite);
				look(object.cell.slot);
				for (const RawSprite& each : object.looks) { look(each); }
				for (const RawSprite& each : object.animation.frames) { look(each); }
			}
			if (text)
			{
				found.insert("assets/tuffy.ttf");
			}
			return { found.begin(), found.end() };
		}

		void writeFile(const std::filesystem::path& file, const std::string& text)
		{
			std::error_code ignored;
			std::filesystem::create_directories(file.parent_path(), ignored);
			std::ofstream out(file, std::ios::binary);
			out << text;
			if (!out)
			{
				throw GenerateError("could not write " + file.string());
			}
		}

		void copyFile(const std::filesystem::path& from, const std::filesystem::path& to)
		{
			std::error_code failed;
			std::filesystem::create_directories(to.parent_path(), failed);
			std::filesystem::copy_file(from, to, std::filesystem::copy_options::overwrite_existing, failed);
			if (failed)
			{
				throw GenerateError("could not copy " + from.string() + " to " + to.string() + ": " + failed.message());
			}
		}

		std::string mainSource(const std::string& title, const std::string& name, const std::string& function, const std::string& from)
		{
			return header("main.cpp")
				+ "//\n"
				"// " + title + ", generated from " + from + " by xgecli --generate windows-cpp. The\n"
				"// game is in source/" + name + ".cpp, as the engine's own data; it is played\n"
				"// by the engine's own code (engine/), on SFML, as xgecli plays it.\n\n"
				"#include \"audio_sfml.h\"\n"
				"#include \"engine.h\"\n"
				"#include \"game.h\"\n"
				"#include \"window_sfml.h\"\n\n"
				"#include <cstdlib>\n"
				"#include <exception>\n"
				"#include <iostream>\n"
				"#include <memory>\n\n"
				"// The game, as written in " + from + " (source/" + name + ".cpp).\n"
				"xge::GameData " + function + "();\n\n"
				"int main()\n"
				"{\n"
				"\ttry\n"
				"\t{\n"
				"\t\txge::Game game{ " + function + "() };\n\n"
				"\t\t// No sound device is no reason not to play: silence, as xgecli does.\n"
				"\t\tstd::unique_ptr<xge::Audio> audio;\n"
				"\t\ttry\n"
				"\t\t{\n"
				"\t\t\taudio = std::make_unique<xge::SFMLAudio>();\n"
				"\t\t}\n"
				"\t\tcatch (const std::exception& error)\n"
				"\t\t{\n"
				"\t\t\tstd::cerr << \"warning: no sound: \" << error.what() << '\\n';\n"
				"\t\t}\n\n"
				"\t\txge::Engine engine(game, std::make_unique<xge::SFMLWindow>(game.getWindowDesc()), std::move(audio));\n"
				"\t\tengine.loop();\n"
				"\t}\n"
				"\tcatch (const std::exception& error)\n"
				"\t{\n"
				"\t\tstd::cerr << \"Error: \" << error.what() << '\\n';\n"
				"\t\treturn EXIT_FAILURE;\n"
				"\t}\n\n"
				"\treturn EXIT_SUCCESS;\n"
				"}\n";
		}

		std::string cmakeLists(const std::string& title, const std::string& name, const std::string& from, bool hasAssets)
		{
			std::string sources;
			for (const std::filesystem::path& file : engineFiles())
			{
				sources += "\tengine/" + file.filename().generic_string() + "\n";
			}

			return "# CMakeLists.txt\n"
				"# " + title + ", generated from " + from + " by xgecli --generate windows-cpp.\n\n"
				"cmake_minimum_required(VERSION 3.28)\n\n"
				"project(" + name + " LANGUAGES CXX)\n\n"
				"# The built game, and any DLLs it needs, go to output/ (with a folder for the\n"
				"# configuration, Debug or Release, under Visual Studio).\n"
				"set(CMAKE_RUNTIME_OUTPUT_DIRECTORY \"${CMAKE_CURRENT_SOURCE_DIR}/output\")\n\n"
				"include(FetchContent)\n\n"
				"# What the engine is built with: SFML 3 for the window and sound, exprtk for\n"
				"# the expressions in the game, lunasvg for any <svg> picture. Each as\n"
				"# installed (vcpkg install sfml exprtk lunasvg), or else fetched and built here.\n"
				"find_package(SFML 3 COMPONENTS Graphics Audio QUIET)\n"
				"if (NOT SFML_FOUND)\n"
				"\tmessage(STATUS \"SFML 3 not found, using FetchContent to download and build it.\")\n"
				"\tset(SFML_BUILD_AUDIO ON CACHE BOOL \"\" FORCE)\n"
				"\tset(SFML_BUILD_NETWORK OFF CACHE BOOL \"\" FORCE)\n"
				"\tFetchContent_Declare(SFML GIT_REPOSITORY https://github.com/SFML/SFML.git GIT_TAG 3.1.0 EXCLUDE_FROM_ALL SYSTEM)\n"
				"\tFetchContent_MakeAvailable(SFML)\n"
				"endif()\n\n"
				"find_path(EXPRTK_INCLUDE_DIR exprtk.hpp)\n"
				"if (EXPRTK_INCLUDE_DIR)\n"
				"\tadd_library(exprtk INTERFACE)\n"
				"\ttarget_include_directories(exprtk SYSTEM INTERFACE \"${EXPRTK_INCLUDE_DIR}\")\n"
				"else()\n"
				"\tmessage(STATUS \"exprtk not found, using FetchContent to download it.\")\n"
				"\tFetchContent_Declare(exprtk GIT_REPOSITORY https://github.com/ArashPartow/exprtk.git GIT_TAG 0.0.3-cmake EXCLUDE_FROM_ALL SYSTEM)\n"
				"\tFetchContent_MakeAvailable(exprtk)\n"
				"endif()\n\n"
				"find_package(lunasvg QUIET)\n"
				"if (NOT lunasvg_FOUND)\n"
				"\tmessage(STATUS \"lunasvg not found, using FetchContent to download and build it.\")\n"
				"\tset(LUNASVG_BUILD_EXAMPLES OFF CACHE BOOL \"\" FORCE)\n"
				"\tset(PLUTOVG_BUILD_EXAMPLES OFF CACHE BOOL \"\" FORCE)\n"
				"\tFetchContent_Declare(lunasvg GIT_REPOSITORY https://github.com/sammycage/lunasvg.git GIT_TAG v3.5.0 EXCLUDE_FROM_ALL SYSTEM)\n"
				"\tFetchContent_MakeAvailable(lunasvg)\n"
				"endif()\n\n"
				"add_executable(" + name + "\n"
				"\tsource/main.cpp\n"
				"\tsource/" + name + ".cpp\n"
				"\t# the engine's own code, as it is in lib/\n"
				+ sources +
				")\n"
				"target_include_directories(" + name + " PRIVATE engine)\n"
				"target_compile_features(" + name + " PRIVATE cxx_std_20)\n"
				"target_link_libraries(" + name + " PRIVATE SFML::Graphics SFML::Audio exprtk lunasvg::lunasvg)\n\n"
				"# The warnings the engine is built with (xge_warnings in its\n"
				"# scripts/cmake/platform.cmake), for its code and the game's alike; /bigobj\n"
				"# because exprtk makes more sections than MSVC allows an object file without it.\n"
				"target_compile_options(" + name + " PRIVATE\n"
				"\t$<$<CXX_COMPILER_ID:MSVC>:/W4\n"
				"\t\t/w14242 /w14254 /w14263 /w14265 /w14287 /w14296 /w14311 /w14545 /w14546 /w14547\n"
				"\t\t/w14549 /w14555 /w14619 /w14640 /w14826 /w14905 /w14906 /w14928\n"
				"\t\t/external:anglebrackets /external:W0 /analyze:external- /bigobj>\n"
				"\t$<$<CXX_COMPILER_ID:GNU,Clang,AppleClang>:-Wall -Wextra -Wpedantic -Wshadow -Wconversion\n"
				"\t\t-Wsign-conversion -Wnon-virtual-dtor -Woverloaded-virtual -Wcast-align -Wunused\n"
				"\t\t-Wnull-dereference -Wformat=2 -Wimplicit-fallthrough\n"
				"\t\t-Wold-style-cast -Wmisleading-indentation>\n"
				"\t$<$<CXX_COMPILER_ID:GNU>:-Wdouble-promotion -Wduplicated-cond -Wduplicated-branches\n"
				"\t\t-Wlogical-op -Wuseless-cast>)\n\n"
				+ (hasAssets
					? "# The game opens its font and pictures from assets/ beside where it runs:\n"
					  "# copied beside the program, and the folder Visual Studio starts it in.\n"
					  "add_custom_command(TARGET " + name + " POST_BUILD\n"
					  "\tCOMMAND ${CMAKE_COMMAND} -E copy_directory \"${CMAKE_CURRENT_SOURCE_DIR}/assets\" \"$<TARGET_FILE_DIR:" + name + ">/assets\")\n"
					: std::string()) +
				"set_property(TARGET " + name + " PROPERTY VS_DEBUGGER_WORKING_DIRECTORY \"$<TARGET_FILE_DIR:" + name + ">\")\n\n"
				"# Visual Studio starts the game when you press F5, rather than ALL_BUILD.\n"
				"set_property(DIRECTORY PROPERTY VS_STARTUP_PROJECT " + name + ")\n\n"
				"# On Windows the program needs its libraries' DLLs beside it when they are shared.\n"
				"if (WIN32)\n"
				"\tadd_custom_command(TARGET " + name + " POST_BUILD\n"
				"\t\tCOMMAND ${CMAKE_COMMAND} -E copy_if_different \"$<TARGET_RUNTIME_DLLS:" + name + ">\" \"$<TARGET_FILE_DIR:" + name + ">\"\n"
				"\t\tCOMMAND_EXPAND_LISTS)\n"
				"endif()\n";
		}

		std::string readme(const std::string& title, const std::string& name, const std::string& from)
		{
			return "# " + title + "\n\n"
				"Generated from `" + from + "` by `xgecli --generate windows-cpp`. It is the game xgecli and\n"
				"xgegui play, built as a program of its own on SFML 3: the game file is written out\n"
				"as C++ data in `source/" + name + ".cpp`, and everything that plays it (the frame, the\n"
				"physics and collisions, the keys, the pictures and the sounds) is the engine's own\n"
				"code, copied unchanged into `engine/`. Only reading XML and choosing among window and\n"
				"sound libraries are left out.\n\n"
				"| File | What |\n"
				"|---|---|\n"
				"| `source/" + name + ".cpp` | the game, as the engine reads it from `" + from + "` |\n"
				"| `source/main.cpp` | makes the game, an SFML window and SFML sound, and runs the engine's loop |\n"
				"| `engine/` | the engine's own files, as they are in its `lib/` |\n"
				"| `assets/` | the font and pictures it opens, copied beside the program when it is built |\n"
				"| `CMakeLists.txt` | the build; it uses installed SFML 3, exprtk and lunasvg, or downloads and builds them |\n"
				"| `.gitignore` | leaves out `build/` and `output/` |\n\n"
				"## Build and play\n\n"
				"On Windows with Visual Studio and vcpkg (`vcpkg install sfml exprtk lunasvg`), from this folder:\n\n"
				"```\n"
				"cmake -B build -DCMAKE_TOOLCHAIN_FILE=%VCPKG_ROOT%/scripts/buildsystems/vcpkg.cmake\n"
				"cmake --build build --config Release\n"
				"output\\Release\\" + name + ".exe\n"
				"```\n\n"
				"Without vcpkg, leave out the toolchain file: the libraries are downloaded and built with the game.\n"
				"The built game, its DLLs and its `assets/` are in `output/`; `build/` holds the rest.\n\n"
				"Change the game in its XML file and generate it again, rather than editing `source/" + name + ".cpp`.\n";
		}
	}

	GeneratedProgram generateGame(const GenerateRequest& request)
	{
		if (request.target != "windows-cpp")
		{
			throw GenerateError("no generator for " + request.target + " (there is one: windows-cpp)");
		}

		const std::filesystem::path gameFile = std::filesystem::absolute(request.gameFile);
		const std::string from = gameFile.filename().string();

		// Read as xgecli reads it, and worked out once, so a game the engine
		// would not load is refused here rather than by the program.
		GameData game;
		try
		{
			game = readGame(gameFile.string());
			const Game check{ game };
		}
		catch (const std::exception& error)
		{
			throw GenerateError("could not generate " + from + ": " + error.what());
		}

		const std::filesystem::path output = std::filesystem::absolute(request.output);
		const bool outputWasThere = std::filesystem::exists(output);
		const std::string name = gameFile.stem().string();
		const std::string function = gameFunctionName(gameFile);
		const std::string title = game.window.name.empty() ? name : game.window.name;

		GeneratedProgram program;
		try
		{
			std::filesystem::create_directories(output);

			std::ostringstream data;
			writeGameData(data, game, function, from);
			const std::vector<std::pair<std::filesystem::path, std::string>> written = {
				{ std::filesystem::path("source") / (name + ".cpp"), data.str() },
				{ "source/main.cpp", mainSource(title, name, function, from) },
				{ "CMakeLists.txt", cmakeLists(title, name, from, !assetsOf(game).empty()) },
				{ "README.md", readme(title, name, from) },
				{ ".gitignore", "# where the game is built, and the built game\nbuild/\noutput/\n" },
			};
			for (const auto& [path, text] : written)
			{
				writeFile(output / path, text);
				program.files.push_back(path);
			}

			for (const std::filesystem::path& file : engineFiles())
			{
				const std::filesystem::path to = std::filesystem::path("engine") / file.filename();
				copyFile(request.dataFolder / file, output / to);
				program.files.push_back(to);
			}

			for (const std::filesystem::path& asset : assetsOf(game))
			{
				copyFile(request.dataFolder / asset, output / asset);
				program.assets.push_back(asset);
			}
		}
		catch (...)
		{
			// Half a program is no use: a folder made for it goes again.
			if (!outputWasThere)
			{
				std::error_code ignored;
				std::filesystem::remove_all(output, ignored);
			}
			throw;
		}

		return program;
	}
}
