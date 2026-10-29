// expr.h
// XML Game Engine
// author: beefviper
// date: Sept 18, 2020

#pragma once

#include "picture/bitmap.h"
#include "picture/color.h"
#include "game/command.h"
#include "load/calc.h"
#include "game/object.h"
#include "game/random.h"
#include "audio/sound.h"
#include "game/states.h"

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
	// Works out what load/xml read: every RawValue to a number, every RawSprite
	// to the Picture the window backends draw, every RawCommand to a
	// typed Command. The arithmetic in a value's text is load/calc's job; the
	// tags (<random>, <formula>) are handled here and in game/command.cpp.
	class game_expr
	{
	public:
		game_expr() = default;
		game_expr(game_expr&&) noexcept = default;
		game_expr& operator=(game_expr&&) noexcept = default;
		game_expr(const game_expr&) = delete;
		game_expr& operator=(const game_expr&) = delete;

		// Fills `variables` from rawVariables (in order, so a variable's value
		// can use the ones declared before it), then builds every object, state
		// and sound. A wave, pitch or length a sound cannot use, and a <play>
		// naming no sound, throw std::runtime_error saying where.
		// Called when the game has been loaded. From then on a value worked out
		// again while the game runs (a timer's interval, a position finished
		// once a text is measured) never throws for a division by 0: see
		// evaluateOperation.
		void finishLoading() { loading = false; }

		// What an object's sprite was drawn as before it was built (init's
		// first pass, or the object a copy is spawned from): its sprite's
		// Picture, the bitmap, what turns, an animation's pictures, and,
		// when given, its looks ready made.
		struct FirstPass
		{
			Picture picture;
			std::shared_ptr<const Bitmap> bitmap;
			std::shared_ptr<const Turnable> turned;
			std::vector<std::shared_ptr<const Bitmap>> animation;
			std::vector<std::shared_ptr<const Turnable>> animationTurned;
			const std::vector<Object::Look>* looks{ nullptr };
		};

		// One object, made from what the file says of it (after init's
		// first pass, or for a copy spawned while the game runs).
		Object makeObject(RawObject& rawObject, const FirstPass& first, const WindowDesc& windowDesc, Vector2f cellOffset, int lockstep);

		// A copy of `from` spawned while the game runs, from `raw` (what
		// the file says of `from`, under the copy's name): its names
		// made known to the expressions (its variables, where it is, its
		// size) and the object made, its pictures shared with `from`'s.
		Object spawnObject(RawObject& raw, const Object& from, const WindowDesc& windowDesc);

		void init(const WindowDesc& windowDesc,
			const std::vector<std::pair<std::string, RawValue>>& rawVariables, std::map<std::string, float>& variables,
			std::vector<RawState>& rawStates, std::vector<State>& states,
			std::vector<RawObject>& rawObjects, std::vector<Object>& objects,
			const std::vector<RawSound>& rawSounds, std::vector<SoundDesc>& sounds,
			const std::vector<RawPath>& rawPaths, std::map<std::string, Path>& paths);

		// Every object's size, keyed by object name and bound into symbolTable as
		// "name.width" / "name.height" (unless the object declares a <variable>
		// of that name, which wins). A circle's or rectangle's is exact from
		// init(); a text's or image's stays {0,0} until setObjectSize() is
		// given what a backend measured. See Object::positionUsesSize. A
		// std::map's references stay valid across insertions, which is what
		// makes binding into it safe.
		std::map<std::string, Vector2f> objectSizes;
		std::map<std::string, ShapeKind> objectShapeKinds;
		void setObjectSize(const std::string& name, const Vector2f& size);

		// The objects, other than shapes, whose size rawObject's <position> uses
		// (by "name.width" / "name.height"): the ones only a backend can measure.
		std::vector<std::string> sizeDependenciesOf(const RawObject& rawObject) const;

		// Whether an object of this kind has a size only a backend can measure.
		static bool sizeNeedsBackend(ShapeKind kind) noexcept
		{
			return kind == ShapeKind::Text || kind == ShapeKind::Image;
		}

		bool loading{ true };

		// While a command's values are worked out at load: they may name
		// liveNumbers (reading 0), since they are worked out again when the
		// command runs.
		bool commandValues{ false };
		std::set<std::string> warnedDivisions;

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
		float evaluate(const RawValue& value, const std::string& where);

		// A path step's commands about its flier (PathStep::ownRaw), made for
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

		// One <add>, <subtract>, <multiply> or <divide> of a <formula>: its
		// first operand combined with each of the others in turn.
		//
		// A divisor of 0 is a load error (std::runtime_error saying which
		// <divide>) when it is certainly 0 - a number, a global variable, or
		// something made only of those - since nothing sensible can be placed or
		// timed with an infinity. A divisor that is only 0 because it is `late`
		// gives 0 and nothing is said: it is worked out again when it is known.
		// Worked out again while the game runs, where throwing would end it, a
		// divisor of 0 gives the answer 0 and a warning, printed once for each
		// place it happens.
		Answer evaluateOperation(const RawOperation& operation, const std::string& where);

		// Whether an operand's name is something that is `late` while loading.
		bool isLate(const std::string& name) const;
		float evaluateExpression(const std::string& text, const std::string& where);

		// The Picture for a sprite, as the window backends draw it. For a
		// drawing of lines, a <bitmap> (rows of text) or an <svg>, a Line the
		// size of the drawing, and the drawing itself is handed back through
		// `bitmap` when that is given (see Object::bitmap).
		//
		// `turnable`, when given, is for an object that has a <heading>: what a
		// drawing of lines or a <bitmap> is to be turned from is handed back
		// through it (see Turnable), `bitmap` is the drawing at heading 0, and
		// the picture's size is the square it turns in.
		//
		// A text's pieces that name a variable of `self`'s group (scores.score
		// in a member of the group scores) mean `self`'s own.
		Picture buildPicture(const RawSprite& sprite, const std::string& where,
			std::shared_ptr<const Bitmap>* bitmap = nullptr,
			std::shared_ptr<const Turnable>* turnable = nullptr,
			const RawObject* self = nullptr);

		// The pieces of a text sprite as they show now: a <number> or a
		// <letter> that is just owner.variable follows that variable (of
		// `self`, for its group's name), the rest worked out once.
		std::vector<TextPiece> textPieces(const RawSprite& sprite, const std::string& where, const RawObject* self);

		// Where each cell of a group laid out in <columns> and <rows> goes,
		// from the group's position, by its name (see RawCell). `pictures`
		// is every object's sprite as it is drawn, for the size of a cell whose
		// sprite is not its row's.
		std::map<std::string, Vector2f> layOutCells(const std::vector<RawObject>& rawObjects,
			const std::map<std::string, Picture>& pictures);

		// A picture drawn from rows of text or an svg, by everything it is
		// drawn from (`key`): drawn by `draw` the first time it is asked for,
		// and shared after that, so the 55 cells of a block of aliens are one
		// picture, drawn once.
		template <typename Draw>
		std::shared_ptr<const Bitmap> drawOnce(const std::string& key, Draw&& draw)
		{
			std::shared_ptr<const Bitmap>& picture = drawnPictures[key];
			if (!picture) { picture = std::make_shared<const Bitmap>(draw()); }
			return picture;
		}
		std::map<std::string, std::shared_ptr<const Bitmap>> drawnPictures;

		// Each picture an object with a <mirror> shows, and it flipped left
		// to right, both ways round: made once, whoever shows it.
		std::map<const Bitmap*, std::shared_ptr<const Bitmap>> flippedPictures;

		// The game's <styles>, for the palettes a <become style> asks for.
		std::vector<RawStyle> styles;

		// Each object's palettes (Object::palettes), for the styles `wanted`
		// names for it by its name, drawn from what the file says of it.
		// Throws std::runtime_error for one with a <heading>, or with no
		// bitmap that takes a style.
		void makePalettes(const std::vector<RawObject>& rawObjects, std::vector<Object>& objects, const std::map<std::string, std::vector<std::string>>& wanted);

		// The flips (Object::flips) of every picture `object` can show.
		std::shared_ptr<const std::map<const Bitmap*, std::shared_ptr<const Bitmap>>> flipsOf(const Object& object);

		// The pictures of an object's <animation>, each drawn once, in the order
		// of its <frame>s. Throws std::runtime_error (saying whose) when a frame
		// is not a <bitmap> or a drawing of <line>s, or when the frames are not
		// all the same size. `turned`, when given, is for an
		// object with a <heading>: it gets what each frame turns from,
		// and every frame must turn in the same size of square.
		std::vector<std::shared_ptr<const Bitmap>> buildAnimationBitmaps(const std::vector<RawSprite>& frames, const std::string& where,
			std::vector<std::shared_ptr<const Turnable>>* turnables = nullptr);

		// How many frames of the game each picture of the animation is shown
		// for: its <interval> in seconds times the window's <framerate>, at
		// least 1. Throws std::runtime_error for an interval that is not above
		// 0, or for a window with no framerate to count seconds in.
		int animationFramesOf(const RawValue& interval, const WindowDesc& windowDesc, const std::string& where);

		std::vector<Command> processCommands(const std::vector<RawCommand>& raw, const std::string& where);

		// An object's or a state's <timers>: the commands made, the interval
		// checked and kept as written (see Timer).
		std::vector<Timer> processTimers(const std::vector<RawTimer>& raw, const std::string& where);

		// A <sound> worked out: its words checked and its lengths evaluated.
		SoundDesc processSound(const RawSound& raw);

		// Every object-declared <variable>, keyed "ownerName.variableName", bound
		// by reference into symbolTable (see init()) so any expression - not just
		// the owning object's own commands - can read another object's variable
		// (e.g. a HUD's sprite showing paddle1.score). A std::map's
		// iterators/references to existing elements stay valid across further
		// insertions, which is what makes binding into it safe here: init()
		// pre-creates every key (so the symbol exists before anything compiles)
		// and only ever assigns through an existing key afterwards.
		std::map<std::string, float> objectVariables;

		// The numbers every object has while the game runs (liveNumberParts:
		// ball.x, ball.velocity.y, ...) and what a rule answering a touch can
		// read of it (touchNumberNames), bound into symbolTable as the
		// variables are. Before a value is worked out while the game runs,
		// Game writes them from where everything is (Game::evaluateNow).
		// While loading they have no meaning yet, and a value that names one
		// stops the load, unless it is a command's (see processCommands).
		std::map<std::string, float> liveNumbers;

		// The texts already found to name none of liveNumbers while loading,
		// so a text many objects share (a map's pieces) is looked through once.
		std::set<std::string> loadCheckedTexts;

		// Whether a value names anything that changes while the game runs: a
		// variable of an object's, or one of liveNumbers.
		bool namesLive(const RawValue& value) const;

		// Every game loaded after this draws its <random>s starting from
		// `value`, the same way each time (see game/random.h, which a generated
		// game draws from too); nullopt goes back to a new start each load.
		static void fixRandomSeed(std::optional<unsigned int> value) noexcept { xge::fixRandomSeed(value); }
	};
}
