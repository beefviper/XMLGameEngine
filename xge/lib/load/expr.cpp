// expr.cpp
// XML Game Engine
// author: beefviper
// date: Sept 21, 2020

#include "load/expr.h"

#include "picture/color.h"
#include "engine/keys.h"
#include "load/spelling.h"
#include "picture/svg.h"

#include <algorithm>
#include <functional>
#include <initializer_list>
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
		// Whether a text sprite has a <number> or a <letter> among its pieces.
		bool showsNumbers(const RawSprite& sprite)
		{
			return std::any_of(sprite.text.begin(), sprite.text.end(),
				[](const RawTextPiece& piece) { return piece.kind != RawTextPiece::Kind::Words; });
		}

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
					return object.name == name || object.inGroup(name);
				});
			if (made != objects.end()) { return &*made; }

			// A copy that will be made while the game runs (shot.2): named by
			// an object that does not exist, with a number after it.
			const auto dot = name.rfind('.');
			if (dot == std::string::npos || dot + 1 == name.size()
				|| !std::all_of(name.begin() + static_cast<std::ptrdiff_t>(dot) + 1, name.end(), [](char c) { return std::isdigit(static_cast<unsigned char>(c)) != 0; }))
			{
				return nullptr;
			}
			const Object* of = findObject(objects, name.substr(0, dot));
			return of && !of->exists ? of : nullptr;
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
				names.insert(object.outerGroups.begin(), object.outerGroups.end());
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
				[](const CmdCarry&) { return std::string("<carry />"); },
				[](const CmdDrop&) { return std::string("<drop />"); },
				[](const CmdTrack&) { return std::string("<track />"); },
				[](const CmdDeflect&) { return std::string("<deflect>"); },
				[](const CmdKnock&) { return std::string("<knock>"); },
				[](const CmdHole&) { return std::string("<hole>"); },
				[](const CmdHold&) { return std::string("<hold>"); },
				[](const CmdReverse&) { return std::string("<reverse />"); },
				[](const CmdMove&) { return std::string("<move>"); },
				[](const CmdHop&) { return std::string("<hop>"); },
				[](const CmdJump&) { return std::string("<jump>"); },
				[](const CmdLand&) { return std::string("<land />"); },
				[](const CmdLeap&) { return std::string("<leap>"); },
				[](const CmdClimb&) { return std::string("<climb>"); },
				[](const CmdChase&) { return std::string("<chase>"); },
				[](const CmdGo&) { return std::string("<go>"); },
				[](const CmdHunt&) { return std::string("<hunt>"); },
				[](const CmdHuntHop&) { return std::string("<hunt> by <hop>"); },
				[](const CmdWander&) { return std::string("<wander>"); },
				[](const CmdAim&) { return std::string("<aim>"); },
				[](const CmdAccelerate&) { return std::string("<accelerate>"); },
				[](const CmdTurn&) { return std::string("<turn>"); },
				[](const CmdThrust&) { return std::string("<thrust>"); },
				[](const CmdRelease&) { return std::string("<release>"); },
				[](const CmdStop&) { return std::string("<stop />"); },
				[](const CmdIncrement&) { return std::string("<inc>"); },
				[](const CmdCopy&) { return std::string("<copy>"); },
				[](const CmdRank&) { return std::string("<rank>"); },
				[](const CmdTurns&) { return std::string("<turns>"); },
				[](const CmdPass&) { return std::string("<pass />"); },
				[](const CmdOut&) { return std::string("<out />"); },
				[](const CmdDemo&) { return std::string("<demo />"); },
				[](const CmdDecrement&) { return std::string("<dec>"); },
				[](const CmdPushState&) { return std::string("<push>"); },
				[](const CmdPopState&) { return std::string("<pop>"); },
				[](const CmdFire&) { return std::string("<fire>"); },
				[](const CmdTriggerAction&) { return std::string("<trigger>"); },
				[](const CmdResetObject&) { return std::string("<reset object>"); },
				[](const CmdPlay&) { return std::string("<play>"); },
				[](const CmdSilence&) { return std::string("<silence>"); },
				[](const CmdBecome&) { return std::string("<become>"); },
				[](const CmdReveal&) { return std::string("<reveal>"); },
				[](const CmdFollow&) { return std::string("<follow>"); },
				[](const CmdEnterScene&) { return std::string("<enter>"); },
				[](const CmdResetScene&) { return std::string("<reset scene>"); },
				[](const CmdSet&) { return std::string("<set>"); },
				[](const CmdSpawn&) { return std::string("<spawn>"); },
				[](const CmdDespawn& d) { return std::string(d.target.empty() ? "<despawn />" : "<despawn object>"); },
				[](const CmdLive& live) { return tagOf(live.loaded->command); },
			}, command);
		}

		// A condition compared with another variable: the object named has
		// it, or, with no object named, every object the condition tests that
		// has its variable has the other one too.
		void checkOtherVariable(const Condition& condition, const std::vector<Object>& objects, const std::string& where)
		{
			const auto has = [&](const Object& object) { return numberOf(object, condition.otherVariable).has_value(); };

			if (!condition.otherObject.empty())
			{
				const auto other = std::find_if(objects.begin(), objects.end(), [&](const Object& object) { return object.name == condition.otherObject; });
				if (other == objects.end())
				{
					std::vector<std::string> names;
					for (const Object& object : objects) { names.push_back(object.name); }
					throw std::runtime_error(where + ": compares with object=\"" + condition.otherObject + "\", and there is no object of that name" + didYouMean(condition.otherObject, names));
				}
				if (!has(*other))
				{
					std::vector<std::string> names;
					for (const auto& [name, value] : other->variable) { names.push_back(name); }
					throw std::runtime_error(where + ": compares with " + condition.otherObject + "." + condition.otherVariable + ", and " + condition.otherObject + " has no variable of that name" + didYouMean(condition.otherVariable, names));
				}
				return;
			}

			for (const Object& object : objects)
			{
				const bool tested = (condition.filterClass.empty() || condition.filterClass == object.objClass)
					&& (condition.filterObject.empty() || condition.filterObject == object.name || object.inGroup(condition.filterObject))
					&& numberOf(object, condition.variableName);
				if (tested && !has(object))
				{
					throw std::runtime_error(where + ": compares " + object.name + "." + condition.variableName + " with its own " + condition.otherVariable + ", and " + object.name + " has no variable of that name");
				}
			}
		}

		// Where a list of commands runs, and what each place does something
		// with. These mirror the dispatchers in play/executor.cpp (a command
		// one of them leaves to its catch-all does nothing there); change one
		// and change the other.
		enum class Place { Edge, Touch, Input, Condition, StateTimer, ObjectTimer, Action, PathStep, Computer };

		struct PlaceRule
		{
			const char* what;
			std::vector<std::string> tags;
		};

		// The commands in the groups the schema has them in (xgedef.xsd,
		// changesCommands and the rest), and the groups each place takes.
		std::vector<std::string> commandsOf(std::initializer_list<const std::vector<std::string>*> groups)
		{
			std::vector<std::string> tags;
			for (const auto* group : groups) { tags.insert(tags.end(), group->begin(), group->end()); }
			return tags;
		}

		const PlaceRule& ruleFor(Place place)
		{
			static const std::vector<std::string> changes{ "<inc>", "<dec>", "<set>", "<copy>", "<rank>", "<play>", "<silence>", "<become>", "<reveal>", "<follow>", "<enter>",
				"<reset />", "<reset object>", "<reset scene>", "<spawn>", "<despawn object>", "<trigger>" };
			static const std::vector<std::string> flow{ "<push>", "<pop>", "<turns>", "<pass />", "<out />", "<demo />" };
			static const std::vector<std::string> self{ "<die />", "<drop />", "<move>", "<release>", "<reverse />", "<stop />", "<fire>", "<despawn />" };
			static const std::vector<std::string> steering{ "<aim>", "<chase>", "<hunt>", "<hunt> by <hop>", "<wander>" };
			static const std::vector<std::string> touch{ "<carry />", "<deflect>", "<hole>", "<knock>", "<land />", "<ride />" };
			static const std::vector<std::string> edge{ "<stick />", "<wrap />" };
			static const std::vector<std::string> bounce{ "<bounce />" };
			static const std::vector<std::string> controls{ "<accelerate>", "<climb>", "<go>", "<hop>", "<jump>", "<leap>", "<thrust>", "<turn>", "<track />" };
			static const std::vector<std::string> press{ "<hold>" };

			static const std::vector<std::string> stateCommands = commandsOf({ &changes, &flow });
			static const std::vector<std::string> objectTimeCommands = commandsOf({ &changes, &self, &steering });
			static const PlaceRule rules[] = {
				{ "a screen-edge <collision>", commandsOf({ &changes, &self, &edge, &bounce }) },
				{ "a <collision> with another object", commandsOf({ &changes, &self, &steering, &touch, &bounce }) },
				{ "an <input>", stateCommands },
				{ "a <condition>", stateCommands },
				{ "a state's <timer>", stateCommands },
				{ "an object's <timer>", objectTimeCommands },
				{ "an <action>", commandsOf({ &changes, &self, &steering, &controls }) },
				{ "a path's <step>", objectTimeCommands },
				{ "a <computer>'s <condition>", press },
			};
			return rules[static_cast<std::size_t>(place)];
		}

		// An action that runs another (<trigger>) must not come round to
		// itself again, or one press would never end.
		void checkTriggerLoop(const std::vector<Object>& objects, const std::string& objectName, const std::string& actionName,
			std::vector<std::string>& path, const std::string& where)
		{
			const std::string step = objectName + " " + actionName;
			if (std::find(path.begin(), path.end(), step) != path.end())
			{
				std::string round;
				for (const std::string& each : path) { round += each + " -> "; }
				throw std::runtime_error(where + ": its <trigger>s run actions in a circle (" + round + step + "), so a press would never end");
			}
			const Object* object = findObject(objects, objectName);
			if (!object) { return; }
			const auto found = object->action.find(actionName);
			if (found == object->action.end()) { return; }

			path.push_back(step);
			for (const Command& command : found->second)
			{
				if (const auto* trigger = std::get_if<CmdTriggerAction>(&shapeOf(command)))
				{
					checkTriggerLoop(objects, trigger->object, trigger->action, path, where);
				}
			}
			path.pop_back();
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
				throw std::runtime_error(where + ": " + tag + " does nothing in " + rule.what + ", so it would be ignored." + hint
					+ " What can go there: " + listOf(rule.tags));
			}
		}

		// Every state, object and action a command names has to exist. Found
		// here, when the game loads, rather than the first time the command
		// runs: pressing a key bound to <push state="pasued" /> would otherwise
		// stop the game in the middle of play (and used to read past the end of
		// the list of states).
		// The variable a command names: owner.variable, the variable after the
		// last dot (the owner may be a path), or, where `ownNumber`, a text
		// object of that whole name counting its own <number> (<inc>, <dec>).
		void checkVariable(const std::string& target, const std::string& tag, const std::vector<Object>& objects, const std::string& where, bool ownNumber)
		{
			const bool whole = ownNumber && std::any_of(objects.begin(), objects.end(), [&](const Object& object) { return object.name == target; });
			const auto dot = whole ? std::string::npos : target.rfind('.');
			if (dot == std::string::npos && !ownNumber)
			{
				throw std::runtime_error(where + ": " + tag + ": '" + target + "' is no variable; name one as owner.variable (player.score)");
			}
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

		void checkCommand(const Command& command, const std::vector<State>& states, const std::vector<Object>& objects,
			const std::vector<SoundDesc>& sounds, const std::map<std::string, Path>& paths, const std::string& where)
		{
			if (std::holds_alternative<CmdEnterScene>(shapeOf(command)) || std::holds_alternative<CmdResetScene>(shapeOf(command)))
			{
				const bool enter = std::holds_alternative<CmdEnterScene>(shapeOf(command));
				const std::string& scene = enter ? std::get<CmdEnterScene>(shapeOf(command)).scene : std::get<CmdResetScene>(shapeOf(command)).scene;
				const std::string& way = enter ? std::get<CmdEnterScene>(shapeOf(command)).way : std::get<CmdResetScene>(shapeOf(command)).way;
				std::vector<std::string> names;
				std::vector<std::string> ways;
				for (const State& state : states)
				{
					for (const Scene& each : state.scenes)
					{
						names.push_back(each.name);
						for (const auto& entry : each.ways)
						{
							if (std::find(ways.begin(), ways.end(), entry.first) == ways.end()) { ways.push_back(entry.first); }
						}
					}
				}
				if (!way.empty())
				{
					// Which scene's way it is, is known only when it runs; one
					// that no scene has can never lead anywhere.
					if (std::find(ways.begin(), ways.end(), way) == ways.end())
					{
						throw std::runtime_error(where + ": <" + (enter ? "enter" : "reset") + " way=\"" + way + "\" /> names no way of any scene" + didYouMean(way, ways)
							+ (ways.empty() ? std::string(" (no scene has <ways>)") : ". The ways are: " + listOf(ways)));
					}
				}
				else if (std::find(names.begin(), names.end(), scene) == names.end())
				{
					throw std::runtime_error(where + ": <" + (enter ? "enter" : "reset") + " scene=\"" + scene + "\" /> names no scene of the game" + didYouMean(scene, names)
						+ (names.empty() ? std::string(" (no state has <scenes>)") : ". The scenes are: " + listOf(names)));
				}
			}
			else if (const auto* follow = std::get_if<CmdFollow>(&shapeOf(command)))
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
			else if (const auto* along = std::get_if<CmdTrack>(&shapeOf(command)); along && !along->path.empty())
			{
				const auto path = paths.find(along->path);
				if (path == paths.end())
				{
					std::vector<std::string> names;
					for (const auto& entry : paths) { names.push_back(entry.first); }
					throw std::runtime_error(where + ": <track path=\"" + along->path + "\" /> names no path of the game" + didYouMean(along->path, names));
				}
				if (!path->second.hasStart)
				{
					throw std::runtime_error(where + ": <track path=\"" + along->path + "\" />: the path needs a <start>, where 0 is");
				}
			}
			else if (const auto* become = std::get_if<CmdBecome>(&shapeOf(command)))
			{
				if (!become->from.empty() && become->target.empty())
				{
					throw std::runtime_error(where + ": <become sprite=\"" + become->sprite + "\" from=\"" + become->from + "\" /> picks which of several change, so it needs an object=\"...\"");
				}
				if (!become->target.empty())
				{
					bool any = false;
					for (const Object& object : objects)
					{
						const bool named = object.name == become->target
							|| object.inGroup(become->target);
						if (!named) { continue; }
						any = true;
						if (std::none_of(object.looks.begin(), object.looks.end(), [&](const Object::Look& look) { return look.name == become->sprite; }))
						{
							throw std::runtime_error(where + ": <become sprite=\"" + become->sprite + "\" object=\"" + become->target + "\" />: '" + object.name + "' has no look of that name");
						}
						if (!become->from.empty() && std::none_of(object.looks.begin(), object.looks.end(), [&](const Object::Look& look) { return look.name == become->from; }))
						{
							throw std::runtime_error(where + ": <become from=\"" + become->from + "\" object=\"" + become->target + "\" />: '" + object.name + "' has no look of that name");
						}
					}
					if (!any)
					{
						throw std::runtime_error(where + ": <become> names '" + become->target + "', and there is no object of that name" + didYouMean(become->target, objectNames(objects)));
					}
				}
			}
			else if (const auto* hunt = std::get_if<CmdHunt>(&shapeOf(command)))
			{
				for (const auto& [name, what] : { std::pair<const std::string*, const char*>{ &hunt->target, "<hunt>" },
					{ &hunt->partner, "<hunt> > <with>" }, { &hunt->retreatTo, "<hunt> > <retreat>" } })
				{
					if (!name->empty() && !findObject(objects, *name))
					{
						throw std::runtime_error(where + ": " + what + " names '" + *name + "', and there is no object of that name" + didYouMean(*name, objectNames(objects)));
					}
				}
			}
			else if (const auto* huntHop = std::get_if<CmdHuntHop>(&shapeOf(command)))
			{
				if (!findObject(objects, huntHop->target))
				{
					throw std::runtime_error(where + ": <hunt> names '" + huntHop->target + "', and there is no object of that name" + didYouMean(huntHop->target, objectNames(objects)));
				}
			}
			else if (const auto* chase = std::get_if<CmdChase>(&shapeOf(command)))
			{
				if (!findObject(objects, chase->target))
				{
					throw std::runtime_error(where + ": <chase> names '" + chase->target + "', and there is no object of that name" + didYouMean(chase->target, objectNames(objects)));
				}
			}
			else if (const auto* aim = std::get_if<CmdAim>(&shapeOf(command)))
			{
				if (!findObject(objects, aim->target))
				{
					throw std::runtime_error(where + ": <aim> names '" + aim->target + "', and there is no object of that name" + didYouMean(aim->target, objectNames(objects)));
				}
			}
			else if (const auto* reveal = std::get_if<CmdReveal>(&shapeOf(command)))
			{
				if (!findObject(objects, reveal->target))
				{
					throw std::runtime_error(where + ": <reveal> names '" + reveal->target + "', and there is no object of that name" + didYouMean(reveal->target, objectNames(objects)));
				}
			}
			else if (std::holds_alternative<CmdPlay>(shapeOf(command)) || std::holds_alternative<CmdSilence>(shapeOf(command)))
			{
				const auto* play = std::get_if<CmdPlay>(&shapeOf(command));
				const std::string& name = play ? play->sound : std::get<CmdSilence>(shapeOf(command)).sound;
				const bool known = std::any_of(sounds.begin(), sounds.end(), [&](const SoundDesc& sound) { return sound.name == name; });
				if (!known)
				{
					std::vector<std::string> names;
					for (const SoundDesc& sound : sounds) { names.push_back(sound.name); }
					throw std::runtime_error(where + ": <" + (play ? "play" : "silence") + " sound=\"" + name + "\" /> names no sound of the game" + didYouMean(name, names));
				}
			}
			else if (const auto* push = std::get_if<CmdPushState>(&shapeOf(command)))
			{
				const bool known = std::any_of(states.begin(), states.end(), [&](const State& state) { return state.name == push->name; });
				if (!known)
				{
					throw std::runtime_error(where + ": <push state=\"" + push->name + "\" /> names no state of the game" + didYouMean(push->name, stateNames(states)));
				}
			}
			else if (const auto* turns = std::get_if<CmdTurns>(&shapeOf(command)))
			{
				for (const std::string& player : turns->players)
				{
					const bool known = player.empty() || std::any_of(states.begin(), states.end(), [&](const State& state) { return state.name == player; });
					if (!known)
					{
						throw std::runtime_error(where + ": <turns> > <player state=\"" + player + "\" /> names no state of the game" + didYouMean(player, stateNames(states)));
					}
				}
			}
			else if (const auto* pop = std::get_if<CmdPopState>(&shapeOf(command)); pop && !pop->name.empty())
			{
				const bool known = std::any_of(states.begin(), states.end(), [&](const State& state) { return state.name == pop->name; });
				if (!known)
				{
					throw std::runtime_error(where + ": <pop state=\"" + pop->name + "\" /> names no state of the game" + didYouMean(pop->name, stateNames(states)));
				}
			}
			else if (const auto* trigger = std::get_if<CmdTriggerAction>(&shapeOf(command)))
			{
				if (trigger->object.empty())
				{
					throw std::runtime_error(where + ": <trigger action=\"" + trigger->action + "\" /> needs object=\"...\" here; only an object's own rules, actions and timers can leave it out");
				}
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
			else if (const auto* fire = std::get_if<CmdFire>(&shapeOf(command)))
			{
				if (!findObject(objects, fire->projectileName))
				{
					throw std::runtime_error(where + ": <fire> names '" + fire->projectileName + "', and there is no object of that name" + didYouMean(fire->projectileName, objectNames(objects)));
				}
			}
			else if (const auto* release = std::get_if<CmdRelease>(&shapeOf(command)))
			{
				if (!findObject(objects, release->target))
				{
					throw std::runtime_error(where + ": <release> names '" + release->target + "', and there is no object of that name" + didYouMean(release->target, objectNames(objects)));
				}
			}
			else if (const auto* reset = std::get_if<CmdResetObject>(&shapeOf(command)))
			{
				if (!findObject(objects, reset->target))
				{
					throw std::runtime_error(where + ": <reset object=\"" + reset->target + "\" /> names no object of the game" + didYouMean(reset->target, objectNames(objects)));
				}
			}
			else if (std::holds_alternative<CmdIncrement>(shapeOf(command)) || std::holds_alternative<CmdDecrement>(shapeOf(command)))
			{
				// A variable belongs to an object: paddle1.score is paddle1's
				// <variable name="score">. A bare name is a text object counting
				// its own <number>.
				const bool inc = std::holds_alternative<CmdIncrement>(shapeOf(command));
				const std::string& target = inc ? std::get<CmdIncrement>(shapeOf(command)).target : std::get<CmdDecrement>(shapeOf(command)).target;
				checkVariable(target, std::string(inc ? "<inc" : "<dec") + " variable=\"" + target + "\" />", objects, where, true);
			}
			else if (const auto* set = std::get_if<CmdSet>(&shapeOf(command)))
			{
				// A variable of an object's, or one of the numbers every
				// object has (where it is, how it is going, its heading).
				const std::string tag = "<set variable=\"" + set->target + "\">";
				const std::string part = liveNumberPart(set->target);
				const Object* holder = part.empty() ? nullptr : findObject(objects, set->target.substr(0, set->target.size() - part.size() - 1));
				if (part.empty() || (holder && holder->variable.count(part)))
				{
					checkVariable(set->target, tag, objects, where, false);
				}
				else
				{
					const std::string owner = set->target.substr(0, set->target.size() - part.size() - 1);
					const Object* found = findObject(objects, owner);
					if (!found)
					{
						throw std::runtime_error(where + ": " + tag + ": there is no object named '" + owner + "'" + didYouMean(owner, objectNames(objects)));
					}
					if (part == "heading" && !found->hasHeading)
					{
						throw std::runtime_error(where + ": " + tag + ": '" + owner + "' has no <heading> to set");
					}
					if ((part == "altitude" || part == "rise") && !found->hasAltitude)
					{
						throw std::runtime_error(where + ": " + tag + ": '" + owner + "' has no <altitude> to set");
					}
				}
			}
			else if (const auto* despawn = std::get_if<CmdDespawn>(&shapeOf(command)); despawn && !despawn->target.empty())
			{
				if (!findObject(objects, despawn->target))
				{
					throw std::runtime_error(where + ": <despawn> names '" + despawn->target + "', and there is no object or group of that name" + didYouMean(despawn->target, objectNames(objects)));
				}
			}
			else if (const auto* spawn = std::get_if<CmdSpawn>(&shapeOf(command)))
			{
				if (!findObject(objects, spawn->target))
				{
					throw std::runtime_error(where + ": <spawn> names '" + spawn->target + "', and there is no object or group of that name" + didYouMean(spawn->target, objectNames(objects)));
				}
			}
			else if (const auto* copy = std::get_if<CmdCopy>(&shapeOf(command)))
			{
				// Both are owner.variable: a copy reads one object's variable
				// and writes another's.
				const std::string tag = "<copy variable=\"" + copy->from + "\" into=\"" + copy->into + "\" />";
				checkVariable(copy->from, tag, objects, where, false);
				checkVariable(copy->into, tag, objects, where, false);
			}
			else if (const auto* rank = std::get_if<CmdRank>(&shapeOf(command)))
			{
				// The entry's variable, and a group whose every member has one
				// of that name to be ordered by.
				const std::string tag = "<rank variable=\"" + rank->entry + "\" into=\"" + rank->table + "\" />";
				checkVariable(rank->entry, tag, objects, where, false);
				const std::string variableName = rank->entry.substr(rank->entry.rfind('.') + 1);
				bool any = false;
				for (const Object& object : objects)
				{
					if (!object.inGroup(rank->table)) { continue; }
					any = true;
					if (!object.variable.count(variableName))
					{
						throw std::runtime_error(where + ": " + tag + ": '" + object.name + "' has no variable named '" + variableName + "' to be ranked by");
					}
				}
				if (!any)
				{
					throw std::runtime_error(where + ": " + tag + ": into= names no <group> of the game" + didYouMean(rank->table, objectNames(objects)));
				}
			}
		}

		void checkReferences(const std::vector<State>& states, const std::vector<Object>& objects, const std::vector<SoundDesc>& sounds,
			const std::map<std::string, Path>& paths)
		{
			// A <tether> joins two objects the game has, and a <pull> and a
			// <seek> name ones it has.
			for (const Object& object : objects)
			{
				for (const auto& [name, strength] : object.pulls)
				{
					if (!findObject(objects, name))
					{
						throw std::runtime_error("object '" + object.name + "' > <pull>: there is no object named '" + name + "'" + didYouMean(name, objectNames(objects)));
					}
				}
				if (!object.shadowOf.empty() && !findObject(objects, object.shadowOf))
				{
					throw std::runtime_error("object '" + object.name + "' > <shadow>: there is no object named '" + object.shadowOf + "'" + didYouMean(object.shadowOf, objectNames(objects)));
				}
				if (!object.seekTarget.empty() && !findObject(objects, object.seekTarget))
				{
					throw std::runtime_error("object '" + object.name + "' > <seek>: there is no object named '" + object.seekTarget + "'" + didYouMean(object.seekTarget, objectNames(objects)));
				}
				for (const std::string* end : { &object.tetherFrom, &object.tetherTo })
				{
					if (!end->empty() && !findObject(objects, *end))
					{
						throw std::runtime_error("object '" + object.name + "' > <tether>: there is no object named '" + *end + "'" + didYouMean(*end, objectNames(objects)));
					}
				}
			}

			// A state's <music> names one of the game's sounds.
			for (const State& state : states)
			{
				if (!state.music.empty() && std::none_of(sounds.begin(), sounds.end(), [&](const SoundDesc& sound) { return sound.name == state.music; }))
				{
					std::vector<std::string> names;
					for (const SoundDesc& sound : sounds) { names.push_back(sound.name); }
					throw std::runtime_error("state '" + state.name + "': <music sound=\"" + state.music + "\" /> names no sound of the game" + didYouMean(state.music, names));
				}
			}

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
					const auto* follow = std::get_if<CmdFollow>(&shapeOf(command));
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
					const auto* become = std::get_if<CmdBecome>(&shapeOf(command));
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
				for (const auto& [axis, commands] : state.axes)
				{
					const std::string here = where + " > <input axis=\"" + axisToString(axis) + "\">";
					checkAll(commands, here);
					for (const Command& command : commands)
					{
						if (!std::holds_alternative<CmdTriggerAction>(shapeOf(command)))
						{
							throw std::runtime_error(here + ": " + tagOf(command) + " does nothing on a control that points; it runs an object's <action> with <trigger object=\"...\" action=\"...\" />, and a <track /> there follows it");
						}
					}
				}
				for (const auto& condition : state.conditions)
				{
					const std::string here = where + " > <condition>";
					checkAll(condition.commands, here); checkOwnLooks(condition.commands, nullptr, here); checkOwnFollow(condition.commands, here); checkPlace(condition.commands, Place::Condition, here);
				}
				for (const auto& condition : state.computer)
				{
					checkPlace(condition.commands, Place::Computer, where + " > <computer> > <condition>");
				}
				for (const auto& timer : state.timers)
				{
					const std::string here = where + " > <timer>";
					checkAll(timer.commands, here); checkOwnLooks(timer.commands, nullptr, here); checkOwnFollow(timer.commands, here); checkPlace(timer.commands, Place::StateTimer, here);
				}
				for (const auto& scene : state.scenes)
				{
					const std::string sceneWhere = "scene '" + scene.name + "'";
					for (const auto& condition : scene.conditions)
					{
						const std::string here = sceneWhere + " > <condition>";
						checkAll(condition.commands, here); checkOwnLooks(condition.commands, nullptr, here); checkOwnFollow(condition.commands, here); checkPlace(condition.commands, Place::Condition, here);
					}
					for (const auto& timer : scene.timers)
					{
						const std::string here = sceneWhere + " > <timer>";
						checkAll(timer.commands, here); checkOwnLooks(timer.commands, nullptr, here); checkOwnFollow(timer.commands, here); checkPlace(timer.commands, Place::StateTimer, here);
					}
				}
				for (const auto& still : state.showStates)
				{
					if (std::none_of(states.begin(), states.end(), [&](const State& other) { return other.name == still; }))
					{
						throw std::runtime_error(where + " > <show state=\"" + still + "\" />: there is no state of that name" + didYouMean(still, stateNames(states)));
					}
				}
			}

			for (const auto& object : objects)
			{
				const std::string where = "object '" + object.name + "'";

				// A walker walks a map the game has; only a walker goes,
				// hunts or wanders.
				if (object.walksMaze() && std::none_of(objects.begin(), objects.end(), [&](const Object& other) { return other.mapName == object.mazeMap; }))
				{
					std::vector<std::string> maps;
					for (const Object& other : objects)
					{
						if (!other.mapName.empty() && std::find(maps.begin(), maps.end(), other.mapName) == maps.end()) { maps.push_back(other.mapName); }
					}
					throw std::runtime_error(where + " > <maze map=\"" + object.mazeMap + "\">: the game has no <map> of that name" + didYouMean(object.mazeMap, maps)
						+ (maps.empty() ? std::string(" (it has no <map>s)") : ". Its maps are: " + listOf(maps)));
				}
				const auto checkWalks = [&](const std::vector<Command>& commands, const std::string& here)
				{
					for (const Command& command : commands)
					{
						if ((std::holds_alternative<CmdGo>(shapeOf(command)) || std::holds_alternative<CmdHunt>(shapeOf(command)) || std::holds_alternative<CmdWander>(shapeOf(command))) && !object.walksMaze())
						{
							throw std::runtime_error(here + ": " + tagOf(command) + " steers an object through a maze, and this one walks none: give it a <maze map=\"...\">");
						}
					}
				};
				for (const auto& timer : object.timers) { checkWalks(timer.commands, where + " > <timer>"); }
				for (const auto& [name, commands] : object.action) { checkWalks(commands, where + " > <action name=\"" + name + "\">"); }
				for (const auto& rule : object.collisionData.others) { checkWalks(rule.commands, where + " > <collision>"); }

				// A hole is cut in a picture that stays as it is: one of
				// <line>s or a <bitmap>, not turned, animated or swapped.
				for (const auto& rule : object.collisionData.others)
				{
					for (const Command& command : rule.commands)
					{
						if (!std::holds_alternative<CmdHole>(shapeOf(command))) { continue; }
						if (!object.bitmap || object.hasHeading || !object.animationBitmaps.empty() || !object.looks.empty() || object.mirrors)
						{
							throw std::runtime_error(where + " > <collision>: <hole> cuts a picture that stays as it is - a sprite of <line>s or a <bitmap>, with no <heading>, <animation>, <mirror> or second <sprite>");
						}
					}
				}
				for (const auto* edge : { &object.collisionData.top, &object.collisionData.bottom, &object.collisionData.left, &object.collisionData.right })
				{
					checkWalks(*edge, where + " > <collision edge>");
				}

				for (const auto& timer : object.timers)
				{
					checkAll(timer.commands, where + " > <timer>"); checkOwnLooks(timer.commands, &object, where + " > <timer>");
					checkPlace(timer.commands, Place::ObjectTimer, where + " > <timer>");
				}
				for (const auto& [name, commands] : object.action) { checkOwnLooks(commands, &object, where); }
				for (const auto& rule : object.collisionData.others) { checkOwnLooks(rule.commands, &object, where); }
				for (const auto* edge : { &object.collisionData.top, &object.collisionData.bottom, &object.collisionData.left, &object.collisionData.right })
				{
					checkOwnLooks(*edge, &object, where);
				}
				for (const auto& [name, commands] : object.action)
				{
					checkAll(commands, where);
					checkPlace(commands, Place::Action, where + " > <action name=\"" + name + "\">");
					std::vector<std::string> path;
					checkTriggerLoop(objects, object.name, name, path, where + " > <action name=\"" + name + "\">");

					// Turning and thrust along the heading mean nothing to an object
					// that does not face any way.
					for (const auto& command : commands)
					{
						if ((std::holds_alternative<CmdTurn>(shapeOf(command)) || std::holds_alternative<CmdThrust>(shapeOf(command))) && !object.hasHeading)
						{
							throw std::runtime_error(where + ": action '" + name + "' turns it or thrusts along its heading, and it has no <heading>");
						}

						// A leap rises against the object's own pull, and only
						// from the ground, which a <land /> rule gives it.
						if (std::holds_alternative<CmdLeap>(shapeOf(command)) && !(object.acceleration.y > 0.0f))
						{
							throw std::runtime_error(where + ": action '" + name + "' has a <leap>, and nothing pulls the object down to come back:"
								" give it an <acceleration> with a <y> above 0, and a <collision> with <land /> to stand on");
						}
						if (std::holds_alternative<CmdLeap>(shapeOf(command)) && std::none_of(object.collisionData.others.begin(), object.collisionData.others.end(),
							[](const CollisionRule& rule) { return std::any_of(rule.commands.begin(), rule.commands.end(), [](const Command& c) { return std::holds_alternative<CmdLand>(shapeOf(c)); }); }))
						{
							throw std::runtime_error(where + ": action '" + name + "' has a <leap>, and the object never stands on anything to leap from:"
								" give it a <collision> with <land /> (<collision class=\"girder\"><land /></collision>)");
						}
						if (const auto* climb = std::get_if<CmdClimb>(&shapeOf(command)))
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
				for (const auto& rule : object.collisionData.others)
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
		seedRandom();

		checkColor(windowDesc.background, "<window> > <background>");

		// add constants to symbol table (pi, for an angle in a value, among them)
		symbolTable.add_constants();
		symbolTable.add_constant("window.top", 0);
		symbolTable.add_constant("window.bottom", windowDesc.height);
		symbolTable.add_constant("window.left", 0);
		symbolTable.add_constant("window.right", windowDesc.width);
		symbolTable.add_constant("window.width.center", windowDesc.width / 2);
		symbolTable.add_constant("window.height.center", windowDesc.height / 2);

		// add variables from XML file to symbol table, in the order the file
		// declares them so that one can use the ones before it
		for (const auto& [name, rawValue] : rawVariables)
		{
			const float value = evaluate(rawValue, "variable '" + name + "'");
			variables[name] = value;
			symbolTable.add_constant(name, value);
		}

		// An object's named timer is a number of the object's (bomb.fuse), its
		// time left, so a text shows it and a rule or a value reads it as it
		// does any variable; its value is given when the object is made.
		for (auto& rawObject : rawObjects)
		{
			for (const RawTimer& timer : rawObject.timers)
			{
				if (timer.name.empty()) { continue; }
				const std::string here = "object '" + rawObject.name + "' > <timer name=\"" + timer.name + "\">";
				if (rawObject.variable.count(timer.name))
				{
					throw std::runtime_error(here + ": the object has a <variable> of that name too, and " + rawObject.name + "." + timer.name + " is the timer's time left");
				}
				const auto& parts = liveNumberParts();
				if (std::find(parts.begin(), parts.end(), timer.name) != parts.end())
				{
					throw std::runtime_error(here + ": " + rawObject.name + "." + timer.name + " is already where the object is or how it moves; give the timer another name");
				}
				rawObject.variable[timer.name] = RawValue::expression("0");
			}
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
			bind(objectVariable.first, objectVariable.second);
		}

		// The ones that draw no random number are worked out now as well, in
		// file order, so a sprite measured below can use them (Pong's ball is
		// a <circle> of radius ball.radius, its own variable); without this it
		// read 0 and was drawn as nothing. One that reads what is not known
		// yet (a size) keeps 0 here and gets its value in the object loop.
		for (auto& rawObject : rawObjects)
		{
			for (auto& rawVariable : rawObject.variable)
			{
				if (rawVariable.second.drawsRandom())
				{
					continue;
				}
				try
				{
					objectVariables[rawObject.name + "." + rawVariable.first] = evaluate(rawVariable.second, "object '" + rawObject.name + "'");
				}
				catch (const std::runtime_error&)
				{
					// worked out, or reported, in the object loop
				}
			}
		}

		// Where each object is and how it is going, and the touch a rule is
		// answering, as names; worked out when they are used (Game::evaluateNow).
		for (const RawObject& rawObject : rawObjects)
		{
			for (const std::string& part : liveNumberParts())
			{
				const std::string name = rawObject.name + "." + part;
				if (!objectVariables.count(name) && !liveNumbers.count(name)) { liveNumbers[name] = 0.0f; }
			}
		}
		for (const std::string& name : touchNumberNames()) { liveNumbers[name] = 0.0f; }
		for (auto& [name, number] : liveNumbers) { bind(name, number); }

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
				rawObject.hasHeading ? &firstPassTurned[rawObject.name] : nullptr, &rawObject);
			const ShapeKind kind = shapeKindFromTag(params.empty() ? std::string{} : params.at(0));
			objectShapeKinds[rawObject.name] = kind;
			objectSizes[rawObject.name] = measureShapeSize(params, kind);
			firstPassSpriteParams[rawObject.name] = params;

			if (rawObject.hasAnimation)
			{
				firstPassAnimation[rawObject.name] = buildAnimationBitmaps(rawObject.animation.frames, "object '" + rawObject.name + "' > <animation>",
					rawObject.hasHeading ? &firstPassAnimationTurned[rawObject.name] : nullptr);
			}
		}
		for (auto& [name, size] : objectSizes)
		{
			if (!objectVariables.count(name + ".width")) { bind(name + ".width", size.x); }
			if (!objectVariables.count(name + ".height")) { bind(name + ".height", size.y); }
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

			FirstPass first;
			first.params = firstPassSpriteParams.at(rawObject.name);
			first.bitmap = firstPassBitmaps[rawObject.name];
			first.turned = firstPassTurned[rawObject.name];
			first.animation = firstPassAnimation[rawObject.name];
			first.animationTurned = firstPassAnimationTurned[rawObject.name];
			const auto offset = cellOffsets.find(rawObject.name);
			objects.push_back(makeObject(rawObject, first, windowDesc, offset == cellOffsets.end() ? Vector2f{} : offset->second, thisLockstep));


			if (rawObject.rawCollisionData.lockstep && rawObject.groupName.empty()) {
				lockstepNum++;
			}
		}

		// A condition made ready, from a state's or a scene's.
		const auto matches = [&](const RawCondition& rawCondition, const Object& object)
		{
			return (rawCondition.filterClass.empty() || rawCondition.filterClass == object.objClass)
				&& (rawCondition.filterObject.empty() || rawCondition.filterObject == object.name || object.inGroup(rawCondition.filterObject));
		};

		// What a condition or a <check> tests, made ready (with its <all> or
		// <any>'s checks, each made the same way).
		std::function<Condition(const RawCondition&, const std::string&, const std::string&)> makeTest;
		makeTest = [&](const RawCondition& rawCondition, const std::string& where, const std::string& owner)
		{
			Condition condition;
			condition.join = rawCondition.join;
			for (std::size_t i = 0; i < rawCondition.parts.size(); ++i)
			{
				const RawCondition& part = rawCondition.parts[i];
				const char* kind = part.join == RawCondition::Join::All ? "<all>" : part.join == RawCondition::Join::Any ? "<any>" : "<check>";
				condition.parts.push_back(makeTest(part, where + " > " + kind + " " + std::to_string(i + 1), owner));
			}
			if (condition.join != RawCondition::Join::None)
			{
				return condition;
			}

			condition.filterClass = rawCondition.filterClass;
			condition.filterObject = rawCondition.filterObject;
			condition.variableName = rawCondition.variableName;
			condition.distanceTo = rawCondition.distance;
			if (rawCondition.total == "count") { condition.total = Total::Count; }
			else if (rawCondition.total == "sum") { condition.total = Total::Sum; }
			else if (rawCondition.total == "highest") { condition.total = Total::Highest; }
			else if (rawCondition.total == "lowest") { condition.total = Total::Lowest; }

			condition.otherObject = rawCondition.otherObject;
			condition.otherVariable = rawCondition.otherVariable;
			const float threshold = rawCondition.otherVariable.empty() ? evaluate(rawCondition.threshold, where) : 0.0f;
			switch (rawCondition.test)
			{
			case RawCondition::Test::AtLeast:   condition.compare = Compare::AtLeast; break;
			case RawCondition::Test::AtMost:    condition.compare = Compare::AtMost; break;
			case RawCondition::Test::Above:     condition.compare = Compare::Above; break;
			case RawCondition::Test::Below:     condition.compare = Compare::Below; break;
			case RawCondition::Test::Equal:     condition.compare = Compare::Equal; break;
			case RawCondition::Test::Remaining: condition.remaining = threshold; break;
			}
			condition.value = threshold;

			if (!condition.otherVariable.empty())
			{
				checkOtherVariable(condition, objects, where + " > <condition variable=\"" + condition.variableName + "\">");
			}

			if (!condition.distanceTo.empty() && !findObject(objects, condition.distanceTo))
			{
				throw std::runtime_error(where + ": distance=\"" + condition.distanceTo + "\" names no object or group" + didYouMean(condition.distanceTo, objectNames(objects)));
			}

			// A total of a variable: some object it can count has it.
			if ((condition.total == Total::Sum || condition.total == Total::Highest || condition.total == Total::Lowest)
				&& std::none_of(objects.begin(), objects.end(), [&](const Object& object) { return matches(rawCondition, object) && numberOf(object, condition.variableName); }))
			{
				throw std::runtime_error(where + ": total=\"" + rawCondition.total + "\" of variable=\"" + condition.variableName + "\", and no object it looks at has a variable of that name");
			}

			if ((condition.remaining || condition.total == Total::Count) && std::none_of(objects.begin(), objects.end(), [&](const Object& object) { return matches(rawCondition, object); }))
			{
				std::cout << "warning: " << owner << (condition.remaining ? ": a condition with remaining= matches no object at all, so it would fire at once\n"
					: ": a condition with total=\"count\" matches no object at all, so it counts 0\n");
			}
			return condition;
		};

		const auto makeCondition = [&](const RawCondition& rawCondition, const std::string& where, const std::string& owner)
		{
			Condition condition = makeTest(rawCondition, where, owner);
			condition.commands = processCommands(rawCondition.commands, where);
			return condition;
		};

		// The checks of the objects' collision rules, now that every object
		// they can name is made.
		for (const RawObject& rawObject : rawObjects)
		{
			const auto object = std::find_if(objects.begin(), objects.end(), [&](const Object& each) { return each.name == rawObject.name; });
			if (object == objects.end()) { continue; }
			const std::string where = "object '" + rawObject.name + "' > <collision>";
			for (std::size_t i = 0; i < rawObject.rawCollisionData.others.size(); ++i)
			{
				const RawCondition& check = rawObject.rawCollisionData.others[i].check;
				if (check.join != RawCondition::Join::None)
				{
					object->collisionData.others[i].check = makeTest(check, where, where);
				}
			}
		}

		for (auto& rawState : rawStates)
		{
			State state{};
			const std::string where = "state '" + rawState.name + "'";

			state.name = rawState.name;
			state.show = rawState.show;
			state.showStates = rawState.showStates;
			state.music = rawState.music;

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

			for (auto& [name, commands] : rawState.axes)
			{
				const Axis axis = axisFromString(name);
				if (axis == Axis::Count)
				{
					throw std::runtime_error(where + " > <input axis=\"" + name + "\">: there is no control named '" + name + "'"
						+ didYouMean(name, axisNames()) + ". The controls are: " + listOf(axisNames()));
				}
				state.axes[axis] = processCommands(commands, where);
			}

			for (auto& rawCondition : rawState.conditions)
			{
				state.conditions.push_back(makeCondition(rawCondition, where, where));
			}
			for (auto& rawCondition : rawState.computer)
			{
				state.computer.push_back(makeCondition(rawCondition, where + " > <computer>", where));
			}

			state.timers = processTimers(rawState.timers, where);

			for (const RawScene& rawScene : rawState.scenes)
			{
				Scene scene;
				scene.name = rawState.name + "." + rawScene.name;
				const std::string sceneWhere = "scene '" + scene.name + "'";
				for (const auto& [way, to] : rawScene.ways)
				{
					// A scene of the same state, by its name there or by its path.
					const auto leads = std::find_if(rawState.scenes.begin(), rawState.scenes.end(), [&](const RawScene& other)
						{
							return other.name == to || rawState.name + "." + other.name == to;
						});
					if (leads == rawState.scenes.end())
					{
						std::vector<std::string> names;
						for (const RawScene& other : rawState.scenes) { names.push_back(other.name); }
						throw std::runtime_error(sceneWhere + ": <way name=\"" + way + "\" scene=\"" + to + "\" /> names no scene of state '" + rawState.name + "'"
							+ didYouMean(to, names) + ". Its scenes are: " + listOf(names));
					}
					scene.ways[way] = rawState.name + "." + leads->name;
				}
				for (const RawCondition& rawCondition : rawScene.conditions)
				{
					scene.conditions.push_back(makeCondition(rawCondition, sceneWhere, sceneWhere));
				}
				scene.timers = processTimers(rawScene.timers, sceneWhere);
				state.scenes.push_back(std::move(scene));
			}

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

	Object game_expr::makeObject(RawObject& rawObject, const FirstPass& first, const WindowDesc& windowDesc, Vector2f cellOffset, int thisLockstep)
	{
		const std::string where = "object '" + rawObject.name + "'";

		// Only a text showing a number needs working out again here, for the
		// value it has by now (see above).
		std::vector<std::string> tempSpriteParams = (rawObject.sprite.kind == "text" && showsNumbers(rawObject.sprite))
			? buildSpriteParams(rawObject.sprite, where, nullptr, nullptr, &rawObject)
			: first.params;
		const ShapeKind rawObjectShapeKind = shapeKindFromTag(tempSpriteParams.empty() ? std::string{} : tempSpriteParams.at(0));

		// A position that uses the size of a text or image (its own, or
		// another object's) can only be finished once a backend has measured
		// it - see Object::positionUsesSize. Sizes of shapes are exact
		// already (objectSizes above).
		const std::vector<std::string> positionSizeDependencies = sizeDependenciesOf(rawObject);

		// How long each picture of an animation lasts.
		const int animationFrames = rawObject.hasAnimation ? animationFramesOf(rawObject.animation.interval, windowDesc, where + " > <animation>") : 0;

		// The object's looks, if it has several sprites and no animation.
		std::vector<Object::Look> looks;
		if (first.looks)
		{
			looks = *first.looks;
		}
		else if (!rawObject.looks.empty())
		{
			if (rawObject.hasHeading) { throw std::runtime_error(where + ": an object with a <heading> cannot have several looks"); }

			for (const RawSprite& rawLook : rawObject.looks)
			{
				Object::Look look;
				look.name = rawLook.name;
				look.spriteParams = buildSpriteParams(rawLook, where + " > <sprite name=\"" + rawLook.name + "\">", &look.bitmap);
				if (!rawLook.frames.empty())
				{
					// A look that moves: its pictures, all one size, and how long each shows.
					const std::string here = where + " > <animation name=\"" + rawLook.name + "\">";
					look.frames = buildAnimationBitmaps(rawLook.frames, here);
					look.frameTicks = animationFramesOf(rawLook.interval, windowDesc, here);
				}
				look.shapeKind = shapeKindFromTag(look.spriteParams.empty() ? std::string{} : look.spriteParams.at(0));
				if (look.shapeKind == ShapeKind::Text && showsNumbers(rawLook))
				{
					throw std::runtime_error(where + ": a look cannot be a text showing a <number> or a <letter>");
				}
				looks.push_back(std::move(look));
			}

			// The first look is the sprite the object starts with; it must be
			// the very same picture.
			looks.front().spriteParams = tempSpriteParams;
			looks.front().bitmap = first.bitmap;
			if (!looks.front().frames.empty()) { looks.front().frames.front() = first.bitmap; }
		}


		Object object{};

		object.spriteParams = tempSpriteParams;
		object.shapeKind = rawObjectShapeKind;
		object.bitmap = first.bitmap;
		if (first.turned) { object.turnables = { first.turned }; }

		if (rawObject.hasAnimation)
		{
			object.animationBitmaps = first.animation;
			if (!first.animationTurned.empty()) { object.turnables = first.animationTurned; }
			object.animationFrames = animationFrames;
			object.bitmap = object.animationBitmaps.front();
		}

		if (object.shapeKind == ShapeKind::Text && showsNumbers(rawObject.sprite))
		{
			std::vector<TextPiece> pieces = textPieces(rawObject.sprite, where, &rawObject);
			if (std::any_of(pieces.begin(), pieces.end(), [](const TextPiece& piece) { return !piece.owner.empty(); }))
			{
				object.textPieces = std::move(pieces);
			}
		}

		object.groupName = rawObject.groupName;
		object.outerGroups = rawObject.outerGroups;
		object.name = rawObject.name;
		object.objClass = rawObject.objClass;
		object.isDrawn = rawObject.isDrawn;
		object.ownerState = rawObject.ownerState;
		object.ownerScene = rawObject.ownerScene;
		object.ownerSceneOriginal = rawObject.ownerScene;

		if (rawObject.objClass == "projectile")
		{
			rawObject.isVisible = false;
		}

		object.isVisible = rawObject.isVisible;
		object.exists = rawObject.exists;
		object.existsOriginal = rawObject.exists;
		// One that does not exist is in nothing that looks for what is out
		// there, as one taken out of play is not.
		if (!object.exists) { object.isVisible = false; }
		object.most = rawObject.most;

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
		object.cellOffset = cellOffset;
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

		if (rawObject.hasMass)
		{
			object.mass = evaluate(rawObject.rawMass, where);
			if (!(object.mass > 0.0f))
			{
				throw std::runtime_error(where + ": <mass> is " + formatDisplayNumber(object.mass) + "; expected more than 0");
			}
		}

		for (const auto& [name, strength] : rawObject.rawPulls)
		{
			object.pulls.emplace_back(name, evaluate(strength, where + " > <pull object=\"" + name + "\">"));
		}
		if (!rawObject.seekTarget.empty())
		{
			object.seekTarget = rawObject.seekTarget;
			object.seekRate = evaluate(rawObject.rawSeek, where + " > <seek>");
			if (object.seekRate < 0.0f) { throw std::runtime_error(where + ": <seek> turns " + formatDisplayNumber(object.seekRate) + " degrees a frame; expected 0 or more"); }
		}

		if (rawObject.hasLight)
		{
			object.light = evaluate(rawObject.rawLight, where + " > <light>");
			if (object.light < 0.0f) { throw std::runtime_error(where + ": <light> reaches " + formatDisplayNumber(object.light) + " pixels; expected 0 or more"); }
			object.lightOriginal = object.light;
		}
		if (rawObject.dark == "unlit") { object.dark = Object::Darkness::Unlit; }
		else if (rawObject.dark == "unseen") { object.dark = Object::Darkness::Unseen; }
		object.inDark = object.dark != Object::Darkness::None;

		if (rawObject.hasAltitude)
		{
			const std::string here = where + " > <altitude>";
			const auto given = [&](const RawValue& value) { return !(value.kind == RawValue::Kind::Expression && value.text.empty()); };
			object.hasAltitude = true;
			object.altitude = given(rawObject.rawAltitude) ? evaluate(rawObject.rawAltitude, here) : 0.0f;
			object.rise = given(rawObject.rawRise) ? evaluate(rawObject.rawRise, here) : 0.0f;
			object.fall = given(rawObject.rawFall) ? evaluate(rawObject.rawFall, here) : 0.0f;
			object.altitudeBounce = given(rawObject.rawAltitudeBounce) ? evaluate(rawObject.rawAltitudeBounce, here) : 0.0f;
			if (object.altitude < 0.0f) { throw std::runtime_error(here + ": <start> is " + formatDisplayNumber(object.altitude) + "; it cannot start below the ground"); }
			if (object.fall < 0.0f) { throw std::runtime_error(here + ": <fall> is " + formatDisplayNumber(object.fall) + "; expected 0 or more"); }
			if (!(object.altitudeBounce >= 0.0f && object.altitudeBounce <= 1.0f)) { throw std::runtime_error(here + ": <bounce> is " + formatDisplayNumber(object.altitudeBounce) + "; expected 0 up to 1"); }
			object.altitudeOriginal = object.altitude;
			object.riseOriginal = object.rise;
		}
		object.shadowOf = rawObject.shadowOf;

		if (rawObject.sprite.kind == "tether")
		{
			if (rawObject.hasHeading || !rawObject.looks.empty() || rawObject.hasAnimation)
			{
				throw std::runtime_error(where + ": a <tether> is drawn where its ends are; it has no <heading>, other looks or <animation>");
			}
			object.tetherFrom = rawObject.sprite.tetherFrom;
			object.tetherTo = rawObject.sprite.tetherTo;
			if (!rawObject.sprite.thickness.text.empty() || rawObject.sprite.thickness.kind != RawValue::Kind::Expression)
			{
				object.tetherThickness = static_cast<int>(std::lround(evaluate(rawObject.sprite.thickness, where)));
				if (object.tetherThickness < 1) { throw std::runtime_error(where + ": a <tether> has a <thickness> under 1"); }
			}
			object.tetherColor = colorFromName(rawObject.sprite.color.empty() ? "color.white" : rawObject.sprite.color);
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
		for (const Timer& timer : object.timers)
		{
			if (timer.name.empty()) { continue; }
			// A named timer's time left (see init): where it starts.
			const float left = std::ceil(timer.seconds);
			object.variable[timer.name] = left;
			object.variableOriginal[timer.name] = left;
			objectVariables[rawObject.name + "." + timer.name] = left;
		}
		object.looks = looks;
		if (!looks.empty() && !looks.front().frames.empty())
		{
			// It starts as a look that moves.
			object.animationBitmaps = looks.front().frames;
			object.animationFrames = looks.front().frameTicks;
		}

		// The look for each way it moves; one left out is another's.
		if (!rawObject.poses.empty())
		{
			const auto lookOf = [&](const std::string& pose) -> std::optional<std::size_t>
			{
				const auto named = rawObject.poses.find(pose);
				if (named == rawObject.poses.end()) { return std::nullopt; }
				for (std::size_t i = 0; i < looks.size(); ++i)
				{
					if (looks[i].name == named->second) { return i; }
				}
				std::vector<std::string> names;
				for (const Object::Look& look : looks) { names.push_back(look.name); }
				throw std::runtime_error(where + " > <poses> > <" + pose + " sprite=\"" + named->second + "\">: the object has no look of that name"
					+ (names.empty() ? std::string(" (it has no looks)") : didYouMean(named->second, names) + ". Its looks are: " + listOf(names)));
			};
			const std::size_t standing = *lookOf("standing");
			const std::size_t walking = lookOf("walking").value_or(standing);
			const std::size_t falling = lookOf("falling").value_or(standing);
			object.poseLooks[static_cast<std::size_t>(Object::Pose::Standing)] = standing;
			object.poseLooks[static_cast<std::size_t>(Object::Pose::Walking)] = walking;
			object.poseLooks[static_cast<std::size_t>(Object::Pose::Skidding)] = lookOf("skidding").value_or(walking);
			object.poseLooks[static_cast<std::size_t>(Object::Pose::Falling)] = falling;
			object.poseLooks[static_cast<std::size_t>(Object::Pose::Rising)] = lookOf("rising").value_or(falling);
			object.hasPoses = true;
			object.firstLook = standing;
			if (standing != 0) { object.showLook(standing); }
		}

		if (!rawObject.mirror.empty())
		{
			if (rawObject.hasHeading) { throw std::runtime_error(where + ": <mirror> flips a picture that does not turn; it has a <heading>"); }
			object.mirrors = true;
			object.mirrorWay = rawObject.mirror == "right" ? Direction::Right : Direction::Left;
			object.goingLeft = object.facing == Direction::Left;
			object.flips = flipsOf(object);
			object.faceMirror();
		}

		if (rawObject.hasGrip)
		{
			object.grip = evaluate(rawObject.rawGrip, where + " > <grip>");
			if (!(object.grip > 0.0f && object.grip <= 1.0f))
			{
				throw std::runtime_error(where + " > <grip> is " + formatDisplayNumber(object.grip) + "; expected above 0, up to 1 (1: it goes at once at the speed its moves ask for)");
			}
		}
		for (const Timer& timer : object.timers)
		{
			if (!timer.sprite.empty() && std::none_of(looks.begin(), looks.end(), [&](const Object::Look& look) { return look.name == timer.sprite; }))
			{
				std::vector<std::string> names;
				for (const Object::Look& look : looks) { names.push_back(look.name); }
				throw std::runtime_error(where + " > <timer sprite=\"" + timer.sprite + "\">: the object has no look of that name"
					+ (names.empty() ? std::string(" (it has no looks: several named <sprite>s and no <animation>)") : didYouMean(timer.sprite, names) + ". Its looks are: " + listOf(names)));
			}
		}

		// On a map: which tile, and the map itself, for what walks it.
		if (rawObject.cell.mapColumn > 0)
		{
			object.mapName = rawObject.cell.map;
			object.mapColumn = rawObject.cell.mapColumn - 1;
			object.mapRow = rawObject.cell.mapRow - 1;
			object.mapColumns = rawObject.cell.mapColumns;
			object.mapRows = rawObject.cell.mapRows;
			object.mapTile = { evaluate(rawObject.cell.tileWidth, where), evaluate(rawObject.cell.tileHeight, where) };
			object.mapOrigin = object.positionOriginal - object.cellOffset;
		}
		object.mazeMap = rawObject.maze.map;
		object.mazeWalls = rawObject.maze.walls;
		for (const MazeWall& wall : object.mazeWalls)
		{
			if (!wall.sprite.empty() && std::none_of(looks.begin(), looks.end(), [&](const Object::Look& look) { return look.name == wall.sprite; }))
			{
				throw std::runtime_error(where + " > <maze> > <wall class=\"" + wall.objClass + "\" sprite=\"" + wall.sprite + "\">: the object has no look of that name");
			}
		}

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

		// Which of its pixels touch: only for a picture of rows, tested
		// pixel by pixel.
		if (!rawObject.rawCollisionData.solid.empty())
		{
			if (object.collisionData.type != CollisionType::Pixel)
			{
				throw std::runtime_error(where + ": a <solid char> says which pixels touch, so it needs <type>pixel</type>");
			}
			const auto rows = [](const RawSprite& sprite) { return sprite.kind == "bitmap"; };
			if (!rows(rawObject.sprite) || !std::all_of(rawObject.looks.begin(), rawObject.looks.end(), rows)
				|| !std::all_of(rawObject.animation.frames.begin(), rawObject.animation.frames.end(), rows))
			{
				throw std::runtime_error(where + ": a <solid char> names characters of a <bitmap>, and every sprite of the object must be one");
			}
		}

		// What it touches with, if not its picture.
		const RawCollisionData& rawCollisions = rawObject.rawCollisionData;
		if (!rawCollisions.hitboxKind.empty())
		{
			if (object.collisionData.type == CollisionType::Pixel)
			{
				throw std::runtime_error(where + ": a <hitbox> is touched as its own shape, not by the pixels of the picture; it cannot go with <type>pixel</type>");
			}
			const auto valueOr0 = [&](const RawValue& value) { return value.kind == RawValue::Kind::Expression && value.text.empty() ? 0.0f : evaluate(value, where + " > <hitbox>"); };
			object.collisionData.hasHitbox = true;
			object.collisionData.hitboxRound = rawCollisions.hitboxKind == "circle";
			if (object.collisionData.hitboxRound)
			{
				const float radius = evaluate(rawCollisions.hitboxRadius, where + " > <hitbox>");
				object.collisionData.hitboxSize = { radius * 2.0f, radius * 2.0f };
			}
			else
			{
				object.collisionData.hitboxSize = { evaluate(rawCollisions.hitboxWidth, where + " > <hitbox>"), evaluate(rawCollisions.hitboxHeight, where + " > <hitbox>") };
			}
			if (!(object.collisionData.hitboxSize.x > 0.0f) || !(object.collisionData.hitboxSize.y > 0.0f))
			{
				throw std::runtime_error(where + ": a <hitbox> must be bigger than nothing; its size is " + formatDisplayNumber(object.collisionData.hitboxSize.x)
					+ " by " + formatDisplayNumber(object.collisionData.hitboxSize.y));
			}
			object.collisionData.hitboxOffset = { valueOr0(rawCollisions.hitboxX), valueOr0(rawCollisions.hitboxY) };
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

		for (auto& rawRule : rawObject.rawCollisionData.others)
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
			const std::string& side = rawRule.side;
			if (side == "top" || side == "vertical") { rule.sides.push_back(Edge::Top); }
			if (side == "bottom" || side == "vertical") { rule.sides.push_back(Edge::Bottom); }
			if (side == "left" || side == "horizontal") { rule.sides.push_back(Edge::Left); }
			if (side == "right" || side == "horizontal") { rule.sides.push_back(Edge::Right); }
			rule.commands = processCommands(rawRule.commands, where);
			object.collisionData.others.push_back(std::move(rule));
		}

		//object.action = rawObject.action;
		for (auto& rawAction : rawObject.action)
		{
			object.action[rawAction.first] = processCommands(rawAction.second, where);
		}


		// object's visual is built later by whichever Window backend is
		// running, once one exists (see Window::init() in window/window.h) -
		// visualDirty starts true (Object's own default), so nothing
		// needs to happen here.
		return object;
	}

	Object game_expr::spawnObject(RawObject& raw, const Object& from, const WindowDesc& windowDesc)
	{
		// What its rules and others' read of it, under its own name, as init
		// makes them known for every object the file has. A name used
		// before (a copy despawned, its name taken again) keeps its place.
		for (const auto& [variable, value] : raw.variable)
		{
			const std::string name = raw.name + "." + variable;
			if (objectVariables.emplace(name, 0.0f).second) { bind(name, objectVariables[name]); }
		}
		for (const std::string& part : liveNumberParts())
		{
			const std::string name = raw.name + "." + part;
			if (!objectVariables.count(name) && liveNumbers.emplace(name, 0.0f).second) { bind(name, liveNumbers[name]); }
		}
		if (const auto size = objectSizes.find(from.name); size != objectSizes.end() && !objectSizes.count(raw.name))
		{
			Vector2f& own = objectSizes[raw.name] = size->second;
			objectShapeKinds[raw.name] = objectShapeKinds[from.name];
			if (!objectVariables.count(raw.name + ".width")) { bind(raw.name + ".width", own.x); }
			if (!objectVariables.count(raw.name + ".height")) { bind(raw.name + ".height", own.y); }
		}

		// Its pictures are the ones `from` was made with, not drawn again.
		FirstPass first;
		first.params = from.looks.empty() ? from.spriteParams : from.looks.front().spriteParams;
		first.bitmap = from.looks.empty() ? from.bitmap : from.looks.front().bitmap;
		if (from.animationBitmaps.empty())
		{
			if (!from.turnables.empty()) { first.turned = from.turnables.front(); }
		}
		else
		{
			first.animation = from.animationBitmaps;
			first.animationTurned = from.turnables;
		}
		if (!from.looks.empty()) { first.looks = &from.looks; }

		return makeObject(raw, first, windowDesc, from.cellOffset, from.collisionData.lockstep);
	}

	SoundDesc game_expr::processSound(const RawSound& raw)
	{
		const std::string where = "sound '" + raw.name + "'";

		SoundDesc sound;
		sound.name = raw.name;
		sound.loop = raw.loop;

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
			note.part = rawNote.part;
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
			return randomBetween(min, max);
		}

		case RawValue::Kind::Formula:
			return evaluateOperation(value.operations->front(), where + " > <formula>").value;

		case RawValue::Kind::Centered:
			throw std::runtime_error(where + ": <centered> is only for an object's <position>");

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

	game_expr::Answer game_expr::evaluateOperation(const RawOperation& operation, const std::string& where)
	{
		const std::string here = where + " > <" + operation.op + ">";

		const auto operandAnswer = [&](const RawOperand& operand)
		{
			if (operand.value.kind == RawValue::Kind::Expression)
			{
				return Answer{ evaluate(operand.value, here), isLate(operand.value.text) };
			}

			// An operation nested in a formula is part of the same formula.
			if (operand.value.kind == RawValue::Kind::Formula) { return evaluateOperation(operand.value.operations->front(), here); }

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
		// Each text is compiled once: a value worked out while the game runs
		// (a timer starting over, a command made again each time it runs)
		// only reads the compiled one again. The symbols it reads are bound
		// by reference, so it always sees the numbers as they are now.
		auto found = compiled.find(text);
		if (found == compiled.end())
		{
			// Only the names it reads go into the symbol table, the first
			// time any text reads them (see bind).
			for (const std::string& name : namesIn(text))
			{
				if (const auto waiting = unbound.find(name); waiting != unbound.end())
				{
					symbolTable.add_variable(name, *waiting->second);
					unbound.erase(waiting);
				}
			}

			exprtk::expression<float> made;
			made.register_symbol_table(symbolTable);
			if (!parser.compile(text, made))
			{
				throw std::runtime_error(where + ": cannot read \"" + text + "\": " + parser.error().c_str());
			}
			found = compiled.emplace(text, made).first;
		}

		// Where things are has no meaning until the game runs.
		if (loading && !commandValues && !liveNumbers.empty() && !loadCheckedTexts.count(text))
		{
			for (const std::string& name : namesIn(text))
			{
				if (liveNumbers.count(name))
				{
					throw std::runtime_error(where + ": \"" + text + "\" reads " + name + ", which is only known while the game runs;"
						" a command's value can read it, worked out when the command runs");
				}
			}
			loadCheckedTexts.insert(text);
		}
		return found->second.value();
	}

	void game_expr::bind(const std::string& name, float& value)
	{
		std::string key = name;
		std::transform(key.begin(), key.end(), key.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
		unbound.emplace(std::move(key), &value);
	}

	const std::vector<std::string>& game_expr::namesIn(const std::string& text) const
	{
		auto found = namesInText.find(text);
		if (found == namesInText.end())
		{
			// Read by hand rather than by exprtk::collect_variables, which
			// builds a whole parser each time: every word that is not a
			// number (paddle1.score, sin), without case, as exprtk sees it.
			// A function's or keyword's name among them names nothing a game
			// can (a variable may not be named like one).
			std::vector<std::string> names;
			const auto letter = [](char c) { return std::isalpha(static_cast<unsigned char>(c)) != 0 || c == '_'; };
			const auto digit = [](char c) { return std::isdigit(static_cast<unsigned char>(c)) != 0; };
			for (std::size_t at = 0; at < text.size();)
			{
				if (letter(text[at]))
				{
					std::string name;
					while (at < text.size() && (letter(text[at]) || digit(text[at]) || text[at] == '.'))
					{
						name += static_cast<char>(std::tolower(static_cast<unsigned char>(text[at])));
						++at;
					}
					if (std::find(names.begin(), names.end(), name) == names.end()) { names.push_back(std::move(name)); }
				}
				else if (digit(text[at]) || text[at] == '.')
				{
					// A number, its exponent too (2e-3).
					while (at < text.size() && (digit(text[at]) || text[at] == '.')) { ++at; }
					if (at + 1 < text.size() && (text[at] == 'e' || text[at] == 'E')
						&& (digit(text[at + 1]) || ((text[at + 1] == '+' || text[at + 1] == '-') && at + 2 < text.size() && digit(text[at + 2]))))
					{
						at += 2;
						while (at < text.size() && digit(text[at])) { ++at; }
					}
				}
				else
				{
					++at;
				}
			}
			found = namesInText.emplace(text, std::move(names)).first;
		}
		return found->second;
	}

	bool game_expr::namesLive(const RawValue& value) const
	{
		for (const std::string* text : value.expressions())
		{
			if (text->empty()) { continue; }
			for (const std::string& name : namesIn(*text))
			{
				if (objectVariables.count(name) || liveNumbers.count(name)) { return true; }
			}
		}
		return false;
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
			// (A <random> in it is checked with the generator put back after, so that
			// checking does not use up the numbers the game will draw.)
			const std::mt19937 drawn = randomState();
			commandValues = true;
			const float seconds = evaluate(rawTimer.interval, here);
			commandValues = false;
			setRandomState(drawn);
			char* end = nullptr;
			const std::string& text = rawTimer.interval.text;
			const bool plainNumber = rawTimer.interval.kind == RawValue::Kind::Expression && !text.empty()
				&& (std::strtof(text.c_str(), &end), end == text.c_str() + text.size());
			// An <after> of 0 goes off on the first frame it counts (a ghost
			// told what to do the moment it turns blue); an <every> of 0 would
			// be every frame, which a frame-counting timer cannot say.
			if (plainNumber && (rawTimer.repeat ? !(seconds > 0.0f) : seconds < 0.0f))
			{
				throw std::runtime_error(here + ": " + (rawTimer.repeat ? "<every> is " + std::to_string(seconds) + "; expected a number of seconds above 0"
					: "<after> is " + std::to_string(seconds) + "; expected a number of seconds, 0 or more"));
			}
			timer.sprite = rawTimer.sprite;
			timer.name = rawTimer.name;
			timer.seconds = seconds;

			timers.push_back(std::move(timer));
		}

		return timers;
	}

	std::vector<Command> game_expr::processCommands(const std::vector<RawCommand>& raw, const std::string& where)
	{
		std::vector<Command> commands;
		commandValues = true;
		try
		{
			for (const RawCommand& each : raw)
			{
				Command made = makeCommand(each, [&](const RawValue& value) { return evaluate(value, where); });

				// One whose numbers can change is made again each time it runs
				// (CmdLive); <set> and <spawn> always are, since what they
				// put somewhere is the point of them.
				const bool live = each.verb == "set" || each.verb == "spawn"
					|| namesLive(each.amount) || namesLive(each.seconds) || namesLive(each.distance) || namesLive(each.ahead);
				if (live)
				{
					auto loaded = std::make_shared<LoadedCommand>();
					loaded->command = std::move(made);
					commands.push_back(CmdLive{ std::make_shared<const RawCommand>(each), std::move(loaded), where });
				}
				else
				{
					commands.push_back(std::move(made));
				}
			}
		}
		catch (const std::runtime_error& error)
		{
			commandValues = false;
			throw std::runtime_error(where + ": " + error.what());
		}
		commandValues = false;
		return commands;
	}

	std::vector<TextPiece> game_expr::textPieces(const RawSprite& sprite, const std::string& where, const RawObject* self)
	{
		std::vector<TextPiece> pieces;
		for (const RawTextPiece& raw : sprite.text)
		{
			TextPiece piece;
			piece.kind = raw.kind;
			if (raw.kind == RawTextPiece::Kind::Words)
			{
				piece.shown = raw.words;
				pieces.push_back(std::move(piece));
				continue;
			}

			if (raw.value.kind == RawValue::Kind::Expression)
			{
				if (const auto reference = parseVariableReference(raw.value.text))
				{
					std::string owner = reference->first;
					if (self && self->inGroup(owner) && objectVariables.count(self->name + "." + reference->second))
					{
						owner = self->name;
					}
					if (const auto found = objectVariables.find(owner + "." + reference->second); found != objectVariables.end())
					{
						piece.owner = owner;
						piece.variable = reference->second;
						piece.choices = raw.choices;
						piece.shown = showPiece(raw.kind, found->second, piece.choices);
						pieces.push_back(std::move(piece));
						continue;
					}
				}
			}

			piece.choices = raw.choices;
			piece.shown = showPiece(raw.kind, evaluate(raw.value, where), piece.choices);
			pieces.push_back(std::move(piece));
		}
		return pieces;
	}

	std::vector<std::string> game_expr::buildSpriteParams(const RawSprite& sprite, const std::string& where,
		std::shared_ptr<const Bitmap>* bitmap, std::shared_ptr<const Turnable>* turnable, const RawObject* self)
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

					// 2. Drawn by lunasvg (picture/svg.cpp), once for every sprite that
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
					// colors (a group's cells).
					std::map<char, Color> inks;
					std::string key = "rows|" + std::to_string(rowsScale) + "|" + color + "|" + sprite.flip;
					for (const auto& [character, ink] : sprite.bitmapColors)
					{
						checkColor(ink, where + " > <color char=\"" + std::string(1, character) + "\">");
						inks[character] = colorFromName(ink);
						key += std::string("|") + character + "=" + ink;
					}
					for (const std::string& row : sprite.bitmapRows) { key += "|" + row; }
					if (!sprite.solid.empty()) { key += "|solid=" + sprite.solid; }
					drawn = drawOnce(key, [&] { return flipped(rasterizeRows(sprite.bitmapRows, rowsScale, colorFromName(color), inks, sprite.solid)); });
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
			params = circleSprite(evaluate(sprite.radius, where), color);
		}
		else if (sprite.kind == "rectangle")
		{
			const float rectangleWidth = evaluate(sprite.width, where);
			params = rectangleSprite(rectangleWidth, evaluate(sprite.height, where), color);
		}
		else if (sprite.kind == "text")
		{
			// Its pieces: fixed labels, numbers shown as the whole numbers they
			// are, with no decimal point, the way someone typing the label
			// would, and letters.
			params = { "text", joinText(textPieces(sprite, where, self)), std::to_string(evaluate(sprite.size, where)), color };
		}
		else if (sprite.kind == "image")
		{
			params = { "image", sprite.path, sprite.flip.empty() ? "color.white" : "flip." + sprite.flip };
		}
		else if (sprite.kind == "tether")
		{
			// Drawn every frame where its two ends are (drawTethers); until
			// then, one clear pixel.
			auto clear = std::make_shared<Bitmap>();
			clear->width = 1;
			clear->height = 1;
			clear->rgba.assign(4, 0);
			params = { "line", "1", "1" };
			if (bitmap) { *bitmap = std::move(clear); }
		}
		else
		{
			throw std::runtime_error(where + ": unknown shape '" + sprite.kind + "'");
		}

		return params;
	}

	std::vector<std::shared_ptr<const Bitmap>> game_expr::buildAnimationBitmaps(const std::vector<RawSprite>& frames, const std::string& where,
		std::vector<std::shared_ptr<const Turnable>>* turnables)
	{
		std::vector<std::shared_ptr<const Bitmap>> pictures;

		for (std::size_t i = 0; i < frames.size(); ++i)
		{
			const RawSprite& frame = frames[i];
			const std::string here = where + " > frame " + std::to_string(i + 1)
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

	std::shared_ptr<const std::map<const Bitmap*, std::shared_ptr<const Bitmap>>> game_expr::flipsOf(const Object& object)
	{
		auto flips = std::make_shared<std::map<const Bitmap*, std::shared_ptr<const Bitmap>>>();
		const auto add = [&](const std::shared_ptr<const Bitmap>& picture)
		{
			if (!picture || flips->count(picture.get())) { return; }
			auto made = flippedPictures.find(picture.get());
			if (made == flippedPictures.end())
			{
				auto flipped = std::make_shared<const Bitmap>(flipBitmap(*picture, true, false));
				flippedPictures[flipped.get()] = picture;
				made = flippedPictures.emplace(picture.get(), std::move(flipped)).first;
			}
			(*flips)[picture.get()] = made->second;
			(*flips)[made->second.get()] = picture;
		};
		add(object.bitmap);
		for (const auto& picture : object.animationBitmaps) { add(picture); }
		for (const Object::Look& look : object.looks)
		{
			add(look.bitmap);
			for (const auto& picture : look.frames) { add(picture); }
		}
		return flips;
	}

	int game_expr::animationFramesOf(const RawValue& interval, const WindowDesc& windowDesc, const std::string& where)
	{
		const float seconds = evaluate(interval, where + " > <interval>");

		if (!(seconds > 0.0f))
		{
			throw std::runtime_error(where + " > <interval> is " + std::to_string(seconds) + "; expected a number of seconds above 0");
		}
		if (windowDesc.framerate < 1)
		{
			throw std::runtime_error(where + " > <interval> is in seconds, so the <window> needs a <framerate> above 0 to count them in");
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

		// What a <map> placed: in the middle of its tile, counted from the
		// map's top left, which is the object's own <position>.
		for (const RawObject& rawObject : rawObjects)
		{
			if (rawObject.cell.mapColumn == 0) { continue; }
			const std::string where = "object '" + rawObject.name + "' (on map '" + rawObject.cell.map + "')";
			const Vector2f tile{ evaluate(rawObject.cell.tileWidth, where + " > <tile> > <width>"), evaluate(rawObject.cell.tileHeight, where + " > <tile> > <height>") };
			if (!(tile.x > 0.0f) || !(tile.y > 0.0f)) { throw std::runtime_error(where + ": a <tile> is " + formatDisplayNumber(tile.x) + " by " + formatDisplayNumber(tile.y) + "; both have to be above 0"); }
			const std::vector<std::string>& params = spriteParams.at(rawObject.name);
			const Vector2f own = measureShapeSize(params, shapeKindFromTag(params.empty() ? std::string{} : params.at(0)));
			offsets[rawObject.name] = { static_cast<float>(rawObject.cell.mapColumn - 1) * tile.x + (tile.x - own.x) / 2.0f,
				static_cast<float>(rawObject.cell.mapRow - 1) * tile.y + (tile.y - own.y) / 2.0f };
		}

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
