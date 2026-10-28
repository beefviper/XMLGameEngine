// merge.cpp
// XML Game Engine
// author: beefviper
// date: Oct 25, 2026

#include "load/merge.h"

#include "load/spelling.h"

#include <algorithm>
#include <cstddef>
#include <sstream>
#include <stdexcept>

namespace xge
{
	namespace
	{
		[[noreturn]] void fail(const std::string& where, const std::string& message)
		{
			throw std::runtime_error(where + ": " + message);
		}

		using Made = std::shared_ptr<Element>;

		// An element as an XmlNode: it, and where it stands among its
		// parent's children, for getNextSibling().
		class ElementNode : public XmlNode
		{
		public:
			ElementNode(ElementPtr element, ElementPtr above, std::size_t at) :
				self(std::move(element)), parent(std::move(above)), index(at)
			{
			}

			std::string getName() const override { return self->name; }
			std::string getAttribute(const std::string& key) const override { return self->attribute(key); }
			std::vector<std::pair<std::string, std::string>> getAttributes() const override { return self->attributes; }
			std::string getText() const override { return self->text; }

			std::unique_ptr<XmlNode> getFirstChild() const override
			{
				if (self->children.empty()) { return nullptr; }
				return std::make_unique<ElementNode>(self->children.front(), self, 0);
			}

			std::unique_ptr<XmlNode> getNextSibling() const override
			{
				if (!parent || index + 1 >= parent->children.size()) { return nullptr; }
				return std::make_unique<ElementNode>(parent->children[index + 1], parent, index + 1);
			}

		private:
			ElementPtr self;
			ElementPtr parent;
			std::size_t index;
		};

		void setAttribute(Element& element, const std::string& key, const std::string& value)
		{
			for (auto& [name, existing] : element.attributes)
			{
				if (name == key)
				{
					existing = value;
					return;
				}
			}
			element.attributes.emplace_back(key, value);
		}

		void dropAttribute(Element& element, const std::string& key)
		{
			element.attributes.erase(std::remove_if(element.attributes.begin(), element.attributes.end(),
				[&](const auto& attribute) { return attribute.first == key; }), element.attributes.end());
		}

		using Children = std::vector<ElementPtr>;

		bool isLayout(const std::string& tag)
		{
			return tag == "member" || tag == "row" || tag == "column" || tag == "cell";
		}

		// How a child meets base's like it, by the tag of what holds it:
		//   Whole  by tag and name, taking its place whole (sprite, animation)
		//   Deep   by tag and name, changing it the same way (group, object)
		//   Named  by tag and name, a name="" never matching (variable, action, timer)
		//   Inner  by tag, its own children merged (position, variables, collisions...)
		//   Tag    every one of a tag in place of every one of base's
		//   All    every one of the kind in place of every one of base's, at the end
		//          (member, row, column, cell; solid)
		//   Added  after base's (collision)
		enum class Match { Whole, Deep, Named, Inner, Tag, All, Added };

		Match matchOf(const std::string& parent, const std::string& tag)
		{
			if (parent == "variables" || parent == "actions" || parent == "timers") { return Match::Named; }
			if (parent == "collisions")
			{
				if (tag == "collision") { return Match::Added; }
				return tag == "solid" ? Match::All : Match::Tag;
			}
			if (parent == "position" || parent == "velocity" || parent == "acceleration" || parent == "padding") { return Match::Tag; }
			if (tag == "sprite" || tag == "animation") { return Match::Whole; }
			if (tag == "group" || tag == "object") { return Match::Deep; }
			if (tag == "position" || tag == "velocity" || tag == "acceleration" || tag == "padding"
				|| tag == "variables" || tag == "actions" || tag == "timers" || tag == "collisions") { return Match::Inner; }
			return isLayout(tag) ? Match::All : Match::Tag;
		}

		bool sameKind(Match match, const ElementPtr& one, const ElementPtr& other)
		{
			if (match == Match::All && isLayout(one->name)) { return isLayout(other->name); }
			if (one->name != other->name) { return false; }
			if (match == Match::Whole || match == Match::Deep || match == Match::Named) { return one->attribute("name") == other->attribute("name"); }
			return true;
		}

		// The last of `children` like `child`, or nothing.
		ElementPtr lastLike(Match match, const Children& children, const ElementPtr& child)
		{
			if (match == Match::Named && child->attribute("name").empty()) { return nullptr; }
			for (auto each = children.rbegin(); each != children.rend(); ++each)
			{
				if (sameKind(match, *each, child)) { return *each; }
			}
			return nullptr;
		}

		// The children of `parent` (its tag), base's changed by over's: base's
		// in order, each in place or changed by over's like it, then over's
		// that took no place (of those alike, only the last).
		Children mergeChildren(const std::string& parent, const Children& base, const Children& over, const std::string& where)
		{
			Children made;
			for (std::size_t i = 0; i < base.size(); ++i)
			{
				const ElementPtr& child = base[i];
				const Match match = matchOf(parent, child->name);
				const ElementPtr like = lastLike(match, over, child);
				if (match == Match::Whole && like) { made.push_back(like); continue; }
				if (match == Match::Named && like) { made.push_back(like); continue; }
				if ((match == Match::Deep || match == Match::Inner) && like) { made.push_back(changed(child, like, where + " > <" + child->name + ">")); continue; }
				if (match == Match::All && like) { continue; }
				if (match == Match::Tag && like)
				{
					// all of over's of the tag, where base's first of it was
					const bool first = std::none_of(base.begin(), base.begin() + static_cast<std::ptrdiff_t>(i), [&](const ElementPtr& each) { return each->name == child->name; });
					if (first)
					{
						for (const ElementPtr& each : over) { if (each->name == child->name) { made.push_back(each); } }
					}
					continue;
				}
				made.push_back(child);
			}
			for (std::size_t i = 0; i < over.size(); ++i)
			{
				const ElementPtr& child = over[i];
				const Match match = matchOf(parent, child->name);
				if (match == Match::All || match == Match::Added) { made.push_back(child); continue; }
				if (match == Match::Tag)
				{
					if (std::none_of(base.begin(), base.end(), [&](const ElementPtr& each) { return each->name == child->name; })) { made.push_back(child); }
					continue;
				}
				if (match == Match::Named && child->attribute("name").empty()) { made.push_back(child); continue; }
				const bool inBase = lastLike(match, base, child) != nullptr;
				const bool later = std::any_of(over.begin() + static_cast<std::ptrdiff_t>(i) + 1, over.end(), [&](const ElementPtr& each) { return sameKind(match, each, child); });
				if (!inBase && !later) { made.push_back(child); }
			}
			return made;
		}

		bool holdsMember(const ElementPtr& element, const std::string& member)
		{
			for (const ElementPtr& child : element->children)
			{
				if (child->name == "member" && child->attribute("name") == member) { return true; }
				if (child->name == "group" && holdsMember(child, member)) { return true; }
			}
			return false;
		}

		bool holdsGroup(const ElementPtr& element, const std::string& group)
		{
			for (const ElementPtr& child : element->children)
			{
				if (child->name == "group" && (child->attribute("name") == group || holdsGroup(child, group))) { return true; }
			}
			return false;
		}

		// The group with `over` changing the group inside it called `group`.
		ElementPtr changedInner(const ElementPtr& holder, const std::string& group, const ElementPtr& over, const std::string& where)
		{
			Made made = std::make_shared<Element>(*holder);
			for (ElementPtr& child : made->children)
			{
				if (child->name != "group") { continue; }
				if (child->attribute("name") == group) { child = changed(child, over, where); }
				else if (holdsGroup(child, group)) { child = changedInner(child, group, over, where); }
			}
			return made;
		}

		// The group with that member alone in it (and the groups inside it
		// that lead to it); nothing when it has no member of that name.
		ElementPtr onlyMember(const ElementPtr& group, const std::string& member)
		{
			Made made = std::make_shared<Element>(*group);
			made->children.clear();
			bool found = false;
			for (const ElementPtr& child : group->children)
			{
				if (child->name == "member")
				{
					if (child->attribute("name") != member) { continue; }
					found = true;
				}
				else if (child->name == "group")
				{
					ElementPtr inner = onlyMember(child, member);
					if (!inner) { continue; }
					made->children.push_back(inner);
					found = true;
					continue;
				}
				made->children.push_back(child);
			}
			return found ? ElementPtr(made) : nullptr;
		}

		// The group with `over` changing its member `member`: what a member
		// can have (sprites, animations, position, velocity) in the member,
		// the rest, when `asGroup`, in the group that holds it. `rename` is
		// the member's new name ("" to keep it).
		ElementPtr changedMember(const ElementPtr& group, const std::string& member, const ElementPtr& over, bool asGroup, const std::string& rename, const std::string& where)
		{
			Made own = std::make_shared<Element>();
			own->name = "member";
			if (!rename.empty() && rename != member) { own->attributes.emplace_back("name", rename); }
			Made rest = std::make_shared<Element>();
			rest->name = "group";
			for (const auto& [key, value] : over->attributes)
			{
				if (key != "name" && key != "from") { rest->attributes.emplace_back(key, value); }
			}
			for (const ElementPtr& child : over->children)
			{
				const std::string& tag = child->name;
				if (tag == "sprite" || tag == "animation" || tag == "position" || tag == "velocity") { own->children.push_back(child); }
				else { rest->children.push_back(child); }
			}
			if (!asGroup && (!rest->children.empty() || !rest->attributes.empty()))
			{
				fail(where, "changes '" + member + "', a member of group '" + group->attribute("name")
					+ "', and a member changes only its <sprite>s, <animation>s, <position> and <velocity>; change the group, or take the member on its own from the <kit>");
			}

			const auto apply = [&](const auto& self, const ElementPtr& holder) -> ElementPtr
			{
				Made made = std::make_shared<Element>(*holder);
				bool here = false;
				for (ElementPtr& child : made->children)
				{
					if (child->name == "member" && child->attribute("name") == member)
					{
						child = changed(child, own, where);
						here = true;
					}
					else if (child->name == "group" && holdsMember(child, member))
					{
						child = self(self, child);
					}
				}
				if (here && (!rest->children.empty() || !rest->attributes.empty())) { return changed(made, rest, where); }
				return made;
			};
			return apply(apply, group);
		}

		// `taken` holds the sets already looked in, so one that takes itself
		// (a path that follows itself) is looked in once.
		void collect(const ElementPtr& element, const Kit& kit, std::set<std::string>& names, std::set<std::pair<std::string, std::string>>& taken)
		{
			const auto look = [&](const std::string& tag, const std::string& name)
			{
				const ElementPtr set = kit.setOf(tag, name);
				if (set && taken.emplace(tag, name).second) { collect(set, kit, names, taken); }
			};
			const std::string& tag = element->name;
			if (tag == "spawn" || tag == "fire" || tag == "release")
			{
				if (const std::string object = element->attribute("object"); !object.empty()) { names.insert(object); }
			}
			else if (tag == "follow") { look("path", element->attribute("path")); }
			else if (tag == "condition" && !element->attribute("set").empty()) { look("conditions", element->attribute("set")); }
			else if (tag == "inputs")
			{
				std::istringstream sets(element->attribute("keys"));
				for (std::string set; sets >> set;) { look("keys", set); }
			}
			for (const ElementPtr& child : element->children) { collect(child, kit, names, taken); }
		}
	}

	std::string Element::attribute(const std::string& key) const
	{
		for (const auto& [each, value] : attributes)
		{
			if (each == key) { return value; }
		}
		return {};
	}

	ElementPtr copyElement(const XmlNode& node)
	{
		Made made = std::make_shared<Element>();
		made->name = node.getName();
		made->attributes = node.getAttributes();
		made->text = node.getText();
		for (std::unique_ptr<XmlNode> child = node.getFirstChild(); child != nullptr; child = child->getNextSibling())
		{
			made->children.push_back(copyElement(*child));
		}
		return made;
	}

	std::unique_ptr<XmlNode> viewOf(const ElementPtr& element)
	{
		return std::make_unique<ElementNode>(element, nullptr, 0);
	}

	ElementPtr changed(const ElementPtr& base, const ElementPtr& over, const std::string& where)
	{
		Made made = std::make_shared<Element>(*base);
		dropAttribute(*made, "from");
		for (const auto& [key, value] : over->attributes)
		{
			if (key != "from") { setAttribute(*made, key, value); }
		}
		made->children = mergeChildren(base->name, base->children, over->children, where);
		return made;
	}

	std::set<std::string> namesIn(const ElementPtr& element)
	{
		std::set<std::string> names;
		if (const std::string name = element->attribute("name"); !name.empty()) { names.insert(name); }
		for (const ElementPtr& child : element->children)
		{
			if (child->name == "group" || child->name == "object" || child->name == "member" || child->name == "map")
			{
				const std::set<std::string> inner = namesIn(child);
				names.insert(inner.begin(), inner.end());
			}
		}
		return names;
	}

	std::set<std::string> spawnedIn(const ElementPtr& element, const Kit& kit)
	{
		std::set<std::string> names;
		std::set<std::pair<std::string, std::string>> taken;
		collect(element, kit, names, taken);
		return names;
	}

	void Kit::add(const ElementPtr& piece, const std::string& where)
	{
		const std::string name = piece->attribute("name");
		const std::string here = where + " > <" + piece->name + " name=\"" + name + "\">";
		if (piece->name != "object" && piece->name != "group" && piece->name != "map") { fail(where, "holds <object>s, <group>s and <map>s, not a <" + piece->name + ">"); }
		if (name.empty()) { fail(where, "has a <" + piece->name + "> with no name=\"...\""); }
		if (name.find('.') != std::string::npos) { fail(here, "a piece's name has no dots: they join the parts of a path"); }
		if (has(name)) { fail(here, "the <kit> has another piece (or a member of one) called '" + name + "'"); }

		ElementPtr made = piece;
		if (const std::string from = piece->attribute("from"); !from.empty())
		{
			if (!isPiece(from)) { fail(here, "from=\"" + from + "\" names no piece of the <kit> before it" + didYouMean(from, names())); }
			if (isMap(from) != (piece->name == "map")) { fail(here, "from=\"" + from + "\": a <map> is from= a <map>, and an object or a group from= an object or a group"); }
			made = take(from, piece, here);
		}

		std::vector<std::pair<std::string, std::string>> named;
		const auto walk = [&](const auto& self, const ElementPtr& group) -> void
		{
			for (const ElementPtr& child : group->children)
			{
				if (child->name == "member" && !child->attribute("name").empty())
				{
					const std::string member = child->attribute("name");
					if (has(member) || member == name || std::any_of(named.begin(), named.end(), [&](const auto& each) { return each.first == member; }))
					{
						fail(here, "has a member called '" + member + "', and the <kit> has another piece or member of that name");
					}
					named.emplace_back(member, name);
				}
				else if (child->name == "group") { self(self, child); }
			}
		};
		if (made->name == "group" || made->name == "map") { walk(walk, made); }

		pieces[name] = made;
		order.push_back(name);
		for (const auto& [member, of] : named) { members[member] = of; }
	}

	bool Kit::has(const std::string& name) const
	{
		return pieces.count(name) > 0 || members.count(name) > 0;
	}

	bool Kit::isPiece(const std::string& name) const
	{
		return pieces.count(name) > 0;
	}

	bool Kit::isMap(const std::string& name) const
	{
		const auto piece = pieces.find(name);
		return piece != pieces.end() && piece->second->name == "map";
	}

	ElementPtr Kit::take(const std::string& name, const ElementPtr& over, const std::string& where) const
	{
		const std::string overName = over->attribute("name");
		if (const auto piece = pieces.find(name); piece != pieces.end())
		{
			return changed(piece->second, over, where);
		}
		const auto member = members.find(name);
		if (member == members.end()) { fail(where, "takes '" + name + "', which the <kit> has no piece or member called" + didYouMean(name, names())); }
		const ElementPtr only = onlyMember(pieces.at(member->second), name);
		return changedMember(only, name, over, true, overName, where);
	}

	std::string Kit::pieceOf(const std::string& name) const
	{
		if (pieces.count(name)) { return name; }
		const auto member = members.find(name);
		return member == members.end() ? std::string{} : member->second;
	}

	std::vector<std::string> Kit::names() const
	{
		std::vector<std::string> all = order;
		for (const auto& [member, of] : members) { all.push_back(member); }
		return all;
	}

	void Kit::addSet(const std::string& tag, const std::string& name, const ElementPtr& set)
	{
		sets[{ tag, name }] = set;
	}

	ElementPtr Kit::setOf(const std::string& tag, const std::string& name) const
	{
		const auto found = sets.find({ tag, name });
		return found == sets.end() ? nullptr : found->second;
	}

	std::vector<ElementPtr> layOut(const XmlNode& objects, std::vector<ElementPtr> inherited, const Kit& kit, const std::string& where)
	{
		std::vector<ElementPtr> list = std::move(inherited);
		for (std::unique_ptr<XmlNode> node = objects.getFirstChild(); node != nullptr; node = node->getNextSibling())
		{
			const ElementPtr written = copyElement(*node);
			const std::string tag = written->name;
			const std::string name = written->attribute("name");
			const std::string from = written->attribute("from");
			if (name.empty()) { fail(where, "has an <" + tag + "> with no name=\"...\""); }
			const std::string here = where + " > <" + tag + " name=\"" + name + "\">";
			if (name.find('.') != std::string::npos)
			{
				fail(here, "names '" + name + "', a numbered member of a group: change the whole group, or give the member a name= and change that");
			}

			const auto same = std::find_if(list.begin(), list.end(), [&](const ElementPtr& each) { return each->attribute("name") == name; });
			if (!from.empty())
			{
				if (!kit.has(from)) { fail(here, "from=\"" + from + "\" names no piece of the <kit>" + didYouMean(from, kit.names())); }
				if (kit.isMap(from) != (tag == "map"))
				{
					fail(here, tag == "map" ? "from=\"" + from + "\" names a piece that is not a <map>; a <map> takes a <map> of the kit"
						: "from=\"" + from + "\" names a <map> of the kit, which only a <map> takes");
				}
				if (same != list.end()) { fail(here, "there is already an object or group called '" + name + "' here"); }
				list.push_back(kit.take(from, written, here));
				continue;
			}
			if (same != list.end())
			{
				*same = changed(*same, written, here);
				continue;
			}
			const auto holder = std::find_if(list.begin(), list.end(), [&](const ElementPtr& each) { return each->name == "group" && holdsMember(each, name); });
			if (holder != list.end())
			{
				*holder = changedMember(*holder, name, written, false, {}, here);
				continue;
			}
			const auto outer = std::find_if(list.begin(), list.end(), [&](const ElementPtr& each) { return each->name == "group" && holdsGroup(each, name); });
			if (outer != list.end() && tag == "group")
			{
				*outer = changedInner(*outer, name, written, here);
				continue;
			}
			// by its name: a map a map of the kit, anything else an object
			// or a group (or a member)
			if (kit.has(name) && kit.isMap(name) == (tag == "map"))
			{
				list.push_back(kit.take(name, written, here));
				continue;
			}
			list.push_back(written);
		}
		return list;
	}

	void addWaiting(std::vector<ElementPtr>& list, std::set<std::string> spawned, const std::set<std::string>& seen, const Kit& kit, const std::string& where)
	{
		std::set<std::string> placed = seen;
		for (const ElementPtr& element : list)
		{
			const std::set<std::string> names = namesIn(element);
			placed.insert(names.begin(), names.end());
			const std::set<std::string> more = spawnedIn(element, kit);
			spawned.insert(more.begin(), more.end());
		}

		while (!spawned.empty())
		{
			std::set<std::string> pieces;
			for (const std::string& name : spawned)
			{
				const std::string piece = kit.pieceOf(name);
				if (!piece.empty() && !placed.count(name) && !placed.count(piece)) { pieces.insert(piece); }
			}
			spawned.clear();
			for (const std::string& piece : pieces)
			{
				Made over = std::make_shared<Element>();
				over->name = "object";
				over->attributes.emplace_back("name", piece);
				Made waiting = std::make_shared<Element>(*kit.take(piece, over, where));
				waiting->children.erase(std::remove_if(waiting->children.begin(), waiting->children.end(),
					[](const ElementPtr& child) { return child->name == "exists"; }), waiting->children.end());
				Made exists = std::make_shared<Element>();
				exists->name = "exists";
				exists->text = "false";
				waiting->children.push_back(exists);
				list.push_back(waiting);

				const std::set<std::string> names = namesIn(waiting);
				placed.insert(names.begin(), names.end());
				const std::set<std::string> more = spawnedIn(waiting, kit);
				spawned.insert(more.begin(), more.end());
			}
		}
	}
}
