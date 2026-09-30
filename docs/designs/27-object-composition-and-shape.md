# 27. Object composition and shape

**Status:** idea; extends [08](08-entity-storage-and-handles.md) and [07](07-object-variables-and-references.md)

## What is an object

The second batch reopened the basic question in the redesign of Pong: what does an object need in order to be one? The answer that kept coming back is little more than a name and a place, with everything else opt-in:

- name (identity, unique) and class (grouping, [06](06-names-and-classes.md)),
- a position,
- optional parts: appearance, physics (velocity, acceleration, gravity, collision), input, actions.

A score readout or a background logo has a position and an appearance and nothing else. Giving it a zero velocity is noise: zero is the absence of motion, not a property. So velocity and collision belong with the physics part, and an object with none of it costs nothing.

## Element naming options

Position and motion can read in the same voice or not. Options seen:

- `position` with `velocity` (plain, physics vocabulary, chosen so far).
- `at` with `moving` (reads as English; an object is at a place, moving so fast).
- `heading` with `speed` (what the object is doing, rather than exposing a vector).

Leaning: keep position and velocity because they belong to a physics part, but note that a heading plus speed pair may suit games with rotating ships.

## Collision responses as a block

Two ways to write the reaction to a touch:

- one element holding named effects (destroy self, add score, spawn an explosion);
- a list of responses, each carrying a command name and a target.

The first reads like a description of the game; the second is easier to validate and to extend. The current form ([13](13-verb-vocabulary.md), [21](21-frogger.md)) is the first, kept because readability was preferred over schema regularity.

## Shapes

Shapes are their own topic. A vector game stores each outline as a short list of points and reuses it: a handful of asteroid outlines, each drawn at several scales and rotated every frame, instead of a bitmap per size and angle. That suggests, for the language:

- a shape as a list of points, referenced by name, with scale and rotation as object properties;
- variety from several outlines rather than many hand-drawn variants;
- splitting an object into smaller ones of the same shape as an ordinary rule (destroy and spawn).

The current sprite options (rectangle, circle, text) would gain a polygon or outline kind ([02](02-file-format.md)).

## Growth systems

The idea already noted in [20](20-ideas-parking-lot.md) (a component that owns a growth stage) fits this shape: a component is a named part with its own state that an object opts into.

## Sources

- "Declarative Pong XML redesign" (2026-09-23): opt-in parts, at/moving naming, response blocks.
- "Asteroids Shape Variations" (2026-07-17): reusable outlines, scaling and rotation.
- "VGDL Design Challenges" (2026-01-28): compositional explosion.
