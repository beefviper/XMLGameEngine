// game_expr.h
// XML Game Engine
// author: beefviper
// date: Sept 18, 2020

#pragma once

#include "object.h"
#include "states.h"

#include <exprtk.hpp>

#include <iostream>
#include <random>

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

		void init(WindowDesc& windowDesc, std::map<std::string, float>& variables,
			std::vector<RawState>& rawStates, std::vector<State>& states,
			std::vector<RawObject>& rawObjects, std::vector<Object>& objects);

		exprtk::symbol_table<float> symbolTable;
		exprtk::expression<float> expression;
		exprtk::parser<float> parser;

		float evaluateString(const RawObject& rawObject, const std::string& input_string);
		std::vector<std::string> processData(const RawObject& rawObject, const std::string& input_string);
		xge::GridData setGridXY(std::vector<std::string>& tempSpriteParams);

		float evaluateString(const RawState& rawState, const std::string& input_string);
		std::vector<std::string> processData(const RawState& rawState, const std::string& input_string);

		static inline std::vector<std::string> tempSParams;

		static inline std::random_device seed;
		static inline std::mt19937 generator{ seed() };

		template <typename T>
		static T randomNumberRange(T min, T max)
		{
			std::uniform_real_distribution<T> distribution(min, max);
			T randNum = distribution(generator);
			return randNum;
		}

		template <typename T>
		struct randomNumber : public exprtk::ifunction<T>
		{
			randomNumber() : exprtk::ifunction<T>(1) {}

			T operator()(const T& randMax)
			{
				return randomNumberRange(0.0f, randMax);
			}
		};

		template <typename T>
		struct randomRange : public exprtk::ifunction<T>
		{
			randomRange() : exprtk::ifunction<T>(2) {}

			T operator()(const T& randMin, const T& randMax)
			{
				return randomNumberRange(randMin, randMax);
			}
		};

		template <typename T>
		struct shapeCircle : public exprtk::igeneric_function<T>
		{
			shapeCircle() noexcept : exprtk::igeneric_function<T>("T|TS") {}

			T operator()(const std::size_t& ps_index, parameter_list_t parameters) override
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

			T operator()(const std::size_t& ps_index, parameter_list_t parameters) override
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
			text() noexcept : exprtk::igeneric_function<T>("ST|STS") {}

			inline T operator()(const std::size_t& ps_index, parameter_list_t parameters) override
			{
				tempSParams.push_back("text");
				tempSParams.push_back(exprtk::to_str(string_t(parameters[0])));
				const float size = scalar_t(parameters[1])();
				tempSParams.push_back(std::to_string(size));

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
		struct image : public exprtk::igeneric_function<T>
		{
			image() noexcept : exprtk::igeneric_function<T>("S|SS") {}

			inline T operator()(const std::size_t& ps_index, parameter_list_t parameters) override
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
			inc() noexcept : exprtk::igeneric_function<T>("S|S") {}

			inline T operator()(const std::size_t& ps_index, parameter_list_t parameters) override
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

			inline T operator()(const std::size_t& ps_index, parameter_list_t parameters) override
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
			bounce() noexcept : exprtk::igeneric_function<T>("Z|Z") {}

			T operator()(const std::size_t& ps_index, parameter_list_t parameters) override
			{
				tempSParams.push_back("collide");
				tempSParams.push_back("bounce");
				return 0;
			}
		};

		template <typename T>
		struct stick : public exprtk::igeneric_function<T>
		{
			stick() noexcept : exprtk::igeneric_function<T>("Z|Z") {}

			T operator()(const std::size_t& ps_index, parameter_list_t parameters) override
			{
				tempSParams.push_back("collide");
				tempSParams.push_back("stick");
				return 0;
			}
		};

		template <typename T>
		struct reset : public exprtk::igeneric_function<T>
		{
			reset() noexcept : exprtk::igeneric_function<T>("Z|Z") {}

			T operator()(const std::size_t& ps_index, parameter_list_t parameters) override
			{
				tempSParams.push_back("collide");
				tempSParams.push_back("reset");
				return 0;
			}
		};

		template <typename T>
		struct die : public exprtk::igeneric_function<T>
		{
			die() noexcept : exprtk::igeneric_function<T>("Z|Z") {}

			T operator()(const std::size_t& ps_index, parameter_list_t parameters) override
			{
				tempSParams.push_back("collide");
				tempSParams.push_back("die");
				return 0;
			}
		};

		template <typename T>
		struct moveUp : public exprtk::igeneric_function<T>
		{
			moveUp() noexcept : exprtk::igeneric_function<T>("T|T") {}

			T operator()(const std::size_t& ps_index, parameter_list_t parameters) override
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
			moveDown() noexcept : exprtk::igeneric_function<T>("T|T") {}

			T operator()(const std::size_t& ps_index, parameter_list_t parameters) override
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
			moveLeft() noexcept : exprtk::igeneric_function<T>("T|T") {}

			T operator()(const std::size_t& ps_index, parameter_list_t parameters) override
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
			moveRight() noexcept : exprtk::igeneric_function<T>("T|T") {}

			T operator()(const std::size_t& ps_index, parameter_list_t parameters) override
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
			state() noexcept : exprtk::igeneric_function<T>("S|S|Z") {}

			T operator()(const std::size_t& ps_index, parameter_list_t parameters) override
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
			action() noexcept : exprtk::igeneric_function<T>("SS|SS") {}

			T operator()(const std::size_t& ps_index, parameter_list_t parameters) override
			{
				tempSParams.push_back(exprtk::to_str(string_t(parameters[0])));
				tempSParams.push_back(exprtk::to_str(string_t(parameters[1])));
				return 0;
			}
		};

		template <typename T>
		struct fire : public exprtk::igeneric_function<T>
		{
			fire() noexcept : exprtk::igeneric_function<T>("S|S") {}

			T operator()(const std::size_t& ps_index, parameter_list_t parameters) override
			{
				tempSParams.push_back("fire");
				tempSParams.push_back(exprtk::to_str(string_t(parameters[0])));
				return 0;
			}
		};
	};
}
