# Spawning and enemy fire

**Status:** Built.

## Spawn and despawn (2026-10-23)

- **What we wanted:** making and taking away objects while a game runs as a first-class part of the engine, with a spawned object no different to any system from one the file wrote. Before this, nothing was made at run time: a `<group>` of hidden members was a pool that `<fire>`, `<release>`, `<reveal>` and `<spawn>` took from, and its size capped how many were out.
- **Looked at:**
  - *Pools only* (the old way): no list changes at run time, and every member is visible to tests. But every pool repeats `<member />` lines or `<columns>`, its size is a guess, a fired shot keeps its old state, and an unlimited count cannot be said.
  - *A separate list of templates* that only spawns could use: rejected, since then two kinds of thing would do the same job, and any object should be spawnable.
  - *Words:* create/destroy, spawn/despawn; `<template>`, `<inplay>`, `<exists>`. Chosen: `spawn`/`despawn` and `<exists>`, since what a rule cares about is whether something exists.
- **Chosen:**
  - `<exists>false</exists>` made an object (or a group) a description: in the file, never in the game. Its position may be left out. Since 2026-10-25 a description is a piece of the game's `<kit>` that a spawn names, and `<exists>` is gone from the file ([kit](13-kit.md)); the engine still marks the piece it puts in beside the spawner as not existing.
  - `<spawn>`, `<fire>` and `<release>` of a description (now a kit piece) make a new copy, named `name.N` with the lowest free N; a group's members are copied in turn. A copy's `<random>`s are drawn for it, its variables and timers are its own, and its own name and `self` in its rules mean the copy. `<spawn>` of an object that exists copies it too.
  - `<most>` caps how many copies exist at once; at the cap the verb is refused (and the rest of the press, as for any refused verb).
  - `<despawn />` takes a copy away for good (its name free again) and puts one the file wrote out until a reset.
  - **Storage:** objects stay in one `std::vector`; copies made during a frame wait in a `std::deque` and join when the frame ends, and despawned copies are erased then, so nothing a frame is iterating moves under it. Caches keyed by position or pointer (names found, live numbers, collision partners, run-in-top) are rebuilt when the list changes. The vector reserves room at load so references to loaded objects stay good.
  - The name of the description means all its copies (rules, conditions, `<despawn object>`); a copy's own name can be written too (`abm.1`), found by what it copies, so a tether can join a copy that comes and goes.
- **Games moved onto it:** Depth Charge, Yars' Revenge, Asteroids, Berserk, Galaxian, Megamania, Demon Attack, Atlantis, Missile Command, both Space Invaders, Kaboom, Donkey Kong, Astrosmash, Air-Sea Battle, Lunar Lander. About 85 lines shorter in all, mostly `<member />` lines and the comments explaining pools.
- **Waiting out of play still works** for objects that come back to where they were (`<reveal>`: Frostbite's igloo, a door, Atlantis' and Air-Sea Battle's planes). Show/hide as verbs were an earlier idea; `<reveal>` and `<die />` are that pair.
- **Fixed sets that cycle:** things that fall and come back (Astrosmash's rocks, Missile Command's missiles) are a fixed set reset to the top, and stay so where that is the game.
- **Open:** each copy's names are registered with the expression engine and live numbers and kept (bounded by reuse of names, so `<most>`-less descriptions that pile up grow them). The collision partner table is rebuilt for every list change, O(n²) in objects; fine at these sizes, worth a measure if a game spawns every frame.

## Enemies fire back

- Enemies use the same `<fire>`: an enemy is a shooter with a facing and a timer; its shots are copies of a piece of the kit. No aiming. Space Invaders (both) and Demon Attack fire this way (Berserk's robots now `<aim>` first); the aliens' catch-all rule (`<collision><die /></collision>`) had to name the cannon's bullet so their own bombs do not kill them.
- Limits: an invader behind others fires through them (the arcade fires only the bottom one of a column); a Berserk robot aims at the man through walls (its shot dies on the wall). Aiming and chasing: [chase-and-paths](24-chase-and-paths.md#chasing-and-aiming-berserk).


## Open

Only the front invader of a column firing, an event queue, mutable game-wide variables (object variables did the job in every game), timers whose numbers follow variables beyond the interval.
