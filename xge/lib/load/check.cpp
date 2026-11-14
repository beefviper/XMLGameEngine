// check.cpp
// XML Game Engine
// author: beefviper
// date: Oct 29, 2026

#include "load/check.h"

#include "picture/color.h"
#include "engine/keys.h"
#include "load/reading.h"
#include "load/spelling.h"
#include "verbs/verb.h"

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
			// Each also as written in its state or scene (log, for
			// playing.log), the way a misspelt name was most likely meant.
			const auto add = [&](const std::string& name)
			{
				names.insert(name);
				for (const std::string* owner : { &object.ownerScene, &object.ownerState })
				{
					if (!owner->empty() && name.size() > owner->size() + 1 && name.compare(0, owner->size() + 1, *owner + ".") == 0)
					{
						names.insert(name.substr(owner->size() + 1));
						break;
					}
				}
			};
			add(object.name);
			if (!object.groupName.empty()) { add(object.groupName); }
			for (const std::string& group : object.outerGroups) { add(group); }
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
			if (std::holds_alternative<CmdWhen>(command)) { return "<condition>"; }
			if (const auto* fall = std::get_if<CmdFall>(&shapeOf(command)); fall && !fall->objClass.empty()) { return "<fall class=\"" + fall->objClass + "\" />"; }
			const Form* form = formOf(command);
			return form ? form->tag : std::string();
		}

		// What a place is called in a message. A command the schema lets
		// through where the engine does nothing with it is kept out by the
		// places its verb's forms name (verbs/verb.h).
		const char* nameOf(Place place)
		{
			static const char* const names[] = {
				"a screen-edge <collision>", "a <collision> with another object", "an <input>", "a <condition>", "a state's <timer>",
				"an object's <timer>", "an <action>", "a path's <step>", "a <computer>'s <condition>", "a map's check",
			};
			return names[static_cast<std::size_t>(place)];
		}

		// Every command that does something in a place, as a message lists them.
		std::vector<std::string> tagsFor(Place place)
		{
			std::vector<std::string> tags;
			for (const Verb* verb : allVerbs())
			{
				for (const Form& form : verb->forms)
				{
					if (form.tag && (form.places & in(place)) && std::find(tags.begin(), tags.end(), form.tag) == tags.end()) { tags.push_back(form.tag); }
				}
			}
			// A condition among an action's commands.
			if (place == Place::Action) { tags.push_back("<condition>"); }
			return tags;
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
				// One about the cells round it may come round: each piece runs
				// at most once a press (Game::triggerAbout).
				if (const auto* when = std::get_if<CmdWhen>(&command); when && when->body)
				{
					for (const Command& inner : when->body->commands)
					{
						const auto* nested = std::get_if<CmdTriggerAction>(&shapeOf(inner));
						if (nested && !nested->place.any()) { checkTriggerLoop(objects, nested->object, nested->action, path, where); }
					}
				}
				const auto* trigger = std::get_if<CmdTriggerAction>(&shapeOf(command));
				if (trigger && !trigger->place.any())
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
			for (const Command& command : commands)
			{
				const std::string tag = tagOf(command);
				// A condition's commands go where it is.
				if (const auto* when = std::get_if<CmdWhen>(&command); when && place == Place::Action)
				{
					if (when->body) { checkPlace(when->body->commands, place, where + " > <condition>"); }
					continue;
				}
				const Form* form = formOf(command);
				if (form && (form->places & in(place))) { continue; }

				std::string hint;
				if (place == Place::Input && (tag == "<move>" || tag == "<fire>" || tag == "<hop>" || tag == "<jump>" || tag == "<climb>"))
				{
					hint = " A key moves or fires an object through one of its <action>s: <trigger object=\"...\" action=\"...\" />.";
				}
				throw std::runtime_error(where + ": " + tag + " does nothing in " + nameOf(place) + ", so it would be ignored." + hint
					+ " What can go there: " + listOf(tagsFor(place)));
			}
		}

	}

	// `other` left in a list that is not a rule about another object.
	static std::string otherOutOfPlace(const std::string& name)
	{
		return name == "other" ? "; other is the object a <collision> rule touched, so only a rule about another object (class=, object= or neither) names it" : std::string();
	}

	void checkObject(const Checking& checking, const std::string& name, const std::string& what, const char* kind)
	{
		if (!findObject(checking.objects, name))
		{
			throw std::runtime_error(checking.where + ": " + what + " names '" + name + "', and there is no " + kind + " of that name" + didYouMean(name, objectNames(checking.objects)) + otherOutOfPlace(name));
		}
	}

	void checkState(const Checking& checking, const std::string& name, const std::string& what)
	{
		const bool known = std::any_of(checking.states.begin(), checking.states.end(), [&](const State& state) { return state.name == name; });
		if (!known)
		{
			throw std::runtime_error(checking.where + ": " + what + " names no state of the game" + didYouMean(name, stateNames(checking.states)));
		}
	}

	void checkSound(const Checking& checking, const char* verb, const std::string& name)
	{
		const bool known = std::any_of(checking.sounds.begin(), checking.sounds.end(), [&](const SoundDesc& sound) { return sound.name == name; });
		if (!known)
		{
			std::vector<std::string> names;
			for (const SoundDesc& sound : checking.sounds) { names.push_back(sound.name); }
			throw std::runtime_error(checking.where + ": <" + verb + " sound=\"" + name + "\" /> names no sound of the game" + didYouMean(name, names));
		}
	}

	void checkScene(const Checking& checking, const char* verb, const std::string& scene, const std::string& way)
	{
		std::vector<std::string> names;
		std::vector<std::string> ways;
		for (const State& state : checking.states)
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
				throw std::runtime_error(checking.where + ": <" + verb + " way=\"" + way + "\" /> names no way of any scene" + didYouMean(way, ways)
					+ (ways.empty() ? std::string(" (no scene has <ways>)") : ". The ways are: " + listOf(ways)));
			}
		}
		else if (std::find(names.begin(), names.end(), scene) == names.end())
		{
			throw std::runtime_error(checking.where + ": <" + verb + " scene=\"" + scene + "\" /> names no scene of the game" + didYouMean(scene, names)
				+ (names.empty() ? std::string(" (no state has <scenes>)") : ". The scenes are: " + listOf(names)));
		}
	}

	void checkCellMap(const Checking& checking, const std::string& map, const std::string& tag)
	{
		const bool found = std::any_of(checking.objects.begin(), checking.objects.end(), [&](const Object& object) { return object.onCells && object.mapName == map; });
		if (!found) { throw std::runtime_error(checking.where + ": " + tag + " names no map with <cells />"); }
	}

	void refuseFollowed(const Checking& checking, const VariableName& target, const std::string& tag)
	{
		const Object* owner = target.variable.empty() ? nullptr : findObject(checking.objects, target.owner);
		if (owner && owner->followsPart(target.variable))
		{
			throw std::runtime_error(checking.where + ": " + tag + ": '" + target.written() + "' is update=\"live\", worked out every frame from what it reads;"
				" no command can change it. Change what it reads, or leave out update=\"live\"");
		}
	}

	void checkVariable(const Checking& checking, const VariableName& target, const std::string& tag)
	{
		const std::vector<Object>& objects = checking.objects;
		const std::string& where = checking.where;
		if (target.variable.empty())
		{
			throw std::runtime_error(where + ": " + tag + ": '" + target.owner + "' is no variable; name one as owner.variable (player.score)"
				+ ". A variable that changes belongs to an object; the game's own <variables> are fixed numbers");
		}
		const std::string& ownerName = target.owner;
		const Object* owner = findObject(objects, ownerName);
		if (!owner)
		{
			throw std::runtime_error(where + ": " + tag + ": there is no object named '" + ownerName + "'" + didYouMean(ownerName, objectNames(objects)) + otherOutOfPlace(ownerName)
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

	void wantPalette(const Checking& checking, const Object& object, const std::string& style)
	{
		const std::vector<RawStyle>& styles = checking.styles;
		if (std::none_of(styles.begin(), styles.end(), [&](const RawStyle& each) { return each.name == style; }))
		{
			std::vector<std::string> names;
			for (const RawStyle& each : styles) { names.push_back(each.name); }
			throw std::runtime_error(checking.where + ": <become style=\"" + style + "\" /> names no <style> of the game's <styles>" + didYouMean(style, names));
		}
		std::vector<std::string>& list = checking.wanted[object.name];
		if (std::find(list.begin(), list.end(), style) == list.end()) { list.push_back(style); }
	}

	namespace
	{
		// Every state, object and action a command names has to exist: each
		// verb checks its own (Verb::check). Found here, when the game loads,
		// rather than the first time the command runs: pressing a key bound
		// to <push state="pasued" /> would otherwise stop the game in the
		// middle of play.
		void checkCommand(const Command& command, const Checking& checking)
		{
			if (const auto* when = std::get_if<CmdWhen>(&command))
			{
				if (!when->body) { return; }
				const std::string inside = checking.where + " > <condition>";
				for (const Command& inner : when->body->commands)
				{
					checkCommand(inner, { checking.states, checking.objects, checking.sounds, checking.paths, checking.styles, checking.wanted, inside });
				}
				return;
			}
			const Verb* verb = verbOf(command);
			if (verb && verb->check) { verb->check(shapeOf(command), checking); }
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
			for (const auto* keys : state.keyMaps())
			{
				for (const auto& [key, commands] : *keys) { visit(commands, nullptr); }
			}
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
			visit(object.animationThen, &object);
			for (const Object::Look& look : object.looks) { visit(look.then, &object); }
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
			onLine(object.line, [&]
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
			});
		}

		// A collision rule's class=, unless= and through= name a class that
		// some object has (a piece only ever spawned counts: it is among the
		// objects, not yet in play), and its object= an object or a group.
		// A misspelt one would never match and the rule would never run.
		std::set<std::string> classSet;
		for (const Object& object : objects)
		{
			if (!object.objClass.empty()) { classSet.insert(object.objClass); }
		}
		const std::vector<std::string> gameClasses(classSet.begin(), classSet.end());
		const auto checkClass = [&](const Object& object, const char* tag, const char* attribute, const std::string& name)
		{
			if (name.empty() || classSet.count(name)) { return; }
			throw std::runtime_error("object '" + object.name + "' > <" + tag + " " + attribute + "=\"" + name + "\">: no object has the class '" + name + "'"
				+ didYouMean(name, gameClasses) + (gameClasses.empty() ? std::string("; no object has a class") : ". The classes are: " + listOf(gameClasses)));
		};
		for (const Object& object : objects)
		{
			onLine(object.line, [&]
			{
				for (const CollisionRule& rule : object.collisionData.others)
				{
					onLine(rule.line, [&]
					{
						checkClass(object, "collision", "class", rule.filterClass);
						checkClass(object, "collision", "unless", rule.unlessClass);
						checkClass(object, "collision", "through", rule.throughClass);
						if (!rule.filterObject.empty() && !findObject(objects, rule.filterObject))
						{
							throw std::runtime_error("object '" + object.name + "' > <collision object=\"" + rule.filterObject + "\">: there is no object or group named '" + rule.filterObject + "'"
								+ didYouMean(rule.filterObject, objectNames(objects)));
						}
					});
				}
				for (const auto& edge : object.collisionData.edges)
				{
					for (const EdgeGuard& guard : edge.guards) { checkClass(object, "collision", "unless", guard.unlessClass); }
				}
				for (const std::string& past : object.lightPast) { checkClass(object, "light > <past", "class", past); }
			});
		}

		// The variables a holder, a pattern or a cell check go by are ones
		// some object has (a card's rank, a jewel's hue), or a number the
		// engine keeps; a misspelt one read nothing, and nothing ever matched.
		std::set<std::string> variableSet(liveNumberParts().begin(), liveNumberParts().end());
		for (const Object& object : objects)
		{
			for (const auto& [name, value] : object.variable) { variableSet.insert(name); }
		}
		const std::vector<std::string> gameVariables(variableSet.begin(), variableSet.end());
		const auto checkVariable = [&](const Object& object, const std::string& tag, const std::string& name)
		{
			if (name.empty() || variableSet.count(name)) { return; }
			throw std::runtime_error("object '" + object.name + "' > <" + tag + " variable=\"" + name + "\">: no object has a variable or number of that name" + didYouMean(name, gameVariables));
		};
		for (const Object& object : objects)
		{
			onLine(object.line, [&]
			{
				for (const std::string& by : object.pile.orderBy) { checkVariable(object, "by", by); }
				// A holder that counts gets a variable of that name itself
				// (game/cards.cpp), so what it adds up is another's: a member's.
				const std::string& counts = object.pile.counts;
				if (!counts.empty() && std::none_of(objects.begin(), objects.end(), [&](const Object& other) { return other.pile.counts != counts && numberOf(other, counts); }))
				{
					throw std::runtime_error("object '" + object.name + "' > <counts variable=\"" + counts + "\">: no object has a variable or number of that name" + didYouMean(counts, gameVariables));
				}
				const PileRule& takes = object.pile.takes;
				checkVariable(object, "wild", takes.wildVariable);
				for (const auto* rules : { &takes.rules, &takes.emptyRules, &takes.anyRules, &takes.wildRules })
				{
					for (const PileRule::VarRule& rule : *rules)
					{
						checkVariable(object, "condition", rule.variable);
						checkVariable(object, "condition", rule.topVariable);
					}
				}
				for (const auto& check : object.cellChecks) { checkVariable(object, "alike", check.alike); }
			});
		}

		// What a state's <shows> name is something the game has: a misspelt
		// one was never drawn.
		for (const State& state : states)
		{
			onLine(state.line, [&]
			{
				for (const std::string& shown : state.show)
				{
					if (!findObject(objects, shown))
					{
						throw std::runtime_error("state '" + state.name + "' > <show object=\"" + shown + "\" />: there is no object or group of that name" + didYouMean(shown, objectNames(objects)));
					}
				}
			});
		}

		// A state's <music> names one of the game's sounds.
		for (const State& state : states)
		{
			onLine(state.line, [&]
			{
				if (!state.music.empty() && std::none_of(sounds.begin(), sounds.end(), [&](const SoundDesc& sound) { return sound.name == state.music; }))
				{
					std::vector<std::string> names;
					for (const SoundDesc& sound : sounds) { names.push_back(sound.name); }
					throw std::runtime_error("state '" + state.name + "': <music sound=\"" + state.music + "\" /> names no sound of the game" + didYouMean(state.music, names));
				}
			});
		}

		const auto checkAll = [&](const std::vector<Command>& commands, const std::string& where)
		{
			for (const auto& command : commands)
			{
				checkCommand(command, { states, objects, sounds, paths, styles, wanted, where });
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
				if (!become->style.empty()) { wantPalette({ states, objects, sounds, paths, styles, wanted, where }, *self, become->style); }
				if (!become->sprite.empty() && std::none_of(self->looks.begin(), self->looks.end(), [&](const Object::Look& look) { return look.name == become->sprite; }))
				{
					throw std::runtime_error(where + ": <become sprite=\"" + become->sprite + "\" />: it has no look of that name (a look is one of several named <sprite>s)");
				}
			}
		};

		for (const auto& state : states)
		{
			onLine(state.line, [&]
			{
				const std::string where = "state '" + state.name + "'";
				for (std::size_t moment = 0; moment < keyMoments.size(); ++moment)
				{
					for (const auto& [key, commands] : *state.keyMaps()[moment])
					{
						const std::string here = where + " > <input button=\"" + keyCodeToString(key) + "\"" + (moment == 0 ? std::string() : std::string(" on=\"") + keyMoments[moment] + "\"") + ">";
						checkAll(commands, here); checkOwnLooks(commands, nullptr, here); checkOwnFollow(commands, here); checkPlace(commands, Place::Input, here);
					}
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
					onLine(condition.line, [&]
					{
						const std::string here = where + " > <condition>";
						checkAll(condition.commands, here); checkOwnLooks(condition.commands, nullptr, here); checkOwnFollow(condition.commands, here); checkPlace(condition.commands, Place::Condition, here);
					});
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
						onLine(condition.line, [&]
						{
							const std::string here = sceneWhere + " > <condition>";
							checkAll(condition.commands, here); checkOwnLooks(condition.commands, nullptr, here); checkOwnFollow(condition.commands, here); checkPlace(condition.commands, Place::Condition, here);
						});
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
			});
		}

		for (const auto& object : objects)
		{
			onLine(object.line, [&]
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
						// On a map of cells a <go> goes a cell at a time, with no maze.
						if (std::holds_alternative<CmdGo>(shapeOf(command)) && object.onCells) { continue; }
						if ((std::holds_alternative<CmdGo>(shapeOf(command)) || std::holds_alternative<CmdHunt>(shapeOf(command)) || std::holds_alternative<CmdWander>(shapeOf(command))) && !object.walksMaze())
						{
							throw std::runtime_error(here + ": " + tagOf(command) + " steers an object through a maze, and this one walks none: give it a <maze map=\"...\">");
						}
					}
				};
				for (const auto& timer : object.timers) { checkWalks(timer.commands, where + " > <timer>"); }
				checkWalks(object.animationThen, where + " > <animation> > <then>");
				for (const auto& look : object.looks) { checkWalks(look.then, where + " > <animation name=\"" + look.name + "\"> > <then>"); }
				for (const auto& [name, commands] : object.action) { checkWalks(commands, where + " > <action name=\"" + name + "\">"); }
				for (const auto& rule : object.collisionData.others) { onLine(rule.line, [&] { checkWalks(rule.commands, where + " > <collision>"); }); }

				// A hole is cut in a picture that stays as it is: one of
				// <line>s or a <bitmap>, not turned, animated or swapped.
				for (const auto& rule : object.collisionData.others)
				{
					onLine(rule.line, [&]
					{
						for (const Command& command : rule.commands)
						{
							if (!std::holds_alternative<CmdHole>(shapeOf(command))) { continue; }
							if (!object.bitmap || object.hasHeading || !object.animationBitmaps.empty() || !object.looks.empty() || object.mirrors)
							{
								throw std::runtime_error(where + " > <collision>: <hole> cuts a picture that stays as it is - a sprite of <line>s or <polygon>s, or a <bitmap>, with no <heading>, <animation>, <mirror> or second <sprite>");
							}
						}
					});
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
				// An animation's <then> runs as an object's timer does.
				const auto checkThen = [&](const std::vector<Command>& commands, const std::string& here)
				{
					checkAll(commands, here); checkOwnLooks(commands, &object, here);
					checkPlace(commands, Place::ObjectTimer, here);
				};
				checkThen(object.animationThen, where + " > <animation> > <then>");
				for (const auto& look : object.looks) { checkThen(look.then, where + " > <animation name=\"" + look.name + "\"> > <then>"); }
				for (const auto& [name, commands] : object.action) { checkOwnLooks(commands, &object, where); }
				for (const auto& rule : object.collisionData.others) { onLine(rule.line, [&] { checkOwnLooks(rule.commands, &object, where); }); }
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
						if (std::holds_alternative<CmdTurn>(shapeOf(command)) && object.isShape()) { continue; }
						if ((std::holds_alternative<CmdTurn>(shapeOf(command)) || std::holds_alternative<CmdThrust>(shapeOf(command)) || std::holds_alternative<CmdSwing>(shapeOf(command))) && !object.hasHeading)
						{
							throw std::runtime_error(where + ": action '" + name + "' turns it or thrusts along its heading, and it has no <heading>");
						}

						// A jump rises against the object's own pull, and only
						// from the ground, which a <land /> rule gives it.
						if (std::holds_alternative<CmdJump>(shapeOf(command)) && !(object.acceleration.y > 0.0f))
						{
							throw std::runtime_error(where + ": action '" + name + "' has a <jump>, and nothing pulls the object down to come back:"
								" give it an <acceleration> with a <y> above 0, and a <collision> with <land /> to stand on");
						}
						if (std::holds_alternative<CmdJump>(shapeOf(command)) && std::none_of(object.collisionData.others.begin(), object.collisionData.others.end(),
							[](const CollisionRule& rule) { return std::any_of(rule.commands.begin(), rule.commands.end(), [](const Command& c) { return std::holds_alternative<CmdLand>(shapeOf(c)); }); }))
						{
							throw std::runtime_error(where + ": action '" + name + "' has a <jump>, and the object never stands on anything to jump from:"
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
					onLine(rule.line, [&]
					{
						checkAll(rule.commands, where);
						checkPlace(rule.commands, Place::Touch, where + " > <collision>");
					});
				}
				for (const auto& edgeRules : object.collisionData.edges)
				{
					checkAll(edgeRules.commands, where);
					checkPlace(edgeRules.commands, Place::Edge, where + " > <collision edge>");
				}
			});
		}
		return wanted;
	}
}
