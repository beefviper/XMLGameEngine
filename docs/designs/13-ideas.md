# 13. Ideas not built

**Status:** all ideas. Nothing here is decided or built unless it says so. Larger ideas that have their own home: arcing jump, AI targeting and paths in [06](06-motion-and-verbs.md); event queue and aiming in [08](08-timers-and-enemy-behavior.md); opt-in parts and handles in [03](03-objects-groups-and-storage.md); data passing between states in [04](04-states-conditions-and-input.md).

## Scrolling and screen regions

- Gap: the language describes non-scrolling games (every built game fits one screen). Scrolling needs a camera, a world larger than the window, and often a fixed area (status bar).
- Where scrolling could live: **on the camera** (world fixed, a camera object follows a target; most natural, and the camera can use the motion vocabulary), **on the world** (whole world offsets; simple for auto-scroll, awkward when objects need world and screen coordinates), **on regions** (screen divided into bands each with its own scroll; how old hardware did it: a fixed HUD, parallax cheaply, no special cases).
- Camera behaviors to name: rigid follow, dead zone, lead in the facing direction; lock to the room (screen by screen) or to a track (auto-scroll); forward-only or free.
- Coordinates: dotted references would need a world/screen qualifier.
- Level data options: a grid map where characters or numbers stand for object names, expanded at load (close to `<grid>`); lists of named placements; tile layers for static scenery with objects for everything that acts; a legend plus rows in XML like academic VGDL. Open: one layer or several, and a tile that fires a rule on entry (a trigger next to object collisions; suggested element names hitbox, map, tile). Routes and branches are level data (named exits leading to regions), not a verb.

## Targets and capability profiles (long term)

- One game description, several **targets** with their own limits: resolution, color count and palette rules, sprites per frame and scanline, sprite size and colors, audio channels, input devices, memory. The toolchain does its best and **reports what it gave up**: too many objects (some dropped or flickered), a sprite using too many colors (reduced), "a file named here could be recreated by hand". The list of assets to redo is the useful part. The declarative version is stronger than a 8-bit toolkit that swaps target code, because the description has no machine code.
- Old hardware tricks the language might say or know about: reusing the sprite budget mid-frame, a sprite-vs-background hit as a timing signal to split the screen (fixed status bar, scrolling playfield), scanline interrupts. These are implementation techniques; the declarative layer would say there are two regions and a limit and the target decides how.
- Null and software backends: a null implementation of each subsystem removes "does it exist" checks (null audio makes play-a-sound always safe; a scripted null input drives tests and AI players); a software renderer is the last fallback.
- Tables as data: keep a machine's capability or instruction table in a file, translate it once at start-up into handlers, then never look up strings in the hot path. The same shape fits target profiles and verb names resolved at load.
- Open: in the XML or a separate profile file; how much of a degradation report is possible without running the game.

## Machine-readable view of a running game

- Export a compact state (positions, velocities, score, brick grid) each frame or step so an AI agent or test reads structured text instead of pixels. Pieces: frame-step the game, or split a fast reflex controller from a slow decision loop; render to a texture and dump a PNG per step for vision; for existing commercial games, emulators with frame stepping, save states, RAM search and scripting (BizHawk, RetroArch). The author's larger goal: an AI that reverse-engineers a game the way a person does (find the lives counter by losing a life) and describes the scene in plain English.
- Relevance: an engine whose whole state is declared in XML is easy to export from. A null input driven by a scripted or learned player through the same command queue as a human ([04](04-states-conditions-and-input.md)).

## Game and mechanic ideas

- **Pong variants.** Contact-point angle with velocity transfer (partly built as `<deflect>`, [05](05-collisions.md)); energy Pong (paddles that move a little toward or away from the ball, absorbing or adding speed, small horizontal range); a third paddle driven by both players (many-to-one bindings; fairness: shorter, or solid only for the defender).
- **A game that mocks bad play:** doing badly earns power-ups with sarcastic messages, doing well takes things away (dynamic difficulty made visible); needs conditions that read performance and change stats. **Unwinnable starts:** guarantee a solvable setup or flag one that is not.
- **Combat resolution styles** (candidates for a battle verb): pairwise (matched one to one, the weaker dies), aggregate/attrition (each side loses a share by strength ratio; the stronger loses less but never nothing), dominance (the stronger wins with near-zero loss). RPG variants: using the target's stat against it, damage as a fraction of max health, reflecting a share of damage.
- **Placement and packing:** large bases on a grid; with a one-cell gap a destroyed base leaves a hole that fits one enemy base, with two cells it fits four. Footprint and spacing are game parameters worth exposing; failure modes depend non-linearly on spacing.
- **Balance math, as value helpers:** clamp, saturate (0 to 1), floor/ceiling, hard cap, diminishing returns, smooth steps, switching a term off by multiplying with a comparison result ([02](02-values-variables-and-names.md)).
- Observations: a successful game's new mechanics become a genre named after it; simulators keep adding fidelity (a physics layer may be asked to model a machine); some games hide a very different game under a simple opening (how much of a game should one XML file hold).
- Candidate next games: a Galaxian step (needs paths and formations), a game that needs a real jump (Donkey Kong / Mario style).

## Smaller ideas

- A rule that must be looked at every frame even when nothing moves (show and hide exist: `<reveal>` and `<die />`); a general way to say where in a sequence of rules a touch falls if `unless` is not enough ([05](05-collisions.md)).
- An `<inc>` amount, or a condition threshold, that follows a variable at runtime (both are worked out at load); evenly spaced group members (`count`, `gap`).
- Polish: a per-glyph or per-value offset in the text vocabulary (a score "1" looks too close to the center line).
- Performance-driven conditions, state predicates (no moves left, a timer).
