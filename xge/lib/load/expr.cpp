// expr.cpp
// XML Game Engine
// author: beefviper
// date: Sept 21, 2020

#include "load/expr.h"

#include "load/calc.h"
#include "load/check.h"
#include "load/reading.h"

#include "picture/color.h"
#include "engine/keys.h"
#include "load/spelling.h"
#include "picture/svg.h"
#include "play/numbers.h"
#include "audio/wave.h"

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
	bool showsNumbers(const RawSprite& sprite)
	{
		return std::any_of(sprite.text.begin(), sprite.text.end(),
			[](const RawTextPiece& piece) { return piece.kind != RawTextPiece::Kind::Words; });
	}

	namespace
	{
		// Whether a condition's class= and object= pick the object.
		bool matchesFilter(const RawCondition& rawCondition, const Object& object)
		{
			return (rawCondition.filterClass.empty() || rawCondition.filterClass == object.objClass)
				&& (rawCondition.filterObject.empty() || object.named(rawCondition.filterObject));
		}

		// What a <holds class> holds: each member of a class some holder
		// holds is held (it moves from holder to holder and turns between
		// its first look and its second), worth its column unless it says,
		// and `high` a second worth it may count instead; a picker of one is
		// a hand.
		void makeHeld(std::vector<RawObject>& objects)
		{
			std::vector<std::string> held;
			for (const RawObject& object : objects)
			{
				if (!object.holder.holdsClass.empty() && std::find(held.begin(), held.end(), object.holder.holdsClass) == held.end()) { held.push_back(object.holder.holdsClass); }
			}
			if (held.empty()) { return; }
			const auto isHeld = [&](const std::string& objClass) { return !objClass.empty() && std::find(held.begin(), held.end(), objClass) != held.end(); };
			const auto number = [](const RawValue& value, const std::string& where, const char* what)
			{
				try { return std::stof(value.text); }
				catch (const std::exception&) { throw std::runtime_error(where + ": its " + what + " is \"" + value.text + "\"; expected a number"); }
			};
			for (RawObject& object : objects)
			{
				if (!isHeld(object.objClass)) { continue; }
				const std::string where = "object '" + object.name + "' (of class " + object.objClass + ", which a <holds> holds)";
				if (object.groupName.empty() || object.cell.column < 1) { throw std::runtime_error(where + ": is a member of a <group> laid out in <columns> and <rows>, its row and column what sort of card it is"); }
				if (object.looks.size() < 2) { throw std::runtime_error(where + ": has two looks, its face first and its back"); }
				object.held = true;
				const auto given = object.variable.find("worth");
				const float worth = given == object.variable.end() ? static_cast<float>(object.cell.column) : number(given->second, where, "worth");
				object.item.worths = { worth };
				if (const auto high = object.variable.find("high"); high != object.variable.end())
				{
					const float more = number(high->second, where, "high");
					if (more != worth) { object.item.worths.push_back(more); }
					std::sort(object.item.worths.begin(), object.item.worths.end());
				}
				object.variable["worth"] = RawValue::expression(formatDisplayNumber(worth));
				object.sprite = object.looks.front();
			}
			for (RawObject& object : objects)
			{
				for (const auto& [action, commands] : object.action)
				{
					for (const RawCommand& command : commands)
					{
						if (command.verb != "pick" || !isHeld(command.objClass)) { continue; }
						object.hand = true;
						if (command.most > 0) { object.handHolds = command.most; }
					}
				}
			}
		}

		// Each state's <starts>: every held member of a group sent into one
		// holder, then from there so many into each holder named (a group's:
		// each member's own), in random order.
		void placeStarts(std::vector<RawState>& states, std::vector<RawObject>& objects)
		{
			const auto holdersFor = [&](const std::string& name, const std::string& state)
			{
				std::vector<RawObject*> found;
				for (RawObject& object : objects)
				{
					if (object.holder.is && object.ownerState == state && (object.named(state + "." + name) || object.named(name))) { found.push_back(&object); }
				}
				return found;
			};
			for (RawState& state : states)
			{
				std::string start;   // the holder every member starts in
				for (const RawStart& step : state.starts)
				{
					const std::string where = "state '" + state.name + "' > <starts> > <" + step.verb + ">";
					if (step.verb == "send" && step.all)
					{
						bool any = false;
						for (RawObject& object : objects)
						{
							if (object.held && object.ownerState == state.name && (object.named(state.name + "." + step.from) || object.named(step.from))) { object.item.startHolder = step.into; any = true; }
						}
						if (!any) { throw std::runtime_error(where + ": from=\"" + step.from + "\" names no held members of this state"); }
						if (holdersFor(step.into, state.name).size() != 1) { throw std::runtime_error(where + ": into=\"" + step.into + "\" names no one holder of this state"); }
						start = step.into;
						continue;
					}
					if (start.empty() || step.from != start)
					{
						throw std::runtime_error(where + ": for now a start sends <all /> of a group into one holder first, then orders and sends from that holder");
					}
					if (step.verb == "order") { continue; }   // what starts in a holder is always in random order
					std::vector<RawObject*> into = holdersFor(step.into, state.name);
					if (into.empty())
					{
						for (const RawObject& object : objects)
						{
							if (object.ownerState != state.name || object.handHolder.empty() || (object.groupName != step.into && object.groupName != state.name + "." + step.into)) { continue; }
							for (RawObject* own : holdersFor(object.handHolder, state.name)) { into.push_back(own); }
						}
					}
					if (into.empty()) { throw std::runtime_error(where + ": into=\"" + step.into + "\" names no holder of this state"); }
					for (RawObject* each : into)
					{
						each->holder.startCount = RawValue::expression(std::to_string(step.count));
						if (step.more != 0) { each->holder.startMore = RawValue::expression(std::to_string(step.more)); }
					}
				}
			}
			for (const RawObject& object : objects)
			{
				if (object.held && object.item.startHolder.empty())
				{
					throw std::runtime_error("object '" + object.name + "': of a held class, but no state's <starts> sends it into a holder");
				}
			}
		}


		// A command's `from` (self, others) made the object `to`: as the
		// object it names, at the start of the variables and values it names,
		// and, as a look, its own name (sprite="self": the look named after
		// it). Whether it named `from` at all.
		bool renameIn(RawCommand& command, const std::string& from, const std::string& to)
		{
			bool changed = false;
			if (command.object == from) { command.object = to; changed = true; }
			if (command.sprite == from)
			{
				command.sprite = to.substr(to.rfind('.') + 1);
				changed = true;
			}
			for (std::string* text : { &command.variable, &command.into })
			{
				std::string renamed = renameStart(*text, from, to);
				if (renamed != *text) { *text = std::move(renamed); changed = true; }
			}
			for (RawValue* each : { &command.amount, &command.seconds, &command.distance, &command.ahead,
				&command.positionX, &command.positionY, &command.velocityX, &command.velocityY, &command.accelerationX, &command.accelerationY, &command.column, &command.row })
			{
				changed = renameInValue(*each, from, to) || changed;
			}
			return changed;
		}

		// A state's condition's commands for the one it matched (self) among
		// what it looks at: self made that one, and a command naming the
		// others made once for each of the rest. Whether they name either.
		bool forMatched(std::vector<RawCommand>& list, const std::string& self, const std::vector<std::string>& others)
		{
			bool named = false;
			std::vector<RawCommand> out;
			for (const RawCommand& command : list)
			{
				RawCommand probe = command;
				if (renameIn(probe, "others", "others_"))
				{
					named = true;
					for (const std::string& other : others)
					{
						RawCommand each = command;
						renameIn(each, "others", other);
						renameIn(each, "self", self);
						out.push_back(std::move(each));
					}
					continue;
				}
				RawCommand each = command;
				named = renameIn(each, "self", self) || named;
				out.push_back(std::move(each));
			}
			list = std::move(out);
			return named;
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
		makeHeld(rawObjects);
		placeStarts(rawStates, rawObjects);

		checkColor(windowDesc.background, "<window> > <background>");

		nameNumbers(windowDesc, rawVariables, variables, rawObjects);
		makeObjects(windowDesc, rawObjects, objects);
		makeRuleTests(rawObjects, objects);
		makeStates(rawStates, states, objects);
		makeSounds(rawSounds, sounds);
		makePaths(rawPaths, paths, states, objects);
		makePalettes(rawObjects, objects, checkReferences(states, objects, sounds, paths, styles));
	}

	// init's names: the window's edges, the game's variables, every object's
	// variables and timers, and the live numbers, as names a text reads.
	void game_expr::nameNumbers(const WindowDesc& windowDesc, const std::vector<std::pair<std::string, RawValue>>& rawVariables, std::map<std::string, float>& variables, std::vector<RawObject>& rawObjects)
	{
		// the window's edges and middles, then the game's variables, as names a text reads
		calc.constant("window.top", 0);
		calc.constant("window.bottom", windowDesc.height);
		calc.constant("window.left", 0);
		calc.constant("window.right", windowDesc.width);
		calc.constant("window.width", windowDesc.width);
		calc.constant("window.height", windowDesc.height);
		calc.constant("window.width.center", windowDesc.width / 2);
		calc.constant("window.height.center", windowDesc.height / 2);

		// add variables from XML file to symbol table, in the order the file
		// declares them so that one can use the ones before it
		for (const auto& [name, rawValue] : rawVariables)
		{
			const float value = evaluate(rawValue, "variable '" + name + "'");
			variables[name] = value;
			calc.constant(name, value);
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
			// A holder's <counts variable>: a variable of its own, kept to the total.
			const std::string& counts = rawObject.holder.counts;
			if (!counts.empty() && !rawObject.variable.count(counts)) { rawObject.variable[counts] = RawValue::expression("0"); }
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
		for (auto& [name, number] : liveNumbers) { bind(name, number); }
	}

	// init's objects: every sprite measured once (its size a name), the
	// cells of groups laid out, then each object made, groups in lockstep.
	void game_expr::makeObjects(const WindowDesc& windowDesc, std::vector<RawObject>& rawObjects, std::vector<Object>& objects)
	{
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
		std::map<std::string, Picture> firstPassPictures;
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

			const Picture picture = buildPicture(rawObject.sprite, "object '" + rawObject.name + "'", &firstPassBitmaps[rawObject.name],
				rawObject.hasHeading ? &firstPassTurned[rawObject.name] : nullptr, &rawObject);
			objectShapeKinds[rawObject.name] = picture.kind;
			objectSizes[rawObject.name] = measureShapeSize(picture);
			firstPassPictures[rawObject.name] = picture;

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
		const std::map<std::string, Vector2f> cellOffsets = layOutCells(rawObjects, firstPassPictures);

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
			first.picture = firstPassPictures.at(rawObject.name);
			first.bitmap = firstPassBitmaps[rawObject.name];
			first.turned = firstPassTurned[rawObject.name];
			first.animation = firstPassAnimation[rawObject.name];
			first.animationTurned = firstPassAnimationTurned[rawObject.name];
			const auto offset = cellOffsets.find(rawObject.name);
			objects.push_back(onLine(rawObject.line, [&] { return makeObject(rawObject, first, windowDesc, offset == cellOffsets.end() ? Vector2f{} : offset->second, thisLockstep); }));


			if (rawObject.rawCollisionData.lockstep && rawObject.groupName.empty()) {
				lockstepNum++;
			}
		}
	}

	// init's tests on collision rules, edge guards and actions' conditions,
	// once every object they can name is made.
	void game_expr::makeRuleTests(const std::vector<RawObject>& rawObjects, std::vector<Object>& objects)
	{
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
					object->collisionData.others[i].check = makeTest(check, where, where, objects);
				}
			}
			for (std::size_t edge = 0; edge < rawObject.rawCollisionData.edges.size(); ++edge)
			{
				const std::vector<EdgeGuard>& guards = rawObject.rawCollisionData.edges[edge].guards;
				for (std::size_t g = 0; g < guards.size(); ++g)
				{
					if (guards[g].rawCheck.join != RawCondition::Join::None)
					{
						object->collisionData.edges[edge].guards[g].check = makeTest(guards[g].rawCheck, where, where, objects);
					}
				}
			}
			// Its actions' conditions, tests about itself.
			std::function<void(std::vector<Command>&, const std::string&)> tests = [&](std::vector<Command>& list, const std::string& here)
			{
				for (Command& command : list)
				{
					auto* when = std::get_if<CmdWhen>(&command);
					if (!when || !when->body) { continue; }
					when->body->test = makeTest(*when->body->raw, here + " > <condition>", here, objects);
					tests(when->body->commands, here + " > <condition>");
				}
			};
			for (auto& [name, list] : object->action) { tests(list, "object '" + rawObject.name + "' > <action name=\"" + name + "\">"); }
		}
	}

	// init's states: each one's inputs, conditions, timers and scenes.
	void game_expr::makeStates(std::vector<RawState>& rawStates, std::vector<State>& states, const std::vector<Object>& objects)
	{
		for (auto& rawState : rawStates)
		{
			State state{};
			const std::string where = "state '" + rawState.name + "'";

			state.name = rawState.name;
			state.line = rawState.line;
			state.show = rawState.show;
			state.showStates = rawState.showStates;
			state.music = rawState.music;

			for (std::size_t moment = 0; moment < keyMoments.size(); ++moment)
			{
				for (auto& rawAction : *rawState.keyMaps()[moment])
				{
					const KeyCode key = keyCodeFromString(rawAction.first);
					if (key == KeyCode::Unknown)
					{
						throw std::runtime_error(where + " > <input button=\"" + rawAction.first + "\">: there is no key named '" + rawAction.first + "'"
							+ didYouMean(rawAction.first, keyNames()) + ". The keys are: " + listOf(keyNames()));
					}
					(*state.keyMaps()[moment])[key] = processCommands(rawAction.second, where);
				}
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

			for (auto& rawCondition : flattened(rawState.conditions))
			{
				state.conditions.push_back(makeCondition(rawCondition, where, where, rawState.name, objects));
			}
			for (auto& rawCondition : flattened(rawState.computer))
			{
				state.computer.push_back(makeCondition(rawCondition, where + " > <computer>", where, rawState.name, objects));
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
				for (const RawCondition& rawCondition : flattened(rawScene.conditions))
				{
					scene.conditions.push_back(makeCondition(rawCondition, sceneWhere, sceneWhere, rawState.name, objects));
				}
				scene.timers = processTimers(rawScene.timers, sceneWhere);
				state.scenes.push_back(std::move(scene));
			}

			states.push_back(state);
		}
	}

	// init's sounds, after everything else so that a length can use any variable.
	void game_expr::makeSounds(const std::vector<RawSound>& rawSounds, std::vector<SoundDesc>& sounds)
	{
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
	}

	// init's paths, worked out last too, and each step's commands about
	// its follower made for each object that can fly it.
	void game_expr::makePaths(const std::vector<RawPath>& rawPaths, std::map<std::string, Path>& paths, const std::vector<State>& states, const std::vector<Object>& objects)
	{
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
					// A half that names something that changes, or the
					// follower (self), is worked out as the step sets off.
					const auto half = [&](const RawValue& value, float& into, std::shared_ptr<const RawValue>& live)
					{
						const std::vector<const std::string*> texts = value.expressions();
						const bool own = std::any_of(texts.begin(), texts.end(),
							[](const std::string* text) { return text->find("self.") != std::string::npos; });
						if (own || namesLive(value))
						{
							live = std::make_shared<const RawValue>(value);
							if (own) { return; }
							commandValues = true;
							evaluate(value, where + " > <step>");
							commandValues = false;
							return;
						}
						into = evaluate(value, where + " > <step>");
					};
					half(rawStep.x, step.by.x, step.liveX);
					half(rawStep.y, step.by.y, step.liveY);

					// Commands about the follower are made for each follower
					// (below); the rest once.
					std::vector<RawCommand> probe = rawStep.commands;
					if (nameSelfIn(probe, "follower"))
					{
						step.ownRaw = std::make_shared<const std::vector<RawCommand>>(rawStep.commands);
					}
					else
					{
						step.commands = processCommands(rawStep.commands, where + " > <step>");
					}
				}
				path.steps.push_back(std::move(step));
			}
			paths[path.name] = std::move(path);
		}

		// A step's commands about the follower, made for each object that
		// can fly that path.
		for (auto& [name, followers] : followersOf(states, objects, paths))
		{
			if (!paths.count(name)) { continue; } // named no path: checkReferences says so
			Path& path = paths.at(name);
			for (PathStep& step : path.steps)
			{
				if (!step.ownRaw) { continue; }
				for (const std::string& follower : followers)
				{
					step.ownBy[follower] = pathCommandsFor(step, follower, "path '" + name + "' > <step>, flown by '" + follower + "'");
				}
			}
		}
	}

	// What a <condition> tests, made ready (with its <all> or
	// <any>'s checks, each made the same way).
	Condition game_expr::makeTest(const RawCondition& rawCondition, const std::string& where, const std::string& owner, const std::vector<Object>& objects)
	{
		return onLine(rawCondition.line, [&] { return makeTestHere(rawCondition, where, owner, objects); });
	}

	Condition game_expr::makeTestHere(const RawCondition& rawCondition, const std::string& where, const std::string& owner, const std::vector<Object>& objects)
	{
		Condition condition;
		condition.line = rawCondition.line;
		condition.join = rawCondition.join;
		for (std::size_t i = 0; i < rawCondition.parts.size(); ++i)
		{
			const RawCondition& part = rawCondition.parts[i];
			const char* kind = part.join == RawCondition::Join::All ? "<all>" : part.join == RawCondition::Join::Any ? "<any>" : "<condition>";
			condition.parts.push_back(makeTest(part, where + " > " + kind + " " + std::to_string(i + 1), owner, objects));
		}
		if (condition.join != RawCondition::Join::None)
		{
			return condition;
		}

		condition.filterClass = rawCondition.filterClass;
		condition.filterObject = rawCondition.filterObject;

		// What it looks at is something the game has: a misspelt class or
		// object would match nothing, and the condition would quietly
		// never hold (or always, for an <atmost>).
		if (!condition.filterClass.empty() && std::none_of(objects.begin(), objects.end(), [&](const Object& object) { return object.objClass == condition.filterClass; }))
		{
			std::set<std::string> classes;
			for (const Object& object : objects) { if (!object.objClass.empty()) { classes.insert(object.objClass); } }
			const std::vector<std::string> names(classes.begin(), classes.end());
			throw std::runtime_error(where + ": class=\"" + condition.filterClass + "\": no object has that class" + didYouMean(condition.filterClass, names)
				+ (names.empty() ? std::string("; no object has a class") : ". The classes are: " + listOf(names)));
		}
		if (!condition.filterObject.empty() && !findObject(objects, condition.filterObject))
		{
			throw std::runtime_error(where + ": object=\"" + condition.filterObject + "\" names no object or group" + didYouMean(condition.filterObject, objectNames(objects)));
		}
		condition.variableName = rawCondition.variableName;
		condition.distanceTo = rawCondition.distance;
		condition.sprite = rawCondition.sprite;
		condition.sees = rawCondition.sees;
		condition.past = rawCondition.past;
		if (rawCondition.total == "count") { condition.total = Total::Count; }
		else if (rawCondition.total == "sum") { condition.total = Total::Sum; }
		else if (rawCondition.total == "highest") { condition.total = Total::Highest; }
		else if (rawCondition.total == "lowest") { condition.total = Total::Lowest; }

		// A number that names something that changes is worked out again
		// each time the condition is tested (Game::testCondition); now, it
		// may read where things are as 0, as a command's values may.
		const bool shows = rawCondition.test == RawCondition::Test::Shows || rawCondition.test == RawCondition::Test::Lowest || rawCondition.test == RawCondition::Test::Highest
			|| rawCondition.test == RawCondition::Test::Sees;
		const bool live = !shows && namesLive(rawCondition.threshold);
		commandValues = live;
		const float threshold = shows ? 0.0f : evaluate(rawCondition.threshold, where);
		commandValues = false;
		if (live)
		{
			condition.live = std::make_shared<const RawValue>(rawCondition.threshold);
			condition.where = where;
			const auto sameName = [](const std::string& a, const std::string& b)
			{
				return a.size() == b.size() && std::equal(a.begin(), a.end(), b.begin(),
					[](char x, char y) { return std::tolower(static_cast<unsigned char>(x)) == std::tolower(static_cast<unsigned char>(y)); });
			};
			for (const std::string* text : rawCondition.threshold.expressions())
			{
				for (const std::string& name : namesIn(*text))
				{
					for (auto& [key, slot] : objectVariables)
					{
						if (!sameName(key, name)) { continue; }
						const std::size_t dot = key.rfind('.');
						condition.reads.push_back({ key.substr(0, dot), key.substr(dot + 1), &slot });
						break;
					}
				}
			}
		}
		switch (rawCondition.test)
		{
		case RawCondition::Test::AtLeast:   condition.compare = Compare::AtLeast; break;
		case RawCondition::Test::AtMost:    condition.compare = Compare::AtMost; break;
		case RawCondition::Test::Above:     condition.compare = Compare::Above; break;
		case RawCondition::Test::Below:     condition.compare = Compare::Below; break;
		case RawCondition::Test::Equal:     condition.compare = Compare::Equal; break;
		case RawCondition::Test::Unequal:   condition.compare = Compare::Unequal; break;
		case RawCondition::Test::Remaining: condition.remaining = threshold; break;
		case RawCondition::Test::Shows:     condition.compare = Compare::Shows; break;
		case RawCondition::Test::Lowest:    condition.compare = Compare::Lowest; break;
		case RawCondition::Test::Highest:   condition.compare = Compare::Highest; break;
		case RawCondition::Test::Sees:      condition.compare = Compare::Equal; break;
		}
		condition.value = rawCondition.test == RawCondition::Test::Sees ? 1.0f : threshold;

		checkTestNames(rawCondition, condition, where, owner, objects);
		return condition;
	}

	// makeTestHere's checks: what a test names (what it sees and what is in
	// the way, a look, a distance, a variable) is something the game has.
	void game_expr::checkTestNames(const RawCondition& rawCondition, const Condition& condition, const std::string& where, const std::string& owner, const std::vector<Object>& objects)
	{
		// <sees>: what is looked for is there, and so is each class in the way.
		if (!condition.sees.empty() && !findObject(objects, condition.sees))
		{
			throw std::runtime_error(where + ": <sees object=\"" + condition.sees + "\"> names no object or group" + didYouMean(condition.sees, objectNames(objects)));
		}
		for (const std::string& past : condition.past)
		{
			if (std::none_of(objects.begin(), objects.end(), [&](const Object& object) { return object.objClass == past; }))
			{
				std::set<std::string> classes;
				for (const Object& object : objects) { if (!object.objClass.empty()) { classes.insert(object.objClass); } }
				throw std::runtime_error(where + ": <past class=\"" + past + "\">: no object has that class" + didYouMean(past, std::vector<std::string>(classes.begin(), classes.end())));
			}
		}

		// A look by name: something it looks at has a look of that name.
		if (!condition.sprite.empty())
		{
			std::vector<std::string> looks;
			for (const Object& object : objects)
			{
				if (!matchesFilter(rawCondition, object)) { continue; }
				for (const auto& look : object.looks) { looks.push_back(look.name); }
			}
			if (std::find(looks.begin(), looks.end(), condition.sprite) == looks.end())
			{
				throw std::runtime_error(where + ": sprite=\"" + condition.sprite + "\" is no look of what the condition looks at" + didYouMean(condition.sprite, looks));
			}
		}

		if (!condition.distanceTo.empty() && !findObject(objects, condition.distanceTo))
		{
			throw std::runtime_error(where + ": distance=\"" + condition.distanceTo + "\" names no object or group" + didYouMean(condition.distanceTo, objectNames(objects)));
		}

		// A total of a variable: some object it can count has it.
		if ((condition.total == Total::Sum || condition.total == Total::Highest || condition.total == Total::Lowest)
			&& condition.distanceTo.empty()
			&& std::none_of(objects.begin(), objects.end(), [&](const Object& object) { return matchesFilter(rawCondition, object) && numberOf(object, condition.variableName); }))
		{
			throw std::runtime_error(where + ": total=\"" + rawCondition.total + "\" of variable=\"" + condition.variableName + "\", and no object it looks at has a variable of that name");
		}

		// One object's variable: something it looks at has it, as a
		// variable of its own or as a number the engine keeps (x,
		// moves, ...). A misspelt one was never there to compare.
		if (condition.total == Total::None && !condition.remaining && condition.distanceTo.empty() && !condition.variableName.empty()
			&& std::none_of(objects.begin(), objects.end(), [&](const Object& object) { return matchesFilter(rawCondition, object) && numberOf(object, condition.variableName); }))
		{
			std::set<std::string> own;
			for (const Object& object : objects)
			{
				if (!matchesFilter(rawCondition, object)) { continue; }
				for (const auto& [name, value] : object.variable) { own.insert(name); }
			}
			std::vector<std::string> names(own.begin(), own.end());
			std::vector<std::string> all = names;
			all.insert(all.end(), liveNumberParts().begin(), liveNumberParts().end());
			throw std::runtime_error(where + ": variable=\"" + condition.variableName + "\": nothing the condition looks at has a variable or number of that name"
				+ didYouMean(condition.variableName, all) + (names.empty() ? std::string("; it has no <variables>") : "; its variables are: " + listOf(names)));
		}

		if ((condition.remaining || condition.total == Total::Count) && std::none_of(objects.begin(), objects.end(), [&](const Object& object) { return matchesFilter(rawCondition, object); }))
		{
			std::cout << "warning: " << owner << (condition.remaining ? ": a condition with remaining= matches no object at all, so it would fire at once\n"
				: ": a condition with total=\"count\" matches no object at all, so it counts 0\n");
		}
	}

	// A state's condition: its test, and its commands; or, when they name
	// self or others, its commands for each object it may match (the
	// last test of an <all>'s, as a condition in a condition is), and
	// they run for each one that holds.
	Condition game_expr::makeConditionHere(const RawCondition& rawCondition, const std::string& where, const std::string& owner, const std::string& stateName, const std::vector<Object>& objects)
	{
		Condition condition = makeTest(rawCondition, where, owner, objects);
		std::vector<RawCommand> probe = rawCondition.commands;
		if (!forMatched(probe, "self_", { "others_" }))
		{
			condition.commands = processCommands(rawCondition.commands, where);
			return condition;
		}
		const RawCondition* test = &rawCondition;
		while (test->join == RawCondition::Join::All && !test->parts.empty()) { test = &test->parts.back(); }
		if (test->join != RawCondition::Join::None || (test->filterClass.empty() && test->filterObject.empty()))
		{
			throw std::runtime_error(where + ": its commands name self or others, so its test (or the last of its <all>) needs class= or object= to say what they are among");
		}
		if (test->total != "" || test->test == RawCondition::Test::Remaining)
		{
			throw std::runtime_error(where + ": its commands name self or others, but its test is a total of everything matched, so no one object holds it");
		}
		std::vector<std::string> matched;
		for (const Object& object : objects)
		{
			if (matchesFilter(*test, object) && (object.ownerState.empty() || object.ownerState == stateName)) { matched.push_back(object.name); }
		}
		// What names self or others runs for each one it holds for; the rest once, after.
		std::vector<RawCommand> eachOne;
		std::vector<RawCommand> once;
		for (const RawCommand& command : rawCondition.commands)
		{
			std::vector<RawCommand> one{ command };
			(forMatched(one, "self_", { "others_" }) ? eachOne : once).push_back(command);
		}
		for (const std::string& self : matched)
		{
			std::vector<std::string> others;
			for (const std::string& other : matched) { if (other != self) { others.push_back(other); } }
			std::vector<RawCommand> commands = eachOne;
			forMatched(commands, self, others);
			condition.each.push_back({ self, processCommands(commands, where) });
		}
		condition.commands = processCommands(once, where);
		return condition;
	}
	Condition game_expr::makeCondition(const RawCondition& rawCondition, const std::string& where, const std::string& owner, const std::string& stateName, const std::vector<Object>& objects)
	{
		return onLine(rawCondition.line, [&] { return makeConditionHere(rawCondition, where, owner, stateName, objects); });
	}

	namespace
	{
		// A sprite as it is with another style's colors in place of the
		// ones it took from its own (see RawSprite::styleChars); its own stay.
		void restyle(RawSprite& sprite, const RawStyle& style)
		{
			if (sprite.kind == "bitmap" && !sprite.style.empty())
			{
				if (sprite.styleColor && !style.color.empty()) { sprite.color = style.color; }
				for (auto& [character, color] : sprite.bitmapColors)
				{
					if (sprite.styleChars.find(character) == std::string::npos) { continue; }
					const auto given = std::find_if(style.bitmapColors.begin(), style.bitmapColors.end(), [&](const auto& each) { return each.first == character; });
					if (given != style.bitmapColors.end()) { color = given->second; }
				}
			}
			for (RawSprite& frame : sprite.frames) { restyle(frame, style); }
		}

		bool takesStyle(const RawSprite& sprite)
		{
			return (sprite.kind == "bitmap" && !sprite.style.empty()) || std::any_of(sprite.frames.begin(), sprite.frames.end(), takesStyle);
		}
	}

	void game_expr::makePalettes(const std::vector<RawObject>& rawObjects, std::vector<Object>& objects, const std::map<std::string, std::vector<std::string>>& wanted)
	{
		for (Object& object : objects)
		{
			const auto asked = wanted.find(object.name);
			if (asked == wanted.end()) { continue; }
			const auto raw = std::find_if(rawObjects.begin(), rawObjects.end(), [&](const RawObject& each) { return each.name == object.name; });
			if (raw == rawObjects.end()) { continue; }

			const std::string where = "object '" + object.name + "'";
			if (!object.turnables.empty())
			{
				throw std::runtime_error(where + ": <become style=\"" + asked->second.front() + "\" />: a palette is for a picture that does not turn; it has a <heading>");
			}
			if (!takesStyle(raw->sprite) && std::none_of(raw->looks.begin(), raw->looks.end(), takesStyle)
				&& std::none_of(raw->animation.frames.begin(), raw->animation.frames.end(), takesStyle))
			{
				throw std::runtime_error(where + ": <become style=\"" + asked->second.front() + "\" />: it has no <bitmap style=\"...\"> whose colors another style could change");
			}
			if (object.looks.size() != raw->looks.size()) { continue; }

			// As loaded: the colors it starts in, and a reset puts back.
			Object::Palette loaded;
			loaded.looks = object.looks;
			loaded.bitmap = object.looks.empty() ? object.facingPicture(object.bitmap) : object.looks.front().bitmap;
			if (object.looks.empty()) { loaded.animation = object.animationBitmaps; }
			object.palettes.push_back(std::move(loaded));

			for (const std::string& name : asked->second)
			{
				const RawStyle& style = *std::find_if(styles.begin(), styles.end(), [&](const RawStyle& each) { return each.name == name; });
				const auto restyled = [&](RawSprite sprite) { restyle(sprite, style); return sprite; };
				const std::string here = where + " in style '" + name + "'";

				Object::Palette palette;
				palette.name = name;
				if (!object.looks.empty())
				{
					palette.looks = object.looks;
					for (std::size_t i = 0; i < palette.looks.size(); ++i)
					{
						const RawSprite look = restyled(raw->looks[i]);
						buildPicture(look, here, &palette.looks[i].bitmap);
						if (!look.frames.empty()) { palette.looks[i].frames = buildAnimationBitmaps(look.frames, here); }
					}
					palette.bitmap = palette.looks.front().bitmap;
				}
				else
				{
					buildPicture(restyled(raw->sprite), here, &palette.bitmap);
					if (!object.animationBitmaps.empty())
					{
						std::vector<RawSprite> frames;
						for (const RawSprite& frame : raw->animation.frames) { frames.push_back(restyled(frame)); }
						palette.animation = buildAnimationBitmaps(frames, here);
					}
				}
				object.palettes.push_back(std::move(palette));
			}
			if (object.mirrors) { object.flips = flipsOf(object); }
		}
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
		first.picture = from.looks.empty() ? from.picture : from.looks.front().picture;
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

		// Its palettes too, wearing the one `from` is (its looks are those).
		Object made = makeObject(raw, first, windowDesc, from.cellOffset, from.collisionData.lockstep);
		made.palettes = from.palettes;
		made.palette = from.palette;
		if (made.mirrors) { made.flips = from.flips; }
		return made;
	}

	SoundDesc game_expr::processSound(const RawSound& raw)
	{
		const std::string where = "sound '" + raw.name + "'";

		SoundDesc sound;
		sound.name = raw.name;
		sound.loop = raw.loop;
		sound.path = raw.path;

		const bool noVolume = raw.volume.kind == RawValue::Kind::Expression && raw.volume.text.empty();
		// A recording plays as loud as it was made unless told otherwise.
		if (!sound.path.empty()) { sound.volume = 1.0f; }
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

		// A recording is read once now, so a missing or unreadable file stops
		// the load before a window opens.
		if (!sound.path.empty())
		{
			try
			{
				(void)readWave(sound.path);
			}
			catch (const std::runtime_error& error)
			{
				throw std::runtime_error(where + ": " + error.what());
			}
			return sound;
		}

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

	// A <hitbox> worked out for `object` (its picture already made).
	Hitbox game_expr::makeHitbox(const RawHitbox& raw, const Object& object, const std::string& where)
	{
		if (object.collisionData.type == CollisionType::Pixel)
		{
			throw std::runtime_error(where + ": a <hitbox> is touched as its own shape, not by the pixels of the picture; it cannot go with <type>pixel</type>");
		}
		const auto valueOr0 = [&](const RawValue& value) { return value.kind == RawValue::Kind::Expression && value.text.empty() ? 0.0f : evaluate(value, where); };
		Hitbox hitbox;
		hitbox.name = raw.name;
		if (raw.kind == "inset")
		{
			hitbox.inset = true;
			hitbox.top = valueOr0(raw.top);
			hitbox.bottom = valueOr0(raw.bottom);
			hitbox.left = valueOr0(raw.left);
			hitbox.right = valueOr0(raw.right);
			// Out from the picture, never; and something must be left.
			if (std::min({ hitbox.top, hitbox.bottom, hitbox.left, hitbox.right }) < 0.0f)
			{
				throw std::runtime_error(where + ": a <hitbox> pulls in from the picture's sides, never out; one side is " + formatDisplayNumber(std::min({ hitbox.top, hitbox.bottom, hitbox.left, hitbox.right })));
			}
			if (object.size.x > 0.0f && object.size.y > 0.0f && (hitbox.left + hitbox.right >= object.size.x || hitbox.top + hitbox.bottom >= object.size.y))
			{
				throw std::runtime_error(where + ": a <hitbox> pulled in that far leaves nothing of a picture " + formatDisplayNumber(object.size.x) + " by " + formatDisplayNumber(object.size.y));
			}
			hitbox.round = object.picture.kind == ShapeKind::Circle;
			return hitbox;
		}

		hitbox.round = raw.kind == "circle";
		if (hitbox.round)
		{
			const float radius = evaluate(raw.radius, where);
			hitbox.size = { radius * 2.0f, radius * 2.0f };
		}
		else
		{
			hitbox.size = { evaluate(raw.width, where), evaluate(raw.height, where) };
		}
		if (!(hitbox.size.x > 0.0f) || !(hitbox.size.y > 0.0f))
		{
			throw std::runtime_error(where + ": a <hitbox> must be bigger than nothing; its size is " + formatDisplayNumber(hitbox.size.x)
				+ " by " + formatDisplayNumber(hitbox.size.y));
		}
		hitbox.offset = { valueOr0(raw.x), valueOr0(raw.y) };
		return hitbox;
	}

	float game_expr::evaluateStart(const RawValue& value, const std::string& where)
	{
		if (!value.live) { return evaluate(value, where); }
		// It may read where things are (reading 0 while loading): the game
		// works it out again before anything is drawn (Game::follow).
		const bool was = commandValues;
		commandValues = true;
		try
		{
			const float answer = evaluate(value, where);
			commandValues = was;
			return answer;
		}
		catch (...)
		{
			commandValues = was;
			throw;
		}
	}

	float game_expr::evaluate(const RawValue& value, const std::string& where)
	{
		switch (value.kind)
		{
		case RawValue::Kind::Random:
		{
			if (value.operations)
			{
				const RawOperation& ends = value.operations->front();
				const float low = evaluate(ends.operands[0].value, where + " > <random> > <min>");
				const float high = evaluate(ends.operands[1].value, where + " > <random> > <max>");
				return randomBetween(low, high);
			}
			const float min = evaluateExpression(value.min, where);
			const float max = evaluateExpression(value.max, where);
			return randomBetween(min, max);
		}

		case RawValue::Kind::Formula:
		{
			const float answer = evaluateOperation(value.operations->front(), where).value;
			if (loading && !std::isfinite(answer))
			{
				throw std::runtime_error(where + ": comes out as " + (std::isnan(answer) ? std::string("not a number") : std::string("infinity")));
			}
			return answer;
		}

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

			// An operation nested in another is part of the same sum.
			if (operand.value.kind == RawValue::Kind::Formula) { return evaluateOperation(operand.value.operations->front(), here); }

			return Answer{ evaluate(operand.value, here), false };
		};

		Answer answer = operandAnswer(operation.operands.front());

		// One operand: the answer is worked out from it alone.
		const float x = answer.value;
		if (operation.op == "absolute") { answer.value = std::fabs(x); return answer; }
		if (operation.op == "negate") { answer.value = -x; return answer; }
		if (operation.op == "sign") { answer.value = sign(x); return answer; }
		if (operation.op == "floor") { answer.value = std::floor(x); return answer; }
		if (operation.op == "ceiling") { answer.value = std::ceil(x); return answer; }
		if (operation.op == "round") { answer.value = std::round(x); return answer; }

		// Of an angle in degrees, like every angle in a game.
		if (operation.op == "sine" || operation.op == "cosine")
		{
			answer.value = operation.op == "sine" ? sine(x) : cosine(x);
			return answer;
		}

		if (operation.op == "clamp")
		{
			const Answer low = operandAnswer(operation.operands[1]);
			const Answer high = operandAnswer(operation.operands[2]);
			answer.late = answer.late || low.late || high.late;
			answer.value = std::min(std::max(answer.value, low.value), high.value);
			return answer;
		}

		for (std::size_t i = 1; i < operation.operands.size(); ++i)
		{
			const Answer operand = operandAnswer(operation.operands[i]);
			answer.late = answer.late || operand.late;

			if ((operation.op == "divide" || operation.op == "remainder") && operand.value == 0.0f)
			{
				if (loading && !operand.late) { throw std::runtime_error(here + ": the divisor is 0"); }

				// Late while loading: not known yet, so nothing to say; it is worked out again.
				if (!loading && warnedDivisions.insert(here).second)
				{
					std::cerr << "warning: " << here << ": the divisor is 0; using 0 for the answer" << std::endl;
				}
				return Answer{ 0.0f, answer.late };
			}

			if (operation.op == "smallest") { answer.value = std::min(answer.value, operand.value); }
			else if (operation.op == "largest") { answer.value = std::max(answer.value, operand.value); }
			else if (operation.op == "add") { answer.value += operand.value; }
			else if (operation.op == "subtract") { answer.value -= operand.value; }
			else if (operation.op == "multiply") { answer.value *= operand.value; }
			else if (operation.op == "divide") { answer.value /= operand.value; }
			// What is left over, from 0 up to the divisor (-1 of 4 leaves 3), so it wraps.
			else if (operation.op == "remainder") { answer.value -= operand.value * std::floor(answer.value / operand.value); }
			// A <compare>, by its test (the second operand's role): 1 when it holds, 0 when not.
			else
			{
				const std::string& test = operation.operands[i].role;
				const bool holds = test == "above" ? answer.value > operand.value
					: test == "below" ? answer.value < operand.value
					: test == "atleast" ? answer.value >= operand.value
					: test == "atmost" ? answer.value <= operand.value
					: answer.value == operand.value;
				answer.value = holds ? 1.0f : 0.0f;
			}
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
			// Only the names it reads go into calc, the first time any text
			// reads them (see bind).
			for (const std::string& name : namesIn(text))
			{
				if (const auto waiting = unbound.find(name); waiting != unbound.end())
				{
					calc.variable(name, *waiting->second);
					unbound.erase(waiting);
				}
			}

			try
			{
				found = compiled.emplace(text, calc.compile(text)).first;
			}
			catch (const std::invalid_argument& error)
			{
				throw std::runtime_error(where + ": cannot read \"" + text + "\": " + error.what());
			}
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

		const float value = Calc::run(found->second);
		if (loading && !std::isfinite(value))
		{
			throw std::runtime_error(where + ": \"" + text + "\" comes out as " + (std::isnan(value) ? "not a number" : "infinity"));
		}
		return value;
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
			found = namesInText.emplace(text, Calc::namesIn(text)).first;
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

	std::vector<Command> game_expr::pathCommandsFor(const PathStep& step, const std::string& follower, const std::string& where)
	{
		if (!step.ownRaw) { return {}; }
		std::vector<RawCommand> own = *step.ownRaw;
		nameSelfIn(own, follower);
		return processCommands(own, where);
	}

	std::vector<Command> game_expr::processCommands(const std::vector<RawCommand>& raw, const std::string& where)
	{
		std::vector<Command> commands;
		commandValues = true;
		try
		{
			for (const RawCommand& each : raw)
			{
				// A condition in an action: its commands now, its test once
				// every object is made (testActionConditions).
				if (each.verb == "condition" && each.test)
				{
					auto body = std::make_shared<WhenBody>();
					body->raw = each.test;
					commandValues = false;
					body->commands = processCommands(each.test->commands, where + " > <condition>");
					commandValues = true;
					commands.push_back(CmdWhen{ std::move(body) });
					continue;
				}
				Command made = onLine(each.line, [&] { return makeCommand(each, [&](const RawValue& value) { return evaluate(value, where); }); });

				// One whose numbers can change is made again each time it runs
				// (CmdLive); <set> and <spawn> always are, since what they
				// put somewhere is the point of them.
				const bool live = each.verb == "set" || each.verb == "spawn"
					|| namesLive(each.amount) || namesLive(each.seconds) || namesLive(each.distance) || namesLive(each.ahead)
					|| (each.verb == "shift" && (namesLive(each.positionX) || namesLive(each.positionY)));
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
		catch (const LoadError& error)
		{
			commandValues = false;
			throw LoadError(where + ": " + error.what(), error.line);
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
					// A holder's live numbers follow it too (shown 0 until dealt).
					if (reference->second == "cards" || reference->second == "held" || reference->second == "points" || reference->second == "called" || reference->second == "calling"
						|| reference->second == "pattern" || reference->second == "strength" || reference->second == "odds"
						|| reference->second == "lead" || reference->second == "winner" || reference->second == "tricks" || reference->second == "deadwood")
					{
						piece.owner = owner;
						piece.variable = reference->second;
						piece.choices = raw.choices;
						piece.shown = showPiece(raw.kind, 0.0f, piece.choices);
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

	namespace
	{
		// A size that cannot be negative (0 is an object that only holds numbers).
		float atLeastZero(float value, const std::string& where)
		{
			if (value < 0.0f) { throw std::runtime_error(where + " is " + formatDisplayNumber(value) + "; expected a number of pixels, 0 or more"); }
			return value;
		}
	}

	Picture game_expr::buildPicture(const RawSprite& sprite, const std::string& where,
		std::shared_ptr<const Bitmap>* bitmap, std::shared_ptr<const Turnable>* turnable, const RawObject* self)
	{
		const std::string color = sprite.color.empty() ? "color.white" : sprite.color;
		checkColor(color, where);
		Picture picture;
		picture.color = color;

		// Lines, rows of text and SVG drawings are all pictures the engine draws
		// itself, and all go the same way, here, once, when the game loads:
		//   1. read what the file describes (line ends, rows, a part of a drawing);
		//   2. draw it into a Bitmap, the pixels every window backend shows and a
		//      pixel collision tests, so no backend ever draws one itself;
		//   3. flip it, if the sprite says so (a <bitmap> or an <svg>), the two
		//      steps done once for all the sprites that are the same picture;
		//   4. for an object with a <heading>, keep it (a Turnable) to be turned
		//      to whatever heading it faces, and start it at heading 0.
		// The Picture carries only the drawing's size. The one difference is in the
		// turning: lines are kept as lines and drawn again at each heading, which
		// keeps their edges sharp, where a finished picture's pixels are turned.
		if (sprite.kind == "line" || sprite.kind == "polygon" || sprite.kind == "bitmap" || sprite.kind == "svg" || sprite.kind == "cell")
		{
			auto kept = std::make_shared<Turnable>();
			std::shared_ptr<const Bitmap> drawn;

			// 3. Flipped.
			const auto flipped = [&](Bitmap unflipped)
			{
				return sprite.flip.empty() ? unflipped : flipBitmap(unflipped, sprite.flip == "horizontal", sprite.flip == "vertical");
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
				else if (sprite.kind == "polygon")
				{
					// 1. The corners and colours; 2. drawn, filled. Turned as a
					// finished picture, pixel by pixel, and holed as one.
					std::vector<FilledPolygon> polygons;
					for (const RawPolygon& rawPolygon : sprite.polygons)
					{
						FilledPolygon polygon;
						for (const RawVector2& point : rawPolygon.points)
						{
							polygon.points.emplace_back(evaluate(point.x, where), evaluate(point.y, where));
						}
						checkColor(rawPolygon.color.empty() ? "color.white" : rawPolygon.color, where + " > <polygon>");
						polygon.color = colorFromName(rawPolygon.color.empty() ? "color.white" : rawPolygon.color);
						polygons.push_back(std::move(polygon));
					}
					drawn = std::make_shared<const Bitmap>(rasterizePolygons(polygons));
				}
				else if (sprite.kind == "cell")
				{
					// 1. Which sheet, which cell or part of it, at what size.
					const auto sheet = std::find_if(sheets.begin(), sheets.end(), [&](const Sheet& each) { return each.name == sprite.sheet; });
					if (sheet == sheets.end()) { throw std::invalid_argument("<cell sheet=\"" + sprite.sheet + "\">: the game has no <sheet name=\"" + sprite.sheet + "\">"); }
					const auto given = [](const RawValue& value) { return !(value.kind == RawValue::Kind::Expression && value.text.empty()); };
					SheetCell cell;
					std::string key = "cell|" + sheet->name + "|";
					if (sprite.hasSvgRegion)
					{
						cell.free = true;
						cell.x = evaluate(sprite.svgX, where);
						cell.y = evaluate(sprite.svgY, where);
						cell.width = evaluate(sprite.svgWidth, where);
						cell.height = evaluate(sprite.svgHeight, where);
						key += std::to_string(cell.x) + "," + std::to_string(cell.y) + "," + std::to_string(cell.width) + "," + std::to_string(cell.height);
					}
					else
					{
						cell.column = given(sprite.column) ? static_cast<int>(std::lround(evaluate(sprite.column, where))) : 1;
						cell.row = given(sprite.row) ? static_cast<int>(std::lround(evaluate(sprite.row, where))) : 1;
						key += std::to_string(cell.column) + "/" + std::to_string(cell.row);
					}
					const bool sized = given(sprite.width) && given(sprite.height);
					const int toWidth = sized ? static_cast<int>(std::lround(evaluate(sprite.width, where))) : 0;
					const int toHeight = sized ? static_cast<int>(std::lround(evaluate(sprite.height, where))) : 0;
					const float cellScale = sprite.hasScale ? evaluate(sprite.scale, where) : 1.0f;
					key += sized ? "|" + std::to_string(toWidth) + "x" + std::to_string(toHeight) : "|*" + std::to_string(cellScale);
					key += "|" + sprite.flip;

					// 2. Cut, once for every sprite of the same cell and size (a
					// a pack's items, a group's members), from the sheet's picture,
					// opened once while the game loads.
					drawn = drawOnce(key, [&]
					{
						std::shared_ptr<SheetPicture>& opened = openSheets[sheet->name];
						if (!opened) { opened = std::make_shared<SheetPicture>(*sheet); }
						const SheetPart part = opened->partOf(cell);
						return flipped(sized ? opened->cut(part, toWidth, toHeight) : opened->cut(part, cellScale));
					});
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

			picture.kind = ShapeKind::Line;
			picture.size = { static_cast<float>(drawn->width), static_cast<float>(drawn->height) };
			if (bitmap) { *bitmap = std::move(drawn); }
		}
		else if (sprite.kind == "circle")
		{
			picture.kind = ShapeKind::Circle;
			picture.radius = atLeastZero(evaluate(sprite.radius, where), where + " > <circle> > <radius>");
		}
		else if (sprite.kind == "rectangle")
		{
			picture.kind = ShapeKind::Rectangle;
			picture.size.x = atLeastZero(evaluate(sprite.width, where), where + " > <rectangle> > <width>");
			picture.size.y = atLeastZero(evaluate(sprite.height, where), where + " > <rectangle> > <height>");
		}
		else if (sprite.kind == "text")
		{
			// Its pieces: fixed labels, numbers shown as the whole numbers they
			// are, with no decimal point, the way someone typing the label
			// would, and letters.
			if (sprite.text.empty())
			{
				throw std::runtime_error(where + " > <text>: needs a <content> (a label), a <number>, a <letter> or a <choose>; only a group's text can leave them to its members, rows or cells");
			}
			picture.kind = ShapeKind::Text;
			picture.text = joinText(textPieces(sprite, where, self));
			const float size = evaluate(sprite.size, where);
			if (!(size > 0.0f)) { throw std::runtime_error(where + " > <text> > <size> is " + formatDisplayNumber(size) + "; expected a number of pixels above 0"); }
			picture.textSize = static_cast<int>(size);
			picture.font = sprite.font;
		}
		else if (sprite.kind == "image")
		{
			picture.kind = ShapeKind::Image;
			picture.file = sprite.path;
			picture.flipHorizontal = sprite.flip == "horizontal";
			picture.flipVertical = sprite.flip == "vertical";
		}
		else if (sprite.kind == "tether")
		{
			// Drawn every frame where its two ends are (drawTethers); until
			// then, one clear pixel.
			auto clear = std::make_shared<Bitmap>();
			clear->width = 1;
			clear->height = 1;
			clear->rgba.assign(4, 0);
			picture.kind = ShapeKind::Line;
			picture.size = { 1.0f, 1.0f };
			if (bitmap) { *bitmap = std::move(clear); }
		}
		else
		{
			throw std::runtime_error(where + ": unknown shape '" + sprite.kind + "'");
		}

		return picture;
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

			if (frame.kind != "bitmap" && frame.kind != "svg" && frame.kind != "line" && frame.kind != "cell")
			{
				throw std::runtime_error(here + ": a frame must be a <bitmap>, an <svg>, a sheet's <cell> or a drawing of <line>s, not a <" + frame.kind + ">");
			}

			std::shared_ptr<const Bitmap> picture;
			std::shared_ptr<const Turnable> turnable;
			buildPicture(frame, here, &picture, turnables ? &turnable : nullptr);

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
		const auto addLooks = [&](const std::vector<Object::Look>& looks)
		{
			for (const Object::Look& look : looks)
			{
				add(look.bitmap);
				for (const auto& picture : look.frames) { add(picture); }
			}
		};
		add(object.bitmap);
		for (const auto& picture : object.animationBitmaps) { add(picture); }
		addLooks(object.looks);
		for (const Object::Palette& palette : object.palettes)
		{
			add(palette.bitmap);
			for (const auto& picture : palette.animation) { add(picture); }
			addLooks(palette.looks);
		}
		return flips;
	}

	int game_expr::animationFramesOf(const RawValue& interval, const WindowDesc& windowDesc, const std::string& where)
	{
		const float seconds = evaluate(interval, where + " > <seconds>");

		if (!(seconds > 0.0f))
		{
			throw std::runtime_error(where + " > <seconds> is " + std::to_string(seconds) + "; expected a number of seconds above 0");
		}
		if (windowDesc.framerate < 1)
		{
			throw std::runtime_error(where + " > <seconds> is a time, so the <window> needs a <framerate> above 0 to count them in");
		}

		return std::max(1, static_cast<int>(std::lround(seconds * static_cast<float>(windowDesc.framerate))));
	}

	std::vector<int> game_expr::animationTimesOf(const std::vector<RawValue>& times, int fallback, const WindowDesc& windowDesc, const std::string& where)
	{
		std::vector<int> frames;
		for (std::size_t i = 0; i < times.size(); ++i)
		{
			frames.push_back(frameTimeGiven(times[i])
				? animationFramesOf(times[i], windowDesc, where + " > <frame> " + std::to_string(i + 1))
				: fallback);
		}
		return frames;
	}

	namespace
	{
		// A picture's size where cells are laid out: a text counts as no size,
		// so rows of text are spaced by their <padding> alone, top to top (lines
		// of words), and each sits at its place's top left; a shape is as
		// written, not rounded.
		Vector2f layoutSize(const Picture& picture)
		{
			return picture.kind == ShapeKind::Text ? Vector2f{} : measureShapeSize(picture);
		}
	}

	std::map<std::string, Vector2f> game_expr::layOutCells(const std::vector<RawObject>& rawObjects,
		const std::map<std::string, Picture>& pictures)
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
			const Vector2f own = layoutSize(pictures.at(rawObject.name));
			const float across = static_cast<float>(rawObject.cell.mapColumn - 1) + (rawObject.cell.between ? 0.5f : 0.0f);
			offsets[rawObject.name] = { across * tile.x + (tile.x - own.x) / 2.0f,
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
				const Picture slotPicture = buildPicture(first.cell.slot, where);
				const Vector2f slot = layoutSize(slotPicture);

				if (row > rows.begin()->first) { top += above + evaluate(first.cell.gapAbove, where); }
				above = slot.y;

				float left = 0.0f;
				for (const RawObject* cell : cells)
				{
					const std::string here = "object '" + cell->name + "'";
					if (cell->cell.column > 1) { left += slot.x + evaluate(cell->cell.gapBefore, here); }

					const Vector2f own = layoutSize(pictures.at(cell->name));
					// A <slope>: each column that much lower than the one before.
					const float drop = cell->cell.drop.text.empty() && cell->cell.drop.kind == RawValue::Kind::Expression ? 0.0f
						: evaluate(cell->cell.drop, here + " > <slope>") * static_cast<float>(cell->cell.column - 1);
					offsets[cell->name] = { left + (slot.x - own.x) / 2.0f, top + drop + (slot.y - own.y) / 2.0f };
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
					if (kind != objectShapeKinds.end() && kind->second == ShapeKind::Text
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
