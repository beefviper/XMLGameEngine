// resources.cpp
// XML Game Engine
// author: beefviper
// date: Nov 17, 2026

#include "engine/resources.h"
#include "picture/image.h"

namespace xge
{
	PictureId Resources::pictureFile(const std::string& path)
	{
		return picture("file|" + path, [&] { return readPictureFile(path); });
	}

	std::size_t Resources::pictureBytes() const noexcept
	{
		std::size_t bytes = 0;
		for (const auto& held : pictures)
		{
			if (held) { bytes += held->rgba.size() + held->touching.size(); }
		}
		return bytes;
	}

	PictureId Turnable::turned(int degrees) const
	{
		std::uint32_t& known = asked[degrees];
		if (known == 0) { known = resources->picture(key + "|turned " + std::to_string(degrees), [&] { return at(static_cast<float>(degrees)); }).index; }
		return PictureId{ known };
	}

	const Bitmap& Turnable::turnedTo(int degrees) const { return resources->bitmap(turned(degrees)); }

	std::shared_ptr<const Bitmap> Turnable::sharedTurned(int degrees) const { return resources->shared(turned(degrees)); }

	std::shared_ptr<const Bitmap> Resources::flippedLeftRight(const std::shared_ptr<const Bitmap>& picture)
	{
		const auto held = byPicture.find(picture.get());
		const PictureId from = held != byPicture.end() ? held->second : keep(picture);
		const std::string key = "flipped|" + std::to_string(from.index);
		const bool fresh = byKey.find(key) == byKey.end();
		const PictureId made = this->picture(key, [&] { return flipBitmap(*picture, true, false); });
		if (fresh) { byKey.emplace("flipped|" + std::to_string(made.index), from); }
		return shared(made);
	}

	PictureId Resources::keep(std::shared_ptr<const Bitmap> made)
	{
		const PictureId id{ static_cast<std::uint32_t>(pictures.size()) };
		byPicture.emplace(made.get(), id);
		pictures.push_back(std::move(made));
		return id;
	}
}
