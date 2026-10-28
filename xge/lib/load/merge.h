// merge.h
// XML Game Engine
// author: beefviper
// date: Oct 25, 2026

#pragma once

#include "xml/document.h"

#include <map>
#include <memory>
#include <set>
#include <string>
#include <utility>
#include <vector>

namespace xge
{
	// An element of the file copied out of it, so that what something starts
	// from can be changed by what is written over it, and then read as if it
	// had been written out whole: a piece of the <kit> taken, an object of a
	// state or scene that another is from=.
	struct Element;
	using ElementPtr = std::shared_ptr<const Element>;

	struct Element
	{
		std::string name;
		std::vector<std::pair<std::string, std::string>> attributes;
		std::string text;
		std::vector<ElementPtr> children;

		// "" when it has none, as XmlNode::getAttribute.
		std::string attribute(const std::string& key) const;
	};

	ElementPtr copyElement(const XmlNode& node);

	// The element read as an XmlNode, so load/xml reads it like any other.
	std::unique_ptr<XmlNode> viewOf(const ElementPtr& element);

	// `base` changed by `over`, the rule every start-from follows. Over's
	// attributes go over base's (from= is dropped). Of its children:
	//   * a <sprite> or <animation> replaces base's of the same name (an
	//     unnamed one base's unnamed one), or is added;
	//   * the <x> and <y> of a <position>, <velocity>, <acceleration> or
	//     <padding> replace base's, each on its own;
	//   * the <variable>s, <action>s and <timer>s in <variables>, <actions>
	//     and <timers> replace base's of the same name, or are added (a timer
	//     with no name is added);
	//   * in <collisions>, the <collision> rules come after base's, <solid>s
	//     replace all of base's, and anything else replaces base's;
	//   * a group's <member>s, <row>s, <column>s and <cell>s replace all of
	//     base's, and a <group> or an <object> inside (a group's, a map's)
	//     changes base's of the same name, the same way, or is added;
	//   * anything else replaces base's of that tag.
	ElementPtr changed(const ElementPtr& base, const ElementPtr& over, const std::string& where);

	// The names an element can be called by: its own, and those of the
	// groups, members and objects inside it.
	std::set<std::string> namesIn(const ElementPtr& element);

	class Kit;

	// The objects that a <spawn>, <fire> or <release> in it names, and in a
	// path it <follow>s, a <keys> set its <inputs keys> take or a
	// <conditions> set a <condition set> takes.
	std::set<std::string> spawnedIn(const ElementPtr& element, const Kit& kit);

	// The game's <kit>: pieces written once (objects, groups and maps), which
	// are in play only where something takes them. A piece can be from= an
	// earlier one.
	class Kit
	{
	public:
		void add(const ElementPtr& piece, const std::string& where);

		// A piece, or a member of a group piece that has a name of its own.
		bool has(const std::string& name) const;
		bool isPiece(const std::string& name) const;
		// A piece that is a <map>, which only a <map> takes.
		bool isMap(const std::string& name) const;

		// The piece a name is (or the group piece a member is of); "" for none.
		std::string pieceOf(const std::string& name) const;

		// What `over` makes of the piece (or member) `name`: a copy of it,
		// changed by over and called by over's name. A member taken is its
		// group with that member alone in it.
		ElementPtr take(const std::string& name, const ElementPtr& over, const std::string& where) const;

		std::vector<std::string> names() const;

		// The game's <path>s, and the <keys> and <conditions> sets of its
		// <states>, by tag and name, for what they spawn.
		void addSet(const std::string& tag, const std::string& name, const ElementPtr& set);
		ElementPtr setOf(const std::string& tag, const std::string& name) const;

	private:
		std::map<std::string, ElementPtr> pieces;
		std::vector<std::string> order;
		std::map<std::string, std::string> members; // a named member: its piece
		std::map<std::pair<std::string, std::string>, ElementPtr> sets;
	};

	// The objects of an <objects>: those of what it is from= (`inherited`),
	// each changed by an object or group written of the same name (or of
	// the name of a member or a group inside one of them), then the rest
	// written, in order, each its own, or a piece of the kit taken (by
	// from=, or by having the name of a piece or of a named member).
	std::vector<ElementPtr> layOut(const XmlNode& objects, std::vector<ElementPtr> inherited, const Kit& kit, const std::string& where);

	// A piece of the kit a <spawn>, <fire> or <release> names (in `spawned`,
	// or in an object of the list), and that nothing in `list` or `seen`
	// is called, is added to the list as one that does not exist until it is
	// spawned; and so on for what those name. Alphabetical, round by round.
	void addWaiting(std::vector<ElementPtr>& list, std::set<std::string> spawned, const std::set<std::string>& seen, const Kit& kit, const std::string& where);
}
