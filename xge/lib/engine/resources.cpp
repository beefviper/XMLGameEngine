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

	PictureId Resources::keep(std::shared_ptr<const Bitmap> made)
	{
		pictures.push_back(std::move(made));
		return PictureId{ static_cast<std::uint32_t>(pictures.size() - 1) };
	}
}
