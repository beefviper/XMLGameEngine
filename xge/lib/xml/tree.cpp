// tree.cpp
// XML Game Engine
// author: beefviper
// date: Nov 19, 2026

#include "xml/tree.h"

#include <string>
#include <utility>
#include <vector>

namespace xge
{
	namespace
	{
		// An element of the tree, and the children of its parent, for
		// getNextSibling().
		class TreeNode : public XmlNode
		{
		public:
			TreeNode(const TreeElement* element, const TreeElement* siblings, std::size_t count) :
				self(element), first(siblings), siblingCount(count)
			{
			}

			std::string getName() const override { return self->name; }

			std::string getAttribute(const std::string& name) const override
			{
				for (std::size_t i = 0; i < self->attributeCount; ++i)
				{
					if (name == self->attributes[i].name) { return self->attributes[i].value; }
				}
				return {};
			}

			std::vector<std::pair<std::string, std::string>> getAttributes() const override
			{
				std::vector<std::pair<std::string, std::string>> all;
				all.reserve(self->attributeCount);
				for (std::size_t i = 0; i < self->attributeCount; ++i)
				{
					all.emplace_back(self->attributes[i].name, self->attributes[i].value);
				}
				return all;
			}

			std::string getText() const override { return self->text ? self->text : ""; }
			int line() const override { return self->line; }

			std::unique_ptr<XmlNode> getFirstChild() const override
			{
				if (self->childCount == 0) { return nullptr; }
				return std::make_unique<TreeNode>(self->children, self->children, self->childCount);
			}

			std::unique_ptr<XmlNode> getNextSibling() const override
			{
				if (first == nullptr) { return nullptr; }
				const std::size_t next = static_cast<std::size_t>(self - first) + 1;
				if (next >= siblingCount) { return nullptr; }
				return std::make_unique<TreeNode>(first + next, first, siblingCount);
			}

		private:
			const TreeElement* self;
			const TreeElement* first;
			std::size_t siblingCount;
		};
	}

	std::unique_ptr<XmlNode> viewOfTree(const TreeElement& root)
	{
		return std::make_unique<TreeNode>(&root, nullptr, 0);
	}
}
