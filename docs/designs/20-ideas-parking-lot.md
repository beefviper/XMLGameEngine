# 20. Ideas parking lot

**Status:** ideas only, none decided

Short items that came up and do not have a topic of their own yet.

## A machine-readable view of a running game

Idea: export a compact state (positions, velocities, score, brick grid) each frame or step, so an AI agent or test can read the game as structured text instead of pixels. Suggested pieces: frame-step the game (pause between decisions) or split a fast reflex controller from a slow high-level decision loop; render to a texture and dump a PNG per step if vision is wanted; use an emulator with frame stepping, save states, RAM search and scripting (BizHawk, RetroArch) as the target for existing commercial games. The author's larger goal in that thread was an AI that reverse-engineers a game the way a person does (search memory for the lives counter, lose a life, search for the new value) and describes the scene in plain English.

Relevance here: an engine whose whole state is already declared in XML would be a very easy game to export state from.

## Descriptions the engine could be tested against

- The `games/` files (Pong, Breakout, Space Invaders, Frogger, Space Race, Kaboom, Freeway, Depth Charge, Astrosmash, Lunar Lander) are the working proof. A "Galaxian" step, and a game that needs jump, would be the natural next tests of the vocabulary ([01](01-vision-and-scope.md), [13](13-verb-vocabulary.md)).
- A written Pong-to-Galaxian chain showing which verb each step adds ([13](13-verb-vocabulary.md)).

## Passing information between states

Space Race (`games/spacerace.xml`) has two win states, `player1wins` and `player2wins`, that are identical except for the text. That is fine for now, but it points at a gap: a state transition carries no information. A condition can send the game to a state, but the destination cannot know which condition fired or which object triggered it.

Idea: let a triggered condition hand something to the state it enters, for example the object (or its name) that satisfied it, so one `wins` state could show "`{winner}` wins" instead of needing one state per player. Related questions: whether the payload is the object, its name, or a copied value, how the destination refers to it (a reserved name such as `trigger`?), and how long it lives once the state is left. The same mechanism would probably also serve high-score entry and "which brick ended the game" style screens. Not designed; noted from the Space Race work (2026-09-30).

## Raylib input polling: possible double poll

SFML and SDL2 have real event queues; Raylib only offers "was this key pressed/released since the last poll" queries. `RaylibWindow::pollEvents()` therefore scans its whole key table every frame and builds `{key, pressed}` pairs from `IsKeyPressed` and `IsKeyReleased`, so the engine sees the same shape of data from every backend. The author likes that pattern and it stays.

Suspected problem, **not verified, Raylib build not tested for it**: `pollEvents()` also calls `PollInputEvents()`, and Raylib's `EndDrawing()` already calls it unless built with `SUPPORT_CUSTOM_FRAME_CONTROL` (not set anywhere in this repo). If both run each frame, the second poll can copy the current key state over the previous one before the scan, so `IsKeyPressed` and `IsKeyReleased` would report nothing and key presses would be dropped.

If Raylib ever seems to miss inputs, check this first. The likely fix is to remove the explicit `PollInputEvents()` call from `pollEvents()` and rely on `EndDrawing()`. Smaller known limits of the scan: a tap shorter than one frame can be lost, and events come out in key-table order rather than the order they happened. Noted 2026-09-30.

## Growth systems as components

A component that owns a growth-stage index and regenerates structure (for example a tree scrubbed through its life) was noted as a possible fit for the entity model ([08](08-entity-storage-and-handles.md)). No follow-up.

## Emulated memory

Heap allocation for an emulated RAM block was mentioned as an architecture decision in the author's history; details are not in the exported conversations.

## Second batch: alternatives

- **Machine descriptions as data.** Keep a target's tables in files, resolve them at start-up ([25](25-targets-and-capability-profiles.md)).
- **AI players through a null input.** A scripted or learned player driving the same command queue as a human ([10](10-input-and-actions.md)).
- **Game ideas** are collected in [28](28-game-ideas-and-test-games.md).

## Sources

- "Training a local LLM to play classic games" (2026-09-25).
- "Simulation idea conversation" (2026-08-20).
- Second batch: "Data-driven tables versus code" (2025-09-04).
