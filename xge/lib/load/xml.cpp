// xml.cpp
// XML Game Engine
// author: beefviper
// date: Sept 21, 2020

#include "load/xml.h"

#include "engine/keys.h"
#include "load/merge.h"
#include "load/reading.h"
#include "load/spelling.h"
#include "verbs/verb.h"

#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <functional>
#include <initializer_list>
#include <map>
#include <memory>
#include <optional>
#include <set>
#include <stdexcept>
#include <string_view>

namespace xge
{
	namespace
	{
		// What load/xml knows how to read is fixed by xgedef.xsd, and every
		// game is checked against it before it gets here, so the readers
		// below trust that shape and check only what a schema cannot say,
		// saying where it was.

		WrittenVector2 readVector2(const XmlNode& node, const std::string& where)
		{
			const std::string here = where + " > <" + node.getName() + ">";
			WrittenVector2 vector;
			vector.x = readValueOf(node, "x", here);
			vector.y = readValueOf(node, "y", here);
			return vector;
		}

		bool isShapeTag(const std::string& name)
		{
			return name == "circle" || name == "rectangle" || name == "text" || name == "image" || name == "bitmap" || name == "svg" || name == "tether" || name == "cell";
		}

		// The <flip> of a picture the engine draws itself (a <bitmap> or an
		// <svg>): "", "horizontal" or "vertical". Checked here, since only
		// Xerces would catch a wrong word from the schema.
		std::string readPictureFlip(const XmlNode& shape, const std::string& where)
		{
			const auto flip = findChild(&shape, "flip");
			if (!flip) { return {}; }

			const std::string text = readText(*flip);
			if (text != "horizontal" && text != "vertical")
			{
				fail(where, "<flip> is \"" + text + "\"; expected horizontal or vertical");
			}
			return text;
		}

		// The pieces of a <text>, in the order written: each <content>,
		// <number> and <letter>.
		std::vector<WrittenTextPiece> readTextPieces(const XmlNode& shape, const std::string& where)
		{
			std::vector<WrittenTextPiece> pieces;
			for (std::unique_ptr<XmlNode> child = shape.getFirstChild(); child != nullptr; child = child->getNextSibling())
			{
				const std::string tag = child->getName();
				WrittenTextPiece piece;
				if (tag == "content")
				{
					piece.words = readText(*child);
				}
				else if (tag == "number" || tag == "letter")
				{
					piece.kind = tag == "number" ? WrittenTextPiece::Kind::Number : WrittenTextPiece::Kind::Letter;
					piece.value = readValue(*child, where + " > <" + tag + ">");
				}
				else if (tag == "choose")
				{
					// <by> which, then its <word>s, the first 0.
					const std::string here = where + " > <choose>";
					piece.kind = WrittenTextPiece::Kind::Choice;
					piece.value = readValue(*requireChild(*child, "by", here), here);
					for (std::unique_ptr<XmlNode> word = child->getFirstChild(); word != nullptr; word = word->getNextSibling())
					{
						if (word->getName() == "word") { piece.choices.push_back(readText(*word)); }
					}
				}
				else
				{
					continue;
				}
				pieces.push_back(std::move(piece));
			}
			return pieces;
		}

		// A bitmap's <color char="w">s: a character and the color it draws,
		// each in place of an earlier one of the same character. One with no
		// char= is the color of the '*'s, read with the other shapes' <color>.
		void readBitmapColors(const XmlNode& shape, const std::string& where, std::vector<std::pair<char, std::string>>& colors)
		{
			bool plain = false;
			std::set<char> given;
			for (std::unique_ptr<XmlNode> node = shape.getFirstChild(); node != nullptr; node = node->getNextSibling())
			{
				if (node->getName() != "color") { continue; }
				const std::string character = node->getAttribute("char");
				if (character.empty())
				{
					if (plain) { fail(where, "has two <color>s with no char=; the one with none is the color of the '*'s"); }
					plain = true;
					continue;
				}
				if (character.size() != 1) { fail(where, "<color char=\"" + character + "\">: char= is one character"); }
				if (character == "." || character == "*")
				{
					fail(where, "<color char=\"" + character + "\">: '.' is always clear and '*' is the <color> with no char=; give another character");
				}
				if (!given.insert(character[0]).second) { fail(where, "has two <color char=\"" + character + "\">s"); }

				const auto same = std::find_if(colors.begin(), colors.end(), [&](const auto& each) { return each.first == character[0]; });
				if (same != colors.end()) { same->second = readText(*node); }
				else { colors.emplace_back(character[0], readText(*node)); }
			}
		}

		// The <color> of a shape: the one with no char= (a bitmap's others are
		// its other colors, readBitmapColors).
		std::unique_ptr<XmlNode> findPlainColor(const XmlNode& shape)
		{
			for (std::unique_ptr<XmlNode> node = shape.getFirstChild(); node != nullptr; node = node->getNextSibling())
			{
				if (node->getName() == "color" && node->getAttribute("char").empty()) { return node; }
			}
			return nullptr;
		}


		// Where on a sheet a <cell sheet="..."> is: its <column> and <row>
		// (from 1, each 1 when left out), or all four of <x>, <y>, <width>
		// and <height>.
		void readCell(const XmlNode& cell, WrittenSprite& sprite, const std::string& here)
		{
			sprite.sheet = cell.getAttribute("sheet");
			const auto x = findChild(&cell, "x");
			const auto y = findChild(&cell, "y");
			const auto width = findChild(&cell, "width");
			const auto height = findChild(&cell, "height");
			if (x || y || width || height)
			{
				if (!(x && y && width && height)) { fail(here, "takes a part of the sheet with all four of <x>, <y>, <width> and <height>, or a <column> and <row>"); }
				if (findChild(&cell, "column") || findChild(&cell, "row")) { fail(here, "has a <column> or <row> and a part by <x> and <y>; give one"); }
				sprite.svgRegion = { readValue(*x, here), readValue(*y, here), readValue(*width, here), readValue(*height, here) };
				return;
			}
			if (auto column = findChild(&cell, "column")) { sprite.column = readValue(*column, here); }
			if (auto row = findChild(&cell, "row")) { sprite.row = readValue(*row, here); }
		}

		// A place on a sheet: a <cell> (a <column> and <row>, or
		// <x>, <y>, <width> and <height>), plain numbers.
		SheetCell readSheetCell(const XmlNode& node, const std::string& here)
		{
			SheetCell cell;
			const auto number = [&](const char* tag, float fallback)
			{
				const auto part = findChild(&node, tag);
				if (!part) { return fallback; }
				const std::string text = readText(*part);
				std::size_t used = 0;
				float value = -1.0f;
				try { value = std::stof(text, &used); }
				catch (const std::exception&) { used = 0; }
				if (text.empty() || used != text.size() || value < 0.0f) { fail(here + " > <" + tag + ">", "is \"" + text + "\"; expected a number from 0"); }
				return value;
			};
			if (findChild(&node, "x") || findChild(&node, "y") || findChild(&node, "width") || findChild(&node, "height"))
			{
				if (!(findChild(&node, "x") && findChild(&node, "y") && findChild(&node, "width") && findChild(&node, "height")))
				{
					fail(here, "takes a part of the sheet with all four of <x>, <y>, <width> and <height>, or a <column> and <row>");
				}
				cell.free = true;
				cell.x = number("x", 0.0f);
				cell.y = number("y", 0.0f);
				cell.width = number("width", 0.0f);
				cell.height = number("height", 0.0f);
				return cell;
			}
			cell.column = static_cast<int>(number("column", 1.0f));
			cell.row = static_cast<int>(number("row", 1.0f));
			return cell;
		}

		// A run of cells on a sheet, <cells>: from its <column> and <row>
		// (each 1 when left out) <count> cells (1 when left out) along the
		// row, or down the column with <down />: an animation's frames.
		std::vector<SheetCell> readSheetRun(const XmlNode& node, const std::string& here)
		{
			const SheetCell start = readSheetCell(node, here);
			if (start.free) { fail(here, "is a run of a sheet's cells, from a <column> and <row>; a part by <x> and <y> is a <cell>"); }
			const auto count = findChild(&node, "count");
			const int many = count ? readCount(*count, here + " > <count>") : 1;
			const bool down = findChild(&node, "down") != nullptr;
			std::vector<SheetCell> cells;
			for (int i = 0; i < many; ++i)
			{
				SheetCell cell = start;
				(down ? cell.row : cell.column) += i;
				cells.push_back(cell);
			}
			return cells;
		}

		// One <circle>, <rectangle>, <text>, <image>, <bitmap> or <svg>, written into `sprite`.
		void readShape(const XmlNode& shape, WrittenSprite& sprite, const std::string& where)
		{
			const std::string kind = shape.getName();
			const std::string here = where + " > <" + kind + ">";

			sprite.kind = kind;

			if (kind == "circle")
			{
				sprite.radius = readValueOf(shape, "radius", here);
			}
			else if (kind == "rectangle")
			{
				sprite.width = readValueOf(shape, "width", here);
				sprite.height = readValueOf(shape, "height", here);
			}
			else if (kind == "bitmap")
			{
				// A picture in rows of text, '.' clear and '*' solid. Which
				// characters are allowed, and that the rows are one width, are
				// checked when it is drawn (picture/bitmap.cpp), with the row named.
				for (std::unique_ptr<XmlNode> row = shape.getFirstChild(); row != nullptr; row = row->getNextSibling())
				{
					if (row->getName() == "row") { sprite.bitmapRows.push_back(readText(*row)); }
				}
				if (sprite.bitmapRows.empty()) { fail(here, "needs at least one <row>"); }

				if (auto scale = findChild(&shape, "scale"))
				{
					sprite.scale = readValue(*scale, here);
				}
				readBitmapColors(shape, here, sprite.bitmapColors);
				sprite.flip = readPictureFlip(shape, here);
				sprite.style = shape.getAttribute("style");
			}
			else if (kind == "svg")
			{
				// A drawing in an SVG file, made into a picture when the game
				// loads (picture/svg.cpp): the file, optionally the part of it to take
				// and how many pixels a unit of it is, and elements to leave out.
				sprite.path = readText(*requireChild(shape, "path", here));

				const auto x = findChild(&shape, "x");
				const auto y = findChild(&shape, "y");
				const auto width = findChild(&shape, "width");
				const auto height = findChild(&shape, "height");
				if (x || y || width || height)
				{
					if (!(x && y && width && height)) { fail(here, "takes a part of the drawing with all four of <x>, <y>, <width> and <height>, or none of them"); }

					sprite.svgRegion = { readValue(*x, here), readValue(*y, here), readValue(*width, here), readValue(*height, here) };
				}

				if (auto scale = findChild(&shape, "scale"))
				{
					sprite.scale = readValue(*scale, here);
				}

				for (std::unique_ptr<XmlNode> hide = shape.getFirstChild(); hide != nullptr; hide = hide->getNextSibling())
				{
					if (hide->getName() == "hide") { sprite.svgHide.push_back(readText(*hide)); }
				}
				sprite.flip = readPictureFlip(shape, here);
			}
			else if (kind == "cell")
			{
				// A picture cut from one of the game's <sheet>s: a cell of
				// its grid, or a part of it by <x>, <y>, <width> and <height>.
				readCell(shape, sprite, here);
				if (auto scale = findChild(&shape, "scale"))
				{
					sprite.scale = readValue(*scale, here);
				}
				sprite.flip = readPictureFlip(shape, here);
			}
			else if (kind == "tether")
			{
				// A line joining two objects' middles, wherever they go.
				sprite.tetherFrom = shape.getAttribute("from");
				sprite.tetherTo = shape.getAttribute("to");
				if (sprite.tetherFrom.empty() || sprite.tetherTo.empty()) { fail(here, "names the two objects it joins, from=\"...\" and to=\"...\""); }
				if (auto thickness = findChild(&shape, "thickness")) { sprite.thickness = readValue(*thickness, here); }
			}
			else if (kind == "text")
			{
				// No pieces is for a group whose members, rows or cells give
				// them; a picture made with none stops the load (buildPicture).
				sprite.text = readTextPieces(shape, here);

				sprite.style = shape.getAttribute("style");
				if (auto size = findChild(&shape, "size")) { sprite.size = readValue(*size, here); }
				else if (sprite.style.empty()) { fail(here, "needs a <size>, or a style= whose <style> has one"); }
				if (auto font = findChild(&shape, "font")) { sprite.font = readText(*font); }
			}
			else
			{
				sprite.path = readText(*requireChild(shape, "path", here));
				if (auto flip = findChild(&shape, "flip")) { sprite.flip = readText(*flip); }
			}

			if (kind != "image" && kind != "svg" && kind != "cell")
			{
				if (auto color = findPlainColor(shape)) { sprite.color = readText(*color); }
			}
		}

		// One <line>: where it goes from and to, and optionally its color and
		// how many pixels thick it is.
		WrittenLine readLine(const XmlNode& line, const std::string& where)
		{
			const std::string here = where + " > <line>";
			WrittenLine writtenLine;

			writtenLine.from = readVector2(*requireChild(line, "from", here), here);
			writtenLine.to = readVector2(*requireChild(line, "to", here), here);

			if (auto color = findChild(&line, "color")) { writtenLine.color = readText(*color); }
			if (auto thickness = findChild(&line, "thickness"))
			{
				writtenLine.thickness = readValue(*thickness, here);
			}

			return writtenLine;
		}

		// A <surface>: its <drag> and <grip>, each if given.
		void readSurface(const XmlNode& surface, WrittenObject& writtenObject, const std::string& where)
		{
			const std::string here = where + " > <surface>";
			writtenObject.hasSurface = true;
			for (std::unique_ptr<XmlNode> child = surface.getFirstChild(); child != nullptr; child = child->getNextSibling())
			{
				if (child->getName() == "drag") { writtenObject.writtenSurfaceDrag = readValue(*child, here); }
				else { writtenObject.writtenSurfaceGrip = readValue(*child, here); }
			}
		}

		// One <polygon>: its <point>s, three or more, and optionally its color.
		WrittenPolygon readPolygon(const XmlNode& polygon, const std::string& where)
		{
			const std::string here = where + " > <polygon>";
			WrittenPolygon writtenPolygon;

			for (std::unique_ptr<XmlNode> node = polygon.getFirstChild(); node != nullptr; node = node->getNextSibling())
			{
				if (node->getName() == "point") { writtenPolygon.points.push_back(readVector2(*node, here + " > <point>")); }
				else { writtenPolygon.color = readText(*node); }
			}

			return writtenPolygon;
		}

		// The tag read; readSprite without the line.
		WrittenSprite readSpriteHere(const XmlNode& spriteNode, const std::string& where);

		WrittenSprite readSprite(const XmlNode& spriteNode, const std::string& where)
		{
			return onLineOf(spriteNode, [&] { return readSpriteHere(spriteNode, where); });
		}

		WrittenSprite readSpriteHere(const XmlNode& spriteNode, const std::string& where)
		{
			const std::string here = where + " > <sprite>";
			WrittenSprite sprite;
			sprite.name = spriteNode.getAttribute("name");

			std::unique_ptr<XmlNode> first = spriteNode.getFirstChild();
			// An empty <sprite name="x" />: a look that draws nothing, for an object
			// that only keeps where something is up to, or a member that is not
			// seen in a look its group has.
			if (!first)
			{
				WrittenSprite nothing = nothingSprite();
				nothing.name = sprite.name;
				return nothing;
			}

			if (first->getName() == "line")
			{
				// A drawing: every <line> of the sprite, drawn in the order written.
				sprite.kind = "line";

				for (std::unique_ptr<XmlNode> node = std::move(first); node != nullptr; node = node->getNextSibling())
				{
					sprite.lines.push_back(onLineOf(*node, [&] { return readLine(*node, here); }));
				}
			}
			else if (first->getName() == "polygon")
			{
				// Filled shapes: every <polygon> of the sprite, drawn in the order written.
				sprite.kind = "polygon";

				for (std::unique_ptr<XmlNode> node = std::move(first); node != nullptr; node = node->getNextSibling())
				{
					sprite.polygons.push_back(onLineOf(*node, [&] { return readPolygon(*node, here); }));
				}
			}
			else if (isShapeTag(first->getName()))
			{
				onLineOf(*first, [&] { readShape(*first, sprite, here); });
			}
			else
			{
				fail(here, "unknown shape <" + first->getName() + ">");
			}

			return sprite;
		}

		// A <sprite> that changes one a group already has, for a member, row,
		// column or cell of it. Of the same shape it need give only what
		// changes (a <color>, say) and keeps the rest; of another shape, or as
		// a drawing of lines, it is a sprite of its own and must be complete.
		WrittenSprite changeSprite(const XmlNode& spriteNode, const WrittenSprite& base, const std::string& where)
		{
			const std::unique_ptr<XmlNode> shape = spriteNode.getFirstChild();
			if (!shape || shape->getName() != base.kind || base.kind == "line" || base.kind == "polygon" || base.kind == "cell") { return readSprite(spriteNode, where); }

			const std::string here = where + " > <sprite> > <" + base.kind + ">";
			WrittenSprite sprite = base;

			const auto change = [&](const char* tag, WrittenValue& value)
			{
				if (auto node = findChild(shape.get(), tag)) { value = readValue(*node, here); }
			};

			if (base.kind == "circle")
			{
				change("radius", sprite.radius);
			}
			else if (base.kind == "rectangle")
			{
				change("width", sprite.width);
				change("height", sprite.height);
			}
			else if (base.kind == "tether")
			{
				// The two it joins are named again; only the line's look carries over.
				sprite.tetherFrom = shape->getAttribute("from");
				sprite.tetherTo = shape->getAttribute("to");
				if (sprite.tetherFrom.empty() || sprite.tetherTo.empty()) { fail(here, "names the two objects it joins, from=\"...\" and to=\"...\""); }
				change("thickness", sprite.thickness);
			}
			else if (base.kind == "text")
			{
				// Pieces given are the whole text; none, and it is kept.
				if (std::vector<WrittenTextPiece> text = readTextPieces(*shape, here); !text.empty())
				{
					sprite.text = std::move(text);
				}
				change("size", sprite.size);
				if (auto font = findChild(shape.get(), "font")) { sprite.font = readText(*font); }
				if (const std::string style = shape->getAttribute("style"); !style.empty()) { sprite.style = style; }
			}
			else if (base.kind == "image")
			{
				if (auto path = findChild(shape.get(), "path")) { sprite.path = readText(*path); }
				if (auto flip = findChild(shape.get(), "flip")) { sprite.flip = readText(*flip); }
			}
			else if (base.kind == "bitmap" || base.kind == "svg")
			{
				// New <row>s are a new picture, all of it; the same for <hide>s.
				std::vector<std::string> rows;
				std::vector<std::string> hides;
				for (std::unique_ptr<XmlNode> node = shape->getFirstChild(); node != nullptr; node = node->getNextSibling())
				{
					if (node->getName() == "row") { rows.push_back(readText(*node)); }
					else if (node->getName() == "hide") { hides.push_back(readText(*node)); }
				}
				if (!rows.empty()) { sprite.bitmapRows = std::move(rows); }
				if (!hides.empty()) { sprite.svgHide = std::move(hides); }

				if (auto path = findChild(shape.get(), "path")) { sprite.path = readText(*path); }

				const auto x = findChild(shape.get(), "x");
				const auto y = findChild(shape.get(), "y");
				const auto width = findChild(shape.get(), "width");
				const auto height = findChild(shape.get(), "height");
				if (x || y || width || height)
				{
					// Changing the part taken may give only what changes; taking
					// a part of what was the whole drawing needs all four.
					if (!sprite.svgRegion && !(x && y && width && height)) { fail(here, "takes a part of the drawing with all four of <x>, <y>, <width> and <height>, or none of them"); }

					WrittenSprite::Region& region = sprite.svgRegion ? *sprite.svgRegion : sprite.svgRegion.emplace();
					if (x) { region.x = readValue(*x, here); }
					if (y) { region.y = readValue(*y, here); }
					if (width) { region.width = readValue(*width, here); }
					if (height) { region.height = readValue(*height, here); }
				}

				if (auto scale = findChild(shape.get(), "scale"))
				{
					sprite.scale = readValue(*scale, here);
				}
				if (base.kind == "bitmap")
				{
					readBitmapColors(*shape, here, sprite.bitmapColors);
					if (const std::string style = shape->getAttribute("style"); !style.empty()) { sprite.style = style; }
				}
				if (findChild(shape.get(), "flip")) { sprite.flip = readPictureFlip(*shape, here); }
			}

			if (base.kind != "image" && base.kind != "svg")
			{
				if (auto color = findPlainColor(*shape)) { sprite.color = readText(*color); }
			}

			return sprite;
		}

		// The group's sprites as a member, row, column or cell (`node`) has
		// them: each changed by the <sprite> of its name that node gives (one
		// with no name changes the group's one with no name), and any sprite
		// node names that the group has not, added. The names of those it gave
		// go in `given`.
		std::vector<WrittenSprite> changeSprites(const std::vector<WrittenSprite>& base, const XmlNode& node, const std::string& where,
			std::set<std::string>* given = nullptr)
		{
			std::vector<WrittenSprite> sprites = base;
			std::set<std::string> names;

			for (std::unique_ptr<XmlNode> child = node.getFirstChild(); child != nullptr; child = child->getNextSibling())
			{
				if (child->getName() != "sprite") { continue; }

				const std::string name = child->getAttribute("name");
				if (!names.insert(name).second) { fail(where, name.empty() ? "has two <sprite>s with no name" : "has two <sprite>s called \"" + name + "\""); }

				const auto found = std::find_if(sprites.begin(), sprites.end(), [&](const WrittenSprite& sprite) { return sprite.name == name; });
				if (found != sprites.end()) { *found = changeSprite(*child, *found, where); }
				else { sprites.push_back(readSprite(*child, where)); }
			}

			if (given) { given->insert(names.begin(), names.end()); }
			return sprites;
		}

		// Every <sprite> child of `parent`, in the order written.
		std::vector<WrittenSprite> readSprites(const XmlNode& parent, const std::string& where)
		{
			std::vector<WrittenSprite> sprites;

			for (std::unique_ptr<XmlNode> node = parent.getFirstChild(); node != nullptr; node = node->getNextSibling())
			{
				if (node->getName() == "sprite") { sprites.push_back(readSprite(*node, where)); }
			}

			return sprites;
		}

		std::vector<WrittenCommand> readCommands(std::unique_ptr<XmlNode> first, const std::string& where);
		WrittenHitbox readHitbox(const XmlNode& hitbox, const std::string& here);

		// An <animation> as written (WrittenFrames). Which sprites its frames are
		// is settled by chooseSprite, once the sprites it can pick from are
		// known (a group's member can bring its own).
		using AnimationSpec = WrittenFrames;

		AnimationSpec readAnimation(const XmlNode& animation, const std::string& where)
		{
			AnimationSpec spec;
			spec.name = animation.getAttribute("name");
			const std::string here = where + " > <animation" + (spec.name.empty() ? std::string{} : " name=\"" + spec.name + "\"") + ">";

			spec.interval = readValueOf(animation, "seconds", here);

			// A frame cut from a sheet: a sprite of its own, named for the
			// animation and numbered.
			const auto addCell = [&](WrittenSprite sprite, const XmlNode& node, const std::string& at)
			{
				sprite.kind = "cell";
				sprite.name = (spec.name.empty() ? std::string("frame") : spec.name) + "." + std::to_string(spec.cells.size() + 1);
				if (auto scale = findChild(&node, "scale"))
				{
					sprite.scale = readValue(*scale, at);
				}
				sprite.flip = readPictureFlip(node, at);
				spec.frames.push_back(sprite.name);
				spec.cells.push_back(std::move(sprite));
			};

			for (std::unique_ptr<XmlNode> node = animation.getFirstChild(); node != nullptr; node = node->getNextSibling())
			{
				const std::string tag = node->getName();
				if (tag == "frame")
				{
					const std::string at = here + " > <frame>";
					spec.frames.push_back(node->getAttribute("sprite"));
					if (auto seconds = findChild(node.get(), "seconds"))
					{
						spec.times.resize(spec.frames.size());
						spec.times.back() = readValue(*seconds, at + " > <seconds>");
					}
					std::unique_ptr<XmlNode> part = node->getFirstChild();
					if (part != nullptr && part->getName() == "seconds") { part = part->getNextSibling(); }
					for (; part != nullptr && part->getName() == "hitbox"; part = part->getNextSibling())
					{
						spec.frameHitboxes.resize(spec.frames.size());
						spec.frameHitboxes.back().push_back(readHitbox(*part, at + " > <hitbox>"));
					}
					// Then what it does as it comes up: a march's note.
					if (part != nullptr)
					{
						spec.frameCommands.resize(spec.frames.size());
						spec.frameCommands.back() = readCommands(std::move(part), at);
					}
				}
				else if (tag == "cell")
				{
					WrittenSprite sprite;
					readCell(*node, sprite, here + " > <cell>");
					addCell(std::move(sprite), *node, here + " > <cell>");
				}
				else if (tag == "cells")
				{
					const std::string at = here + " > <cells>";
					for (const SheetCell& cell : readSheetRun(*node, at))
					{
						WrittenSprite sprite;
						sprite.sheet = requireAttribute(*node, "sheet", at);
						sprite.column = WrittenValue::expression(std::to_string(cell.column));
						sprite.row = WrittenValue::expression(std::to_string(cell.row));
						addCell(std::move(sprite), *node, at);
					}
				}
				else if (tag == "once") { spec.once = true; }
				else if (tag == "stepped") { spec.stepped = true; }
				else if (tag == "then")
				{
					if (!spec.once) { fail(here + " > <then>", "comes when the animation ends, so it needs <once />"); }
					spec.thenSprite = node->getAttribute("sprite");
					spec.thenCommands = readCommands(node->getFirstChild(), here + " > <then>");
				}
			}
			if (!spec.times.empty()) { spec.times.resize(spec.frames.size()); }
			if (!spec.frameHitboxes.empty()) { spec.frameHitboxes.resize(spec.frames.size()); }
			if (!spec.frameCommands.empty()) { spec.frameCommands.resize(spec.frames.size()); }

			return spec;
		}

		// All of `given` added to `animations`, each in place of the one of
		// its name (the unnamed one in place of the unnamed one).
		void mergeAnimations(std::vector<AnimationSpec>& animations, const std::vector<AnimationSpec>& given)
		{
			for (const AnimationSpec& animation : given)
			{
				const auto same = std::find_if(animations.begin(), animations.end(), [&](const AnimationSpec& each) { return each.name == animation.name; });
				if (same == animations.end()) { animations.push_back(animation); }
				else { *same = animation; }
			}
		}

		// Every <animation> of `node`, in the order written; two of one name
		// (or two with none) stop the load.
		std::vector<AnimationSpec> readAnimations(const XmlNode& node, const std::string& where)
		{
			std::vector<AnimationSpec> animations;
			for (std::unique_ptr<XmlNode> child = node.getFirstChild(); child != nullptr; child = child->getNextSibling())
			{
				if (child->getName() != "animation") { continue; }
				AnimationSpec animation = readAnimation(*child, where);
				if (std::any_of(animations.begin(), animations.end(), [&](const AnimationSpec& each) { return each.name == animation.name; }))
				{
					fail(where, animation.name.empty() ? std::string("has two <animation>s; it has one, or each is named as a look") : "two <animation>s are called \"" + animation.name + "\"");
				}
				animations.push_back(std::move(animation));
			}
			return animations;
		}

		// Settles what `object` looks like from the <sprite>s it has and its
		// <animation>s, if any. One sprite and no animation is the ordinary
		// case. With one unnamed animation, each <frame> names one of the
		// sprites, and the object is the first of them until it moves on. With
		// named animations, each is a look that moves (lookWithAnimations).
		// Anything that cannot be settled, such as several sprites and nothing
		// to say which is shown when, or a frame naming a sprite that is not
		// there, stops the load.
		void chooseSprite(const std::vector<WrittenSprite>& written, const std::vector<AnimationSpec>& animations,
			const std::string& where, WrittenObject& object)
		{
			// The sprites written, and the frames the animations cut from sheets.
			std::vector<WrittenSprite> sprites = written;
			for (const AnimationSpec& animation : animations) { sprites.insert(sprites.end(), animation.cells.begin(), animation.cells.end()); }
			if (sprites.empty()) { fail(where, "missing <sprite>"); }

			const bool named = std::any_of(animations.begin(), animations.end(), [](const AnimationSpec& each) { return !each.name.empty(); });
			if (named)
			{
				if (std::any_of(animations.begin(), animations.end(), [](const AnimationSpec& each) { return each.name.empty(); }))
				{
					fail(where, "has an <animation> with no name beside named ones; name it, as a look that moves, or leave the names off for one animation of the whole object");
				}
				lookWithAnimations(object, sprites, animations, where);
				return;
			}
			if (animations.size() > 1) { fail(where, "has two <animation>s; it has one, or each is named as a look"); }
			const std::optional<AnimationSpec> animation = animations.empty() ? std::nullopt : std::optional<AnimationSpec>(animations.front());

			for (std::size_t i = 0; i < sprites.size(); ++i)
			{
				if (sprites[i].name.empty()) { continue; }

				for (std::size_t j = i + 1; j < sprites.size(); ++j)
				{
					if (sprites[j].name == sprites[i].name) { fail(where, "two <sprite>s are called \"" + sprites[i].name + "\""); }
				}
			}

			if (!animation)
			{
				// Several sprites and no animation: the object's looks, which
				// <become> switches between; it starts as the first. Each needs a
				// name to be asked for by.
				if (sprites.size() > 1)
				{
					for (const WrittenSprite& sprite : sprites)
					{
						if (sprite.name.empty())
						{
							fail(where, "has " + std::to_string(sprites.size()) + " <sprite>s and no <animation>, so they are looks for <become>, and every one needs a name");
						}
					}
					object.looks = sprites;
				}

				object.sprite = sprites.front();
				return;
			}

			if (animation->frames.size() < (animation->once ? 1u : 2u)) { fail(where, "<animation> needs at least two <frame>s (one, with <once />)"); }

			WrittenAnimation chosen;
			chosen.interval = animation->interval;
			chosen.times = animation->times;
			chosen.frameHitboxes = animation->frameHitboxes;
			chosen.frameCommands = animation->frameCommands;
			chosen.once = animation->once;
			chosen.stepped = animation->stepped;
			chosen.thenCommands = animation->thenCommands;
			if (!animation->thenSprite.empty())
			{
				fail(where + " > <animation> > <then sprite=\"" + animation->thenSprite + "\">", "an object's one animation has no looks to go to; name the animation to make it a look");
			}

			std::vector<bool> used(sprites.size(), false);
			for (const std::string& name : animation->frames)
			{
				const auto found = std::find_if(sprites.begin(), sprites.end(), [&](const WrittenSprite& sprite) { return sprite.name == name; });
				if (found == sprites.end()) { fail(where, "<animation> > <frame sprite=\"" + name + "\">: no <sprite name=\"" + name + "\"> to show"); }

				used[static_cast<std::size_t>(found - sprites.begin())] = true;
				chosen.frames.push_back(*found);
			}

			for (std::size_t i = 0; i < sprites.size(); ++i)
			{
				if (!used[i])
				{
					fail(where, sprites[i].name.empty() ? "a <sprite> with no name is never shown by the <animation>"
						: "<sprite name=\"" + sprites[i].name + "\"> is never shown by the <animation>");
				}
			}

			object.hasAnimation = true;
			object.sprite = chosen.frames.front();
			object.animation = std::move(chosen);
		}

		// An object's <pull>s and its <seek>, each naming what with object=.
		// A <light>: how far it reaches, then a <past class> for each class
		// it does not shine through.
		void readLight(const XmlNode& light, WrittenObject& writtenObject, const std::string& where)
		{
			const std::string here = where + " > <light>";
			writtenObject.lightPast.clear();
			for (std::unique_ptr<XmlNode> past = light.getFirstChild(); past != nullptr; past = past->getNextSibling())
			{
				if (past->getName() == "past") { writtenObject.lightPast.push_back(past->getAttribute("class")); }
			}
			writtenObject.writtenLight = writtenObject.lightPast.empty() ? readValue(light, here)
				: readLeadValue(light, { "past" }, "how far it reaches, then a <past class=\"...\" /> for each class it does not shine through", here);
		}

		// An <altitude>: its <start>, <rise>, <fall> and <bounce>, each if given.
		void readAltitude(const XmlNode& altitude, WrittenObject& writtenObject, const std::string& where)
		{
			const std::string here = where + " > <altitude>";
			writtenObject.hasAltitude = true;
			for (std::unique_ptr<XmlNode> child = altitude.getFirstChild(); child != nullptr; child = child->getNextSibling())
			{
				const std::string tag = child->getName();
				if (tag == "start") { writtenObject.writtenAltitude = readValue(*child, here); }
				else if (tag == "rise") { writtenObject.writtenRise = readValue(*child, here); }
				else if (tag == "fall") { writtenObject.writtenFall = readValue(*child, here); }
				else { writtenObject.writtenAltitudeBounce = readValue(*child, here); }
			}
		}


		// A plain number of a holder's rules: "-1", "13".
		float readPlainNumber(const XmlNode& node, const std::string& here)
		{
			const std::string text = readText(node);
			std::size_t used = 0;
			float value = 0.0f;
			try { value = std::stof(text, &used); }
			catch (const std::exception&) { used = 0; }
			if (text.empty() || used != text.size()) { fail(here, "is \"" + text + "\"; expected a number"); }
			return value;
		}

		// The rules of an <accepts> (or of its <any>, <wild> or <empty>):
		// <condition>s on the arriving one, which is self. Each compares a
		// variable with <equal>, <unequal>, <above>, <below>, <atleast> or
		// <atmost> a number, or (on a holder holding something) with a
		// variable of the top one, plus or minus a number:
		// <equal>top.suit</equal>, <unequal>top.color</unequal>,
		// <equal>top.rank - 1</equal>, <below>top.worth</below>. total="sum" adds up that variable over
		// what is laid at once; total="count" with <equal>1</equal> lays one
		// at a time.
		void readAcceptRules(const XmlNode& node, HolderRule& rule, std::vector<HolderRule::VarRule>& into, const std::string& here, int level)
		{
			// level 0: an <accepts>; 1: an <any> in it; 2: an <empty> or a <wild>, where nothing lies under it.
			using Kind = HolderRule::VarRule::Kind;
			const bool top = level == 0;
			for (std::unique_ptr<XmlNode> part = node.getFirstChild(); part != nullptr; part = part->getNextSibling())
			{
				const std::string name = part->getName();
				const std::string at = here + " > <" + name + ">";
				if (top && name == "empty") { readAcceptRules(*part, rule, rule.emptyRules, at, 2); continue; }
				if (top && name == "any")
				{
					readAcceptRules(*part, rule, rule.anyRules, at, 1);
					continue;
				}
				if (top && name == "wild")
				{
					rule.wildVariable = part->getAttribute("variable");
					readAcceptRules(*part, rule, rule.wildRules, at, 2);
					continue;
				}
				const std::unique_ptr<XmlNode> test = part->getFirstChild();
				const std::string kind = test->getName();
				HolderRule::VarRule each;
				each.kind = kind == "equal" ? Kind::Equal : kind == "unequal" ? Kind::Unequal : kind == "above" ? Kind::Above
					: kind == "below" ? Kind::Below : kind == "atleast" ? Kind::AtLeast : Kind::AtMost;
				const std::string testAt = at + " > <" + kind + ">";
				const std::string total = part->getAttribute("total");
				each.variable = part->getAttribute("variable");
				if (total == "count")
				{
					if (!top) { fail(at, "total=\"count\" says how many are laid at once, so it stands in the <accepts> itself"); }
					if (!each.variable.empty()) { fail(at, "total=\"count\" counts what is laid, so it reads no variable="); }
					if (kind != "equal" || readPlainNumber(*test, testAt) != 1.0f) { fail(at, "total=\"count\" takes <equal>1</equal> for now: one at a time"); }
					rule.one = true;
					continue;
				}
				if (each.variable.empty()) { fail(at, "needs variable=\"...\": the arriving one's variable it compares"); }
				if (total == "sum")
				{
					if (level > 1) { fail(at, "total=\"sum\" adds up what is laid with the top, so it is not in an <empty> or a <wild>"); }
					if (kind != "equal") { fail(at, "total=\"sum\" takes an <equal> number for now"); }
					each.kind = Kind::Total;
					each.amount = readPlainNumber(*test, testAt);
					into.push_back(std::move(each));
					continue;
				}

				// top.<variable>, or <add> or <subtract> of it and a number: the one
				// it would go on, as <equal><add><value>top.rank</value><value>1</value></add></equal>.
				std::string text = readText(*test);
				float shift = 0.0f;
				if (std::unique_ptr<XmlNode> by = test->getFirstChild())
				{
					const std::string how = by->getName();
					const std::string byAt = testAt + " > <" + how + ">";
					std::unique_ptr<XmlNode> first = by->getFirstChild();
					std::unique_ptr<XmlNode> second = first ? first->getNextSibling() : nullptr;
					if ((how != "add" && how != "subtract") || by->getNextSibling() || !text.empty() || !second || second->getNextSibling() || first->getName() != "value" || second->getName() != "value")
					{
						fail(testAt, "expected top. and a variable of it, or an <add> or <subtract> of two <value>s, top.rank and a number");
					}
					text = readText(*first);
					if (text.rfind("top.", 0) != 0) { fail(byAt, "adds to or takes from top. and a variable of it, as top.rank"); }
					shift = (how == "subtract" ? -1.0f : 1.0f) * readPlainNumber(*second, byAt);
				}
				if (text.rfind("top.", 0) == 0)
				{
					if (level > 1) { fail(testAt, "names top, but in an <empty> or a <wild> nothing lies under it"); }
					const std::string named = text.substr(4);
					if (!isName(named, false)) { fail(testAt, "is \"" + text + "\"; expected top. and a variable of it, as top.rank"); }
					each.top = true;
					if (named != each.variable) { each.topVariable = named; }
					each.amount = shift;
				}
				else
				{
					each.amount = readPlainNumber(*test, testAt);
				}
				into.push_back(std::move(each));
			}
		}

		// A <score patterns>: its <melds />, <with object> and <best>.
		void readScore(const XmlNode& node, WrittenObject::WrittenHolder& holder, const std::string& at)
		{
			holder.score = node.getAttribute("patterns");
			for (std::unique_ptr<XmlNode> part = node.getFirstChild(); part != nullptr; part = part->getNextSibling())
			{
				if (part->getName() == "with")
				{
					holder.scoreWith = part->getAttribute("object");
					if (auto full = findChild(part.get(), "full")) { holder.scoreFull = readCount(*full, at + " > <with> > <full>"); }
				}
				else if (part->getName() == "best") { holder.scoreBest = readCount(*part, at + " > <best>"); }
				else { holder.melds = holder.score; }
			}
			// <melds />: scored by splitting the cards into groups, not by the best pattern among them.
			if (!holder.melds.empty())
			{
				if (!holder.scoreWith.empty()) { fail(at, "has <melds /> and <with>; melds are a holder's own cards"); }
				holder.meldsBest = holder.scoreBest;
				holder.score.clear();
				holder.scoreBest = 0;
			}
		}

		// A <holds class sprite>: the class of what it holds and the look
		// they show laid there, then its <spread>s, <accepts>, <run />,
		// <top sprite>, <lift> and <picked>. Read into the holder it is
		// (Object::Holder) as written; the looks it names are checked once
		// the members they belong to are known (load/check.cpp).
		void readHolds(const XmlNode& node, WrittenObject::WrittenHolder& holder, const std::string& here)
		{
			holder.is = true;
			holder.holdsClass = node.getAttribute("class");
			holder.holdsSprite = node.getAttribute("sprite");
			const auto halves = [&](const XmlNode& vector, WrittenValue& x, WrittenValue& y, const std::string& at)
			{
				if (auto across = findChild(&vector, "x")) { x = readValue(*across, at); }
				if (auto down = findChild(&vector, "y")) { y = readValue(*down, at); }
			};
			for (std::unique_ptr<XmlNode> child = node.getFirstChild(); child != nullptr; child = child->getNextSibling())
			{
				const std::string tag = child->getName();
				const std::string at = here + " > <" + tag + ">";
				if (tag == "spread")
				{
					const std::string look = child->getAttribute("sprite");
					if (look.empty())
					{
						halves(*child, holder.spreadX, holder.spreadY, at);
						if (auto width = findChild(child.get(), "width")) { holder.spreadWidth = readValue(*width, at + " > <width>"); }
					}
					else
					{
						if (!holder.spreadSprite.empty()) { fail(at, "is a second <spread sprite>; a holder has one plain <spread> and one for a look"); }
						holder.spreadSprite = look;
						halves(*child, holder.lookSpreadX, holder.lookSpreadY, at);
					}
				}
				else if (tag == "accepts")
				{
					holder.takes.given = true;
					readAcceptRules(*child, holder.takes, holder.takes.rules, at, 0);
				}
				else if (tag == "carry") { holder.carry = true; }
				else if (tag == "counts")
				{
					// What its members are worth added: a variable of each,
					// <atmost> a limit a member's `high` stays within.
					holder.counts = child->getAttribute("variable");
					holder.hasPoints = true;
					if (auto most = child->getFirstChild()) { holder.pointsMost = readValue(*most, at + " > <atmost>"); }
				}
				else if (tag == "top") { holder.topSprite = child->getAttribute("sprite"); }
				else if (tag == "lift") { holder.lift = readValue(*child, at); }
				else if (tag == "score") { readScore(*child, holder, at); }
				else if (tag == "trick")
				{
					// A trick: any card leads, then the value led is followed;
					// <breaks> one that leads only once played, <trumps> one over the rest.
					HolderRule& rule = holder.takes;
					rule.trickBy = child->getAttribute("variable");
					rule.given = true;
					rule.follow = true;
					for (std::unique_ptr<XmlNode> part = child->getFirstChild(); part != nullptr; part = part->getNextSibling())
					{
						(part->getName() == "breaks" ? rule.breaks : rule.trumps).push_back(readText(*part));
					}
				}
				else if (tag == "order")
				{
					for (std::unique_ptr<XmlNode> by = child->getFirstChild(); by != nullptr; by = by->getNextSibling())
					{
						holder.orderBy.push_back(by->getAttribute("variable"));
					}
				}
				else if (tag == "picked")
				{
					// For now: a <send into>N (taking from it sends that many
					// there), and an <empty> with a <send from><all /> (and its
					// <leave>) and <order><random /></order>.
					for (std::unique_ptr<XmlNode> part = child->getFirstChild(); part != nullptr; part = part->getNextSibling())
					{
						const std::string name = part->getName();
						if (name == "send")
						{
							holder.pickedInto = part->getAttribute("into");
							holder.pickedSends = readLeadValue(*part, {}, "how many", at);
						}
						else if (name == "play") { holder.pickedSound = part->getAttribute("sound"); }
						else if (name == "empty")
						{
							for (std::unique_ptr<XmlNode> step = part->getFirstChild(); step != nullptr; step = step->getNextSibling())
							{
								if (step->getName() == "send")
								{
									holder.emptyFrom = step->getAttribute("from");
									if (auto leave = findChild(step.get(), "leave")) { holder.emptyLeave = readValue(*leave, at + " > <empty> > <send> > <leave>"); }
								}
								else { holder.emptyRandom = true; }
							}
						}
					}
				}
			}
		}

		// Every command from `first` on, in the order written.
		std::vector<WrittenCommand> readCommands(std::unique_ptr<XmlNode> first, const std::string& where)
		{
			std::vector<WrittenCommand> commands;

			for (std::unique_ptr<XmlNode> node = std::move(first); node != nullptr; node = node->getNextSibling())
			{
				commands.push_back(readCommand(*node, where));
			}

			return commands;
		}

		// A key's commands: as readCommands, but a <trigger> may hold an
		// <otherwise>, commands run only when its action was refused. They
		// follow the trigger in the list, which counts them (`otherwise`).
		std::vector<WrittenCommand> readKeyCommands(std::unique_ptr<XmlNode> first, const std::string& where)
		{
			std::vector<WrittenCommand> commands;
			for (std::unique_ptr<XmlNode> node = std::move(first); node != nullptr; node = node->getNextSibling())
			{
				std::unique_ptr<XmlNode> otherwise = node->getName() == "trigger" ? node->getFirstChild() : nullptr;
				if (!otherwise)
				{
					commands.push_back(readCommand(*node, where));
					continue;
				}
				const std::string here = where + " > <trigger action=\"" + node->getAttribute("action") + "\">";
				if (otherwise->getName() != "otherwise" || otherwise->getNextSibling()) { fail(here, "holds <" + otherwise->getName() + ">; a trigger holds one <otherwise> if wanted, run when its action is refused"); }
				WrittenCommand trigger;
				trigger.verb = "trigger";
				trigger.line = node->line();
				trigger.object = node->getAttribute("object");
				trigger.action = node->getAttribute("action");
				trigger.from = node->getAttribute("from");
				std::vector<WrittenCommand> instead = readCommands(otherwise->getFirstChild(), here + " > <otherwise>");
				if (instead.empty()) { fail(here + " > <otherwise>", "is empty; leave it out"); }
				trigger.own.emplace<WrittenTrigger>().otherwise = static_cast<int>(instead.size());
				commands.push_back(std::move(trigger));
				commands.insert(commands.end(), instead.begin(), instead.end());
			}
			return commands;
		}

		// <timers>: each <timer> an <every> or an <after>, then its commands.
		// An object's timer may follow one of its looks (sprite="blue"); a
		// state or a scene has no looks (`ofObject` false).
		std::vector<WrittenTimer> readTimers(const XmlNode* timersNode, const std::string& where, bool ofObject = true)
		{
			std::vector<WrittenTimer> timers;
			if (!timersNode) { return timers; }

			for (std::unique_ptr<XmlNode> timer = timersNode->getFirstChild(); timer != nullptr; timer = timer->getNextSibling())
			{
				const std::string here = where + " > <timer>";
				std::unique_ptr<XmlNode> when = timer->getFirstChild();

				WrittenTimer written;
				written.name = timer->getAttribute("name");
				if (!written.name.empty() && std::any_of(timers.begin(), timers.end(), [&](const WrittenTimer& other) { return other.name == written.name; }))
				{
					fail(here, "has two timers called '" + written.name + "'");
				}
				written.sprite = timer->getAttribute("sprite");
				if (!written.sprite.empty() && !ofObject)
				{
					fail(here, "sprite=\"" + written.sprite + "\": only an object's timer follows a look; a state or a scene has none");
				}
				written.repeat = when->getName() == "every";
				written.interval = readValue(*when, here);
				written.commands = readCommands(when->getNextSibling(), here);
				timers.push_back(std::move(written));
			}

			return timers;
		}

		// A <path>: its <speed>, maybe a <start>, then its <step>s and <home />s
		// in the order flown.
		WrittenPath readPath(const XmlNode& node)
		{
			WrittenPath path;
			path.name = node.getAttribute("name");
			const std::string where = "path '" + path.name + "'";

			// A <speed>, maybe a <start>, then one or more <step>s and <home />s (the schema).

			for (std::unique_ptr<XmlNode> child = node.getFirstChild(); child != nullptr; child = child->getNextSibling())
			{
				const std::string name = child->getName();
				if (name == "speed")
				{
					path.speed = readValue(*child, where + " > <speed>");
				}
				else if (name == "start")
				{
					path.start = readVector2(*child, where + " > <start>");
				}
				else if (name == "home")
				{
					WrittenPathStep step;
					step.home = true;
					path.steps.push_back(std::move(step));
				}
				else
				{
					const std::string here = where + " > <step>";
					WrittenPathStep step;
					step.x = readValueOf(*child, "x", here);
					step.y = readValueOf(*child, "y", here);

					// The commands come after the <x> and the <y>.
					std::unique_ptr<XmlNode> first = child->getFirstChild();
					while (first && (first->getName() == "x" || first->getName() == "y"))
					{
						first = first->getNextSibling();
					}
					step.commands = readCommands(std::move(first), here);
					path.steps.push_back(std::move(step));
				}
			}
			return path;
		}

		// A whole number of cells, maybe below 0, from <x> or <y>.
		int readCells(const XmlNode& node, const char* tag, const std::string& where)
		{
			const auto part = findChild(&node, tag);
			const std::string text = readText(*part);
			const std::size_t digits = text.size() > 0 && text[0] == '-' ? 1 : 0;
			if (text.size() <= digits || text.size() > digits + 3
				|| !std::all_of(text.begin() + static_cast<std::ptrdiff_t>(digits), text.end(), [](unsigned char c) { return std::isdigit(c) != 0; }))
			{
				fail(where, std::string("<") + tag + "> is \"" + text + "\"; expected a whole number of cells, such as -1");
			}
			return std::stoi(text);
		}

		// A whole number of at least 1 from that child of `node`.
		int readWholeChild(const XmlNode& node, const char* tag, const std::string& where)
		{
			const auto part = findChild(&node, tag);
			if (!part) { fail(where, std::string("missing <") + tag + ">"); }
			return readCount(*part, where);
		}

		// <route>: its cells in order. A <row number> or <column number> is
		// the cells <from> one <to> another along it, either way round; a
		// <start> or <end> (<column> and <row>) one cell, first or last.
		void readRoute(const XmlNode& node, WrittenObject::WrittenMoveSet& set, const std::string& where)
		{
			for (std::unique_ptr<XmlNode> part = node.getFirstChild(); part != nullptr; part = part->getNextSibling())
			{
				const std::string tag = part->getName();
				const std::string here = where + " > <" + tag + ">";
				if (tag == "start" || tag == "end")
				{
					if (tag == "start" && !set.route.empty()) { fail(here, "comes first: it is where the route begins"); }
					set.route.emplace_back(readWholeChild(*part, "column", here), readWholeChild(*part, "row", here));
					if (tag == "start") { set.routeStart = true; continue; }
					set.routeEnd = true;
					if (findChild(part.get(), "home")) { set.home = readWholeChild(*part, "home", here); }
					continue;
				}
				if (set.routeEnd) { fail(here, "comes after <end>, which is the route's last cell"); }
				const std::string text = requireAttribute(*part, "number", here);
				if (text.empty() || text.size() > 4 || !std::all_of(text.begin(), text.end(), [](unsigned char c) { return std::isdigit(c) != 0; }) || std::stoi(text) < 1)
				{
					fail(here, "number=\"" + text + "\": expected a whole number from 1");
				}
				const int line = std::stoi(text);
				const int from = readWholeChild(*part, "from", here);
				const int to = readWholeChild(*part, "to", here);
				const int way = to >= from ? 1 : -1;
				for (int at = from;; at += way)
				{
					if (tag == "row") { set.route.emplace_back(at, line); }
					else { set.route.emplace_back(line, at); }
					if (at == to) { break; }
				}
			}
			if (set.route.size() < 2) { fail(where, "goes through fewer than two cells"); }
		}

		// <moves>: a board game piece's steps and jumps, a set for each look
		// that has its own (sprite=) and one for the rest.
		void readPieceMoves(const XmlNode& node, WrittenObject& object, const std::string& where)
		{
			bool any = false;
			for (std::unique_ptr<XmlNode> child = node.getFirstChild(); child != nullptr; child = child->getNextSibling())
			{
				if (child->getName() != "moves") { continue; }
				if (!any) { object.moveSets.clear(); any = true; }
				WrittenObject::WrittenMoveSet set;
				set.sprite = child->getAttribute("sprite");
				const std::string here = where + " > <moves" + (set.sprite.empty() ? std::string() : " sprite=\"" + set.sprite + "\"") + ">";
				for (const WrittenObject::WrittenMoveSet& other : object.moveSets)
				{
					if (other.sprite == set.sprite) { fail(here, set.sprite.empty() ? "a second <moves> for no look; give each look's own a sprite=" : "a second <moves> for that look"); }
				}
				for (std::unique_ptr<XmlNode> move = child->getFirstChild(); move != nullptr; move = move->getNextSibling())
				{
					const std::string tag = move->getName();
					if (tag == "forced") { set.forced = true; continue; }
					if (tag == "promote")
					{
						set.promote = move->getAttribute("sprite");
						const auto row = findChild(move.get(), "row");
						const std::string text = readText(*row);
						if (text.empty() || text.size() > 3 || !std::all_of(text.begin(), text.end(), [](unsigned char c) { return std::isdigit(c) != 0; }) || std::stoi(text) < 1)
						{
							fail(here + " > <promote> > <row>", "is \"" + text + "\"; expected a row, counting from 1");
						}
						set.promoteRow = std::stoi(text);
						continue;
					}
					if (tag == "fall") { set.fall = true; continue; }
					if (tag == "route") { readRoute(*move, set, here + " > <route>"); continue; }
					if (tag == "steps")
					{
						set.steps = move->getAttribute("holder");
						set.stepsInto = move->getAttribute("into");
						continue;
					}
					if (tag == "hit")
					{
						set.hits.push_back(move->getAttribute("class"));
						continue;
					}
					if (tag == "on")
					{
						set.on.push_back(move->getAttribute("class"));
						continue;
					}
					if (tag == "close")
					{
						set.closeClass = move->getAttribute("class");
						set.closeObject = move->getAttribute("object");
						continue;
					}
					if (tag == "flank")
					{
						WrittenObject::WrittenMoveSet::WrittenFlank flank;
						flank.objClass = move->getAttribute("class");
						flank.straight = move->getFirstChild() != nullptr;
						set.flanks.push_back(std::move(flank));
						continue;
					}
					WrittenObject::WrittenPieceMove each;
					each.jump = tag == "jump";
					const std::string at = here + " > <" + tag + ">";
					if (each.jump) { each.objClass = move->getAttribute("class"); }
					each.x = readCells(*move, "x", at);
					each.y = readCells(*move, "y", at);
					if (each.x == 0 && each.y == 0) { fail(at, "goes nowhere: <x> and <y> are both 0"); }
					set.moves.push_back(std::move(each));
				}
				object.moveSets.push_back(std::move(set));
			}

			bool anyMarks = false;
			for (std::unique_ptr<XmlNode> child = node.getFirstChild(); child != nullptr; child = child->getNextSibling())
			{
				if (child->getName() != "marks") { continue; }
				if (!anyMarks) { object.markSets.clear(); anyMarks = true; }
				WrittenObject::WrittenMarkSet set;
				set.sprite = child->getAttribute("sprite");
				for (std::unique_ptr<XmlNode> mark = child->getFirstChild(); mark != nullptr; mark = mark->getNextSibling())
				{
					WrittenObject::WrittenMark each;
					each.kind = mark->getName();
					each.object = mark->getAttribute("object");
					each.objClass = mark->getAttribute("class");
					each.variable = mark->getAttribute("variable");
					set.marks.push_back(std::move(each));
				}
				object.markSets.push_back(std::move(set));
			}
		}

		// <owns object="mine" />, a player's own holder, or <owns class="x" />,
		// the pieces a player of a board game moves: one of them.
		void readOwns(const XmlNode& owns, const std::string& where, std::string& holder, std::string& pieces)
		{
			holder = owns.getAttribute("object");
			pieces = owns.getAttribute("class");
			if (holder.empty() == pieces.empty()) { fail(where, "names its holder with object= or its pieces with class=, one of them"); }
		}

		// <poses>: the look for each way it moves, standing first.
		void readPoses(const XmlNode& poses, WrittenObject& object, const std::string&)
		{
			object.poses.clear();
			for (std::unique_ptr<XmlNode> pose = poses.getFirstChild(); pose != nullptr; pose = pose->getNextSibling())
			{
				object.poses[pose->getName()] = pose->getAttribute("sprite");
			}
		}

		// <joins>: the look for each side the piece it is joined to lies, and alone.
		void readJoins(const XmlNode& joins, WrittenObject& object, const std::string&)
		{
			object.joins.clear();
			for (std::unique_ptr<XmlNode> join = joins.getFirstChild(); join != nullptr; join = join->getNextSibling())
			{
				object.joins[join->getName()] = join->getAttribute("sprite");
			}
		}


		// A name that reads as a number. The engine's own words are tags
		// (<self>, <window>), so every other name is the game's.
		void refuseNumber(const std::string& name, const char* kind, const std::string& where)
		{
			const bool number = !name.empty() && std::any_of(name.begin(), name.end(), [](char c) { return std::isdigit(static_cast<unsigned char>(c)) != 0; })
				&& std::all_of(name.begin(), name.end(), [](char c) { return std::isdigit(static_cast<unsigned char>(c)) != 0 || c == '.' || c == '-'; });
			if (number)
			{
				fail(where, std::string(kind) + " may not be called '" + name + "': a number reads that way; give it another name");
			}
		}

		void readVariables(const XmlNode* variablesNode, const std::string& where,
			std::vector<std::pair<std::string, WrittenValue>>& out)
		{
			if (!variablesNode) { return; }

			for (std::unique_ptr<XmlNode> variable = variablesNode->getFirstChild(); variable != nullptr; variable = variable->getNextSibling())
			{
				const std::string name = variable->getAttribute("name");
				refuseNumber(name, "a variable", where);
				WrittenValue value = readValue(*variable, where + " > <variable name=\"" + name + "\">");

				// Declaring a name twice keeps the later value, in the earlier place.
				const auto existing = std::find_if(out.begin(), out.end(), [&](const auto& entry) { return entry.first == name; });
				if (existing != out.end()) { existing->second = std::move(value); }
				else { out.emplace_back(name, std::move(value)); }
			}
		}

		// What a <condition>, an <all> or an <any> tests (below, with <condition>s).
		std::unique_ptr<XmlNode> readCheck(const XmlNode& node, const std::string& here, WrittenCondition& written);
		void readJoin(const XmlNode& join, const std::string& here, WrittenCondition& written);

		// A <hitbox>: pulled in from the picture's sides, each side taking
		// the most particular word given for it, or a shape and its place.
		WrittenHitbox readHitbox(const XmlNode& hitbox, const std::string& here)
		{
			WrittenHitbox read;
			read.name = hitbox.getAttribute("name");
			std::unique_ptr<XmlNode> shape = hitbox.getFirstChild();
			const auto isSide = [](const std::string& tag)
			{
				return tag == "all" || tag == "horizontal" || tag == "vertical" || tag == "top" || tag == "bottom" || tag == "left" || tag == "right";
			};
			if (shape && isSide(shape->getName()))
			{
				std::map<std::string, WrittenValue> given;
				for (std::unique_ptr<XmlNode> part = hitbox.getFirstChild(); part != nullptr; part = part->getNextSibling())
				{
					given[part->getName()] = readValue(*part, here + " > <" + part->getName() + ">");
				}
				const auto side = [&](const char* own, const char* pair) -> WrittenValue
				{
					for (const char* tag : { own, pair, "all" })
					{
						if (const auto found = given.find(tag); found != given.end()) { return found->second; }
					}
					return {};
				};
				read.kind = "inset";
				read.top = side("top", "vertical");
				read.bottom = side("bottom", "vertical");
				read.left = side("left", "horizontal");
				read.right = side("right", "horizontal");
				return read;
			}

			if (!shape)
			{
				fail(here, "expected <all>, <horizontal>, <vertical>, <top>, <bottom>, <left> or <right>, or a <rectangle> or a <circle> then a <position> if wanted");
			}
			read.kind = shape->getName();
			const std::string shapeHere = here + " > <" + shape->getName() + ">";
			if (shape->getName() == "rectangle")
			{
				read.width = readValueOf(*shape, "width", shapeHere);
				read.height = readValueOf(*shape, "height", shapeHere);
			}
			else
			{
				read.radius = readValueOf(*shape, "radius", shapeHere);
			}
			for (std::unique_ptr<XmlNode> next = shape->getNextSibling(); next != nullptr; next = next->getNextSibling())
			{
				const PartialVector2 at = readPartialVector2(*next, here);
				if (at.x) { read.x = *at.x; }
				if (at.y) { read.y = *at.y; }
			}
			return read;
		}

		// A <collisions>, added to what `base` already has: a group inside
		// another keeps the outer group's rules and adds its own after them.
		WrittenCollisionData readCollisionsHere(const XmlNode& collisions, const std::string& where, WrittenCollisionData base);

		WrittenCollisionData readCollisions(const XmlNode& collisions, const std::string& where, WrittenCollisionData base = {})
		{
			return onLineOf(collisions, [&] { return readCollisionsHere(collisions, where, base); });
		}

		// A rule may hold rules: what it says (its attributes and
		// checks) the rules it holds say too, and its <first>
		// runs as a rule of its own before theirs.
		struct Said
		{
			std::string edge, filterClass, filterObject, unlessClass, throughClass, sprite, side, hitbox, by;
			std::vector<WrittenCondition> checks;
		};

		// What a <collision> says, added to what the rules holding it said: its
		// attributes and the checks it starts with. firstCommand is left at what
		// follows the checks.
		Said readSaid(const XmlNode& collision, const Said& held, const std::string& ruleHere, std::unique_ptr<XmlNode>& firstCommand)
		{
			Said said = held;
			const auto take = [&](std::string& into, const char* attribute)
			{
				const std::string own = collision.getAttribute(attribute);
				if (own.empty()) { return; }
				if (!into.empty()) { fail(ruleHere, std::string(attribute) + "=\"" + own + "\", but the <collision> holding it says " + attribute + "=\"" + into + "\" for every rule it holds"); }
				into = own;
			};
			take(said.edge, "edge");
			take(said.filterClass, "class");
			take(said.filterObject, "object");
			take(said.unlessClass, "unless");
			take(said.throughClass, "through");
			take(said.sprite, "sprite");
			take(said.side, "side");
			take(said.hitbox, "hitbox");
			take(said.by, "by");
			const std::string& edge = said.edge;

			firstCommand = collision.getFirstChild();
			if (firstCommand && (firstCommand->getName() == "slower" || firstCommand->getName() == "faster"))
			{
				fail(ruleHere, "<" + firstCommand->getName() + "> is gone; test the speed as the touch happened with <condition variable=\"speed\"><below>N</below></condition> (or <atleast>)");
			}

			// Then the checks that must all hold for it to run.
			if (firstCommand && firstCommand->getName() == "all")
			{
				fail(ruleHere + " > <all>", "a rule's checks must all hold already; write them one after another, with no <all>");
			}
			while (firstCommand && (firstCommand->getName() == "condition" || firstCommand->getName() == "any"))
			{
				const std::string name = firstCommand->getName();
				WrittenCondition part;
				if (name == "condition")
				{
					if (readCheck(*firstCommand, ruleHere + " > <condition>", part)) { fail(ruleHere + " > <condition>", "has its one test and nothing after it; the commands go after the checks"); }
				}
				else
				{
					readJoin(*firstCommand, ruleHere + " > <" + name + ">", part);
				}
				said.checks.push_back(std::move(part));
				firstCommand = firstCommand->getNextSibling();
			}
			const std::string& side = said.side;
			if (!side.empty())
			{
				if (!edge.empty()) { fail(ruleHere, "side= is the side of the object another object touches; a rule about a screen edge says which with edge="); }
				if (side != "top" && side != "bottom" && side != "left" && side != "right" && side != "horizontal" && side != "vertical")
				{
					fail(ruleHere, "side=\"" + side + "\"; expected top, bottom, left, right, horizontal or vertical");
				}
			}
			return said;
		}

		// One rule's commands, as what has been said so far.
		void addRule(WrittenCollisionData& collisionData, const XmlNode& collision, const Said& said, std::vector<WrittenCommand> commands, const std::string& ruleHere)
		{
			const std::string& edge = said.edge;
			// With an edge the rule is about the screen edge. Without one it
			// is about another object: class and/or object narrow which, and
			// neither means anything at all.
			if (!edge.empty())
			{
				if (!said.hitbox.empty() || !said.by.empty())
				{
					fail(ruleHere, "edge= with hitbox= or by=; the window's edges are met by the whole picture");
				}
				if (!said.filterClass.empty() || !said.filterObject.empty() || !said.throughClass.empty())
				{
					fail(ruleHere, "edge= with class=, object= or through=; a rule about a screen edge is about no other object"
						" (unless=\"class\" passes it over while touching that class)");
				}
				WrittenCondition check;
				if (!said.checks.empty())
				{
					check.join = WrittenCondition::Join::All;
					check.parts = said.checks;
				}
				if (!addEdgeRule(collisionData, edge, commands, said.sprite, said.unlessClass, check))
				{
					fail(ruleHere, "edge=\"" + edge + "\"; expected left, right, top, bottom, horizontal, vertical or all");
				}
				return;
			}
			WrittenCollisionRule rule;
			rule.line = collision.line();
			rule.filterClass = said.filterClass;
			rule.filterObject = said.filterObject;
			rule.unlessClass = said.unlessClass;
			rule.throughClass = said.throughClass;
			rule.whileSprite = said.sprite;
			rule.side = said.side;
			rule.hitbox = said.hitbox;
			rule.by = said.by;
			if (!said.checks.empty())
			{
				rule.check.join = WrittenCondition::Join::All;
				rule.check.parts = said.checks;
			}
			rule.commands = std::move(commands);
			collisionData.others.push_back(std::move(rule));
		}

		void readRule(WrittenCollisionData& collisionData, const XmlNode& collision, const Said& held, const std::string& ruleHere)
		{
			std::unique_ptr<XmlNode> firstCommand;
			const Said said = readSaid(collision, held, ruleHere, firstCommand);

			// Holding rules: a <first>, then the rules, and nothing else.
			bool holding = false;
			for (std::unique_ptr<XmlNode> each = collision.getFirstChild(); each; each = each->getNextSibling())
			{
				if (each->getName() == "collision") { holding = true; }
			}
			if (!holding)
			{
				if (firstCommand && firstCommand->getName() == "first") { fail(ruleHere, "has a <first> but holds no rules; its commands go in it directly"); }
				addRule(collisionData, collision, said, readCommands(std::move(firstCommand), ruleHere), ruleHere);
				return;
			}
			std::unique_ptr<XmlNode> part = std::move(firstCommand);
			if (part && part->getName() == "first")
			{
				std::vector<WrittenCommand> first = readCommands(part->getFirstChild(), ruleHere + " > <first>");
				if (!first.empty()) { addRule(collisionData, collision, said, std::move(first), ruleHere); }
				part = part->getNextSibling();
			}
			for (; part && part->getName() == "collision"; part = part->getNextSibling())
			{
				readRule(collisionData, *part, said, ruleHere + " > <collision>");
			}
			if (part)
			{
				fail(ruleHere, "holds rules, so <" + part->getName() + "> is out of place; a rule holding rules has a <first> before them, run whenever it holds, and every rule it holds runs too (there is no <otherwise>)");
			}
		}

		WrittenCollisionData readCollisionsHere(const XmlNode& collisions, const std::string& where, WrittenCollisionData base)
		{
			const std::string collisionsHere = where + " > <collisions>";
			WrittenCollisionData collisionData = std::move(base);

			// A <collisions> is on unless it says <off />; <lockstep /> adds to
			// what an outer group said. The characters of its bitmap that touch,
			// each <solid char>, and what it touches with, if not its picture,
			// its <hitbox>es, replace an outer group's.
			collisionData.enabled = true;
			std::string solid;
			std::vector<WrittenHitbox> hitboxes;
			for (const auto& [tag, child] : Children(collisions))
			{
				if (tag == "off") { collisionData.enabled = false; }
				else if (tag == "lockstep") { collisionData.lockstep = true; }
				else if (tag == "type") { collisionData.type = readText(*child) == "pixel" ? CollisionType::Pixel : CollisionType::Box; }
				else if (tag == "solid")
				{
					const std::string character = child->getAttribute("char");
					if (character.size() != 1 || character == ".")
					{
						fail(collisionsHere + " > <solid>", "char=\"" + character + "\"; expected one character of the bitmap, '*' or one of its <color char>s");
					}
					solid += character;
				}
				else if (tag == "hitbox") { hitboxes.push_back(readHitbox(*child, collisionsHere + " > <hitbox>")); }
				else if (tag == "collision") { readRule(collisionData, *child, Said{}, collisionsHere + " > <collision>"); }
			}
			if (!solid.empty()) { collisionData.solid = solid; }
			if (!hitboxes.empty()) { collisionData.hitboxes = std::move(hitboxes); }

			return collisionData;
		}

		// An action's commands: as readCommands, but a <condition> among them
		// (about the object itself unless it names another) holds commands
		// run only while it holds.
		void readCondition(const XmlNode& condition, const std::string& where, std::vector<WrittenCondition>& out);

		std::vector<WrittenCommand> readActionCommands(std::unique_ptr<XmlNode> first, const std::string& where)
		{
			std::vector<WrittenCommand> commands;
			for (std::unique_ptr<XmlNode> node = std::move(first); node != nullptr; node = node->getNextSibling())
			{
				if (node->getName() != "condition")
				{
					commands.push_back(readCommand(*node, where));
					continue;
				}
				std::vector<WrittenCondition> read;
				readCondition(*node, where, read);
				if (!read.front().inner.empty())
				{
					fail(where + " > <condition>", "holds conditions; in an action a condition holds commands, for now");
				}
				WrittenCommand when;
				when.verb = "condition";
				when.line = node->line();
				when.test = std::make_shared<WrittenCondition>(std::move(read.front()));
				commands.push_back(std::move(when));
			}
			return commands;
		}

		// <actions>: named things the object can do, run by a <trigger>. Nothing
		// is added if `object` has none.
		void readActions(const XmlNode& object, const std::string& where,
			std::map<std::string, std::vector<WrittenCommand>>& out)
		{
			if (std::unique_ptr<XmlNode> actions = findChild(&object, "actions"))
			{
				for (std::unique_ptr<XmlNode> action = actions->getFirstChild(); action != nullptr; action = action->getNextSibling())
				{
					const std::string actionName = action->getAttribute("name");
					out[actionName] = readActionCommands(action->getFirstChild(), where + " > <action name=\"" + actionName + "\">");
				}
			}
		}

		// <variables>: numbers the object owns, read elsewhere as name.variable.
		void readObjectVariables(const XmlNode& object, const std::string& where, std::map<std::string, WrittenValue>& out)
		{
			std::unique_ptr<XmlNode> variablesNode = findChild(&object, "variables");
			std::vector<std::pair<std::string, WrittenValue>> variables;
			readVariables(variablesNode.get(), where + " > <variables>", variables);
			for (auto& [name, value] : variables) { out[name] = std::move(value); }
		}

		// The value of what a game leaves out: 0.
		WrittenValue zeroValue()
		{
			WrittenValue value;
			value.text = "0";
			return value;
		}

		// <maze map="maze">, its <wall class="..." sprite="..." />s and its
		// <slow class="...">share</slow>s.
		WrittenMaze readMaze(const XmlNode& node, const std::string& where)
		{
			const std::string here = where + " > <maze>";
			WrittenMaze maze;
			maze.map = node.getAttribute("map");
			for (std::unique_ptr<XmlNode> child = node.getFirstChild(); child != nullptr; child = child->getNextSibling())
			{
				if (child->getName() == "slow")
				{
					const std::string there = here + " > <slow>";
					MazeSlow slow;
					slow.objClass = child->getAttribute("class");
					const std::string text = readText(*child);
					slow.share = std::stof(text);
					if (!(slow.share > 0.0f)) { fail(there, "is " + text + "; a walker slowed to nothing would stop for good, so give a share above 0"); }
					maze.slows.push_back(std::move(slow));
					continue;
				}
				MazeWall wall;
				wall.objClass = child->getAttribute("class");
				wall.sprite = child->getAttribute("sprite");
				maze.walls.push_back(std::move(wall));
			}
			return maze;
		}

		// A <shape>'s <row>s of characters, top to bottom, all as wide.
		std::vector<std::string> readShapeRows(const XmlNode& node, const std::string& where)
		{
			std::vector<std::string> rows;
			const std::string here = where + " > <shape>";
			for (std::unique_ptr<XmlNode> child = node.getFirstChild(); child != nullptr; child = child->getNextSibling())
			{
				const std::string row = readText(*child);
				if (row.empty()) { fail(here + " > <row> " + std::to_string(rows.size() + 1), "is empty"); }
				if (!rows.empty() && row.size() != rows.front().size())
				{
					fail(here + " > <row> " + std::to_string(rows.size() + 1), "is " + std::to_string(row.size()) + " cells wide, but row 1 is " + std::to_string(rows.front().size()));
				}
				rows.push_back(row);
			}
			return rows;
		}


		// A cell map's <checks>: each a question the map asks after
		// something settles (<full> rows), and its commands.
		std::vector<WrittenCheck> readChecks(const XmlNode& node, const std::string& where)
		{
			std::vector<WrittenCheck> checks;
			for (std::unique_ptr<XmlNode> child = node.getFirstChild(); child != nullptr; child = child->getNextSibling())
			{
				const std::string tag = child->getName();
				const std::string here = where + " > <checks> > <" + tag + ">";
				WrittenCheck check;
				check.kind = tag;
				check.objClass = child->getAttribute("class");
				for (std::unique_ptr<XmlNode> command = child->getFirstChild(); command != nullptr; command = command->getNextSibling())
				{
					if (command->getName() == "atleast")
					{
						check.count = readCount(*command, here);
						continue;
					}
					if (command->getName() == "straight") { check.straight = true; continue; }
					if (command->getName() == "alike") { check.alike = command->getAttribute("variable"); continue; }
					check.commands.push_back(readCommand(*command, here));
				}
				if (tag == "line" && check.count < 2) { fail(here, "needs <atleast> 2 or more: how many alike make a line"); }
				checks.push_back(std::move(check));
			}
			return checks;
		}

		// A <map> as what is on it is read (readMap): the map, and which
		// character each of its groups and objects picks, so that no two pick
		// the same one.
		struct MapLayout : WrittenMap
		{
			std::map<char, std::string>* picked = nullptr;
		};

		// The one character a group or object on a map picks (char="."),
		// noted as picked by `who`.
		char readMapCharacter(const XmlNode& node, const MapLayout& map, const std::string& who, const std::string& where)
		{
			const std::string character = requireAttribute(node, "char", where);
			if (character.size() != 1) { fail(where, "char=\"" + character + "\": a map's tiles are one character each"); }
			const auto [earlier, isNew] = map.picked->emplace(character[0], who);
			if (!isNew) { fail(where, "char=\"" + character + "\" is picked by " + earlier->second + " already"); }
			return character[0];
		}

		// An object on a map starts on the one tile of the character it picks.
		// On a cell map, one on no tile (or picking none) waits to be spawned.
		void placeObjectOnMap(const XmlNode& node, const MapLayout& map, WrittenObject& object, const std::string& where)
		{
			if (map.cells && node.getAttribute("char").empty())
			{
				waitOnMap(object, map);
				return;
			}
			const char character = readMapCharacter(node, map, where, where);
			if (startOnMap(object, map, character)) { return; }
			const std::size_t found = tilesOf(map, character).size();
			if (found == 0 && map.cells)
			{
				waitOnMap(object, map, character);
				return;
			}
			fail(where, "picks '" + std::string(1, character) + "', which is on " + std::to_string(found) + " tiles of map '" + map.name
				+ "'; an object starts on one, or between two side by side, and a group is what has a member on each");
		}


		// <exists>false</exists>: whether the object is in the game at the start.
		bool readExists(const XmlNode& object, const std::string& where)
		{
			const auto exists = findChild(&object, "exists");
			return !exists || readBool(*exists, where);
		}

		// The parts an object, or a group for its members, gives by one tag
		// each and reads alike: the tag and how it is read into the object.
		struct SimplePart
		{
			std::string_view tag;
			void (*read)(const XmlNode& node, WrittenObject& into, const std::string& where);
		};

		constexpr SimplePart simpleParts[]
		{
			{ "acceleration", [](const XmlNode& node, WrittenObject& into, const std::string& where) { into.writtenAcceleration = readVector2(node, where); } },
			{ "heading", [](const XmlNode& node, WrittenObject& into, const std::string& where) { into.writtenHeading = readValue(node, where); } },
			{ "pivot", [](const XmlNode& node, WrittenObject& into, const std::string& where) { into.writtenPivot = readVector2(node, where); } },
			{ "drag", [](const XmlNode& node, WrittenObject& into, const std::string& where) { into.writtenDrag = readValue(node, where); } },
			{ "friction", [](const XmlNode& node, WrittenObject& into, const std::string& where) { into.writtenFriction = readValue(node, where); } },
			{ "mass", [](const XmlNode& node, WrittenObject& into, const std::string& where) { into.writtenMass = readValue(node, where); } },
			{ "surface", [](const XmlNode& node, WrittenObject& into, const std::string& where) { readSurface(node, into, where); } },
			{ "facing", [](const XmlNode& node, WrittenObject& into, const std::string&) { into.facing = readText(node); } },
			{ "maze", [](const XmlNode& node, WrittenObject& into, const std::string& where) { into.maze = readMaze(node, where); } },
			{ "most", [](const XmlNode& node, WrittenObject& into, const std::string& where) { into.most = readCount(node, where); } },
			// <waiting />: out of play until a <reveal>; <ready />, in play after
			// all (a take of a waiting piece). <hidden />: in play, never drawn.
			{ "waiting", [](const XmlNode&, WrittenObject& into, const std::string&) { into.isVisible = false; } },
			{ "ready", [](const XmlNode&, WrittenObject& into, const std::string&) { into.isVisible = true; } },
			{ "hidden", [](const XmlNode&, WrittenObject& into, const std::string&) { into.isDrawn = false; } },
			{ "pull", [](const XmlNode& node, WrittenObject& into, const std::string& where) { into.writtenPulls.emplace_back(node.getAttribute("object"), readValue(node, where + " > <pull>")); } },
			{ "seek", [](const XmlNode& node, WrittenObject& into, const std::string& where) { into.seekTarget = node.getAttribute("object"); into.writtenSeek = readValue(node, where + " > <seek>"); } },
			{ "light", readLight },
			{ "dark", [](const XmlNode& node, WrittenObject& into, const std::string&) { into.dark = readText(node); } },
			{ "altitude", readAltitude },
			{ "shadow", [](const XmlNode& node, WrittenObject& into, const std::string&) { into.shadowOf = node.getAttribute("object"); } },
			{ "holds", [](const XmlNode& node, WrittenObject& into, const std::string& where) { readHolds(node, into.holder, where + " > <holds>"); } },
			{ "owns", [](const XmlNode& node, WrittenObject& into, const std::string& where) { readOwns(node, where + " > <owns>", into.handHolder, into.ownsClass); } },
			{ "poses", readPoses },
			{ "joins", readJoins },
			{ "mirror", [](const XmlNode& node, WrittenObject& into, const std::string&) { into.mirror = readText(node); } },
			{ "grip", [](const XmlNode& node, WrittenObject& into, const std::string& where) { into.writtenGrip = readValue(node, where + " > <grip>"); } },
			// An inner group's touches add to and change its outer group's.
			{ "collisions", [](const XmlNode& node, WrittenObject& into, const std::string& where) { into.writtenCollisionData = readCollisions(node, where, into.writtenCollisionData); } },
			// An inner group's timers come after its outer group's.
			{ "timers", [](const XmlNode& node, WrittenObject& into, const std::string& where) { for (WrittenTimer& timer : readTimers(&node, where)) { into.timers.push_back(std::move(timer)); } } },
		};

		const SimplePart* simplePart(std::string_view tag)
		{
			for (const SimplePart& part : simpleParts)
			{
				if (part.tag == tag) { return &part; }
			}
			return nullptr;
		}

		// An <object>; on a <map> (`map`), one that starts on the one tile of
		// the character it picks, with no <position> of its own.
		// The tag read; readObject without the line.
		WrittenObject readObjectHere(const XmlNode& object, const MapLayout* map);

		WrittenObject readObject(const XmlNode& object, const MapLayout* map = nullptr)
		{
			return onLineOf(object, [&] { return readObjectHere(object, map); });
		}

		WrittenObject readObjectHere(const XmlNode& object, const MapLayout* map)
		{
			WrittenObject writtenObject;
			writtenObject.line = object.line();
			writtenObject.name = object.getAttribute("name");
			refuseNumber(writtenObject.name, "an object", "<object name=\"" + writtenObject.name + "\">");
			writtenObject.objClass = object.getAttribute("class");

			const std::string where = "object '" + writtenObject.name + "'";
			const Children children(object);

			std::vector<AnimationSpec> animations = readAnimations(object, where);
			std::vector<WrittenSprite> sprites = readSprites(object, where);

			// No <sprite>: an object that only holds numbers (a score, the
			// lives, the clock), never drawn and, unless it says, at 0, 0.
			const bool holder = sprites.empty() && animations.empty() && !map;
			// Looks that are all empty: one that only keeps where something is up
			// to (a look for each), drawn as nothing, and, unless it says, at 0, 0.
			bool blank = !sprites.empty() && animations.empty() && !map;
			for (const Children::Child& child : children)
			{
				if (child.name == "sprite" && child.node->getFirstChild()) { blank = false; }
			}
			const bool shape = children.find("shape") != nullptr;
			if (shape)
			{
				if (!map || !map->cells) { fail(where, "has a <shape>, which is for an object on a map with <cells />"); }
				if (!sprites.empty() || !animations.empty()) { fail(where, "has a <shape>, which is drawn by its cells, so it has no <sprite> or <animation>"); }
				writtenObject.shapeRows = readShapeRows(*children.find("shape"), where);
				writtenObject.sprite = nothingSprite();
				writtenObject.isDrawn = false;
			}
			else if (holder)
			{
				writtenObject.sprite = nothingSprite();
				writtenObject.isDrawn = false;
				if (children.find("hidden")) { fail(where, "has no <sprite>, so it is never drawn; <hidden /> is for something with a picture"); }
				if (!children.find("variables") && !children.find("timers") && !children.find("holds")) { fail(where, "has no <sprite> and no <variables>, <timers> or <holds>, so it is nothing"); }
			}
			else
			{
				chooseSprite(std::move(sprites), animations, where, writtenObject);
				if (blank && children.find("hidden")) { fail(where, "draws nothing in any of its looks; <hidden /> is for something with a picture"); }
			}

			if (map)
			{
				if (children.find("position")) { fail(where, "is on a <map>, where the tile of its char= is where it starts, so it has no <position>"); }
				placeObjectOnMap(object, *map, writtenObject, where);
			}
			else
			{
				if (!object.getAttribute("char").empty()) { fail(where, "char=\"" + object.getAttribute("char") + "\" picks a character of a <map>'s rows; this object is on no <map>"); }
				if (auto position = children.find("position")) { writtenObject.writtenPosition = readVector2(*position, where); }
				else if (holder || blank || !readExists(object, where)) { writtenObject.writtenPosition = { zeroValue(), zeroValue() }; }
				else { fail(where, "missing <position>"); }
			}
			// What a game leaves out is nothing: no <velocity> (or no half of
			// one) stands still, and no <collisions> touches nothing.
			if (auto velocity = children.find("velocity"))
			{
				const PartialVector2 given = readPartialVector2(*velocity, where);
				writtenObject.writtenVelocity.x = given.x.value_or(zeroValue());
				writtenObject.writtenVelocity.y = given.y.value_or(zeroValue());
			}
			else
			{
				writtenObject.writtenVelocity = { zeroValue(), zeroValue() };
			}
			for (const Children::Child& child : children)
			{
				if (const SimplePart* part = simplePart(child.name)) { part->read(*child.node, writtenObject, where); }
			}
			readPieceMoves(object, writtenObject, where);
			if (auto chain = children.find("chain"))
			{
				if (!map || !map->cells) { fail(where, "has a <chain>, which is for an object on a map with <cells />"); }
				writtenObject.chain = chain->getAttribute("object");
			}
			writtenObject.exists = writtenObject.exists && readExists(object, where);
			readActions(object, where, writtenObject.action);
			readObjectVariables(object, where, writtenObject.variable);

			return writtenObject;
		}

		// What a member says for one thing, else what its group says, else an error.
		WrittenValue pickValue(const std::optional<WrittenValue>& own, const std::optional<WrittenValue>& shared,
			const std::string& where, const std::string& what)
		{
			if (own) { return *own; }
			if (shared) { return *shared; }
			fail(where, "has no " + what + ", and neither does its group");
		}

		// Which rows or columns a <row> or <column> picks, counting from 1:
		// one by number="2", several by a <number> each, every odd or even
		// one by <odd /> or <even />.
		std::vector<bool> readPick(const XmlNode& node, int count, const std::string& where)
		{
			std::vector<bool> picked(static_cast<std::size_t>(count) + 1, false);
			const auto pick = [&](const std::string& text)
			{
				if (text.empty() || text.size() > 4 || !std::all_of(text.begin(), text.end(), [](unsigned char c) { return std::isdigit(c) != 0; })
					|| std::stoi(text) < 1 || std::stoi(text) > count)
				{
					fail(where, "\"" + text + "\" is not one of 1 to " + std::to_string(count)
						+ (text.find_first_of(" \t") == std::string::npos ? "" : "; write several as a <number> each"));
				}
				picked[static_cast<std::size_t>(std::stoi(text))] = true;
			};

			const std::string attribute = node.getAttribute("number");
			bool any = !attribute.empty();
			if (any) { pick(attribute); }
			for (std::unique_ptr<XmlNode> child = node.getFirstChild(); child != nullptr; child = child->getNextSibling())
			{
				const std::string tag = child->getName();
				if (tag != "number" && tag != "odd" && tag != "even") { continue; }
				if (!attribute.empty()) { fail(where, "has number=\"" + attribute + "\" and a <" + tag + ">; use one or the other"); }
				any = true;
				if (tag == "number") { pick(readText(*child)); }
				else
				{
					for (int i = tag == "odd" ? 1 : 2; i <= count; i += 2) { picked[static_cast<std::size_t>(i)] = true; }
				}
			}
			if (!any) { fail(where, "picks nothing; give number=\"...\", a <number> for each, <odd /> or <even />"); }

			return picked;
		}

		// What a <member>, <row>, <column> or <cell> of a group says for itself.
		struct GroupPart
		{
			std::string where;
			std::unique_ptr<XmlNode> node;
			std::vector<bool> rows;                 // <row>: the rows it picks
			std::vector<bool> columns;              // <column>: the columns it picks
			int row{ 0 };                           // <cell>
			int column{ 0 };                        // <cell>
			std::set<std::string> sprites;          // the names of the sprites it changes
			std::vector<AnimationSpec> animations;
			PartialVector2 position;
			PartialVector2 velocity;
			PartialVector2 padding;
			std::map<std::string, WrittenValue> variables;
			std::string owns;                       // <owns object=>
			std::string ownsClass;                  // <owns class=>
			std::vector<WrittenCommand> think;          // <think>
			int team{};                             // <team>: its side, partners alike
		};

		// The parts of a member, row, column or cell, as far as they are its
		// own. Its sprites are applied to the group's later (changeSprites).
		GroupPart readGroupPart(std::unique_ptr<XmlNode> node, const std::string& where)
		{
			GroupPart part;
			part.where = where;

			for (std::unique_ptr<XmlNode> child = node->getFirstChild(); child != nullptr; child = child->getNextSibling())
			{
				const std::string tag = child->getName();

				if (tag == "sprite") { part.sprites.insert(child->getAttribute("name")); }
				else if (tag == "animation") { /* below, all at once */ }
				else if (tag == "position") { part.position = readPartialVector2(*child, where); }
				else if (tag == "velocity") { part.velocity = readPartialVector2(*child, where); }
				else if (tag == "padding") { part.padding = readPartialVector2(*child, where); }
				else if (tag == "owns") { readOwns(*child, where + " > <owns>", part.owns, part.ownsClass); }
				else if (tag == "think") { part.think = { readCommand(*child, where) }; }
				else if (tag == "team") { part.team = readCount(*child, where + " > <team>"); }
			}
			readObjectVariables(*node, where, part.variables);
			part.animations = readAnimations(*node, where);

			part.node = std::move(node);
			return part;
		}

		// A <row> and a <column> (or two rows, or two columns) that both pick
		// a cell must not both change the same thing in it: which would win is
		// not something to guess. A <cell> may change anything they do.
		void checkNoClash(const GroupPart& one, const GroupPart& other, const std::string& cell)
		{
			const auto clash = [&](const std::string& what)
			{
				fail(cell, "is picked by " + one.where + " and by " + other.where + ", and both change its " + what
					+ "; change it in one of them, or in a <cell>");
			};

			for (const std::string& name : one.sprites)
			{
				if (other.sprites.count(name)) { clash(name.empty() ? std::string("<sprite>") : "<sprite name=\"" + name + "\">"); }
			}
			for (const AnimationSpec& animation : one.animations)
			{
				if (std::any_of(other.animations.begin(), other.animations.end(), [&](const AnimationSpec& each) { return each.name == animation.name; }))
				{
					clash(animation.name.empty() ? std::string("<animation>") : "<animation name=\"" + animation.name + "\">");
				}
			}
			if (one.velocity.x && other.velocity.x) { clash("<velocity><x>"); }
			if (one.velocity.y && other.velocity.y) { clash("<velocity><y>"); }
			if (one.padding.x && other.padding.x) { clash("<padding><x>"); }
			if (one.padding.y && other.padding.y) { clash("<padding><y>"); }
			for (const auto& [name, value] : one.variables)
			{
				if (other.variables.count(name)) { clash("<variable name=\"" + name + "\">"); }
			}
		}

		// What a <group> gives what is inside it, already merged with what the
		// groups around it give: a group inside another starts from all of this.
		struct GroupShared
		{
			std::vector<WrittenSprite> sprites;
			std::vector<AnimationSpec> animations;
			PartialVector2 position;
			PartialVector2 velocity;
			PartialVector2 padding;
			std::optional<WrittenValue> slope;
			int columns = 0;
			int rows = 0;
			// Visibility, facing, acceleration, collisions, actions, variables,
			// timers, class and the names of the groups around it.
			WrittenObject object;
			// The <map> it is on, if it is: its members are the tiles of the
			// character it picks.
			const MapLayout* map = nullptr;
			// On a map of cells: the character a group around it picks for
			// all its members (Columns' jewels), each waiting to be spawned.
			char character = '\0';
		};

		// A <group>, read as one WrittenObject per member (logrow3.2, or
		// bricks.5.3 for column 5, row 3), each taking from the group what it
		// does not give itself; an inner <group> takes from the outer one the
		// same way. What each part means: docs/reference/objects.md.
		// The tag read; readGroup without the line.
		void readGroupHere(const XmlNode& group, std::vector<WrittenObject>& out, const GroupShared& outer);

		void readGroup(const XmlNode& group, std::vector<WrittenObject>& out, const GroupShared& outer = {})
		{
			return onLineOf(group, [&] { return readGroupHere(group, out, outer); });
		}

		// The tags of a <group> other than its members and the groups inside:
		// what it gives them, added to what the groups around it give. True when
		// it holds groups.
		bool readGroupGives(const XmlNode& group, GroupShared& own, const std::string& where)
		{
			WrittenObject& shared = own.object;
			bool inner = false;
			for (std::unique_ptr<XmlNode> child = group.getFirstChild(); child != nullptr; child = child->getNextSibling())
			{
				const std::string tag = child->getName();
				if (own.map && (tag == "columns" || tag == "rows" || tag == "padding" || tag == "slope" || tag == "position" || tag == "member" || tag == "row" || tag == "column" || tag == "cell"))
				{
					fail(where, "is on map '" + own.map->name + "', whose rows say where its members are, so it has no <" + tag + ">");
				}

				if (const SimplePart* part = simplePart(tag)) { part->read(*child, shared, where); }
				else if (tag == "exists") { shared.exists = readBool(*child, where); }
				else if (tag == "columns" || tag == "rows")
				{
					(tag == "columns" ? own.columns : own.rows) = readCount(*child, where);
					// What its members' column or row is also called, beside column and row.
					const std::string alias = child->getAttribute("name");
					if (!alias.empty()) { (tag == "columns" ? shared.cell.columnName : shared.cell.rowName) = alias; }
				}
				else if (tag == "padding")
				{
					const PartialVector2 given = readPartialVector2(*child, where);
					if (given.x) { own.padding.x = given.x; }
					if (given.y) { own.padding.y = given.y; }
				}
				else if (tag == "slope") { own.slope = readValue(*child, where); }
				else if (tag == "animation") { /* below, all at once */ }
				else if (tag == "position" || tag == "velocity")
				{
					PartialVector2& into = tag == "position" ? own.position : own.velocity;
					const PartialVector2 given = readPartialVector2(*child, where);
					if (given.x) { into.x = given.x; }
					if (given.y) { into.y = given.y; }
				}
				else if (tag == "moves" || tag == "marks") { /* below, all at once */ }
				else if (tag == "turns")
				{
					shared.takesTurns = true;
					shared.turnsSprite = child->getAttribute("sprite");
				}
				else if (tag == "chain")
				{
					if (!own.map || !own.map->cells) { fail(where, "has a <chain>, which is for a group on a map with <cells />"); }
					shared.chain = child->getAttribute("object");
				}
				else if (tag == "shape")
				{
					if (!own.map || !own.map->cells) { fail(where, "has a <shape>, which is for a group on a map with <cells />"); }
					shared.shapeRows = readShapeRows(*child, where);
				}
				else if (tag == "group") { inner = true; }
			}

			own.sprites = changeSprites(own.sprites, group, where);
			mergeAnimations(own.animations, readAnimations(group, where));
			readActions(group, where, shared.action);
			readObjectVariables(group, where, shared.variable);
			readPieceMoves(group, shared, where);
			if (const std::string objClass = group.getAttribute("class"); !objClass.empty()) { shared.objClass = objClass; }
			return inner;
		}

		// A group of groups: each inner group read from this one.
		void readInnerGroups(const XmlNode& group, std::vector<WrittenObject>& out, GroupShared& own, const std::string& name, const std::string& where)
		{
			if (!group.getAttribute("char").empty())
			{
				// On a map of cells, one character for all of them: a shape's
				// cell of it is any one, at random.
				if (!own.map || !own.map->cells || own.character != '\0') { fail(where, "has groups inside it, so it picks no character itself; give the char= to the groups inside"); }
				own.character = readMapCharacter(group, *own.map, where, where);
				if (!tilesOf(*own.map, own.character).empty()) { fail(where, "has groups inside it and picks '" + std::string(1, own.character) + "', which is on tiles of map '" + own.map->name + "'; a group of groups picks a character only for shapes, waiting"); }
			}
			own.object.outerGroups.push_back(name);
			for (std::unique_ptr<XmlNode> child = group.getFirstChild(); child != nullptr; child = child->getNextSibling())
			{
				const std::string tag = child->getName();
				if (tag == "member" || tag == "row" || tag == "column" || tag == "cell")
				{
					fail(where, "has <group>s inside it, so it has no <" + tag + ">s of its own; give them to the groups inside");
				}
				if (tag == "group") { readGroup(*child, out, own); }
			}
		}

		// One member or cell from the group and its own parts, the most
		// particular last.
		WrittenObject makeMember(const XmlNode& group, const GroupShared& own, const std::string& memberName, const std::string& here, const std::vector<const GroupPart*>& parts)
		{
			const std::vector<WrittenSprite>& sprites = own.sprites;
			const std::vector<AnimationSpec>& animations = own.animations;
			const PartialVector2& position = own.position;
			const PartialVector2& velocity = own.velocity;
			WrittenObject writtenObject = own.object;
			writtenObject.name = memberName;
			writtenObject.line = group.line();

			std::vector<WrittenSprite> used = sprites;
			std::vector<AnimationSpec> usedAnimations = animations;
			std::optional<WrittenValue> positionX = position.x;
			std::optional<WrittenValue> positionY = position.y;
			std::optional<WrittenValue> velocityX = velocity.x;
			std::optional<WrittenValue> velocityY = velocity.y;
			for (const GroupPart* part : parts)
			{
				used = changeSprites(used, *part->node, part->where);
				mergeAnimations(usedAnimations, part->animations);
				if (part->position.x) { positionX = part->position.x; }
				if (part->position.y) { positionY = part->position.y; }
				if (part->velocity.x) { velocityX = part->velocity.x; }
				if (part->velocity.y) { velocityY = part->velocity.y; }
				for (const auto& [variable, value] : part->variables) { writtenObject.variable[variable] = value; }
				if (!part->owns.empty()) { writtenObject.handHolder = part->owns; }
				if (!part->ownsClass.empty()) { writtenObject.ownsClass = part->ownsClass; }
				if (!part->think.empty()) { writtenObject.think = part->think; }
				if (part->team != 0) { writtenObject.handTeam = part->team; }
			}

			if (!writtenObject.shapeRows.empty())
			{
				if (!used.empty() || !usedAnimations.empty()) { fail(here, "has a <shape>, which is drawn by its cells, so it and its group have no <sprite> or <animation>"); }
				writtenObject.sprite = nothingSprite();
				writtenObject.isDrawn = false;
			}
			else
			{
				const bool cut = std::any_of(usedAnimations.begin(), usedAnimations.end(), [](const AnimationSpec& each) { return !each.cells.empty(); });
				if (used.empty() && !cut) { fail(here, "has no <sprite>, and neither does its group"); }
				chooseSprite(used, usedAnimations, here, writtenObject);
			}

			// One that does not exist at the start is placed by the <spawn>
			// that makes it, so its <position> may be left out (0, 0).
			if (!writtenObject.exists)
			{
				if (!positionX) { positionX = zeroValue(); }
				if (!positionY) { positionY = zeroValue(); }
			}
			writtenObject.writtenPosition.x = pickValue(positionX, std::nullopt, here, "<position><x>");
			writtenObject.writtenPosition.y = pickValue(positionY, std::nullopt, here, "<position><y>");
			writtenObject.writtenVelocity.x = velocityX.value_or(zeroValue());
			writtenObject.writtenVelocity.y = velocityY.value_or(zeroValue());
			return writtenObject;
		}

		// On a map: a member for each tile of its character, named under
		// the map by its number in the order the rows are read
		// (maze.dots.17), so two maps in one list each have their own.
		void readMapMembers(const XmlNode& group, std::vector<WrittenObject>& out, const GroupShared& own, const std::string& name, const std::string& where)
		{
			const MapLayout& map = *own.map;
			// On a cell map, a group picking no character, or one on no
			// tile, is one piece waiting to be spawned (map.group).
			const auto waiting = [&](char character)
			{
				const std::string memberName = map.name + "." + name;
				WrittenObject member = makeMember(group, own, memberName, "object '" + memberName + "' (" + where + ")", {});
				waitOnMap(member, map, character);
				out.push_back(std::move(member));
			};
			if (map.cells && group.getAttribute("char").empty())
			{
				waiting(own.character);
				return;
			}
			if (own.character != '\0') { fail(where, "picks a character, and so does a group around it; give it to one of them"); }
			const char character = readMapCharacter(group, map, where, where);
			if (map.cells && tilesOf(map, character).empty())
			{
				waiting(character);
				return;
			}
			const auto tiles = tilesOf(map, character);
			// Room for them all at once, growing as a vector does: each move of
			// a written object moves all its text.
			if (out.capacity() < out.size() + tiles.size()) { out.reserve(std::max(out.size() + tiles.size(), out.capacity() * 2)); }
			// Every member is the same but for its name and tile, so the
			// first is made and the rest are copies of it.
			std::optional<WrittenObject> first;
			int count = 0;
			for (const auto& [column, row] : tiles)
			{
				++count;
				const std::string memberName = map.name + "." + name + "." + std::to_string(count);
				if (!first) { first = makeMember(group, own, memberName, "object '" + memberName + "' (a member of " + where + ")", {}); }
				WrittenObject& member = out.emplace_back(*first);
				member.name = memberName;
				placeOnMap(member, map, column, row, character);
			}
			if (count == 0) { fail(where, "picks '" + std::string(1, character) + "', which is on no tile of map '" + map.name + "'"); }
			return;
		}

		// Members listed, each a <member>.
		void readMembers(std::vector<std::unique_ptr<XmlNode>>& children, std::vector<WrittenObject>& out, const XmlNode& group, const GroupShared& own, const std::string& name, const std::string& where)
		{
			int count = 0;
			for (std::unique_ptr<XmlNode>& child : children)
			{
				const std::string tag = child->getName();
				if (tag == "row" || tag == "column" || tag == "cell")
				{
					fail(where, "has a <" + tag + ">, but no <columns> and <rows> for it to pick from");
				}
				if (tag != "member") { continue; }

				++count;
				const std::string ownName = child->getAttribute("name");
				if (!ownName.empty()) { refuseNumber(ownName, "a member", where); }
				const std::string memberName = ownName.empty() ? name + "." + std::to_string(count) : ownName;
				const std::string here = "object '" + memberName + "' (a member of " + where + ")";

				const GroupPart member = readGroupPart(std::move(child), here);
				out.push_back(makeMember(group, own, memberName, here, { &member }));
			}

			if (count == 0) { fail(where, "has no <member>s, and no <columns> and <rows> to lay out cells in"); }
			return;
		}

		// The <row>s and <column>s, in the order written, and the <cell>s of a
		// group laid out in columns and rows.
		void readLinesAndCells(std::vector<std::unique_ptr<XmlNode>>& children, int columns, int rows, const std::string& where, std::vector<GroupPart>& lines, std::vector<GroupPart>& cells)
		{
			for (std::unique_ptr<XmlNode>& child : children)
			{
				const std::string tag = child->getName();
				if (tag == "member") { fail(where, "is laid out in <columns> and <rows>, so it has no <member>s; a <cell> changes one place"); }

				if (tag == "row" || tag == "column")
				{
					const std::string number = child->getAttribute("number");
					const std::string here = where + " > <" + tag + (number.empty() ? std::string() : " number=\"" + number + "\"") + ">";
					std::vector<bool> picked = readPick(*child, tag == "row" ? rows : columns, here);
					GroupPart line = readGroupPart(std::move(child), here);
					if (tag == "row") { line.rows = std::move(picked); }
					else
					{
						if (line.padding.y) { fail(here, "<padding> of a column gives only an <x>, the gap before it"); }
						line.columns = std::move(picked);
					}
					lines.push_back(std::move(line));
				}
				else if (tag == "cell")
				{
					const std::string rowText = child->getAttribute("row");
					const std::string columnText = child->getAttribute("column");
					const std::string here = where + " > <cell row=\"" + rowText + "\" column=\"" + columnText + "\">";

					const auto place = [&](const std::string& text, int count) -> int
					{
						if (text.empty() || text.size() > 4 || !std::all_of(text.begin(), text.end(), [](unsigned char c) { return std::isdigit(c) != 0; })
							|| std::stoi(text) < 1 || std::stoi(text) > count)
						{
							fail(here, "is not a place in the group; its rows and columns count from 1 to " + std::to_string(rows) + " and " + std::to_string(columns));
						}
						return std::stoi(text);
					};
					const int row = place(rowText, rows);
					const int column = place(columnText, columns);

					for (const GroupPart& other : cells)
					{
						if (other.row == row && other.column == column) { fail(here, "is the same place as an earlier <cell>; say all of it in one"); }
					}

					GroupPart cell = readGroupPart(std::move(child), here);
					cell.row = row;
					cell.column = column;
					cells.push_back(std::move(cell));
				}
			}
		}

		// A member for each place in the columns and rows, with what the rows,
		// columns and <cell> that pick it change.
		void placeCells(const XmlNode& group, std::vector<WrittenObject>& out, const GroupShared& own, const std::vector<GroupPart>& lines, const std::vector<GroupPart>& cells,
			int columns, int rows, const std::string& name, const std::string& where)
		{
			const PartialVector2& padding = own.padding;
			const auto picks = [](const GroupPart& line, int row, int column)
			{
				return line.rows.empty() ? line.columns[static_cast<std::size_t>(column)] : line.rows[static_cast<std::size_t>(row)];
			};

			for (int row = 1; row <= rows; ++row)
			{
				for (int column = 1; column <= columns; ++column)
				{
					const GroupPart* cell = nullptr;
					for (const GroupPart& candidate : cells)
					{
						if (candidate.row == row && candidate.column == column) { cell = &candidate; }
					}

					const std::string ownName = cell ? cell->node->getAttribute("name") : std::string{};
					// Named by column and row; by the one given, when only one is.
					const std::string place = own.columns == 0 ? std::to_string(row)
						: own.rows == 0 ? std::to_string(column)
						: std::to_string(column) + "." + std::to_string(row);
					const std::string cellName = ownName.empty() ? name + "." + place : ownName;
					const std::string here = "object '" + cellName + "' (column " + std::to_string(column) + ", row " + std::to_string(row) + " of " + where + ")";

					// The rows and columns that pick it, then its own <cell>; and
					// the rows alone, which give its row's sprite and so its slot.
					std::vector<const GroupPart*> parts;
					std::vector<const GroupPart*> rowParts;
					const GroupPart* gapBefore = nullptr;
					const GroupPart* gapAbove = nullptr;
					for (const GroupPart& line : lines)
					{
						if (!picks(line, row, column)) { continue; }
						for (const GroupPart* earlier : parts) { checkNoClash(*earlier, line, here); }
						parts.push_back(&line);
						if (!line.rows.empty()) { rowParts.push_back(&line); }
						if (line.padding.x) { gapBefore = &line; }
						if (line.padding.y) { gapAbove = &line; }
					}
					if (cell) { parts.push_back(cell); }

					WrittenObject writtenObject = makeMember(group, own, cellName, here, parts);

					writtenObject.cell.slot = makeMember(group, own, cellName, here, rowParts).sprite;
					writtenObject.cell.column = column;
					writtenObject.cell.row = row;
					writtenObject.takeOwnCell();
					if (const std::string clash = writtenObject.numberCell(); !clash.empty())
					{
						fail(here, "its group names its rows or columns '" + clash + "', and it has a variable of that name already");
					}
					writtenObject.cell.gapBefore = column == 1 ? WrittenValue::expression("0")
						: gapBefore ? *gapBefore->padding.x : padding.x ? *padding.x : WrittenValue::expression("0");
					writtenObject.cell.gapAbove = row == 1 ? WrittenValue::expression("0")
						: gapAbove ? *gapAbove->padding.y : padding.y ? *padding.y : WrittenValue::expression("0");
					if (own.slope) { writtenObject.cell.drop = *own.slope; }

					out.push_back(std::move(writtenObject));
				}
			}
		}

		void readGroupHere(const XmlNode& group, std::vector<WrittenObject>& out, const GroupShared& outer)
		{
			const std::string name = group.getAttribute("name");
			const std::string where = "group '" + name + "'";
			refuseNumber(name, "a group", where);

			GroupShared own = outer;
			// Its members are of one kind (Kind): their rules are written
			// only here, so they are alike.
			static int lastKind = 0;
			own.object.kind = ++lastKind;

			if (!group.getAttribute("char").empty() && !own.map)
			{
				fail(where, "char=\"" + group.getAttribute("char") + "\" picks a character of a <map>'s rows; this group is on no <map>");
			}

			if (readGroupGives(group, own, where))
			{
				readInnerGroups(group, out, own, name, where);
				return;
			}

			// <rows> alone is one column (lines of text), <columns> alone one row.
			const int columns = own.columns > 0 || own.rows == 0 ? own.columns : 1;
			const int rows = own.rows > 0 || own.columns == 0 ? own.rows : 1;

			own.object.groupName = name;
			if (own.map)
			{
				own.position.x = own.map->position.x;
				own.position.y = own.map->position.y;
				readMapMembers(group, out, own, name, where);
				return;
			}

			std::vector<std::unique_ptr<XmlNode>> children;
			for (std::unique_ptr<XmlNode> child = group.getFirstChild(); child != nullptr;)
			{
				std::unique_ptr<XmlNode> next = child->getNextSibling();
				children.push_back(std::move(child));
				child = std::move(next);
			}

			if (columns == 0)
			{
				readMembers(children, out, group, own, name, where);
				return;
			}

			std::vector<GroupPart> lines;
			std::vector<GroupPart> cells;
			readLinesAndCells(children, columns, rows, where, lines, cells);
			placeCells(group, out, own, lines, cells, columns, rows, name, where);
		}

		// A <map name="maze">: the size of a <tile>, where its top left is (its
		// <position>), the <row>s of characters, one to a tile, and then the
		// groups and objects on it. A group on it (or inside a group on it)
		// picks a character with char="." and has a member on each tile of it,
		// in the middle of the tile; an object picks one used on one tile only,
		// where it starts. A character nothing picks is an empty tile. The
		// map's name means everything on it. Rows are trimmed like any text, so
		// an empty tile at either end of a row needs a character, not a space.
		// The tag read; readMap without the line.
		void readMapHere(const XmlNode& node, std::vector<WrittenObject>& out);

		void readMap(const XmlNode& node, std::vector<WrittenObject>& out)
		{
			return onLineOf(node, [&] { return readMapHere(node, out); });
		}

		void readMapHere(const XmlNode& node, std::vector<WrittenObject>& out)
		{
			MapLayout map;
			map.name = node.getAttribute("name");
			const std::string where = "map '" + map.name + "'";
			refuseNumber(map.name, "a map", where);
			const Children children(node);

			const XmlNode* tile = children.find("tile");
			const XmlNode* position = children.find("position");
			if (!tile) { fail(where, "missing <tile>"); }
			if (!position) { fail(where, "missing <position>"); }
			map.tileWidth = readValueOf(*tile, "width", where + " > <tile>");
			map.tileHeight = readValueOf(*tile, "height", where + " > <tile>");
			map.position = readVector2(*position, where);
			if (auto columns = children.find("columns")) { map.columns = readCount(*columns, where); }
			if (auto rows = children.find("rows")) { map.rowCount = readCount(*rows, where); }
			map.cells = children.find("cells") != nullptr;
			if (const XmlNode* cells = children.find("cells"))
			{
				const std::string here = where + " > <cells>";
				if (findChild(cells, "width")) { map.cellWidth = readWholeChild(*cells, "width", here); }
				if (findChild(cells, "height")) { map.cellHeight = readWholeChild(*cells, "height", here); }
				if (map.cellWidth > 1 && map.cellHeight > 1) { fail(here, "has a <width> and a <height>; a cell's pieces stack one way, so give one"); }
			}
			if (auto checks = children.find("checks"))
			{
				if (!map.cells) { fail(where, "has <checks>, which ask about a map with <cells />"); }
				map.checks = readChecks(*checks, where);
			}

			for (const auto& [tag, child] : children)
			{
				if (tag != "row") { continue; }
				const std::string row = readText(*child);
				const std::string here = where + " > <row> " + std::to_string(map.rows.size() + 1);
				if (row.empty()) { fail(here, "is empty"); }
				if (!map.rows.empty() && row.size() != map.rows.front().size())
				{
					fail(here, "is " + std::to_string(row.size()) + " tiles wide, but row 1 is " + std::to_string(map.rows.front().size()) + "; every row is as wide as the map");
				}
				map.rows.push_back(row);
			}
			if ((map.columns > 0) != (map.rowCount > 0)) { fail(where, "has <columns> or <rows> and not the other; a map without <row>s needs both"); }
			if (map.rows.empty() && map.columns == 0) { fail(where, "has no <row>s, and no <columns> and <rows>"); }
			if (!map.rows.empty() && map.columns > 0) { fail(where, "has <row>s and <columns> and <rows>; its rows say how big it is, so give one or the other"); }
			if (map.across() % map.cellWidth != 0 || map.down() % map.cellHeight != 0)
			{
				fail(where, "is " + std::to_string(map.across()) + " by " + std::to_string(map.down()) + " tiles, which are not whole cells of "
					+ std::to_string(map.cellWidth) + " by " + std::to_string(map.cellHeight));
			}

			std::map<char, std::string> picked;
			map.picked = &picked;
			const std::size_t first = out.size();

			GroupShared outer;
			outer.map = &map;
			bool any = false;
			for (const auto& [tag, child] : children)
			{
				if (tag == "group")
				{
					readGroup(*child, out, outer);
					any = true;
				}
				else if (tag == "object")
				{
					out.push_back(readObject(*child, &map));
					any = true;
				}
			}
			if (!any && map.cells) { out.push_back(mapHolder(map)); }
			else if (!any) { fail(where, "has nothing on it; give it <group>s and <object>s that pick its characters"); }

			pickShapePieces(out, first);
		}

		// A <note> or <rest> of a sound, in the part given; a note with no
		// wave of its own takes the part's (`wave`, "" for the sound's).
		WrittenNote readNote(const XmlNode& node, const std::string& where, unsigned part, const std::string& wave, const WrittenValue& decay)
		{
			WrittenNote note;
			note.part = part;
			if (node.getName() == "rest")
			{
				note.rest = true;
			}
			else
			{
				note.wave = node.getAttribute("wave");
				if (note.wave.empty()) { note.wave = wave; }
				note.pitch = node.getAttribute("pitch");
				note.to = node.getAttribute("to");
				note.decay = decay;
			}
			note.length = readValue(node, where);
			return note;
		}

		// A <sound>, as written: its words (wave, pitch) are checked, and its
		// lengths worked out, by load/expr.
		// The tag read; readSound without the line.
		WrittenSound readSoundHere(const XmlNode& sound);

		WrittenSound readSound(const XmlNode& sound)
		{
			return onLineOf(sound, [&] { return readSoundHere(sound); });
		}

		WrittenSound readSoundHere(const XmlNode& sound)
		{
			WrittenSound written;
			written.name = sound.getAttribute("name");
			refuseNumber(written.name, "a sound", "<sound name=\"" + written.name + "\">");
			written.wave = sound.getAttribute("wave");

			const std::string where = "sound '" + written.name + "'";

			// The schema holds the order (<loop />, <volume>, <decay>) and that
			// a sound is <note>s and <rest>s, <part>s or one <path>, never two
			// of them, and a part a <decay> then one or more notes.
			unsigned parts = 0;
			WrittenValue decay;     // the sound's <decay>, every note's that its part does not change
			bool decayed = false;
			for (std::unique_ptr<XmlNode> child = sound.getFirstChild(); child != nullptr; child = child->getNextSibling())
			{
				const std::string tag = child->getName();

				if (tag == "loop") { written.loop = true; }
				else if (tag == "volume") { written.volume = readValue(*child, where); }
				else if (tag == "decay")
				{
					decay = readValue(*child, where);
					decayed = true;
				}
				else if (tag == "path")
				{
					if (decayed) { fail(where, "plays a recording (<path>), so it has no <decay>"); }
					if (!written.wave.empty()) { fail(where, "plays a recording (<path>), so it has no wave="); }
					written.path = readText(*child);
					if (written.path.empty()) { fail(where, "has an empty <path>"); }
				}
				else if (tag == "note" || tag == "rest")
				{
					written.notes.push_back(readNote(*child, where, 0, "", decay));
				}
				else if (tag == "part")
				{
					const std::string partWhere = where + " > <part> " + std::to_string(parts + 1);
					const std::string wave = child->getAttribute("wave");
					WrittenValue partDecay = decay;
					for (std::unique_ptr<XmlNode> step = child->getFirstChild(); step != nullptr; step = step->getNextSibling())
					{
						if (step->getName() == "decay") { partDecay = readValue(*step, partWhere); }
						else { written.notes.push_back(readNote(*step, partWhere, parts, wave, partDecay)); }
					}
					++parts;
				}
			}
			return written;
		}

		// A state's <inputs> or a <keys> set: what each key runs at each of
		// its moments (in the order of keyMoments), and the controls that point.
		struct KeyBindings
		{
			std::array<std::map<std::string, std::vector<WrittenCommand>>, keyMoments.size()> buttons;
			std::map<std::string, std::vector<WrittenCommand>> axes;
		};

		// The names a node gives one way or the other: one by an attribute
		// (button="space"), several by an element each, written first
		// (<button>space</button><button>p</button>). `rest` is left at the
		// first child that is not one of them.
		std::vector<std::string> namesGiven(const XmlNode& node, const std::string& word, const std::string& where, std::unique_ptr<XmlNode>* rest = nullptr)
		{
			std::vector<std::string> names;
			const std::string attribute = node.getAttribute(word);
			if (!attribute.empty())
			{
				if (attribute.find_first_of(" \t\n") != std::string::npos)
				{
					fail(where, word + "=\"" + attribute + "\" names several; write each as a <" + word + "> of its own");
				}
				names.push_back(attribute);
			}

			std::unique_ptr<XmlNode> child = node.getFirstChild();
			for (; child != nullptr && child->getName() == word; child = child->getNextSibling())
			{
				if (!attribute.empty()) { fail(where, "has " + word + "=\"" + attribute + "\" and a <" + word + ">; use one or the other"); }
				const std::string name = readText(*child);
				if (name.empty()) { fail(where, "has an empty <" + word + ">"); }
				names.push_back(name);
			}
			if (rest) { *rest = std::move(child); }
			return names;
		}

		// The <input>s of a state's <inputs>: a key
		// (button="a"), or several, a <button> each, or a control that points
		// (axis="mouse.x"), and the commands each one runs. One given twice
		// keeps the later binding.
		void readInputs(const XmlNode& node, const std::string& where, KeyBindings& out)
		{
			for (std::unique_ptr<XmlNode> input = node.getFirstChild(); input != nullptr; input = input->getNextSibling())
			{
				std::unique_ptr<XmlNode> commandsStart;
				const std::vector<std::string> buttons = namesGiven(*input, "button", where + " > <input>", &commandsStart);
				const std::string axis = input->getAttribute("axis");
				if (buttons.empty() == axis.empty())
				{
					fail(where, "an <input> names a button (button=\"...\", or a <button> each) or an axis=\"...\", one of the two");
				}
				const std::string on = input->getAttribute("on");
				if (!axis.empty())
				{
					if (!on.empty()) { fail(where + " > <input axis=\"" + axis + "\">", "on=\"" + on + "\" is for a button; a control that points runs as it moves"); }
					if (out.axes.count(axis)) { fail(where + " > <input axis=\"" + axis + "\">", "binds axis '" + axis + "' again: this state's <inputs> already has it"); }
					out.axes[axis] = readCommands(input->getFirstChild(), where + " > <input axis=\"" + axis + "\">");
					continue;
				}

				std::string written;
				for (const std::string& button : buttons) { written += (written.empty() ? "" : " ") + button; }
				const auto moment = on.empty() ? keyMoments.begin()
					: std::find_if(keyMoments.begin(), keyMoments.end(), [&](const char* name) { return on == name; });
				const std::vector<WrittenCommand> commands = readKeyCommands(std::move(commandsStart), where + " > <input " + written + ">");
				auto& to = out.buttons[static_cast<std::size_t>(moment - keyMoments.begin())];
				for (const std::string& button : buttons)
				{
					if (to.count(button)) { fail(where + " > <input " + written + ">", "binds '" + button + "' again: this state's <inputs> already has it" + (on.empty() ? std::string() : " on=\"" + on + "\"")); }
					to[button] = commands;
				}
			}
		}

		// A state's <show>s, in order: <show object="..."> names one object (or
		// group), <show state="..."> a state drawn still underneath.
		void readShows(const XmlNode& node, const std::string& where, std::vector<std::string>& out, std::vector<std::string>& states)
		{
			for (std::unique_ptr<XmlNode> show = node.getFirstChild(); show != nullptr; show = show->getNextSibling())
			{
				const std::string object = show->getAttribute("object");
				const std::string stateName = show->getAttribute("state");
				if (object.empty() == stateName.empty()) { fail(where, "a <show> names an object=\"...\" or a state=\"...\", one of them"); }
				if (!stateName.empty()) { states.push_back(stateName); }
				else { out.push_back(object); }
			}
		}

		// An <any>, into `written`: its <condition>s and <any>s in order, one or
		// more (the schema).
		void readJoin(const XmlNode& join, const std::string& here, WrittenCondition& written)
		{
			written.line = join.line();
			written.join = WrittenCondition::Join::Any;
			for (std::unique_ptr<XmlNode> part = join.getFirstChild(); part != nullptr; part = part->getNextSibling())
			{
				const std::string partName = part->getName();
				WrittenCondition check;
				if (partName == "condition")
				{
					const std::string checkHere = here + " > <condition>";
					if (readCheck(*part, checkHere, check)) { fail(checkHere, "has its one test and nothing after it; the commands go at the end of the <condition>"); }
				}
				else
				{
					readJoin(*part, here + " > <" + partName + ">", check);
				}
				written.parts.push_back(std::move(check));
			}
		}

		// What a <condition> tests, into `written`: its filters and
		// what it measures (attributes), then its test tag, or an <all> or
		// <any> of <condition>s. Gives back what follows (a condition's commands).
		std::unique_ptr<XmlNode> readCheck(const XmlNode& node, const std::string& here, WrittenCondition& written)
		{
			written.line = node.line();
			written.filterClass = node.getAttribute("class");
			written.filterObject = node.getAttribute("object");
			if (const std::string who = readWho(node, here); !who.empty())
			{
				if (!written.filterObject.empty()) { fail(here, "has object= and who=; who= says which object, so give one"); }
				written.filterObject = who;
			}
			written.variableName = node.getAttribute("variable");
			written.total = node.getAttribute("total");
			written.distance = node.getAttribute("distance");
			written.sprite = node.getAttribute("sprite");

			std::unique_ptr<XmlNode> test = node.getFirstChild();

			// <sees object>: the line from each one to the nearest of that
			// crosses nothing of the classes its <past>s name.
			if (test && test->getName() == "sees")
			{
				const std::string seesHere = here + " > <sees>";
				if (!written.variableName.empty() || !written.total.empty() || !written.distance.empty())
				{
					fail(here, "<sees> looks along a line, so the condition has no variable=, total= or distance=");
				}
				written.test = WrittenCondition::Test::Sees;
				// One or more <past class>es (the schema).
				written.sees = test->getAttribute("object");
				for (std::unique_ptr<XmlNode> past = test->getFirstChild(); past != nullptr; past = past->getNextSibling())
				{
					written.past.push_back(past->getAttribute("class"));
				}
				return test->getNextSibling();
			}

			// No test tag: with sprite=, showing that look is the test; with
			// class= or object= alone, being there is (each of them, for
			// commands that name self).
			static const std::set<std::string> tests{ "atleast", "atmost", "above", "below", "equal", "unequal", "remaining", "lowest", "highest", "all", "any" };
			const bool named = !written.filterClass.empty() || !written.filterObject.empty();
			if (named && (!test || !tests.contains(test->getName()))
				&& (!written.sprite.empty() || (written.variableName.empty() && written.total.empty() && written.distance.empty())))
			{
				if (!written.variableName.empty() || !written.total.empty() || !written.distance.empty())
				{
					fail(here, "has variable=, total= or distance= but no test of them; add an <atleast>, <atmost>, <above>, <below> or <equal>");
				}
				written.test = WrittenCondition::Test::Shows;
				return test;
			}
			if (!test) { fail(here, "needs an <atleast>, <atmost>, <above>, <below>, <equal>, <unequal> or <remaining>, or an <any>"); }

			const std::string testName = test->getName();

			// <any>: several checks, each a test of its own.
			if (testName == "any")
			{
				if (!written.filterClass.empty() || !written.filterObject.empty() || !written.variableName.empty() || !written.total.empty() || !written.distance.empty() || !written.sprite.empty())
				{
					fail(here, "<" + testName + "> says what is tested in each of its <condition>s, so the condition has no class=, object=, variable=, total=, distance= or sprite=");
				}
				readJoin(*test, here + " > <" + testName + ">", written);
				return test->getNextSibling();
			}

			if (testName == "atleast") { written.test = WrittenCondition::Test::AtLeast; }
			else if (testName == "atmost") { written.test = WrittenCondition::Test::AtMost; }
			else if (testName == "above") { written.test = WrittenCondition::Test::Above; }
			else if (testName == "below") { written.test = WrittenCondition::Test::Below; }
			else if (testName == "equal") { written.test = WrittenCondition::Test::Equal; }
			else if (testName == "unequal") { written.test = WrittenCondition::Test::Unequal; }
			else if (testName == "remaining") { written.test = WrittenCondition::Test::Remaining; }
			else if (testName == "lowest") { written.test = WrittenCondition::Test::Lowest; }
			else if (testName == "highest") { written.test = WrittenCondition::Test::Highest; }
			else { fail(here, "starts with <" + testName + ">; expected <atleast>, <atmost>, <above>, <below>, <equal>, <unequal>, <remaining>, <lowest /> or <highest />, or an <any>"); }

			// <lowest /> and <highest />: the one whose variable is lowest (or highest) of all those matched.
			if (written.test == WrittenCondition::Test::Lowest || written.test == WrittenCondition::Test::Highest)
			{
				if (!written.total.empty()) { fail(here, "<" + testName + " /> holds for one of those matched, so it takes no total= (total=\"" + testName + "\" is the number itself)"); }
				if (written.variableName.empty() && written.distance.empty()) { fail(here, "<" + testName + " /> compares a variable, so it needs variable=\"...\""); }
				return test->getNextSibling();
			}

			const bool remaining = written.test == WrittenCondition::Test::Remaining;
			if (written.total == "count" && !written.distance.empty()) { fail(here, "total=\"count\" counts the objects in play, so it measures no distance="); }
			if (remaining && (!written.total.empty() || !written.distance.empty())) { fail(here, "<remaining> counts what is in play, so it takes no total= or distance="); }
			if (!written.distance.empty() && !written.variableName.empty()) { fail(here, "distance=\"" + written.distance + "\" measures how far, so it reads no variable="); }
			if (written.total == "count" && !written.variableName.empty()) { fail(here, "total=\"count\" counts the objects in play, so it reads no variable="); }
			if (!remaining && written.variableName.empty() && written.total != "count" && written.distance.empty())
			{
				fail(here, "<" + testName + "> reads a variable, so it needs variable=\"...\"");
			}

			// The number compared with, which may name another variable
			// (<above>gun2.score</above>), read as the game runs.
			written.threshold = readValue(*test, here);
			return test->getNextSibling();
		}

		// One <condition> into `out`: a test, then the commands to run when it
		// holds; or, holding conditions (each read the same way), a <first>
		// before them and an <otherwise> after.
		void readConditionHere(const XmlNode& condition, const std::string& where, std::vector<WrittenCondition>& out);

		void readCondition(const XmlNode& condition, const std::string& where, std::vector<WrittenCondition>& out)
		{
			onLineOf(condition, [&] { readConditionHere(condition, where, out); });
		}

		// The tag read; readCondition without the line.
		void readConditionHere(const XmlNode& condition, const std::string& where, std::vector<WrittenCondition>& out)
		{
			const std::string conditionHere = where + " > <condition>";
			WrittenCondition written;
			written.name = condition.getAttribute("name");
			if (!written.name.empty() && std::any_of(out.begin(), out.end(), [&](const WrittenCondition& other) { return other.name == written.name; }))
			{
				fail(conditionHere, "has two conditions called '" + written.name + "'");
			}
			std::unique_ptr<XmlNode> after = readCheck(condition, conditionHere, written);

			// Holding conditions: <first>, the conditions, <otherwise>, and no
			// commands loose beside them, so which run when is written.
			bool holding = false;
			for (std::unique_ptr<XmlNode> each = condition.getFirstChild(); each; each = each->getNextSibling())
			{
				if (each->getName() == "condition") { holding = true; }
			}
			if (!holding)
			{
				for (std::unique_ptr<XmlNode> each = condition.getFirstChild(); each; each = each->getNextSibling())
				{
					if (each->getName() == "first" || each->getName() == "otherwise")
					{
						fail(conditionHere, "has a <" + each->getName() + "> but holds no conditions; its commands go in it directly");
					}
				}
				written.commands = readCommands(std::move(after), conditionHere);
				out.push_back(std::move(written));
				return;
			}
			std::unique_ptr<XmlNode> part = std::move(after);
			if (part && part->getName() == "first")
			{
				written.first = readCommands(part->getFirstChild(), conditionHere + " > <first>");
				part = part->getNextSibling();
			}
			for (; part && part->getName() == "condition"; part = part->getNextSibling())
			{
				readCondition(*part, conditionHere, written.inner);
			}
			if (part && part->getName() == "otherwise")
			{
				written.commands = readCommands(part->getFirstChild(), conditionHere + " > <otherwise>");
				part = part->getNextSibling();
			}
			if (part)
			{
				fail(conditionHere, "holds conditions, so <" + part->getName() + "> is out of place; a condition holding conditions has a <first> before them, run as soon as it holds, and an <otherwise> after, run when none of them does");
			}
			out.push_back(std::move(written));
		}

		// The <condition>s under a node (a state's <conditions>, or a
		// <conditions> set), in order.
		void readConditions(const XmlNode& node, const std::string& where, std::vector<WrittenCondition>& out)
		{
			for (std::unique_ptr<XmlNode> condition = node.getFirstChild(); condition != nullptr; condition = condition->getNextSibling())
			{
				readCondition(*condition, where, out);
			}
		}

		// Objects laid out by load/merge, each read as if it had been written
		// out whole there: an <object>, a <group> or a <map>.
		void readAll(const std::vector<ElementPtr>& list, std::vector<WrittenObject>& out)
		{
			for (const ElementPtr& element : list)
			{
				const std::unique_ptr<XmlNode> node = viewOf(element);
				if (element->name == "group") { readGroup(*node, out); }
				else if (element->name == "map") { readMap(*node, out); }
				else { out.push_back(readObject(*node)); }
			}
		}

		// What a <spawn> or <fire> in those children of a state or
		// scene names (load/merge spawnedIn): a piece of the top-level <objects> named so
		// comes into the list they run beside (addWaiting).
		std::set<std::string> spawnedBy(const Children& children, const std::vector<std::string>& tags, const Kit& kit)
		{
			std::set<std::string> names;
			for (const std::string& tag : tags)
			{
				if (const XmlNode* child = children.find(tag))
				{
					const std::set<std::string> more = spawnedIn(elementOf(*child), kit);
					names.insert(more.begin(), more.end());
				}
			}
			return names;
		}

		std::set<std::string> namesOf(const std::vector<ElementPtr>& list, std::set<std::string> names)
		{
			for (const ElementPtr& element : list)
			{
				const std::set<std::string> own = namesIn(element);
				names.insert(own.begin(), own.end());
			}
			return names;
		}

		// <scenes>: each <scene>, its objects laid out by load/merge: those of
		// the scene it is from=, changed by its own of the same names, then its
		// own and the top-level <objects> pieces it takes. `seen` is every name of its state's
		// objects and the game's.
		void readScenes(const XmlNode& scenes, const std::string& stateWhere, const Kit& kit, const std::set<std::string>& seen, WrittenState& state)
		{
			// Each scene so far, laid out, for a scene that is from= it.
			std::map<std::string, std::vector<ElementPtr>> lists;

			for (std::unique_ptr<XmlNode> node = scenes.getFirstChild(); node != nullptr; node = node->getNextSibling())
			{
				WrittenScene scene;
				scene.name = node->getAttribute("name");
				scene.from = node->getAttribute("from");
				const std::string where = stateWhere + " > scene '" + scene.name + "'";
				const Children parts(*node);
				if (lists.count(scene.name)) { fail(stateWhere, "has two scenes called '" + scene.name + "'"); }
				if (scene.name.find('.') != std::string::npos) { fail(where, "a scene's name has no dots: they join the parts of a path (" + state.name + "." + scene.name + ")"); }

				std::vector<ElementPtr> list;
				if (!scene.from.empty())
				{
					const auto from = lists.find(scene.from);
					if (from == lists.end()) { fail(where, "from=\"" + scene.from + "\" names no scene before it"); }
					list = from->second;
				}

				if (const XmlNode* ways = parts.find("ways"))
				{
					for (std::unique_ptr<XmlNode> way = ways->getFirstChild(); way != nullptr; way = way->getNextSibling())
					{
						if (way->getName() != "way") { continue; }
						const std::string wayWhere = where + " > <ways>";
						const std::string name = way->getAttribute("name");
						const std::string to = way->getAttribute("scene");
						const bool twice = std::any_of(scene.ways.begin(), scene.ways.end(), [&](const auto& each) { return each.first == name; });
						if (twice) { fail(wayWhere, "has two ways called '" + name + "'"); }
						scene.ways.emplace_back(name, to);
					}
				}

				readVariables(parts.find("variables"), where + " > <variables>", scene.variables);

				if (const XmlNode* objects = parts.find("objects"))
				{
					list = layOut(*objects, std::move(list), kit, where + " > <objects>");
				}

				if (const XmlNode* conditions = parts.find("conditions"))
				{
					readConditions(*conditions, where, scene.conditions);
				}
				scene.timers = readTimers(parts.find("timers"), where, false);

				addWaiting(list, spawnedBy(parts, { "conditions", "timers" }, kit), seen, kit, where);
				readAll(list, scene.objects);

				lists[scene.name] = std::move(list);
				state.scenes.push_back(std::move(scene));
			}
		}

		// What comes after a state's <inputs>: its <conditions> (optional - the
		// schema allows a state with none) and its <timers> (optional: counted
		// while this is the current state).
		WrittenState finishState(const Children& children, const std::string& where, WrittenState writtenState)
		{
			if (const XmlNode* conditions = children.find("conditions"))
			{
				readConditions(*conditions, where, writtenState.conditions);
			}
			if (const XmlNode* computer = children.find("computer"))
			{
				readConditions(*computer, where + " > <computer>", writtenState.computer);
				// Every one that holds holds its buttons, so one holding conditions has no "otherwise".
				const std::function<void(const std::vector<WrittenCondition>&)> plain = [&](const std::vector<WrittenCondition>& list)
				{
					for (const WrittenCondition& each : list)
					{
						if (!each.inner.empty() && !each.commands.empty())
						{
							fail(where + " > <computer> > <condition>", "has an <otherwise>; in <computer> every condition that holds holds its buttons, so buttons for when none does go in a condition of their own");
						}
						plain(each.inner);
					}
				};
				plain(writtenState.computer);
			}
			writtenState.timers = readTimers(children.find("timers"), where, false);
			return writtenState;
		}

		// A <state>. Its objects are laid out by load/merge: those of the state
		// it is from= (`lists`, each state so far laid out), changed by its own
		// of the same names, then its own and the top-level <objects> pieces it takes.
		// The tag read; readState without the line.
		WrittenState readStateHere(const XmlNode& state, std::map<std::string, std::vector<ElementPtr>>& lists, const Kit& kit, const std::set<std::string>& gameNames);

		WrittenState readState(const XmlNode& state, std::map<std::string, std::vector<ElementPtr>>& lists, const Kit& kit, const std::set<std::string>& gameNames)
		{
			return onLineOf(state, [&] { return readStateHere(state, lists, kit, gameNames); });
		}

		WrittenState readStateHere(const XmlNode& state, std::map<std::string, std::vector<ElementPtr>>& lists, const Kit& kit, const std::set<std::string>& gameNames)
		{
			WrittenState writtenState{};
			writtenState.line = state.line();
			writtenState.name = state.getAttribute("name");
			refuseNumber(writtenState.name, "a state", "<state name=\"" + writtenState.name + "\">");
			writtenState.from = state.getAttribute("from");

			const std::string where = "state '" + writtenState.name + "'";
			const Children children(state);

			std::vector<ElementPtr> list;
			if (!writtenState.from.empty())
			{
				const auto from = lists.find(writtenState.from);
				if (from == lists.end()) { fail(where, "from=\"" + writtenState.from + "\" names no state before it"); }
				list = from->second;
			}

			// <objects>: its own, in play in every scene of it
			if (const XmlNode* objects = children.find("objects"))
			{
				list = layOut(*objects, std::move(list), kit, where + " > <objects>");
			}

			// <starts> (optional): the deal, done when it begins
			if (const XmlNode* starts = children.find("starts"))
			{
				for (std::unique_ptr<XmlNode> step = starts->getFirstChild(); step != nullptr; step = step->getNextSibling())
				{
					WrittenStart start;
					const std::string tag = step->getName();
					start.step = tag == "send" ? WrittenStart::Step::Send : tag == "distribute" ? WrittenStart::Step::Distribute : WrittenStart::Step::Order;
					const std::string at = where + " > <starts> > <" + tag + ">";
					if (start.step != WrittenStart::Step::Order)
					{
						start.from = step->getAttribute("from");
						start.into = step->getAttribute("into");
						if (findChild(step.get(), "all")) { start.all = true; }
						else
						{
							const WrittenValue count = readLeadValue(*step, { "more" }, start.step == WrittenStart::Step::Send ? "how many, or <all />" : "how many each, then <more> if wanted", at);
							try { start.count = std::stoi(count.text); }
							catch (const std::exception&) { fail(at, "is \"" + count.text + "\"; expected a whole number"); }
						}
						if (auto more = findChild(step.get(), "more"))
						{
							start.more = static_cast<int>(readPlainNumber(*more, at + " > <more>"));
						}
					}
					else { start.from = step->getAttribute("group"); }
					writtenState.starts.push_back(std::move(start));
				}
			}

			// <shows> (optional): objects of the game's, named <shows> sets
			// taken whole, and states drawn still underneath
			if (const XmlNode* shows = children.find("shows"))
			{
				readShows(*shows, where + " > <shows>", writtenState.show, writtenState.showStates);
			}

			// <music sound="..." /> (optional)
			if (const XmlNode* music = children.find("music"))
			{
				writtenState.music = music->getAttribute("sound");
			}

			// <inputs>: its own <input>s, over those of the state it is from=.
			const XmlNode* inputs = children.find("inputs");
			if (!inputs && writtenState.from.empty()) { fail(where, "missing <inputs>"); }
			if (inputs)
			{
				KeyBindings own;
				readInputs(*inputs, where + " > <inputs>", own);
				for (std::size_t moment = 0; moment < keyMoments.size(); ++moment)
				{
					for (auto& [button, commands] : own.buttons[moment]) { (*writtenState.keyMaps()[moment])[button] = std::move(commands); }
				}
				for (auto& [axis, commands] : own.axes) { writtenState.axes[axis] = std::move(commands); }
			}
			writtenState = finishState(children, where, std::move(writtenState));

			// What its own inputs, conditions and timers spawn (a piece of the top-level <objects>
			// comes into its objects), then its scenes, then its objects read.
			addWaiting(list, spawnedBy(children, { "inputs", "conditions", "computer", "timers" }, kit), gameNames, kit, where);

			if (const XmlNode* scenes = children.find("scenes"))
			{
				readScenes(*scenes, where, kit, namesOf(list, gameNames), writtenState);
			}

			readAll(list, writtenState.objects);
			lists[writtenState.name] = std::move(list);
			return writtenState;
		}
	}

	namespace
	{
		// A <demo> as --record writes it: its seed, its length in seconds,
		// and each key pressed and let go with the time it happened at.
		WrittenDemo readDemoElement(const XmlNode& demoNode)
		{
			WrittenDemo demo;
			const std::string demoWhere = "<demo>";
			const auto whole = [&](const XmlNode& node, const std::string& here)
			{
				const std::string text = readText(node);
				if (text.empty() || text.find_first_not_of("0123456789") != std::string::npos)
				{
					fail(here, "\"" + text + "\" is not a whole number");
				}
				return static_cast<unsigned int>(std::stoul(text));
			};
			const auto seconds = [&](const XmlNode& node, const std::string& here)
			{
				const std::string text = readText(node);
				std::size_t used = 0;
				double value = -1;
				try { value = std::stod(text, &used); }
				catch (const std::exception&) { used = 0; }
				if (text.empty() || used != text.size() || value < 0)
				{
					fail(here, "\"" + text + "\" is not a time in seconds");
				}
				return value;
			};
			demo.recorded = true;
			demo.seed = whole(*requireChild(demoNode, "seed", demoWhere), demoWhere + " > <seed>");
			demo.seconds = seconds(*requireChild(demoNode, "seconds", demoWhere), demoWhere + " > <seconds>");
			for (std::unique_ptr<XmlNode> child = demoNode.getFirstChild(); child != nullptr; child = child->getNextSibling())
			{
				const std::string tag = child->getName();
				if (tag != "press" && tag != "release") { continue; }
				const std::string here = demoWhere + " > <" + tag + ">";
				DemoKey key;
				key.button = requireAttribute(*child, "button", here);
				key.pressed = tag == "press";
				key.seconds = seconds(*child, here);
				if (keyCodeFromString(key.button) == KeyCode::Unknown)
				{
					fail(here, "there is no key named '" + key.button + "'" + didYouMean(key.button, keyNames()));
				}
				demo.keys.push_back(std::move(key));
			}
			return demo;
		}
	}

	namespace
	{
		// <window name="..."> and its settings. These are plain numbers, not
		// expressions - the expressions in the rest of the file are worked out
		// against the window's size.
		void readWindow(const XmlNode& window, WindowDesc& windowDesc)
		{
			// The schema holds the sizes to whole pixels from 1 and the scale
			// and frame rate to whole numbers from 1.
			windowDesc.name = window.getAttribute("name");
			windowDesc.width = std::stof(readText(*findChild(&window, "width")));
			windowDesc.height = std::stof(readText(*findChild(&window, "height")));
			if (std::unique_ptr<XmlNode> scale = findChild(&window, "scale")) { windowDesc.scale = std::stoi(readText(*scale)); }
			// Left out: black, a window, 60 frames a second.
			const auto background = findChild(&window, "background");
			const auto fullscreen = findChild(&window, "fullscreen");
			const auto framerate = findChild(&window, "framerate");
			windowDesc.background = background ? readText(*background) : "black";
			windowDesc.fullscreen = fullscreen ? "true" : "false";
			windowDesc.framerate = framerate ? std::stoi(readText(*framerate)) : 60;
		}

		// <styles> (optional): what a <text> or a <bitmap> can take by style="...".
		void readStyles(const Children& top, std::vector<WrittenStyle>& writtenStyles)
		{
			if (const XmlNode* stylesNode = top.find("styles"))
			{
				for (std::unique_ptr<XmlNode> style = stylesNode->getFirstChild(); style != nullptr; style = style->getNextSibling())
				{
					WrittenStyle written;
					written.name = style->getAttribute("name");
					const std::string here = "<styles> > <style name=\"" + written.name + "\">";
					if (std::any_of(writtenStyles.begin(), writtenStyles.end(), [&](const WrittenStyle& each) { return each.name == written.name; }))
					{
						fail("<styles>", "two <style>s are called '" + written.name + "'");
					}
					if (auto size = findChild(style.get(), "size")) { written.size = readValue(*size, here); }
					if (auto scale = findChild(style.get(), "scale"))
					{
						written.scale = readValue(*scale, here);
					}
					if (auto color = findPlainColor(*style)) { written.color = readText(*color); }
					readBitmapColors(*style, here, written.bitmapColors);
					if (auto font = findChild(style.get(), "font")) { written.font = readText(*font); }
					writtenStyles.push_back(std::move(written));
				}
			}
		}

		// The <sheet>s among the <pictures> (optional): the pictures sprites,
		// animations and packs cut theirs from.
		void readSheets(const Children& top, std::vector<Sheet>& writtenSheets)
		{
			if (const XmlNode* picturesNode = top.find("pictures"))
			{
				for (std::unique_ptr<XmlNode> node = picturesNode->getFirstChild(); node != nullptr; node = node->getNextSibling())
				{
					if (node->getName() != "sheet") { continue; }
					Sheet sheet;
					sheet.name = node->getAttribute("name");
					const std::string here = "<pictures> > <sheet name=\"" + sheet.name + "\">";
					if (std::any_of(writtenSheets.begin(), writtenSheets.end(), [&](const Sheet& each) { return each.name == sheet.name; }))
					{
						fail("<pictures>", "two <sheet>s are called '" + sheet.name + "'");
					}
					sheet.path = readText(*findChild(node.get(), "path"));
					// A number the schema has read; it cannot say from 0.
					const auto plain = [&](const XmlNode& parent, const char* tag, const std::string& at)
					{
						const auto part = findChild(&parent, tag);
						if (!part) { return 0.0f; }
						const float value = std::stof(readText(*part));
						if (value < 0.0f) { fail(at + " > <" + tag + ">", "is " + readText(*part) + "; expected a number from 0"); }
						return value;
					};
					// <columns> and <rows> come together, or a <cell> size instead (the schema).
					if (auto columns = findChild(node.get(), "columns")) { sheet.columns = readCount(*columns, here); }
					if (auto rows = findChild(node.get(), "rows")) { sheet.rows = readCount(*rows, here); }
					if (auto cell = findChild(node.get(), "cell"))
					{
						sheet.cellWidth = plain(*cell, "width", here + " > <cell>");
						sheet.cellHeight = plain(*cell, "height", here + " > <cell>");
						if (!(sheet.cellWidth > 0.0f && sheet.cellHeight > 0.0f)) { fail(here + " > <cell>", "needs a <width> and a <height> above 0"); }
					}
					sheet.margin = plain(*node, "margin", here);
					sheet.gap = plain(*node, "gap", here);
					for (std::unique_ptr<XmlNode> hide = node->getFirstChild(); hide != nullptr; hide = hide->getNextSibling())
					{
						if (hide->getName() == "hide") { sheet.hide.push_back(readText(*hide)); }
					}
					writtenSheets.push_back(std::move(sheet));
				}
			}
		}

		// <patterns> (optional): what a holder's <score patterns="..."> is scored by.
		void readPatterns(const Children& top, std::vector<WrittenPatterns>& writtenPatterns)
		{
			for (const auto& [name, node] : top)
			{
				if (name != "patterns") { continue; }
				WrittenPatterns written;
				written.name = node->getAttribute("name");
				const std::string here = "<patterns name=\"" + written.name + "\">";
				if (std::any_of(writtenPatterns.begin(), writtenPatterns.end(), [&](const WrittenPatterns& each) { return each.name == written.name; }))
				{
					fail("<patterns>", "two <patterns> are called '" + written.name + "'");
				}
				for (std::unique_ptr<XmlNode> pattern = node->getFirstChild(); pattern != nullptr; pattern = pattern->getNextSibling())
				{
					WrittenPatterns::Pattern one;
					one.name = pattern->getAttribute("name");
					const std::string at = here + " > <pattern name=\"" + one.name + "\">";
					// One or more, each an <alike> or a <run> (the schema).
					for (std::unique_ptr<XmlNode> part = pattern->getFirstChild(); part != nullptr; part = part->getNextSibling())
					{
						const std::string tag = part->getName();
						// The variable compared: <alike> one value of it, <run> values in a row.
						const WrittenPatterns::Part kind = tag == "run" ? WrittenPatterns::Part::Run : WrittenPatterns::Part::Alike;
						one.by.push_back(part->getAttribute("variable"));
						one.parts.emplace_back(kind, readCount(*part, at + " > <" + tag + ">"));
					}
					written.patterns.push_back(std::move(one));
				}
				writtenPatterns.push_back(std::move(written));
			}
		}

		// The states, each read with what it takes from the pieces.
		void readStates(const XmlNode& states, std::vector<WrittenState>& writtenStates, Kit& kit, const std::set<std::string>& gameNames)
		{
			std::map<std::string, std::vector<ElementPtr>> stateLists;
			for (std::unique_ptr<XmlNode> state = states.getFirstChild(); state != nullptr; state = state->getNextSibling())
			{
				if (state->getName() != "state") { continue; }
				const std::string name = state->getAttribute("name");
				if (std::any_of(writtenStates.begin(), writtenStates.end(), [&](const WrittenState& each) { return each.name == name; }))
				{
					fail("<states>", "has two states called '" + name + "'");
				}
				writtenStates.push_back(readState(*state, stateLists, kit, gameNames));
			}
		}
	}

	GameData readGame(const XmlNode& file)
	{
		GameData data;
		WindowDesc& windowDesc = data.window;
		std::vector<std::pair<std::string, WrittenValue>>& writtenVariables = data.variables;
		std::vector<WrittenState>& writtenStates = data.states;
		std::vector<WrittenObject>& writtenObjects = data.objects;
		std::vector<WrittenSound>& writtenSounds = data.sounds;
		std::vector<WrittenPath>& writtenPaths = data.paths;
		std::vector<WrittenStyle>& writtenStyles = data.styles;
		std::vector<Sheet>& writtenSheets = data.sheets;
		std::vector<WrittenPatterns>& writtenPatterns = data.patterns;
		std::vector<std::string>& writtenKeeps = data.keeps;
		WrittenDemo& writtenDemo = data.demo;

		// The pictures put in where sprites take them (<bitmap from>), so the
		// rest reads each as if written out there.
		const ElementPtr pictured = withPictures(copyElement(file));
		const std::unique_ptr<XmlNode> root = viewOf(pictured);
		const Children top(*root);

		const XmlNode* window = top.find("window");
		const XmlNode* variablesNode = top.find("variables");
		const XmlNode* piecesNode = top.find("objects");  // none: nothing is taken
		const XmlNode* states = top.find("states");
		std::unique_ptr<XmlNode> objectsNode = findChild(states, "objects"); // none: every object is a state's own

		readWindow(*window, windowDesc);

		// load variables
		readVariables(variablesNode, "<variables>", writtenVariables);

		// load sounds (optional: a game can be silent)
		if (const XmlNode* soundsNode = top.find("sounds"))
		{
			for (std::unique_ptr<XmlNode> sound = soundsNode->getFirstChild(); sound != nullptr; sound = sound->getNextSibling())
			{
				writtenSounds.push_back(readSound(*sound));
			}
		}

		// load paths (optional: only a game whose objects fly them has any)
		if (const XmlNode* pathsNode = top.find("paths"))
		{
			for (std::unique_ptr<XmlNode> path = pathsNode->getFirstChild(); path != nullptr; path = path->getNextSibling())
			{
				writtenPaths.push_back(readPath(*path));
			}
		}

		readStyles(top, writtenStyles);
		readSheets(top, writtenSheets);
		readPatterns(top, writtenPatterns);

		// load the pieces, the top-level <objects>: written once, in play
		// only where something takes them (load/merge)
		Kit kit;
		if (piecesNode)
		{
			for (std::unique_ptr<XmlNode> piece = piecesNode->getFirstChild(); piece != nullptr; piece = piece->getNextSibling())
			{
				kit.add(elementOf(*piece), "the top-level <objects>");
			}
		}
		if (const XmlNode* pathsNode = top.find("paths"))
		{
			for (std::unique_ptr<XmlNode> path = pathsNode->getFirstChild(); path != nullptr; path = path->getNextSibling())
			{
				kit.addSet("path", path->getAttribute("name"), elementOf(*path));
			}
		}

		// load objects: the game's own (<states> > <objects>), in play in
		// every state that shows them
		std::vector<ElementPtr> gameList;
		if (objectsNode) { gameList = layOut(*objectsNode, {}, kit, "<states> > <objects>"); }
		addWaiting(gameList, {}, {}, kit, "<states> > <objects>");
		readAll(gameList, writtenObjects);
		const std::set<std::string> gameNames = namesOf(gameList, {});

		// load keeps (optional: what a full reset passes over)
		if (const XmlNode* keepsNode = top.find("keeps"))
		{
			for (std::unique_ptr<XmlNode> keep = keepsNode->getFirstChild(); keep != nullptr; keep = keep->getNextSibling())
			{
				writtenKeeps.push_back(keep->getAttribute("object"));
			}
		}

		// load the demo (optional: a recorded game for the attract mode)
		if (const XmlNode* demoNode = top.find("demo"))
		{
			writtenDemo = readDemoElement(*demoNode);
		}

		readStates(*states, writtenStates, kit, gameNames);
		return data;
	}

	WrittenDemo readDemo(const XmlNode& demo)
	{
		return readDemoElement(demo);
	}
}
