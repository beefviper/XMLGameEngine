# The engine's structure

**Status:** Decided in outline, not built. The work is the [C++ review](../plan.md#the-c-review) section of the plan; points marked open there are the author's.

## What we want

The language had a pass that asked of every word whether it was needed. The C++ gets the same pass: a few ideas, each in one place, and nothing an object, a command or a picture carries that it does not use.

## What was measured

| Thing | Found |
|---|---|
| One `Object` | 4,712 bytes, whatever it is. A Pong ball carries a card holder (776 bytes), a hand (192), cell-map state (184) and key-drive state (224) |
| One object as written (`RawObject`) | 14,600 bytes |
| One command as written (`RawCommand`) | 3,328 bytes: 61 fields shared by every verb, most of them empty for any one |
| Pictures | Each object draws its own bitmap from its description, so 55 alike invaders are 55 bitmaps, and each window makes a texture per object. Only image files are shared, through a hidden static map |
| The reader | Looks each child up by name from the first child again (about 40 lookups an object), making a new node object for every sibling it passes, after the whole tree was copied once |
| One tag | Written in about twelve places ([the trail of `<drag>`](../plan.md#the-c-review)) |
| Comments | 19% of the lines in `game/`, 12% in `load/` |

A4 put an object's fields into parts by feature, which made it readable. It did not make it smaller: every object still holds every part.

## Decided by the author

- **One owner of resources.** Everything loaded or made (pictures, turned and flipped and recoloured pictures, sheets, fonts, sounds, schemas, a window's textures) is held once by a resource manager and named by a small id. A hundred objects with one picture share it.
- **Components.** A part an object does not use is not in it. Parts live in a collection each, an object keeps an index into the ones it has, and an object's own handle carries a generation so a stale one is caught.
- **The engine's run reads as it is:** input, update, render, spelled out, so another stage can go between.
- **Names over comments.** A good name replaces a comment; one-line comments stay as landmarks over sections and functions.
- **No special rules.** One mechanic does not bend the flow for itself: where one does, it becomes a property anything could have, or it goes.

## How the parts are grouped

The author's starting point is a part per verb. Looked at part by part, the rule that holds is **a part per feature, where a feature is one verb family or one object tag**:

- Most parts do come from verbs: `<swing>` has `Swing`, the held `<move>`, `<climb>` and `<thrust>` have `Drive`.
- Many come from an object's own tags, with no verb: `<holds>`, `<light>`, `<maze>`, `<moves>`, `<altitude>`.
- Some state is shared by several verbs because it is one fact: `<hop>`, `<jump>` and `<land>` all act on one flight.

**Shared state is not copied into each verb's part.** Two copies of one fact have to be kept equal by hand, which is how the engine's lists of "every value in a command" came to disagree. Where two verbs need the same data it is one part that both use; where they only seemed to share a field by accident (`seconds` meaning a hop's time here and a wait there), each gets its own.

Position, velocity and the rest of a `Moment` stay in the object: everything has them.

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

- **A full ECS with systems over packed arrays,** as large engines have: built for tens of thousands of alike things. A game here has tens to hundreds of objects, and the order things happen in a step is part of what a game means. Taken: the storage (collections, indices, generations). Not taken: rewriting the frame as free-running systems. A pass may walk one collection where that reads better, in object order.
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
