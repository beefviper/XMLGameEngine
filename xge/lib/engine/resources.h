// resources.h
// XML Game Engine
// author: beefviper
// date: Nov 17, 2026

#pragma once

#include "picture/bitmap.h"

#include <cstddef>
#include <cstdint>
#include <deque>
#include <map>
#include <memory>
#include <string>
#include <utility>

namespace xge
{
	// Everything a game loads or makes once and shares, each held here once
	// however many objects show it, and named by its PictureId. One a Game:
	// two games open side by side hold their own.
	class Resources
	{
	public:
		Resources() = default;
		Resources(const Resources&) = delete;
		Resources& operator=(const Resources&) = delete;
		// Moved whole, each picture stays where it is, so its ids stay good.
		Resources(Resources&&) = default;
		Resources& operator=(Resources&&) = default;

		// The picture file at `path`, read the first time it is asked for
		// (readPictureFile), the same id after that.
		PictureId pictureFile(const std::string& path);

		// The picture described by `key` (everything that decides its pixels,
		// as text), drawn by `draw` the first time and the same id after.
		template <typename Draw>
		PictureId picture(const std::string& key, Draw&& draw)
		{
			if (const auto found = byKey.find(key); found != byKey.end()) { return found->second; }
			const PictureId made = keep(std::make_shared<const Bitmap>(draw()));
			byKey.emplace(key, made);
			return made;
		}

		// The picture flipped left to right (a <mirror>'s), made once; the
		// flip of that is the picture again.
		PictureId flippedLeftRight(PictureId picture);

		// One object's own picture, which nothing else shows (a picture with
		// holes cut in it, a tether's line): drawn again in place by
		// `replace`, and let go by `release` when the object is done with it
		// (it then reads as empty). Only for what own() gave.
		PictureId own(Bitmap made);
		void replace(PictureId owned, Bitmap made);
		void release(PictureId owned);

		const Bitmap& bitmap(PictureId id) const { return *id; }

		// The picture as a shared pointer, for what still takes one (a
		// window's texture, until it keeps them by id).
		std::shared_ptr<const Bitmap> shared(PictureId id) const { return id ? held(id).bitmap : nullptr; }

		// How many pictures are held, and the bytes of their pixels.
		std::size_t pictureCount() const noexcept;
		std::size_t pictureBytes() const noexcept;

	private:
		PictureId keep(std::shared_ptr<const Bitmap> made);
		PictureId::Held& held(PictureId id) { return pictures[id.index() - 1]; }
		const PictureId::Held& held(PictureId id) const { return pictures[id.index() - 1]; }

		std::deque<PictureId::Held> pictures;   // [0] is id 1
		std::map<std::string, PictureId> byKey;
	};
}
