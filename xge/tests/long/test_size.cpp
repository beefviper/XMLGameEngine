// test_size.cpp
// XML Game Engine
// author: beefviper
// date: Nov 17, 2026
//
// How big the engine's things are, and how many pictures and textures a game
// holds. Not a check, and part of xgetest-long, so ordinary runs skip it: run
// `xgetest-long "[size]"` to print the table, and compare a build before and
// after a change (the C++ review in docs/plan.md).

#include "played.h"
#include "game/random.h"
#include "load/xml.h"

#include <catch2/catch_test_macros.hpp>

#include <cstdio>
#include <memory>
#include <set>
#include <string>
#include <unordered_set>
#include <vector>

namespace
{
	// A window that keeps a texture by object name and builds it again when
	// asked, as every real backend does today, and counts what it made.
	class CountingWindow : public xge::Window
	{
	public:
		bool isOpen() const override { return true; }
		void close() override {}
		void init(std::vector<xge::Object>& objects) override
		{
			for (xge::Object& object : objects) { prepare(object); }
		}
		std::vector<std::pair<xge::KeyCode, bool>> pollEvents() override { return {}; }
		void clear(const std::string& colorName) override { (void)colorName; }
		void prepare(xge::Object& object) override
		{
			const bool fresh = textures.insert(object.name).second;
			if (!fresh && !object.drawn.visualDirty) { return; }
			const std::shared_ptr<const xge::Bitmap> picture = object.drawnPicture(pictureScale());
			++builds;
			textureBytes += picture->rgba.size();
			object.drawn.visualDirty = false;
		}
		void draw(xge::Object& object) override { prepare(object); }
		void display() override {}

		std::set<std::string> textures;   // one a name, as the backends keep them
		std::size_t builds{};             // textures made, counting each made again
		std::size_t textureBytes{};       // pixels sent over all those builds
	};

	struct Pictures
	{
		std::unordered_set<const xge::Bitmap*> seen;
		std::size_t bytes{};

		void add(const std::shared_ptr<const xge::Bitmap>& picture)
		{
			if (picture && seen.insert(picture.get()).second) { bytes += picture->rgba.size() + picture->touching.size(); }
		}
		void add(const std::shared_ptr<const xge::Turnable>& turnable)
		{
			if (!turnable) { return; }
			add(turnable->picture);
			for (const auto& [degrees, turned] : turnable->kept) { (void)degrees; add(turned); }
		}
		void addLooks(const std::vector<xge::Object::Look>& looks)
		{
			for (const xge::Object::Look& look : looks)
			{
				add(look.bitmap);
				for (const auto& frame : look.frames.shows().bitmaps) { add(frame); }
				for (const auto& turnable : look.turnables) { add(turnable); }
			}
		}
		// Every picture an object holds, each counted once however many hold it.
		void addObject(const xge::Parts& parts, const xge::Object& object)
		{
			add(object.drawn.bitmap);
			if (const xge::Object::Unholed* unholed = parts.unholed.find(object.id)) { add(unholed->bitmap); }
			for (const auto& turnable : object.drawn.turnables) { add(turnable); }
			for (const auto& frame : object.animation.frames.shows().bitmaps) { add(frame); }
			addLooks(object.looks.all());
			for (const xge::Object::Palette& palette : object.looks.set->palettes)
			{
				add(palette.bitmap);
				for (const auto& frame : palette.animation) { add(frame); }
				addLooks(palette.looks);
			}
			if (object.mirror.flips)
			{
				for (const auto& [from, to] : *object.mirror.flips) { (void)from; add(to); }
			}
		}
	};
}

TEST_CASE("The size of the engine's things", "[long][size]")
{
	std::printf("%-12s %8s\n", "type", "bytes");
	std::printf("%-12s %8zu\n", "Object", sizeof(xge::Object));
	std::printf("%-12s %8zu\n", "RawObject", sizeof(xge::RawObject));
	std::printf("%-12s %8zu\n", "RawCommand", sizeof(xge::RawCommand));
	std::printf("%-12s %8zu\n", "Command", sizeof(xge::Command));
	std::printf("\n");

	xge::fixRandomSeed(1);
	std::printf("%-14s %8s %9s %12s %9s %9s %12s\n", "game", "objects", "pictures", "picture KB", "textures", "builds", "built KB");
	for (const std::string name : { "pong", "spaceinvaders", "pacman", "klondike" })
	{
		xge::Game game{ "descriptions/" + name + ".xml" };
		auto counting = std::make_unique<CountingWindow>();
		CountingWindow& window = *counting;
		xge::Engine engine{ game, std::move(counting) };

		// Started, and played a little, so what is made on the way is counted.
		engine.handleKeyPressed(xge::KeyCode::Space);
		engine.handleKeyReleased(xge::KeyCode::Space);
		for (int frame = 0; frame < 600; ++frame) { engine.step(xge::kSixtieth); engine.render(); }

		Pictures pictures;
		for (const xge::Object& object : game.getCurrentObjects()) { pictures.addObject(game.parts(), object); }
		std::printf("%-14s %8zu %9zu %12.1f %9zu %9zu %12.1f\n", name.c_str(), game.getCurrentObjects().size(), pictures.seen.size(),
			static_cast<double>(pictures.bytes) / 1024.0, window.textures.size(), window.builds, static_cast<double>(window.textureBytes) / 1024.0);
	}
	SUCCEED();
}
