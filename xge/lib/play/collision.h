// collision.h
// XML Game Engine
// author: beefviper
// date: Sept 30, 2026

#pragma once

#include "game/command.h"
#include "play/geometry.h"
#include "game/object.h"

#include <optional>

namespace xge
{
	// Whether Objects touch, and where: play/geometry.h's arithmetic asked of
	// their shapes and pictures, with no side effects. A test of two answers
	// with the edge of the second that the first met.

	// Has `object` crossed the given screen edge?
	bool touchesScreenEdge(const Object& object, const WindowDesc& windowDesc, Edge edge);

	// Are the two overlapping right now? A circle is measured against the
	// other's box; with a pixel test the boxes overlap first and then some
	// pixels must (pixelsOverlap), the edge still the boxes'.
	// byPicture, here and below: by the two pictures, hitboxes aside, as a
	// pair whose rules stand on, stop at or bounce off is (touchesByPicture).
	std::optional<Edge> objectsOverlap(const Object& a, const Object& b, bool byPicture = false);

	// Whether a's hitbox named hitboxA meets b's named hitboxB, as the two
	// are now (just touching counts). An empty name is any part: its
	// hitboxes, or its picture when it has none. A pair touching by
	// pictures still asks this of their hitboxes.
	bool touchesWith(const Object& a, const std::string& hitboxA, const Object& b, const std::string& hitboxB);

	// Whether either object's collisions are of type pixel, so that a pair
	// is only a hit where pixels actually meet.
	bool usesPixels(const Object& a, const Object& b) noexcept;

	// Do any two pixels, one drawn by each object, fall on the same spot on
	// the screen? An object of type pixel is its picture (a sprite of lines,
	// see Bitmap), with empty pixels where nothing was drawn; any other
	// object, in a pair with one that is, counts as solid all over its
	// shape (a rectangle, or a circle as a circle). Each pixel of the
	// screen is asked about at its centre.
	bool pixelsOverlap(const Object& a, const Object& b);

	// Moves `a` by moveA and `b` by moveB over one step and finds the first
	// moment they touch, so a fast object cannot pass through a thin one.
	// Already overlapping is a hit at 0; touching and moving apart is none.
	// A pixel pair is swept as boxes, then walked on half a pixel at a time
	// until pixels meet; the edge is the one the motion came in through.
	std::optional<SweepHit> sweepObjects(const Object& a, const Vector2f& moveA, const Object& b, const Vector2f& moveB, bool byPicture = false);

	// The same for a pair of which one or both turn as they go (a swinging
	// flipper): turnA and turnB are the degrees each turns over the step, and
	// one turning is turned about its pivot, as drawn at each whole degree.
	// The two are walked together, half a pixel of the furthest any point
	// goes at a time, until they touch; the edge is the way the first goes
	// against the second there.
	std::optional<SweepHit> sweepTurning(const Object& a, const Vector2f& moveA, float turnA, const Object& b, const Vector2f& moveB, float turnB);

	// The box what the object touches with covers over this move
	// (sweptBox): two whose boxes do not meet (boxesMeet) cannot touch.
	Box reachOver(const Object& object, const Vector2f& move) noexcept;

	// Whether the straight line from `from` to `to` crosses what `object`
	// is drawn as: its picture's shape (a circle as a circle), or, of type
	// pixel, its drawn pixels (walked half a pixel at a time). What it
	// sees past, for <sees>.
	bool lineCrosses(const Vector2f& from, const Vector2f& to, const Object& object);

	// Everything about a touch just found, `time` through the frame, on
	// that edge of b, as the two are now (play/geometry.h's contactOf):
	// where they meet, the way apart, where along each side, how deep.
	Contact contactBetween(const Object& a, const Object& b, Edge edgeOfB, float time, bool byPicture = false);
}
