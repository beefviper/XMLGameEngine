// game_expr.h
// XML Game Engine
// author: beefviper
// date: Sept 18, 2020

#pragma once

#include "bitmap.h"
#include "color.h"
#include "command.h"
#include "object.h"
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
		// can use the ones declared before it), then builds every object and
		// state.
		void init(const WindowDesc& windowDesc,
			const std::vector<std::pair<std::string, RawValue>>& rawVariables, std::map<std::string, float>& variables,
			std::vector<RawState>& rawStates, std::vector<State>& states,
			std::vector<RawObject>& rawObjects, std::vector<Object>& objects);

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

		exprtk::symbol_table<float> symbolTable;
		exprtk::expression<float> expression;
		exprtk::parser<float> parser;

		// A value's number: its expression worked out, or the number its tag
		// makes. `where` says whose value it is, for the message if the
		// expression does not compile (std::runtime_error).
		float evaluate(const RawValue& value, const std::string& where);
		float evaluateExpression(const std::string& text, const std::string& where);

		// The spriteParams for a sprite, in the shape the window backends read:
		// {"circle", radius, "0", color}, {"rectangle", width, height, color},
		// {"text", label, size, color}, {"image", path, flip-or-color}, and for
		// a grid, {"grid", columns, rows, xPadding, yPadding} after those. A
		// drawing of lines is {"line", width, height} - the size of the picture
		// - and the picture itself is handed back through `bitmap` when that is
		// given (see Object::bitmap).
		std::vector<std::string> buildSpriteParams(const RawSprite& sprite, const std::string& where,
			std::shared_ptr<const Bitmap>* bitmap = nullptr);
		xge::GridData gridDataOf(const RawSprite& sprite, const std::string& where);

		std::vector<Command> processCommands(const std::vector<RawCommand>& raw, const std::string& where);

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
