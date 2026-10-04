// game_expr.cpp
// XML Game Engine
// author: beefviper
// date: Sept 21, 2020

#include "game_expr.h"

#include "keycode.h"
#include "svg.h"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstdlib>
#include <memory>
#include <optional>
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
					return object.baseName == name || (!object.groupName.empty() && object.groupName == name);
				});
			return made == objects.end() ? nullptr : &*made;
		}

		// Every state, object and action a command names has to exist. Found
		// here, when the game loads, rather than the first time the command
		// runs: pressing a key bound to <push state="pasued" /> would otherwise
		// stop the game in the middle of play (and used to read past the end of
		// the list of states).
		void checkCommand(const Command& command, const std::vector<State>& states, const std::vector<Object>& objects,
			const std::vector<SoundDesc>& sounds, const std::string& where)
		{
			if (const auto* become = std::get_if<CmdBecome>(&command))
			{
				if (!become->target.empty())
				{
					bool any = false;
					for (const Object& object : objects)
					{
						const bool named = object.name == become->target || object.baseName == become->target
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
						throw std::runtime_error(where + ": <become> names '" + become->target + "', and there is no object of that name");
					}
				}
			}
			else if (const auto* reveal = std::get_if<CmdReveal>(&command))
			{
				if (!findObject(objects, reveal->target))
				{
					throw std::runtime_error(where + ": <reveal> names '" + reveal->target + "', and there is no object of that name");
				}
			}
			else if (const auto* play = std::get_if<CmdPlay>(&command))
			{
				const bool known = std::any_of(sounds.begin(), sounds.end(), [&](const SoundDesc& sound) { return sound.name == play->sound; });
				if (!known)
				{
					throw std::runtime_error(where + ": <play sound=\"" + play->sound + "\" /> names no sound of the game");
				}
			}
			else if (const auto* push = std::get_if<CmdPushState>(&command))
			{
				const bool known = std::any_of(states.begin(), states.end(), [&](const State& state) { return state.name == push->name; });
				if (!known)
				{
					throw std::runtime_error(where + ": <push state=\"" + push->name + "\" /> names no state of the game");
				}
			}
			else if (const auto* trigger = std::get_if<CmdTriggerAction>(&command))
			{
				const Object* object = findObject(objects, trigger->object);
				if (!object)
				{
					throw std::runtime_error(where + ": an action of '" + trigger->object + "' is asked for, and there is no object of that name");
				}
				if (!object->action.count(trigger->action))
				{
					throw std::runtime_error(where + ": '" + trigger->object + "' has no action named '" + trigger->action + "'");
				}
			}
			else if (const auto* fire = std::get_if<CmdFire>(&command))
			{
				if (!findObject(objects, fire->projectileName))
				{
					throw std::runtime_error(where + ": <fire> names '" + fire->projectileName + "', and there is no object of that name");
				}
			}
			else if (const auto* release = std::get_if<CmdRelease>(&command))
			{
				if (!findObject(objects, release->target))
				{
					throw std::runtime_error(where + ": <release> names '" + release->target + "', and there is no object of that name");
				}
			}
			else if (const auto* reset = std::get_if<CmdResetObject>(&command))
			{
				if (!findObject(objects, reset->target))
				{
					throw std::runtime_error(where + ": <reset object=\"" + reset->target + "\" /> names no object of the game");
				}
			}
		}

		void checkReferences(const std::vector<State>& states, const std::vector<Object>& objects, const std::vector<SoundDesc>& sounds)
		{
			const auto checkAll = [&](const std::vector<Command>& commands, const std::string& where)
			{
				for (const auto& command : commands)
				{
					checkCommand(command, states, objects, sounds, where);
				}
			};

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
				for (const auto& [key, commands] : state.input) { checkAll(commands, where); checkOwnLooks(commands, nullptr, where); }
				for (const auto& condition : state.conditions) { checkAll(condition.commands, where); checkOwnLooks(condition.commands, nullptr, where); }
				for (const auto& timer : state.timers) { checkAll(timer.commands, where + " > <timer>"); checkOwnLooks(timer.commands, nullptr, where + " > <timer>"); }
			}

			for (const auto& object : objects)
			{
				const std::string where = "object '" + object.baseName + "'";
				for (const auto& timer : object.timers) { checkAll(timer.commands, where + " > <timer>"); checkOwnLooks(timer.commands, &object, where + " > <timer>"); }
				for (const auto& [name, commands] : object.action) { checkOwnLooks(commands, &object, where); }
				for (const auto& rule : object.collisionData.basic) { checkOwnLooks(rule.commands, &object, where); }
				for (const auto* edge : { &object.collisionData.top, &object.collisionData.bottom, &object.collisionData.left, &object.collisionData.right })
				{
					checkOwnLooks(*edge, &object, where);
				}
				for (const auto& [name, commands] : object.action)
				{
					checkAll(commands, where);

					// Turning and thrust along the heading mean nothing to an object
					// that does not face any way.
					for (const auto& command : commands)
					{
						if ((std::holds_alternative<CmdTurn>(command) || std::holds_alternative<CmdThrust>(command)) && !object.hasHeading)
						{
							throw std::runtime_error(where + ": action '" + name + "' turns it or thrusts along its heading, and it has no <heading>");
						}
					}
				}
				for (const auto& rule : object.collisionData.basic) { checkAll(rule.commands, where); }
			}
		}
	}

	void game_expr::init(const WindowDesc& windowDesc,
		const std::vector<std::pair<std::string, RawValue>>& rawVariables, std::map<std::string, float>& variables,
		std::vector<RawState>& rawStates, std::vector<State>& states,
		std::vector<RawObject>& rawObjects, std::vector<Object>& objects,
		const std::vector<RawSound>& rawSounds, std::vector<SoundDesc>& sounds)
	{
		generator.seed(seed());

		// add constants to symbol table
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
				continue; // every cell of a <grid> comes from this one object
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

		// Objects in lockstep share a number: every cell of a <grid>, and every
		// member of a <group>, moves as one block with the others of its own
		// grid or group. A plain object that asks for it is a block of one.
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
			const GridData gridData = gridDataOf(rawObject.sprite, where);
			const ShapeKind rawObjectShapeKind = shapeKindFromTag(tempSpriteParams.empty() ? std::string{} : tempSpriteParams.at(0));

			// Every grid cell shares the same footprint (one <circle> or
			// <rectangle> covers the whole <grid> - see games/breakout.xml,
			// games/spaceinvaders.xml), so this is computed
			// once per rawObject rather than per cell.
			const Vector2f gridObjSize = measureShapeSize(tempSpriteParams, rawObjectShapeKind);

			// A position that uses the size of a text or image (its own, or
			// another object's) can only be finished once a backend has measured
			// it - see Object::positionUsesSize. Sizes of shapes are exact
			// already (objectSizes above).
			const std::vector<std::string> positionSizeDependencies = sizeDependenciesOf(rawObject);

			if ((gridData.max.x > 1 || gridData.max.y > 1)
				&& (rawObjectShapeKind == ShapeKind::Text || rawObjectShapeKind == ShapeKind::Image))
			{
				std::cout << "warning: <grid>: '" << rawObject.name << "' is a "
					<< (rawObjectShapeKind == ShapeKind::Text ? "text" : "image")
					<< " object - its real footprint can't be known without a Window "
					<< "backend (see measureShapeSize, command.cpp), so grid spacing "
					<< "here will collapse to just the padding\n";
			}

			const bool isGrid = gridData.max.x > 1 || gridData.max.y > 1;

			// How long each picture of an animation lasts, drawn once for the
			// object (every cell of a grid shares it).
			const int animationFrames = rawObject.hasAnimation ? animationFramesOf(rawObject, windowDesc, where) : 0;

			// The object's looks, if it has several sprites and no animation:
			// built once, shared by every cell.
			std::vector<Object::Look> looks;
			if (!rawObject.looks.empty())
			{
				if (rawObject.hasHeading) { throw std::runtime_error(where + ": an object with a <heading> cannot have several looks"); }
				if (isGrid) { throw std::runtime_error(where + ": a <grid> cannot have several looks"); }

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

			for (auto gridX = 0; gridX < gridData.max.x; gridX++)
			{
				for (auto gridY = 0; gridY < gridData.max.y; gridY++)
				{
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

					// A grid gets one name per cell (aliens.3.2 - column, row,
					// from 1) so each cell can be found, hit and removed on
					// its own; a plain object keeps the name it was given.
					object.baseName = rawObject.name;
					object.groupName = rawObject.groupName;
					object.name = isGrid
						? rawObject.name + "." + std::to_string(gridX + 1) + "." + std::to_string(gridY + 1)
						: rawObject.name;
					object.objClass = rawObject.objClass;

					if (rawObject.objClass == "projectile")
					{
						rawObject.isVisible = false;
					}

					object.isVisible = rawObject.isVisible;

					object.positionOriginal.x = evaluate(rawObject.rawPosition.x, where);
					object.positionOriginal.y = evaluate(rawObject.rawPosition.y, where);

					// Finalizes this grid cell's real screen position right
					// here - Game is now done with position the moment its
					// own constructor returns, with no Window/backend needed
					// (see main.cpp). A non-grid object always has
					// gridData.max == {1,1}, so gridX == gridY == 0 and this
					// reduces to plain object.position = object.positionOriginal,
					// same as it always has. positionOriginal is then bumped
					// to match, exactly as before, so Game::resetObject/
					// resetAll restores each grid cell (e.g. each brick) to
					// its own slot, not the shared base corner.
					object.gridOffset.x = (gridObjSize.x + static_cast<float>(gridData.padding.x)) * static_cast<float>(gridX);
					object.gridOffset.y = (gridObjSize.y + static_cast<float>(gridData.padding.y)) * static_cast<float>(gridY);
					object.position = object.positionOriginal + object.gridOffset;
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

					// object's visual is built later by whichever Window backend is
					// running, once one exists (see Window::init() in window.h) -
					// visualDirty starts true (Object's own default), so nothing
					// needs to happen here.
					objects.push_back(std::move(object));
				}
			}

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
				state.input[keyCodeFromString(rawAction.first)] = processCommands(rawAction.second, where);
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
							&& (rawCondition.filterObject.empty() || rawCondition.filterObject == object.name || rawCondition.filterObject == object.baseName
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

		checkReferences(states, objects, sounds);
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
		std::vector<std::string> params;

		// Lines, rows of text and SVG drawings are all pictures the engine draws
		// itself, and all go the same way, here, once, when the game loads:
		//   1. read what the file describes (line ends, rows, a part of a drawing);
		//   2. draw it into a Bitmap, the pixels every window backend shows and a
		//      pixel collision tests, so no backend ever draws one itself;
		//   3. flip it, if the sprite says so (a <bitmap> or an <svg>);
		//   4. for an object with a <heading>, keep it (a Turnable) to be turned
		//      to whatever heading it faces, and start it at heading 0.
		// The params carry only the picture's size. The one difference is in the
		// turning: lines are kept as lines and drawn again at each heading, which
		// keeps their edges sharp, where a finished picture's pixels are turned.
		if (sprite.kind == "line" || sprite.kind == "bitmap" || sprite.kind == "svg")
		{
			auto kept = std::make_shared<Turnable>();
			std::shared_ptr<const Bitmap> drawn;

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

					// 2. Drawn by lunasvg (svg.cpp).
					drawn = std::make_shared<const Bitmap>(rasterizeSvg(sprite.path, region, svgScale, sprite.svgHide));
				}
				else
				{
					// 1. The rows, and how many pixels to a character.
					const int rowsScale = sprite.hasScale ? static_cast<int>(std::lround(evaluate(sprite.scale, where))) : 1;

					// 2. Drawn.
					drawn = std::make_shared<const Bitmap>(rasterizeRows(sprite.bitmapRows, rowsScale, colorFromName(color)));
				}

				// 3. Flipped.
				if (!sprite.flip.empty() && sprite.kind != "line")
				{
					drawn = std::make_shared<const Bitmap>(flipBitmap(*drawn, sprite.flip == "horizontal", sprite.flip == "vertical"));
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

		if (sprite.isGrid)
		{
			params.push_back("grid");
			params.push_back(std::to_string(evaluate(sprite.columns, where)));
			params.push_back(std::to_string(evaluate(sprite.rows, where)));
			params.push_back(sprite.hasPadding ? std::to_string(evaluate(sprite.padding.x, where)) : std::to_string(0));
			params.push_back(sprite.hasPadding ? std::to_string(evaluate(sprite.padding.y, where)) : std::to_string(0));
		}

		return params;
	}

	std::vector<std::shared_ptr<const Bitmap>> game_expr::buildAnimationBitmaps(const RawObject& rawObject, const std::string& where,
		std::vector<std::shared_ptr<const Turnable>>* turnables)
	{
		std::vector<std::shared_ptr<const Bitmap>> pictures;
		GridData firstGrid;

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

			const GridData grid = gridDataOf(frame, here);
			if (i == 0)
			{
				firstGrid = grid;
			}
			else
			{
				if (picture->width != pictures.front()->width || picture->height != pictures.front()->height)
				{
					throw std::runtime_error(here + ": is " + std::to_string(picture->width) + " by " + std::to_string(picture->height)
						+ " pixels, but frame 1 is " + std::to_string(pictures.front()->width) + " by " + std::to_string(pictures.front()->height)
						+ "; the frames of an animation must all be the same size");
				}

				if (grid.max.x != firstGrid.max.x || grid.max.y != firstGrid.max.y
					|| grid.padding.x != firstGrid.padding.x || grid.padding.y != firstGrid.padding.y)
				{
					throw std::runtime_error(here + ": is not repeated as a <grid> the same way as frame 1; the frames of an animation must share one layout");
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

	xge::GridData game_expr::gridDataOf(const RawSprite& sprite, const std::string& where)
	{
		GridData gridData;

		if (sprite.isGrid)
		{
			gridData.max.x = static_cast<int>(evaluate(sprite.columns, where));
			gridData.max.y = static_cast<int>(evaluate(sprite.rows, where));

			if (sprite.hasPadding)
			{
				gridData.padding.x = static_cast<int>(evaluate(sprite.padding.x, where));
				gridData.padding.y = static_cast<int>(evaluate(sprite.padding.y, where));
			}
		}

		return gridData;
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
