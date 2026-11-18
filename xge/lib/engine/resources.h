// resources.h
// XML Game Engine
// author: beefviper
// date: Nov 17, 2026

#pragma once

#include "picture/bitmap.h"

#include <cstddef>
#include <cstdint>
#include <map>
#include <memory>
#include <string>
#include <utility>
#include <vector>

namespace xge
{
	// A picture held by a game's Resources. 0 is none; an id is never given
	// to another picture while the game is loaded.
	struct PictureId
	{
		std::uint32_t index{};

		explicit operator bool() const noexcept { return index != 0; }
		friend bool operator==(PictureId, PictureId) = default;
	};

	// Everything a game loads or makes once and shares, each held here once
	// however many objects show it. One a Game: two games open side by side
	// hold their own.
	class Resources
	{
	public:
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

		const Bitmap& bitmap(PictureId id) const { return *pictures.at(id.index); }

		// The picture flipped left to right (a <mirror>'s), made once; the
		// flip of that is the picture again. One not held yet is held from
		// now on.
		std::shared_ptr<const Bitmap> flippedLeftRight(const std::shared_ptr<const Bitmap>& picture);

		// The picture as an object holds it today (CR2 step 4 replaces this
		// with the id).
		const std::shared_ptr<const Bitmap>& shared(PictureId id) const { return pictures.at(id.index); }

		// How many pictures are held, and the bytes of their pixels.
		std::size_t pictureCount() const noexcept { return pictures.size() - 1; }
		std::size_t pictureBytes() const noexcept;

	private:
		PictureId keep(std::shared_ptr<const Bitmap> made);

		std::vector<std::shared_ptr<const Bitmap>> pictures{ nullptr };   // [0] is none
		std::map<std::string, PictureId> byKey;
		std::map<const Bitmap*, PictureId> byPicture;
	};
}
