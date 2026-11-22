// expr.h
// XML Game Engine
// author: beefviper
// date: Sept 18, 2020

#pragma once

#include "picture/bitmap.h"
#include "picture/image.h"
#include "engine/resources.h"
#include "game/parts.h"
#include "picture/color.h"
#include "game/command.h"
#include "load/calc.h"
#include "game/object.h"
#include "game/random.h"
#include "audio/sound.h"
#include "game/states.h"

#include <functional>
#include <iostream>
#include <map>
#include <memory>
#include <optional>
#include <set>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

namespace xge
{
	// Whether a text sprite has a <number> or a <letter> among its pieces.
	bool showsNumbers(const WrittenSprite& sprite);

	// Works out what load/xml read: every WrittenValue to a number, every
	// WrittenSprite to the Picture the window backends draw, every
	// WrittenCommand to a typed Command. A value's text is load/calc's job.
	class Evaluator
	{
	public:
		Evaluator() = default;
		Evaluator(Evaluator&&) noexcept = default;
		Evaluator& operator=(Evaluator&&) noexcept = default;
		Evaluator(const Evaluator&) = delete;
		Evaluator& operator=(const Evaluator&) = delete;

		// <people />, how many people play, as texts read it (@people).
		void seatPeople(int count) noexcept { *people = static_cast<float>(count); }

		// Loading is over: from now on a division by 0 warns instead of
		// throwing (evaluateOperation), and the sheets' pictures, held while
		// their cells were cut, are let go.
		void finishLoading()
		{
			loading = false;
			resources.letGoOfSheets();
		}

		// What an object's sprite was drawn as before it was built (init's
		// first pass, or the object a copy is spawned from): its sprite's
		// Picture, the bitmap, what turns, an animation's pictures, and,
		// when given, its looks ready made.
		struct FirstPass
		{
			Picture picture;
			PictureId bitmap;
			std::shared_ptr<const Turnable> turned;
			std::vector<PictureId> animation;
			std::vector<std::shared_ptr<const Turnable>> animationTurned;
			const std::vector<Object::Look>* looks{ nullptr };
		};

		// One object, made from what the file says of it (after init's
		// first pass, or for a copy spawned while the game runs).
		Object makeObject(WrittenObject& writtenObject, const FirstPass& first, Vector2f cellOffset, int lockstep);

		// A written <drag> or <grip> as the rate it loses speed at (load/build.cpp).
		float lossRate(float written, const std::string& what) const;

		// makeObject's looks: each named <sprite> or <animation> made, the first the
		// very picture it starts with, and where each once look goes when it ends.
		std::vector<Object::Look> buildLooks(WrittenObject& writtenObject, const FirstPass& first, const HeldPicture& picture, const std::string& where);
		// makeObject's picture: the sprite it starts as, its animation, and a
		// text's live pieces.
		void buildSprite(WrittenObject& writtenObject, const FirstPass& first, const HeldPicture& picture, float animationSeconds, Object& object, const std::string& where);
		// makeObject's names: its group, class and owners, and whether it is
		// drawn, seen and there.
		void buildNames(WrittenObject& writtenObject, Object& object);
		// makeObject's numbers: its variables, then where it starts, what it
		// follows, its velocity, acceleration and heading.
		void buildNumbers(WrittenObject& writtenObject, Object& object, Vector2f cellOffset, const std::vector<std::string>& positionSizeDependencies, const std::string& where);
		// makeObject's ways of moving: drag, surface, friction, mass, pulls,
		// seeking, light and dark, altitude, a tether and which way it faces.
		void buildMotion(WrittenObject& writtenObject, Object& object, const std::string& where);
		// makeObject's timers, a named one's time left a number of its own.
		void buildTimers(WrittenObject& writtenObject, Object& object, const std::string& where);
		// makeObject's looks given it, the first one's animation, the look for
		// each way it moves (<poses>) and for where what it is joined to lies.
		void buildLookParts(WrittenObject& writtenObject, Object& object, const std::vector<Object::Look>& looks, const std::string& where);
		// makeObject's board-game moves and marks, each set's look found.
		void buildMoves(WrittenObject& writtenObject, Object& object, const std::vector<Object::Look>& looks, const std::string& where);
		// makeObject's holder (<holds>), hand, held item and turns.
		void buildHolder(WrittenObject& writtenObject, Object& object, std::uint64_t order, const std::string& where);
		// makeObject's mirror, grip, timers' looks, its tile on a map, a
		// maze's walls and its pivot.
		void buildPlace(WrittenObject& writtenObject, Object& object, const std::vector<Object::Look>& looks, const std::string& where);
		// makeObject's collisions: on or off, the kind, solid pixels, hitboxes
		// (its own and each frame's), its edge rules and its rules with others.
		void buildCollisions(WrittenObject& writtenObject, Object& object, const std::vector<Object::Look>& looks, int thisLockstep, const std::string& where);
		// makeObject's actions.
		void buildActions(WrittenObject& writtenObject, Object& object, const std::string& where);
		// init's names: the window's edges, the game's variables, every object's
		// variables and timers, and the live numbers, as names a text reads.
		void nameNumbers(const WindowDesc& windowDesc, const std::vector<std::pair<std::string, WrittenValue>>& writtenVariables, std::map<std::string, float>& variables, std::vector<WrittenObject>& writtenObjects);
		// init's objects: every sprite measured once (its size a name), the
		// cells of groups laid out, then each object made, groups in lockstep.
		void makeObjects(std::vector<WrittenObject>& writtenObjects, std::vector<Object>& objects);
		// init's tests on collision rules, edge guards and actions' conditions,
		// once every object they can name is made.
		void makeRuleTests(const std::vector<WrittenObject>& writtenObjects, std::vector<Object>& objects);
		// init's states: each one's inputs, conditions, timers and scenes.
		void makeStates(std::vector<WrittenState>& writtenStates, std::vector<State>& states, const std::vector<Object>& objects);
		// init's sounds, after everything else so that a length can use any variable.
		void makeSounds(const std::vector<WrittenSound>& writtenSounds, std::vector<SoundDesc>& sounds);
		// init's paths, worked out last too, and each step's commands about
		// its follower made for each object that can fly it.
		void makePaths(const std::vector<WrittenPath>& writtenPaths, std::map<std::string, Path>& paths, const std::vector<State>& states, const std::vector<Object>& objects);
		// What a <condition> tests, made ready (with its <all> or <any>'s
		// checks, each made the same way), checked against the objects.
		Condition makeTest(const WrittenCondition& writtenCondition, const std::string& where, const std::string& owner, const std::vector<Object>& objects);
		Condition makeTestHere(const WrittenCondition& writtenCondition, const std::string& where, const std::string& owner, const std::vector<Object>& objects);
		// makeTestHere's checks: what a test names (what it sees and what is in
		// the way, a look, a distance, a variable) is something the game has.
		void checkTestNames(const WrittenCondition& writtenCondition, const Condition& condition, const std::string& where, const std::string& owner, const std::vector<Object>& objects);
		// A state's condition: its test, and its commands; or, when they name
		// self or others, its commands for each object it may match.
		Condition makeCondition(const WrittenCondition& writtenCondition, const std::string& where, const std::string& owner, const std::string& stateName, const std::vector<Object>& objects);
		Condition makeConditionHere(const WrittenCondition& writtenCondition, const std::string& where, const std::string& owner, const std::string& stateName, const std::vector<Object>& objects);

		// buildPicture's steps 1 to 3 for a line, polygon, cell, svg or bitmap:
		// read, drawn into a Bitmap and flipped; a line's segments kept to turn.
		PictureId drawPicture(const WrittenSprite& sprite, const std::string& where, const std::string& color, const std::shared_ptr<Turnable>& kept);
		// A copy of `from` spawned while the game runs, from `written` (what
		// the file says of `from`, under the copy's name): its names
		// made known to the expressions (its variables, where it is, its
		// size) and the object made, its pictures shared with `from`'s.
		Object spawnObject(WrittenObject& written, const Object& from);

		void init(const WindowDesc& windowDesc,
			const std::vector<std::pair<std::string, WrittenValue>>& writtenVariables, std::map<std::string, float>& variables,
			std::vector<WrittenState>& writtenStates, std::vector<State>& states,
			std::vector<WrittenObject>& writtenObjects, std::vector<Object>& objects,
			const std::vector<WrittenSound>& writtenSounds, std::vector<SoundDesc>& sounds,
			const std::vector<WrittenPath>& writtenPaths, std::map<std::string, Path>& paths);

		// Every object's size, read as name.width and name.height unless the
		// object has a <variable> of that name. A text's or image's is {0,0}
		// until setObjectSize() gives what a backend measured. A std::map, so
		// the numbers stay where calc bound them as more are added.
		std::map<std::string, Vector2f> objectSizes;
		std::map<std::string, ShapeKind> objectShapeKinds;
		void setObjectSize(const std::string& name, const Vector2f& size);

		// The objects, other than shapes, whose size writtenObject's <position> uses
		// (by "name.width" / "name.height"): the ones only a backend can measure.
		std::vector<std::string> sizeDependenciesOf(const WrittenObject& writtenObject) const;

		bool loading{ true };

		// While a command's values are worked out at load: they may name
		// liveNumbers (reading 0), since they are worked out again when the
		// command runs.
		bool commandValues{ false };
		std::set<std::string> warnedDivisions;

		// The object whose rules, timers and actions are being made: a
		// command of theirs that says self is made as it runs (CmdLive), for
		// the object running it; the one made at load is about this one.
		std::string selfName;

		// Once loaded and checked: the objects of one kind share the first's
		// Kind (see Kind).
		void shareKinds(std::vector<Object>& objects);
		// Objects whose looks came out the same (a group's members, a board's
		// pieces) share one LookSet.
		void shareLooks(std::vector<Object>& objects);
		// The same for other, in a rule about touching something: the
		// object a command saying other is made for at load.
		std::string otherName;

		// While a test about one object is worked out (Game::comparedWith):
		// its name, which self. in a text reads as; selfText is the room the
		// text is named in.
		const std::string* selfNow{ nullptr };
		const std::string* otherNow{ nullptr };
		std::string selfText;

		// The names a text may read, and every text compiled so far
		// (evaluateExpression); with the names each text reads (namesIn):
		// the same texts are worked out again and again.
		Calc calc;
		std::unordered_map<std::string, Calc::Program> compiled;
		mutable std::unordered_map<std::string, std::vector<std::string>> namesInText;
		const std::vector<std::string>& namesIn(const std::string& text) const;

		// A number a text may read by name (an object's variable, size or live
		// number), bound into calc only once a text reads it: a big game has
		// thousands, and most are never read. Names are without case; the
		// first bound under a name keeps it.
		void bind(const std::string& name, float& value);
		std::unordered_map<std::string, float*> unbound;

		// A value's number: its expression worked out, or the number its tag
		// makes. `where` says whose value it is, for the message if the
		// expression does not compile (std::runtime_error).
		float evaluate(const WrittenValue& value, const std::string& where);
		// The same for where an object starts: an update="live" one may read
		// what is only known while the game runs (Object::follows).
		float evaluateStart(const WrittenValue& value, const std::string& where);

		// A path step's commands about its flier (PathStep::ownWritten), made for
		// the object `follower`: self is its name.
		std::vector<Command> pathCommandsFor(const PathStep& step, const std::string& follower, const std::string& where);

		// A number worked out, and whether it may still change: `late` is true
		// when it was made from something that has no final value while the game
		// is loading (another object's variable, which reads 0 until that object
		// is built, or an object's size, which reads 0 until a window has
		// measured a text or image). Such a number is worked out again once it
		// is known (a position, a timer's interval).
		struct Answer
		{
			float value{ 0.0f };
			bool late{ false };
		};

		// One <add>, <subtract>, <multiply> or <divide>: its first operand
		// combined with each of the others in turn. A divisor of 0 gives 0:
		// while loading it is an error unless the divisor is `late`; while
		// the game runs, where throwing would end it, a warning once a place.
		Answer evaluateOperation(const WrittenOperation& operation, const std::string& where);

		// Whether an operand's name is something that is `late` while loading.
		bool isLate(const std::string& name) const;
		float evaluateExpression(const std::string& text, const std::string& where);

		// The Picture for a sprite, as the window backends draw it. For a
		// drawing of lines, a <bitmap> (rows of text) or an <svg>, a Line the
		// size of the drawing, and the drawing itself is handed back through
		// `bitmap` when that is given (see Object::Drawn).
		//
		// `turnable`, when given, is for an object that has a <heading>: what a
		// drawing of lines or a <bitmap> is to be turned from is handed back
		// through it (see Turnable), `bitmap` is the drawing at heading 0, and
		// the picture's size is the square it turns in.
		//
		// A text's pieces that name a variable of `self`'s group (scores.score
		// in a member of the group scores) mean `self`'s own.
		Picture buildPicture(const WrittenSprite& sprite, const std::string& where,
			PictureId* bitmap = nullptr,
			std::shared_ptr<const Turnable>* turnable = nullptr,
			const WrittenObject* self = nullptr);

		// The pieces of a text sprite as they show now: a <number> or a
		// <letter> that is just owner.variable follows that variable (of
		// `self`, for its group's name), the rest worked out once.
		std::vector<TextPiece> textPieces(const WrittenSprite& sprite, const std::string& where, const WrittenObject* self);

		// Where each cell of a group laid out in <columns> and <rows> goes,
		// from the group's position, by its name (see WrittenCell). `pictures`
		// is every object's sprite as it is drawn, for the size of a cell whose
		// sprite is not its row's.
		std::map<std::string, Vector2f> layOutCells(const std::vector<WrittenObject>& writtenObjects,
			const std::map<std::string, Picture>& pictures);

		// The picture `key` names (everything it is drawn from), drawn by
		// `draw` the first time and shared by the game's Resources after
		// that, so the 55 cells of a block of aliens are one picture.
		template <typename Draw>
		PictureId drawOnce(const std::string& key, Draw&& draw)
		{
			return resources.picture(key, std::forward<Draw>(draw));
		}


		// Every picture the game has loaded or made, each held once.
		Resources resources;

		// The game's handles and parts (Game::parts), which each object made
		// joins.
		Parts* parts{ nullptr };
		// How the object being made started (Parts::starts), given it the first time.
		Moment& startFor(const Object& object) { return parts->give(parts->starts, object.id); }

		// The game's <styles>, for the palettes a <become style> asks for.
		std::vector<WrittenStyle> styles;


		// The game's <sheets> (their pictures are opened by the Resources).
		std::vector<Sheet> sheets;

		// Each object's palettes (Object::Looks::palettes), for the styles `wanted`
		// names for it by its name, drawn from what the file says of it.
		// Throws std::runtime_error for one with a <heading>, or with no
		// bitmap that takes a style.
		void makePalettes(const std::vector<WrittenObject>& writtenObjects, std::vector<Object>& objects, const std::map<std::string, std::vector<std::string>>& wanted);

		// The flips (Object::Mirror::flips) of every picture `object` can show.
		std::shared_ptr<const std::map<PictureId, PictureId>> flipsOf(const Object& object);

		// The pictures of an object's <animation>, each drawn once, in the order
		// of its <frame>s. Throws std::runtime_error (saying whose) when a frame
		// is not a <bitmap> or a drawing of <line>s, or when the frames are not
		// all the same size. `turned`, when given, is for an
		// object with a <heading>: it gets what each frame turns from,
		// and every frame must turn in the same size of square.
		std::vector<PictureId> buildAnimationBitmaps(const std::vector<WrittenSprite>& frames, const std::string& where,
			std::vector<std::shared_ptr<const Turnable>>* turnables = nullptr);

		// How many seconds each picture of the animation is shown for: its
		// <interval> (0 for a live one, until it is worked out). Throws
		// std::runtime_error for an interval that is not above 0.
		float animationSecondsOf(const WrittenValue& interval, const std::string& where);

		// Each picture's own time in seconds: a frame's own <seconds> as
		// animationSecondsOf reads them, any other `fallback`. Empty when no
		// frame has its own.
		std::vector<float> animationTimesOf(const std::vector<WrittenValue>& times, float fallback, const std::string& where);
		// Each frame's own commands, made (empty when no frame has any).
		std::vector<std::vector<Command>> frameCommandsOf(const std::vector<std::vector<WrittenCommand>>& frames, const std::string& where);

		std::vector<Command> processCommands(const std::vector<WrittenCommand>& written, const std::string& where);

		// An object's or a state's <timers>: the commands made, the interval
		// checked and kept as written (see Timer).
		std::vector<Timer> processTimers(const std::vector<WrittenTimer>& written, const std::string& where);

		// A <hitbox> worked out for an object whose picture is made.
		Hitbox makeHitbox(const WrittenHitbox& written, const Object& object, const std::string& where);

		// A <sound> worked out: its words checked and its lengths evaluated.
		SoundDesc processSound(const WrittenSound& written);

		// Every object's <variable>s as owner.name, so any value can read
		// another object's (a score's text showing paddle1.score). A std::map,
		// so the numbers stay where calc bound them as more are added.
		std::map<std::string, float> objectVariables;

		// The numbers every object has while the game runs (livePartNames:
		// ball.x, ball.velocity.y, ...), bound into symbolTable as the variables
		// are. Before a value is worked out while the game runs,
		// Game writes them from where everything is (Game::evaluateNow).
		// While loading they have no meaning yet, and a value that names one
		// stops the load, unless it is a command's (see processCommands).
		// Each is under its name as a text reads it (Calc::nameOf), and goes
		// into calc the first time a text reads it.
		std::unordered_map<std::string, float> liveNumbers;
		// On the heap, so a text compiled to read it still can after a move.
		std::unique_ptr<float> people = std::make_unique<float>(1.0f);
		// How many of them have gone into calc so far; one that goes in while
		// the game runs is first given its number by whenLiveRead.
		std::size_t liveRead = 0;
		std::function<void()> whenLiveRead;

		// The texts already found to name none of liveNumbers while loading,
		// so a text many objects share (a map's pieces) is looked through once.
		std::set<std::string> loadCheckedTexts;

		// Whether a value names anything that changes while the game runs: a
		// variable of an object's, or one of liveNumbers.
		bool namesLive(const WrittenValue& value) const;

		// Every game loaded after this draws its <random>s starting from
		// `value`, the same way each time (see game/random.h, which a generated
		// game draws from too); nullopt goes back to a new start each load.
		static void fixRandomSeed(std::optional<unsigned int> value) noexcept { xge::fixRandomSeed(value); }
	};
}
