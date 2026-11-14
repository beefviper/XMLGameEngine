// test_picture_scale.cpp
// XML Game Engine
// author: beefviper
// date: Nov 14, 2026
//
// Catch2 tests for pictures drawn at a window's size (Object::drawnPicture,
// SettledFit in window/window.h): in a window bigger than the game, a text,
// a circle and a rectangle are drawn with more pixels, so they stay sharp,
// while the game's sizes stay its own; a bitmap keeps its pixels.

#include "game/object.h"
#include "window/window.h"

#include <catch2/catch_test_macros.hpp>

#include <cmath>
#include <cstdlib>
#include <memory>

using namespace xge;

namespace
{
	Object text(const std::string& words, int size)
	{
		Object object;
		object.picture.kind = ShapeKind::Text;
		object.picture.text = words;
		object.picture.textSize = size;
		object.picture.color = "color.white";
		return object;
	}
}

TEST_CASE("a text drawn at twice the scale has twice the pixels and the same game size", "[picture_scale]")
{
	const Object title = text("PONG", 64);
	const Vector2f size = title.pictureSize();
	const std::shared_ptr<const Bitmap> once = title.drawnPicture(1.0f);
	const std::shared_ptr<const Bitmap> twice = title.drawnPicture(2.0f);

	CHECK(static_cast<float>(once->width) == size.x);
	CHECK(static_cast<float>(once->height) == size.y);
	// The font's own rounding at another size moves an edge a pixel at most.
	CHECK(std::abs(twice->width - 2 * once->width) <= 2);
	CHECK(std::abs(twice->height - 2 * once->height) <= 2);
	CHECK(title.pictureSize().x == size.x);
	CHECK(title.drawnAtScale());

	// A pixel font at its own multiple doubles exactly.
	Object score = text("SCORE", 16);
	score.picture.font = "assets/pressstart2p.ttf";
	CHECK(score.drawnPicture(2.0f)->width == 2 * score.drawnPicture(1.0f)->width);
}

TEST_CASE("a circle and a rectangle are drawn by the engine, at any scale", "[picture_scale]")
{
	Object ball;
	ball.picture.kind = ShapeKind::Circle;
	ball.picture.radius = 5.0f;
	ball.picture.color = "color.red";

	const std::shared_ptr<const Bitmap> small = ball.drawnPicture(1.0f);
	CHECK(small->width == 10);
	CHECK(small->height == 10);
	CHECK(small->solidAt(5, 5));
	CHECK_FALSE(small->solidAt(0, 0));          // a corner is outside the round
	CHECK(ball.drawnPicture(3.0f)->width == 30);
	CHECK(ball.drawnPicture(2.5f)->width == 25);

	Object paddle;
	paddle.picture.kind = ShapeKind::Rectangle;
	paddle.picture.size = { 10.0f, 50.0f };
	paddle.picture.color = "color.white";
	const std::shared_ptr<const Bitmap> tall = paddle.drawnPicture(2.0f);
	CHECK(tall->width == 20);
	CHECK(tall->height == 100);
	CHECK(tall->solidAt(0, 0));
	CHECK(tall->solidAt(19, 99));
	CHECK(paddle.pictureSize().x == 10.0f);
}

TEST_CASE("a bitmap keeps its own pixels at any scale", "[picture_scale]")
{
	Object ship;
	ship.picture.kind = ShapeKind::Line;
	auto drawn = std::make_shared<Bitmap>();
	drawn->width = 4;
	drawn->height = 2;
	drawn->rgba.assign(4 * 2 * 4, 255);
	ship.bitmap = drawn;

	CHECK_FALSE(ship.drawnAtScale());
	CHECK(ship.drawnPicture(3.0f) == ship.bitmap);   // the very same, not a copy
}

TEST_CASE("pictures are drawn again once a resize has held still a quarter second", "[picture_scale]")
{
	SettledFit fit{ 2.0f };

	// Opened at 2: nothing to do.
	CHECK_FALSE(fit.update(2.0f, 0.0));
	CHECK_FALSE(fit.update(2.0f, 1.0));

	// Dragged: each new size starts the wait again.
	CHECK_FALSE(fit.update(2.3f, 1.0));
	CHECK_FALSE(fit.update(2.6f, 1.1));
	CHECK_FALSE(fit.update(3.0f, 1.2));
	CHECK_FALSE(fit.update(3.0f, 1.4));

	// Let go: once, a quarter second after the last change.
	const auto settled = fit.update(3.0f, 1.45);
	REQUIRE(settled);
	CHECK(*settled == 3.0f);
	CHECK_FALSE(fit.update(3.0f, 2.0));

	// Back to the size it was drawn at before the wait ends: nothing to redraw.
	CHECK_FALSE(fit.update(2.5f, 3.0));
	CHECK_FALSE(fit.update(3.0f, 3.1));
	CHECK_FALSE(fit.update(3.0f, 4.0));
}
