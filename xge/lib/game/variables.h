// variables.h
// XML Game Engine
// author: beefviper
// date: Nov 18, 2026

#pragma once

#include "game/name.h"

#include <cstddef>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace xge
{
	// An object's <variable>s, in name order: one list, held together, read
	// as a std::map is. Each is under a Name, so a step that holds the Name
	// finds it by id; by its text is for loading, messages and tests. A
	// name is added only as the game loads or a verb sets one it did not
	// have, so the list stays short. A number found in it is good only
	// until a name is added.
	class Variables
	{
	public:
		using value_type = std::pair<Name, float>;
		using iterator = std::vector<value_type>::iterator;
		using const_iterator = std::vector<value_type>::const_iterator;

		iterator begin() noexcept { return entries.begin(); }
		iterator end() noexcept { return entries.end(); }
		const_iterator begin() const noexcept { return entries.begin(); }
		const_iterator end() const noexcept { return entries.end(); }
		bool empty() const noexcept { return entries.empty(); }
		std::size_t size() const noexcept { return entries.size(); }

		iterator find(Name name) noexcept
		{
			for (iterator at = entries.begin(); at != entries.end(); ++at)
			{
				if (at->first == name) { return at; }
			}
			return entries.end();
		}
		const_iterator find(Name name) const noexcept
		{
			for (const_iterator at = entries.begin(); at != entries.end(); ++at)
			{
				if (at->first == name) { return at; }
			}
			return entries.end();
		}
		std::size_t count(Name name) const noexcept { return find(name) == end() ? 0 : 1; }

		// The value under that name, or `otherwise` without one.
		float valueOr(Name name, float otherwise) const noexcept
		{
			const const_iterator found = find(name);
			return found == end() ? otherwise : found->second;
		}

		float& operator[](Name name) { return try_emplace(name, 0.0f).first->second; }

		std::pair<iterator, bool> try_emplace(Name name, float value)
		{
			if (const iterator found = find(name); found != entries.end()) { return { found, false }; }
			iterator at = entries.begin();
			while (at != entries.end() && at->first.str() < name.str()) { ++at; }
			return { entries.emplace(at, name, value), true };
		}

		// By its text: for loading, messages and tests.
		iterator findText(std::string_view name) noexcept
		{
			for (iterator at = entries.begin(); at != entries.end(); ++at)
			{
				if (at->first.str() == name) { return at; }
			}
			return entries.end();
		}
		const_iterator findText(std::string_view name) const noexcept
		{
			for (const_iterator at = entries.begin(); at != entries.end(); ++at)
			{
				if (at->first.str() == name) { return at; }
			}
			return entries.end();
		}
		std::size_t countText(std::string_view name) const noexcept { return findText(name) == end() ? 0 : 1; }
		float& at(std::string_view name)
		{
			const iterator found = findText(name);
			if (found == entries.end()) { throw std::out_of_range("no variable '" + std::string(name) + "'"); }
			return found->second;
		}
		const float& at(std::string_view name) const
		{
			const const_iterator found = findText(name);
			if (found == entries.end()) { throw std::out_of_range("no variable '" + std::string(name) + "'"); }
			return found->second;
		}

		friend bool operator==(const Variables&, const Variables&) = default;

	private:
		std::vector<value_type> entries;
	};
}
