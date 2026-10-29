// check.cpp
// XML Game Engine
// author: beefviper
// date: Oct 29, 2026

#include "load/check.h"

#include "picture/color.h"
#include "engine/keys.h"
#include "load/spelling.h"

#include <algorithm>
#include <cctype>
#include <initializer_list>
#include <set>
#include <stdexcept>

namespace xge
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
				return object.named(name);
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

	namespace
	{
		std::vector<std::string> stateNames(const std::vector<State>& states)
		{
			std::vector<std::string> names;
			for (const State& state : states) { names.push_back(state.name); }
			return names;
		}

	}

	// A color is one of the color. names; anything else would draw nothing.
	void checkColor(const std::string& name, const std::string& where)
	{
		if (!isColorName(name))
		{
			throw std::runtime_error(where + ": there is no color named '" + name + "'" + didYouMean(name, colorNames()) + ". The colors are: " + listOf(colorNames()));
		}
	}

	namespace
	{
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
				[](const CmdShift&) { return std::string("<shift>"); },
				[](const CmdEnterScene&) { return std::string("<enter>"); },
				[](const CmdResetScene&) { return std::string("<reset scene>"); },
				[](const CmdSet&) { return std::string("<set>"); },
				[](const CmdSpawn&) { return std::string("<spawn>"); },
				[](const CmdDespawn& d) { return std::string(d.target.empty() ? "<despawn />" : "<despawn object>"); },
				[](const CmdLive& live) { return tagOf(live.loaded->command); },
			}, command);
		}

		// Where a list of commands runs, and what each place does something
		// with. CommandExecutor::run (play/executor.cpp) does each verb as
		// its Doer says, and does nothing with one that has no object, touch
		// or action it needs; this is what keeps such a verb out of a place.
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
			static const std::vector<std::string> changes{ "<inc>", "<dec>", "<set>", "<copy>", "<rank>", "<play>", "<silence>", "<become>", "<reveal>", "<follow>", "<shift>", "<enter>",
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
		// last dot (the owner may be a path).
		void checkVariable(const VariableName& target, const std::string& tag, const std::vector<Object>& objects, const std::string& where)
		{
			if (target.variable.empty())
			{
				throw std::runtime_error(where + ": " + tag + ": '" + target.owner + "' is no variable; name one as owner.variable (player.score)"
					+ ". A variable that changes belongs to an object; the game's own <variables> are fixed numbers");
			}
			const std::string& ownerName = target.owner;
			const Object* owner = findObject(objects, ownerName);
			if (!owner)
			{
				throw std::runtime_error(where + ": " + tag + ": there is no object named '" + ownerName + "'" + didYouMean(ownerName, objectNames(objects))
					+ ". A variable that changes belongs to an object (paddle1.score is paddle1's <variable name=\"score\">); the game's own <variables> are fixed numbers");
			}
			const std::string& variableName = target.variable;
			if (!owner->variable.count(variableName))
			{
				std::vector<std::string> names;
				for (const auto& entry : owner->variable) { names.push_back(entry.first); }
				throw std::runtime_error(where + ": " + tag + ": '" + ownerName + "' has no variable named '" + variableName + "'" + didYouMean(variableName, names)
					+ (names.empty() ? std::string("; it has no <variables>") : "; its variables are: " + listOf(names)));
			}
		}

		void wantPalette(WantedPalettes& wanted, const std::vector<RawStyle>& styles, const Object& object, const std::string& style, const std::string& where)
		{
			if (std::none_of(styles.begin(), styles.end(), [&](const RawStyle& each) { return each.name == style; }))
			{
				std::vector<std::string> names;
				for (const RawStyle& each : styles) { names.push_back(each.name); }
				throw std::runtime_error(where + ": <become style=\"" + style + "\" /> names no <style> of the game's <styles>" + didYouMean(style, names));
			}
			std::vector<std::string>& list = wanted[object.name];
			if (std::find(list.begin(), list.end(), style) == list.end()) { list.push_back(style); }
		}

		void checkCommand(const Command& command, const std::vector<State>& states, const std::vector<Object>& objects,
			const std::vector<SoundDesc>& sounds, const std::map<std::string, Path>& paths, const std::vector<RawStyle>& styles,
			WantedPalettes& wanted, const std::string& where)
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
				if (!become->target.empty())
				{
					bool any = false;
					for (const Object& object : objects)
					{
						const bool named = object.named(become->target);
						if (!named) { continue; }
						any = true;
						if (!become->style.empty()) { wantPalette(wanted, styles, object, become->style, where); }
						if (!become->sprite.empty() && std::none_of(object.looks.begin(), object.looks.end(), [&](const Object::Look& look) { return look.name == become->sprite; }))
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
			else if (const auto* shift = std::get_if<CmdShift>(&shapeOf(command)))
			{
				if (!findObject(objects, shift->target))
				{
					throw std::runtime_error(where + ": <shift> names '" + shift->target + "', and there is no object of that name" + didYouMean(shift->target, objectNames(objects)));
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
				// <variable name="score">.
				const bool inc = std::holds_alternative<CmdIncrement>(shapeOf(command));
				const VariableName& target = inc ? std::get<CmdIncrement>(shapeOf(command)).target : std::get<CmdDecrement>(shapeOf(command)).target;
				const std::string written = target.variable.empty() ? target.owner : target.written();
				checkVariable(target, std::string(inc ? "<inc" : "<dec") + " variable=\"" + written + "\" />", objects, where);
			}
			else if (const auto* set = std::get_if<CmdSet>(&shapeOf(command)))
			{
				// A variable of an object's, or one of the numbers every
				// object has (where it is, how it is going, its heading).
				const std::string tag = "<set variable=\"" + (set->target.variable.empty() ? set->target.owner : set->target.written()) + "\">";
				const std::string part = liveNumberPart(set->target.written());
				const Object* holder = part.empty() ? nullptr : findObject(objects, set->target.owner);
				if (part.empty() || (holder && holder->variable.count(part)))
				{
					checkVariable(set->target, tag, objects, where);
				}
				else
				{
					const std::string& owner = set->target.owner;
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
				const std::string tag = "<copy variable=\"" + copy->from.written() + "\" into=\"" + copy->into.written() + "\" />";
				checkVariable(copy->from, tag, objects, where);
				checkVariable(copy->into, tag, objects, where);
			}
			else if (const auto* rank = std::get_if<CmdRank>(&shapeOf(command)))
			{
				// The entry's variable, and a group whose every member has one
				// of that name to be ordered by.
				const std::string tag = "<rank variable=\"" + rank->entry.written() + "\" into=\"" + rank->table + "\" />";
				checkVariable(rank->entry, tag, objects, where);
				const std::string& variableName = rank->entry.variable;
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

	}

	// Which objects can fly each path: those a <follow> anywhere sends
	// (itself, in an object's own commands, or every one of the name or
	// group it names), and those of a path whose step sends its flier on
	// along another.
	std::map<std::string, std::set<std::string>> followersOf(const std::vector<State>& states, const std::vector<Object>& objects,
		const std::map<std::string, Path>& paths)
	{
		std::map<std::string, std::set<std::string>> followers;
		const auto visit = [&](const std::vector<Command>& commands, const Object* self)
		{
			for (const Command& command : commands)
			{
				const auto* follow = std::get_if<CmdFollow>(&shapeOf(command));
				if (!follow) { continue; }
				if (follow->target.empty())
				{
					if (self) { followers[follow->path].insert(self->name); }
					continue;
				}
				for (const Object& object : objects)
				{
					if (object.named(follow->target)) { followers[follow->path].insert(object.name); }
				}
			}
		};

		for (const State& state : states)
		{
			for (const auto& [key, commands] : state.input) { visit(commands, nullptr); }
			for (const auto& [axis, commands] : state.axes) { visit(commands, nullptr); }
			for (const auto* conditions : { &state.conditions, &state.computer })
			{
				for (const Condition& condition : *conditions) { visit(condition.commands, nullptr); }
			}
			for (const Timer& timer : state.timers) { visit(timer.commands, nullptr); }
			for (const Scene& scene : state.scenes)
			{
				for (const Condition& condition : scene.conditions) { visit(condition.commands, nullptr); }
				for (const Timer& timer : scene.timers) { visit(timer.commands, nullptr); }
			}
		}
		for (const Object& object : objects)
		{
			for (const Timer& timer : object.timers) { visit(timer.commands, &object); }
			for (const auto& [name, commands] : object.action) { visit(commands, &object); }
			for (const auto& rule : object.collisionData.others) { visit(rule.commands, &object); }
			for (const auto& edgeRules : object.collisionData.edges)
			{
				visit(edgeRules.commands, &object);
			}
		}

		// A step that sends its flier on along another path: that path's
		// fliers are this one's too, until nothing more is added.
		for (bool grew = true; grew;)
		{
			grew = false;
			for (const auto& [name, path] : paths)
			{
				for (const PathStep& step : path.steps)
				{
					for (const Command& command : step.commands)
					{
						const auto* follow = std::get_if<CmdFollow>(&shapeOf(command));
						if (!follow || !follow->target.empty() || follow->path == name || !followers.count(name)) { continue; }
						const std::set<std::string> from = followers[name];
						std::set<std::string>& to = followers[follow->path];
						const std::size_t before = to.size();
						to.insert(from.begin(), from.end());
						grew = grew || to.size() != before;
					}
				}
			}
		}
		return followers;
	}

	WantedPalettes checkReferences(const std::vector<State>& states, const std::vector<Object>& objects, const std::vector<SoundDesc>& sounds,
		const std::map<std::string, Path>& paths, const std::vector<RawStyle>& styles)
	{
		WantedPalettes wanted;

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
				checkCommand(command, states, objects, sounds, paths, styles, wanted, where);
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
				for (const auto& [follower, own] : step.ownBy)
				{
					const std::string here = "path '" + name + "' > <step>, flown by '" + follower + "'";
					checkAll(own, here);
					checkPlace(own, Place::PathStep, here);
				}
				for (const auto& command : step.commands)
				{
					const auto* become = std::get_if<CmdBecome>(&shapeOf(command));
					if (become && become->target.empty() && !become->style.empty())
					{
						throw std::runtime_error("path '" + name + "' > <step>: <become style=\"" + become->style + "\" /> needs object=\"...\" here; which palettes a path's flier has is not known");
					}
				}
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
					throw std::runtime_error(where + ": <become> needs object=\"...\" here; only an object's own rules, actions and timers can leave it out");
				}
				if (!become->style.empty()) { wantPalette(wanted, styles, *self, become->style, where); }
				if (!become->sprite.empty() && std::none_of(self->looks.begin(), self->looks.end(), [&](const Object::Look& look) { return look.name == become->sprite; }))
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
			for (const auto& edgeRules : object.collisionData.edges)
			{
				checkWalks(edgeRules.commands, where + " > <collision edge>");
			}

			for (const auto& timer : object.timers)
			{
				checkAll(timer.commands, where + " > <timer>"); checkOwnLooks(timer.commands, &object, where + " > <timer>");
				checkPlace(timer.commands, Place::ObjectTimer, where + " > <timer>");
			}
			for (const auto& [name, commands] : object.action) { checkOwnLooks(commands, &object, where); }
			for (const auto& rule : object.collisionData.others) { checkOwnLooks(rule.commands, &object, where); }
			for (const auto& edgeRules : object.collisionData.edges)
			{
				checkOwnLooks(edgeRules.commands, &object, where);
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
			for (const auto& edgeRules : object.collisionData.edges)
			{
				checkAll(edgeRules.commands, where);
				checkPlace(edgeRules.commands, Place::Edge, where + " > <collision edge>");
			}
		}
		return wanted;
	}
}
