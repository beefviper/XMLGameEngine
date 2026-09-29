# 20. Ideas parking lot

**Status:** ideas only, none decided

Short items that came up and do not have a topic of their own yet.

## A machine-readable view of a running game

Idea: export a compact state (positions, velocities, score, brick grid) each frame or step, so an AI agent or test can read the game as structured text instead of pixels. Suggested pieces: frame-step the game (pause between decisions) or split a fast reflex controller from a slow high-level decision loop; render to a texture and dump a PNG per step if vision is wanted; use an emulator with frame stepping, save states, RAM search and scripting (BizHawk, RetroArch) as the target for existing commercial games. The author's larger goal in that thread was an AI that reverse-engineers a game the way a person does (search memory for the lives counter, lose a life, search for the new value) and describes the scene in plain English.

Relevance here: an engine whose whole state is already declared in XML would be a very easy game to export state from.

## Descriptions the engine could be tested against

- The `games/` files (Pong, Breakout, Space Invaders) are the working proof. A "Galaxian" step, and a game that needs jump, would be the natural next tests of the vocabulary ([01](01-vision-and-scope.md), [13](13-verb-vocabulary.md)).
- A written Pong-to-Galaxian chain showing which verb each step adds ([13](13-verb-vocabulary.md)).

## Growth systems as components

A component that owns a growth-stage index and regenerates structure (for example a tree scrubbed through its life) was noted as a possible fit for the entity model ([08](08-entity-storage-and-handles.md)). No follow-up.

## Emulated memory

Heap allocation for an emulated RAM block was mentioned as an architecture decision in the author's history; details are not in the exported conversations.

## Sources

- "Training a local LLM to play classic games" (2026-09-25).
- "Dynamic tree aging through scaling" (2026-08-20).
