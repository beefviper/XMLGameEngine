// game_expr.h
// XML Game Engine
// author: beefviper
// date: Sept 18, 2020

#pragma once

#include "command.h"
#include "object.h"
#include "states.h"

#include <exprtk.hpp>

#include <iostream>
#include <random>
#include <cassert>

namespace xge
{
	class game_expr
	{
	public:
		using igenfunct_t = exprtk::igeneric_function<float>;
		using generic_t = typename igenfunct_t::generic_type;
		using parameter_list_t = typename igenfunct_t::parameter_list_t;
		using string_t = typename generic_t::string_view;
		using scalar_t = typename generic_t::scalar_view;

		void init(const WindowDesc& windowDesc, std::map<std::string, float>& variables,
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

		float evaluateString(const RawObject& rawObject, const std::string& input_string);
		std::vector<std::string> processData(const RawObject& rawObject, const std::string& input_string);
		std::vector<Command> processCommands(const RawObject& rawObject, const std::string& input_string);
		xge::GridData setGridXY(std::vector<std::string>& tempSpriteParams);

		float evaluateString(const RawState& rawState, const std::string& input_string);
		std::vector<std::string> processData(const RawState& rawState, const std::string& input_string);
		std::vector<Command> processCommands(const RawState& rawState, const std::string& input_string);

		static inline std::vector<std::string> tempSParams;

		// Every object-declared <variable>, keyed "ownerName.variableName", bound
		// by reference into symbolTable (see init()) so any expression - not just
		// the owning object's own commands - can read another object's variable
		// (e.g. a HUD's sprite doing text(paddle1.score, ...)). A std::map's
		// iterators/references to existing elements stay valid across further
		// insertions, which is what makes binding into it safe here: init()
		// pre-creates every key (so the symbol exists before anything compiles)
		// and only ever assigns through an existing key afterwards.
		std::map<std::string, float> objectVariables;

		static inline std::random_device seed;
		static inline std::mt19937 generator;

		template <typename T>
		static T randomNumberRange(T min, T max)
		{
			std::uniform_real_distribution<T> distribution(min, max);
			T randNum = distribution(generator);
			return randNum;
		}

		template <typename T>
		struct randomNumber final : public exprtk::ifunction<T>
		{
			randomNumber() noexcept(std::is_nothrow_constructible_v<exprtk::ifunction<T>, int>)
				: exprtk::ifunction<T>(1) {}

			T operator()(const T& randMax) override
			{
				return randomNumberRange(0.0f, randMax);
			}
		};

		template <typename T>
		struct randomRange final : public exprtk::ifunction<T>
		{
			randomRange() noexcept(std::is_nothrow_constructible_v<exprtk::ifunction<T>, int>)
				: exprtk::ifunction<T>(2) {}

			T operator()(const T& randMin, const T& randMax) override
			{
				return randomNumberRange(randMin, randMax);
			}
		};

		template <typename T>
		struct shapeCircle : public exprtk::igeneric_function<T>
		{
			shapeCircle() noexcept : exprtk::igeneric_function<T>("T|TS") {}

			T operator()([[maybe_unused]] const std::size_t& ps_index, parameter_list_t parameters) override
			{
				tempSParams.push_back("circle");
				float const radius = scalar_t(parameters[0])();
				tempSParams.push_back(std::to_string(radius));
				tempSParams.push_back(std::to_string(0));

				if (parameters.size() == 2)
				{
					tempSParams.push_back(exprtk::to_str(string_t(parameters[1])));
				}
				else
				{
					tempSParams.push_back("color.white");
				}

				return 0;
			}
		};

		template <typename T>
		struct shapeRectangle : public exprtk::igeneric_function<T>
		{
			shapeRectangle() noexcept : exprtk::igeneric_function<T>("TT|TTS") {}

			T operator()([[maybe_unused]] const std::size_t& ps_index, parameter_list_t parameters) override
			{
				tempSParams.push_back("rectangle");
				const float width = scalar_t(parameters[0])();
				const float height = scalar_t(parameters[1])();
				tempSParams.push_back(std::to_string(width));
				tempSParams.push_back(std::to_string(height));

				if (parameters.size() == 3)
				{
					tempSParams.push_back(exprtk::to_str(string_t(parameters[2])));
				}
				else
				{
					tempSParams.push_back("color.white");
				}

				return 0;
			}
		};

		template <typename T>
		struct text : public exprtk::igeneric_function<T>
		{
			// "ST"/"STS": a literal string label, e.g. text('PAUSED', 128, ...).
			// "TT"/"TTS": a live numeric value, e.g. text(paddle1.score, 128, ...)
			// - displays that object's own <variable> and (via
			// Object::boundVariableOwner/boundVariableName, set from the raw src
			// text - see parseTextVariableBinding) stays in sync with it whenever
			// the variable changes through inc(). ps_index (not parameters.size(),
			// which "ST" and "TT" share) is what tells these two apart.
			text() noexcept : exprtk::igeneric_function<T>("ST|STS|TT|TTS") {}

			inline T operator()(const std::size_t& ps_index, parameter_list_t parameters) override
			{
				tempSParams.push_back("text");

				if (ps_index == 0 || ps_index == 1) // "ST" / "STS"
				{
					tempSParams.push_back(exprtk::to_str(string_t(parameters[0])));
				}
				else // "TT" / "TTS"
				{
					tempSParams.push_back(formatDisplayNumber(scalar_t(parameters[0])()));
				}

				const float size = scalar_t(parameters[1])();
				tempSParams.push_back(std::to_string(size));

				if (ps_index == 1 || ps_index == 3) // "STS" / "TTS"
				{
					tempSParams.push_back(exprtk::to_str(string_t(parameters[2])));
				}
				else
				{
					tempSParams.push_back("color.white");
				}

				return 0;
			}
		};

		template <typename T>
		struct image : public exprtk::igeneric_function<T>
		{
			image() noexcept : exprtk::igeneric_function<T>("S|SS") {}

			inline T operator()([[maybe_unused]] const std::size_t& ps_index, parameter_list_t parameters) override
			{
				tempSParams.push_back("image");
				tempSParams.push_back(exprtk::to_str(string_t(parameters[0])));

				if (parameters.size() == 2)
				{
					tempSParams.push_back(exprtk::to_str(string_t(parameters[1])));
				}
				else
				{
					tempSParams.push_back("color.white");
				}

				return 0;
			}
		};

		template <typename T>
		struct inc : public exprtk::igeneric_function<T>
		{
			inc() noexcept : exprtk::igeneric_function<T>("S") {}

			inline T operator()(parameter_list_t parameters) override
			{
				tempSParams.push_back("inc");
				tempSParams.push_back(exprtk::to_str(string_t(parameters[0])));

				return 0;
			}
		};

		template <typename T>
		struct grid : public exprtk::igeneric_function<T>
		{
			grid() noexcept : exprtk::igeneric_function<T>("TTT|TTTTT") {}

			inline T operator()([[maybe_unused]] const std::size_t& ps_index, parameter_list_t parameters) override
			{
				tempSParams.push_back("grid");
				const float width = scalar_t(parameters[0])();
				const float height = scalar_t(parameters[1])();
				tempSParams.push_back(std::to_string(width));
				tempSParams.push_back(std::to_string(height));

				if (parameters.size() == 5)
				{
					const float width_padding = scalar_t(parameters[2])();
					const float height_padding = scalar_t(parameters[3])();

					tempSParams.push_back(std::to_string(width_padding));
					tempSParams.push_back(std::to_string(height_padding));
				}
				else
				{
					tempSParams.push_back(std::to_string(0));
					tempSParams.push_back(std::to_string(0));
				}

				return 0;
			}
		};

		template <typename T>
		struct bounce : public exprtk::igeneric_function<T>
		{
			bounce() noexcept : exprtk::igeneric_function<T>("Z") {}

			T operator()([[maybe_unused]] parameter_list_t parameters) override
			{
				tempSParams.push_back("collide");
				tempSParams.push_back("bounce");
				return 0;
			}
		};

		template <typename T>
		struct stick : public exprtk::igeneric_function<T>
		{
			stick() noexcept : exprtk::igeneric_function<T>("Z") {}

			T operator()([[maybe_unused]] parameter_list_t parameters) override
			{
				tempSParams.push_back("collide");
				tempSParams.push_back("stick");
				return 0;
			}
		};

		template <typename T>
		struct reset : public exprtk::igeneric_function<T>
		{
			// "Z": reset() with no args. Meaning depends on where it's used, same
			// as CmdReset already does between executeScreenEdgeCollision (resets
			// position) and executeObjectCollision (ignored): inside a collision
			// action it resets just the colliding object's own position to
			// positionOriginal (unchanged original meaning); inside a state's
			// <input>/<condition> action - where there's no "colliding object" to
			// be implicit about - it instead means a full game reset (see
			// Game::resetAll): every object's position/velocity/variables back to
			// how they loaded, and the state stack collapsed back down to the
			// first state, so mainmenu -> playing -> gameover -> mainmenu -> ...
			// doesn't grow the stack forever.
			// "S": reset('objectName') - resets just that one named object's
			// position, velocity, AND every <variable> back to its starting
			// values (see Game::resetObject). ps_index (not parameters.size(),
			// which "Z" doesn't have anyway) tells the two signatures apart,
			// same technique as text<T>'s "ST|STS|TT|TTS".
			reset() noexcept : exprtk::igeneric_function<T>("Z|S") {}

			T operator()(const std::size_t& ps_index, parameter_list_t parameters) override
			{
				if (ps_index == 0) // "Z"
				{
					tempSParams.push_back("collide");
					tempSParams.push_back("reset");
				}
				else // "S"
				{
					tempSParams.push_back("resetobject");
					tempSParams.push_back(exprtk::to_str(string_t(parameters[0])));
				}
				return 0;
			}
		};

		template <typename T>
		struct die : public exprtk::igeneric_function<T>
		{
			die() noexcept : exprtk::igeneric_function<T>("Z") {}

			T operator()([[maybe_unused]] parameter_list_t parameters) override
			{
				tempSParams.push_back("collide");
				tempSParams.push_back("die");
				return 0;
			}
		};

		template <typename T>
		struct moveUp : public exprtk::igeneric_function<T>
		{
			moveUp() noexcept : exprtk::igeneric_function<T>("T") {}

			T operator()(parameter_list_t parameters) override
			{
				tempSParams.push_back("moveup");
				const float step = scalar_t(parameters[0])();
				tempSParams.push_back(std::to_string(step));
				return 0;
			}
		};

		template <typename T>
		struct moveDown : public exprtk::igeneric_function<T>
		{
			moveDown() noexcept : exprtk::igeneric_function<T>("T") {}

			T operator()(parameter_list_t parameters) override
			{
				tempSParams.push_back("movedown");
				const float step = scalar_t(parameters[0])();
				tempSParams.push_back(std::to_string(step));
				return 0;
			}
		};

		template <typename T>
		struct moveLeft : public exprtk::igeneric_function<T>
		{
			moveLeft() noexcept : exprtk::igeneric_function<T>("T") {}

			T operator()(parameter_list_t parameters) override
			{
				tempSParams.push_back("moveleft");
				const float step = scalar_t(parameters[0])();
				tempSParams.push_back(std::to_string(step));
				return 0;
			}
		};

		template <typename T>
		struct moveRight : public exprtk::igeneric_function<T>
		{
			moveRight() noexcept : exprtk::igeneric_function<T>("T") {}

			T operator()(parameter_list_t parameters) override
			{
				tempSParams.push_back("moveright");
				const float step = scalar_t(parameters[0])();
				tempSParams.push_back(std::to_string(step));
				return 0;
			}
		};

		template <typename T>
		struct state : public exprtk::igeneric_function<T>
		{
			state() noexcept : exprtk::igeneric_function<T>("S|Z") {}

			T operator()([[maybe_unused]] const std::size_t& ps_index, parameter_list_t parameters) override
			{
				if (parameters.size() > 0)
				{
					tempSParams.push_back("state");
					tempSParams.push_back(exprtk::to_str(string_t(parameters[0])));
				}
				else
				{
					tempSParams.push_back("state");
					tempSParams.push_back("pop");
				}
				return 0;
			}
		};

		template <typename T>
		struct action : public exprtk::igeneric_function<T>
		{
			action() noexcept : exprtk::igeneric_function<T>("SS") {}

			T operator()(parameter_list_t parameters) override
			{
				tempSParams.push_back("action");
				tempSParams.push_back(exprtk::to_str(string_t(parameters[0])));
				tempSParams.push_back(exprtk::to_str(string_t(parameters[1])));
				return 0;
			}
		};

		template <typename T>
		struct fire : public exprtk::igeneric_function<T>
		{
			fire() noexcept : exprtk::igeneric_function<T>("S") {}

			T operator()(parameter_list_t parameters) override
			{
				tempSParams.push_back("fire");
				tempSParams.push_back(exprtk::to_str(string_t(parameters[0])));
				return 0;
			}
		};
	};
}
