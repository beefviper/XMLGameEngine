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
				// Its first look is its face and its second its back; with one look
				// (a token no one sees) it shows the same either way.
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
				if (!object.looks.empty()) { object.sprite = object.looks.front(); }
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
					if (step.verb == "order")
					{
						// What starts in it is shuffled; with no <order>, it lies in the pack's order.
						for (RawObject* each : holdersFor(step.from, state.name)) { each->holder.startShuffled = true; }
						continue;
					}
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
		makeObjects(rawObjects, objects);
		makeRuleTests(rawObjects, objects);
		makeStates(rawStates, states, objects);
		makeSounds(rawSounds, sounds);
		makePaths(rawPaths, paths, states, objects);
		makePalettes(rawObjects, objects, checkReferences(*parts, states, objects, sounds, paths, styles));
		for (Object& object : objects)
		{
			for (CollisionRule& rule : object.kind->collisionData.others) { std::vector<std::vector<Command>>().swap(rule.loadChecks); }
		}
		shareKinds(objects);
	}

	void game_expr::shareKinds(std::vector<Object>& objects)
	{
		// Each was made and checked for itself; from here the first of its
		// kind's is every one's.
		std::map<int, std::shared_ptr<Kind>> first;
		for (Object& object : objects)
		{
			const int written = object.kind->written;
			if (written == 0) { continue; }
			const auto [found, fresh] = first.emplace(written, object.kind);
			if (!fresh) { object.kind = found->second; }
		}
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
				const auto& liveNames = liveNumberParts();
				if (std::find(liveNames.begin(), liveNames.end(), timer.name) != liveNames.end())
				{
					throw std::runtime_error(here + ": " + rawObject.name + "." + timer.name + " is already where the object is or how it moves; give the timer another name");
				}
				rawObject.variable[timer.name] = RawValue::expression("0");
			}
			// A holder's <counts variable>: a variable of its own, kept to the total.
			const std::string& counts = rawObject.holder.counts;
			if (!counts.empty() && !rawObject.variable.count(counts)) { rawObject.variable[counts] = RawValue::expression("0"); }
		}
		// A holder's top.<variable>: each variable its held class has, the top
		// one's while the game runs (Game::refreshHolder), so bag.top.shape reads.
		for (auto& holder : rawObjects)
		{
			if (!holder.holder.is || holder.holder.holdsClass.empty()) { continue; }
			for (const auto& member : rawObjects)
			{
				if (&member == &holder || member.objClass != holder.holder.holdsClass) { continue; }
				for (const auto& [name, value] : member.variable) { holder.variable.try_emplace("top." + name, RawValue::expression("0")); }
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
		for (auto& [name, number] : liveNumbers) { bind(name, number); }
	}

	// init's objects: every sprite measured once (its size a name), the
	// cells of groups laid out, then each object made, groups in lockstep.
	void game_expr::makeObjects(std::vector<RawObject>& rawObjects, std::vector<Object>& objects)
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
			objects.push_back(onLine(rawObject.line, [&] { return makeObject(rawObject, first, offset == cellOffsets.end() ? Vector2f{} : offset->second, thisLockstep); }));


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
			// Their tests say self of this one (makeTestHere).
			struct Self
			{
				std::string& name;
				std::string was;
				~Self() { name = std::move(was); }
			} self{ selfName, std::exchange(selfName, rawObject.name) };
			for (std::size_t i = 0; i < rawObject.rawCollisionData.others.size(); ++i)
			{
				const RawCollisionRule& rule = rawObject.rawCollisionData.others[i];
				const RawCondition& check = rule.check;
				if (check.join == RawCondition::Join::None) { continue; }
				// One naming other: made, so checked, for each thing it can
				// touch, and kept as made for the first.
				for (std::size_t t = rule.touchable.size(); t-- > 1;)
				{
					const std::string was = std::exchange(otherName, rule.touchable[t]);
					makeTest(check, where + ", touching '" + otherName + "'", where, objects);
					otherName = was;
				}
				const std::string was = std::exchange(otherName, rule.touchable.empty() ? std::string() : rule.touchable.front());
				object->kind->collisionData.others[i].check = makeTest(check, where, where, objects);
				otherName = was;
			}
			for (std::size_t edge = 0; edge < rawObject.rawCollisionData.edges.size(); ++edge)
			{
				const std::vector<EdgeGuard>& guards = rawObject.rawCollisionData.edges[edge].guards;
				for (std::size_t g = 0; g < guards.size(); ++g)
				{
					if (guards[g].rawCheck.join != RawCondition::Join::None)
					{
						object->kind->collisionData.edges[edge].guards[g].check = makeTest(guards[g].rawCheck, where, where, objects);
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
			for (auto& [name, list] : object->kind->action) { tests(list, "object '" + rawObject.name + "' > <action name=\"" + name + "\">"); }
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

	Object game_expr::spawnObject(RawObject& raw, const Object& from)
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
		first.picture = from.looks.all().empty() ? from.drawn.picture : from.looks.all().front().picture;
		first.bitmap = from.looks.all().empty() ? from.drawn.bitmap : from.looks.all().front().bitmap;
		if (from.animation.frames.shows().bitmaps.empty())
		{
			if (!from.drawn.turnables.empty()) { first.turned = from.drawn.turnables.front(); }
		}
		else
		{
			first.animation = from.animation.frames.shows().bitmaps;
			first.animationTurned = from.drawn.turnables;
		}
		if (!from.looks.all().empty()) { first.looks = &from.looks.all(); }

		// Its palettes too, wearing the one `from` is (its looks are those).
		Object made = makeObject(raw, first, from.layout.cellOffset, from.kind->collisionData.lockstep);
		made.looks.set->palettes = from.looks.set->palettes;
		made.looks.worn = from.looks.worn;
		if (made.mirror.on) { made.mirror.flips = from.mirror.flips; }
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
				if (rawNote.decay.kind != RawValue::Kind::Expression || !rawNote.decay.text.empty())
				{
					note.decay = evaluate(rawNote.decay, here + " > <decay>");
					if (!(note.decay > 0.0f && note.decay <= kMaxNoteSeconds))
					{
						throw std::runtime_error(here + ": <decay> is " + formatDisplayNumber(note.decay) + " seconds; expected more than 0 and at most "
							+ std::to_string(static_cast<int>(kMaxNoteSeconds)));
					}
				}
			}

			sound.notes.push_back(note);
		}

		return sound;
	}

	// A <hitbox> worked out for `object` (its picture already made).
	Hitbox game_expr::makeHitbox(const RawHitbox& raw, const Object& object, const std::string& where)
	{
		if (object.kind->collisionData.type == CollisionType::Pixel)
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
			hitbox.round = object.drawn.picture.kind == ShapeKind::Circle;
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
		// self., while one object's test is worked out, is that object's.
		if (selfNow && text.compare(0, 5, "self.") == 0)
		{
			selfText.assign(*selfNow).append(text, 4, std::string::npos);
			return evaluateExpression(selfText, where);
		}
		if (otherNow && text.compare(0, 6, "other.") == 0)
		{
			selfText.assign(*otherNow).append(text, 5, std::string::npos);
			return evaluateExpression(selfText, where);
		}
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
				// One that says self is made for the object running it, each
				// time it runs; at load, for the one being made.
				const bool own = !selfName.empty() && namesSelf(each);
				const bool theirs = !otherName.empty() && namesOther(each);
				std::vector<RawCommand> named;
				if (own || theirs)
				{
					named.push_back(each);
					if (own) { nameSelfIn(named.front(), selfName); }
					if (theirs) { renameObjectIn(named, "other", otherName); }
				}
				const RawCommand& making = own || theirs ? named.front() : each;
				Command made = onLine(each.line, [&] { return makeCommand(making, [&](const RawValue& value) { return evaluate(value, where); }); });

				// One whose numbers can change is made again each time it runs
				// (CmdLive); <set> and <spawn> always are, since what they
				// put somewhere is the point of them.
				const bool live = own || theirs || each.verb == "set" || each.verb == "spawn"
					|| namesLive(making.amount) || namesLive(making.seconds) || namesLive(making.distance) || namesLive(making.ahead)
					|| (each.verb == "shift" && (namesLive(making.positionX) || namesLive(making.positionY)));
				if (live)
				{
					auto loaded = std::make_shared<LoadedCommand>();
					loaded->command = std::move(made);
					commands.push_back(CmdLive{ std::make_shared<const RawCommand>(each), std::move(loaded), where, own, theirs });
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
