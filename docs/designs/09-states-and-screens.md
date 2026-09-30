# 09. States and screens

**Status:** implemented (state stack, shows, inputs, conditions)

## What a state is

A `<state>` is a **specific screen**: a menu, the playfield, a pause screen, a game-over screen. It defines what is shown, which buttons run which commands in that context, and which conditions to watch. States are loaded much like objects, and a group of them sits in a `<states>` block, mirroring `<objects>`.

A state is a declarative definition (a template), not a live thing. Which state is current at a given moment is separate run-time information.

## Decision: a stack

`state('playing')` pushes a state and `state()` with no argument pops to the previous one. Pause is then `space` in `playing` pushing `paused`, and `space` in `paused` popping back. A bare `reset()` from a state input or condition collapses the stack to the first state, so `mainmenu -> playing -> gameover -> mainmenu -> ...` does not grow forever.

## Naming history (file layout)

During a July 2026 restructure the engine's files were grouped by responsibility, with a `compile_*` group for the pipeline (parse, validate, evaluate, generate) and a second group for the data those steps produce. Names considered for the second group: `model_`, `runtime_`, `world_`, `instance_`, `def_`, `schema_`, `live_`, `sim_`.

The useful conclusion was that `model_object` and `model_states` are **parallel**: both are definitions parsed from XML, so they should share a prefix rather than one moving to `runtime_`. What is genuinely *runtime* is the current state and the live objects. The current code (`object.h`, `states.h`, `game.cpp`) has since been flattened and no longer uses these directory prefixes; see [16](16-code-layout-and-pipeline.md).

## Design questions still open

- Do conditions belong to states, to objects, or both? Today only states have them. Object-level conditions (for example changing a color when speed passes a limit, or playing a death animation at zero health) were raised, not built ([14](14-conditions-and-win-conditions.md)).
- Objects that behave differently per state. A state only chooses which objects are shown; it cannot change what a shown object does. Every shown object with a velocity moves, whichever state is current, so decoration on a menu (the Space Invaders aliens standing still behind the title) cannot be told apart from the same objects in play. Ways it could be expressed, none built: a per-`<show>` override (`<show object="aliens" velocity="0"/>`); entry and exit commands on a state (`<enter>`/`<exit>` running `reset('aliens')`-style commands, or a verb that freezes and unfreezes an object); an `active` flag on objects that states switch; or separate copies of the object per state. The last is possible today but duplicates definitions. Related to the question above about where behavior belongs: to the state, the object, or the pairing of the two. **Open**.
- Menus and settings screens exist as states in every shipped game, but there is no vocabulary for a real settings UI.

## Sources

- "XML game engine project structure review" (2026-07-20).
- "Renaming game folder file prefixes" (2026-07-20).
