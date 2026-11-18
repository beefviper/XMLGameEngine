// name.h
// XML Game Engine
// author: beefviper
// date: Nov 17, 2026

#pragma once

#include <algorithm>
#include <cstddef>
#include <deque>
#include <functional>
#include <map>
#include <ostream>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace xge
{
	// A name the game file gives (an object's, its group's, its class's, a
	// state's), held once in the game's NameStore and handed round as this:
	// one pointer, read back as text where it is shown. The empty name
	// holds nothing. Two names of one game are the same name when they
	// point at the same text.
	class Name
	{
	public:
		Name() = default;

		const std::string& str() const noexcept { return text ? *text : none(); }
		operator const std::string&() const noexcept { return str(); }
		bool empty() const noexcept { return text == nullptr; }
		void clear() noexcept { text = nullptr; }
		std::size_t size() const noexcept { return str().size(); }

		friend bool operator==(Name a, Name b) noexcept { return a.text == b.text; }
		friend bool operator==(Name a, const std::string& b) noexcept { return a.str() == b; }
		friend bool operator==(Name a, const char* b) noexcept { return a.str() == b; }
		friend bool operator<(Name a, Name b) noexcept { return a.str() < b.str(); }

		friend std::string operator+(const std::string& a, Name b) { return a + b.str(); }
		friend std::string operator+(Name a, const std::string& b) { return a.str() + b; }
		friend std::string operator+(const char* a, Name b) { return a + b.str(); }
		friend std::string operator+(Name a, const char* b) { return a.str() + b; }
		friend std::string operator+(Name a, char b) { return a.str() + b; }
		friend std::ostream& operator<<(std::ostream& out, Name name) { return out << name.str(); }

	private:
		friend class NameStore;
		explicit Name(const std::string* held) noexcept : text(held) {}
		static const std::string& none() noexcept
		{
			static const std::string empty;
			return empty;
		}
		const std::string* text{ nullptr };
	};

	// Several names in order (an object's outer groups), held once in the
	// game's NameStore like a Name: one pointer. The empty list holds
	// nothing; objects of one group share the list.
	class NameList
	{
	public:
		NameList() = default;

		const std::vector<Name>& all() const noexcept { return list ? *list : none(); }
		std::vector<Name>::const_iterator begin() const noexcept { return all().begin(); }
		std::vector<Name>::const_iterator end() const noexcept { return all().end(); }
		std::size_t size() const noexcept { return all().size(); }
		bool empty() const noexcept { return list == nullptr; }
		Name operator[](std::size_t at) const noexcept { return all()[at]; }

	private:
		friend class NameStore;
		explicit NameList(const std::vector<Name>* held) noexcept : list(held) {}
		static const std::vector<Name>& none() noexcept
		{
			static const std::vector<Name> empty;
			return empty;
		}
		const std::vector<Name>* list{ nullptr };
	};

	// The game's names, each held once (an object keeps a Name, not its
	// own copy of the text). A name once held stays where it is, so a
	// Name read on another thread while one is added stays good.
	class NameStore
	{
	public:
		NameStore() = default;
		NameStore(const NameStore&) = delete;
		NameStore& operator=(const NameStore&) = delete;
		NameStore(NameStore&&) noexcept = default;
		NameStore& operator=(NameStore&&) noexcept = default;

		// The name of that text, held from now on ("" is the empty name).
		Name of(std::string_view text)
		{
			if (text.empty()) { return {}; }
			if (const auto found = index.find(text); found != index.end()) { return Name(found->second); }
			const std::string& kept = held.emplace_back(text);
			index.emplace(std::string_view(kept), &kept);
			return Name(&kept);
		}

		// The names of those texts, in order.
		std::vector<Name> of(const std::vector<std::string>& texts)
		{
			std::vector<Name> made;
			made.reserve(texts.size());
			for (const std::string& text : texts) { made.push_back(of(text)); }
			return made;
		}

		// The list of those texts' names, held from now on (none: the
		// empty list).
		NameList listOf(const std::vector<std::string>& texts)
		{
			if (texts.empty()) { return {}; }
			std::vector<Name> names = of(texts);
			if (const auto found = lists.find(names); found != lists.end()) { return NameList(found->second); }
			const std::vector<Name>& kept = heldLists.emplace_back(std::move(names));
			lists.emplace(kept, &kept);
			return NameList(&kept);
		}

		// The name of that text if it is held, else the empty name.
		Name find(std::string_view text) const
		{
			const auto found = index.find(text);
			return found == index.end() ? Name{} : Name(found->second);
		}

		std::size_t size() const noexcept { return held.size(); }

	private:
		std::deque<std::string> held;
		std::unordered_map<std::string_view, const std::string*> index;
		// Lists, told apart by which names they hold (each held once, so
		// the same name is the same text).
		struct ByHeld
		{
			bool operator()(const std::vector<Name>& a, const std::vector<Name>& b) const noexcept
			{
				return std::lexicographical_compare(a.begin(), a.end(), b.begin(), b.end(),
					[](Name x, Name y) { return std::less<const std::string*>{}(&x.str(), &y.str()); });
			}
		};
		std::deque<std::vector<Name>> heldLists;
		std::map<std::vector<Name>, const std::vector<Name>*, ByHeld> lists;
	};
}

namespace xge
{
	// The texts of those names, in order.
	inline std::vector<std::string> textsOf(const std::vector<Name>& names)
	{
		std::vector<std::string> texts;
		texts.reserve(names.size());
		for (const Name& name : names) { texts.push_back(name.str()); }
		return texts;
	}
	inline std::vector<std::string> textsOf(const NameList& names) { return textsOf(names.all()); }
}

template <>
struct std::hash<xge::Name>
{
	std::size_t operator()(xge::Name name) const noexcept { return std::hash<std::string>{}(name.str()); }
};
