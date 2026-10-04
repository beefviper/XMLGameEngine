// game_expr.h
// XML Game Engine
// author: beefviper
// date: Sept 18, 2020

#pragma once

#include "bitmap.h"
#include "color.h"
#include "command.h"
#include "object.h"
#include "sound.h"
#include "states.h"

// exprtk is a third-party header. MSVC reports C4702 (unreachable code) from
// inside it during code generation, which /external:W0 does not cover, so the
// warning is turned off just around the include. Other compilers do not
// understand this pragma, hence the _MSC_VER test.
#if defined(_MSC_VER)
#pragma warning(push)
#pragma warning(disable : 4702)
#endif

#include <exprtk.hpp>

#if defined(_MSC_VER)
#pragma warning(pop)
#endif

#include <iostream>
#include <map>
#include <memory>
#include <random>
#include <set>
#include <string>
#include <utility>
#include <vector>

namespace xge
{
	// Works out what game_xml read: every RawValue to a number, every RawSprite
	// to the spriteParams the window backends draw from, every RawCommand to a
	// typed Command. The arithmetic in a value's text is exprtk's job; the tags
	// (<random>) and everything that used to be a function call in the game
	// file (shapes, commands) are handled here and in command.cpp, not by
	// exprtk.
	class game_expr
	{
	public:
		// Fills `variables` from rawVariables (in order, so a variable's value
		// can use the ones declared before it), then builds every object, state
		// and sound. A wave, pitch or length a sound cannot use, and a <play>
		// naming no sound, throw std::runtime_error saying where.
		// Called when the game has been loaded. From then on a value worked out
		// again while the game runs (a timer's interval, a position finished
		// once a text is measured) never throws for a division by 0: see
		// evaluateOperation.
		void finishLoading() { loading = false; }

		void init(const WindowDesc& windowDesc,
			const std::vector<std::pair<std::string, RawValue>>& rawVariables, std::map<std::string, float>& variables,
			std::vector<RawState>& rawStates, std::vector<State>& states,
			std::vector<RawObject>& rawObjects, std::vector<Object>& objects,
			const std::vector<RawSound>& rawSounds, std::vector<SoundDesc>& sounds);

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
		std::set<std::string> warnedDivisions;

		exprtk::symbol_table<float> symbolTable;
		exprtk::expression<float> expression;
		exprtk::parser<float> parser;

		// A value's number: its expression worked out, or the number its tag
		// makes. `where` says whose value it is, for the message if the
		// expression does not compile (std::runtime_error).
		float evaluate(const RawValue& value, const std::string& where);

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

		// One <add>, <subtract>, <multiply> or <divide> of an <equation> or a
		// <formula>: its first operand combined with each of the others in turn.
		// `steps` holds the answers of the equation's named steps so far (a
		// formula has none); an operand that names one is that answer.
		//
		// A divisor of 0 is a load error (std::runtime_error saying which
		// <divide>) when it is certainly 0 - a number, a global variable, or
		// something made only of those - since nothing sensible can be placed or
		// timed with an infinity. A divisor that is only 0 because it is `late`
		// gives 0 and nothing is said: it is worked out again when it is known.
		// Worked out again while the game runs, where throwing would end it, a
		// divisor of 0 gives the answer 0 and a warning, printed once for each
		// place it happens.
		Answer evaluateOperation(const RawOperation& operation, const std::map<std::string, Answer>& steps, const std::string& where);

		// Whether an operand's name is something that is `late` while loading.
		bool isLate(const std::string& name) const;
		float evaluateExpression(const std::string& text, const std::string& where);

		// The spriteParams for a sprite, in the shape the window backends read:
		// {"circle", radius, "0", color}, {"rectangle", width, height, color},
		// {"text", label, size, color}, {"image", path, flip-or-color}, and for
		// a grid, {"grid", columns, rows, xPadding, yPadding} after those. A
		// drawing of lines is {"line", width, height} - the size of the picture
		// - and the picture itself is handed back through `bitmap` when that is
		// given (see Object::bitmap). A bitmap (a picture in rows of text) is
		// drawn the same way and has the same params, with a grid's after them
		// when it is repeated.
		//
		// `turnable`, when given, is for an object that has a <heading>: what a
		// drawing of lines or a <bitmap> is to be turned from is handed back
		// through it (see Turnable), `bitmap` is the drawing at heading 0, and
		// the size in the params is the square it turns in.
		std::vector<std::string> buildSpriteParams(const RawSprite& sprite, const std::string& where,
			std::shared_ptr<const Bitmap>* bitmap = nullptr,
			std::shared_ptr<const Turnable>* turnable = nullptr);
		xge::GridData gridDataOf(const RawSprite& sprite, const std::string& where);

		// The pictures of an object's <animation>, each drawn once, in the order
		// of its <frame>s. Throws std::runtime_error (saying whose) when a frame
		// is not a <bitmap> or a drawing of <line>s, or when the frames are not
		// all the same size and the same grid. `turned`, when given, is for an
		// object with a <heading>: it gets what each frame turns from,
		// and every frame must turn in the same size of square.
		std::vector<std::shared_ptr<const Bitmap>> buildAnimationBitmaps(const RawObject& rawObject, const std::string& where,
			std::vector<std::shared_ptr<const Turnable>>* turnables = nullptr);

		// How many frames of the game each picture of the animation is shown
		// for: its <interval> in seconds times the window's <framerate>, at
		// least 1. Throws std::runtime_error for an interval that is not above
		// 0, or for a window with no framerate to count seconds in.
		int animationFramesOf(const RawObject& rawObject, const WindowDesc& windowDesc, const std::string& where);

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

		static inline std::random_device seed;
		static inline std::mt19937 generator;

		// A random number from min to max (either order), for <random>.
		template <typename T>
		static T randomNumberRange(T min, T max)
		{
			if (min > max) { std::swap(min, max); }
			std::uniform_real_distribution<T> distribution(min, max);
			T randNum = distribution(generator);
			return randNum;
		}
	};
}
