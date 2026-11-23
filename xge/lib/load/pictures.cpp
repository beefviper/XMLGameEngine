// pictures.cpp
// XML Game Engine
// author: beefviper
// date: Nov 15, 2026
//
// Pictures made when the game loads: a sprite's Picture and Bitmap
// (Evaluator::buildPicture), a text's pieces, animation frames and their
// times, flipped copies, palettes in other styles, and where the cells of
// a group laid out in columns and rows go.

#include "load/expr.h"
#include "load/calc.h"
#include "load/check.h"
#include "load/reading.h"
#include "picture/color.h"
#include "engine/keys.h"
#include "load/spelling.h"
#include "picture/svg.h"
#include "play/numbers.h"
#include "audio/wave.h"
#include <algorithm>
#include <charconv>
#include <iterator>
#include <functional>
#include <initializer_list>
#include <cctype>
#include <cmath>
#include <cstdlib>
#include <memory>
#include <optional>
#include <set>
#include <stdexcept>

namespace xge
{
	namespace
	{
		// A sprite as it is with another style's colors in place of the
		// ones it took from its own (see WrittenSprite::styleChars); its own stay.
		void restyle(WrittenSprite& sprite, const WrittenStyle& style)
		{
			if (sprite.kind == "bitmap" && !sprite.style.empty())
			{
				if (sprite.styleColor && !style.color.empty()) { sprite.color = style.color; }
				for (auto& [character, color] : sprite.bitmapColors)
				{
					if (sprite.styleChars.find(character) == std::string::npos) { continue; }
					const auto given = std::find_if(style.bitmapColors.begin(), style.bitmapColors.end(), [&](const auto& each) { return each.first == character; });
					if (given != style.bitmapColors.end()) { color = given->second; }
				}
			}
			for (WrittenSprite& frame : sprite.frames) { restyle(frame, style); }
		}

		// A number in a picture's key, exactly: two keys are the same only
		// when every number in them is.
		std::string keyNumber(float value)
		{
			char text[32];
			const auto written = std::to_chars(std::begin(text), std::end(text), value);
			return std::string(text, written.ptr);
		}

		std::string colorKey(const Color& color)
		{
			return std::to_string(color.r) + "." + std::to_string(color.g) + "." + std::to_string(color.b) + "." + std::to_string(color.a);
		}

		bool takesStyle(const WrittenSprite& sprite)
		{
			return (sprite.kind == "bitmap" && !sprite.style.empty()) || std::any_of(sprite.frames.begin(), sprite.frames.end(), takesStyle);
		}
	}

	void Evaluator::makePalettes(const std::vector<WrittenObject>& writtenObjects, std::vector<Object>& objects, const std::map<std::string, std::vector<std::string>>& wanted)
	{
		for (Object& object : objects)
		{
			const auto asked = wanted.find(object.name);
			if (asked == wanted.end()) { continue; }
			const auto written = std::find_if(writtenObjects.begin(), writtenObjects.end(), [&](const WrittenObject& each) { return each.name == object.name; });
			if (written == writtenObjects.end()) { continue; }

			const std::string where = "object '" + object.name + "'";
			if (!object.drawn.turnables.empty())
			{
				throw std::runtime_error(where + ": <become style=\"" + asked->second.front() + "\" />: a palette is for a picture that does not turn; it has a <heading>");
			}
			if (!takesStyle(written->sprite) && std::none_of(written->looks.begin(), written->looks.end(), takesStyle)
				&& std::none_of(written->animation.frames.begin(), written->animation.frames.end(), takesStyle))
			{
				throw std::runtime_error(where + ": <become style=\"" + asked->second.front() + "\" />: it has no <bitmap style=\"...\"> whose colors another style could change");
			}
			if (object.looks.all().size() != written->looks.size()) { continue; }

			// As loaded: the colors it starts in, and a reset puts back.
			Object::Palette loaded;
			loaded.looks = object.looks.all();
			loaded.bitmap = object.looks.all().empty() ? object.facingPicture(object.drawn.bitmap) : object.looks.all().front().bitmap;
			if (object.looks.all().empty()) { loaded.animation = object.animation.frames.shows().bitmaps; }
			object.looks.set->palettes.push_back(std::move(loaded));

			for (const std::string& name : asked->second)
			{
				const WrittenStyle& style = *std::find_if(styles.begin(), styles.end(), [&](const WrittenStyle& each) { return each.name == name; });
				const auto restyled = [&](WrittenSprite sprite) { restyle(sprite, style); return sprite; };
				const std::string here = where + " in style '" + name + "'";

				Object::Palette palette;
				palette.name = name;
				if (!object.looks.all().empty())
				{
					palette.looks = object.looks.all();
					for (std::size_t i = 0; i < palette.looks.size(); ++i)
					{
						const WrittenSprite look = restyled(written->looks[i]);
						buildPicture(look, here, &palette.looks[i].bitmap);
						if (!look.frames.empty()) { palette.looks[i].frames.edit().bitmaps = buildAnimationBitmaps(look.frames, here); }
					}
					palette.bitmap = palette.looks.front().bitmap;
				}
				else
				{
					buildPicture(restyled(written->sprite), here, &palette.bitmap);
					if (!object.animation.frames.shows().bitmaps.empty())
					{
						std::vector<WrittenSprite> frames;
						for (const WrittenSprite& frame : written->animation.frames) { frames.push_back(restyled(frame)); }
						palette.animation = buildAnimationBitmaps(frames, here);
					}
				}
				object.looks.set->palettes.push_back(std::move(palette));
			}
			if (object.mirror.on) { object.looks.set->flips = flipsOf(object); }
		}
	}

	std::vector<TextPiece> Evaluator::textPieces(const WrittenSprite& sprite, const std::string& where, const WrittenObject* self)
	{
		std::vector<TextPiece> pieces;
		for (const WrittenTextPiece& written : sprite.text)
		{
			TextPiece piece;
			piece.kind = written.kind;
			if (written.kind == WrittenTextPiece::Kind::Words)
			{
				piece.shown = written.words;
				pieces.push_back(std::move(piece));
				continue;
			}

			if (written.value.kind == WrittenValue::Kind::Expression)
			{
				if (const auto reference = parseVariableReference(written.value.text))
				{
					std::string owner = reference->first;
					if (self && owner == "@self") { owner = self->name; }
					if (self && self->inGroup(owner) && objectVariables.count(self->name + "." + reference->second))
					{
						owner = self->name;
					}
					if (const auto found = objectVariables.find(owner + "." + reference->second); found != objectVariables.end())
					{
						piece.owner = owner;
						piece.variable = reference->second;
						piece.choices = written.choices;
						piece.shown = showPiece(written.kind, found->second, piece.choices);
						pieces.push_back(std::move(piece));
						continue;
					}
					// A holder's live numbers follow it too (shown 0 until dealt).
					if (reference->second == "cards" || reference->second == "held" || reference->second == "points" || reference->second == "called" || reference->second == "calling"
						|| reference->second == "pattern" || reference->second == "strength" || reference->second == "odds"
						|| reference->second == "lead" || reference->second == "winner" || reference->second == "tricks" || reference->second == "deadwood")
					{
						piece.owner = owner;
						piece.variable = reference->second;
						piece.choices = written.choices;
						piece.shown = showPiece(written.kind, 0.0f, piece.choices);
						pieces.push_back(std::move(piece));
						continue;
					}
				}
			}

			piece.choices = written.choices;
			piece.shown = showPiece(written.kind, evaluate(written.value, where), piece.choices);
			pieces.push_back(std::move(piece));
		}
		return pieces;
	}

	namespace
	{
		// A size that cannot be negative (0 is an object that only holds numbers).
		float atLeastZero(float value, const std::string& where)
		{
			if (value < 0.0f) { throw std::runtime_error(where + " is " + formatDisplayNumber(value) + "; expected a number of pixels, 0 or more"); }
			return value;
		}
	}

	Picture Evaluator::buildPicture(const WrittenSprite& sprite, const std::string& where,
		PictureId* bitmap, std::shared_ptr<const Turnable>* turnable, const WrittenObject* self)
	{
		const std::string color = sprite.color.empty() ? "white" : sprite.color;
		checkColor(color, where);
		Picture picture;
		picture.color = Name::of(color);

		// Lines, rows of text and SVG drawings are all pictures the engine draws
		// itself, and all go the same way, here, once, when the game loads:
		//   1. read what the file describes (line ends, rows, a part of a drawing);
		//   2. draw it into a Bitmap, the pixels every window backend shows and a
		//      pixel collision tests, so no backend ever draws one itself;
		//   3. flip it, if the sprite says so (a <bitmap> or an <svg>), the two
		//      steps done once for all the sprites that are the same picture;
		//   4. for an object with a <heading>, keep it (a Turnable) to be turned
		//      to whatever heading it faces, and start it at heading 0.
		// The Picture carries only the drawing's size. The one difference is in the
		// turning: lines are kept as lines and drawn again at each heading, which
		// keeps their edges sharp, where a finished picture's pixels are turned.
		if (sprite.kind == "line" || sprite.kind == "polygon" || sprite.kind == "bitmap" || sprite.kind == "svg" || sprite.kind == "cell")
		{
			auto kept = std::make_shared<Turnable>();
			kept->resources = &resources;
			PictureId drawn;

			try
			{
				drawn = drawPicture(sprite, where, color, kept);


				// 4. Kept to be turned, for an object that faces somewhere; heading 0
				// is what it starts as, and the square it turns in is its size.
				if (turnable)
				{
					if (kept->lines.empty()) { kept->picture = drawn; }
					if (!kept->lines.empty() || drawn->width > 0)
					{
						drawn = kept->turned(0);
					}
					*turnable = std::move(kept);
				}
			}
			catch (const std::exception& error)
			{
				// std::invalid_argument is a mistake in the numbers, rows or
				// lines, std::runtime_error an svg file that cannot be read;
				// either way it is the game file that is at fault, so say where.
				throw std::runtime_error(where + ": " + error.what());
			}

			picture.kind = ShapeKind::Line;
			picture.size = { static_cast<float>(drawn->width), static_cast<float>(drawn->height) };
			if (bitmap) { *bitmap = std::move(drawn); }
		}
		else if (sprite.kind == "circle")
		{
			picture.kind = ShapeKind::Circle;
			picture.radius = atLeastZero(evaluate(sprite.radius, where), where + " > <circle> > <radius>");
		}
		else if (sprite.kind == "rectangle")
		{
			picture.kind = ShapeKind::Rectangle;
			picture.size.x = atLeastZero(evaluate(sprite.width, where), where + " > <rectangle> > <width>");
			picture.size.y = atLeastZero(evaluate(sprite.height, where), where + " > <rectangle> > <height>");
		}
		else if (sprite.kind == "text")
		{
			// Its pieces: fixed labels, numbers shown as the whole numbers they
			// are, with no decimal point, the way someone typing the label
			// would, and letters.
			if (sprite.text.empty())
			{
				throw std::runtime_error(where + " > <text>: needs a <content> (a label), a <number>, a <letter> or a <choose>; only a group's text can leave them to its members, rows or cells");
			}
			picture.kind = ShapeKind::Text;
			picture.text = joinText(textPieces(sprite, where, self));
			const float size = evaluate(sprite.size, where);
			if (!(size > 0.0f)) { throw std::runtime_error(where + " > <text> > <size> is " + formatDisplayNumber(size) + "; expected a number of pixels above 0"); }
			picture.textSize = static_cast<int>(size);
			picture.font = Name::of(sprite.font);
		}
		else if (sprite.kind == "image")
		{
			picture.kind = ShapeKind::Image;
			picture.file = Name::of(sprite.path);
			picture.flipHorizontal = sprite.flip == "horizontal";
			picture.flipVertical = sprite.flip == "vertical";
			try
			{
				PictureId file = resources.pictureFile(sprite.path);
				if (picture.flipHorizontal || picture.flipVertical)
				{
					file = resources.picture("file|" + sprite.path + "|" + sprite.flip,
						[&] { return flipBitmap(resources.bitmap(file), picture.flipHorizontal, picture.flipVertical); });
				}
				picture.image = file;
			}
			catch (const std::exception& error) { throw std::runtime_error(where + ": " + error.what()); }
		}
		else if (sprite.kind == "tether")
		{
			// Drawn every frame where its two ends are (drawTethers); until
			// then, one clear pixel.
			const PictureId clear = resources.picture("clear pixel", []
			{
				Bitmap pixel;
				pixel.width = 1;
				pixel.height = 1;
				pixel.rgba.assign(4, 0);
				return pixel;
			});
			picture.kind = ShapeKind::Line;
			picture.size = { 1.0f, 1.0f };
			if (bitmap) { *bitmap = clear; }
		}
		else
		{
			throw std::runtime_error(where + ": unknown shape '" + sprite.kind + "'");
		}

		return picture;
	}


	// buildPicture's steps 1 to 3 for a line, polygon, cell, svg or bitmap:
	// read, drawn into a Bitmap and flipped; a line's segments kept to turn.
	PictureId Evaluator::drawPicture(const WrittenSprite& sprite, const std::string& where, const std::string& color, const std::shared_ptr<Turnable>& kept)
	{
		PictureId drawn;
		// 3. Flipped.
		const auto flipped = [&](Bitmap unflipped)
		{
			return sprite.flip.empty() ? unflipped : flipBitmap(unflipped, sprite.flip == "horizontal", sprite.flip == "vertical");
		};
		if (sprite.kind == "line")
		{
			// 1. The line ends, colours and thicknesses.
			for (const WrittenLine& writtenLine : sprite.lines)
			{
				LineSegment segment;
				segment.x1 = evaluate(writtenLine.from.x, where);
				segment.y1 = evaluate(writtenLine.from.y, where);
				segment.x2 = evaluate(writtenLine.to.x, where);
				segment.y2 = evaluate(writtenLine.to.y, where);
				checkColor(writtenLine.color.empty() ? "white" : writtenLine.color, where + " > <line>");
				segment.color = colorFromName(writtenLine.color.empty() ? "white" : writtenLine.color);
				if (writtenLine.thickness) { segment.thickness = static_cast<int>(std::lround(evaluate(*writtenLine.thickness, where))); }
				if (segment.thickness < 1) { throw std::invalid_argument("a <line> has a <thickness> under 1"); }
				kept->lines.push_back(segment);
			}

			// 2. Drawn, once for every sprite of the same lines. (Kept as
			// lines for turning: see step 4.)
			std::string key = "lines";
			for (const LineSegment& line : kept->lines)
			{
				key += "|" + keyNumber(line.x1) + "," + keyNumber(line.y1) + "," + keyNumber(line.x2) + "," + keyNumber(line.y2)
					+ "," + std::to_string(line.thickness) + "," + colorKey(line.color);
			}
			drawn = drawOnce(key, [&] { return rasterizeLines(kept->lines); });
			kept->key = key;
		}
		else if (sprite.kind == "polygon")
		{
			// 1. The corners and colours; 2. drawn, filled. Turned as a
			// finished picture, pixel by pixel, and holed as one.
			std::vector<FilledPolygon> polygons;
			for (const WrittenPolygon& writtenPolygon : sprite.polygons)
			{
				FilledPolygon polygon;
				for (const WrittenVector2& point : writtenPolygon.points)
				{
					polygon.points.emplace_back(evaluate(point.x, where), evaluate(point.y, where));
				}
				checkColor(writtenPolygon.color.empty() ? "white" : writtenPolygon.color, where + " > <polygon>");
				polygon.color = colorFromName(writtenPolygon.color.empty() ? "white" : writtenPolygon.color);
				polygons.push_back(std::move(polygon));
			}
			std::string key = "polygons";
			for (const FilledPolygon& polygon : polygons)
			{
				key += "|" + colorKey(polygon.color);
				for (const auto& [x, y] : polygon.points) { key += ";" + keyNumber(x) + "," + keyNumber(y); }
			}
			drawn = drawOnce(key, [&] { return rasterizePolygons(polygons); });
			kept->key = key;
		}
		else if (sprite.kind == "cell")
		{
			// 1. Which sheet, which cell or part of it, at what size.
			const auto sheet = std::find_if(sheets.begin(), sheets.end(), [&](const Sheet& each) { return each.name == sprite.sheet; });
			if (sheet == sheets.end()) { throw std::invalid_argument("<cell sheet=\"" + sprite.sheet + "\">: the game has no <sheet name=\"" + sprite.sheet + "\">"); }
			const auto given = [](const WrittenValue& value) { return !(value.kind == WrittenValue::Kind::Expression && value.text.empty()); };
			SheetCell cell;
			std::string key = "cell|" + sheet->name + "|";
			if (sprite.svgRegion)
			{
				cell.free = true;
				cell.x = evaluate(sprite.svgRegion->x, where);
				cell.y = evaluate(sprite.svgRegion->y, where);
				cell.width = evaluate(sprite.svgRegion->width, where);
				cell.height = evaluate(sprite.svgRegion->height, where);
				key += keyNumber(cell.x) + "," + keyNumber(cell.y) + "," + keyNumber(cell.width) + "," + keyNumber(cell.height);
			}
			else
			{
				cell.column = given(sprite.column) ? static_cast<int>(std::lround(evaluate(sprite.column, where))) : 1;
				cell.row = given(sprite.row) ? static_cast<int>(std::lround(evaluate(sprite.row, where))) : 1;
				key += std::to_string(cell.column) + "/" + std::to_string(cell.row);
			}
			const bool sized = given(sprite.width) && given(sprite.height);
			const int toWidth = sized ? static_cast<int>(std::lround(evaluate(sprite.width, where))) : 0;
			const int toHeight = sized ? static_cast<int>(std::lround(evaluate(sprite.height, where))) : 0;
			const float cellScale = sprite.scale ? evaluate(*sprite.scale, where) : 1.0f;
			key += sized ? "|" + std::to_string(toWidth) + "x" + std::to_string(toHeight) : "|*" + keyNumber(cellScale);
			key += "|" + sprite.flip;

			// 2. Cut, once for every sprite of the same cell and size (a
			// a pack's items, a group's members), from the sheet's picture,
			// opened once while the game loads.
			drawn = drawOnce(key, [&]
			{
				const SheetPicture& opened = resources.sheet(*sheet);
				const SheetPart part = opened.partOf(cell);
				return flipped(sized ? opened.cut(part, toWidth, toHeight) : opened.cut(part, cellScale));
			});
			kept->key = key;
		}
		else if (sprite.kind == "svg")
		{
			// 1. The part of the drawing, and how many pixels to a unit.
			SvgRegion region;
			float svgScale = 1.0f;
			if (sprite.svgRegion)
			{
				region = { evaluate(sprite.svgRegion->x, where), evaluate(sprite.svgRegion->y, where),
					evaluate(sprite.svgRegion->width, where), evaluate(sprite.svgRegion->height, where) };
				if (region.isWhole()) { throw std::invalid_argument("an svg's <width> and <height> must both be above 0"); }
			}
			if (sprite.scale) { svgScale = evaluate(*sprite.scale, where); }

			// 2. Drawn by lunasvg (picture/svg.cpp), once for every sprite that
			// takes the same part at the same scale (a group's cells).
			std::string key = "svg|" + sprite.path + "|" + keyNumber(region.x) + "," + keyNumber(region.y) + ","
				+ keyNumber(region.width) + "," + keyNumber(region.height) + "|" + keyNumber(svgScale) + "|" + sprite.flip;
			for (const std::string& hide : sprite.svgHide) { key += "|" + hide; }
			drawn = drawOnce(key, [&] { return flipped(rasterizeSvg(sprite.path, region, svgScale, sprite.svgHide)); });
			kept->key = key;
		}
		else
		{
			// 1. The rows, and how many pixels to a character.
			const int rowsScale = sprite.scale ? static_cast<int>(std::lround(evaluate(*sprite.scale, where))) : 1;

			// 2. Drawn, once for every sprite of the same rows, scale and
			// colors (a group's cells).
			std::map<char, Color> inks;
			std::string key = "rows|" + std::to_string(rowsScale) + "|" + color + "|" + sprite.flip;
			for (const auto& [character, ink] : sprite.bitmapColors)
			{
				checkColor(ink, where + " > <color char=\"" + std::string(1, character) + "\">");
				inks[character] = colorFromName(ink);
				key += std::string("|") + character + "=" + ink;
			}
			for (const std::string& row : sprite.bitmapRows) { key += "|" + row; }
			if (!sprite.solid.empty()) { key += "|solid=" + sprite.solid; }
			drawn = drawOnce(key, [&] { return flipped(rasterizeRows(sprite.bitmapRows, rowsScale, colorFromName(color), inks, sprite.solid)); });
			kept->key = key;
		}
		return drawn;
	}
	std::vector<PictureId> Evaluator::buildAnimationBitmaps(const std::vector<WrittenSprite>& frames, const std::string& where,
		std::vector<std::shared_ptr<const Turnable>>* turnables)
	{
		std::vector<PictureId> pictures;

		for (std::size_t i = 0; i < frames.size(); ++i)
		{
			const WrittenSprite& frame = frames[i];
			const std::string here = where + " > frame " + std::to_string(i + 1)
				+ (frame.name.empty() ? std::string{} : " (\"" + frame.name + "\")");

			if (frame.kind != "bitmap" && frame.kind != "svg" && frame.kind != "line" && frame.kind != "cell")
			{
				throw std::runtime_error(here + ": a frame must be a <bitmap>, an <svg>, a sheet's <cell> or a drawing of <line>s, not a <" + frame.kind + ">");
			}

			PictureId picture;
			std::shared_ptr<const Turnable> turnable;
			buildPicture(frame, here, &picture, turnables ? &turnable : nullptr);

			if (i > 0)
			{
				if (picture->width != pictures.front()->width || picture->height != pictures.front()->height)
				{
					throw std::runtime_error(here + ": is " + std::to_string(picture->width) + " by " + std::to_string(picture->height)
						+ " pixels, but frame 1 is " + std::to_string(pictures.front()->width) + " by " + std::to_string(pictures.front()->height)
						+ "; the frames of an animation must all be the same size");
				}
			}

			pictures.push_back(std::move(picture));
			if (turnables) { turnables->push_back(std::move(turnable)); }
		}

		return pictures;
	}

	std::shared_ptr<const std::map<PictureId, PictureId>> Evaluator::flipsOf(const Object& object)
	{
		auto flips = std::make_shared<std::map<PictureId, PictureId>>();
		const auto add = [&](const PictureId& picture)
		{
			if (!picture || flips->count(picture)) { return; }
			const PictureId flipped = resources.flippedLeftRight(picture);
			(*flips)[picture] = flipped;
			(*flips)[flipped] = picture;
		};
		const auto addLooks = [&](const std::vector<Object::Look>& looks)
		{
			for (const Object::Look& look : looks)
			{
				add(look.bitmap);
				for (const auto& picture : look.frames.shows().bitmaps) { add(picture); }
			}
		};
		add(object.drawn.bitmap);
		for (const auto& picture : object.animation.frames.shows().bitmaps) { add(picture); }
		addLooks(object.looks.all());
		for (const Object::Palette& palette : object.looks.set->palettes)
		{
			add(palette.bitmap);
			for (const auto& picture : palette.animation) { add(picture); }
			addLooks(palette.looks);
		}
		return flips;
	}

	float Evaluator::animationSecondsOf(const WrittenValue& interval, const std::string& where)
	{
		// A live one may read its own object's numbers, not made yet: it is
		// timed as the game starts, before anything is drawn (Game::follow).
		if (interval.live) { return 0.0f; }

		const float seconds = evaluate(interval, where + " > <seconds>");
		if (!(seconds > 0.0f))
		{
			throw std::runtime_error(where + " > <seconds> is " + std::to_string(seconds) + "; expected a number of seconds above 0");
		}
		return seconds;
	}

	std::vector<float> Evaluator::animationTimesOf(const std::vector<WrittenValue>& times, float fallback, const std::string& where)
	{
		std::vector<float> frames;
		for (std::size_t i = 0; i < times.size(); ++i)
		{
			frames.push_back(frameTimeGiven(times[i])
				? animationSecondsOf(times[i], where + " > <frame> " + std::to_string(i + 1))
				: fallback);
		}
		return frames;
	}

	std::vector<std::vector<Command>> Evaluator::frameCommandsOf(const std::vector<std::vector<WrittenCommand>>& frames, const std::string& where)
	{
		std::vector<std::vector<Command>> made;
		for (std::size_t i = 0; i < frames.size(); ++i)
		{
			made.push_back(processCommands(frames[i], where + " > <frame> " + std::to_string(i + 1)));
		}
		return made;
	}

	namespace
	{
		// A picture's size where cells are laid out: a text counts as no size,
		// so rows of text are spaced by their <padding> alone, top to top (lines
		// of words), and each sits at its place's top left; a shape is as
		// written, not rounded.
		Vector2f layoutSize(const Picture& picture)
		{
			return picture.kind == ShapeKind::Text ? Vector2f{} : measureShapeSize(picture);
		}
	}

	std::map<std::string, Vector2f> Evaluator::layOutCells(const std::vector<WrittenObject>& writtenObjects,
		const std::map<std::string, Picture>& pictures)
	{
		// Each group's cells by row, in the order they were read: a row's
		// columns left to right.
		std::map<std::string, std::map<int, std::vector<const WrittenObject*>>> groups;
		for (const WrittenObject& writtenObject : writtenObjects)
		{
			if (writtenObject.cell.column > 0) { groups[writtenObject.groupName][writtenObject.cell.row].push_back(&writtenObject); }
		}

		std::map<std::string, Vector2f> offsets;

		// What a <map> placed: in the middle of its tile, counted from the
		// map's top left, which is the object's own <position>.
		for (const WrittenObject& writtenObject : writtenObjects)
		{
			if (writtenObject.cell.mapColumn == 0) { continue; }
			const std::string where = "object '" + writtenObject.name + "' (on map '" + writtenObject.cell.map + "')";
			const Vector2f tile{ evaluate(writtenObject.cell.tileWidth, where + " > <tile> > <width>"), evaluate(writtenObject.cell.tileHeight, where + " > <tile> > <height>") };
			if (!(tile.x > 0.0f) || !(tile.y > 0.0f)) { throw std::runtime_error(where + ": a <tile> is " + formatDisplayNumber(tile.x) + " by " + formatDisplayNumber(tile.y) + "; both have to be above 0"); }
			// By its largest look: one that starts showing none (a cherry
			// that comes later) still shows in the middle when it comes.
			Vector2f own = layoutSize(pictures.at(writtenObject.name));
			for (const WrittenSprite& look : writtenObject.looks)
			{
				const Vector2f size = layoutSize(buildPicture(look, where, nullptr, nullptr, &writtenObject));
				own = { std::max(own.x, size.x), std::max(own.y, size.y) };
			}
			const float across = static_cast<float>(writtenObject.cell.mapColumn - 1) + (writtenObject.cell.between ? 0.5f : 0.0f);
			offsets[writtenObject.name] = { across * tile.x + (tile.x - own.x) / 2.0f,
				static_cast<float>(writtenObject.cell.mapRow - 1) * tile.y + (tile.y - own.y) / 2.0f };
		}

		for (const auto& [group, rows] : groups)
		{
			float top = 0.0f;
			float above = 0.0f; // the height of the row above
			for (const auto& [row, cells] : rows)
			{
				const WrittenObject& first = *cells.front();
				const std::string where = "object '" + first.name + "'";

				// The slot is the row's sprite, so a cell with a sprite of its
				// own of another size sits in the middle of the place its row
				// gives it.
				const Picture slotPicture = buildPicture(first.cell.slot, where, nullptr, nullptr, &first);
				const Vector2f slot = layoutSize(slotPicture);

				if (row > rows.begin()->first) { top += above + evaluate(first.cell.gapAbove, where); }
				above = slot.y;

				float left = 0.0f;
				for (const WrittenObject* cell : cells)
				{
					const std::string here = "object '" + cell->name + "'";
					if (cell->cell.column > 1) { left += slot.x + evaluate(cell->cell.gapBefore, here); }

					const Vector2f own = layoutSize(pictures.at(cell->name));
					// A <slope>: each column that much lower than the one before.
					const float drop = cell->cell.drop.text.empty() && cell->cell.drop.kind == WrittenValue::Kind::Expression ? 0.0f
						: evaluate(cell->cell.drop, here + " > <slope>") * static_cast<float>(cell->cell.column - 1);
					offsets[cell->name] = { left + (slot.x - own.x) / 2.0f, top + drop + (slot.y - own.y) / 2.0f };
				}
			}
		}

		return offsets;
	}
}
