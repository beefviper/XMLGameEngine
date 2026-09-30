# 26. Scrolling and screen regions

**Status:** idea; nothing built (all built games fit one screen)

## The gap

The language describes non-scrolling games. Scrolling games need a camera, a world larger than the window, and often a fixed area (a status bar) next to the moving one. [13](13-verb-vocabulary.md) asks whether scrolling belongs to the camera, the world, or is a verb; this note collects the alternatives.

## Alternatives for where scrolling lives

- **On the camera.** The world is fixed; a camera object has a position and follows a target. Everything else is unchanged. Most natural, and the camera can itself be moved by the same motion vocabulary ([22](22-motion-models.md)).
- **On the world.** The whole world offsets; simple for auto-scrolling games but awkward when objects need world and screen coordinates at the same time.
- **On regions.** The screen is divided into regions, each with its own scroll. This is how old hardware did it: a status bar that never moves, a playfield that does, and sometimes bands that scroll at different rates.

Regions also give parallax cheaply (each band its own rate) and let a HUD live outside the world without special cases.

## Camera behaviors worth naming

- Follow the target rigidly, follow with a dead zone, follow with a lead in the facing direction.
- Lock to the room (screen-by-screen scrolling), lock to a track (auto-scroll).
- Scroll only forward, so the player cannot go back (some games), or free.

## Coordinates

Expressions such as window width and center already position things per window size ([07](07-object-variables-and-references.md)). With a camera the author needs both world and screen coordinates of an object; the current dotted references could grow a world/screen qualifier.

## Level data

A larger world needs a way to place many objects: a map. Options discussed for authoring:

- A grid map where characters or numbers stand for object names, expanded at load (already close to what `grid()` does for simple fields).
- Lists of named placements with positions.
- Tile layers for the static scenery, objects for everything that acts.

Early exchanges about this also asked whether maps have several layers or one, and how a step onto a certain tile triggers an event; suggested element names for a tile map were hitbox, map and tile. A tile that fires a rule on entry is a natural trigger form next to object collisions ([14](14-conditions-and-win-conditions.md)).

The earlier VGDL work uses a text level map with a legend; the same idea in XML is a legend element plus rows. Which of these reads best in XML is undecided.

## Pacing and routes

Level layout is part of design too. One discussion argued that speed-based games feel unbalanced when the fast stretches keep being interrupted by hazards, and suggested a structure in which a fast segment branches into different slower, more deliberate sections. The point for the language: routes and branches are a level-data feature (named exits leading to different regions), not a verb.

## Sources

- "Sprite 0 hit effect" (2025-08-18): split regions.
- "Sonic vs Mario Debate" (2025-06-01): pacing and branching routes.
- General chat A (engine segment only, 2025-07-25): tile-map layers and tile events.
- "Jump Behavior Models" (2026-09-23): scrolling deferred until non-scrolling games are covered.
