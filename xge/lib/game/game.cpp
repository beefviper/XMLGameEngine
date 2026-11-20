// game.cpp
// XML Game Engine
// author: beefviper
// date: Sept 18, 2020

#include "game/game.h"
#include "game/words.h"
#include "game/players.h"

#include "play/executor.h"
#include "play/geometry.h"
#include "game/random.h"
#include "game/scenes.h"
#include "load/spelling.h"
#include "play/verbs.h"


#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <iomanip>
#include <limits>
#include <optional>
#include <regex>
#include <sstream>
#include <stdexcept>
#include <utility>
#include <variant>

namespace xge
{
	Game::Game(GameData data)
	{
		// States that are copies of others, then what states and scenes own,
		// among the game's objects and named in full.
		takeStates(data.states);
		layOutScenes(data);
		settlePlayers(data);
		// Then, with every object's name final, its styles, what it
		// leaves to mean itself, and its centering.
		applyStyles(data.objects, data.styles);
		markSolid(data.objects);
		nameSelf(data.objects);
		nameOther(data.objects);
		centerOnOwnSize(data.objects);
		windowDesc = std::move(data.window);
		validation = std::move(data.validation);
		writtenVariables = std::move(data.variables);
		writtenStates = std::move(data.states);
		writtenObjects = std::move(data.objects);
		writtenSounds = std::move(data.sounds);
		writtenPaths = std::move(data.paths);
		for (const WrittenPatterns& written : data.patterns)
		{
			Patterns& set = patterns.emplace_back();
			set.name = Name::of(written.name);
			for (const WrittenPatterns::Pattern& writtenPattern : written.patterns)
			{
				set.patterns.push_back({ Name::of(writtenPattern.name), writtenPattern.parts, NameStore::program().of(writtenPattern.by) });
			}
		}
		const std::vector<Name> keeps = NameStore::program().of(data.keeps);
		demo = std::move(data.demo);

		// Room for what is spawned, so that the file's objects stay where
		// they are (and a reference to one good) as copies join.
		objects.reserve(writtenObjects.size() * 2 + 256);
		expr.parts = &objectParts;
		expr.styles = data.styles;
		expr.sheets = data.sheets;
		expr.init(windowDesc, writtenVariables, variables, writtenStates, states, writtenObjects, objects, writtenSounds, sounds, writtenPaths, paths);
		expr.finishLoading();
		for (std::size_t i = 0; i < objects.size(); ++i) { objectParts.placeAt(objects[i].id, i); }
		expr.whenLiveRead = [this] { refreshLive(); };
		keep(keeps);
		relabelAll();
		// Every picture's size is known now, with no window: the engine draws
		// text and reads image files itself.
		for (Object& object : objects)
		{
			object.size = object.pictureSize();
		}
		checkDemo();

		for (const auto& state : states)
		{
			stateTimers[state.name] = state.timers;
			// An <undo /> on a key: each press that moves a piece is kept.
			for (const auto* keys : state.keyMaps())
			{
				for (const auto& [key, commands] : *keys)
				{
					for (const Command& command : commands) { keepsUndo = keepsUndo || std::holds_alternative<CmdUndo>(shapeOf(command)); }
				}
			}
		}
		startScenes();
		// What a pattern compares is a variable of what is held.
		for (const Patterns& set : patterns)
		{
			for (const Patterns::Pattern& pattern : set.patterns)
			{
				for (const Name by : pattern.by)
				{
					if (by == words::worth || std::any_of(objects.begin(), objects.end(), [&](const Object& object) { return itemOf(object).is && object.variable.count(by); })) { continue; }
					throw std::runtime_error("<patterns name=\"" + set.name + "\"> > <pattern name=\"" + pattern.name + "\">: variable=\"" + by + "\"; nothing held has a variable of that name");
				}
			}
		}
		// The cards shuffled and dealt, holders scored by patterns there are.
		for (const Object& object : objects)
		{
			if (!isHolder(object)) { continue; }
			if (!holderOf(object).score.empty() && std::none_of(patterns.begin(), patterns.end(), [&](const Patterns& each) { return each.name == holderOf(object).score; }))
			{
				throw std::runtime_error("<object name=\"" + object.name + "\"> > <holds> > <score patterns=\"" + holderOf(object).score + "\">: names no <patterns>");
			}
			for (const Patterns& set : patterns)
			{
				if (set.name != holderOf(object).score) { continue; }
				std::set<std::string> alike;
				for (const Patterns::Pattern& pattern : set.patterns)
				{
					const std::string where = "<patterns name=\"" + set.name + "\"> > <pattern name=\"" + pattern.name + "\">";
					for (std::size_t i = 0; i < pattern.parts.size(); ++i)
					{
						if (pattern.parts[i].first == Patterns::Part::Alike) { alike.insert(pattern.by[i]); }
						else if (pattern.by[i] != "worth") { throw std::runtime_error(where + ": scores a hand of " + object.name + "; for now such a run is of worth (runs of another variable are for <melds />)"); }
					}
					if (alike.size() > 4) { throw std::runtime_error(where + ": scores a hand of " + object.name + "; for now a set scoring a hand compares at most four variables"); }
				}
			}
			if (!holderOf(object).melds.empty() && std::none_of(patterns.begin(), patterns.end(), [&](const Patterns& each) { return each.name == holderOf(object).melds; }))
			{
				throw std::runtime_error("<object name=\"" + object.name + "\"> > <holds> > <score patterns=\"" + holderOf(object).melds + "\"> with <melds />: names no <patterns>");
			}
		}
		itemHolders.markOddsRead();
		itemHolders.sendStarts();
		itemHolders.scoreDueHolders();
		// The players: the first one's turn; those the computer plays.
		for (const WrittenObject& written : writtenObjects)
		{
			if (written.think.empty()) { continue; }
			const Command made = makeCommand(written.think.front(), [&](const WrittenValue& value) { return expr.evaluate(value, "object '" + written.name + "' > <think>"); });
			if (const auto* plays = std::get_if<CmdThink>(&shapeOf(made))) { memberThinks.emplace(Name::of(written.name), *plays); }
		}
		startPlayers();
		loadedCount = objects.size();
		runsInTop.assign(objects.size(), 0);
		runsInTopStage.assign(objects.size(), 0);
		// Which objects' rules answer which others, found while loading
		// rather than in the first frame.
		partners(objects);
		// What follows something, from where everything starts.
		follow();
		// Positions made from a text's size, from the text it shows now.
		resolveSizeDependentPositions();
	}

	void Game::resolveSizeDependentPositions(void)
	{
		// What every text measures now, in the expressions' own name.width /
		// name.height variables.
		for (const auto& object : objects)
		{
			if (object.drawn.picture->kind == ShapeKind::Text) { expr.setObjectSize(object.name, object.size); }
		}

		// Only the objects whose position uses a size (most games: none).
		objectParts.sizeUses.each([&](ObjectId id, Object::SizeUse& use)
		{
			const std::size_t at = objectParts.at(id);
			if (at >= objects.size()) { return; }
			Object& object = objects[at];

			// Worked out again only when a size it uses has changed.
			std::vector<Vector2f> sizes;
			bool allThere = true;
			for (const auto& dependency : use.sizesOf)
			{
				const Object* measured = find(dependency);
				if (!measured || measured->name != dependency)
				{
					allThere = false;
					break;
				}
				sizes.push_back(measured->size);
			}

			if (!allThere || sizes == use.sizesUsed)
			{
				return;
			}

			const auto writtenObject = std::find_if(writtenObjects.begin(), writtenObjects.end(),
				[&](const WrittenObject& written) { return written.name == object.name; });
			if (writtenObject == writtenObjects.end())
			{
				return;
			}

			const std::string where = "object '" + writtenObject->name + "'";
			startFor(object).position.x = expr.evaluate(writtenObject->writtenPosition.x, where);
			startFor(object).position.y = expr.evaluate(writtenObject->writtenPosition.y, where);
			startFor(object).position = startOf(object).position + object.layout.cellOffset;
			object.position = startOf(object).position;
			use.sizesUsed = sizes;
		});
	}

	void Game::updateObjects(float seconds)
	{
		beginStep(seconds);
		// Anything changed from outside since (a test moving an object to
		// another scene) is seen, and so is anything this frame changes.
		settle();
		playOneFrame(getCurrentObjects(), windowDesc, *this, seconds);
		settle();
		cellMapping.showJoins();
		placeByResizedTexts();
	}

	void Game::settle()
	{
		settleSpawns();
		stageChanged();
	}

	void Game::placeByResizedTexts()
	{
		if (!textsResized) { return; }
		textsResized = false;
		resolveSizeDependentPositions();
	}

	bool Game::isDescription(Name name)
	{
		const auto known = descriptions.find(name);
		if (known != descriptions.end()) { return known->second; }

		bool any = false;
		bool all = true;
		for (std::size_t i = 0; i < loadedCount && i < objects.size(); ++i)
		{
			if (!objects[i].named(name)) { continue; }
			any = true;
			all = all && !startOf(objects[i]).exists;
		}
		return descriptions[name] = any && all;
	}

	Object* Game::spawn(Name target, bool random, std::optional<int> member)
	{
		// What it is a copy of: the object of that name, or the group's
		// members in turn; never a copy (only the file's objects are
		// spawned from, so a name means the same thing all game).
		std::vector<std::size_t> from;
		for (std::size_t i = 0; i < loadedCount && i < objects.size(); ++i)
		{
			if (objects[i].named(target)) { from.push_back(i); }
		}
		if (from.empty())
		{
			const Name full = fullName(target);
			if (full != target) { return spawn(full, random, member); }
			return nullptr;
		}
		// The one of that number, one of them at random, or each in turn.
		if (member && (*member < 1 || static_cast<std::size_t>(*member) > from.size())) { return nullptr; }
		const std::size_t pick = member ? static_cast<std::size_t>(*member - 1) : random ? std::min(from.size() - 1, static_cast<std::size_t>(randomBetween(0.0f, static_cast<float>(from.size()))))
			: spawnTurns[target]++ % from.size();
		const Object& original = objects[from[pick]];

		const auto copyOfIt = [&](const Object& each) { return each.copyOf == original.name && each.exists; };
		if (original.most > 0)
		{
			const auto there = std::count_if(objects.begin(), objects.end(), copyOfIt) + std::count_if(spawned.begin(), spawned.end(), copyOfIt);
			if (there >= original.most) { return nullptr; }
		}

		// The first of its numbers that no object has now.
		Name name;
		for (std::size_t number = 1; name.empty(); ++number)
		{
			const Name candidate = Name::of(original.name + "." + std::to_string(number));
			const auto taken = [&](const Object& each) { return each.name == candidate; };
			if (std::none_of(objects.begin(), objects.end(), taken) && std::none_of(spawned.begin(), spawned.end(), taken)) { name = candidate; }
		}

		const auto written = std::find_if(writtenObjects.begin(), writtenObjects.end(), [&](const WrittenObject& each) { return each.name == original.name; });
		if (written == writtenObjects.end()) { return nullptr; }
		WrittenObject copy = *written;
		const Renamed renamed = renameOwn(copy, original.name, name);
		const bool namedIt = renamed.kind;
		copy.outerGroups = textsOf(original.outerGroups);
		if (!original.groupName.empty()) { copy.outerGroups.push_back(original.groupName); }
		copy.groupName = original.name;
		copy.exists = true;
		copy.isVisible = true;

		Object made = expr.spawnObject(copy, original);

		// Of its kind, unless its rules named the one it is copied from:
		// then their tests, made at load for that one, about the copy.
		if (!namedIt) { made.kind = original.kind; }
		// Its looks too, while the one copied from shows them as loaded.
		if (!renamed.looks && !original.looks.set->changed) { made.looks.set = original.looks.set; }
		const auto retarget = [&](Condition& check) { retargetCheck(check, original.name, name); };
		for (std::size_t i = 0; namedIt && i < made.kind->collisionData.others.size() && i < original.kind->collisionData.others.size(); ++i)
		{
			if (!original.kind->collisionData.others[i].check) { continue; }
			Condition check = *original.kind->collisionData.others[i].check;
			retarget(check);
			made.kind->collisionData.others[i].check = std::move(check);
		}
		for (std::size_t edge = 0; namedIt && edge < made.kind->collisionData.edges.size(); ++edge)
		{
			const std::vector<EdgeGuard>& was = original.kind->collisionData.edges[edge].guards;
			std::vector<EdgeGuard>& guards = made.kind->collisionData.edges[edge].guards;
			for (std::size_t g = 0; g < guards.size() && g < was.size(); ++g)
			{
				if (!was[g].check) { continue; }
				Condition check = *was[g].check;
				retarget(check);
				guards[g].check = std::move(check);
			}
		}
		// Its actions' conditions, the same way.
		const std::function<void(const std::vector<Command>&, std::vector<Command>&)> whens = [&](const std::vector<Command>& theirs, std::vector<Command>& copied)
		{
			for (std::size_t i = 0; i < theirs.size() && i < copied.size(); ++i)
			{
				const auto* was = std::get_if<CmdWhen>(&theirs[i]);
				auto* now = std::get_if<CmdWhen>(&copied[i]);
				if (!was || !now || !was->body || !now->body || !was->body->test) { continue; }
				Condition check = *was->body->test;
				retarget(check);
				now->body->test = std::move(check);
				whens(was->body->commands, now->body->commands);
			}
		};
		for (auto& [actionName, list] : made.kind->action)
		{
			if (!namedIt) { break; }
			if (const auto found = original.kind->action.find(actionName); found != original.kind->action.end()) { whens(found->second, list); }
		}

		made.copyOf = original.name;
		made.groupName = Name::of(copy.groupName);
		made.outerGroups = NameStore::program().listOf(copy.outerGroups);
		made.ownerState = original.ownerState;
		made.ownerScene = original.ownerScene;
		startFor(made).ownerScene = original.ownerScene;
		made.exists = true;
		startFor(made).exists = true;
		made.isVisible = true;
		startFor(made).isVisible = true;
		// Its picture is the one it is copied from, and so its size.
		made.size = original.size;
		writtenCopies[name] = std::move(copy);
		spawned.push_back(std::move(made));
		if (objectParts.boundTexts.has(spawned.back().id)) { relabel(spawned.back()); }
		return &spawned.back();
	}

	void Game::despawn(Object& object)
	{
		object.exists = false;
		object.isVisible = false;
		object.collisionsOn = false;
		objectParts.onPaths.remove(object.id);
		objectParts.carries.each([&](ObjectId, Object::Carry& carry) { if (carry.what == object.name) { carry.what.clear(); } });
		if (Object::Carry* carry = objectParts.carries.find(object.id)) { carry->what.clear(); }
		stageChanged();
	}

	void Game::despawn(Name target)
	{
		// The name as written, or the end of a path (bomb for playing.bomb).
		const Name name = fullName(target);
		for (auto& object : objects)
		{
			if (object.exists && (object.named(name))) { despawn(object); }
		}
		for (auto& object : spawned)
		{
			if (object.exists && (object.named(name))) { despawn(object); }
		}
	}

	// A copy that leaves lets go of the pictures that were its own: one
	// with holes cut in it, a tether's line.
	void Game::letGoOwnPictures(const Object& object)
	{
		if (objectParts.unholed.has(object.id)) { expr.resources.release(object.drawn.bitmap); }
		if (const Object::Tether* tether = objectParts.tethers.find(object.id)) { expr.resources.release(tether->drawn); }
	}

	void Game::settleSpawns(void)
	{
		bool changed = false;
		const auto gone = [](const Object& object) { return object.isCopy() && !object.exists; };
		if (std::any_of(objects.begin(), objects.end(), gone))
		{
			for (const Object& object : objects)
			{
				if (gone(object))
				{
					letGoOwnPictures(object);
					objectParts.freeId(object.id);
				}
			}
			objects.erase(std::remove_if(objects.begin(), objects.end(), gone), objects.end());
			changed = true;
		}
		if (!spawned.empty())
		{
			for (Object& object : spawned)
			{
				if (object.exists) { objects.push_back(std::move(object)); }
				else
				{
					letGoOwnPictures(object);
					objectParts.freeId(object.id);
				}
			}
			spawned.clear();
			changed = true;
		}
		if (changed) { rosterChanged(); }
	}

	Object* Game::find(ObjectId id) noexcept
	{
		if (!objectParts.current(id)) { return nullptr; }
		const std::size_t at = objectParts.at(id);
		if (at < objects.size() && objects[at].id == id) { return &objects[at]; }
		for (Object& waiting : spawned)
		{
			if (waiting.id == id) { return &waiting; }
		}
		return nullptr;
	}

	void Game::rosterChanged(void)
	{
		objectParts.placeNowhere();
		for (std::size_t i = 0; i < objects.size(); ++i) { objectParts.placeAt(objects[i].id, i); }
		foundObjects.clear();
		fullNames.clear();
		filtered.clear();
		liveLinked = false;
		liveLinks.clear();
		runsInTop.assign(objects.size(), 0);
		runsInTopStage.assign(objects.size(), 0);
		forgetPartners();
		stageChanged();
	}

	void Game::printGame(void)
	{
		std::cout << "schema validation: " << validation << "\n\n";

		std::cout << windowDesc << '\n';

		for (auto& variable : variables)
		{
			std::cout << "variable: name=" << variable.first << ", value=" << variable.second << '\n';
		}
		std::cout << '\n';

		for (auto& writtenObject : writtenObjects)
		{
			std::cout << writtenObject << '\n';
		}

		for (auto& object : objects)
		{
			std::cout << object << '\n';
		}

		for (auto& state : states)
		{
			std::cout << state << '\n';
		}

		for (const auto& sound : sounds)
		{
			std::cout << sound << '\n';
		}
	}

	const std::vector<SoundDesc>& Game::getSounds(void) const noexcept
	{
		return sounds;
	}

	void Game::requestSound(Name name)
	{
		// Within a frame the last word wins: a loop silenced and asked for
		// again (one of two keys let go, the other still down) plays on.
		if (soundLoops(name))
		{
			loops.insert(name);
			silenceRequests.erase(std::remove(silenceRequests.begin(), silenceRequests.end(), name), silenceRequests.end());
			return;
		}

		if (std::find(soundRequests.begin(), soundRequests.end(), name) == soundRequests.end())
		{
			soundRequests.push_back(name);
		}
	}

	std::vector<std::string> Game::takeSoundRequests(void)
	{
		return std::exchange(soundRequests, {});
	}

	const std::set<std::string>& Game::loopingSounds(void) const noexcept
	{
		return loops;
	}

	std::set<std::string> Game::soundsOn(void) const
	{
		std::set<std::string> on = loops;
		if (!currentState.empty() && !currentState.top().music.empty() && soundLoops(currentState.top().music))
		{
			on.insert(currentState.top().music);
		}
		return on;
	}

	void Game::silenceSound(Name name)
	{
		loops.erase(name);
		soundRequests.erase(std::remove(soundRequests.begin(), soundRequests.end(), name), soundRequests.end());
		if (std::find(silenceRequests.begin(), silenceRequests.end(), name) == silenceRequests.end())
		{
			silenceRequests.push_back(name);
		}
	}

	std::vector<std::string> Game::takeSilenceRequests(void)
	{
		return std::exchange(silenceRequests, {});
	}

	bool Game::soundLoops(Name name) const noexcept
	{
		return std::any_of(sounds.begin(), sounds.end(), [&](const SoundDesc& sound) { return sound.loop && sound.name == name; });
	}

	WindowDesc& Game::getWindowDesc(void) noexcept
	{
		return windowDesc;
	}

	const Object* Game::nearestInPlay(const Object& from, Name name)
	{
		return xge::nearestInPlay(objects, from, name, [this](const Object& other) { return isShown(other); });
	}

	bool Game::isShown(const Object& object) noexcept
	{
		if (!object.isVisible || !object.exists || currentState.empty()) { return false; }

		// One of the game's own objects (not a copy held elsewhere): its
		// answer is kept (runsInTop).
		const std::uintptr_t offset = reinterpret_cast<std::uintptr_t>(&object) - reinterpret_cast<std::uintptr_t>(objects.data());
		const std::size_t index = offset / sizeof(Object);
		if (offset % sizeof(Object) != 0 || index >= runsInTopStage.size())
		{
			return runsIn(object, currentState.top());
		}

		if (runsInTopStage[index] != stage)
		{
			runsInTop[index] = runsIn(object, currentState.top()) ? 1 : 0;
			runsInTopStage[index] = stage;
		}
		return runsInTop[index] != 0;
	}

	bool Game::runsIn(const Object& object, const State& state) const
	{
		if (!object.ownerState.empty() && object.ownerState == state.name)
		{
			if (object.ownerScene.empty()) { return true; }
			const auto current = currentScenes.find(state.name);
			if (current != currentScenes.end() && current->second == object.ownerScene) { return true; }
		}

		return std::any_of(state.show.begin(), state.show.end(), [&](Name shown) { return object.named(shown); });
	}

	bool Game::isStill(const Object& object) noexcept
	{
		if (currentState.empty()) { return false; }
		const State& top = currentState.top();
		if (top.showStates.empty() || !object.isVisible || !object.exists || runsIn(object, top)) { return false; }

		for (const Name name : top.showStates)
		{
			const auto state = std::find_if(states.begin(), states.end(), [&](const State& candidate) { return candidate.name == name; });
			if (state != states.end() && runsIn(object, *state)) { return true; }
		}
		return false;
	}

	bool Game::inScope(const Object& object) const
	{
		if (object.ownerState.empty()) { return true; }
		if (currentState.empty() || object.ownerState != currentState.top().name) { return false; }
		if (object.ownerScene.empty()) { return true; }
		const auto current = currentScenes.find(object.ownerState);
		return current != currentScenes.end() && current->second == object.ownerScene;
	}

	bool Game::isAway(const Object& object) const
	{
		if (object.ownerScene.empty()) { return false; }
		const auto current = currentScenes.find(object.ownerState);
		return current == currentScenes.end() || current->second != object.ownerScene;
	}

	Name Game::currentScene(Name state) const
	{
		const auto current = currentScenes.find(state);
		return current == currentScenes.end() ? Name{} : current->second;
	}

	void Game::startScenes(void)
	{
		currentScenes.clear();
		sceneTimers.clear();
		stageChanged();
		for (const State& state : states)
		{
			if (!state.scenes.empty()) { currentScenes[state.name] = state.scenes.front().name; }
			for (const Scene& scene : state.scenes) { sceneTimers[scene.name] = scene.timers; }
		}
	}

	void Game::enterScene(Name scene)
	{
		const Name state = Name::of(scene.str().substr(0, scene.str().find('.')));
		if (!sceneTimers.count(scene))
		{
			// The file is checked for this when it loads; this is for a caller in C++.
			throw std::out_of_range("no scene named '" + scene + "'");
		}

		Name& current = currentScenes[state];
		if (current == scene) { return; }
		current = scene;
		++sceneChanges;
		stageChanged();

		// What is carried by something that is in every scene (the state's
		// own, or the game's) belongs to the scene it is carried into.
		for (const Object& carrier : objects)
		{
			const Name what = carrying(carrier);
			if (what.empty() || !carrier.ownerScene.empty()) { continue; }
			Object* carried = find(what);
			if (carried && carried->ownerState == state && !carried->ownerScene.empty()) { carried->ownerScene = scene; }
		}
		stageChanged();
	}

	Name Game::fullName(Name name) const
	{
		if (const auto known = fullNames.find(name); known != fullNames.end()) { return known->second; }
		const Name full = findFullName(name);
		fullNames.emplace(name, full);
		return full;
	}

	Name Game::findFullName(Name name) const
	{
		const auto calls = [&](const Object& object, Name wanted) { return object.named(wanted); };
		if (std::any_of(objects.begin(), objects.end(), [&](const Object& object) { return calls(object, name); })) { return name; }

		const std::string end = "." + name;
		const auto endsWith = [&](const std::string& text) { return text.size() > end.size() && text.compare(text.size() - end.size(), end.size(), end) == 0; };
		// The shortest path wins, as when the game loads (playing.rock, the
		// rock of every room, over playing.room1.rock); two as short: neither.
		const auto dots = [](const std::string& text) { return std::count(text.begin(), text.end(), '.'); };
		std::string found;
		bool tied = false;
		for (const Object& object : objects)
		{
			std::vector<std::string> callings{ object.name };
			if (!object.groupName.empty()) { callings.push_back(object.groupName); }
			callings.insert(callings.end(), object.outerGroups.begin(), object.outerGroups.end());
			for (const std::string& calling : callings)
			{
				if (!endsWith(calling) || calling == found) { continue; }
				if (found.empty() || dots(calling) < dots(found)) { found = calling; tied = false; }
				else if (dots(calling) == dots(found)) { tied = true; }
			}
		}
		return found.empty() || tied ? name : Name(found);
	}

	Object& Game::object(Name name)
	{
		Object* object = find(name);
		if (!object)
		{
			throw std::out_of_range("no object named '" + name + "'");
		}
		return *object;
	}

	// An object's own name (aliens.3.2) finds that object; the name of a
	// <group> (aliens) finds its first member.
	Object* Game::find(Name name) noexcept
	{
		if (const auto found = foundObjects.find(name); found != foundObjects.end())
		{
			return found->second;
		}

		Object* object = search(name);
		foundObjects.emplace(name, object);
		return object;
	}

	Object* Game::search(Name name) noexcept
	{
		const auto exact = std::find_if(std::begin(objects), std::end(objects), [&](const Object& obj) { return obj.name == name; });
		if (exact != std::end(objects))
		{
			return &(*exact);
		}

		const auto first = std::find_if(std::begin(objects), std::end(objects), [&](const Object& obj) { return obj.inGroup(name); });
		if (first != std::end(objects))
		{
			return &(*first);
		}

		// The end of one path: robots for playing.room1.robots.
		const Name full = fullName(name);
		return full == name ? nullptr : search(full);
	}

	float Game::getVariable(Name name)
	{
		const auto result = variables.find(name);

		if (result == variables.end())
		{
			return 0;
		}
		else
		{
			return result->second;
		}
	}

	const State& Game::getCurrentState(void) const
	{
		return currentState.top();
	}

	void Game::setCurrentState(const int& index)
	{
		currentState.push(states.at(static_cast<std::size_t>(index)));
		++stateChanges;
		stageChanged();
		beginMusic();
	}

	void Game::setCurrentState(Name name)
	{
		pushState(name);
	}

	void Game::pushState(Name name)
	{
		auto result = std::find_if(std::begin(states), std::end(states), [&](State& state) { return state.name == name; });
		if (result == std::end(states))
		{
			// The file is checked for this when it loads (see
			// load/expr::checkReferences); this is for a caller in C++.
			throw std::out_of_range("no state named '" + name + "'");
		}

		// Each visit starts its timers over: an <after> goes off once a
		// visit, not once a game. A pop back to a state carries on its own.
		stateTimers[name] = result->timers;
		currentState.push(*result);
		++stateChanges;
		stageChanged();
		beginMusic();
	}

	void Game::beginMusic(void)
	{
		const Name music = currentState.top().music;
		if (!music.empty() && !soundLoops(music))
		{
			requestSound(music);
		}
	}

	void Game::popState(void) noexcept
	{
		// The state the game started in is never popped: with no state at
		// all there would be nothing to show and no keys to read.
		if (currentState.size() <= 1)
		{
			return;
		}

		currentState.pop();
		++stateChanges;
		stageChanged();
	}

	void Game::drawStartAgain(Object& object)
	{
		if (!object.startDrawsRandom)
		{
			return;
		}

		const WrittenObject* written = nullptr;
		if (const auto copy = writtenCopies.find(object.name); object.isCopy() && copy != writtenCopies.end())
		{
			written = &copy->second;
		}
		else if (const auto found = std::find_if(writtenObjects.begin(), writtenObjects.end(), [&](const WrittenObject& candidate) { return candidate.name == object.name; }); found != writtenObjects.end())
		{
			written = &*found;
		}
		if (!written)
		{
			return;
		}

		const std::string where = "object '" + written->name + "'";
		for (const auto& [name, value] : written->variable)
		{
			if (!value.drawsRandom()) { continue; }
			const float drawn = expr.evaluate(value, where);
			object.variable[Name::of(name)] = drawn;
			startFor(object).variable[Name::of(name)] = drawn;
			variableChanged(object.name, Name::of(name), drawn);
		}

		// The position, then the velocity, the order they are drawn in when the
		// game loads (load/expr::init), so a start drawn again takes the same
		// numbers as the first did, and a generated program, which draws them
		// in that order, draws the same ones.
		startFor(object).position = Vector2f{ expr.evaluate(written->writtenPosition.x, where), expr.evaluate(written->writtenPosition.y, where) } + object.layout.cellOffset;

		startFor(object).velocity = { expr.evaluate(written->writtenVelocity.x, where), expr.evaluate(written->writtenVelocity.y, where) };
	}

	void Game::popState(Name name)
	{
		if (name.empty())
		{
			popState();
			return;
		}

		auto result = std::find_if(std::begin(states), std::end(states), [&](State& state) { return state.name == name; });
		if (result == std::end(states))
		{
			throw std::out_of_range("no state named '" + name + "'");
		}

		if (!currentState.empty())
		{
			currentState.pop();
		}
		stateTimers[name] = result->timers;
		currentState.push(*result);
		++stateChanges;
		stageChanged();
		beginMusic();
	}

	unsigned long Game::stateChangeCount(void) const noexcept
	{
		return stateChanges;
	}

	const Patterns* Game::patternsNamed(Name name) const noexcept
	{
		const auto set = std::find_if(patterns.begin(), patterns.end(), [&](const Patterns& each) { return each.name == name; });
		return set == patterns.end() ? nullptr : &*set;
	}

	const std::vector<State>& Game::getStates(void) const noexcept
	{
		return states;
	}

	void Game::addTo(const VariableName& target, float amount)
	{
		if (float* value = variableOf(target))
		{
			setVariable(target.owner, target.variable, *value + amount);
		}
		else
		{
			std::cout << "warning: no variable " << target.written() << " to change\n";
		}
	}

	void Game::copyVariable(const VariableName& from, const VariableName& into)
	{
		const float* value = variableOf(from);
		if (!value || !variableOf(into))
		{
			std::cout << "warning: <copy variable=\"" << from.written() << "\" into=\"" << into.written() << "\" />: no such variable\n";
			return;
		}
		setVariable(into.owner, into.variable, *value);
	}

	float* Game::variableOf(const VariableName& name) noexcept
	{
		Object* owner = anyTurns ? find(turnOwner(name.owner)) : find(name.owner);
		if (!owner) { return nullptr; }
		const auto found = owner->variable.find(name.variable);
		return found == owner->variable.end() ? nullptr : &found->second;
	}

	void Game::checkDemo(void) const
	{
		if (demo.recorded)
		{
			return;
		}
		const auto check = [&](const std::vector<Command>& commands, const std::string& where)
		{
			for (const Command& command : commands)
			{
				if (std::holds_alternative<CmdDemo>(command))
				{
					throw std::runtime_error(where + ": <demo /> plays the game's recorded <demo>, and this game has none; record one with xgecli --record");
				}
			}
		};
		for (const State& state : states)
		{
			const std::string where = "state '" + state.name + "'";
			for (const auto* keys : state.keyMaps())
			{
				for (const auto& [key, commands] : *keys) { check(commands, where + " > <input>"); }
			}
			for (const Condition& condition : state.conditions) { check(condition.commands, where + " > <condition>"); }
			for (const Timer& timer : state.timers) { check(timer.commands, where + " > <timer>"); }
			for (const Scene& scene : state.scenes)
			{
				for (const Condition& condition : scene.conditions) { check(condition.commands, where + " > <condition>"); }
				for (const Timer& timer : scene.timers) { check(timer.commands, where + " > <timer>"); }
			}
		}
	}

	void Game::startTurns(const std::vector<std::string>& playerStates)
	{
		turnsState = currentState.empty() ? Name{} : currentState.top().name;
		Turn start;
		putTurnAway(start);
		turns.assign(playerStates.size(), start);
		for (std::size_t i = 0; i < playerStates.size(); ++i)
		{
			turns[i].state = Name::of(playerStates[i]);
		}
		turn = 0;
		beginTurn();
	}

	void Game::passTurn(bool out)
	{
		if (turns.empty())
		{
			return;
		}
		if (out)
		{
			turns[turn].out = true;
		}

		std::size_t next = turn;
		bool anyone = false;
		for (std::size_t k = 1; k <= turns.size(); ++k)
		{
			next = (turn + k) % turns.size();
			if (!turns[next].out)
			{
				anyone = true;
				break;
			}
		}
		if (!anyone)
		{
			turns.clear(); // everyone is out: what follows <out /> runs
			return;
		}
		if (next == turn)
		{
			return; // the only one still in plays on
		}

		if (turnPutAway) { turnPutAway(); }
		putTurnAway(turns[turn]);
		turn = next;
		takeTurnUp(turns[turn]);
		turnHandedOver = out;
		beginTurn();
	}

	bool Game::takeTurnStop(void) noexcept
	{
		return std::exchange(turnHandedOver, false);
	}

	void Game::putTurnAway(Turn& into) const
	{
		into.objects = objects;
		into.parts = objectParts;
		into.stateTimers = stateTimers;
		into.currentScenes = currentScenes;
		into.sceneTimers = sceneTimers;
	}

	void Game::takeTurnUp(const Turn& from)
	{
		// The file's objects as the player left them, then the copies
		// spawned in their game.
		for (const Object& waiting : spawned) { objectParts.removeParts(waiting.id); }
		spawned.clear();
		for (std::size_t i = loadedCount; i < objects.size(); ++i) { objectParts.removeParts(objects[i].id); }
		objects.resize(std::min(objects.size(), loadedCount));
		for (std::size_t i = 0; i < objects.size() && i < from.objects.size(); ++i)
		{
			if (objects[i].kept) { continue; }
			objects[i] = from.objects[i];
			objectParts.copyPartsFrom(from.parts, objects[i].id);
			objects[i].drawn.visualDirty = true;
		}
		for (std::size_t i = loadedCount; i < from.objects.size(); ++i)
		{
			objects.push_back(from.objects[i]);
			objectParts.copyPartsFrom(from.parts, objects.back().id);
			objects.back().drawn.visualDirty = true;
		}
		rosterChanged();
		stateTimers = from.stateTimers;
		currentScenes = from.currentScenes;
		sceneTimers = from.sceneTimers;
		++sceneChanges;
		stageChanged();
	}

	void Game::beginTurn(void)
	{
		// What was playing stops; the turn is played in the state the turns
		// started in, with the player's own state over it.
		loops.clear();
		while (currentState.size() > 1 && currentState.top().name != turnsState)
		{
			currentState.pop();
		}
		stageChanged();
		const Name state = turns[turn].state;
		if (!state.empty())
		{
			pushState(state);
		}
		++stateChanges;
		relabelAll();
	}

	bool Game::keepsAnything(void) const noexcept
	{
		return std::any_of(objects.begin(), objects.end(), [](const Object& object) { return object.kept; });
	}

	void Game::saveKept(std::ostream& out) const
	{
		out << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
			<< "<!-- What the game keeps from one game to the next (its <keeps>), written by the engine. -->\n"
			<< "<kept>\n";
		for (const auto& object : objects)
		{
			if (!object.kept) { continue; }
			out << "  <object name=\"" << object.name << "\"";
			if (!object.lookName().empty()) { out << " sprite=\"" << object.lookName() << "\""; }
			out << ">\n";
			for (const auto& [name, value] : object.variable)
			{
				std::ostringstream number;
				number << std::setprecision(std::numeric_limits<float>::max_digits10) << value;
				out << "    <variable name=\"" << name << "\">" << number.str() << "</variable>\n";
			}
			out << "  </object>\n";
		}
		out << "</kept>\n";
	}

	namespace
	{
		// Text as it may stand in an XML attribute.
		std::string xmlEscaped(const std::string& text)
		{
			std::string escaped;
			for (const char c : text)
			{
				switch (c)
				{
				case '&': escaped += "&amp;"; break;
				case '<': escaped += "&lt;"; break;
				case '>': escaped += "&gt;"; break;
				case '"': escaped += "&quot;"; break;
				default: escaped += c; break;
				}
			}
			return escaped;
		}
	}

	void Game::describe(std::ostream& out, unsigned long frame, double seconds, const std::string& indent)
	{
		const auto pair = [&](const char* tag, Vector2f value, const std::string& in)
		{
			out << in << '<' << tag << "><x>" << value.x << "</x><y>" << value.y << "</y></" << tag << ">\n";
		};
		const auto numbers = [&](const auto& values, const std::string& in)
		{
			if (values.empty()) { return; }
			out << in << "<variables>\n";
			for (const auto& [name, value] : values)
			{
				out << in << "  <variable name=\"" << xmlEscaped(name) << "\">" << value << "</variable>\n";
			}
			out << in << "</variables>\n";
		};

		out << indent << "<moment>\n"
			<< indent << "  <frame>" << frame << "</frame>\n"
			<< indent << "  <seconds>" << seconds << "</seconds>\n";
		if (!currentState.empty())
		{
			const Name state = currentState.top().name;
			const Name scene = currentScene(state);
			out << indent << "  <state name=\"" << xmlEscaped(state) << '"';
			if (!scene.empty()) { out << " scene=\"" << xmlEscaped(scene) << '"'; }
			out << " />\n";
		}
		numbers(variables, indent + "  ");
		for (const Object& object : objects)
		{
			if (!isShown(object)) { continue; }
			out << indent << "  <object name=\"" << xmlEscaped(object.name) << '"';
			if (!object.objClass.empty()) { out << " class=\"" << xmlEscaped(object.objClass) << '"'; }
			if (!object.groupName.empty()) { out << " group=\"" << xmlEscaped(object.groupName) << '"'; }
			if (!object.lookName().empty()) { out << " sprite=\"" << xmlEscaped(object.lookName()) << '"'; }
			out << ">\n";
			pair("position", object.position, indent + "    ");
			pair("velocity", object.velocity, indent + "    ");
			numbers(object.variable, indent + "    ");
			out << indent << "  </object>\n";
		}
		out << indent << "</moment>\n";
	}

	void Game::loadKept(std::istream& in)
	{
		// The file is the engine's own, written by saveKept, one tag a line,
		// so it is read a line at a time, without an XML library (a generated
		// program has none).
		static const std::regex objectLine{ R"line(^\s*<object name="([^"]*)"(?: sprite="([^"]*)")?>\s*$)line" };
		static const std::regex variableLine{ R"line(^\s*<variable name="([^"]*)">([^<]*)</variable>\s*$)line" };

		Object* object = nullptr;
		std::string line;
		while (std::getline(in, line))
		{
			std::smatch match;
			if (std::regex_match(line, match, objectLine))
			{
				object = find(Name(match[1].str()));
				if (object && !object->kept) { object = nullptr; }
				if (object && match[2].matched)
				{
					const auto look = std::find_if(object->looks.all().begin(), object->looks.all().end(),
						[&](const Object::Look& each) { return each.name == match[2].str(); });
					if (look != object->looks.all().end()) { object->showLook(static_cast<std::size_t>(look - object->looks.all().begin())); }
				}
			}
			else if (object && std::regex_match(line, match, variableLine))
			{
				const auto variable = object->variable.findText(match[1].str());
				if (variable == object->variable.end()) { continue; }
				try
				{
					variable->second = std::stof(match[2].str());
				}
				catch (const std::exception&)
				{
					std::cout << "warning: kept " << object->name << "." << variable->first << " is \"" << match[2].str() << "\", which is no number; left as it was\n";
				}
			}
		}
		relabelAll();
	}

	void Game::rank(const VariableName& entry, Name table)
	{
		Object* from = find(entry.owner);
		const std::string& by = entry.variable;
		if (!from || !from->variable.countText(by))
		{
			std::cout << "warning: <rank variable=\"" << entry.written() << "\" into=\"" << table << "\" />: no such variable\n";
			return;
		}

		// The places, the best first, in the order the group's members are written.
		const Name group = fullName(table);
		std::vector<Object*> places;
		for (auto& object : objects)
		{
			if (object.inGroup(group)) { places.push_back(&object); }
		}

		// Its place is the first one it beats; an equal score stays above it.
		const float score = from->variable.at(by);
		const auto place = std::find_if(places.begin(), places.end(),
			[&](const Object* each) { return each->variable.countText(by) && each->variable.at(by) < score; });
		if (place == places.end())
		{
			return;
		}

		for (auto below = places.end() - 1; below != place; --below)
		{
			(*below)->variable = (*(below - 1))->variable;
		}
		for (auto& [name, value] : (*place)->variable)
		{
			if (const auto given = from->variable.find(name); given != from->variable.end())
			{
				value = given->second;
			}
		}

		for (Object* each : places)
		{
			for (const auto& [name, value] : each->variable)
			{
				variableChanged(each->name, name, value);
			}
		}
	}

	void Game::keep(const std::vector<Name>& names)
	{
		// A name is an object or a group (every member of it), written out in
		// full the way a command's are.
		for (const Name given : names)
		{
			const Name name = fullName(given);
			bool found = false;
			for (auto& object : objects)
			{
				if (object.named(name))
				{
					object.kept = true;
					found = true;
				}
			}
			if (!found)
			{
				std::vector<std::string> known;
				for (const auto& object : objects) { known.push_back(object.name); }
				throw std::runtime_error("<keeps> > <keep object=\"" + given + "\" />: names no object of the game" + didYouMean(given, known));
			}
		}
	}

	bool Game::tickTimer(const Timer& timer, Timer::Clock& clock, const TimerOwner& owner, float& owed)
	{
		if (clock.done)
		{
			owed = 0.0f;
			return false;
		}

		if (!clock.counting)
		{
			clock.secondsLeft = static_cast<double>(evaluateNow(timer.interval, std::string(owner.kind) + " '" + owner.name + "' > <timer>"));
			clock.counting = true;
			// None at all: it goes off once a step.
			if (!(clock.secondsLeft > 0.0)) { clock.secondsLeft = static_cast<double>(owed); }
		}

		clock.secondsLeft -= static_cast<double>(owed);
		if (clock.secondsLeft > static_cast<double>(kTimeSlack))
		{
			owed = 0.0f;
			return false;
		}

		// Gone off: what the step had beyond it is still owed. A repeating
		// timer starts over, with its interval worked out again after its
		// commands have run (so they can change it, and a <random> waits a
		// new time); a once-only one is finished.
		owed = static_cast<float>(std::max(0.0, -clock.secondsLeft));
		if (timer.repeat) { clock.counting = false; }
		else { clock.done = true; }
		return true;
	}

	void Game::showTimeLeft(Object& object, const Timer& timer, const Timer::Clock& clock)
	{
		const float left = clock.done || !clock.counting ? 0.0f : static_cast<float>(std::ceil(std::max(0.0, clock.secondsLeft - static_cast<double>(kTimeSlack))));
		const auto found = object.variable.find(timer.name);
		if (found == object.variable.end() || found->second == left) { return; }
		found->second = left;
		variableChanged(object.name, timer.name, left);
	}

	void Game::animationEnded(std::size_t index, std::size_t look)
	{
		if (index >= objects.size()) { return; }
		Object& object = objects[index];
		// A copy: a command may reset the object, or spawn and move it.
		const std::vector<Command> commands = object.looks.all().empty() ? object.animation.frames.shows().then
			: look < object.looks.all().size() ? object.looks.all()[look].frames.shows().then : std::vector<Command>{};
		CommandExecutor executor(*this);
		for (const Command& command : commands)
		{
			if (!executor.run(command, Doer::of(&objects[index]))) { break; }
		}
	}

	void Game::frameCameUp(std::size_t index, std::size_t look, std::size_t picture)
	{
		if (index >= objects.size()) { return; }
		Object& object = objects[index];
		if (!object.hasFrameCommands(look, picture)) { return; }
		// A copy, as for <then>.
		const std::vector<Command> commands = object.looks.all().empty() ? object.animation.frames.shows().commands[picture] : object.looks.all()[look].frames.shows().commands[picture];
		CommandExecutor executor(*this);
		for (const Command& command : commands)
		{
			if (!executor.run(command, Doer::of(&objects[index]))) { break; }
		}
	}

	void Game::updateTimers(float seconds)
	{
		CommandExecutor executor(*this);

		// An object's timers, while it is shown and in play. By index: a timer
		// can fire a projectile or release a pool, which changes other objects
		// but never how many there are.
		for (std::size_t i = 0; i < objects.size(); ++i)
		{
			const std::vector<Timer>& timers = objects[i].kind->timers;
			// Found again each time: a command can give another object its
			// clocks, which moves the store's entries.
			const auto clocks = [&]() { return objectParts.clocks.find(objects[i].id); };
			const auto count = [&]() { const std::vector<Timer::Clock>* each = clocks(); return each ? each->size() : 0; };
			for (std::size_t t = 0; t < count() && t < timers.size(); ++t)
			{
				// Not shown, or sent away from its own map (a next-piece box).
				if (!isShown(objects[i]) || awayFromHome(objects[i]))
				{
					break;
				}

				// One that follows a look counts only while the object shows it.
				const Name sprite = timers[t].sprite;
				if (!sprite.empty() && objects[i].lookName() != sprite)
				{
					continue;
				}

				// As often as the step owes it, its commands each time.
				for (float owed = seconds; owed > kTimeSlack && t < count() && tickTimer(timers[t], (*clocks())[t], { "object", objects[i].name }, owed);)
				{
					// Its kind's: a command that resets the object leaves them be.
					for (const auto& command : timers[t].commands)
					{
						if (!executor.run(command, Doer::of(&objects[i]))) { break; }
					}
					// Hidden, or out of the look it follows, it stops counting.
					if (!isShown(objects[i]) || (!sprite.empty() && objects[i].lookName() != sprite)) { break; }
				}
				if (!timers[t].name.empty() && t < count()) { showTimeLeft(objects[i], timers[t], (*clocks())[t]); }
			}
		}

		// The current state's timers. A command can change the state, which
		// ends this state's turn for the frame.
		const Name stateName = currentState.top().name;
		auto found = stateTimers.find(stateName);
		if (found == stateTimers.end())
		{
			return;
		}

		const unsigned long changes = stateChanges;
		const unsigned long scenes = sceneChanges;
		const auto unchanged = [&] { return stateChanges == changes && sceneChanges == scenes; };
		const auto count = [&](std::vector<Timer>& timers, const TimerOwner& where)
		{
			for (std::size_t t = 0; t < timers.size() && unchanged(); ++t)
			{
				for (float owed = seconds; owed > kTimeSlack && unchanged() && tickTimer(timers[t], timers[t].clock, where, owed);)
				{
					const std::vector<Command> commands = timers[t].commands;
					takeTurnStop();
					for (const auto& command : commands)
					{
						executor.run(command, Doer{});
						if (takeTurnStop() || !unchanged()) { break; }
					}
				}
			}
		};
		count(found->second, { "state", stateName });

		// Then its current scene's.
		const Name scene = currentScene(stateName);
		const auto sceneFound = sceneTimers.find(scene);
		if (unchanged() && sceneFound != sceneTimers.end())
		{
			count(sceneFound->second, { "scene", scene });
		}
	}

	void Game::shareVariables(const Object& object)
	{
		for (const auto& [variableName, value] : object.variable)
		{
			if (float* held = expressionSlot(object.name, variableName)) { *held = value; }
		}
	}

	float* Game::expressionSlot(Name owner, Name variable)
	{
		// A copy spawned under a new name adds its own.
		if (expressionSlotsOf != expr.objectVariables.size())
		{
			expressionSlots.clear();
			expressionSlotsOf = expr.objectVariables.size();
		}
		const auto [at, added] = expressionSlots.try_emplace({ owner, variable }, nullptr);
		if (added)
		{
			const auto found = expr.objectVariables.find(owner + "." + variable);
			if (found != expr.objectVariables.end()) { at->second = &found->second; }
		}
		return at->second;
	}

	void Game::variableChanged(Name ownerName, Name variableName, float value)
	{
		// Expressions read the variable too (a timer's <every>, worked out again
		// each round), so they see the new value.
		if (float* held = expressionSlot(ownerName, variableName)) { *held = value; }

		for (auto& object : objects)
		{
			if (showsVariable(object, ownerName, variableName))
			{
				relabel(object);
			}
		}
	}

	bool Game::setVariable(Name objectName, Name variableName, float value)
	{
		Object* owner = find(anyTurns ? turnOwner(objectName) : objectName);
		if (!owner)
		{
			return false;
		}

		auto variableIt = owner->variable.find(variableName);
		if (variableIt == owner->variable.end())
		{
			return false;
		}

		variableIt->second = value;
		variableChanged(owner->name, variableName, value);
		return true;
	}

	namespace
	{
		// Shared by Game::resetObject (one named object) and Game::resetAll
		// (every object): every number of its Moment back as it started
		// (whatever has died or been fired since is back: a bullet in flight
		// is put away, an alien is back), and nothing it was in the middle
		// of still going. Doesn't touch bound text displays - callers do
		// that afterward, once they know which objects actually changed.
		void resetObjectState(Object& object, Parts& parts, Resources& resources)
		{
			static_cast<Moment&>(object) = startOf(parts, object);
			object.placed = true;

			// What its keys drive is dropped too, so a key held across the
			// reset cannot bring back part of the old velocity. A <stop />
			// that took the acceleration away is given back above.
			parts.drives.remove(object.id);
			if (Object::Swing* swing = parts.swings.find(object.id)) { swing->held = false; swing->done = 0.0f; }
			if (Object::Picker* picker = parts.pickers.find(object.id))
			{
				picker->picked.clear();
				picker->inChain = false;
				picker->fromColumn = 0;
				picker->fromRow = 0;
				picker->toColumn = 0;
				picker->toRow = 0;
				picker->marks.marked.clear();
				picker->marks.copies.clear();
			}
			if (Object::Rest* rest = parts.rests.find(object.id)) { *rest = {}; }
			if (Object::Cells* cells = parts.cells.find(object.id)) { cells->place = cells->start; }
			object.turning.spin = 0.0f;
			object.showHeading();

			// An animation starts again from its first picture, and a
			// picture with holes in it is whole again.
			object.restartAnimation();
			// Nothing of it seen yet.
			object.dark.seen = false;
			object.dark.hidden = object.dark.kind != Object::Darkness::None;

			if (Object::Unholed* unholed = parts.unholed.find(object.id))
			{
				resources.release(object.drawn.bitmap);
				object.drawn.bitmap = unholed->bitmap;
				parts.unholed.remove(object.id);
				object.drawn.visualDirty = true;
			}

			// Nor should a reset object go on riding, or jump; it
			// faces the way it started, and its timers start over.
			object.riding = {};
			parts.carries.remove(object.id);
			object.hopPending = {};
			parts.flights.remove(object.id);
			object.airborne = false;
			object.footing.grounded = false;
			object.footing.jumping = false;
			object.footing.climbing = false;
			if (Object::Climb* climb = parts.climbs.find(object.id))
			{
				climb->climbOnce = false;
				climb->activeClimb = {};
			}
			object.shifted = {};
			object.aim.reset();
			parts.onPaths.remove(object.id);
			if (Maze* maze = parts.mazes.find(object.id)) { maze->walk = {}; }
			object.blink = 0.0f;
			object.mirror.goingLeft = startOf(parts, object).facing == Direction::Left;
			object.faceMirror();
			object.wearPalette(0);
			if (!object.looks.all().empty()) { object.showLook(object.looks.set->first); }
			if (std::vector<Timer::Clock>* clocks = parts.clocks.find(object.id))
			{
				for (Timer::Clock& clock : *clocks) { clock = {}; }
			}
		}
	}

	void Game::resetObject(Name given)
	{
		const Name name = fullName(given);
		Object* object = find(name);
		if (!object)
		{
			std::cout << "warning: <reset object=\"" << name << "\" />: no such object\n";
			return;
		}

		// A group's name (aliens) means every member of it, and an object's
		// own name (aliens.3.2) just that one.
		for (auto& candidate : objects)
		{
			if (candidate.named(name))
			{
				// A copy did not exist when the game started.
				if (candidate.isCopy()) { despawn(candidate); continue; }
				drawStartAgain(candidate);
				resetObjectState(candidate, objectParts, expr.resources);
				shareVariables(candidate);
				stageChanged();
			}
		}

		// Back where it started, it is no longer carried.
		objectParts.carries.each([&](ObjectId, Object::Carry& carry)
		{
			const Object* carried = carry.what.empty() ? nullptr : find(carry.what);
			if (carried && (carried->named(name)))
			{
				carry.what.clear();
			}
		});

		// Refresh every text display bound to one of this object's variables
		// (same notify pattern as setVariable), so e.g. a HUD showing
		// paddle1.score visibly drops back to 0 immediately, not just internally.
		const std::string owner = object->name;
		for (auto& other : objects)
		{
			const std::vector<TextPiece>& pieces = textPiecesOf(other);
			if (std::any_of(pieces.begin(), pieces.end(), [&](const TextPiece& piece) { return piece.owner == owner; }))
			{
				relabel(other);
			}
		}
	}

	void Game::become(Name target, Name sprite, Name from, Name style)
	{
		bool handChanged = false;
		for (auto& object : objects)
		{
			// A group of players taking turns: the one on its turn.
			const bool offTurn = turnsOf(object).takes && object.inGroup(target) && object.variable[words::turn] == 0.0f;
			if (object.named(target) && !offTurn && (from.empty() || object.lookName() == from))
			{
				const Name before = isHand(object) ? object.lookName() : Name();
				// A holder given a look of what it holds, not its own: what
				// it holds turned to it (a dealer's hidden card shown).
				const bool own = std::any_of(object.looks.all().begin(), object.looks.all().end(), [&](const Object::Look& look) { return look.name == sprite; });
				if (!sprite.empty() && isHolder(object) && !own)
				{
					itemHolders.turnHeld(object, sprite);
					continue;
				}
				if (!sprite.empty()) { showLook(object, sprite); }
				if (!style.empty()) { wearPaletteNamed(object, style); }
				handChanged = handChanged || (isHand(object) && object.lookName() != before);
			}
		}
		// A hand sits centred on what it points at, by the size of its look.
		if (handChanged) { itemHolders.layOutItems(); }
	}

	void Game::reveal(Name target, int count)
	{
		for (auto& object : objects)
		{
			if (count <= 0) { break; }

			const bool named = object.named(target);
			if (!named || object.isVisible || !object.exists || isAway(object)) { continue; }

			// Back in play is a new start: a <random> in it is drawn again,
			// and its timers and animation start over.
			drawStartAgain(object);
			object.restartAnimation();
			if (std::vector<Timer::Clock>* clocks = clocksOf(object))
			{
				for (Timer::Clock& clock : *clocks) { clock = {}; }
			}
			object.position = startOf(object).position;
			object.velocity = startOf(object).velocity;
			object.isVisible = true;
			object.collisionsOn = startOf(object).collisionsOn;
			objectParts.onPaths.remove(object.id);
			--count;
		}
	}

	const std::map<std::string, Path>& Game::getPaths(void) const noexcept
	{
		return paths;
	}

	void Game::follow(Object& object, Name path, float wait)
	{
		const Path& chosen = paths.at(path);
		if (isFollowing(object))
		{
			return;
		}

		objectParts.onPaths.add(object.id, objectParts.orderOf(object.id), Object::OnPath{ path, 0, {}, false, std::max(0.0f, wait) });
		object.velocity = {};
		if (chosen.start)
		{
			object.position = *chosen.start;
		}
	}

	void Game::follow(Name target, Name path, float stagger, int nearest, const Object* from)
	{
		std::vector<Object*> going;
		for (auto& object : objects)
		{
			const bool named = object.named(target);
			if (!named || !object.isVisible || isFollowing(object) || isAway(object) || &object == from) { continue; }
			going.push_back(&object);
		}

		// Only the nearest so many to the one sending them, nearest first.
		if (nearest > 0 && from)
		{
			const Vector2f middle = from->position + from->size * 0.5f;
			const auto apart = [&](const Object* object)
			{
				const Vector2f d = object->position + object->size * 0.5f - middle;
				return dotOf(d, d);
			};
			std::stable_sort(going.begin(), going.end(), [&](const Object* a, const Object* b) { return apart(a) < apart(b); });
			if (going.size() > static_cast<std::size_t>(nearest)) { going.resize(static_cast<std::size_t>(nearest)); }
		}

		int setOff = 0;
		for (Object* object : going)
		{
			follow(*object, path, static_cast<float>(setOff) * stagger);
			++setOff;
		}
	}

	void Game::shift(Name target, Vector2f by)
	{
		const Name name = fullName(target);
		for (auto& object : objects)
		{
			if (!object.named(name)) { continue; }
			object.shifted += by;
			if (!isFollowing(object)) { object.position += by; }
		}
	}

	void Game::applyPaths(float seconds)
	{
		CommandExecutor executor(*this);

		for (auto& object : objects)
		{
			Object::OnPath* onPath = objectParts.onPaths.find(object.id);
			if (!onPath || !isShown(object))
			{
				continue;
			}

			if (onPath->wait > kTimeSlack)
			{
				onPath->wait -= seconds;
				object.velocity = {};
				continue;
			}

			Path& path = paths.at(onPath->name);
			bool moving = false;
			// How far it goes this step: a corner reached on the way is passed
			// (the object put on it) and the rest goes on along the next leg.
			const float t = seconds;
			float budget = path.speed * t;
			while (onPath && onPath->leg < path.steps.size())
			{
				PathStep& leg = path.steps[onPath->leg];
				if (!onPath->legStarted)
				{
					onPath->legStarted = true;
					onPath->left = leg.by;
					if (leg.liveX) { onPath->left.x = stepHalf(*leg.liveX, object); }
					if (leg.liveY) { onPath->left.y = stepHalf(*leg.liveY, object); }

					// A copy: a command may reset the object, which ends the
					// path. Its own commands too, made for it when the game
					// loaded, or, a copy made since, now, once.
					std::vector<Command> commands = leg.commands;
					if (leg.ownWritten)
					{
						auto own = leg.ownBy.find(object.name);
						if (own == leg.ownBy.end())
						{
							own = leg.ownBy.emplace(object.name, expr.pathCommandsFor(leg, object.name,
								"path '" + path.name + "' > <step>, flown by '" + object.name + "'")).first;
						}
						commands.insert(commands.begin(), own->second.begin(), own->second.end());
					}
					for (const auto& command : commands)
					{
						if (!executor.run(command, Doer::of(&object))) { break; }
					}
					// The commands may have ended the path, or moved the parts.
					onPath = objectParts.onPaths.find(object.id);
					if (!onPath) { break; }
				}

				// What is left of this leg: the rest of a step, or the way home
				// from wherever it has got to.
				const Vector2f left = leg.home ? startOf(object).position + object.shifted - object.position : onPath->left;
				const std::optional<Vector2f> step = pathStep(left, budget);
				const bool last = onPath->leg + 1 == path.steps.size();
				const float distance = lengthOf(left);
				if (!step)
				{
					++onPath->leg;
					onPath->legStarted = false;
					continue;
				}

				// Home within this step: it is put there (a place a <shift>
				// keeps moving would otherwise stay a step ahead of it). A
				// corner reached with more legs to come: put on it, the rest
				// of the step for the next leg.
				if (distance <= budget && (leg.home || !last))
				{
					object.position += left;
					object.velocity = {};
					if (!leg.home) { onPath->left = {}; }
					budget -= distance;
					++onPath->leg;
					onPath->legStarted = false;
					continue;
				}

				object.velocity = t > 0.0f ? *step * (1.0f / t) : Vector2f{};
				if (!leg.home) { onPath->left -= *step; }
				// One that faces somewhere faces the way it flies.
				if (object.turning.hasHeading)
				{
					object.heading = headingOf(*step);
					object.showHeading();
				}
				moving = true;
				break;
			}

			// The end of the path: it comes to rest where it is.
			if (onPath && !moving)
			{
				object.velocity = {};
				objectParts.onPaths.remove(object.id);
			}
		}
	}

	void Game::resetAll()
	{
		// A new game starts quiet: whatever loop the last one left on (music,
		// a thrust held into the game over) stops.
		loops.clear();
		++resets;
		turns.clear();
		turnHandedOver = false;

		// What was spawned since the game began is gone.
		spawned.clear();
		for (auto& object : objects)
		{
			if (object.isCopy()) { object.exists = false; }
		}
		settleSpawns();
		spawnTurns.clear();

		for (auto& object : objects)
		{
			if (object.kept) { continue; }
			drawStartAgain(object);
			resetObjectState(object, objectParts, expr.resources);
			shareVariables(object);
			stageChanged();
		}

		for (const auto& state : states)
		{
			stateTimers[state.name] = state.timers;
		}
		startScenes();

		// Refresh every text display bound to another object's variable, now
		// that all of them are back to their starting values (same notify
		// pattern as resetObject/setVariable, just over every bound
		// display instead of one object's worth).
		relabelAll();

		// Collapse the whole state stack. title -> playing -> over ->
		// title -> ... is push-only (see pushState/setCurrentState) - only
		// settings and paused ever pop back off - so left alone it grows for as
		// long as the game keeps getting played. Clear it and start fresh from
		// the very first state, exactly like Engine's constructor does once at
		// startup.
		while (!currentState.empty())
		{
			currentState.pop();
		}
		setCurrentState(0);

		// A new game: the cards shuffled and dealt again, the first player's turn.
		itemHolders.sendStarts();
		startPlayers();
	}

	void Game::relabelAll(void)
	{
		for (auto& object : objects)
		{
			if (objectParts.boundTexts.has(object.id))
			{
				relabel(object);
			}
		}
	}

	void Game::relabel(Object& object)
	{
		Object::BoundText* bound = objectParts.boundTexts.find(object.id);
		if (object.drawn.picture->kind != ShapeKind::Text || !bound)
		{
			return;
		}

		for (TextPiece& piece : bound->pieces)
		{
			if (piece.owner.empty())
			{
				continue;
			}
			// Its own numbers (self.value) read from itself: a copy just
			// spawned is not yet among the objects a name finds.
			const Object* owner = piece.owner == object.name ? &object : find(piece.owner);
			if (!owner)
			{
				continue;
			}
			const auto value = owner->variable.find(piece.variable);
			if (value != owner->variable.end())
			{
				piece.shown = showPiece(piece.kind, value->second, piece.choices);
			}
			else if (const auto live = numberOf(objectParts, *owner, piece.variable))
			{
				piece.shown = showPiece(piece.kind, *live, piece.choices);
			}
		}

		const std::string text = joinText(bound->pieces);
		if (object.drawn.picture->text != text)
		{
			object.drawn.picture.edit().text = text;
			object.drawn.visualDirty = true;
			// The size of its new words, now: what this frame's edges, touches
			// and the positions made from it look at.
			const Vector2f before = object.size;
			object.size = object.pictureSize();
			textsResized = textsResized || object.size != before;
		}
	}

	Name Game::wayLeadsTo(Name way) const
	{
		if (!currentState.empty())
		{
			const State& state = currentState.top();
			const std::string current = currentScene(state.name);
			for (const Scene& scene : state.scenes)
			{
				if (scene.name != current) { continue; }
				const auto found = scene.ways.find(way);
				if (found != scene.ways.end()) { return found->second; }
			}
		}
		std::cout << "warning: way=\"" << way << "\": the current scene has no way of that name\n";
		return {};
	}

	void Game::enter(const CmdEnterScene& command)
	{
		const Name scene = command.way.empty() ? command.scene : wayLeadsTo(command.way);
		if (!scene.empty()) { enterScene(scene); }
		// A new level: nothing to undo into the last.
		undoSteps.clear();
	}

	void Game::reset(const CmdResetScene& command)
	{
		const Name scene = command.way.empty() ? command.scene : wayLeadsTo(command.way);
		if (!scene.empty()) { resetScene(scene); }
	}

	void Game::resetScene(Name scene)
	{
		const auto owner = std::find_if(states.begin(), states.end(), [&](const State& state)
			{
				return std::any_of(state.scenes.begin(), state.scenes.end(), [&](const Scene& each) { return each.name == scene; });
			});
		if (owner == states.end())
		{
			throw std::out_of_range("no scene named '" + scene + "'");
		}

		for (auto& object : objects)
		{
			if (startOf(object).ownerScene != scene) { continue; }
			if (object.isCopy()) { despawn(object); continue; }
			drawStartAgain(object);
			resetObjectState(object, objectParts, expr.resources);
			shareVariables(object);
			stageChanged();

			// Back where it started, it is no longer carried.
			objectParts.carries.each([&](ObjectId, Object::Carry& carry) { if (carry.what == object.name) { carry.what.clear(); } });
		}

		for (const Scene& each : owner->scenes)
		{
			if (each.name == scene) { sceneTimers[scene] = each.timers; }
		}
		relabelAll();
	}

	void Game::updateLockstepObjects(const Object& object) noexcept
	{
		const int lockstepNum = object.kind->collisionData.lockstep;

		// Every member gets the same treatment. A stepped one is taken back by its whole last step, so the rest of
		// the column that met the edge with it are inside again; the others
		// turn at the moment the edge is met (in the sweep), where they are.
		for (auto& obj : objects)
		{
			if (obj.kind->collisionData.lockstep == lockstepNum)
			{
				obj.velocity.x *= -1;
				if (obj.animation.frames.shows().stepped) { obj.position.x += obj.velocity.x * obj.animation.shareBefore; }
			}
		}
	}

	namespace
	{
		// Shared by a collision rule's class/object filter and a state
		// condition's: empty filterClass/filterObject match anything; either or
		// both narrow it to a specific class and/or one specific named object.
		bool matchesClassOrObjectFilter(const Name filterClass, const Name filterObject, const Object& candidate)
		{
			if (!filterClass.empty() && filterClass != candidate.objClass) { return false; }
			if (!filterObject.empty() && !candidate.named(filterObject)) { return false; }
			return true;
		}

		// The name is the object's, or one of its groups', under the path of
		// the scene or state it belongs to: not the name its state gives that
		// piece in every scene.
		bool namedByPath(const Object& object, const Name name)
		{
			const Name owner = object.ownerScene.empty() ? object.ownerState : object.ownerScene;
			return !name.empty() && !owner.empty() && object.named(name)
				&& name.str().compare(0, owner.size() + 1, owner + ".") == 0;
		}

		bool collisionRuleMatches(const CollisionRule& rule, const Object& other)
		{
			return matchesClassOrObjectFilter(rule.filterClass, rule.filterObject, other);
		}
	}

	// The object is at or past this edge (playOneFrame looked): the edge's
	// rules, in the order written.
	void Game::atEdge(Object& object, Edge edge)
	{
		CommandExecutor executor(*this);
		const std::vector<Command>& commands = object.kind->collisionData.edges[edgeIndex(edge)].commands;
		const std::vector<EdgeGuard>& guards = object.kind->collisionData.edges[edgeIndex(edge)].guards;
		// sprite= asks for the look it had as it met the edge, not one an
		// earlier rule changed (or the next would answer a turn again).
		const Name look = object.lookName();
		for (std::size_t i = 0; i < commands.size();)
		{
			// unless= is looked at as it comes, after the rules before it
			const auto guard = std::find_if(guards.begin(), guards.end(), [&](const EdgeGuard& g) { return g.first == i; });
			if (guard != guards.end() && isEdgeRulePassedOver(object, look, *guard))
			{
				i += guard->count;
				continue;
			}
			// A verb refused ends its rule there; the next rule still runs.
			const std::size_t end = guard != guards.end() ? guard->first + guard->count : i + 1;
			while (i < end && executor.run(commands[i], Doer::atEdgeOf(object, edge))) { ++i; }
			i = end;
		}
	}

	bool Game::isEdgeRulePassedOver(const Object& object, Name look, const EdgeGuard& guard)
	{
		return (!guard.sprite.empty() && look != guard.sprite)
			|| (!guard.unlessClass.empty() && isTouchingClass(object, object, guard.unlessClass))
			|| (guard.check && !holdsFor(*guard.check, &object));
	}

	// Whether the edge has a stick() rule that holds right now (one in a rule
	// with sprite= or unless= only while it holds): playOneFrame puts the
	// object back inside after the frame's move, whether or not it is still
	// moving. Other verbs (bounce, reset, inc, ...) run only from atEdge,
	// before the move.
	bool Game::sticksAt(const Object& object, Edge edge)
	{
		const auto& commands = object.kind->collisionData.edges[edgeIndex(edge)].commands;
		const auto& guards = object.kind->collisionData.edges[edgeIndex(edge)].guards;
		for (std::size_t i = 0; i < commands.size(); ++i)
		{
			const bool passedOver = std::any_of(guards.begin(), guards.end(), [&](const EdgeGuard& guard)
				{ return i >= guard.first && i < guard.first + guard.count && isEdgeRulePassedOver(object, object.lookName(), guard); });
			if (!passedOver && std::holds_alternative<CmdStick>(commands[i]))
			{
				return true;
			}
		}
		return false;
	}

	// Whether a rule of either answers to the other (playOneFrame has checked
	// they can touch at all).
	bool Game::canCollide(const Object& a, const Object& b)
	{
		// Pieces of cell maps meet each other by their moves, and a shape
		// touches nothing but by them.
		if ((a.onCells && b.onCells) || (a.onCells && isShape(a)) || (b.onCells && isShape(b))) { return false; }

		const auto answers = [](const Object& self, const Object& other)
		{
			return std::any_of(self.kind->collisionData.others.begin(), self.kind->collisionData.others.end(),
				[&](const CollisionRule& rule) { return collisionRuleMatches(rule, other); });
		};

		return answers(a, b) || answers(b, a);
	}

	std::vector<std::size_t> Game::answerGroups(const std::vector<Object>& among)
	{
		// The names rules pick by (not only a class): what bears one of them
		// is answered for alone, and so is what has such a rule.
		std::set<std::string, std::less<>> picked;
		for (const Object& object : among)
		{
			for (const CollisionRule& rule : object.kind->collisionData.others)
			{
				if (!rule.filterObject.empty()) { picked.insert(rule.filterObject); }
			}
		}
		const auto alone = [&](const Object& object)
		{
			const auto& rules = object.kind->collisionData.others;
			if (std::any_of(rules.begin(), rules.end(), [](const CollisionRule& rule) { return !rule.filterObject.empty(); })) { return true; }
			return std::any_of(picked.begin(), picked.end(), [&](Name name) { return object.named(name); });
		};

		// The rest by what canCollide reads of them: kind (its rules), class,
		// and whether it is a piece or a shape on cells.
		std::map<std::tuple<const void*, const void*, int>, std::size_t> groups;
		std::vector<std::size_t> numbers(among.size());
		for (std::size_t i = 0; i < among.size(); ++i)
		{
			const Object& object = among[i];
			if (alone(object)) { numbers[i] = i; continue; }
			const int cells = (object.onCells ? 1 : 0) + (isShape(object) ? 2 : 0);
			const auto key = std::make_tuple(static_cast<const void*>(object.kind.get()), static_cast<const void*>(&object.objClass.str()), cells);
			numbers[i] = groups.emplace(key, among.size() + groups.size()).first->second;
		}
		return numbers;
	}

	// A hitbox is for what hurts or is taken; what is stood on, stopped at,
	// bounced off or ridden is met by the whole picture, so feet pulled in
	// never sink into a floor.
	bool Game::touchesByPicture(const Object& a, const Object& b)
	{
		const auto has = [](const Object& object) { return !object.hitboxes().empty() || !object.animation.frames.shows().hitboxes.empty()
			|| std::any_of(object.looks.all().begin(), object.looks.all().end(), [](const Object::Look& look) { return !look.frames.shows().hitboxes.empty(); }); };
		if (!has(a) && !has(b)) { return false; }

		const auto solid = [](const Command& command)
		{
			const Command& shape = shapeOf(command);
			return std::holds_alternative<CmdLand>(shape) || std::holds_alternative<CmdStick>(shape) || std::holds_alternative<CmdStop>(shape)
				|| std::holds_alternative<CmdBounce>(shape) || std::holds_alternative<CmdRide>(shape) || std::holds_alternative<CmdKnock>(shape)
				|| std::holds_alternative<CmdDeflect>(shape) || std::holds_alternative<CmdShove>(shape);
		};
		const auto standsOn = [&](const Object& self, const Object& other)
		{
			return std::any_of(self.kind->collisionData.others.begin(), self.kind->collisionData.others.end(), [&](const CollisionRule& rule)
				{
					return collisionRuleMatches(rule, other) && std::any_of(rule.commands.begin(), rule.commands.end(), solid);
				});
		};
		return standsOn(a, b) || standsOn(b, a);
	}

	// A thruster burns 60 units a second of the object's own `variable`,
	// if it has any left (one a frame at 60, as it always did).
	bool Game::burn(Object& object, Name variable)
	{
		const auto fuel = object.variable.find(variable);
		if (fuel == object.variable.end() || fuel->second <= 0.0f) { return false; }
		setVariable(object.name, variable, std::max(0.0f, fuel->second - stepSeconds * 60.0f));
		return true;
	}

	// Whether `object` is touching, right now, any other object in play that is
	// of class `objClass` - `excluding` (the one it is in the middle of a
	// collision with) is left out, and so is `object` itself.
	bool Game::isTouchingClass(const Object& object, const Object& excluding, Name objClass)
	{
		return std::any_of(objects.begin(), objects.end(), [&](const Object& other)
			{
				return &other != &object && &other != &excluding
					&& !other.isAirborne()
					&& other.objClass == objClass
					&& isShown(other) && other.collisionsOn
					&& objectsOverlap(object, other).has_value();
			});
	}

	void Game::retargetCheck(Condition& check, Name from, Name to)
	{
		for (Condition& part : check.parts) { retargetCheck(part, from, to); }
		if (check.filterObject == from) { check.filterObject = to; }
		if (check.distanceTo == from) { check.distanceTo = to; }
		if (check.sees == from) { check.sees = to; }
		if (check.live)
		{
			WrittenValue renamed = *check.live;
			renameInValue(renamed, from, to);
			check.live = std::make_shared<const WrittenValue>(std::move(renamed));
		}
		for (Condition::Read& read : check.reads)
		{
			if (read.owner != from) { continue; }
			read.owner = to;
			read.slot = &expr.objectVariables[to + "." + read.variable];
		}
	}

	bool Game::seesPast(const Object& from, const Object& to, const std::vector<Name>& past)
	{
		const Vector2f start = from.position + from.size * 0.5f;
		const Vector2f end = to.position + to.size * 0.5f;
		for (const Name objClass : past)
		{
			for (const std::size_t index : matching(objClass, Name{}))
			{
				const Object& other = objects[index];
				if (&other == &from || &other == &to || !other.isVisible || !isShown(other)) { continue; }
				if (lineCrosses(start, end, other)) { return false; }
			}
		}
		return true;
	}

	Game::Reaction Game::reactTo(Object& self, Object& other, const Touch& touch)
	{
		CommandExecutor executor(*this);

		// How fast it is going at the moment of the touch (its live speed while
		// the rules run); taken once, so that a rule that stops it does not
		// change which of the rules after it run.
		const Vector2f motion = motionOf(self, 1.0f);
		self.motion.speed = std::sqrt(dotOf(motion, motion));
		self.motion.speedHeld = true;
		// Its look as the touch happens, for sprite=, likewise.
		const std::string look = self.lookName();

		// A rule with unless= is passed over while `self` is also touching
		// something of that class.
		for (auto& rule : self.kind->collisionData.others)
		{
			if (!collisionRuleMatches(rule, other)) { continue; }
			if (!rule.whileSprite.empty() && look != rule.whileSprite) { continue; }
			if (!rule.unlessClass.empty() && isTouchingClass(self, other, rule.unlessClass)) { continue; }
			if (touch.through ? *touch.through != rule.throughClass : !rule.throughClass.empty()) { continue; }
			if (!rule.sides.empty() && std::find(rule.sides.begin(), rule.sides.end(), touch.edge) == rule.sides.end()) { continue; }
			// hitbox= and by=: which of each touched.
			if ((!rule.hitbox.empty() || !rule.by.empty()) && !touchesWith(self, rule.hitbox, other, rule.by)) { continue; }
			if (rule.check && !holdsFor(*rule.check, &self, &other)) { continue; }

			// A verb refused ends the rule there, as it ends an action.
			for (const auto& command : rule.commands)
			{
				if (!executor.run(command, Doer::meeting(self, other, touch))) { break; }
			}
		}
		self.motion.speedHeld = false;
		return { executor.throughAsked };
	}

	void Game::refreshLive(void)
	{
		if (!liveLinked || liveLinkedAt != expr.liveRead)
		{
			liveLinked = true;
			liveLinkedAt = expr.liveRead;
			liveLinks.clear();
			const std::vector<LivePart>& live = liveParts();
			for (std::size_t i = 0; i < objects.size(); ++i)
			{
				const std::vector<float*>& numbers = liveNumbersFor(objects[i].name);
				for (std::size_t part = 0; part < numbers.size(); ++part)
				{
					if (!numbers[part] || (live[part].has && !live[part].has(objectParts, objects[i]))) { continue; }
					liveLinks.push_back({ numbers[part], i, part });
				}
			}
		}

		const std::vector<LivePart>& parts = liveParts();
		for (const LiveLink& link : liveLinks)
		{
			*link.number = parts[link.part].of(objectParts, objects[link.object]);
		}
	}

	const std::vector<float*>& Game::liveNumbersFor(Name owner)
	{
		// Calc reads more of them: each is found again.
		if (liveNumbersAt != expr.liveRead)
		{
			liveNumbersOf.clear();
			liveNumbersAt = expr.liveRead;
		}
		const auto [at, added] = liveNumbersOf.try_emplace(owner);
		if (added)
		{
			const std::string prefix = Calc::nameOf(owner) + ".";
			for (const std::string& part : livePartNames())
			{
				const std::string name = prefix + part;
				const auto found = expr.liveNumbers.find(name);
				at->second.push_back(found != expr.liveNumbers.end() && expr.calc.has(name) ? &found->second : nullptr);
			}
		}
		return at->second;
	}

	void Game::follow(void)
	{
		for (Object& object : getCurrentObjects())
		{
			const Object::Follows* follows = objectParts.follows.find(object.id);
			if (!follows || !object.exists) { continue; }
			for (const Object::Follow& each : follows->each)
			{
				const float value = evaluateFor(each.value, object);
				switch (each.part)
				{
				case Object::Follow::Part::X: object.position.x = value + object.layout.cellOffset.x; break;
				case Object::Follow::Part::Y: object.position.y = value + object.layout.cellOffset.y; break;
				case Object::Follow::Part::Heading:
				{
					float degrees = std::fmod(value, 360.0f);
					if (degrees < 0.0f) { degrees += 360.0f; }
					if (degrees != object.heading) { object.heading = degrees; object.showHeading(); }
					break;
				}
				case Object::Follow::Part::Variable:
				{
					const auto held = object.variable.find(each.variable);
					if (held != object.variable.end() && held->second != value)
					{
						held->second = value;
						variableChanged(object.name, each.variable, value);
					}
					break;
				}
				case Object::Follow::Part::Seconds:
					object.timeAnimation(each.look, each.frame, std::max(0.0f, value));
					break;
				}
			}
		}
	}

	float Game::stepHalf(const WrittenValue& value, const Object& follower)
	{
		WrittenValue own = value;
		renameInValue(own, "self", follower.name);
		return evaluateNow(own, "object '" + follower.name + "' > <follow> > <step>");
	}

	float Game::evaluateNow(const WrittenValue& value, const std::string& where)
	{
		refreshLive();
		return expr.evaluate(value, where);
	}

	float Game::evaluateFor(const WrittenValue& value, const Object& object)
	{
		// Where it is written is told only when it fails.
		try { return evaluateNow(value, std::string{}); }
		catch (const std::runtime_error& error) { throw std::runtime_error("object '" + object.name + "'" + error.what()); }
	}

	Command Game::makeNow(const CmdLive& live, const Object* self, const Object* other)
	{
		try
		{
			// One that says self, for the object running it, and other, for
			// the one it touched; with none to say, as it was made at load.
			if (live.namesSelf || live.namesOther)
			{
				if ((live.namesSelf && !self) || (live.namesOther && !other)) { return live.loaded->command; }
				// Into one kept for it, so its texts' room is used again.
				ownCommands.resize(1);
				ownCommands.front() = *live.written;
				if (live.namesSelf) { nameSelfIn(ownCommands.front(), self->name); }
				if (live.namesOther) { renameObjectIn(ownCommands, "other", other->name); }
				return makeCommand(ownCommands.front(), [&](const WrittenValue& value) { return evaluateNow(value, live.where); });
			}
			return makeCommand(*live.written, [&](const WrittenValue& value) { return evaluateNow(value, live.where); });
		}
		catch (const std::runtime_error& error)
		{
			if (liveWarned.insert(live.where + error.what()).second)
			{
				std::cout << "warning: " << error.what() << "; it does what it did as the game loaded\n";
			}
			return live.loaded->command;
		}
	}

	bool Game::setNumber(const VariableName& target, float value)
	{
		if (variableOf(target))
		{
			return setVariable(target.owner, target.variable, value);
		}
		const Name owner = target.owner;
		const LivePart* part = livePartNamed(target.variable);
		if (!part)
		{
			std::cout << "warning: <set variable=\"" << target.written() << "\">: no such variable\n";
			return false;
		}

		bool found = false;
		for (Object& object : objects)
		{
			if (!object.named(owner)) { continue; }
			found = true;
			const bool heading = std::string(part->name) == "heading";
			if (heading && !object.turning.hasHeading) { continue; }
			part->of(objectParts, object) = part->notBelowZero ? std::max(0.0f, value) : value;
			if (heading) { object.showHeading(); }
		}
		if (!found)
		{
			std::cout << "warning: <set variable=\"" << target.written() << "\">: no object named '" << owner << "'\n";
		}
		return found;
	}

	// Each side sees the touch from its own point of view: the edge of b that
	// was touched is b's own side of it, and a's is the opposite one.
	void Game::touched(Object& a, Object& b, const Contact& contact)
	{
		reactTo(a, b, touchOfFirst(contact));
		reactTo(b, a, touchOfSecond(contact));
	}

	void Game::checkConditions()
	{
		// currentState.top() directly, rather than getCurrentState() (which
		// returns a copy of the whole State) - this runs every frame, so
		// copying every command list in it just to read .conditions would be
		// wasteful. The state's own conditions, then its current scene's.
		const State& state = currentState.top();
		if (checkConditionList(state.conditions)) { return; }

		const std::string scene = currentScene(state.name);
		const auto found = std::find_if(state.scenes.begin(), state.scenes.end(), [&](const Scene& each) { return each.name == scene; });
		if (found != state.scenes.end()) { checkConditionList(found->conditions); }
	}

	std::set<KeyCode> Game::computerButtons(std::set<KeyCode>& taps, float seconds)
	{
		std::set<KeyCode> held;
		bool thought = false;
		// A <think> with no class= plays the pieces of the player on its turn.
		const auto play = [&](const CmdThink& written)
		{
			thought = true;
			CmdThink each = written;
			if (each.objClass.empty()) { each.objClass = turnClass(); }
			if (const auto key = thinker.think(*this, each, seconds)) { taps.insert(*key); }
		};
		for (const Condition& condition : currentState.top().computer)
		{
			if (!holds(condition)) { continue; }
			for (const Command& command : condition.commands)
			{
				if (const auto* hold = std::get_if<CmdHold>(&shapeOf(command))) { held.insert(hold->button); }
				if (const auto* each = std::get_if<CmdThink>(&shapeOf(command))) { play(*each); }
			}
		}
		// A player the computer plays, on its turn.
		for (const auto& [name, plays] : memberThinks)
		{
			const Object* member = find(name);
			if (!member || !member->exists || !isShown(*member) || member->variable.find(words::turn)->second == 0.0f) { continue; }
			play(plays);
		}
		// Its turn is over (or the game was put back): the next is planned anew.
		if (!thought)
		{
			thinker.stop();
			itemHolders.itemWait = Holders::kItemPace;
		}
		return held;
	}

	bool Game::checkConditionList(const std::vector<Condition>& conditions)
	{
		for (auto& condition : conditions)
		{
			if (condition.each.empty())
			{
				if (!holds(condition)) { continue; }
				runConditionCommands(condition.commands);
				return true; // the state may have just changed - stop for this frame
			}
			// Commands that name self: for each one it holds for, in order.
			std::vector<const Object*> who;
			if (!holds(condition, &who)) { continue; }
			for (const Condition::Each& each : condition.each)
			{
				if (std::any_of(who.begin(), who.end(), [&](const Object* one) { return one->name == each.object; }))
				{
					runConditionCommands(each.commands);
				}
			}
			runConditionCommands(condition.commands);
			return true;
		}
		return false;
	}

	const std::vector<std::size_t>& Game::matching(Name filterClass, Name filterObject)
	{
		auto found = filtered.find({ filterClass, filterObject });
		if (found == filtered.end())
		{
			std::vector<std::size_t> indices;
			for (std::size_t i = 0; i < objects.size(); ++i)
			{
				if (matchesClassOrObjectFilter(filterClass, filterObject, objects[i])) { indices.push_back(i); }
			}
			found = filtered.emplace(std::pair<Name, Name>{ filterClass, filterObject }, std::move(indices)).first;
		}
		return found->second;
	}

	// A class, or a name every scene has (robots, for playing.robots),
	// is what is in this state and scene (and the game's own); a name
	// by its path (room2.robots, playing.room2.robots.3) is that
	// thing, wherever it is.
	bool Game::considers(const Condition& condition, const Object& object) const
	{
		return (inScope(object) || namedByPath(object, selfIn(condition, condition.filterObject)))
			&& (condition.sprite.empty() || object.lookName() == condition.sprite);
	}

	// The number compared with: as it was at load, or, when it names
	// something that changes, as it is now. One that cannot be worked out
	// now stays as it was at load, with one warning.
	float Game::comparedWith(const Condition& condition)
	{
		float against = condition.value;
		if (condition.live)
		{
			for (const Condition::Read& read : condition.reads)
			{
				if (const Object* owner = find(read.owner))
				{
					if (const auto found = owner->variable.find(read.variable); found != owner->variable.end()) { *read.slot = found->second; }
				}
			}
			const Object* self = condition.saysSelf && !condition.madeFor.empty() ? (testing ? testing : find(condition.madeFor)) : nullptr;
			const Object* other = condition.saysSelf && !condition.madeAgainst.empty() ? (testingOther ? testingOther : find(condition.madeAgainst)) : nullptr;
			const auto readOwn = [&](const Object* owner, const std::vector<Name>& reads)
			{
				if (!owner) { return; }
				for (const Name variable : reads)
				{
					float* const held = expressionSlot(owner->name, variable);
					const auto found = owner->variable.find(variable);
					if (held && found != owner->variable.end()) { *held = found->second; }
				}
			};
			readOwn(self, condition.selfReads);
			readOwn(other, condition.otherReads);
			try
			{
				const std::string* was = std::exchange(expr.selfNow, self ? &self->name.str() : nullptr);
				const std::string* wasOther = std::exchange(expr.otherNow, other ? &other->name.str() : nullptr);
				try { against = evaluateNow(*condition.live, condition.where); }
				catch (...) { expr.selfNow = was; expr.otherNow = wasOther; throw; }
				expr.selfNow = was;
				expr.otherNow = wasOther;
			}
			catch (const std::runtime_error& error)
			{
				if (liveWarned.insert(condition.where + error.what()).second)
				{
					std::cout << "warning: " << error.what() << "; the condition compares with what it was as the game loaded\n";
				}
			}
		}
		return against;
	}

	// What is tested of one object: how far it is from the nearest of
	// the other (both in play), or its variable.
	std::optional<float> Game::measure(const Condition& condition, const Object& object)
	{
		if (!condition.sees.empty())
		{
			if (!object.isVisible || !isShown(object)) { return std::nullopt; }
			const Object* nearest = nearestInPlay(object, selfIn(condition, condition.sees));
			if (!nearest) { return std::nullopt; }
			return seesPast(object, *nearest, condition.past) ? 1.0f : 0.0f;
		}
		if (condition.distanceTo.empty()) { return numberOf(objectParts, object, condition.variableName); }
		if (!object.isVisible || !isShown(object)) { return std::nullopt; }
		const Object* nearest = nearestInPlay(object, selfIn(condition, condition.distanceTo));
		if (!nearest) { return std::nullopt; }
		const Vector2f apart = (nearest->position + nearest->size * 0.5f) - (object.position + object.size * 0.5f);
		return lengthOf(apart);
	}

	// A total over everything that matches and is in play.
	bool Game::holdsTotal(const Condition& condition, const std::vector<std::size_t>& candidates, float against)
	{
		float total = 0.0f;
		std::size_t counted = 0;
		for (const std::size_t index : candidates)
		{
			const Object& object = objects[index];
			if (!object.isVisible || !considers(condition, object)) { continue; }
			if (condition.total == Total::Count)
			{
				++counted;
				continue;
			}
			const std::optional<float> number = measure(condition, object);
			if (!number) { continue; }
			const float value = *number;
			if (condition.total == Total::Sum) { total += value; }
			else if (counted == 0) { total = value; }
			else if (condition.total == Total::Highest) { total = std::max(total, value); }
			else { total = std::min(total, value); }
			++counted;
		}
		if (condition.total == Total::Count)
		{
			total = static_cast<float>(counted);
		}
		else if (counted == 0 && condition.total != Total::Sum)
		{
			return false; // no highest or lowest of nothing
		}
		return passes(condition.compare, total, against);
	}

	// The lowest or highest of them: the first that has it.
	bool Game::holdsExtreme(const Condition& condition, const std::vector<std::size_t>& candidates, std::vector<const Object*>* who)
	{
		const Object* best = nullptr;
		float extreme = 0.0f;
		for (const std::size_t index : candidates)
		{
			const Object& object = objects[index];
			if (!considers(condition, object)) { continue; }
			const std::optional<float> number = measure(condition, object);
			if (!number) { continue; }
			if (!best || (condition.compare == Compare::Lowest ? *number < extreme : *number > extreme))
			{
				best = &object;
				extreme = *number;
			}
		}
		if (best && who) { who->push_back(best); }
		return best != nullptr;
	}

	Name Game::selfIn(const Condition& condition, Name name) const noexcept
	{
		if (!condition.saysSelf) { return name; }
		if (name == words::self) { return testing ? testing->name : condition.madeFor; }
		if (name == words::other) { return testingOther ? testingOther->name : condition.madeAgainst; }
		return name;
	}

	bool Game::holdsFor(const Condition& condition, const Object* self, const Object* other)
	{
		const Object* was = std::exchange(testing, self);
		const Object* wasOther = std::exchange(testingOther, other);
		const bool held = holds(condition);
		testing = was;
		testingOther = wasOther;
		return held;
	}

	bool Game::holds(const Condition& condition, std::vector<const Object*>* who)
	{
		if (condition.join == WrittenCondition::Join::All)
		{
			for (std::size_t i = 0; i < condition.parts.size(); ++i)
			{
				if (!holds(condition.parts[i], i + 1 == condition.parts.size() ? who : nullptr)) { return false; }
			}
			return true;
		}
		if (condition.join == WrittenCondition::Join::Any)
		{
			return std::any_of(condition.parts.begin(), condition.parts.end(), [&](const Condition& part)
				{
					if (who) { who->clear(); }
					return holds(part, who);
				});
		}

		// Only the objects the filter matches are looked at (matching).
		const std::vector<std::size_t>& candidates = matching(condition.filterClass, selfIn(condition, condition.filterObject));
		const float against = comparedWith(condition);

		// "No more than N of them left": everything that matches, still
		// in play.
		if (condition.remaining)
		{
			const auto stillIn = std::count_if(candidates.begin(), candidates.end(), [&](std::size_t index)
				{
					return objects[index].isVisible && considers(condition, objects[index]);
				});
			return static_cast<float>(stillIn) <= (condition.live ? against : *condition.remaining);
		}

		if (condition.total != Total::None) { return holdsTotal(condition, candidates, against); }
		if (condition.compare == Compare::Lowest || condition.compare == Compare::Highest) { return holdsExtreme(condition, candidates, who); }

		bool held = false;
		for (const std::size_t index : candidates)
		{
			const Object& object = objects[index];
			if (!considers(condition, object))
			{
				continue;
			}

			const std::optional<float> tested = condition.compare == Compare::Shows ? std::optional<float>{ 0.0f } : measure(condition, object);
			if (tested && passes(condition.compare, *tested, against))
			{
				if (!who) { return true; }
				who->push_back(&object);
				held = true;
			}
		}
		return held;
	}

	// A copy of the commands, not the state's own list: the list belongs to the
	// current state, and a <pop /> among them frees that state while the rest
	// of the list is still to run.
	void Game::runConditionCommands(std::vector<Command> commands)
	{
		CommandExecutor executor(*this);
		takeTurnStop();
		for (const auto& command : commands)
		{
			if (!executor.run(command, Doer{}) || takeTurnStop()) { break; }
		}
	}
}
