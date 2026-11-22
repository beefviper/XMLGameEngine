// expr.cpp
// XML Game Engine
// author: beefviper
// date: Sept 21, 2020

#include "load/expr.h"
#include "verbs/verb.h"

#include "load/calc.h"
#include "load/check.h"
#include "load/reading.h"

#include "picture/color.h"
#include "engine/keys.h"
#include "load/spelling.h"
#include "picture/svg.h"
#include "play/numbers.h"

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
	bool showsNumbers(const WrittenSprite& sprite)
	{
		return std::any_of(sprite.text.begin(), sprite.text.end(),
			[](const WrittenTextPiece& piece) { return piece.kind != WrittenTextPiece::Kind::Words; });
	}

	namespace
	{
		// What a <holds class> holds: each member of a class some holder
		// holds is held (it moves from holder to holder and turns between
		// its first look and its second), worth its column unless it says,
		// and `high` a second worth it may count instead; a picker of one is
		// a hand.
		void makeHeld(std::vector<WrittenObject>& objects)
		{
			std::vector<std::string> held;
			for (const WrittenObject& object : objects)
			{
				if (!object.holder.holdsClass.empty() && std::find(held.begin(), held.end(), object.holder.holdsClass) == held.end()) { held.push_back(object.holder.holdsClass); }
			}
			if (held.empty()) { return; }
			const auto isHeld = [&](const std::string& objClass) { return !objClass.empty() && std::find(held.begin(), held.end(), objClass) != held.end(); };
			const auto number = [](const WrittenValue& value, const std::string& where, const char* what)
			{
				try { return std::stof(value.text); }
				catch (const std::exception&) { throw std::runtime_error(where + ": its " + what + " is \"" + value.text + "\"; expected a number"); }
			};
			for (WrittenObject& object : objects)
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
				object.variable["worth"] = WrittenValue::expression(formatDisplayNumber(worth));
				if (!object.looks.empty()) { object.sprite = object.looks.front(); }
			}
			for (WrittenObject& object : objects)
			{
				for (const auto& [action, commands] : object.action)
				{
					for (const WrittenCommand& command : commands)
					{
						if (command.verb != "pick" || !isHeld(command.objClass)) { continue; }
						object.hand = true;
						if (const WrittenPick* pick = std::get_if<WrittenPick>(&command.own); pick && pick->most > 0) { object.handHolds = pick->most; }
					}
				}
			}
		}

		// Each state's <starts>: every held member of a group sent into one
		// holder, then from there so many into each holder named (a group's:
		// each member's own), in random order.
		void placeStarts(std::vector<WrittenState>& states, std::vector<WrittenObject>& objects)
		{
			const auto holdersFor = [&](const std::string& name, const std::string& state)
			{
				std::vector<WrittenObject*> found;
				for (WrittenObject& object : objects)
				{
					if (object.holder.is && object.ownerState == state && (object.named(state + "." + name) || object.named(name))) { found.push_back(&object); }
				}
				return found;
			};
			for (WrittenState& state : states)
			{
				std::string start;   // the holder every member starts in
				for (const WrittenStart& step : state.starts)
				{
					const std::string where = "state '" + state.name + "' > <starts> > <" + step.tag() + ">";
					if (step.step == WrittenStart::Step::Send && step.all)
					{
						bool any = false;
						for (WrittenObject& object : objects)
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
					if (step.step == WrittenStart::Step::Order)
					{
						// What starts in it is shuffled; with no <order>, it lies in the pack's order.
						for (WrittenObject* each : holdersFor(step.from, state.name)) { each->holder.startShuffled = true; }
						continue;
					}
					std::vector<WrittenObject*> into = holdersFor(step.into, state.name);
					if (into.empty())
					{
						for (const WrittenObject& object : objects)
						{
							if (object.ownerState != state.name || object.handHolder.empty() || (object.groupName != step.into && object.groupName != state.name + "." + step.into)) { continue; }
							for (WrittenObject* own : holdersFor(object.handHolder, state.name)) { into.push_back(own); }
						}
					}
					if (into.empty()) { throw std::runtime_error(where + ": into=\"" + step.into + "\" names no holder of this state"); }
					for (WrittenObject* each : into)
					{
						each->holder.startCount = WrittenValue::expression(std::to_string(step.count));
						if (step.more != 0) { each->holder.startMore = WrittenValue::expression(std::to_string(step.more)); }
					}
				}
			}
			for (const WrittenObject& object : objects)
			{
				if (object.held && object.item.startHolder.empty())
				{
					throw std::runtime_error("object '" + object.name + "': of a held class, but no state's <starts> sends it into a holder");
				}
			}
		}

	}

	void Evaluator::init(const WindowDesc& windowDesc,
		const std::vector<std::pair<std::string, WrittenValue>>& writtenVariables, std::map<std::string, float>& variables,
		std::vector<WrittenState>& writtenStates, std::vector<State>& states,
		std::vector<WrittenObject>& writtenObjects, std::vector<Object>& objects,
		const std::vector<WrittenSound>& writtenSounds, std::vector<SoundDesc>& sounds,
		const std::vector<WrittenPath>& writtenPaths, std::map<std::string, Path>& paths)
	{
		seedRandom();
		makeHeld(writtenObjects);
		placeStarts(writtenStates, writtenObjects);

		checkColor(windowDesc.background, "<window> > <background>");

		nameNumbers(windowDesc, writtenVariables, variables, writtenObjects);
		makeObjects(writtenObjects, objects);
		makeRuleTests(writtenObjects, objects);
		makeStates(writtenStates, states, objects);
		makeSounds(writtenSounds, sounds);
		makePaths(writtenPaths, paths, states, objects);
		makePalettes(writtenObjects, objects, checkReferences(*parts, states, objects, sounds, paths, styles));
		for (Object& object : objects)
		{
			for (CollisionRule& rule : object.kind->collisionData.others) { std::vector<std::vector<Command>>().swap(rule.loadChecks); }
		}
		shareKinds(objects);
		shareLooks(objects);
	}

	void Evaluator::shareKinds(std::vector<Object>& objects)
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

	namespace
	{
		bool sameLook(const Object::Look& a, const Object::Look& b)
		{
			return a.name == b.name && a.picture == b.picture && a.bitmap == b.bitmap && a.frames.held == b.frames.held
				&& a.thenLook == b.thenLook && a.turnables == b.turnables;
		}

		bool sameLooks(const std::vector<Object::Look>& a, const std::vector<Object::Look>& b)
		{
			return std::equal(a.begin(), a.end(), b.begin(), b.end(), sameLook);
		}

		bool samePalette(const Object::Palette& a, const Object::Palette& b)
		{
			return a.name == b.name && a.bitmap == b.bitmap && a.animation == b.animation && sameLooks(a.looks, b.looks);
		}

		bool sameLookSet(const Object::LookSet& a, const Object::LookSet& b)
		{
			const bool sameFlips = a.flips == b.flips || (a.flips && b.flips && *a.flips == *b.flips);
			return a.first == b.first && a.hasPoses == b.hasPoses && a.poses == b.poses && a.hasJoins == b.hasJoins && a.joins == b.joins
				&& a.changed == b.changed && sameFlips && sameLooks(a.all, b.all)
				&& std::equal(a.palettes.begin(), a.palettes.end(), b.palettes.begin(), b.palettes.end(), samePalette);
		}
	}

	void Evaluator::shareLooks(std::vector<Object>& objects)
	{
		std::vector<std::shared_ptr<Object::LookSet>> distinct;
		for (Object& object : objects)
		{
			if (object.looks.set->all.empty()) { continue; }
			const auto found = std::find_if(distinct.begin(), distinct.end(), [&](const std::shared_ptr<Object::LookSet>& set) { return sameLookSet(*set, *object.looks.set); });
			if (found == distinct.end()) { distinct.push_back(object.looks.set); }
			else { object.looks.set = *found; }
		}
	}

	// init's names: the window's edges, the game's variables, every object's
	// variables and timers, and the live numbers, as names a text reads.
	void Evaluator::nameNumbers(const WindowDesc& windowDesc, const std::vector<std::pair<std::string, WrittenValue>>& writtenVariables, std::map<std::string, float>& variables, std::vector<WrittenObject>& writtenObjects)
	{
		// the window's edges and middles, then the game's variables, as names a text reads
		calc.constant("@window.bottom", windowDesc.height);
		calc.constant("@window.right", windowDesc.width);
		calc.constant("@window.width", windowDesc.width);
		calc.constant("@window.height", windowDesc.height);
		calc.constant("@window.centerx", windowDesc.width / 2);
		calc.constant("@window.centery", windowDesc.height / 2);

		// add variables from XML file to symbol table, in the order the file
		// declares them so that one can use the ones before it
		for (const auto& [name, writtenValue] : writtenVariables)
		{
			const float value = evaluate(writtenValue, "variable '" + name + "'");
			variables[name] = value;
			calc.constant(name, value);
		}

		// An object's named timer is a number of the object's (bomb.fuse), its
		// time left, so a text shows it and a rule or a value reads it as it
		// does any variable; its value is given when the object is made.
		for (auto& writtenObject : writtenObjects)
		{
			for (const WrittenTimer& timer : writtenObject.timers)
			{
				if (timer.name.empty()) { continue; }
				const std::string here = "object '" + writtenObject.name + "' > <timer name=\"" + timer.name + "\">";
				if (writtenObject.variable.count(timer.name))
				{
					throw std::runtime_error(here + ": the object has a <variable> of that name too, and " + writtenObject.name + "." + timer.name + " is the timer's time left");
				}
				const auto& liveNames = livePartNames();
				if (std::find(liveNames.begin(), liveNames.end(), timer.name) != liveNames.end())
				{
					throw std::runtime_error(here + ": " + writtenObject.name + "." + timer.name + " is already where the object is or how it moves; give the timer another name");
				}
				writtenObject.variable[timer.name] = WrittenValue::expression("0");
			}
			// A holder's <counts variable>: a variable of its own, kept to the total.
			const std::string& counts = writtenObject.holder.counts;
			if (!counts.empty() && !writtenObject.variable.count(counts)) { writtenObject.variable[counts] = WrittenValue::expression("0"); }
		}
		// A holder's top.<variable>: each variable its held class has, the top
		// one's while the game runs (Game::refreshHolder), so bag.top.shape reads.
		for (auto& holder : writtenObjects)
		{
			if (!holder.holder.is || holder.holder.holdsClass.empty()) { continue; }
			for (const auto& member : writtenObjects)
			{
				if (&member == &holder || member.objClass != holder.holder.holdsClass) { continue; }
				for (const auto& [name, value] : member.variable) { holder.variable.try_emplace("top." + name, WrittenValue::expression("0")); }
			}
		}

		// Every object's variables are named first, at 0, so a value may read
		// one whose object comes later in the file; the object loop below
		// gives each its real value.
		for (auto& writtenObject : writtenObjects)
		{
			for (auto& writtenVariable : writtenObject.variable)
			{
				objectVariables[writtenObject.name + "." + writtenVariable.first] = 0.0f;
			}
		}
		for (auto& objectVariable : objectVariables)
		{
			bind(objectVariable.first, objectVariable.second);
		}

		// The ones that draw no random number are worked out now as well, in
		// file order, so a sprite measured below can use them (a <circle> of
		// radius ball.radius). One that reads a size keeps 0 until the object loop.
		for (auto& writtenObject : writtenObjects)
		{
			for (auto& writtenVariable : writtenObject.variable)
			{
				if (writtenVariable.second.drawsRandom())
				{
					continue;
				}
				try
				{
					objectVariables[writtenObject.name + "." + writtenVariable.first] = evaluate(writtenVariable.second, "object '" + writtenObject.name + "'");
				}
				catch (const std::runtime_error&)
				{
					// worked out, or reported, in the object loop
				}
			}
		}

		// Where each object is and how it is going, and the touch a rule is
		// answering, as names; worked out when they are used (Game::evaluateNow).
		// An object's own variable of a part's name is read instead.
		liveNumbers.reserve(writtenObjects.size() * livePartNames().size());
		for (const WrittenObject& writtenObject : writtenObjects)
		{
			const std::string owner = Calc::nameOf(writtenObject.name) + ".";
			for (const std::string& part : livePartNames())
			{
				if (!writtenObject.variable.count(part)) { liveNumbers.try_emplace(owner + part, 0.0f); }
			}
		}
	}

	// init's objects: every sprite measured once (its size a name), the
	// cells of groups laid out, then each object made, groups in lockstep.
	void Evaluator::makeObjects(std::vector<WrittenObject>& writtenObjects, std::vector<Object>& objects)
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
		std::map<std::string, PictureId> firstPassBitmaps;
		std::map<std::string, std::shared_ptr<const Turnable>> firstPassTurned;
		std::map<std::string, std::vector<PictureId>> firstPassAnimation;
		std::map<std::string, std::vector<std::shared_ptr<const Turnable>>> firstPassAnimationTurned;
		for (auto& writtenObject : writtenObjects)
		{
			if (objectSizes.count(writtenObject.name))
			{
				continue; // measured already
			}

			const Picture picture = buildPicture(writtenObject.sprite, "object '" + writtenObject.name + "'", &firstPassBitmaps[writtenObject.name],
				writtenObject.writtenHeading ? &firstPassTurned[writtenObject.name] : nullptr, &writtenObject);
			objectShapeKinds[writtenObject.name] = picture.kind;
			objectSizes[writtenObject.name] = measureShapeSize(picture);
			firstPassPictures[writtenObject.name] = picture;

			if (writtenObject.hasAnimation)
			{
				firstPassAnimation[writtenObject.name] = buildAnimationBitmaps(writtenObject.animation.frames, "object '" + writtenObject.name + "' > <animation>",
					writtenObject.writtenHeading ? &firstPassAnimationTurned[writtenObject.name] : nullptr);
			}
		}
		for (auto& [name, size] : objectSizes)
		{
			if (!objectVariables.count(name + ".width")) { bind(name + ".width", size.x); }
			if (!objectVariables.count(name + ".height")) { bind(name + ".height", size.y); }
		}


		// Where each cell of a group laid out in <columns> and <rows> goes.
		const std::map<std::string, Vector2f> cellOffsets = layOutCells(writtenObjects, firstPassPictures);

		// Objects in lockstep share a number: every member of a <group> moves
		// as one block with the others of its group. A plain object that asks
		// for it is a block of one.
		int lockstepNum = 1;
		std::map<std::string, int> groupLockstep;

		// evaluate strings in objects
		for (auto& writtenObject : writtenObjects)
		{
			int thisLockstep = 0;
			if (writtenObject.writtenCollisionData.lockstep)
			{
				if (writtenObject.groupName.empty())
				{
					thisLockstep = lockstepNum;
				}
				else
				{
					const auto [entry, isFirstMember] = groupLockstep.try_emplace(writtenObject.groupName, lockstepNum);
					if (isFirstMember) { lockstepNum++; }
					thisLockstep = entry->second;
				}
			}

			FirstPass first;
			first.picture = firstPassPictures.at(writtenObject.name);
			first.bitmap = firstPassBitmaps[writtenObject.name];
			first.turned = firstPassTurned[writtenObject.name];
			first.animation = firstPassAnimation[writtenObject.name];
			first.animationTurned = firstPassAnimationTurned[writtenObject.name];
			const auto offset = cellOffsets.find(writtenObject.name);
			objects.push_back(onLine(writtenObject.line, [&] { return makeObject(writtenObject, first, offset == cellOffsets.end() ? Vector2f{} : offset->second, thisLockstep); }));


			if (writtenObject.writtenCollisionData.lockstep && writtenObject.groupName.empty()) {
				lockstepNum++;
			}
		}
	}

	// init's tests on collision rules, edge guards and actions' conditions,
	// once every object they can name is made.
	void Evaluator::makeRuleTests(const std::vector<WrittenObject>& writtenObjects, std::vector<Object>& objects)
	{
		// The checks of the objects' collision rules, now that every object
		// they can name is made.
		for (const WrittenObject& writtenObject : writtenObjects)
		{
			const auto object = std::find_if(objects.begin(), objects.end(), [&](const Object& each) { return each.name == writtenObject.name; });
			if (object == objects.end()) { continue; }
			const std::string where = "object '" + writtenObject.name + "' > <collision>";
			// Their tests say self of this one (makeTestHere).
			struct Self
			{
				std::string& name;
				std::string was;
				~Self() { name = std::move(was); }
			} self{ selfName, std::exchange(selfName, writtenObject.name) };
			for (std::size_t i = 0; i < writtenObject.writtenCollisionData.others.size(); ++i)
			{
				const WrittenCollisionRule& rule = writtenObject.writtenCollisionData.others[i];
				const WrittenCondition& check = rule.check;
				if (check.join == WrittenCondition::Join::None) { continue; }
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
			for (std::size_t edge = 0; edge < writtenObject.writtenCollisionData.edges.size(); ++edge)
			{
				const std::vector<EdgeGuard>& guards = writtenObject.writtenCollisionData.edges[edge].guards;
				for (std::size_t g = 0; g < guards.size(); ++g)
				{
					if (guards[g].writtenCheck.join != WrittenCondition::Join::None)
					{
						object->kind->collisionData.edges[edge].guards[g].check = makeTest(guards[g].writtenCheck, where, where, objects);
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
					when->body->test = makeTest(*when->body->written, here + " > <condition>", here, objects);
					tests(when->body->commands, here + " > <condition>");
				}
			};
			for (auto& [name, list] : object->kind->action) { tests(list, "object '" + writtenObject.name + "' > <action name=\"" + name + "\">"); }
		}
	}

	// init's states: each one's inputs, conditions, timers and scenes.
	void Evaluator::makeStates(std::vector<WrittenState>& writtenStates, std::vector<State>& states, const std::vector<Object>& objects)
	{
		for (auto& writtenState : writtenStates)
		{
			State state{};
			const std::string where = "state '" + writtenState.name + "'";

			state.name = writtenState.name;
			state.line = writtenState.line;
			state.show = NameStore::program().of(writtenState.show);
			state.showStates = NameStore::program().of(writtenState.showStates);
			state.music = writtenState.music;

			for (std::size_t moment = 0; moment < keyMoments.size(); ++moment)
			{
				for (auto& writtenAction : *writtenState.keyMaps()[moment])
				{
					const KeyCode key = keyCodeFromString(writtenAction.first);
					if (key == KeyCode::Unknown)
					{
						throw std::runtime_error(where + " > <input button=\"" + writtenAction.first + "\">: there is no key named '" + writtenAction.first + "'"
							+ didYouMean(writtenAction.first, keyNames()) + ". The keys are: " + listOf(keyNames()));
					}
					(*state.keyMaps()[moment])[key] = processCommands(writtenAction.second, where);
				}
			}

			for (auto& [name, commands] : writtenState.axes)
			{
				const Axis axis = axisFromString(name);
				if (axis == Axis::Count)
				{
					throw std::runtime_error(where + " > <input axis=\"" + name + "\">: there is no control named '" + name + "'"
						+ didYouMean(name, axisNames()) + ". The controls are: " + listOf(axisNames()));
				}
				state.axes[axis] = processCommands(commands, where);
			}

			for (auto& writtenCondition : flattened(writtenState.conditions))
			{
				state.conditions.push_back(makeCondition(writtenCondition, where, where, writtenState.name, objects));
			}
			for (auto& writtenCondition : flattened(writtenState.computer))
			{
				state.computer.push_back(makeCondition(writtenCondition, where + " > <computer>", where, writtenState.name, objects));
			}

			state.timers = processTimers(writtenState.timers, where);

			for (const WrittenScene& writtenScene : writtenState.scenes)
			{
				Scene scene;
				scene.name = writtenState.name + "." + writtenScene.name;
				const std::string sceneWhere = "scene '" + scene.name + "'";
				for (const auto& [way, to] : writtenScene.ways)
				{
					// A scene of the same state, by its name there or by its path.
					const auto leads = std::find_if(writtenState.scenes.begin(), writtenState.scenes.end(), [&](const WrittenScene& other)
						{
							return other.name == to || writtenState.name + "." + other.name == to;
						});
					if (leads == writtenState.scenes.end())
					{
						std::vector<std::string> names;
						for (const WrittenScene& other : writtenState.scenes) { names.push_back(other.name); }
						throw std::runtime_error(sceneWhere + ": <way name=\"" + way + "\" scene=\"" + to + "\" /> names no scene of state '" + writtenState.name + "'"
							+ didYouMean(to, names) + ". Its scenes are: " + listOf(names));
					}
					scene.ways[Name(way)] = Name(writtenState.name + "." + leads->name);
				}
				for (const WrittenCondition& writtenCondition : flattened(writtenScene.conditions))
				{
					scene.conditions.push_back(makeCondition(writtenCondition, sceneWhere, sceneWhere, writtenState.name, objects));
				}
				scene.timers = processTimers(writtenScene.timers, sceneWhere);
				state.scenes.push_back(std::move(scene));
			}

			states.push_back(state);
		}
	}

	// init's sounds, after everything else so that a length can use any variable.
	void Evaluator::makeSounds(const std::vector<WrittenSound>& writtenSounds, std::vector<SoundDesc>& sounds)
	{
		// The sounds, worked out after everything else so that a length can use
		// any variable.
		for (const WrittenSound& writtenSound : writtenSounds)
		{
			const bool taken = std::any_of(sounds.begin(), sounds.end(), [&](const SoundDesc& sound) { return sound.name == writtenSound.name; });
			if (taken)
			{
				throw std::runtime_error("sound '" + writtenSound.name + "': there is already a sound of that name");
			}
			sounds.push_back(processSound(writtenSound));
		}
	}

	// init's paths, worked out last too, and each step's commands about
	// its follower made for each object that can fly it.
	void Evaluator::makePaths(const std::vector<WrittenPath>& writtenPaths, std::map<std::string, Path>& paths, const std::vector<State>& states, const std::vector<Object>& objects)
	{
		// The paths, worked out last too: a step's numbers can use any variable.
		for (const WrittenPath& writtenPath : writtenPaths)
		{
			const std::string where = "path '" + writtenPath.name + "'";
			if (paths.count(writtenPath.name))
			{
				throw std::runtime_error(where + ": there is already a path of that name");
			}

			Path path;
			path.name = writtenPath.name;
			path.speed = evaluate(writtenPath.speed, where + " > <speed>");
			if (!(path.speed > 0.0f))
			{
				throw std::runtime_error(where + ": the <speed> is not above 0, so it would never get anywhere");
			}
			if (writtenPath.start)
			{
				path.start = Vector2f{ evaluate(writtenPath.start->x, where + " > <start>"), evaluate(writtenPath.start->y, where + " > <start>") };
			}
			for (const WrittenPathStep& writtenStep : writtenPath.steps)
			{
				PathStep step;
				step.home = writtenStep.home;
				if (!writtenStep.home)
				{
					// A half that names something that changes, or the
					// follower (self), is worked out as the step sets off.
					const auto half = [&](const WrittenValue& value, float& into, std::shared_ptr<const WrittenValue>& live)
					{
						const std::vector<const std::string*> texts = value.expressions();
						const bool own = std::any_of(texts.begin(), texts.end(),
							[](const std::string* text) { return text->find("@self.") != std::string::npos; });
						if (own || namesLive(value))
						{
							live = std::make_shared<const WrittenValue>(value);
							if (own) { return; }
							commandValues = true;
							evaluate(value, where + " > <step>");
							commandValues = false;
							return;
						}
						into = evaluate(value, where + " > <step>");
					};
					half(writtenStep.x, step.by.x, step.liveX);
					half(writtenStep.y, step.by.y, step.liveY);

					// Commands about the follower are made for each follower
					// (below); the rest once.
					std::vector<WrittenCommand> probe = writtenStep.commands;
					if (nameSelfIn(probe, "follower"))
					{
						step.ownWritten = std::make_shared<const std::vector<WrittenCommand>>(writtenStep.commands);
					}
					else
					{
						step.commands = processCommands(writtenStep.commands, where + " > <step>");
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
				if (!step.ownWritten) { continue; }
				for (const std::string& follower : followers)
				{
					step.ownBy[follower] = pathCommandsFor(step, follower, "path '" + name + "' > <step>, flown by '" + follower + "'");
				}
			}
		}
	}

	Object Evaluator::spawnObject(WrittenObject& written, const Object& from)
	{
		// What its rules and others' read of it, under its own name, as init
		// makes them known for every object the file has. A name used
		// before (a copy despawned, its name taken again) keeps its place.
		for (const auto& [variable, value] : written.variable)
		{
			const std::string name = written.name + "." + variable;
			if (objectVariables.emplace(name, 0.0f).second) { bind(name, objectVariables[name]); }
		}
		for (const std::string& part : livePartNames())
		{
			if (!written.variable.count(part)) { liveNumbers.try_emplace(Calc::nameOf(written.name) + "." + part, 0.0f); }
		}
		if (const auto size = objectSizes.find(from.name); size != objectSizes.end() && !objectSizes.count(written.name))
		{
			Vector2f& own = objectSizes[written.name] = size->second;
			objectShapeKinds[written.name] = objectShapeKinds[from.name];
			if (!objectVariables.count(written.name + ".width")) { bind(written.name + ".width", own.x); }
			if (!objectVariables.count(written.name + ".height")) { bind(written.name + ".height", own.y); }
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
		Object made = makeObject(written, first, from.layout.cellOffset, from.kind->collisionData.lockstep);
		made.looks.set->palettes = from.looks.set->palettes;
		made.looks.worn = from.looks.worn;
		if (made.mirror.on) { made.looks.set->flips = from.looks.set->flips; }
		return made;
	}

	SoundDesc Evaluator::processSound(const WrittenSound& written)
	{
		const std::string where = "sound '" + written.name + "'";

		SoundDesc sound;
		sound.name = written.name;
		sound.loop = written.loop;
		sound.path = written.path;

		const bool noVolume = written.volume.kind == WrittenValue::Kind::Expression && written.volume.text.empty();
		// A recording plays as loud as it was made unless told otherwise.
		if (!sound.path.empty()) { sound.volume = 1.0f; }
		if (!noVolume)
		{
			sound.volume = evaluate(written.volume, where + " > <volume>");
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
				// Read once, here, where a bad file is told; the audio plays
				// these samples.
				(void)resources.samples(sound);
			}
			catch (const std::runtime_error& error)
			{
				throw std::runtime_error(where + ": " + error.what());
			}
			return sound;
		}

		const Waveform soundWave = waveOf(written.wave, where);

		for (std::size_t i = 0; i < written.notes.size(); ++i)
		{
			const WrittenNote& writtenNote = written.notes[i];
			const std::string here = where + " > " + (writtenNote.rest ? "<rest>" : "<note>") + " " + std::to_string(i + 1);

			SoundNote note;
			note.rest = writtenNote.rest;
			note.part = writtenNote.part;
			note.seconds = evaluate(writtenNote.length, here);
			if (!(note.seconds > 0.0f && note.seconds <= kMaxNoteSeconds))
			{
				throw std::runtime_error(here + ": lasts " + std::to_string(note.seconds) + " seconds; expected more than 0 and at most "
					+ std::to_string(static_cast<int>(kMaxNoteSeconds)));
			}

			if (!writtenNote.rest)
			{
				note.wave = writtenNote.wave.empty() ? soundWave : waveOf(writtenNote.wave, here);
				note.startHz = pitchOf(writtenNote.pitch, "pitch", here);
				note.endHz = writtenNote.to.empty() ? note.startHz : pitchOf(writtenNote.to, "to", here);
				if (writtenNote.decay.kind != WrittenValue::Kind::Expression || !writtenNote.decay.text.empty())
				{
					note.decay = evaluate(writtenNote.decay, here + " > <decay>");
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
	Hitbox Evaluator::makeHitbox(const WrittenHitbox& written, const Object& object, const std::string& where)
	{
		if (object.kind->collisionData.type == CollisionType::Pixel)
		{
			throw std::runtime_error(where + ": a <hitbox> is touched as its own shape, not by the pixels of the picture; it cannot go with <type>pixel</type>");
		}
		const auto valueOr0 = [&](const WrittenValue& value) { return value.kind == WrittenValue::Kind::Expression && value.text.empty() ? 0.0f : evaluate(value, where); };
		Hitbox hitbox;
		hitbox.name = written.name;
		if (written.kind == "inset")
		{
			hitbox.inset = true;
			hitbox.top = valueOr0(written.top);
			hitbox.bottom = valueOr0(written.bottom);
			hitbox.left = valueOr0(written.left);
			hitbox.right = valueOr0(written.right);
			// Out from the picture, never; and something must be left.
			if (std::min({ hitbox.top, hitbox.bottom, hitbox.left, hitbox.right }) < 0.0f)
			{
				throw std::runtime_error(where + ": a <hitbox> pulls in from the picture's sides, never out; one side is " + formatDisplayNumber(std::min({ hitbox.top, hitbox.bottom, hitbox.left, hitbox.right })));
			}
			if (object.size.x > 0.0f && object.size.y > 0.0f && (hitbox.left + hitbox.right >= object.size.x || hitbox.top + hitbox.bottom >= object.size.y))
			{
				throw std::runtime_error(where + ": a <hitbox> pulled in that far leaves nothing of a picture " + formatDisplayNumber(object.size.x) + " by " + formatDisplayNumber(object.size.y));
			}
			hitbox.round = object.drawn.picture->kind == ShapeKind::Circle;
			return hitbox;
		}

		hitbox.round = written.kind == "circle";
		if (hitbox.round)
		{
			const float radius = evaluate(written.radius, where);
			hitbox.size = { radius * 2.0f, radius * 2.0f };
		}
		else
		{
			hitbox.size = { evaluate(written.width, where), evaluate(written.height, where) };
		}
		if (!(hitbox.size.x > 0.0f) || !(hitbox.size.y > 0.0f))
		{
			throw std::runtime_error(where + ": a <hitbox> must be bigger than nothing; its size is " + formatDisplayNumber(hitbox.size.x)
				+ " by " + formatDisplayNumber(hitbox.size.y));
		}
		hitbox.offset = { valueOr0(written.x), valueOr0(written.y) };
		return hitbox;
	}

	float Evaluator::evaluateStart(const WrittenValue& value, const std::string& where)
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

	float Evaluator::evaluate(const WrittenValue& value, const std::string& where)
	{
		switch (value.kind)
		{
		case WrittenValue::Kind::Random:
		{
			if (value.operations)
			{
				const WrittenOperation& ends = value.operations->front();
				// Where each end is written is made only for a message.
				const auto end = [&](const WrittenValue& written, const char* tag)
				{
					try { return evaluate(written, std::string{}); }
					catch (const std::runtime_error& error) { throw std::runtime_error(where + " > <random> > <" + tag + ">" + error.what()); }
				};
				const float low = end(ends.operands[0].value, "min");
				const float high = end(ends.operands[1].value, "max");
				return randomBetween(low, high);
			}
			const float min = evaluateExpression(value.min, where);
			const float max = evaluateExpression(value.max, where);
			return randomBetween(min, max);
		}

		case WrittenValue::Kind::Formula:
		{
			const float answer = evaluateOperation(value.operations->front(), where).value;
			if (loading && !std::isfinite(answer))
			{
				throw std::runtime_error(where + ": comes out as " + (std::isnan(answer) ? std::string("not a number") : std::string("infinity")));
			}
			return answer;
		}

		case WrittenValue::Kind::Centered:
			throw std::runtime_error(where + ": <centered> is only for an object's <position>");

		case WrittenValue::Kind::Expression:
			break;
		}

		return evaluateExpression(value.text, where);
	}

	bool Evaluator::isLate(const std::string& name) const
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

	Evaluator::Answer Evaluator::evaluateOperation(const WrittenOperation& operation, const std::string& where)
	{
		const std::string here = where + " > <" + operation.op + ">";

		const auto operandAnswer = [&](const WrittenOperand& operand)
		{
			if (operand.value.kind == WrittenValue::Kind::Expression)
			{
				return Answer{ evaluate(operand.value, here), isLate(operand.value.text) };
			}

			// An operation nested in another is part of the same sum.
			if (operand.value.kind == WrittenValue::Kind::Formula) { return evaluateOperation(operand.value.operations->front(), here); }

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

	float Evaluator::evaluateExpression(const std::string& text, const std::string& where)
	{
		// Each text is compiled once: a value worked out while the game runs
		// (a timer starting over, a command made again each time it runs)
		// only reads the compiled one again. The symbols it reads are bound
		// by reference, so it always sees the numbers as they are now.
		// self., while one object's test is worked out, is that object's.
		if (selfNow && text.compare(0, 6, "@self.") == 0)
		{
			selfText.assign(*selfNow).append(text, 5, std::string::npos);
			return evaluateExpression(selfText, where);
		}
		if (otherNow && text.compare(0, 7, "@other.") == 0)
		{
			selfText.assign(*otherNow).append(text, 6, std::string::npos);
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
				else if (const auto live = liveNumbers.find(name); live != liveNumbers.end() && !calc.has(name))
				{
					calc.variable(name, live->second);
					++liveRead;
					if (whenLiveRead) { whenLiveRead(); }
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

	void Evaluator::bind(const std::string& name, float& value)
	{
		unbound.emplace(Calc::nameOf(name), &value);
	}

	const std::vector<std::string>& Evaluator::namesIn(const std::string& text) const
	{
		auto found = namesInText.find(text);
		if (found == namesInText.end())
		{
			found = namesInText.emplace(text, Calc::namesIn(text)).first;
		}
		return found->second;
	}

	bool Evaluator::namesLive(const WrittenValue& value) const
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


	std::vector<Timer> Evaluator::processTimers(const std::vector<WrittenTimer>& written, const std::string& where)
	{
		std::vector<Timer> timers;
		const std::string here = where + " > <timer>";

		for (const WrittenTimer& writtenTimer : written)
		{
			Timer timer;
			timer.repeat = writtenTimer.repeat;
			timer.interval = writtenTimer.interval;
			timer.commands = processCommands(writtenTimer.commands, here);

			// Worked out once now so that a mistake in it is a load error, with
			// the random generator put back after so the game's draws are not
			// used up. Only a plain number can be held to being above 0 now: an
			// expression may read a variable that has no value yet.
			const std::mt19937 drawn = randomState();
			commandValues = true;
			const float seconds = evaluate(writtenTimer.interval, here);
			commandValues = false;
			setRandomState(drawn);
			char* end = nullptr;
			const std::string& text = writtenTimer.interval.text;
			const bool plainNumber = writtenTimer.interval.kind == WrittenValue::Kind::Expression && !text.empty()
				&& (std::strtof(text.c_str(), &end), end == text.c_str() + text.size());
			// An <after> of 0 goes off on the first frame it counts (a ghost
			// told what to do the moment it turns blue); an <every> of 0 would
			// be every frame, which a frame-counting timer cannot say.
			if (plainNumber && (writtenTimer.repeat ? !(seconds > 0.0f) : seconds < 0.0f))
			{
				throw std::runtime_error(here + ": " + (writtenTimer.repeat ? "<every> is " + std::to_string(seconds) + "; expected a number of seconds above 0"
					: "<after> is " + std::to_string(seconds) + "; expected a number of seconds, 0 or more"));
			}
			timer.sprite = writtenTimer.sprite;
			timer.name = writtenTimer.name;
			timer.seconds = seconds;

			timers.push_back(std::move(timer));
		}

		return timers;
	}

	std::vector<Command> Evaluator::pathCommandsFor(const PathStep& step, const std::string& follower, const std::string& where)
	{
		if (!step.ownWritten) { return {}; }
		std::vector<WrittenCommand> own = *step.ownWritten;
		nameSelfIn(own, follower);
		return processCommands(own, where);
	}

	std::vector<Command> Evaluator::processCommands(const std::vector<WrittenCommand>& written, const std::string& where)
	{
		std::vector<Command> commands;
		commandValues = true;
		try
		{
			for (const WrittenCommand& each : written)
			{
				// A condition in an action: its commands now, its test once
				// every object is made (testActionConditions).
				if (each.verb == "condition" && each.test)
				{
					auto body = std::make_shared<WhenBody>();
					body->written = each.test;
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
				std::vector<WrittenCommand> named;
				if (own || theirs)
				{
					named.push_back(each);
					if (own) { nameSelfIn(named.front(), selfName); }
					if (theirs) { renameObjectIn(named, "@other", otherName); }
				}
				const WrittenCommand& making = own || theirs ? named.front() : each;
				Command made = onLine(each.line, [&] { return makeCommand(making, [&](const WrittenValue& value) { return evaluate(value, where); }); });

				// One whose numbers can change is made again each time it runs
				// (CmdLive), and so is one whose verb says so (Verb::alwaysLive).
				bool readsLive = false;
				making.forEachValue([&](const WrittenValue& value) { readsLive = readsLive || namesLive(value); });
				const bool live = own || theirs || alwaysLive(each) || readsLive;
				if (live)
				{
					auto loaded = std::make_shared<LoadedCommand>();
					loaded->command = std::move(made);
					commands.push_back(CmdLive{ std::make_shared<const WrittenCommand>(each), std::move(loaded), where, own, theirs });
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

	void Evaluator::setObjectSize(const std::string& name, const Vector2f& size)
	{
		if (auto it = objectSizes.find(name); it != objectSizes.end())
		{
			it->second = size;
		}
	}

	std::vector<std::string> Evaluator::sizeDependenciesOf(const WrittenObject& writtenObject) const
	{
		std::vector<std::string> dependencies;

		// Every identifier in the position expressions, e.g. "title.width" or
		// "@window.centerx"; the ones that end in .width / .height and name
		// an object whose size needs a backend are what this is looking for.
		std::vector<const std::string*> positionExpressions = writtenObject.writtenPosition.x.expressions();
		for (const std::string* yExpression : writtenObject.writtenPosition.y.expressions())
		{
			positionExpressions.push_back(yExpression);
		}

		for (const std::string* positionExpression : positionExpressions)
		{
			std::size_t i = 0;
			while (i < positionExpression->size())
			{
				const auto isIdentifierChar = [](char c) { return std::isalnum(static_cast<unsigned char>(c)) || c == '_' || c == '.' || c == '@'; };
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
