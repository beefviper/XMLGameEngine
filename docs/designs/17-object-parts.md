# An object's parts

**Status:** Counted; parts go in a store each (the author). The per-part proposals are applied as each part moves ([CR3](../plan.md#cr3-parts-as-components)); `Holder`, `Drive`, `Hand`, `Maze`, `Cells`, `Tether`, `Pull`, `Aloft`, `Rest`, `Carry`, `Picker` (with `marks` in it), `Turns`, `Item`, `SizeUse` (from `layout`) and `Climb` (from `footing`) have moved. The collision rules and actions are one `Kind` shared by a group's members and its copies (step 5a); this table's "Kind" column is the older sense, a part's grouping. A piece's place on a map of cells (column, row, `<go>`, chain links, joins) is in its `Cells` part, not in the `Moment`, so only pieces of a cell map carry it. Why parts become components is in [engine-structure](16-engine-structure.md).

## How they were counted

Every shipped game was loaded, started with Space and played 1,800 frames with the movement keys held in turn. An object counts as using a part if, at any tenth frame, the part was not as a fresh object has it. Spawned copies count as objects: 5,926 in all. Sizes are `sizeof` on Linux (GCC, 64-bit); `Object` is 4,752 bytes and `Moment` 216.

Parts used by under 1% of objects but present in all of them cost the most: `Holder` alone is 776 bytes in every object and used by 75.

## The table

Kind is as [engine-structure](16-engine-structure.md#how-the-parts-are-grouped) defines: a verb's own, shared, or a family's. Reset: what a reset does today, by hand in `resetObjectState` (`game/game.cpp`) unless it is in `Moment`.

| Part | Kind | Verbs or tag | Bytes | Objects | Reset | Proposal |
|---|---|---|---|---|---|---|
| `drawn` | shared | every sprite | 280 | 5,722 | holes undone | stays in the object (the author), its picture a `PictureId` |
| `map` (`MapPlace`) | family | `<map>`, cells | 104 | 2,550 | none | merge with Moment's cell fields as `cellPlace` |
| Moment's cell fields | family | `<map>` cells, `<chain>`, `<joins>` | (in Moment) | 877 | Moment | into `cellPlace`, its start kept by the store |
| Moment `variable` | shared | variables, numbers | 48 | 2,297 | Moment | keep in the object |
| `collisionData` | shared | `<collisions>` | 248 | 1,279 | none | keep, rename `collisions` |
| `looks` | family | named sprites, `<poses>`, palettes | 176 | 881 | first look | keep |
| Moment velocity, acceleration | shared | every mover | (in Moment) | 865 | Moment | stays in the object (the author) |
| `motion` | shared | `<speed>`, `<drag>`, `<grip>`, `<mass>` | 32 | 767 | none | keep |
| `layout` | family | sizes from other objects, cells | 64 | 534 | none | keep, rename `sizing` |
| `timers` | verb's own | `<timer>` | 24 | 476 | started over | keep |
| `dark` | family | `<dark>`, `<light>` | 8 | 466 | unseen | merge with Moment `light` and `lightPast` as `light` |
| `item` | family | cards | 80 | 430 | none | keep |
| `action` | shared | named commands | 48 | 374 | none | keep, rename `actions` |
| `animation` | family | `<animation>` | 176 | 330 | first picture | keep |
| `cell` (`Cells`) | family | `<shape>`, `<chain>`, `<checks>` | 184 | 185 | none | keep, rename `cellPiece` |
| `moves` | family | board `<moves>` | 32 | 99 | none | keep |
| `surface` | family | `<surface>` | 28 | 86 | none | keep |
| `holder` | family | cards | 776 | 75 | items back | split: what was written (strings, shared by copies) from what it holds now |
| `follows` | shared | live parts | 24 | 73 | none | keep |
| `path` | verb's own | `<follow>` paths | 56 | 49 | stopped | keep |
| `turning` | shared | `<heading>`, `<pivot>`, `<turn>`, `<spin>` | 20 | 43 | spin stopped | keep; `swing` joins it |
| `drive` | verb's own | held `<move>`, `<thrust>`, `<turn>` | 224 | 37 | let go | keep |
| `mirror` | family | `<mirror>` | 32 | 28 | faces as it started | merge into `looks` |
| `turns` | family | cards, `<turns>` | 72 | 26 | none | keep |
| `footing` | shared | `<jump>`, `<climb>`, `<land>` | 56 | 15 | grounded off | keep |
| `hand` | family | cards | 192 | 14 | emptied | keep |
| `maze` | family | `<maze>`, `<go>`, `<hunt>`, `<wander>` | 256 | 11 | walk forgotten | keep |
| `tether` | family | `<tether>` | 72 | 5 | none | keep |
| `flight` | shared | `<hop>`, `<jump>`, `<land>` | 32 | 4 | landed | keep |
| Moment `altitude`, `rise` | family | `<altitude>`, `<shadow>` | (in Moment) | 4 | Moment | merge with `aloft` as `altitude`, its start kept by the store |
| `marks` | family | the picker's `<marks>` | 72 | 4 | cleared | merge into `picker` |
| `swing` | verb's own | `<swing>` | 16 | 2 | let go | into `turning` |
| `picker` | family | board `<pick>`, `<place>` | 72 | 1 | dropped | keep; `marks` join it |
| `rest` | shared | resting contacts (pinball) | 104 | 1 | not resting | keep |
| `pull` | verb's own | `<pull>`, `<seek>` | 64 | 1 | none | keep |
| `aloft` | family | `<altitude>`, `<shadow>` | 48 | 1 | none | into `altitude` |
| `carry` | verb's own | `<carry>`, `<drop>` | 72 | 1 | dropped | keep |
| `aim`, `hasAim` | verb's own | `<aim>` | 9 | 1 | dropped | a part `aim` |

Loose fields that stay in the object: `name`, `objClass`, `groupName` and `outerGroups` (how rules find it), `line`, `copyOf` and `most` (spawns), `kept`, `ownerState`, `isDrawn`, `drawOrder`, `size`. Loose fields that join a part: `hasFacing` (into `turning`), `blink` (into `drawn`), `riding`, `hopPending` and `shifted` (into `motion`), `startDrawsRandom` (into `looks`).

## Found while counting

- **`before` is a whole `Moment`,** copied for every object every frame (`Object::remember`), its variables map and three vectors with it. One place reads it (`play/frame.cpp`, whether an object moved): its position, `collisionsOn` and `isVisible`. `before` now holds those three (`Object::Before`), which took 208 bytes and a map copy a frame off every object, and changed no game.
- **The reset is a list by hand.** `resetObjectState` names each part's in-flight fields one by one, the same kind of list CR1 step 2 is about. With a store, a part that resets keeps its start (`Store::start`) and the list goes.
- **`Picture` grew 16 bytes** in CR2 step 1 (an image's pixels); CR2 step 4 makes it an id.

## How a part is held

**A store per part (the author).** Each kind of part lives in its own collection in the `Game`, found by the object's `ObjectId`: the storage of an ECS, done the right way rather than the cheapest. Looked at and not taken: a part held by pointer in the object, which saves the same memory with fewer changed call sites (about 1,400 reads of a part in the engine and 640 in the tests go through the game instead), but keeps the object the owner of its parts.

## Open

- The proposals above are the author's to change.

