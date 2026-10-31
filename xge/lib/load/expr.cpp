// expr.cpp
// XML Game Engine
// author: beefviper
// date: Sept 21, 2020

#include "load/expr.h"

#include "load/calc.h"
#include "load/check.h"

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

		// the window's edges and middles, then the game's variables, as names a text reads
		calc.constant("window.top", 0);
		calc.constant("window.bottom", windowDesc.height);
		calc.constant("window.left", 0);
		calc.constant("window.right", windowDesc.width);
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
			objects.push_back(makeObject(rawObject, first, windowDesc, offset == cellOffsets.end() ? Vector2f{} : offset->second, thisLockstep));


			if (rawObject.rawCollisionData.lockstep && rawObject.groupName.empty()) {
				lockstepNum++;
			}
		}

		// A condition made ready, from a state's or a scene's.
		const auto matches = [&](const RawCondition& rawCondition, const Object& object)
		{
			return (rawCondition.filterClass.empty() || rawCondition.filterClass == object.objClass)
				&& (rawCondition.filterObject.empty() || object.named(rawCondition.filterObject));
		};

		// What a <condition> tests, made ready (with its <all> or
		// <any>'s checks, each made the same way).
		std::function<Condition(const RawCondition&, const std::string&, const std::string&)> makeTest;
		makeTest = [&](const RawCondition& rawCondition, const std::string& where, const std::string& owner)
		{
			Condition condition;
			condition.join = rawCondition.join;
			for (std::size_t i = 0; i < rawCondition.parts.size(); ++i)
			{
				const RawCondition& part = rawCondition.parts[i];
				const char* kind = part.join == RawCondition::Join::All ? "<all>" : part.join == RawCondition::Join::Any ? "<any>" : "<condition>";
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

			// A number that names something that changes is worked out again
			// each time the condition is tested (Game::testCondition); now, it
			// may read where things are as 0, as a command's values may.
			const bool live = namesLive(rawCondition.threshold);
			commandValues = live;
			const float threshold = evaluate(rawCondition.threshold, where);
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
			case RawCondition::Test::Remaining: condition.remaining = threshold; break;
			}
			condition.value = threshold;

			if (!condition.distanceTo.empty() && !findObject(objects, condition.distanceTo))
			{
				throw std::runtime_error(where + ": distance=\"" + condition.distanceTo + "\" names no object or group" + didYouMean(condition.distanceTo, objectNames(objects)));
			}

			// A total of a variable: some object it can count has it.
			if ((condition.total == Total::Sum || condition.total == Total::Highest || condition.total == Total::Lowest)
				&& condition.distanceTo.empty()
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

		makePalettes(rawObjects, objects, checkReferences(states, objects, sounds, paths, styles));
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

	Object game_expr::makeObject(RawObject& rawObject, const FirstPass& first, const WindowDesc& windowDesc, Vector2f cellOffset, int thisLockstep)
	{
		const std::string where = "object '" + rawObject.name + "'";

		// Only a text showing a number needs working out again here, for the
		// value it has by now (see above).
		const Picture picture = (rawObject.sprite.kind == "text" && showsNumbers(rawObject.sprite))
			? buildPicture(rawObject.sprite, where, nullptr, nullptr, &rawObject)
			: first.picture;

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
			for (const RawSprite& rawLook : rawObject.looks)
			{
				Object::Look look;
				look.name = rawLook.name;
				std::shared_ptr<const Turnable> turned;
				look.picture = buildPicture(rawLook, where + " > <sprite name=\"" + rawLook.name + "\">", &look.bitmap,
					rawObject.hasHeading ? &turned : nullptr);
				if (turned) { look.turnables = { turned }; }
				if (!rawLook.frames.empty())
				{
					// A look that moves: its pictures, all one size, and how long each shows.
					const std::string here = where + " > <animation name=\"" + rawLook.name + "\">";
					look.turnables.clear();
					look.frames = buildAnimationBitmaps(rawLook.frames, here, rawObject.hasHeading ? &look.turnables : nullptr);
					look.frameTicks = animationFramesOf(rawLook.interval, windowDesc, here);
				}
				if (look.picture.kind == ShapeKind::Text && showsNumbers(rawLook))
				{
					throw std::runtime_error(where + ": a look cannot be a text showing a <number> or a <letter>");
				}
				looks.push_back(std::move(look));
			}

			// The first look is the sprite the object starts with; it must be
			// the very same picture.
			looks.front().picture = picture;
			looks.front().bitmap = first.bitmap;
			if (!looks.front().frames.empty()) { looks.front().frames.front() = first.bitmap; }
			if (first.turned && looks.front().frames.empty()) { looks.front().turnables = { first.turned }; }
		}


		Object object{};

		object.picture = picture;
		object.bitmap = first.bitmap;
		if (first.turned) { object.turnables = { first.turned }; }

		if (rawObject.hasAnimation)
		{
			object.animationBitmaps = first.animation;
			if (!first.animationTurned.empty()) { object.turnables = first.animationTurned; }
			object.animationFrames = animationFrames;
			object.bitmap = object.animationBitmaps.front();
		}

		if (object.picture.kind == ShapeKind::Text && showsNumbers(rawObject.sprite))
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
		object.start.ownerScene = rawObject.ownerScene;

		if (rawObject.objClass == "projectile")
		{
			rawObject.isVisible = false;
		}

		object.isVisible = rawObject.isVisible;
		object.exists = rawObject.exists;
		object.start.exists = rawObject.exists;
		// One that does not exist is in nothing that looks for what is out
		// there, as one taken out of play is not.
		if (!object.exists) { object.isVisible = false; }
		object.most = rawObject.most;

		// Its variables first, so its position and velocity can use them
		// (a serve drawn as an angle: its speed times the cosine of its angle).
		for (auto& rawVariable : rawObject.variable)
		{
			const float value = evaluate(rawVariable.second, where);
			object.variable[rawVariable.first] = value;
			object.start.variable[rawVariable.first] = value;

			// Keep the cross-object symbol table entry (registered above,
			// before any expression compiled) up to date with the real
			// value now that it's known.
			objectVariables[rawObject.name + "." + rawVariable.first] = value;
		}

		object.startDrawsRandom = rawObject.rawPosition.x.drawsRandom() || rawObject.rawPosition.y.drawsRandom()
			|| rawObject.rawVelocity.x.drawsRandom() || rawObject.rawVelocity.y.drawsRandom()
			|| std::any_of(rawObject.variable.begin(), rawObject.variable.end(), [](const auto& variable) { return variable.second.drawsRandom(); });

		object.start.position.x = evaluate(rawObject.rawPosition.x, where);
		object.start.position.y = evaluate(rawObject.rawPosition.y, where);

		// A cell is its group's position and its place in the
		// group; start.position keeps both, so a reset puts each
		// cell (each brick) back in its own place.
		object.cellOffset = cellOffset;
		object.position = object.start.position + object.cellOffset;
		object.start.position = object.position;

		object.sizeDependencies = positionSizeDependencies;
		object.positionUsesSize = !positionSizeDependencies.empty();
		object.positionResolved = !object.positionUsesSize;

		object.velocity.x = evaluate(rawObject.rawVelocity.x, where);
		object.velocity.y = evaluate(rawObject.rawVelocity.y, where);

		object.start.velocity = object.velocity;

		if (rawObject.hasAcceleration)
		{
			object.acceleration.x = evaluate(rawObject.rawAcceleration.x, where);
			object.acceleration.y = evaluate(rawObject.rawAcceleration.y, where);
		}
		object.start.acceleration = object.acceleration;

		if (rawObject.hasHeading)
		{
			object.hasHeading = true;
			float degrees = std::fmod(evaluate(rawObject.rawHeading, where), 360.0f);
			if (degrees < 0.0f) { degrees += 360.0f; }
			object.heading = degrees;
		}
		object.start.heading = object.heading;

		if (rawObject.hasDrag)
		{
			object.drag = evaluate(rawObject.rawDrag, where);
			if (object.drag < 0.0f || object.drag >= 1.0f)
			{
				throw std::runtime_error(where + ": <drag> is " + std::to_string(object.drag) + "; expected 0 up to (not including) 1");
			}
		}

		if (rawObject.hasSurface)
		{
			object.isSurface = true;
			if (rawObject.hasSurfaceDrag)
			{
				object.surfaceDrag = evaluate(rawObject.rawSurfaceDrag, where + " > <surface> > <drag>");
				if (*object.surfaceDrag < 0.0f || *object.surfaceDrag >= 1.0f)
				{
					throw std::runtime_error(where + " > <surface> > <drag> is " + formatDisplayNumber(*object.surfaceDrag) + "; expected 0 up to (not including) 1");
				}
			}
			if (rawObject.hasSurfaceGrip)
			{
				object.surfaceGrip = evaluate(rawObject.rawSurfaceGrip, where + " > <surface> > <grip>");
				if (!(*object.surfaceGrip > 0.0f && *object.surfaceGrip <= 1.0f))
				{
					throw std::runtime_error(where + " > <surface> > <grip> is " + formatDisplayNumber(*object.surfaceGrip) + "; expected above 0, up to 1");
				}
			}
		}

		if (rawObject.hasFriction)
		{
			object.friction = evaluate(rawObject.rawFriction, where);
			if (object.friction < 0.0f)
			{
				throw std::runtime_error(where + ": <friction> is " + std::to_string(object.friction) + "; expected 0 or more");
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
			object.start.light = object.light;
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
			object.start.altitude = object.altitude;
			object.start.rise = object.rise;
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
		object.start.facing = object.facing;
		object.timers = processTimers(rawObject.timers, where);
		for (const Timer& timer : object.timers)
		{
			if (timer.name.empty()) { continue; }
			// A named timer's time left (see init): where it starts.
			const float left = std::ceil(timer.seconds);
			object.variable[timer.name] = left;
			object.start.variable[timer.name] = left;
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
			object.poseLooks[static_cast<std::size_t>(Object::Pose::Thrusting)] = lookOf("thrusting").value_or(standing);
			object.hasPoses = true;
			object.firstLook = standing;
			if (standing != 0) { object.showLook(standing); }
		}

		// The look for where the piece it is joined to lies.
		if (!rawObject.joins.empty())
		{
			std::size_t at = 0;
			for (const char* join : { "alone", "left", "right", "up", "down" })
			{
				const std::string& name = rawObject.joins.at(join);
				const auto look = std::find_if(looks.begin(), looks.end(), [&](const Object::Look& each) { return each.name == name; });
				if (look == looks.end())
				{
					std::vector<std::string> names;
					for (const Object::Look& each : looks) { names.push_back(each.name); }
					throw std::runtime_error(where + " > <joins> > <" + join + " sprite=\"" + name + "\">: the object has no look of that name"
						+ (names.empty() ? std::string(" (it has no looks)") : didYouMean(name, names) + ". Its looks are: " + listOf(names)));
				}
				object.joinLooks[at++] = static_cast<std::size_t>(look - looks.begin());
			}
			object.hasJoins = true;
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
			object.homeMap = rawObject.cell.map;
			object.chain = rawObject.chain;
			object.mapColumn = rawObject.cell.mapColumn - 1;
			object.mapRow = rawObject.cell.mapRow - 1;
			object.mapColumns = rawObject.cell.mapColumns;
			object.mapRows = rawObject.cell.mapRows;
			object.mapTile = { evaluate(rawObject.cell.tileWidth, where), evaluate(rawObject.cell.tileHeight, where) };
			object.mapOrigin = object.start.position - object.cellOffset;

			// On a cell map: the cell it is in, its map's checks, and a
			// shape's cells, each the object its character picks.
			object.onCells = rawObject.cell.cells;
			if (object.onCells)
			{
				object.cellColumn = object.start.cellColumn = static_cast<float>(rawObject.cell.mapColumn);
				object.cellRow = object.start.cellRow = static_cast<float>(rawObject.cell.mapRow);
				for (const RawCheck& check : rawObject.cell.checks)
				{
					object.cellChecks.push_back({ check.kind, check.objClass, processCommands(check.commands, "map '" + rawObject.cell.map + "' > <checks> > <" + check.kind + ">"), check.count, check.straight, check.alike });
				}
			}
			for (std::size_t row = 0; row < rawObject.shapeRows.size(); ++row)
			{
				const std::string& cells = rawObject.shapeRows[row];
				object.shapeSize = std::max({ object.shapeSize, static_cast<int>(cells.size()), static_cast<int>(rawObject.shapeRows.size()) });
				for (std::size_t column = 0; column < cells.size(); ++column)
				{
					const auto piece = rawObject.shapePieces.find(cells[column]);
					if (piece == rawObject.shapePieces.end()) { continue; }
					object.shapeCells.push_back({ static_cast<int>(column), static_cast<int>(row), piece->second });
				}
			}
			if (object.isShape() && object.shapeCells.empty())
			{
				throw std::runtime_error(where + " > <shape>: none of its characters picks an object of map '" + rawObject.cell.map + "'");
			}
			object.shapeStart = object.shapeCells;
		}
		object.mazeMap = rawObject.maze.map;
		object.mazeWalls = rawObject.maze.walls;
		object.mazeSlows = rawObject.maze.slows;
		for (const MazeWall& wall : object.mazeWalls)
		{
			if (!wall.sprite.empty() && std::none_of(looks.begin(), looks.end(), [&](const Object::Look& look) { return look.name == wall.sprite; }))
			{
				throw std::runtime_error(where + " > <maze> > <wall class=\"" + wall.objClass + "\" sprite=\"" + wall.sprite + "\">: the object has no look of that name");
			}
		}

		object.showHeading();

		// <pivot>: the point it turns about, from the middle of the picture as
		// drawn; its <position> is where that point is on the screen, so the
		// square of the turned picture goes where puts it there.
		if (rawObject.hasPivot)
		{
			if (object.turnables.empty() || !object.bitmap)
			{
				throw std::runtime_error(where + ": <pivot> is the point a <heading> turns a picture of <line>s or a <bitmap> about; it has no such picture that turns");
			}
			const Turnable& drawn = *object.turnables.front();
			Vector2f middle;
			if (!drawn.lines.empty())
			{
				float left = drawn.lines.front().x1, right = left, top = drawn.lines.front().y1, bottom = top;
				for (const LineSegment& line : drawn.lines)
				{
					for (const float x : { line.x1, line.x2 }) { left = std::min(left, x); right = std::max(right, x); }
					for (const float y : { line.y1, line.y2 }) { top = std::min(top, y); bottom = std::max(bottom, y); }
				}
				middle = { (left + right) / 2.0f, (top + bottom) / 2.0f };
			}
			else
			{
				middle = { static_cast<float>(drawn.picture->width) / 2.0f, static_cast<float>(drawn.picture->height) / 2.0f };
			}
			const Vector2f pivot{ evaluate(rawObject.rawPivot.x, where + " > <pivot>"), evaluate(rawObject.rawPivot.y, where + " > <pivot>") };
			object.hasPivot = true;
			object.pivotFromMiddle = pivot - middle;
			object.pivotDegrees = wholeDegrees(object.heading);
			const Vector2f square{ static_cast<float>(object.bitmap->width), static_cast<float>(object.bitmap->height) };
			object.position = object.position - square * 0.5f - turnedBy(object.pivotFromMiddle, static_cast<float>(object.pivotDegrees));
			object.start.position = object.position;
			object.start.pivotDegrees = object.pivotDegrees;
		}

		object.collisionsOn = rawObject.rawCollisionData.enabled;
		object.start.isVisible = object.isVisible;
		object.start.collisionsOn = object.collisionsOn;
		object.collisionData.lockstep = thisLockstep;
		object.collisionData.type = rawObject.rawCollisionData.type;

		// A pixel collision tests the pixels that are drawn; a line
		// drawing has them from the start, and a circle or rectangle
		// is solid all over, but what a text or an image looks like
		// is only known once a window backend has drawn it.
		if (object.collisionData.type == CollisionType::Pixel
			&& (object.picture.kind == ShapeKind::Text || object.picture.kind == ShapeKind::Image))
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

		for (std::size_t edge = 0; edge < object.collisionData.edges.size(); ++edge)
		{
			const EdgeRules<RawCommand>& raw = rawObject.rawCollisionData.edges[edge];
			object.collisionData.edges[edge] = { processCommands(raw.commands, where), raw.guards };
			for (const EdgeGuard& edgeGuard : raw.guards)
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
			rule.throughClass = rawRule.throughClass;
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

		for (auto& rawAction : rawObject.action)
		{
			object.action[rawAction.first] = processCommands(rawAction.second, where);
		}

		// How it started is how it was before its first frame.
		object.before = object.start;

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

		// One operand: the answer is worked out from it alone.
		const float x = answer.value;
		if (operation.op == "absolute") { answer.value = std::fabs(x); return answer; }
		if (operation.op == "negate") { answer.value = -x; return answer; }
		if (operation.op == "sign") { answer.value = x > 0.0f ? 1.0f : x < 0.0f ? -1.0f : 0.0f; return answer; }
		if (operation.op == "floor") { answer.value = std::floor(x); return answer; }
		if (operation.op == "ceiling") { answer.value = std::ceil(x); return answer; }
		if (operation.op == "round") { answer.value = std::round(x); return answer; }

		// Of an angle in degrees, like every angle in a game.
		if (operation.op == "sine" || operation.op == "cosine")
		{
			constexpr float kRadiansPerDegree = 3.14159265358979323846f / 180.0f;
			const float radians = x * kRadiansPerDegree;
			answer.value = operation.op == "sine" ? std::sin(radians) : std::cos(radians);
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
		return Calc::run(found->second);
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
				Command made = makeCommand(each, [&](const RawValue& value) { return evaluate(value, where); });

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
		if (sprite.kind == "line" || sprite.kind == "polygon" || sprite.kind == "bitmap" || sprite.kind == "svg")
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
			picture.radius = evaluate(sprite.radius, where);
		}
		else if (sprite.kind == "rectangle")
		{
			picture.kind = ShapeKind::Rectangle;
			picture.size.x = evaluate(sprite.width, where);
			picture.size.y = evaluate(sprite.height, where);
		}
		else if (sprite.kind == "text")
		{
			// Its pieces: fixed labels, numbers shown as the whole numbers they
			// are, with no decimal point, the way someone typing the label
			// would, and letters.
			picture.kind = ShapeKind::Text;
			picture.text = joinText(textPieces(sprite, where, self));
			picture.textSize = static_cast<int>(evaluate(sprite.size, where));
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

			if (frame.kind != "bitmap" && frame.kind != "svg" && frame.kind != "line")
			{
				throw std::runtime_error(here + ": a frame must be a <bitmap>, an <svg> or a drawing of <line>s, not a <" + frame.kind + ">");
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
			const Vector2f own = measureShapeSize(pictures.at(rawObject.name));
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
				const Picture slotPicture = buildPicture(first.cell.slot, where);
				const ShapeKind slotKind = slotPicture.kind;
				const Vector2f slot = measureShapeSize(slotPicture);
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

					const Vector2f own = measureShapeSize(pictures.at(cell->name));
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
