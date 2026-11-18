// resources.cpp
// XML Game Engine
// author: beefviper
// date: Nov 17, 2026

#include "engine/resources.h"
#include "picture/image.h"

#include <string>

namespace xge
{
	PictureId Resources::pictureFile(const std::string& path)
	{
		return picture("file|" + path, [&] { return readPictureFile(path); });
	}

	PictureId Turnable::turned(int degrees) const
	{
		PictureId& known = asked[degrees];
		if (!known) { known = resources->picture(key + "|turned " + std::to_string(degrees), [&] { return at(static_cast<float>(degrees)); }); }
		return known;
	}

	PictureId Resources::flippedLeftRight(PictureId picture)
	{
		const std::string key = "flipped|" + std::to_string(picture.index());
		const bool fresh = byKey.find(key) == byKey.end();
		const PictureId made = this->picture(key, [&] { return flipBitmap(*picture, true, false); });
		if (fresh) { byKey.emplace("flipped|" + std::to_string(made.index()), picture); }
		return made;
	}

	PictureId Resources::own(Bitmap made)
	{
		return keep(std::make_shared<const Bitmap>(std::move(made)));
	}

	void Resources::replace(PictureId owned, Bitmap made)
	{
		if (owned) { held(owned).bitmap = std::make_shared<const Bitmap>(std::move(made)); }
	}

	void Resources::release(PictureId owned)
	{
		if (owned) { held(owned).bitmap.reset(); }
	}

	std::size_t Resources::pictureCount() const noexcept
	{
		std::size_t count = 0;
		for (const PictureId::Held& each : pictures)
		{
			if (each.bitmap) { ++count; }
		}
		return count;
	}

	std::size_t Resources::pictureBytes() const noexcept
	{
		std::size_t bytes = 0;
		for (const PictureId::Held& each : pictures)
		{
			if (each.bitmap) { bytes += each.bitmap->rgba.size() + each.bitmap->touching.size(); }
		}
		return bytes;
	}

	PictureId Resources::keep(std::shared_ptr<const Bitmap> made)
	{
		pictures.push_back({ std::move(made), static_cast<std::uint32_t>(pictures.size() + 1) });
		return PictureId(&pictures.back());
	}
}
