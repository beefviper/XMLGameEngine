// variables.h
// XML Game Engine
// author: beefviper
// date: Nov 18, 2026

#pragma once

#include <cstddef>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace xge
{
	// An object's <variable>s by name, in name order: one list, held
	// together, read as a std::map is. A name is added only as the game
	// loads or a verb sets one it did not have, so the list stays short.
	// A number found in it is good only until a name is added.
	class Variables
	{
	public:
		using value_type = std::pair<std::string, float>;
		using iterator = std::vector<value_type>::iterator;
		using const_iterator = std::vector<value_type>::const_iterator;

		iterator begin() noexcept { return entries.begin(); }
		iterator end() noexcept { return entries.end(); }
		const_iterator begin() const noexcept { return entries.begin(); }
		const_iterator end() const noexcept { return entries.end(); }
		bool empty() const noexcept { return entries.empty(); }
		std::size_t size() const noexcept { return entries.size(); }

		iterator find(const std::string& name) noexcept
		{
			for (iterator at = entries.begin(); at != entries.end(); ++at)
			{
				const int order = at->first.compare(name);
				if (order == 0) { return at; }
				if (order > 0) { break; }
			}
			return entries.end();
		}
		const_iterator find(const std::string& name) const noexcept
		{
			for (const_iterator at = entries.begin(); at != entries.end(); ++at)
			{
				const int order = at->first.compare(name);
				if (order == 0) { return at; }
				if (order > 0) { break; }
			}
			return entries.end();
		}
		std::size_t count(const std::string& name) const noexcept { return find(name) == end() ? 0 : 1; }

		float& at(const std::string& name)
		{
			const iterator found = find(name);
			if (found == entries.end()) { throw std::out_of_range("no variable '" + name + "'"); }
			return found->second;
		}
		const float& at(const std::string& name) const
		{
			const const_iterator found = find(name);
			if (found == entries.end()) { throw std::out_of_range("no variable '" + name + "'"); }
			return found->second;
		}

		// The number of that name, added as 0 if it has none.
		float& operator[](const std::string& name) { return try_emplace(name, 0.0f).first->second; }

		// The number of that name, added as `value` if it has none.
		std::pair<iterator, bool> try_emplace(const std::string& name, float value)
		{
			const iterator at = place(name);
			if (at != entries.end() && at->first == name) { return { at, false }; }
			return { entries.emplace(at, name, value), true };
		}

		friend bool operator==(const Variables&, const Variables&) = default;

	private:
		// Where that name is, or would go: the lists are short, so a walk.
		iterator place(const std::string& name) noexcept
		{
			iterator at = entries.begin();
			while (at != entries.end() && at->first.compare(name) < 0) { ++at; }
			return at;
		}
		const_iterator place(const std::string& name) const noexcept
		{
			const_iterator at = entries.begin();
			while (at != entries.end() && at->first.compare(name) < 0) { ++at; }
			return at;
		}

		std::vector<value_type> entries;
	};
}
