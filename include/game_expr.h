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
		void init(WindowDesc& windowDesc, std::map<std::string, float>& variables, std::vector<State>& states,
			std::vector<RawObject>& rawObjects, std::vector<Object>& objects);

		exprtk::symbol_table<float> symbolTable;
		exprtk::expression<float> expression;
		exprtk::parser<float> parser;

		float evaluateString(const RawObject& rawObject, const std::string& input_string);
		std::vector<std::string> processData(const RawObject& rawObject, const std::string& input_string);
		xge::GridData setGridData(std::vector<std::string>& tempSpriteParams);
	};

	namespace
	{
		std::vector<std::string> tempSParams;

		std::random_device seed;
		std::mt19937 generator(seed());
	}

	template <typename T>
	T randomNumberRange(T min, T max)
	{
		std::uniform_real_distribution<T> distribution(min, max);
		T randNum = distribution(generator);
		return randNum;
	}

	template <typename T>
	struct randomNumber : public exprtk::ifunction<T>
	{
		randomNumber() : exprtk::ifunction<T>(1)
		{

		}

		T operator()(const T& randMax)
		{
			return randomNumberRange(0.0f, randMax);
		}
	};

	template <typename T>
	struct randomRange : public exprtk::ifunction<T>
	{
		randomRange() : exprtk::ifunction<T>(2)
		{

		}

		T operator()(const T& randMin, const T& randMax)
		{
			return randomNumberRange(randMin, randMax);
		}
	};

	template <typename T>
	struct shapeCircle : public exprtk::igeneric_function<T>
	{
		using igenfunct_t = exprtk::igeneric_function<T>;
		using generic_t = typename igenfunct_t::generic_type;
		using parameter_list_t = typename igenfunct_t::parameter_list_t;
		using string_t = typename generic_t::string_view;
		using scalar_t = typename generic_t::scalar_view;

		shapeCircle() noexcept : exprtk::igeneric_function<T>("T|TS")
		{

		}

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
		using igenfunct_t = exprtk::igeneric_function<T>;
		using generic_t = typename igenfunct_t::generic_type;
		using parameter_list_t = typename igenfunct_t::parameter_list_t;
		using string_t = typename generic_t::string_view;
		using scalar_t = typename generic_t::scalar_view;

		shapeRectangle() noexcept : exprtk::igeneric_function<T>("TT|TTS")
		{

		}

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
		using igenfunct_t = exprtk::igeneric_function<T>;
		using generic_t = typename igenfunct_t::generic_type;
		using parameter_list_t = typename igenfunct_t::parameter_list_t;
		using string_t = typename generic_t::string_view;
		using scalar_t = typename generic_t::scalar_view;

		text() noexcept : exprtk::igeneric_function<T>("ST|STS")
		{

		}

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
		using igenfunct_t = exprtk::igeneric_function<T>;
		using generic_t = typename igenfunct_t::generic_type;
		using parameter_list_t = typename igenfunct_t::parameter_list_t;
		using string_t = typename generic_t::string_view;
		using scalar_t = typename generic_t::scalar_view;

		image() noexcept : exprtk::igeneric_function<T>("S|SS")
		{

		}

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
	struct collide : public exprtk::igeneric_function<T>
	{
		using igenfunct_t = exprtk::igeneric_function<T>;
		using generic_t = typename igenfunct_t::generic_type;
		using parameter_list_t = typename igenfunct_t::parameter_list_t;
		using string_t = typename generic_t::string_view;
		using scalar_t = typename generic_t::scalar_view;

		collide() noexcept : exprtk::igeneric_function<T>("S|S")
		{

		}

		inline T operator()(const std::size_t& ps_index, parameter_list_t parameters) override
		{
			tempSParams.push_back("collide");
			tempSParams.push_back(exprtk::to_str(string_t(parameters[0])));

			return 0;
		}
	};

	template <typename T>
	struct inc : public exprtk::igeneric_function<T>
	{
		using igenfunct_t = exprtk::igeneric_function<T>;
		using generic_t = typename igenfunct_t::generic_type;
		using parameter_list_t = typename igenfunct_t::parameter_list_t;
		using string_t = typename generic_t::string_view;
		using scalar_t = typename generic_t::scalar_view;

		inc() noexcept : exprtk::igeneric_function<T>("S|S")
		{

		}

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
		using igenfunct_t = exprtk::igeneric_function<T>;
		using generic_t = typename igenfunct_t::generic_type;
		using parameter_list_t = typename igenfunct_t::parameter_list_t;
		using string_t = typename generic_t::string_view;
		using scalar_t = typename generic_t::scalar_view;

		grid() noexcept : exprtk::igeneric_function<T>("TTT|TTTTT")
		{

		}

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
}
