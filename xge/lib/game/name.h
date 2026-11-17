// name.h
// XML Game Engine
// author: beefviper
// date: Nov 17, 2026

#pragma once

#include <cstddef>
#include <deque>
#include <functional>
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
}

template <>
struct std::hash<xge::Name>
{
	std::size_t operator()(xge::Name name) const noexcept { return std::hash<std::string>{}(name.str()); }
};
