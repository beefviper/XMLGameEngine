// scenes.cpp
// XML Game Engine
// author: beefviper
// date: Oct 15, 2026

#include "game/scenes.h"

#include "load/spelling.h"

#include <algorithm>
#include <cctype>
#include <cstddef>
#include <map>
#include <memory>
#include <set>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace xge
{
	namespace
	{
		bool named(const RawObject& object, const std::string& name)
		{
			return object.name == name || object.inGroup(name);
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

		// The object and its groups named by their path from `prefix` on.
		void qualify(RawObject& object, const std::string& prefix)
		{
			object.name = prefix + object.name;
			if (!object.groupName.empty()) { object.groupName = prefix + object.groupName; }
			for (std::string& group : object.outerGroups) { group = prefix + group; }
			if (!object.cell.map.empty()) { object.cell.map = prefix + object.cell.map; }
		}

		// A whole sprite in place of the one of its name, wherever the object
		// keeps it: what it shows, its looks, the pictures of its animation,
		// and its cell's slot.
		void replaceSprite(RawObject& object, const RawSprite& sprite, const std::string& where)
		{
			bool replaced = false;
			const auto swap = [&](RawSprite& into)
			{
				if (into.name == sprite.name)
				{
					into = sprite;
					replaced = true;
				}
			};
			swap(object.sprite);
			for (RawSprite& look : object.looks) { swap(look); }
			for (RawSprite& frame : object.animation.frames) { swap(frame); }
			if (object.cell.column > 0) { swap(object.cell.slot); }

			if (!replaced)
			{
				throw std::runtime_error(where + ": '" + object.name + "' has no <sprite> "
					+ (sprite.name.empty() ? std::string("without a name") : "called \"" + sprite.name + "\"") + " to change");
			}
		}

		void applyChange(RawObject& object, const RawSceneChange& change, const std::string& where)
		{
			if (change.positionX) { object.rawPosition.x = *change.positionX; }
			if (change.positionY) { object.rawPosition.y = *change.positionY; }
			if (change.velocityX) { object.rawVelocity.x = *change.velocityX; }
			if (change.velocityY) { object.rawVelocity.y = *change.velocityY; }
			if (change.hidden) { object.isVisible = !*change.hidden; }
			if (change.drawn) { object.isDrawn = *change.drawn; }
			for (const auto& [name, value] : change.variables) { object.variable[name] = value; }
			for (const RawSprite& sprite : change.sprites) { replaceSprite(object, sprite, where); }
		}

		// The copied list with the own one's added: one named like a copied
		// one takes its place there, the rest go after, in the order written.
		template <typename Item>
		std::vector<Item> mergedByName(const std::vector<Item>& copied, const std::vector<Item>& own)
		{
			std::vector<Item> merged = copied;
			for (const Item& item : own)
			{
				const auto same = item.name.empty() ? merged.end()
					: std::find_if(merged.begin(), merged.end(), [&](const Item& each) { return each.name == item.name; });
				if (same != merged.end()) { *same = item; }
				else { merged.push_back(item); }
			}
			return merged;
		}

		// What a scene is made of, its names as written (before its path is
		// put in front), for a scene that is from= it.
		struct Made
		{
			std::vector<RawObject> objects;
			std::vector<std::pair<std::string, RawValue>> variables;
		};

		// Where a name is used: its state and scene (a path), either or both
		// empty, and what to say in a message.
		struct Context
		{
			std::string state;
			std::string scene;
			std::string where;
		};

		bool endsWith(const std::string& text, const std::string& end)
		{
			return text.size() >= end.size() && text.compare(text.size() - end.size(), end.size(), end) == 0;
		}

		// Every name in the game, written out in full, and how a name used
		// somewhere is found among them.
		class Names
		{
		public:
			std::set<std::string> objects;   // objects only
			std::set<std::string> all;       // objects and groups
			std::set<std::string> scenes;    // scene paths
			std::map<std::string, std::set<std::string>> sceneVariables;
			std::set<std::string> gameVariables; // the game's <variables>, as written (ball.radius)
			std::map<std::string, std::set<std::string>> sceneWays; // each scene's <ways>, by name

			// An object or group, from the inside out: the scene's, the state's,
			// the game's, then the one path that ends with it. Unknown is left
			// as it is (the load says so later, as for any name); `quiet`
			// leaves a name that could be two things as it is too.
			std::string object(const std::string& name, const Context& context, bool quiet = false) const
			{
				if (name.empty()) { return name; }
				if (!context.scene.empty() && all.count(context.scene + "." + name)) { return context.scene + "." + name; }
				if (!context.state.empty() && all.count(context.state + "." + name)) { return context.state + "." + name; }
				if (all.count(name)) { return name; }
				return endOfOnePath(name, all, context, quiet);
			}

			// A scene: one of the state's, a path, or the one path ending with it.
			std::string scene(const std::string& name, const Context& context) const
			{
				if (name.empty()) { return name; }
				if (!context.state.empty() && scenes.count(context.state + "." + name)) { return context.state + "." + name; }
				if (scenes.count(name)) { return name; }
				return endOfOnePath(name, scenes, context, false);
			}

			// What <inc>/<dec> variable= names: a text object counting its own
			// number, or owner.variable with the owner found as any object is.
			std::string variableTarget(const std::string& target, const Context& context) const
			{
				const std::string whole = object(target, context, true);
				if (objects.count(whole)) { return whole; }

				if (const std::string live = liveNumberPart(target); !live.empty() && live.find('.') != std::string::npos)
				{
					return object(target.substr(0, target.size() - live.size() - 1), context) + "." + live;
				}

				const auto dot = target.rfind('.');
				if (dot == std::string::npos) { return whole; }
				return object(target.substr(0, dot), context) + target.substr(dot);
			}

			// An expression with every name in it written out: owner.variable,
			// name.width and the like by their owner, and a plain name that is
			// one of its scene's variables by the scene's path.
			std::string expression(const std::string& text, const Context& context) const
			{
				std::string out;
				const auto word = [](char c) { return std::isalnum(static_cast<unsigned char>(c)) != 0 || c == '_' || c == '.'; };
				std::size_t i = 0;
				while (i < text.size())
				{
					const char c = text[i];
					if (std::isalpha(static_cast<unsigned char>(c)) != 0 || c == '_')
					{
						std::size_t end = i;
						while (end < text.size() && word(text[end])) { ++end; }
						out += token(text.substr(i, end - i), context);
						i = end;
					}
					else if (std::isdigit(static_cast<unsigned char>(c)) != 0 || c == '.')
					{
						// a number (3.5, 1e3) is passed over whole
						std::size_t end = i;
						while (end < text.size() && word(text[end])) { ++end; }
						out += text.substr(i, end - i);
						i = end;
					}
					else
					{
						out += c;
						++i;
					}
				}
				return out;
			}

		private:
			std::string token(const std::string& name, const Context& context) const
			{
				const auto dot = name.rfind('.');
				if (dot == std::string::npos)
				{
					if (!context.scene.empty())
					{
						const auto variables = sceneVariables.find(context.scene);
						if (variables != sceneVariables.end() && variables->second.count(name)) { return context.scene + "." + name; }
					}
					return name;
				}

				// A game variable written with a dot is that variable, whatever
				// object its first part might name.
				if (gameVariables.count(name)) { return name; }

				// One of the numbers every object has (ball.velocity.x): the
				// owner is all before it.
				if (const std::string live = liveNumberPart(name); !live.empty() && live.find('.') != std::string::npos)
				{
					const std::string liveOwner = object(name.substr(0, name.size() - live.size() - 1), context, true);
					if (all.count(liveOwner)) { return liveOwner + "." + live; }
				}

				const std::string owner = name.substr(0, dot);
				const std::string part = name.substr(dot);
				const std::string found = object(owner, context);
				if (all.count(found)) { return found + part; }

				// A scene's variable by its scene: wave2.pace
				const std::string scenePath = scene(owner, context);
				const auto variables = sceneVariables.find(scenePath);
				if (variables != sceneVariables.end() && variables->second.count(name.substr(dot + 1))) { return scenePath + part; }
				return name;
			}

			static std::string endOfOnePath(const std::string& name, const std::set<std::string>& among, const Context& context, bool quiet)
			{
				// The shortest path that ends with it: rock is playing.rock (the
				// rock of every scene) before playing.room1.rock.
				const std::string end = "." + name;
				std::vector<std::string> found;
				std::size_t fewest = std::string::npos;
				for (const std::string& candidate : among)
				{
					if (!endsWith(candidate, end)) { continue; }
					const auto dots = static_cast<std::size_t>(std::count(candidate.begin(), candidate.end(), '.'));
					if (dots < fewest) { found.clear(); fewest = dots; }
					if (dots == fewest) { found.push_back(candidate); }
				}
				if (found.size() == 1) { return found.front(); }
				if (found.size() > 1 && !quiet)
				{
					throw std::runtime_error(context.where + ": '" + name + "' could be " + listOf(found) + "; write the path of the one meant");
				}
				return name;
			}
		};

		class Resolver
		{
		public:
			explicit Resolver(const Names& names) : names_(names) {}

			void value(RawValue& value, const Context& context) const
			{
				switch (value.kind)
				{
				case RawValue::Kind::Expression:
				case RawValue::Kind::Centered:
					value.text = names_.expression(value.text, context);
					break;
				case RawValue::Kind::Random:
					value.min = names_.expression(value.min, context);
					value.max = names_.expression(value.max, context);
					break;
				case RawValue::Kind::Equation:
				case RawValue::Kind::Formula:
					if (value.operations)
					{
						// The steps are shared with every copy of the value, so
						// this one gets steps of its own.
						auto operations = std::make_shared<std::vector<RawOperation>>(*value.operations);
						for (RawOperation& operation : *operations)
						{
							for (RawOperand& operand : operation.operands) { this->value(operand.value, context); }
						}
						value.operations = std::move(operations);
					}
					break;
				}
			}

			void commands(std::vector<RawCommand>& commands, const Context& context) const
			{
				for (RawCommand& command : commands)
				{
					if (command.verb == "inc" || command.verb == "dec" || command.verb == "copy" || command.verb == "rank" || command.verb == "set") { command.variable = names_.variableTarget(command.variable, context); }
					if (command.verb == "copy") { command.into = names_.variableTarget(command.into, context); }
					if (command.verb == "rank") { command.into = names_.object(command.into, context); }
					command.object = names_.object(command.object, context);
					command.partner = names_.object(command.partner, context);
					command.retreat = names_.object(command.retreat, context);
					command.scene = names_.scene(command.scene, context);
					if (!command.way.empty() && !context.scene.empty())
					{
						// Inside a scene, a way it does not have can only be a mistake.
						const auto ways = names_.sceneWays.find(context.scene);
						if (ways == names_.sceneWays.end() || !ways->second.count(command.way))
						{
							const std::vector<std::string> known = ways == names_.sceneWays.end() ? std::vector<std::string>{} : std::vector<std::string>(ways->second.begin(), ways->second.end());
							throw std::runtime_error(context.where + ": way=\"" + command.way + "\" is no way of scene '" + context.scene + "'" + didYouMean(command.way, known)
								+ (known.empty() ? std::string(" (it has no <ways>)") : ". Its ways are: " + listOf(known)));
						}
					}
					value(command.amount, context);
					value(command.seconds, context);
					value(command.distance, context);
					value(command.ahead, context);
					value(command.positionX, context);
					value(command.positionY, context);
					value(command.velocityX, context);
					value(command.velocityY, context);
				}
			}

			void sprite(RawSprite& sprite, const Context& context) const
			{
				for (RawLine& line : sprite.lines)
				{
					value(line.from.x, context);
					value(line.from.y, context);
					value(line.to.x, context);
					value(line.to.y, context);
					value(line.thickness, context);
				}
				for (RawValue* part : { &sprite.scale, &sprite.svgX, &sprite.svgY, &sprite.svgWidth, &sprite.svgHeight,
					&sprite.radius, &sprite.width, &sprite.height, &sprite.size, &sprite.thickness })
				{
					value(*part, context);
				}
				for (RawTextPiece& piece : sprite.text)
				{
					value(piece.value, context);
				}
				if (!sprite.tetherFrom.empty()) { sprite.tetherFrom = names_.object(sprite.tetherFrom, context); }
				if (!sprite.tetherTo.empty()) { sprite.tetherTo = names_.object(sprite.tetherTo, context); }
			}

			void timers(std::vector<RawTimer>& timers, const Context& context) const
			{
				for (RawTimer& timer : timers)
				{
					value(timer.interval, context);
					commands(timer.commands, context);
				}
			}

			void conditions(std::vector<RawCondition>& conditions, const Context& context) const
			{
				for (RawCondition& condition : conditions)
				{
					condition.filterObject = names_.object(condition.filterObject, context);
					condition.otherObject = names_.object(condition.otherObject, context);
					condition.distance = names_.object(condition.distance, context);
					value(condition.threshold, context);
					this->conditions(condition.parts, context);
					commands(condition.commands, context);
				}
			}

			void object(RawObject& object) const
			{
				const Context context{ object.ownerState, object.ownerScene, "object '" + object.name + "'" };

				sprite(object.sprite, context);
				for (RawSprite& look : object.looks) { sprite(look, context); }
				value(object.animation.interval, context);
				for (RawSprite& frame : object.animation.frames) { sprite(frame, context); }
				for (RawValue* part : { &object.rawPosition.x, &object.rawPosition.y, &object.rawVelocity.x, &object.rawVelocity.y,
					&object.rawAcceleration.x, &object.rawAcceleration.y, &object.rawHeading, &object.rawDrag, &object.rawMass })
				{
					value(*part, context);
				}
				timers(object.timers, context);

				RawCollisionData& collisions = object.rawCollisionData;
				for (auto* edge : { &collisions.top, &collisions.bottom, &collisions.left, &collisions.right }) { commands(*edge, context); }
				for (RawCollisionRule& rule : collisions.others)
				{
					rule.filterObject = names_.object(rule.filterObject, context);
					if (rule.slower) { value(*rule.slower, context); }
					if (rule.faster) { value(*rule.faster, context); }
					conditions(rule.check.parts, context);
					commands(rule.commands, context);
				}

				for (auto& [name, list] : object.action) { commands(list, context); }
				for (auto& [name, variable] : object.variable) { value(variable, context); }

				sprite(object.cell.slot, context);
				value(object.cell.gapBefore, context);
				value(object.cell.gapAbove, context);
				value(object.cell.tileWidth, context);
				value(object.cell.tileHeight, context);
				object.maze.map = names_.object(object.maze.map, context);
				for (auto& [name, strength] : object.rawPulls)
				{
					name = names_.object(name, context);
					value(strength, context);
				}
				object.seekTarget = names_.object(object.seekTarget, context);
				value(object.rawSeek, context);
				value(object.rawLight, context);
				value(object.rawAltitude, context);
				value(object.rawRise, context);
				value(object.rawFall, context);
				value(object.rawAltitudeBounce, context);
				object.shadowOf = names_.object(object.shadowOf, context);
			}

		private:
			const Names& names_;
		};
	}

	void takeStates(std::vector<RawState>& states)
	{
		for (std::size_t index = 0; index < states.size(); ++index)
		{
			RawState& state = states[index];
			if (state.from.empty()) { continue; }

			const std::string where = "state '" + state.name + "'";
			const auto earlier = std::find_if(states.begin(), states.begin() + static_cast<std::ptrdiff_t>(index),
				[&](const RawState& each) { return each.name == state.from; });
			if (earlier == states.begin() + static_cast<std::ptrdiff_t>(index))
			{
				throw std::runtime_error(where + ": from=\"" + state.from + "\" names no state before it");
			}
			if (!earlier->kit.empty() || !earlier->scenes.empty())
			{
				throw std::runtime_error(where + ": from=\"" + state.from + "\" names a state with scenes, and a copy of one is not built yet");
			}

			RawState made = *earlier;
			made.name = state.name;
			made.from = state.from;
			made.changes.clear();

			// The copied objects with what it changes in them, then its own.
			for (const RawSceneEntry& entry : state.changes)
			{
				bool changed = false;
				for (RawObject& object : made.objects)
				{
					const bool named = object.name == entry.take || object.groupName == entry.take
						|| std::find(object.outerGroups.begin(), object.outerGroups.end(), entry.take) != object.outerGroups.end();
					if (!named) { continue; }
					applyChange(object, entry.change, where);
					changed = true;
				}
				if (!changed) { throw std::runtime_error(where + ": changes '" + entry.take + "', which state '" + state.from + "' has not got"); }
			}
			made.objects.insert(made.objects.end(), state.objects.begin(), state.objects.end());

			const auto add = [](std::vector<std::string>& to, const std::vector<std::string>& own)
			{
				for (const std::string& name : own)
				{
					if (std::find(to.begin(), to.end(), name) == to.end()) { to.push_back(name); }
				}
			};
			add(made.show, state.show);
			add(made.showStates, state.showStates);
			for (const auto& [button, commands] : state.input) { made.input[button] = commands; }
			for (const auto& [axis, commands] : state.axes) { made.axes[axis] = commands; }
			made.conditions = mergedByName(earlier->conditions, state.conditions);
			made.computer = mergedByName(earlier->computer, state.computer);
			made.timers = mergedByName(earlier->timers, state.timers);
			if (!state.music.empty()) { made.music = state.music; }
			made.kit = std::move(state.kit);
			made.scenes = std::move(state.scenes);

			state = std::move(made);
		}
	}

	void layOutScenes(GameData& data)
	{
		const bool owns = std::any_of(data.states.begin(), data.states.end(), [](const RawState& state)
			{
				return !state.objects.empty() || !state.kit.empty() || !state.scenes.empty();
			});
		if (!owns) { return; }

		Names names;
		for (const auto& variable : data.variables) { names.gameVariables.insert(variable.first); }
		const std::set<std::string> gameNames = namesOf(data.objects);

		// The game's own objects first, then each state's, with the scene
		// variables (and the scene each is read in) as they are made.
		std::vector<RawObject> all = std::move(data.objects);
		std::vector<std::pair<std::size_t, Context>> sceneVariables;

		for (RawState& state : data.states)
		{
			const std::string where = "state '" + state.name + "'";
			const bool stateOwns = !state.objects.empty() || !state.kit.empty() || !state.scenes.empty();
			if (stateOwns && state.name.find('.') != std::string::npos)
			{
				throw std::runtime_error(where + ": a state that has objects or scenes has no dots in its name: they join the parts of a path");
			}

			std::map<std::string, Made> made;
			for (std::size_t index = 0; index < state.scenes.size(); ++index)
			{
				RawScene& scene = state.scenes[index];
				const std::string sceneWhere = where + " > scene '" + scene.name + "'";
				const std::string path = state.name + "." + scene.name;
				if (made.count(scene.name)) { throw std::runtime_error(where + ": has two scenes called '" + scene.name + "'"); }

				Made local;
				if (!scene.from.empty())
				{
					const auto from = made.find(scene.from);
					if (from == made.end()) { throw std::runtime_error(sceneWhere + ": from=\"" + scene.from + "\" names no scene before it"); }
					local = from->second;

					// Its ways, conditions and timers too: its own are added,
					// each in place of the one of the same name, if there is one.
					const RawScene& earlier = *std::find_if(state.scenes.begin(), state.scenes.begin() + static_cast<std::ptrdiff_t>(index),
						[&](const RawScene& each) { return each.name == scene.from; });
					std::vector<std::pair<std::string, std::string>> ways = earlier.ways;
					for (const auto& way : scene.ways)
					{
						const auto existing = std::find_if(ways.begin(), ways.end(), [&](const auto& each) { return each.first == way.first; });
						if (existing != ways.end()) { existing->second = way.second; }
						else { ways.push_back(way); }
					}
					scene.ways = std::move(ways);
					scene.conditions = mergedByName(earlier.conditions, scene.conditions);
					scene.timers = mergedByName(earlier.timers, scene.timers);
				}

				for (const auto& [name, value] : scene.variables)
				{
					const auto existing = std::find_if(local.variables.begin(), local.variables.end(), [&](const auto& entry) { return entry.first == name; });
					if (existing != local.variables.end()) { existing->second = value; }
					else { local.variables.emplace_back(name, value); }
				}

				for (const RawSceneEntry& entry : scene.entries)
				{
					if (entry.take.empty())
					{
						for (const RawObject& object : entry.objects)
						{
							if (std::any_of(local.objects.begin(), local.objects.end(), [&](const RawObject& other) { return other.name == object.name; }))
							{
								throw std::runtime_error(sceneWhere + ": has two objects called '" + object.name + "'");
							}
							local.objects.push_back(object);
						}
						continue;
					}

					// A piece it has already (from the scene it is from=) is
					// changed where it is; one of the kit's is copied in.
					const std::string entryWhere = sceneWhere + " > '" + entry.take + "'";
					bool changed = false;
					for (RawObject& object : local.objects)
					{
						if (named(object, entry.take))
						{
							applyChange(object, entry.change, entryWhere);
							changed = true;
						}
					}
					if (changed) { continue; }

					bool taken = false;
					for (const RawObject& piece : state.kit)
					{
						if (!named(piece, entry.take)) { continue; }
						RawObject copy = piece;
						applyChange(copy, entry.change, entryWhere);
						local.objects.push_back(std::move(copy));
						taken = true;
					}
					if (!taken)
					{
						const std::set<std::string> kitNames = namesOf(state.kit);
						throw std::runtime_error(entryWhere + ": takes '" + entry.take + "', and neither the kit of " + where + " nor the scene it is from has a piece of that name"
							+ didYouMean(entry.take, { kitNames.begin(), kitNames.end() }));
					}
				}

				names.scenes.insert(path);
				for (const auto& way : scene.ways) { names.sceneWays[path].insert(way.first); }
				for (const auto& [name, value] : local.variables)
				{
					names.sceneVariables[path].insert(name);
					sceneVariables.emplace_back(data.variables.size(), Context{ state.name, path, sceneWhere + " > <variable name=\"" + name + "\">" });
					data.variables.emplace_back(path + "." + name, value);
				}

				for (RawObject object : local.objects)
				{
					// Each name it has inside its scene is also a name in its
					// state for the same thing in every scene (playing.rock), so
					// the state's own rules can say rock and mean whichever is
					// in play.
					std::vector<std::string> acrossScenes{ state.name + "." + object.name };
					if (!object.groupName.empty()) { acrossScenes.push_back(state.name + "." + object.groupName); }
					for (const std::string& group : object.outerGroups) { acrossScenes.push_back(state.name + "." + group); }

					qualify(object, path + ".");
					object.outerGroups.insert(object.outerGroups.end(), acrossScenes.begin(), acrossScenes.end());
					object.ownerState = state.name;
					object.ownerScene = path;
					all.push_back(std::move(object));
				}
				made[scene.name] = std::move(local);
			}

			for (RawObject object : state.objects)
			{
				qualify(object, state.name + ".");
				object.ownerState = state.name;
				all.push_back(std::move(object));
			}

			// Laid out: what is left of them is the scenes' names, conditions and timers.
			state.objects.clear();
			state.kit.clear();
			for (RawScene& scene : state.scenes) { scene.entries.clear(); }
		}

		// Every name, and no name twice where both can be seen.
		std::map<std::string, std::string> objectOwners;
		for (const RawObject& object : all)
		{
			names.objects.insert(object.name);
			names.all.insert(object.name);
			if (!object.groupName.empty()) { names.all.insert(object.groupName); }
			names.all.insert(object.outerGroups.begin(), object.outerGroups.end());
			if (!objectOwners.emplace(object.name, object.ownerScene.empty() ? object.ownerState : object.ownerScene).second && !object.ownerState.empty())
			{
				throw std::runtime_error("object '" + object.name + "': there are two objects of that name");
			}
		}
		for (const RawObject& object : all)
		{
			if (object.ownerState.empty()) { continue; }

			const std::string owner = object.ownerScene.empty() ? object.ownerState : object.ownerScene;
			std::vector<std::string> own{ object.name };
			if (!object.groupName.empty()) { own.push_back(object.groupName); }
			for (const std::string& group : object.outerGroups)
			{
				if (group.compare(0, owner.size() + 1, owner + ".") == 0) { own.push_back(group); }
			}
			for (const std::string& name : own)
			{
				const std::string local = name.substr(owner.size() + 1);
				const bool inScene = !object.ownerScene.empty();
				if (inScene && std::any_of(all.begin(), all.end(), [&](const RawObject& other)
					{
						return other.ownerScene.empty() && other.ownerState == object.ownerState
							&& (other.name == object.ownerState + "." + local || other.inGroup(object.ownerState + "." + local));
					}))
				{
					throw std::runtime_error("scene '" + owner + "': its '" + local + "' and state '" + object.ownerState + "''s '" + local + "' would both be '" + local + "' inside it; give one another name");
				}
				if (gameNames.count(local))
				{
					throw std::runtime_error(std::string(inScene ? "scene '" : "state '") + owner + "': its '" + local + "' and the game's '" + local + "' would both be '" + local + "' inside it; give one another name");
				}
			}
		}
		for (const std::string& scene : names.scenes)
		{
			if (names.all.count(scene)) { throw std::runtime_error("scene '" + scene + "': an object of the state has the same name; give one another name"); }
		}

		// Every name used, written out in full where it is used.
		const Resolver resolve(names);
		for (RawObject& object : all) { resolve.object(object); }
		for (const auto& [index, context] : sceneVariables) { resolve.value(data.variables[index].second, context); }

		for (RawState& state : data.states)
		{
			const Context context{ state.name, {}, "state '" + state.name + "'" };
			for (std::string& shown : state.show) { shown = names.object(shown, context); }
			for (auto& [button, list] : state.input) { resolve.commands(list, context); }
			for (auto& [axis, list] : state.axes) { resolve.commands(list, context); }
			resolve.conditions(state.conditions, context);
			resolve.conditions(state.computer, context);
			resolve.timers(state.timers, context);
			for (RawScene& scene : state.scenes)
			{
				const std::string path = state.name + "." + scene.name;
				const Context inScene{ state.name, path, "state '" + state.name + "' > scene '" + scene.name + "'" };
				resolve.conditions(scene.conditions, inScene);
				resolve.timers(scene.timers, inScene);
			}
		}

		const Context game{ {}, {}, "the game's <paths>" };
		for (RawPath& path : data.paths)
		{
			for (RawPathStep& step : path.steps)
			{
				resolve.value(step.x, game);
				resolve.value(step.y, game);
				resolve.commands(step.commands, game);
			}
		}

		data.objects = std::move(all);
	}
}
