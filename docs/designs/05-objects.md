# Objects and storage

**Status:** Built. Opt-in parts, handles and outline shapes are ideas.

## What an object is

- An object is a name and a place, with parts. Every object carries a sprite and a position; the idea is that **everything else is opt-in** (appearance, physics, input, actions). A zero velocity is noise: zero is the absence of motion.
- **What a game leaves out is nothing (2026-10-13).** `<velocity>` and `<collisions>` were required in every object, so 339 of 429 objects and groups said a velocity of 0,0 and 260 said only `<enabled>false</enabled>`: about 1,100 of the games' 14,950 lines, mostly text. Chosen: both optional. No `<velocity>` stands still and a half left out is 0, the way a C++ member left out of a braced initializer is; no `<collisions>` touches nothing. A member or cell still gives a 0 when it must override its group. Looked at: defaults written once at the top of the file (another mechanism for the same thing). A written `<collisions>` became on by default on 2026-10-28 ([file-format](02-file-format.md)). `<position>` stays required: an object at 0,0 by accident is a bug that a default would hide. Litmus test for a core field: if any entity you can imagine lacks it, it is a component. ID, generation and name pass; "renderable", "has health", "has a collider" do not (a trigger volume has none).
- **Objects that only hold numbers (2026-10-16).** Ten objects in eight games (a `status` with the score, Atlantis's `tally`, Mega Mania's `energy`) carried a 1 by 1 black rectangle at 0, 0 and `<drawn>false</drawn>`: nine lines of picture for an object nobody sees. Chosen: an object with no `<sprite>` holds numbers. It is never drawn, has an empty rectangle (`nothingSprite`), needs no `<position>`, and must have `<variables>` or `<timers>`, so an empty `<object name="rokc" />` (a mistyped take) is still an error. A `<drawn>` on it is an error, as is a group with no sprite. Looked at: a separate `<holder>` element (another word for an object without a part) and a game-level variables block per subject (the variables already have owners, and rules address them as `status.score`). This is the litmus test above applied to the sprite; `<position>` is still required wherever there is a picture.
- Element naming options seen: `position`/`velocity` (chosen: they belong to a physics part), `at`/`moving` (reads as English), `heading`/`speed` (maybe for rotating ships; `<heading>` now exists for that, [motion](22-motion.md)).
- Collision responses as a block: one element holding named effects (reads like a game description; chosen) vs a list of response elements each with a command and target (easier to validate and extend).
- Shapes as reusable named outlines (a few asteroid outlines at several scales) would add a polygon sprite kind; unbuilt.


## Start, before and now (2026-10-29)

- Need: a reset was 50 hand-written lines copying about fifteen `xxxOriginal` fields back one by one, the last frame's place was a field of its own (`settledAt`), and a new number that a reset should put back had to be remembered in three places.
- Looked at: a snapshot of the whole object to copy back (about 200 fields with bitmaps, maps and rule lists, so each object twice over for one function). Beef's idea instead: the few numbers that change in play, kept at each point in time an object has. **Chosen:** `Moment` (`game/object.h`): position, velocity, acceleration, heading, facing, light, altitude, rise, in play, collisions on, exists, scene and variables. An `Object` is a Moment (its current values, no prefix: `object.position`) and holds two more, `start` (as worked out from the file, drawn again when a `<random>` start is) and `before` (as the last frame left it). The fourth layer, the file's text, is the `RawObject` the game keeps. A reset is `backToStart()` plus clearing what was under way (held keys, jumps, rides, timers, the look); the end of a frame is `remember()`. About 100 bytes more per object; copying it each frame allocates nothing.
- Not in it: what does not change in play (`drag`, `grip`, pulls, mass), and what is under way rather than a value (held moves, a jump, a ride), which the reset clears by hand.
- Open: rules reading `before` (how fast something went before a bounce), if a game needs it.

## Storage and identity

- Today: `vector<Object>` by value; a name lookup is cached once found (`foundObjects`), and the cache and every per-index answer are rebuilt when objects join or leave (`rosterChanged`). Copies spawned in a frame wait in a `deque` and join between frames (`settleSpawns`). No handles, no ECS.
- Options for growth: vector by value (any growth invalidates pointers), `vector<unique_ptr>` (stable addresses, no removal safety), plain indices (silently wrong after swap-and-pop), `deque` (push_back keeps references), and the **decided** form: a handle `{index, generation}`. Bump the generation on free, keep a freelist, `uint32_t` generation, handles are plain copyable data (an action's target, a projectile's owner). A stale handle fails loudly.
- Type identity is data, not an enum: a type name from XML, interned to an integer at load, no `switch` over an engine enum for author-defined types. C++ reflection would not change this. Real enums remain right for what the engine fixes.
- Growth components (a component with its own state, such as a growth stage that regenerates structure) fit the opt-in-parts model; nothing built ([ideas](57-ideas.md)).


## Lockstep

- `<lockstep>true</lockstep>` gives all members or cells of a group one lockstep number: they never collide with each other, `<move>` moves all, and one hitting a side with `<bounce />` moves the block (the invaders march). It changes behavior only, never geometry: every cell is swept alone, so a bullet meets only living cells and there is no bounding box.
