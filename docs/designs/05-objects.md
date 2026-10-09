# Objects and storage

**Status:** Built. Opt-in parts, handles and outline shapes are ideas.

## What an object is

- An object is a name and a place, with parts. Today every object carries sprite, position, velocity and collisions, even a score readout; the idea is that **everything else is opt-in** (appearance, physics, input, actions). A zero velocity is noise: zero is the absence of motion. Litmus test for a core field: if any entity you can imagine lacks it, it is a component. ID, generation and name pass; "renderable", "has health", "has a collider" do not (a trigger volume has none).
- Element naming options seen: `position`/`velocity` (chosen: they belong to a physics part), `at`/`moving` (reads as English), `heading`/`speed` (maybe for rotating ships; `<heading>` now exists for that, [motion](22-motion.md)).
- Collision responses as a block: one element holding named effects (reads like a game description; chosen) vs a list of response elements each with a command and target (easier to validate and extend).
- Shapes as reusable named outlines (a few asteroid outlines at several scales) would add a polygon sprite kind; unbuilt.


## Storage and identity

- Today: `vector<Object>` by value, found by linear name search (`getObject`, `tryGetObject`). No handles, no ECS.
- Options for growth: vector by value (any growth invalidates pointers), `vector<unique_ptr>` (stable addresses, no removal safety), plain indices (silently wrong after swap-and-pop), `deque` (push_back keeps references), and the **decided** form: a handle `{index, generation}`. Bump the generation on free, keep a freelist, `uint32_t` generation, handles are plain copyable data (an action's target, a projectile's owner). A stale handle fails loudly.
- Type identity is data, not an enum: a type name from XML, interned to an integer at load, no `switch` over an engine enum for author-defined types. C++ reflection would not change this. Real enums remain right for what the engine fixes.
- Growth components (a component with its own state, such as a growth stage that regenerates structure) fit the opt-in-parts model; nothing built ([ideas](57-ideas.md)).


## Lockstep

- `<lockstep>true</lockstep>` gives all members or cells of a group one lockstep number: they never collide with each other, `<move>` moves all, and one hitting a side with `<bounce />` moves the block (the invaders march). It changes behavior only, never geometry: every cell is swept alone, so a bullet meets only living cells and there is no bounding box.
