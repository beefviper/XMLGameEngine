// types.h
// XML Game Engine
// author: beefviper
// date: Sept 28, 2026

#pragma once

namespace xge
{
	// Backend-agnostic 2D vector - used for Object::position/velocity/size so
	// no windowing/graphics library's vector type has to leak into Object,
	// Game, or Engine (see window.h).
	struct Vector2f
	{
		float x{};
		float y{};

		Vector2f() = default;
		Vector2f(float xValue, float yValue) : x(xValue), y(yValue) {}

		Vector2f operator+(const Vector2f& other) const { return { x + other.x, y + other.y }; }
		Vector2f operator-(const Vector2f& other) const { return { x - other.x, y - other.y }; }
		Vector2f operator*(float scalar) const { return { x * scalar, y * scalar }; }

		Vector2f& operator+=(const Vector2f& other) { x += other.x; y += other.y; return *this; }
		Vector2f& operator-=(const Vector2f& other) { x -= other.x; y -= other.y; return *this; }
		Vector2f& operator*=(float scalar) { x *= scalar; y *= scalar; return *this; }

		friend bool operator==(const Vector2f& a, const Vector2f& b) noexcept { return a.x == b.x && a.y == b.y; }
		friend bool operator!=(const Vector2f& a, const Vector2f& b) noexcept { return !(a == b); }
	};
}
