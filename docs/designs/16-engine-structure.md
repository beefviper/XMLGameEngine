# The engine's structure

**Status:** Decided in outline, not built. The work is the [C++ review](../plan.md#the-c-review) section of the plan; points marked open there are the author's.

## What we want

The language had a pass that asked of every word whether it was needed. The C++ gets the same pass: a few ideas, each in one place, and nothing an object, a command or a picture carries that it does not use.

## What was measured

| Thing | Found |
|---|---|
| One `Object` | 4,736 bytes, whatever it is. A Pong ball carries a card holder (776 bytes), a hand (192), cell-map state (184) and key-drive state (224) |
| One object as written (`RawObject`) | 14,600 bytes |
| One command as written (`RawCommand`) | 3,328 bytes: 61 fields shared by every verb, most of them empty for any one. A `Command` is 184 |
| Pictures | Rows, SVGs and sheet cells are already drawn once per description (`drawOnce`), so Space Invaders' 77 objects hold 16 pictures. Lines and polygons are drawn per object; circles, rectangles and texts are drawn again each time a window asks. Five caches share them: `drawOnce`, the flips, the open sheets, each `Turnable`'s turned pictures and a static map of image files |
| Textures | Each window keeps a texture per object name and builds it again whenever the object's picture changes: in 600 frames Space Invaders builds 1,109 textures (9 MB of pixels) for its 16 pictures, Pac-Man keeps 777 textures for 35 |
| `before` | A whole `Moment` (its variables map and three vectors included) copied for every object every frame, read in one place for its position and two flags |
| The reader | Looks each child up by name from the first child again (about 40 lookups an object), making a new node object for every sibling it passes, after the whole tree was copied once |
| One tag | Written in about twelve places ([the trail of `<drag>`](../plan.md#the-c-review)) |
| Comments | 19% of the lines in `game/`, 12% in `load/` |

`xgetest-long "[size]"` prints these, each game started and played 600 frames into a window that keeps textures as the backends do:

| Game | Objects | Pictures held | Picture KB | Textures | Textures built | KB built |
|---|---|---|---|---|---|---|
| Pong | 29 | 0 | 0 | 29 | 32 | 1,310 |
| Space Invaders | 77 | 16 | 154 | 78 | 1,109 | 9,248 |
| Pac-Man | 777 | 35 | 85 | 777 | 1,037 | 1,445 |
| Klondike | 76 | 55 | 2,903 | 76 | 76 | 3,938 |

With textures kept a picture (CR2 step 5) the same games keep 11, 27, 43 and 20 textures, and build 14, 29, 57 and 20 of them.

A4 put an object's fields into parts by feature, which made it readable. It did not make it smaller: every object still holds every part. Each part's size and how many objects use it: [object-parts](17-object-parts.md).

## Decided by the author

- **One owner of resources.** Everything loaded or made (pictures, turned and flipped and recoloured pictures, sheets, fonts, sounds, schemas, a window's textures) is held once by a resource manager and named by a small id. A hundred objects with one picture share it. A picture's id is one pointer to where the manager keeps it (as a `Name` is into the names), so it reads like the picture wherever an object is at hand, with no manager passed in; the author asked for ids, and this one is also the picture.
- **Components.** A part an object does not use is not in it. Parts live in a collection each, an object keeps an index into the ones it has, and an object's own handle carries a generation so a stale one is caught.
- **The engine's run reads as it is:** input, update, render, spelled out, so another stage can go between.
- **Names over comments.** A good name replaces a comment; one-line comments stay as landmarks over sections and functions.
- **No special rules.** One mechanic does not bend the flow for itself: where one does, it becomes a property anything could have, or it goes.

## How the parts are grouped

The author: **an object needs only its name.** Everything else is a part it has or does not have, and a few wasted bytes in a part are fine beside what is carried today. A number-holder (a score, a clock) has no place, speed or picture, so even those are parts.

What is left in the object itself: its name, its class and groups (how rules find it), its handle, and which parts it has.

Parts come in three kinds:

| Kind | What it holds | Examples |
|---|---|---|
| **A verb's own** | what one verb or verb family needs to do its work | `<swing>`'s swing; the held `<move>`, `<climb>` and `<thrust>` share the key drive |
| **Shared** | one fact that several verbs and the frame all act on | where it is and how fast (`Moment`); a flight, which `<hop>`, `<jump>` and `<land>` all act on; what it looks like |
| **A family's** | what belongs to a kind of game or an object tag, with no one verb | cell maps, mazes, holders and hands (cards), board moves, light and dark |

- **A shared fact is one part, never a copy in each verb's part.** Two copies have to be kept equal by hand, which is how the engine's lists of "every value in a command" came to disagree.
- Where two verbs only seemed to share a field by accident (`seconds` meaning a hop's time here and a wait there), each gets its own.
- Small fields with no better home are grouped with the nearest family, not left loose in the object.

## How a step runs over them

"The storage of an ECS, not free-running systems" means this. An ECS is two ideas:

- **Storage:** parts live in a collection each, and an object is an id with an index into the ones it has. Taken whole.
- **Systems:** the game's logic is rewritten as independent loops, one per kind of part ("everything with a velocity", "everything with an animation"), each run over its whole collection, in an order the framework is free to choose or to run side by side. Not taken as it stands.

The reason is that here the order of a step is part of what a game means: timers, then paths, then the pull, then the move with its touches, then conditions, and within one pass the objects in the file's order. A rule that runs mid-move changes what the next touch finds. So the step keeps its written order of passes, and a pass may walk one collection (the animations, the flights) where that reads better, in object order. That gets the gain of systems, a pass touching only the objects that have the part, without their freedom to reorder.

## Versions of one value

There are four, and the names should say which:

| Today | What it is | Proposed |
|---|---|---|
| `Raw...`, `raw...` | as written in the file: text, not yet worked out | `Written...` |
| `start` | worked out when the game loaded; what a reset returns to | `start` |
| (no prefix) | now | (none) |
| `before` | at the end of the last step | `before` |

The rename is the author's to approve as a table before anything is touched.

## What we looked at

- **A full ECS with systems free to run in any order,** as large engines have: built for tens of thousands of alike things. Taken and not taken as [above](#how-a-step-runs-over-them).
- **`std::optional` parts inside the object:** smaller change, but the object is still the sum of every part's size.
- **Parts by verb strictly:** above.
- **A resource manager as a global:** two games can be open at once in `xgegui`, and the search plays copies of a game beside it. It belongs to the game being played, with the window keeping its own textures by the manager's ids.

## Other patterns that fit at this size

- **Names turned to ids at load.** Objects, classes, variables and sounds are compared by text in places while playing. An id per name, made at load, is what a component index and a resource id already are.
- **Effects that wait.** Spawns already wait until a safe point. The same list could hold every effect that must not happen mid-pass, in place of a flag per mechanic.
- **A table for the reader.** What an object may hold, as rows of tag and where it lands, read in one pass over the children.

Each is a step in the plan only where it removes code.

## Open

- Whether `engine-cpp` may carry the game as a tree for the engine's own reader, which retires `data.xsl`.
- The names table.
- Whether parts are moved before or after the frame is reshaped; the plan moves storage first.
