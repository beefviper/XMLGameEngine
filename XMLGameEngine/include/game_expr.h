// game_expr.h
// XML Game Engine
// author: beefviper
// date: Sept 18, 2020

#pragma once

// disable warnings generated by exprtk
#pragma warning(push)
#pragma warning(disable : 26812)
#pragma warning(disable : 26451)
#pragma warning(disable : 26495)
#pragma warning(disable : 26498)
#pragma warning(disable : 26819)

#include <exprtk.hpp>

#pragma warning(pop)

#include <random>

namespace xge
{
	namespace
	{
		std::vector<float> tempParams;
		std::vector<std::string> tempSParams;

		std::random_device seed;
		std::mt19937 generator(seed());

		void clearAllTempParams(void)
		{
			tempParams.clear();
			tempSParams.clear();
		}
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
	struct exprGenericFunction : public exprtk::igeneric_function<T>
	{
		using igenfunct_t = exprtk::igeneric_function<T>;
		using generic_t = typename igenfunct_t::generic_type;
		using parameter_list_t = typename igenfunct_t::parameter_list_t;
		using string_t = typename generic_t::string_view;
		using scalar_t = typename generic_t::scalar_view;

		exprGenericFunction(const std::string& funcSig)
			: igeneric_function(funcSig)
		{

		}
	};

	template <typename T>
	struct shapeCircle : public exprGenericFunction<T>
	{
		shapeCircle() : exprGenericFunction<T>("T|TS")
		{

		}

		T operator()(const std::size_t& ps_index, parameter_list_t parameters)
		{
			clearAllTempParams();
			tempParams.push_back(scalar_t(parameters[0])());

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
	struct shapeRectangle : public exprGenericFunction<T>
	{
		shapeRectangle() : exprGenericFunction<T>("TT|TTS")
		{

		}

		T operator()(const std::size_t& ps_index, parameter_list_t parameters)
		{
			clearAllTempParams();
			tempParams.push_back(scalar_t(parameters[0])());
			tempParams.push_back(scalar_t(parameters[1])());

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
	struct text : public exprGenericFunction<T>
	{
		text() : exprGenericFunction<T>("ST|STS")
		{

		}

		inline T operator()(const std::size_t& ps_index, parameter_list_t parameters)
		{
			clearAllTempParams();
			tempSParams.push_back(exprtk::to_str(string_t(parameters[0])));
			tempParams.push_back(scalar_t(parameters[1])());

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
	struct image : public exprGenericFunction<T>
	{
		image() : exprGenericFunction<T>("S|S")
		{

		}

		inline T operator()(const std::size_t& ps_index, parameter_list_t parameters)
		{
			clearAllTempParams();

			tempSParams.push_back(exprtk::to_str(string_t(parameters[0])));

			return 0;
		}
	};
}
