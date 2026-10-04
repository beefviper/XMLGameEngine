# 03. Objects, groups and storage

**Status:** built: objects, `<grid>`, `<group>`. Opt-in parts, handles and outline shapes are ideas.

## What an object is

- An object is a name and a place, with parts. Today every object carries sprite, position, velocity and collisions, even a score readout; the idea is that **everything else is opt-in** (appearance, physics, input, actions). A zero velocity is noise: zero is the absence of motion. Litmus test for a core field: if any entity you can imagine lacks it, it is a component. ID, generation and name pass; "renderable", "has health", "has a collider" do not (a trigger volume has none).
- Element naming options seen: `position`/`velocity` (chosen: they belong to a physics part), `at`/`moving` (reads as English), `heading`/`speed` (maybe for rotating ships; `<heading>` now exists for that, [06](06-motion-and-verbs.md)).
- Collision responses as a block: one element holding named effects (reads like a game description; chosen) vs a list of response elements each with a command and target (easier to validate and extend).
- Shapes as reusable named outlines (a few asteroid outlines at several scales) would add a polygon sprite kind; unbuilt.

## Storage and identity

- Today: `vector<Object>` by value, found by linear name search (`getObject`, `tryGetObject`). No handles, no ECS.
- Options for growth: vector by value (any growth invalidates pointers), `vector<unique_ptr>` (stable addresses, no removal safety), plain indices (silently wrong after swap-and-pop), `deque` (push_back keeps references), and the **decided** form: a handle `{index, generation}`. Bump the generation on free, keep a freelist, `uint32_t` generation, handles are plain copyable data (an action's target, a projectile's owner). A stale handle fails loudly.
- Type identity is data, not an enum: a type name from XML, interned to an integer at load, no `switch` over an engine enum for author-defined types. C++ reflection would not change this. Real enums remain right for what the engine fixes.
- Growth components (a component with its own state, such as a growth stage that regenerates structure) fit the opt-in-parts model; nothing built ([13](13-ideas.md)).

## Grids and lockstep

- `<grid>` repeats one shape (circle, rectangle, text, image, bitmap, svg) as separate objects with a `<padding>`. Identical cells, one spacing, one velocity.
- `<lockstep>true</lockstep>` gives all cells (or all members of a group) one lockstep number: they never collide with each other, `<move>` moves all, and one hitting a side with `<bounce />` moves the block (the invaders march). It changes behavior only, never geometry: every cell is swept alone, so a bullet meets only living cells and there is no bounding box.

## Groups

- Problem: a Frogger lane is three logs sharing shape, speed, row and rule (`wrap`), differing only in start. As separate objects each was about 22 lines, and 42 of Frogger's 59 objects repeated their neighbors. A `<grid>` cannot help: one spacing, one velocity, one shape.
- **A `<group>` sits beside `<object>`.** It says what its members share, then lists `<member>`s. One rule: **whatever a member leaves out it takes from its group**, then it must be complete like an `<object>` (else a load error naming the member). `<position>` and `<velocity>` can be given in halves (the group gives `<y>`, each member `<x>`).
- A member can give its own `<sprite>`s, `<animation>`, `<position>`, `<velocity>`. It **cannot** give `<collisions>`, `<actions>`, `<variables>` or timers: those are the group's, so members act the same way. A member that acts differently is an `<object>` beside the group.
- **Members are ordinary objects.** `game_xml.cpp` reads a group as one raw object per member; the engine only remembers `Object::groupName`. Each log wraps by itself, each pad dies by itself. Names are `group.N` from 1 (or `name=`); the group's name means all members wherever a name is taken (`<show>`, rules, conditions, `<reset object>`, `<fire>`, `<release>`, `<reveal>`). Drawn in file order where the group stands. `class` is the group's.
- A hidden group is a **pool** ([08](08-timers-and-enemy-behavior.md)); a group with lockstep is a block of grids (Space Invaders: three kinds of alien as one group of three grids).
- **Measured:** Frogger and Space Race got about 30% smaller (lines, bytes, tags). Still 5 lines per member and a repeated 7-line `wrap` block per lane.

| Option | Verdict |
|---|---|
| **One group per lane, members override what differs** | Chosen: one rule |
| Members give full x and y | Repeats the row in every member |
| Group position as an origin | Reads worse, hides off-screen starts |
| Nested groups, inner inherits outer | Bigger saving (the shared `wrap` block) but needs merge rules for partly given sprites |
| `<grid>` with per-row velocity and a position list | Fixes lanes only, one shape per grid |
| Reusable named templates | A different idea (opt-in parts) |
| Members overriding collisions/actions | Not allowed |

- Open: nested groups; a bare `<x>` in a member (3 lines instead of 5, about 35% total saving); evenly spaced members (`count`, `gap`). The flag that makes grid cells move as one was once also called `<group>`; it is `<lockstep>` now.
