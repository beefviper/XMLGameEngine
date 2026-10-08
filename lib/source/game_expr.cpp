// game_expr.cpp
// XML Game Engine
// author: beefviper
// date: Sept 21, 2020

#include "game_expr.h"

#include "color.h"
#include "keycode.h"
#include "spelling.h"
#include "svg.h"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstdlib>
#include <memory>
#include <optional>
#include <set>
#include <stdexcept>

namespace xge
{
	namespace
	{
		// The object a command names: its exact name (aliens.3.2), the name
		// from the file (aliens), or the name of its <group> - the same lookup
		// Game::tryGetObject does while the game runs.
		const Object* findObject(const std::vector<Object>& objects, const std::string& name)
		{
			const auto exact = std::find_if(objects.begin(), objects.end(), [&](const Object& object) { return object.name == name; });
			if (exact != objects.end())
			{
				return &*exact;
			}

			const auto made = std::find_if(objects.begin(), objects.end(), [&](const Object& object)
				{
					return object.name == name || (!object.groupName.empty() && object.groupName == name);
				});
			return made == objects.end() ? nullptr : &*made;
		}

		// Every name an object can be called by (its own and its group's),
		// for a message about one the game does not have.
		std::vector<std::string> objectNames(const std::vector<Object>& objects)
		{
			std::set<std::string> names;
			for (const Object& object : objects)
			{
				names.insert(object.name);
				if (!object.groupName.empty()) { names.insert(object.groupName); }
			}
			return { names.begin(), names.end() };
		}

		std::vector<std::string> stateNames(const std::vector<State>& states)
		{
			std::vector<std::string> names;
			for (const State& state : states) { names.push_back(state.name); }
			return names;
		}

		// A color is one of the color. names; anything else would draw nothing.
		void checkColor(const std::string& name, const std::string& where)
		{
			if (!isColorName(name))
			{
				throw std::runtime_error(where + ": there is no color named '" + name + "'" + didYouMean(name, colorNames()) + ". The colors are: " + listOf(colorNames()));
			}
		}

		// The tag a command was written as, for a message about it.
		std::string tagOf(const Command& command)
		{
			return std::visit(overload{
				[](const CmdBounce&) { return std::string("<bounce />"); },
				[](const CmdStick&) { return std::string("<stick />"); },
				[](const CmdReset&) { return std::string("<reset />"); },
				[](const CmdDie&) { return std::string("<die />"); },
				[](const CmdWrap&) { return std::string("<wrap />"); },
				[](const CmdRide&) { return std::string("<ride />"); },
				[](const CmdDeflect&) { return std::string("<deflect>"); },
				[](const CmdReverse&) { return std::string("<reverse />"); },
				[](const CmdMove&) { return std::string("<move>"); },
				[](const CmdHop&) { return std::string("<hop>"); },
				[](const CmdJump&) { return std::string("<jump>"); },
				[](const CmdLand&) { return std::string("<land />"); },
				[](const CmdLeap&) { return std::string("<leap>"); },
				[](const CmdClimb&) { return std::string("<climb>"); },
				[](const CmdChase&) { return std::string("<chase>"); },
				[](const CmdAim&) { return std::string("<aim>"); },
				[](const CmdAccelerate&) { return std::string("<accelerate>"); },
				[](const CmdTurn&) { return std::string("<turn>"); },
				[](const CmdThrust&) { return std::string("<thrust>"); },
				[](const CmdRelease&) { return std::string("<release>"); },
				[](const CmdStop&) { return std::string("<stop />"); },
				[](const CmdIncrement&) { return std::string("<inc>"); },
				[](const CmdDecrement&) { return std::string("<dec>"); },
				[](const CmdPushState&) { return std::string("<push>"); },
				[](const CmdPopState&) { return std::string("<pop>"); },
				[](const CmdFire&) { return std::string("<fire>"); },
				[](const CmdTriggerAction&) { return std::string("<trigger>"); },
				[](const CmdResetObject&) { return std::string("<reset object>"); },
				[](const CmdPlay&) { return std::string("<play>"); },
				[](const CmdBecome&) { return std::string("<become>"); },
				[](const CmdReveal&) { return std::string("<reveal>"); },
				[](const CmdFollow&) { return std::string("<follow>"); },
			}, command);
		}

		// Where a list of commands runs, and what each place does something
		// with. These mirror the dispatchers in command_executor.cpp (a command
		// one of them leaves to its catch-all does nothing there); change one
		// and change the other.
		enum class Place { Edge, Touch, Input, Condition, StateTimer, ObjectTimer, Action, PathStep };

		struct PlaceRule
		{
			const char* what;
			std::vector<std::string> tags;
		};

		const PlaceRule& ruleFor(Place place)
		{
			static const std::vector<std::string> stateCommands{ "<push>", "<pop>", "<reset />", "<reset object>", "<trigger>", "<inc>", "<dec>", "<play>", "<become>", "<reveal>", "<follow>" };
			static const std::vector<std::string> ownerCommands{ "<push>", "<pop>", "<reset />", "<reset object>", "<trigger>", "<inc>", "<dec>", "<play>", "<become>", "<reveal>", "<follow>",
				"<fire>", "<reverse />", "<die />", "<stop />", "<move>", "<release>" };
			static std::vector<std::string> objectTimerCommands = [] {
				std::vector<std::string> tags{ ownerCommands };
				tags.push_back("<chase>");
				tags.push_back("<aim>");
				return tags;
			}();
			static const PlaceRule rules[] = {
				{ "a screen-edge <collision>", { "<bounce />", "<stick />", "<reset />", "<die />", "<stop />", "<wrap />", "<release>", "<move>", "<inc>", "<dec>",
					"<play>", "<reverse />", "<reset object>", "<become>", "<reveal>", "<follow>" } },
				{ "a <collision> with another object", { "<bounce />", "<deflect>", "<die />", "<stop />", "<reset />", "<release>", "<move>", "<inc>", "<dec>",
					"<ride />", "<land />", "<chase>", "<aim>", "<play>", "<reverse />", "<reset object>", "<become>", "<reveal>", "<follow>" } },
				{ "an <input>", stateCommands },
				{ "a <condition>", stateCommands },
				{ "a state's <timer>", stateCommands },
				{ "an object's <timer>", objectTimerCommands },
				{ "an <action>", { "<move>", "<hop>", "<jump>", "<leap>", "<climb>", "<reset />", "<become>", "<reveal>", "<follow>", "<accelerate>", "<turn>", "<thrust>", "<fire>", "<play>" } },
				{ "a path's <step>", ownerCommands },
			};
			return rules[static_cast<std::size_t>(place)];
		}

		// A command the schema lets through where the engine does nothing with
		// it (a <move> in an <input>, a <push> in a <collision>) stops the load,
		// saying what can go there.
		void checkPlace(const std::vector<Command>& commands, Place place, const std::string& where)
		{
			const PlaceRule& rule = ruleFor(place);
			for (const Command& command : commands)
			{
				const std::string tag = tagOf(command);
				if (std::find(rule.tags.begin(), rule.tags.end(), tag) != rule.tags.end())
				{
					continue;
				}

				std::string hint;
				if (place == Place::Input && (tag == "<move>" || tag == "<fire>" || tag == "<hop>" || tag == "<jump>" || tag == "<leap>" || tag == "<climb>" || tag == "<accelerate>"))
				{
					hint = " A key moves or fires an object through one of its <action>s: <trigger object=\"...\" action=\"...\" />.";
				}
				else if (tag == "<reset object>" && place == Place::Action)
				{
					hint = " Here only a bare <reset /> (the object itself) works.";
				}
				throw std::runtime_error(where + ": " + tag + " does nothing in " + rule.what + ", so it would be ignored." + hint
					+ " What can go there: " + listOf(rule.tags));
			}
		}

		// Every state, object and action a command names has to exist. Found
		// here, when the game loads, rather than the first time the command
		// runs: pressing a key bound to <push state="pasued" /> would otherwise
		// stop the game in the middle of play (and used to read past the end of
		// the list of states).
		void checkCommand(const Command& command, const std::vector<State>& states, const std::vector<Object>& objects,
			const std::vector<SoundDesc>& sounds, const std::map<std::string, Path>& paths, const std::string& where)
		{
			if (const auto* follow = std::get_if<CmdFollow>(&command))
			{
				if (!paths.count(follow->path))
				{
					std::vector<std::string> names;
					for (const auto& entry : paths) { names.push_back(entry.first); }
					throw std::runtime_error(where + ": <follow path=\"" + follow->path + "\" /> names no path of the game" + didYouMean(follow->path, names));
				}
				if (!follow->target.empty() && !findObject(objects, follow->target))
				{
					throw std::runtime_error(where + ": <follow> names '" + follow->target + "', and there is no object of that name" + didYouMean(follow->target, objectNames(objects)));
				}
			}
			else if (const auto* become = std::get_if<CmdBecome>(&command))
			{
				if (!become->target.empty())
				{
					bool any = false;
					for (const Object& object : objects)
					{
						const bool named = object.name == become->target
							|| (!object.groupName.empty() && object.groupName == become->target);
						if (!named) { continue; }
						any = true;
						if (std::none_of(object.looks.begin(), object.looks.end(), [&](const Object::Look& look) { return look.name == become->sprite; }))
						{
							throw std::runtime_error(where + ": <become sprite=\"" + become->sprite + "\" object=\"" + become->target + "\" />: '" + object.name + "' has no look of that name");
						}
					}
					if (!any)
					{
						throw std::runtime_error(where + ": <become> names '" + become->target + "', and there is no object of that name" + didYouMean(become->target, objectNames(objects)));
					}
				}
			}
			else if (const auto* chase = std::get_if<CmdChase>(&command))
			{
				if (!findObject(objects, chase->target))
				{
					throw std::runtime_error(where + ": <chase> names '" + chase->target + "', and there is no object of that name" + didYouMean(chase->target, objectNames(objects)));
				}
			}
			else if (const auto* aim = std::get_if<CmdAim>(&command))
			{
				if (!findObject(objects, aim->target))
				{
					throw std::runtime_error(where + ": <aim> names '" + aim->target + "', and there is no object of that name" + didYouMean(aim->target, objectNames(objects)));
				}
			}
			else if (const auto* reveal = std::get_if<CmdReveal>(&command))
			{
				if (!findObject(objects, reveal->target))
				{
					throw std::runtime_error(where + ": <reveal> names '" + reveal->target + "', and there is no object of that name" + didYouMean(reveal->target, objectNames(objects)));
				}
			}
			else if (const auto* play = std::get_if<CmdPlay>(&command))
			{
				const bool known = std::any_of(sounds.begin(), sounds.end(), [&](const SoundDesc& sound) { return sound.name == play->sound; });
				if (!known)
				{
					std::vector<std::string> names;
					for (const SoundDesc& sound : sounds) { names.push_back(sound.name); }
					throw std::runtime_error(where + ": <play sound=\"" + play->sound + "\" /> names no sound of the game" + didYouMean(play->sound, names));
				}
			}
			else if (const auto* push = std::get_if<CmdPushState>(&command))
			{
				const bool known = std::any_of(states.begin(), states.end(), [&](const State& state) { return state.name == push->name; });
				if (!known)
				{
					throw std::runtime_error(where + ": <push state=\"" + push->name + "\" /> names no state of the game" + didYouMean(push->name, stateNames(states)));
				}
			}
			else if (const auto* pop = std::get_if<CmdPopState>(&command); pop && !pop->name.empty())
			{
				const bool known = std::any_of(states.begin(), states.end(), [&](const State& state) { return state.name == pop->name; });
				if (!known)
				{
					throw std::runtime_error(where + ": <pop state=\"" + pop->name + "\" /> names no state of the game" + didYouMean(pop->name, stateNames(states)));
				}
			}
			else if (const auto* trigger = std::get_if<CmdTriggerAction>(&command))
			{
				const Object* object = findObject(objects, trigger->object);
				if (!object)
				{
					throw std::runtime_error(where + ": an action of '" + trigger->object + "' is asked for, and there is no object of that name" + didYouMean(trigger->object, objectNames(objects)));
				}
				if (!object->action.count(trigger->action))
				{
					std::vector<std::string> names;
					for (const auto& entry : object->action) { names.push_back(entry.first); }
					throw std::runtime_error(where + ": '" + trigger->object + "' has no action named '" + trigger->action + "'" + didYouMean(trigger->action, names)
						+ (names.empty() ? std::string(" (it has no <actions>)") : "; its actions are: " + listOf(names)));
				}
			}
			else if (const auto* fire = std::get_if<CmdFire>(&command))
			{
				if (!findObject(objects, fire->projectileName))
				{
					throw std::runtime_error(where + ": <fire> names '" + fire->projectileName + "', and there is no object of that name" + didYouMean(fire->projectileName, objectNames(objects)));
				}
			}
			else if (const auto* release = std::get_if<CmdRelease>(&command))
			{
				if (!findObject(objects, release->target))
				{
					throw std::runtime_error(where + ": <release> names '" + release->target + "', and there is no object of that name" + didYouMean(release->target, objectNames(objects)));
				}
			}
			else if (const auto* reset = std::get_if<CmdResetObject>(&command))
			{
				if (!findObject(objects, reset->target))
				{
					throw std::runtime_error(where + ": <reset object=\"" + reset->target + "\" /> names no object of the game" + didYouMean(reset->target, objectNames(objects)));
				}
			}
			else if (std::holds_alternative<CmdIncrement>(command) || std::holds_alternative<CmdDecrement>(command))
			{
				// A variable belongs to an object: paddle1.score is paddle1's
				// <variable name="score">. A bare name is a text object counting
				// its own <number>.
				const bool inc = std::holds_alternative<CmdIncrement>(command);
				const std::string& target = inc ? std::get<CmdIncrement>(command).target : std::get<CmdDecrement>(command).target;
				const std::string tag = std::string(inc ? "<inc" : "<dec") + " variable=\"" + target + "\" />";
				const auto dot = target.find('.');
				const std::string ownerName = target.substr(0, dot);
				const Object* owner = findObject(objects, ownerName);
				if (!owner)
				{
					throw std::runtime_error(where + ": " + tag + ": there is no object named '" + ownerName + "'" + didYouMean(ownerName, objectNames(objects))
						+ ". A variable that changes belongs to an object (paddle1.score is paddle1's <variable name=\"score\">); the game's own <variables> are fixed numbers");
				}
				if (dot != std::string::npos)
				{
					const std::string variableName = target.substr(dot + 1);
					if (!owner->variable.count(variableName))
					{
						std::vector<std::string> names;
						for (const auto& entry : owner->variable) { names.push_back(entry.first); }
						throw std::runtime_error(where + ": " + tag + ": '" + ownerName + "' has no variable named '" + variableName + "'" + didYouMean(variableName, names)
							+ (names.empty() ? std::string("; it has no <variables>") : "; its variables are: " + listOf(names)));
					}
				}
			}
		}

		void checkReferences(const std::vector<State>& states, const std::vector<Object>& objects, const std::vector<SoundDesc>& sounds,
			const std::map<std::string, Path>& paths)
		{
			const auto checkAll = [&](const std::vector<Command>& commands, const std::string& where)
			{
				for (const auto& command : commands)
				{
					checkCommand(command, states, objects, sounds, paths, where);
				}
			};

			// A <follow> with no object= is about the object running it, which a
			// state's commands do not have.
			const auto checkOwnFollow = [&](const std::vector<Command>& commands, const std::string& where)
			{
				for (const auto& command : commands)
				{
					const auto* follow = std::get_if<CmdFollow>(&command);
					if (follow && follow->target.empty())
					{
						throw std::runtime_error(where + ": <follow path=\"" + follow->path + "\" /> needs object=\"...\" here; only an object's own rules, actions and timers can leave it out");
					}
				}
			};

			// A path's commands are run by whichever object is flying it, as on
			// one of its own timers.
			for (const auto& [name, path] : paths)
			{
				for (const auto& step : path.steps)
				{
					checkAll(step.commands, "path '" + name + "' > <step>");
					checkPlace(step.commands, Place::PathStep, "path '" + name + "' > <step>");
				}
			}

			// A <become> with no object= is about the object running it, which
			// a state's commands do not have, and which must have that look.
			const auto checkOwnLooks = [&](const std::vector<Command>& commands, const Object* self, const std::string& where)
			{
				for (const auto& command : commands)
				{
					const auto* become = std::get_if<CmdBecome>(&command);
					if (!become || !become->target.empty()) { continue; }
					if (!self)
					{
						throw std::runtime_error(where + ": <become sprite=\"" + become->sprite + "\" /> needs object=\"...\" here; only an object's own rules, actions and timers can leave it out");
					}
					if (std::none_of(self->looks.begin(), self->looks.end(), [&](const Object::Look& look) { return look.name == become->sprite; }))
					{
						throw std::runtime_error(where + ": <become sprite=\"" + become->sprite + "\" />: it has no look of that name (a look is one of several named <sprite>s)");
					}
				}
			};

			for (const auto& state : states)
			{
				const std::string where = "state '" + state.name + "'";
				for (const auto& [key, commands] : state.input)
				{
					const std::string here = where + " > <input button=\"" + keyCodeToString(key) + "\">";
					checkAll(commands, here); checkOwnLooks(commands, nullptr, here); checkOwnFollow(commands, here); checkPlace(commands, Place::Input, here);
				}
				for (const auto& condition : state.conditions)
				{
					const std::string here = where + " > <condition>";
					checkAll(condition.commands, here); checkOwnLooks(condition.commands, nullptr, here); checkOwnFollow(condition.commands, here); checkPlace(condition.commands, Place::Condition, here);
				}
				for (const auto& timer : state.timers)
				{
					const std::string here = where + " > <timer>";
					checkAll(timer.commands, here); checkOwnLooks(timer.commands, nullptr, here); checkOwnFollow(timer.commands, here); checkPlace(timer.commands, Place::StateTimer, here);
				}
			}

			for (const auto& object : objects)
			{
				const std::string where = "object '" + object.name + "'";
				for (const auto& timer : object.timers)
				{
					checkAll(timer.commands, where + " > <timer>"); checkOwnLooks(timer.commands, &object, where + " > <timer>");
					checkPlace(timer.commands, Place::ObjectTimer, where + " > <timer>");
				}
				for (const auto& [name, commands] : object.action) { checkOwnLooks(commands, &object, where); }
				for (const auto& rule : object.collisionData.basic) { checkOwnLooks(rule.commands, &object, where); }
				for (const auto* edge : { &object.collisionData.top, &object.collisionData.bottom, &object.collisionData.left, &object.collisionData.right })
				{
					checkOwnLooks(*edge, &object, where);
				}
				for (const auto& [name, commands] : object.action)
				{
					checkAll(commands, where);
					checkPlace(commands, Place::Action, where + " > <action name=\"" + name + "\">");

					// Turning and thrust along the heading mean nothing to an object
					// that does not face any way.
					for (const auto& command : commands)
					{
						if ((std::holds_alternative<CmdTurn>(command) || std::holds_alternative<CmdThrust>(command)) && !object.hasHeading)
						{
							throw std::runtime_error(where + ": action '" + name + "' turns it or thrusts along its heading, and it has no <heading>");
						}

						// A leap rises against the object's own pull, and only
						// from the ground, which a <land /> rule gives it.
						if (std::holds_alternative<CmdLeap>(command) && !(object.acceleration.y > 0.0f))
						{
							throw std::runtime_error(where + ": action '" + name + "' has a <leap>, and nothing pulls the object down to come back:"
								" give it an <acceleration> with a <y> above 0, and a <collision> with <land /> to stand on");
						}
						if (std::holds_alternative<CmdLeap>(command) && std::none_of(object.collisionData.basic.begin(), object.collisionData.basic.end(),
							[](const CollisionRule& rule) { return std::any_of(rule.commands.begin(), rule.commands.end(), [](const Command& c) { return std::holds_alternative<CmdLand>(c); }); }))
						{
							throw std::runtime_error(where + ": action '" + name + "' has a <leap>, and the object never stands on anything to leap from:"
								" give it a <collision> with <land /> (<collision class=\"girder\"><land /></collision>)");
						}
						if (const auto* climb = std::get_if<CmdClimb>(&command))
						{
							std::vector<std::string> classes;
							for (const Object& other : objects)
							{
								if (!other.objClass.empty() && std::find(classes.begin(), classes.end(), other.objClass) == classes.end()) { classes.push_back(other.objClass); }
							}
							if (std::find(classes.begin(), classes.end(), climb->ladderClass) == classes.end())
							{
								throw std::runtime_error(where + ": action '" + name + "' climbs class=\"" + climb->ladderClass + "\", and no object has that class"
									+ didYouMean(climb->ladderClass, classes) + (classes.empty() ? std::string{} : ". The classes are: " + listOf(classes)));
							}
						}
					}
				}
				for (const auto& rule : object.collisionData.basic)
				{
					checkAll(rule.commands, where);
					checkPlace(rule.commands, Place::Touch, where + " > <collision>");
				}
				for (const auto* edge : { &object.collisionData.top, &object.collisionData.bottom, &object.collisionData.left, &object.collisionData.right })
				{
					checkAll(*edge, where);
					checkPlace(*edge, Place::Edge, where + " > <collision edge>");
				}
			}
		}
	}

	void game_expr::init(const WindowDesc& windowDesc,
		const std::vector<std::pair<std::string, RawValue>>& rawVariables, std::map<std::string, float>& variables,
		std::vector<RawState>& rawStates, std::vector<State>& states,
		std::vector<RawObject>& rawObjects, std::vector<Object>& objects,
		const std::vector<RawSound>& rawSounds, std::vector<SoundDesc>& sounds,
		const std::vector<RawPath>& rawPaths, std::map<std::string, Path>& paths)
	{
		generator.seed(seed());

		checkColor(windowDesc.background, "<window> > <background>");

		// add constants to symbol table (pi, for an angle in a value, among them)
		symbolTable.add_constants();
		symbolTable.add_constant("window.top", 0);
		symbolTable.add_constant("window.bottom", windowDesc.height);
		symbolTable.add_constant("window.left", 0);
		symbolTable.add_constant("window.right", windowDesc.width);
		symbolTable.add_constant("window.width.center", windowDesc.width / 2);
		symbolTable.add_constant("window.height.center", windowDesc.height / 2);

		// register symbol table with expression
		expression.register_symbol_table(symbolTable);

		// add variables from XML file to symbol table, in the order the file
		// declares them so that one can use the ones before it
		for (const auto& [name, rawValue] : rawVariables)
		{
			const float value = evaluate(rawValue, "variable '" + name + "'");
			variables[name] = value;
			symbolTable.add_constant(name, value);
		}

		// Pre-register every object's own <variable> entries, keyed
		// "ownerName.variableName", so any expression evaluated below can refer
		// to another object's variable regardless of which object appears first
		// in the XML (e.g. score1's sprite reading paddle1.score, even though
		// paddle1 is declared after score1). Values start at 0 here; the object
		// loop further down fills in the real evaluated value once it reaches
		// that object - see the objectVariables[...] assignment there.
		for (auto& rawObject : rawObjects)
		{
			for (auto& rawVariable : rawObject.variable)
			{
				objectVariables[rawObject.name + "." + rawVariable.first] = 0.0f;
			}
		}
		for (auto& objectVariable : objectVariables)
		{
			symbolTable.add_variable(objectVariable.first, objectVariable.second);
		}

		// Every object's size as name.width / name.height, so an expression can
		// place an object by its own size (or another's) the same way it reads
		// paddle1.score. Worked out before anything is placed so the order
		// objects appear in the file does not matter; a text's or image's is
		// {0,0} for now (see objectSizes). An object's own <variable> named
		// width or height takes the name instead.
		//
		// The sprite of each object is worked out here once, and a shape (unlike
		// a text showing a variable, whose number may have changed by then) is
		// kept as it came out, so that a <random> in it gives the size used here
		// and the size drawn the same number.
		std::map<std::string, std::vector<std::string>> firstPassSpriteParams;
		std::map<std::string, std::shared_ptr<const Bitmap>> firstPassBitmaps;
		std::map<std::string, std::shared_ptr<const Turnable>> firstPassTurned;
		std::map<std::string, std::vector<std::shared_ptr<const Bitmap>>> firstPassAnimation;
		std::map<std::string, std::vector<std::shared_ptr<const Turnable>>> firstPassAnimationTurned;
		for (auto& rawObject : rawObjects)
		{
			if (objectSizes.count(rawObject.name))
			{
				continue; // measured already
			}

			const std::vector<std::string> params = buildSpriteParams(rawObject.sprite, "object '" + rawObject.name + "'", &firstPassBitmaps[rawObject.name],
				rawObject.hasHeading ? &firstPassTurned[rawObject.name] : nullptr);
			const ShapeKind kind = shapeKindFromTag(params.empty() ? std::string{} : params.at(0));
			objectShapeKinds[rawObject.name] = kind;
			objectSizes[rawObject.name] = measureShapeSize(params, kind);
			firstPassSpriteParams[rawObject.name] = params;

			if (rawObject.hasAnimation)
			{
				firstPassAnimation[rawObject.name] = buildAnimationBitmaps(rawObject, "object '" + rawObject.name + "'",
					rawObject.hasHeading ? &firstPassAnimationTurned[rawObject.name] : nullptr);
			}
		}
		for (auto& [name, size] : objectSizes)
		{
			if (!objectVariables.count(name + ".width")) { symbolTable.add_variable(name + ".width", size.x); }
			if (!objectVariables.count(name + ".height")) { symbolTable.add_variable(name + ".height", size.y); }
		}

		// Where each cell of a group laid out in <columns> and <rows> goes.
		const std::map<std::string, Vector2f> cellOffsets = layOutCells(rawObjects, firstPassSpriteParams);

		// Objects in lockstep share a number: every member of a <group> moves
		// as one block with the others of its group. A plain object that asks
		// for it is a block of one.
		int lockstepNum = 1;
		std::map<std::string, int> groupLockstep;

		// evaluate strings in objects
		for (auto& rawObject : rawObjects)
		{
			const std::string where = "object '" + rawObject.name + "'";

			// Only a text showing a number needs working out again here, for the
			// value it has by now (see above).
			std::vector<std::string> tempSpriteParams = (rawObject.sprite.kind == "text" && rawObject.sprite.textIsNumber)
				? buildSpriteParams(rawObject.sprite, where)
				: firstPassSpriteParams.at(rawObject.name);
			const ShapeKind rawObjectShapeKind = shapeKindFromTag(tempSpriteParams.empty() ? std::string{} : tempSpriteParams.at(0));

			// A position that uses the size of a text or image (its own, or
			// another object's) can only be finished once a backend has measured
			// it - see Object::positionUsesSize. Sizes of shapes are exact
			// already (objectSizes above).
			const std::vector<std::string> positionSizeDependencies = sizeDependenciesOf(rawObject);

			// How long each picture of an animation lasts.
			const int animationFrames = rawObject.hasAnimation ? animationFramesOf(rawObject, windowDesc, where) : 0;

			// The object's looks, if it has several sprites and no animation.
			std::vector<Object::Look> looks;
			if (!rawObject.looks.empty())
			{
				if (rawObject.hasHeading) { throw std::runtime_error(where + ": an object with a <heading> cannot have several looks"); }

				for (const RawSprite& rawLook : rawObject.looks)
				{
					Object::Look look;
					look.name = rawLook.name;
					look.spriteParams = buildSpriteParams(rawLook, where + " > <sprite name=\"" + rawLook.name + "\">", &look.bitmap);
					look.shapeKind = shapeKindFromTag(look.spriteParams.empty() ? std::string{} : look.spriteParams.at(0));
					if (look.shapeKind == ShapeKind::Text && rawLook.textIsNumber)
					{
						throw std::runtime_error(where + ": a look cannot be a text showing a <number>");
					}
					looks.push_back(std::move(look));
				}

				// The first look is the sprite the object starts with; it must be
				// the very same picture.
				looks.front().spriteParams = tempSpriteParams;
				looks.front().bitmap = firstPassBitmaps[rawObject.name];
			}

			int thisLockstep = 0;
			if (rawObject.rawCollisionData.lockstep)
			{
				if (rawObject.groupName.empty())
				{
					thisLockstep = lockstepNum;
				}
				else
				{
					const auto [entry, isFirstMember] = groupLockstep.try_emplace(rawObject.groupName, lockstepNum);
					if (isFirstMember) { lockstepNum++; }
					thisLockstep = entry->second;
				}
			}

			Object object{};

			object.spriteParams = tempSpriteParams;
			object.shapeKind = rawObjectShapeKind;
			object.bitmap = firstPassBitmaps[rawObject.name];
			if (firstPassTurned[rawObject.name]) { object.turnables = { firstPassTurned[rawObject.name] }; }

			if (rawObject.hasAnimation)
			{
				object.animationBitmaps = firstPassAnimation[rawObject.name];
				if (!firstPassAnimationTurned[rawObject.name].empty()) { object.turnables = firstPassAnimationTurned[rawObject.name]; }
				object.animationFrames = animationFrames;
				object.bitmap = object.animationBitmaps.front();
			}

			if (object.shapeKind == ShapeKind::Text && rawObject.sprite.textIsNumber
				&& rawObject.sprite.number.kind == RawValue::Kind::Expression)
			{
				if (auto binding = parseVariableReference(rawObject.sprite.number.text))
				{
					object.boundVariableOwner = binding->first;
					object.boundVariableName = binding->second;
				}
			}

			object.groupName = rawObject.groupName;
			object.name = rawObject.name;
			object.objClass = rawObject.objClass;

			if (rawObject.objClass == "projectile")
			{
				rawObject.isVisible = false;
			}

			object.isVisible = rawObject.isVisible;

			// Its variables first, so its position and velocity can use them
			// (a serve drawn as an angle: ball.speed * cos(ball.angle)).
			for (auto& rawVariable : rawObject.variable)
			{
				const float value = evaluate(rawVariable.second, where);
				object.variable[rawVariable.first] = value;
				object.variableOriginal[rawVariable.first] = value;

				// Keep the cross-object symbol table entry (registered above,
				// before any expression compiled) up to date with the real
				// value now that it's known.
				objectVariables[rawObject.name + "." + rawVariable.first] = value;
			}

			object.startDrawsRandom = rawObject.rawPosition.x.drawsRandom() || rawObject.rawPosition.y.drawsRandom()
				|| rawObject.rawVelocity.x.drawsRandom() || rawObject.rawVelocity.y.drawsRandom()
				|| std::any_of(rawObject.variable.begin(), rawObject.variable.end(), [](const auto& variable) { return variable.second.drawsRandom(); });

			object.positionOriginal.x = evaluate(rawObject.rawPosition.x, where);
			object.positionOriginal.y = evaluate(rawObject.rawPosition.y, where);

			// A cell is its group's position and its place in the
			// group; positionOriginal keeps both, so a reset puts each
			// cell (each brick) back in its own place.
			if (const auto offset = cellOffsets.find(rawObject.name); offset != cellOffsets.end()) { object.cellOffset = offset->second; }
			object.position = object.positionOriginal + object.cellOffset;
			object.positionOriginal = object.position;

			object.sizeDependencies = positionSizeDependencies;
			object.positionUsesSize = !positionSizeDependencies.empty();
			object.positionResolved = !object.positionUsesSize;

			object.velocity.x = evaluate(rawObject.rawVelocity.x, where);
			object.velocity.y = evaluate(rawObject.rawVelocity.y, where);

			object.velocityOriginal = object.velocity;

			if (rawObject.hasAcceleration)
			{
				object.acceleration.x = evaluate(rawObject.rawAcceleration.x, where);
				object.acceleration.y = evaluate(rawObject.rawAcceleration.y, where);
			}
			object.accelerationOriginal = object.acceleration;

			if (rawObject.hasHeading)
			{
				object.hasHeading = true;
				float degrees = std::fmod(evaluate(rawObject.rawHeading, where), 360.0f);
				if (degrees < 0.0f) { degrees += 360.0f; }
				object.heading = degrees;
			}
			object.headingOriginal = object.heading;

			if (rawObject.hasDrag)
			{
				object.drag = evaluate(rawObject.rawDrag, where);
				if (object.drag < 0.0f || object.drag >= 1.0f)
				{
					throw std::runtime_error(where + ": <drag> is " + std::to_string(object.drag) + "; expected 0 up to (not including) 1");
				}
			}

			if (!rawObject.facing.empty())
			{
				object.hasFacing = true;
				object.facing = rawObject.facing == "down" ? Direction::Down
					: rawObject.facing == "left" ? Direction::Left
					: rawObject.facing == "right" ? Direction::Right
					: Direction::Up;
			}
			object.facingOriginal = object.facing;
			object.timers = processTimers(rawObject.timers, where);
			object.looks = looks;

			object.showHeading();

			object.collisionData.enabled = rawObject.rawCollisionData.enabled;
			object.isVisibleOriginal = object.isVisible;
			object.collisionEnabledOriginal = object.collisionData.enabled;
			object.collisionData.lockstep = thisLockstep;
			object.collisionData.type = rawObject.rawCollisionData.type;

			// A pixel collision tests the pixels that are drawn; a line
			// drawing has them from the start, and a circle or rectangle
			// is solid all over, but what a text or an image looks like
			// is only known once a window backend has drawn it.
			if (object.collisionData.type == CollisionType::Pixel
				&& (object.shapeKind == ShapeKind::Text || object.shapeKind == ShapeKind::Image))
			{
				throw std::runtime_error(where + ": <type>pixel</type> needs a sprite of lines, a circle or a rectangle; the pixels of text and images are only known to a window backend");
			}

			object.collisionData.top = processCommands(rawObject.rawCollisionData.top, where);
			object.collisionData.bottom = processCommands(rawObject.rawCollisionData.bottom, where);
			object.collisionData.left = processCommands(rawObject.rawCollisionData.left, where);
			object.collisionData.right = processCommands(rawObject.rawCollisionData.right, where);
			object.collisionData.topGuards = rawObject.rawCollisionData.topGuards;
			object.collisionData.bottomGuards = rawObject.rawCollisionData.bottomGuards;
			object.collisionData.leftGuards = rawObject.rawCollisionData.leftGuards;
			object.collisionData.rightGuards = rawObject.rawCollisionData.rightGuards;
			for (const auto* edgeGuards : { &object.collisionData.topGuards, &object.collisionData.bottomGuards, &object.collisionData.leftGuards, &object.collisionData.rightGuards })
			{
				for (const EdgeGuard& edgeGuard : *edgeGuards)
				{
					if (!edgeGuard.sprite.empty() && std::none_of(looks.begin(), looks.end(), [&](const Object::Look& look) { return look.name == edgeGuard.sprite; }))
					{
						throw std::runtime_error(where + ": a <collision edge sprite=\"" + edgeGuard.sprite + "\"> names no look of the object (a look is one of several named <sprite>s)");
					}
				}
			}

			for (auto& rawRule : rawObject.rawCollisionData.basic)
			{
				CollisionRule rule;
				rule.filterClass = rawRule.filterClass;
				rule.filterObject = rawRule.filterObject;
				rule.unlessClass = rawRule.unlessClass;
				rule.whileSprite = rawRule.whileSprite;
				if (!rule.whileSprite.empty() && std::none_of(looks.begin(), looks.end(), [&](const Object::Look& look) { return look.name == rule.whileSprite; }))
				{
					throw std::runtime_error(where + ": a <collision sprite=\"" + rule.whileSprite + "\"> names no look of the object (a look is one of several named <sprite>s)");
				}
				if (rawRule.slower) { rule.slower = evaluate(*rawRule.slower, where); }
				if (rawRule.faster) { rule.faster = evaluate(*rawRule.faster, where); }
				rule.commands = processCommands(rawRule.commands, where);
				object.collisionData.basic.push_back(std::move(rule));
			}

			//object.action = rawObject.action;
			for (auto& rawAction : rawObject.action)
			{
				object.action[rawAction.first] = processCommands(rawAction.second, where);
			}


			// object's visual is built later by whichever Window backend is
			// running, once one exists (see Window::init() in window.h) -
			// visualDirty starts true (Object's own default), so nothing
			// needs to happen here.
			objects.push_back(std::move(object));

			if (rawObject.rawCollisionData.lockstep && rawObject.groupName.empty()) {
				lockstepNum++;
			}
		}

		for (auto& rawState : rawStates)
		{
			State state{};
			const std::string where = "state '" + rawState.name + "'";

			state.name = rawState.name;
			state.show = rawState.show;

			for (auto& rawAction : rawState.input)
			{
				const KeyCode key = keyCodeFromString(rawAction.first);
				if (key == KeyCode::Unknown)
				{
					throw std::runtime_error(where + " > <input button=\"" + rawAction.first + "\">: there is no key named '" + rawAction.first + "'"
						+ didYouMean(rawAction.first, keyNames()) + ". The keys are: " + listOf(keyNames()));
				}
				state.input[key] = processCommands(rawAction.second, where);
			}

			for (auto& rawCondition : rawState.conditions)
			{
				Condition condition;
				condition.filterClass = rawCondition.filterClass;
				condition.filterObject = rawCondition.filterObject;
				condition.variableName = rawCondition.variableName;

				const float threshold = evaluate(rawCondition.threshold, where);
				switch (rawCondition.test)
				{
				case RawCondition::Test::AtLeast:   condition.value = threshold; break;
				case RawCondition::Test::AtMost:    condition.atMost = threshold; break;
				case RawCondition::Test::Remaining: condition.remaining = threshold; break;
				}

				if (condition.remaining && std::none_of(objects.begin(), objects.end(), [&](const Object& object)
					{
						return (rawCondition.filterClass.empty() || rawCondition.filterClass == object.objClass)
							&& (rawCondition.filterObject.empty() || rawCondition.filterObject == object.name
								|| (!object.groupName.empty() && rawCondition.filterObject == object.groupName));
					}))
				{
					std::cout << "warning: state '" << rawState.name << "': a condition with remaining= matches no object at all, so it would fire at once\n";
				}
				condition.commands = processCommands(rawCondition.commands, where);
				state.conditions.push_back(std::move(condition));
			}

			state.timers = processTimers(rawState.timers, where);

			states.push_back(state);
		}

		// The sounds, worked out after everything else so that a length can use
		// any variable.
		for (const RawSound& rawSound : rawSounds)
		{
			const bool taken = std::any_of(sounds.begin(), sounds.end(), [&](const SoundDesc& sound) { return sound.name == rawSound.name; });
			if (taken)
			{
				throw std::runtime_error("sound '" + rawSound.name + "': there is already a sound of that name");
			}
			sounds.push_back(processSound(rawSound));
		}

		// The paths, worked out last too: a step's numbers can use any variable.
		for (const RawPath& rawPath : rawPaths)
		{
			const std::string where = "path '" + rawPath.name + "'";
			if (paths.count(rawPath.name))
			{
				throw std::runtime_error(where + ": there is already a path of that name");
			}

			Path path;
			path.name = rawPath.name;
			path.speed = evaluate(rawPath.speed, where + " > <speed>");
			if (!(path.speed > 0.0f))
			{
				throw std::runtime_error(where + ": the <speed> is not above 0, so it would never get anywhere");
			}
			path.hasStart = rawPath.hasStart;
			if (rawPath.hasStart)
			{
				path.start = { evaluate(rawPath.startX, where + " > <start>"), evaluate(rawPath.startY, where + " > <start>") };
			}
			for (const RawPathStep& rawStep : rawPath.steps)
			{
				PathStep step;
				step.home = rawStep.home;
				if (!rawStep.home)
				{
					step.by = { evaluate(rawStep.x, where + " > <step>"), evaluate(rawStep.y, where + " > <step>") };
					step.commands = processCommands(rawStep.commands, where + " > <step>");
				}
				path.steps.push_back(std::move(step));
			}
			paths[path.name] = std::move(path);
		}

		checkReferences(states, objects, sounds, paths);
	}

	SoundDesc game_expr::processSound(const RawSound& raw)
	{
		const std::string where = "sound '" + raw.name + "'";

		SoundDesc sound;
		sound.name = raw.name;

		const bool noVolume = raw.volume.kind == RawValue::Kind::Expression && raw.volume.text.empty();
		if (!noVolume)
		{
			sound.volume = evaluate(raw.volume, where + " > <volume>");
			if (!(sound.volume >= 0.0f && sound.volume <= 1.0f))
			{
				throw std::runtime_error(where + ": <volume> is " + std::to_string(sound.volume) + "; expected 0 (silent) to 1 (loudest)");
			}
		}

		const auto waveOf = [&](const std::string& name, const std::string& here)
		{
			if (name.empty()) { return Waveform::Square; }
			const std::optional<Waveform> wave = waveformFromName(name);
			if (!wave)
			{
				throw std::runtime_error(here + ": wave=\"" + name + "\"; expected square, triangle, sawtooth, sine or noise");
			}
			return *wave;
		};

		const auto pitchOf = [&](const std::string& name, const char* attribute, const std::string& here)
		{
			const std::optional<float> hz = pitchFromName(name);
			if (!hz)
			{
				throw std::runtime_error(here + ": " + attribute + "=\"" + name + "\" is not a pitch; expected a note such as C4, F#3 or Bb5, or a number of hertz");
			}
			return *hz;
		};

		const Waveform soundWave = waveOf(raw.wave, where);

		for (std::size_t i = 0; i < raw.notes.size(); ++i)
		{
			const RawNote& rawNote = raw.notes[i];
			const std::string here = where + " > " + (rawNote.rest ? "<rest>" : "<note>") + " " + std::to_string(i + 1);

			SoundNote note;
			note.rest = rawNote.rest;
			note.seconds = evaluate(rawNote.length, here);
			if (!(note.seconds > 0.0f && note.seconds <= kMaxNoteSeconds))
			{
				throw std::runtime_error(here + ": lasts " + std::to_string(note.seconds) + " seconds; expected more than 0 and at most "
					+ std::to_string(static_cast<int>(kMaxNoteSeconds)));
			}

			if (!rawNote.rest)
			{
				note.wave = rawNote.wave.empty() ? soundWave : waveOf(rawNote.wave, here);
				note.startHz = pitchOf(rawNote.pitch, "pitch", here);
				note.endHz = rawNote.to.empty() ? note.startHz : pitchOf(rawNote.to, "to", here);
			}

			sound.notes.push_back(note);
		}

		return sound;
	}

	float game_expr::evaluate(const RawValue& value, const std::string& where)
	{
		switch (value.kind)
		{
		case RawValue::Kind::Random:
		{
			const float min = evaluateExpression(value.min, where);
			const float max = evaluateExpression(value.max, where);
			return randomNumberRange(min, max);
		}

		case RawValue::Kind::Equation:
		{
			std::map<std::string, Answer> steps;
			Answer answer;
			const std::string here = where + " > <equation>";

			for (const RawOperation& step : *value.operations)
			{
				answer = evaluateOperation(step, steps, here);
				if (!step.name.empty()) { steps[step.name] = answer; }
			}

			return answer.value;
		}

		case RawValue::Kind::Formula:
			return evaluateOperation(value.operations->front(), {}, where + " > <formula>").value;

		case RawValue::Kind::Expression:
			break;
		}

		return evaluateExpression(value.text, where);
	}

	bool game_expr::isLate(const std::string& name) const
	{
		if (objectVariables.count(name)) { return true; }

		for (const char* suffix : { ".width", ".height" })
		{
			const std::string tail = suffix;
			if (name.size() > tail.size() && name.compare(name.size() - tail.size(), tail.size(), tail) == 0
				&& objectSizes.count(name.substr(0, name.size() - tail.size())))
			{
				return true;
			}
		}

		return false;
	}

	game_expr::Answer game_expr::evaluateOperation(const RawOperation& operation, const std::map<std::string, Answer>& steps, const std::string& where)
	{
		const std::string here = where + " > <" + operation.op + ">";

		const auto operandAnswer = [&](const RawOperand& operand)
		{
			if (operand.value.kind == RawValue::Kind::Expression)
			{
				if (const auto step = steps.find(operand.value.text); step != steps.end()) { return step->second; }
				return Answer{ evaluate(operand.value, here), isLate(operand.value.text) };
			}

			// An operation nested in a formula is part of the same formula.
			if (operand.value.kind == RawValue::Kind::Formula) { return evaluateOperation(operand.value.operations->front(), {}, here); }

			return Answer{ evaluate(operand.value, here), false };
		};

		Answer answer = operandAnswer(operation.operands.front());

		for (std::size_t i = 1; i < operation.operands.size(); ++i)
		{
			const Answer operand = operandAnswer(operation.operands[i]);
			answer.late = answer.late || operand.late;

			if (operation.op == "divide" && operand.value == 0.0f)
			{
				if (loading && !operand.late) { throw std::runtime_error(here + ": the divisor is 0"); }

				// Late while loading: not known yet, so nothing to say; it is worked out again.
				if (!loading && warnedDivisions.insert(here).second)
				{
					std::cerr << "warning: " << here << ": the divisor is 0; using 0 for the answer" << std::endl;
				}
				return Answer{ 0.0f, answer.late };
			}

			if (operation.op == "add") { answer.value += operand.value; }
			else if (operation.op == "subtract") { answer.value -= operand.value; }
			else if (operation.op == "multiply") { answer.value *= operand.value; }
			else { answer.value /= operand.value; }
		}

		return answer;
	}

	float game_expr::evaluateExpression(const std::string& text, const std::string& where)
	{
		if (!parser.compile(text, expression))
		{
			throw std::runtime_error(where + ": cannot read \"" + text + "\": " + parser.error().c_str());
		}
		return expression.value();
	}

	std::vector<Timer> game_expr::processTimers(const std::vector<RawTimer>& raw, const std::string& where)
	{
		std::vector<Timer> timers;
		const std::string here = where + " > <timer>";

		for (const RawTimer& rawTimer : raw)
		{
			Timer timer;
			timer.repeat = rawTimer.repeat;
			timer.interval = rawTimer.interval;
			timer.commands = processCommands(rawTimer.commands, here);

			// Worked out once now so that a mistake in it is a load error; it
			// is worked out again every time the timer starts over. Only a plain
			// number can be held to being above 0 now: an expression may read a
			// variable that has no value yet (and at run time a timer never
			// waits less than one frame).
			const float seconds = evaluate(rawTimer.interval, here);
			char* end = nullptr;
			const std::string& text = rawTimer.interval.text;
			const bool plainNumber = rawTimer.interval.kind == RawValue::Kind::Expression && !text.empty()
				&& (std::strtof(text.c_str(), &end), end == text.c_str() + text.size());
			if (plainNumber && !(seconds > 0.0f))
			{
				throw std::runtime_error(here + ": " + (rawTimer.repeat ? "<every>" : "<after>") + " is " + std::to_string(seconds) + "; expected a number of seconds above 0");
			}

			timers.push_back(std::move(timer));
		}

		return timers;
	}

	std::vector<Command> game_expr::processCommands(const std::vector<RawCommand>& raw, const std::string& where)
	{
		try
		{
			return makeCommands(raw, [&](const RawValue& value) { return evaluate(value, where); });
		}
		catch (const std::runtime_error& error)
		{
			throw std::runtime_error(where + ": " + error.what());
		}
	}

	std::vector<std::string> game_expr::buildSpriteParams(const RawSprite& sprite, const std::string& where,
		std::shared_ptr<const Bitmap>* bitmap, std::shared_ptr<const Turnable>* turnable)
	{
		const std::string color = sprite.color.empty() ? "color.white" : sprite.color;
		checkColor(color, where);
		std::vector<std::string> params;

		// Lines, rows of text and SVG drawings are all pictures the engine draws
		// itself, and all go the same way, here, once, when the game loads:
		//   1. read what the file describes (line ends, rows, a part of a drawing);
		//   2. draw it into a Bitmap, the pixels every window backend shows and a
		//      pixel collision tests, so no backend ever draws one itself;
		//   3. flip it, if the sprite says so (a <bitmap> or an <svg>), the two
		//      steps done once for all the sprites that are the same picture;
		//   4. for an object with a <heading>, keep it (a Turnable) to be turned
		//      to whatever heading it faces, and start it at heading 0.
		// The params carry only the picture's size. The one difference is in the
		// turning: lines are kept as lines and drawn again at each heading, which
		// keeps their edges sharp, where a finished picture's pixels are turned.
		if (sprite.kind == "line" || sprite.kind == "bitmap" || sprite.kind == "svg")
		{
			auto kept = std::make_shared<Turnable>();
			std::shared_ptr<const Bitmap> drawn;

			// 3. Flipped.
			const auto flipped = [&](Bitmap picture)
			{
				return sprite.flip.empty() ? picture : flipBitmap(picture, sprite.flip == "horizontal", sprite.flip == "vertical");
			};

			try
			{
				if (sprite.kind == "line")
				{
					// 1. The line ends, colours and thicknesses.
					for (const RawLine& rawLine : sprite.lines)
					{
						LineSegment segment;
						segment.x1 = evaluate(rawLine.from.x, where);
						segment.y1 = evaluate(rawLine.from.y, where);
						segment.x2 = evaluate(rawLine.to.x, where);
						segment.y2 = evaluate(rawLine.to.y, where);
						checkColor(rawLine.color.empty() ? "color.white" : rawLine.color, where + " > <line>");
						segment.color = colorFromName(rawLine.color.empty() ? "color.white" : rawLine.color);
						if (rawLine.hasThickness) { segment.thickness = static_cast<int>(std::lround(evaluate(rawLine.thickness, where))); }
						if (segment.thickness < 1) { throw std::invalid_argument("a <line> has a <thickness> under 1"); }
						kept->lines.push_back(segment);
					}

					// 2. Drawn. (Kept as lines for turning: see step 4.)
					drawn = std::make_shared<const Bitmap>(rasterizeLines(kept->lines));
				}
				else if (sprite.kind == "svg")
				{
					// 1. The part of the drawing, and how many pixels to a unit.
					SvgRegion region;
					float svgScale = 1.0f;
					if (sprite.hasSvgRegion)
					{
						region = { evaluate(sprite.svgX, where), evaluate(sprite.svgY, where),
							evaluate(sprite.svgWidth, where), evaluate(sprite.svgHeight, where) };
						if (region.isWhole()) { throw std::invalid_argument("an svg's <width> and <height> must both be above 0"); }
					}
					if (sprite.hasScale) { svgScale = evaluate(sprite.scale, where); }

					// 2. Drawn by lunasvg (svg.cpp), once for every sprite that
					// takes the same part at the same scale (a group's cells).
					std::string key = "svg|" + sprite.path + "|" + std::to_string(region.x) + "," + std::to_string(region.y) + ","
						+ std::to_string(region.width) + "," + std::to_string(region.height) + "|" + std::to_string(svgScale) + "|" + sprite.flip;
					for (const std::string& hide : sprite.svgHide) { key += "|" + hide; }
					drawn = drawOnce(key, [&] { return flipped(rasterizeSvg(sprite.path, region, svgScale, sprite.svgHide)); });
				}
				else
				{
					// 1. The rows, and how many pixels to a character.
					const int rowsScale = sprite.hasScale ? static_cast<int>(std::lround(evaluate(sprite.scale, where))) : 1;

					// 2. Drawn, once for every sprite of the same rows, scale and
					// color (a group's cells).
					std::string key = "rows|" + std::to_string(rowsScale) + "|" + color + "|" + sprite.flip;
					for (const std::string& row : sprite.bitmapRows) { key += "|" + row; }
					drawn = drawOnce(key, [&] { return flipped(rasterizeRows(sprite.bitmapRows, rowsScale, colorFromName(color))); });
				}


				// 4. Kept to be turned, for an object that faces somewhere; heading 0
				// is what it starts as, and the square it turns in is its size.
				if (turnable)
				{
					if (kept->lines.empty()) { kept->picture = drawn; }
					if (!kept->lines.empty() || drawn->width > 0)
					{
						drawn = std::make_shared<const Bitmap>(kept->at(0.0f));
					}
					*turnable = std::move(kept);
				}
			}
			catch (const std::exception& error)
			{
				// std::invalid_argument is a mistake in the numbers, rows or
				// lines, std::runtime_error an svg file that cannot be read;
				// either way it is the game file that is at fault, so say where.
				throw std::runtime_error(where + ": " + error.what());
			}

			params = { "line", std::to_string(drawn->width), std::to_string(drawn->height) };
			if (bitmap) { *bitmap = std::move(drawn); }
		}
		else if (sprite.kind == "circle")
		{
			params = { "circle", std::to_string(evaluate(sprite.radius, where)), std::to_string(0), color };
		}
		else if (sprite.kind == "rectangle")
		{
			params = { "rectangle", std::to_string(evaluate(sprite.width, where)), std::to_string(evaluate(sprite.height, where)), color };
		}
		else if (sprite.kind == "text")
		{
			// A fixed label, or a number - shown as the whole number it is, with
			// no decimal point, the way someone typing the label would.
			const std::string label = sprite.textIsNumber ? formatDisplayNumber(evaluate(sprite.number, where)) : sprite.content;
			params = { "text", label, std::to_string(evaluate(sprite.size, where)), color };
		}
		else if (sprite.kind == "image")
		{
			params = { "image", sprite.path, sprite.flip.empty() ? "color.white" : "flip." + sprite.flip };
		}
		else
		{
			throw std::runtime_error(where + ": unknown shape '" + sprite.kind + "'");
		}

		return params;
	}

	std::vector<std::shared_ptr<const Bitmap>> game_expr::buildAnimationBitmaps(const RawObject& rawObject, const std::string& where,
		std::vector<std::shared_ptr<const Turnable>>* turnables)
	{
		std::vector<std::shared_ptr<const Bitmap>> pictures;

		for (std::size_t i = 0; i < rawObject.animation.frames.size(); ++i)
		{
			const RawSprite& frame = rawObject.animation.frames[i];
			const std::string here = where + " > <animation> > frame " + std::to_string(i + 1)
				+ (frame.name.empty() ? std::string{} : " (\"" + frame.name + "\")");

			if (frame.kind != "bitmap" && frame.kind != "svg" && frame.kind != "line")
			{
				throw std::runtime_error(here + ": a frame must be a <bitmap>, an <svg> or a drawing of <line>s, not a <" + frame.kind + ">");
			}

			std::shared_ptr<const Bitmap> picture;
			std::shared_ptr<const Turnable> turnable;
			buildSpriteParams(frame, here, &picture, turnables ? &turnable : nullptr);

			if (i > 0)
			{
				if (picture->width != pictures.front()->width || picture->height != pictures.front()->height)
				{
					throw std::runtime_error(here + ": is " + std::to_string(picture->width) + " by " + std::to_string(picture->height)
						+ " pixels, but frame 1 is " + std::to_string(pictures.front()->width) + " by " + std::to_string(pictures.front()->height)
						+ "; the frames of an animation must all be the same size");
				}
			}

			pictures.push_back(std::move(picture));
			if (turnables) { turnables->push_back(std::move(turnable)); }
		}

		return pictures;
	}

	int game_expr::animationFramesOf(const RawObject& rawObject, const WindowDesc& windowDesc, const std::string& where)
	{
		const float seconds = evaluate(rawObject.animation.interval, where + " > <animation> > <interval>");

		if (!(seconds > 0.0f))
		{
			throw std::runtime_error(where + ": <animation> > <interval> is " + std::to_string(seconds) + "; expected a number of seconds above 0");
		}
		if (windowDesc.framerate < 1)
		{
			throw std::runtime_error(where + ": <animation> > <interval> is in seconds, so the <window> needs a <framerate> above 0 to count them in");
		}

		return std::max(1, static_cast<int>(std::lround(seconds * static_cast<float>(windowDesc.framerate))));
	}

	std::map<std::string, Vector2f> game_expr::layOutCells(const std::vector<RawObject>& rawObjects,
		const std::map<std::string, std::vector<std::string>>& spriteParams)
	{
		// Each group's cells by row, in the order they were read: a row's
		// columns left to right.
		std::map<std::string, std::map<int, std::vector<const RawObject*>>> groups;
		for (const RawObject& rawObject : rawObjects)
		{
			if (rawObject.cell.column > 0) { groups[rawObject.groupName][rawObject.cell.row].push_back(&rawObject); }
		}

		std::map<std::string, Vector2f> offsets;
		for (const auto& [group, rows] : groups)
		{
			float top = 0.0f;
			float above = 0.0f; // the height of the row above
			for (const auto& [row, cells] : rows)
			{
				const RawObject& first = *cells.front();
				const std::string where = "object '" + first.name + "'";

				// The slot is the row's sprite, so a cell with a sprite of its
				// own of another size sits in the middle of the place its row
				// gives it.
				const std::vector<std::string> slotParams = buildSpriteParams(first.cell.slot, where);
				const ShapeKind slotKind = shapeKindFromTag(slotParams.empty() ? std::string{} : slotParams.at(0));
				const Vector2f slot = measureShapeSize(slotParams, slotKind);
				if (slotKind == ShapeKind::Text || slotKind == ShapeKind::Image)
				{
					std::cout << "warning: group '" << group << "': row " << row << " is "
						<< (slotKind == ShapeKind::Text ? "text" : "an image")
						<< ", whose size is only known to a window backend (see measureShapeSize), so its cells are spaced by the <padding> alone\n";
				}

				if (row > rows.begin()->first) { top += above + evaluate(first.cell.gapAbove, where); }
				above = slot.y;

				float left = 0.0f;
				for (const RawObject* cell : cells)
				{
					const std::string here = "object '" + cell->name + "'";
					if (cell->cell.column > 1) { left += slot.x + evaluate(cell->cell.gapBefore, here); }

					const std::vector<std::string>& params = spriteParams.at(cell->name);
					const Vector2f own = measureShapeSize(params, shapeKindFromTag(params.empty() ? std::string{} : params.at(0)));
					offsets[cell->name] = { left + (slot.x - own.x) / 2.0f, top + (slot.y - own.y) / 2.0f };
				}
			}
		}

		return offsets;
	}

	void game_expr::setObjectSize(const std::string& name, const Vector2f& size)
	{
		if (auto it = objectSizes.find(name); it != objectSizes.end())
		{
			it->second = size;
		}
	}

	std::vector<std::string> game_expr::sizeDependenciesOf(const RawObject& rawObject) const
	{
		std::vector<std::string> dependencies;

		// Every identifier in the position expressions, e.g. "title.width" or
		// "window.width.center"; the ones that end in .width / .height and name
		// an object whose size needs a backend are what this is looking for.
		std::vector<const std::string*> positionExpressions = rawObject.rawPosition.x.expressions();
		for (const std::string* yExpression : rawObject.rawPosition.y.expressions())
		{
			positionExpressions.push_back(yExpression);
		}

		for (const std::string* positionExpression : positionExpressions)
		{
			std::size_t i = 0;
			while (i < positionExpression->size())
			{
				const auto isIdentifierChar = [](char c) { return std::isalnum(static_cast<unsigned char>(c)) || c == '_' || c == '.'; };
				if (!isIdentifierChar((*positionExpression)[i]))
				{
					++i;
					continue;
				}

				std::size_t end = i;
				while (end < positionExpression->size() && isIdentifierChar((*positionExpression)[end])) { ++end; }
				const std::string token = positionExpression->substr(i, end - i);
				i = end;

				for (const std::string suffix : { ".width", ".height" })
				{
					if (token.size() <= suffix.size() || token.compare(token.size() - suffix.size(), suffix.size(), suffix) != 0)
					{
						continue;
					}

					const std::string name = token.substr(0, token.size() - suffix.size());
					const auto kind = objectShapeKinds.find(name);
					if (kind != objectShapeKinds.end() && sizeNeedsBackend(kind->second)
						&& !objectVariables.count(token)
						&& std::find(dependencies.begin(), dependencies.end(), name) == dependencies.end())
					{
						dependencies.push_back(name);
					}
				}
			}
		}

		return dependencies;
	}
}
