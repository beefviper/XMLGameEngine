// spelling.cpp
// XML Game Engine
// author: beefviper
// date: Oct 7, 2026

#include "spelling.h"

#include <algorithm>
#include <cctype>
#include <cstddef>

namespace xge
{
	namespace
	{
		std::string lowerCase(std::string text)
		{
			std::transform(text.begin(), text.end(), text.begin(),
				[](unsigned char c) { return static_cast<char>(std::tolower(c)); });
			return text;
		}

		// How many letters have to be added, taken away or changed to turn one
		// word into the other (Levenshtein), a swap of two neighbours counting one.
		std::size_t distance(const std::string& a, const std::string& b)
		{
			std::vector<std::vector<std::size_t>> d(a.size() + 1, std::vector<std::size_t>(b.size() + 1));
			for (std::size_t i = 0; i <= a.size(); ++i) { d[i][0] = i; }
			for (std::size_t j = 0; j <= b.size(); ++j) { d[0][j] = j; }

			for (std::size_t i = 1; i <= a.size(); ++i)
			{
				for (std::size_t j = 1; j <= b.size(); ++j)
				{
					const std::size_t change = a[i - 1] == b[j - 1] ? 0 : 1;
					d[i][j] = std::min({ d[i - 1][j] + 1, d[i][j - 1] + 1, d[i - 1][j - 1] + change });
					if (i > 1 && j > 1 && a[i - 1] == b[j - 2] && a[i - 2] == b[j - 1])
					{
						d[i][j] = std::min(d[i][j], d[i - 2][j - 2] + 1);
					}
				}
			}

			return d[a.size()][b.size()];
		}
	}

	std::string didYouMean(const std::string& name, const std::vector<std::string>& names)
	{
		const std::string wanted = lowerCase(name);
		const std::string* best = nullptr;
		std::size_t bestDistance = 0;

		for (const std::string& candidate : names)
		{
			const std::size_t d = distance(wanted, lowerCase(candidate));
			if (!best || d < bestDistance)
			{
				best = &candidate;
				bestDistance = d;
			}
		}

		// Two letters out in a long name, one in a short one: further than that
		// is a different word, and a guess would mislead.
		const std::size_t allowed = wanted.size() >= 5 ? 2 : 1;
		if (!best || bestDistance > allowed || *best == name)
		{
			return {};
		}
		return " (did you mean '" + *best + "'?)";
	}

	std::string listOf(const std::vector<std::string>& names)
	{
		std::string text;
		for (const std::string& name : names)
		{
			if (!text.empty()) { text += ", "; }
			text += name;
		}
		return text;
	}
}
