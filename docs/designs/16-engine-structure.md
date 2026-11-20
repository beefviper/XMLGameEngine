# The engine's structure

**Status:** Built, except the later steps of components (CR3), which are the [C++ review](../plan.md#the-c-review) section of the plan.

## What we want

The language had a pass that asked of every word whether it was needed. The C++ gets the same pass: a few ideas, each in one place, and nothing an object, a command or a picture carries that it does not use.

## What was measured

| Thing | Found |
|---|---|
| One `Object` | 4,736 bytes, whatever it is. A Pong ball carries a card holder (776 bytes), a hand (192), cell-map state (184) and key-drive state (224) |
| One object as written (`WrittenObject`) | 14,600 bytes |
| One command as written (`WrittenCommand`) | 3,328 bytes: 61 fields shared by every verb, most of them empty for any one (2,496 after CR7, each verb's own). A `Command` is 184 |
| Pictures | Rows, SVGs and sheet cells are already drawn once per description (`drawOnce`), so Space Invaders' 77 objects hold 16 pictures. Lines and polygons are drawn per object; circles, rectangles and texts are drawn again each time a window asks. Five caches share them: `drawOnce`, the flips, the open sheets, each `Turnable`'s turned pictures and a static map of image files |
| Textures | Each window keeps a texture per object name and builds it again whenever the object's picture changes: in 600 frames Space Invaders builds 1,109 textures (9 MB of pixels) for its 16 pictures, Pac-Man keeps 777 textures for 35 |
| `before` | A whole `Moment` (its variables map and three vectors included) copied for every object every frame, read in one place for its position and two flags |
| The reader | Looks each child up by name from the first child again (about 40 lookups an object), making a new node object for every sibling it passes, after the whole tree was copied once |
| One tag | Written in about twelve places: `<drag>` was in the schema, the object reader, the group reader, the written object, `load/build.cpp`, `Object::Motion`, the frame, the printer and the generator's second reader |
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

## What was done

| Task | Result |
|---|---|
| CR1 | sizes on record (`xgetest-long "[size]"`); one walk over a command's values (`forEachValue`) in place of six hand lists, five of which missed `<arc>` and `<below>` |
| CR2 | `Resources` (`engine/resources.h`): each picture and sound held once, named by an id; windows keep a texture a picture |
| CR4 | `Engine::run` is `input()`, `update(seconds)`, `render()`; `Game::updateObjects` and `playOneFrame` are named stages only |
| CR5 | every game checked by a schema; the reader trusts it (`fail(` 251 to 156) and walks each element once (`Children`); Pac-Man's `--check` 3.69G to 2.07G instructions |
| CR6 | the generator's second reader (`data.xsl`, `merge.xsl`, about 6,000 lines) gone: a generated program holds its game as a tree and runs the engine's reader |
| CR7 | each verb owns its data; `WrittenCommand` 3,328 to 2,496 bytes |
| CR8 | verb-name checks 14 to 2; names as ids while playing, frames about 10% faster |
| CR9 | the names below; comments cut a folder at a time |

## Decided by the author

- **One owner of resources.** Everything loaded or made (pictures, turned and flipped and recoloured pictures, sheets, fonts, sounds, schemas, a window's textures) is held once by a resource manager and named by a small id. A hundred objects with one picture share it. A picture's id is one pointer to where the manager keeps it (as a `Name` is into the names), so it reads like the picture wherever an object is at hand, with no manager passed in; the author asked for ids, and this one is also the picture. Two things outlive a game and stay the program's, each read once: a parsed schema (a game is read before any `Engine` exists to hold one) and a font file (texts are measured where no game is at hand, on the computer's thread too, so its lock stays).
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

There are four, and the names say which:

| Name | What it is |
|---|---|
| `Written...`, `written...` | as the file writes it: text, not yet worked out |
| `start` | worked out when the game loaded; what a reset returns to |
| (no prefix) | now |
| `before` | at the end of the last step |

## Names (plan CR9)

The author approved these as a table, and each was made by tool, one commit a row, with the generator's files and the docs.

| Was | Is | Why |
|---|---|---|
| `Raw...` (69 types) | `Written...` | as the file says it; `WrittenValue` is read while playing too, and "written" still says what it is |
| `raw`, `rawObject`... | `written`, `writtenObject`...; a verb's own part, once `written`, is `own` | the same word as its type |
| `game_expr` | `Evaluator` | it works written values out, at load and while playing |
| `getObject`, `tryGetObject`, `findObject`, `findAnywhere` | `object(Name)` (throws), `find(Name)` (null), beside `find(ObjectId)`; `findSpawning(Name)`; private `search` | five names for one question |
| `holdVariablesForExpressions`, `heldFor`, `refreshBoundTexts`, `refreshAllBoundTexts` | `shareVariables`, `expressionSlot`, `variableChanged`, `relabelAll` | what each does, not how |
| `liveNumberParts` | `livePartNames` | the parts' names; `liveParts` says how each is read |
| `Game::nameOf` | `Name::of` | a wrapper left from the per-game table |

Kept, as they say what they are: `start`, `before`, `Moment`, `Kind`, `Doer`, `Compared`, `CommandExecutor`, and the `xxxOf` (reads a part) and `xxxFor` (gives one) pairs.

Then the comments, a folder at a time: those that told history, repeated a reference page, described arithmetic in strings the language no longer has, or had been left stacked on the wrong function went. The share of comment lines fell most in `xml/` (16% to 10%) and least in `game/` and `verbs/` (19% and 13%, mostly verb notes that say why). The rule is in [AGENTS.md](../../AGENTS.md#rules).

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

## Special rules (plan CR8)

Where one mechanic bent shared code for itself, and what each became (the author agreed the list).

| Where | What it does | Proposal |
|---|---|---|
| `game/scenes.cpp` (4), `game/object.cpp` (2) | Renames know which verbs' `variable=`, `into=`, `from=` and `object=` name a variable, an object or an exact object, by listing verbs (inc, dec, copy, rank, set, spawn, send, trigger) | Done: `Verb::names` says what each names (`.into = Naming::Object`); renames ask the verb |
| `object.cpp` `nameSelfIn` | A `<trigger>` with no object= means self | Done: `Naming::OneObject`, left out the one running it |
| `load/expr.cpp` | `<set>` and `<spawn>` are always made again as they run | Done: `Verb::alwaysLive` |
| `load/xml.cpp`, `load/expr.cpp` (3) | A state's `<starts>` (the deal) reads send, distribute and order by hand into its own `WrittenStart` | Done, smaller than first proposed: a step is read once into its kind (`WrittenStart::Step`). The deal is worked out at load into the holders, not run, so it does not become commands |
| `load/expr.cpp` | A `<pick>` of a held class makes its object a hand | Stays: a fact about the object worked out from its actions, now read from `WrittenPick` |
| `verbs/all.cpp` | A hint for the old `<accelerate>` | Done: removed |
| `verbs/all.cpp` | The shared send reader asks whether it is a send or a distribute | Done: each passes whether it takes `<all />` |
| `load/expr.cpp` | `condition` in a list | Stays: the list's own flow, not a verb |
| `game/cells.cpp` `goThrough` | `<through />` sets a game-wide flag that the cell step reads back after the touch's rules | Done: `Game::reactTo` returns what its rules asked (`Reaction::through`) |
| `hasAim` | A second facing, as a vector, set by a diagonal move and `<aim>`, cleared by four other verbs | Done, smaller: `aim` is a `std::optional`; facing stays one of four ways, which 38 places read |
| `placed`, `hopPending`, lockstep | A move made by a command, a hop waiting for the frame, a block moving as one | Stay: parts any object can have, each read in one place |
| `isHand`, `onCells` in verbs | A verb does a different thing to a hand or a piece on cells | Stays: each is the verb's own choice, in its file |

Verb-name checks outside `verbs/`: 14 before, 2 now (pick, condition).

Built twice, and what became of it (plan CR8 step 3):

| What | Now |
|---|---|
| A vector's dot, cross and length: three helpers, two lambdas and 25 places by hand | `dotOf`, `crossOf`, `lengthOf` beside `Vector2f` |
| Six `execute...` calls on the executor that only made a `Doer` and called `run` | `run` with `Doer::of`, `meeting`, `atEdgeOf` or `finding`; the key and pointer entries stay, as they do work |
| A touch straight across an edge, made by hand twice | `touchAcross` |
| Stopping what carries a thing out through an edge, for the window and for a touch | `stopGoingOut`, used by `stickInside` |
| A shot by facing, worked out side by side | `shotAlong` the facing's way |
| A null-safe `getAttribute` for nodes never null, and `readValueBody` in the header | Gone; the body is private to `readValue` |

## Text looked up while playing (plan CR8 step 4)

The author: a name is looked up once, when it is read or when its object comes into being, and then held as an id; nothing is found by its text while playing. Measured with callgrind over `[speed]` (every game, 3,600 frames, only `Engine::update` counted): 555G instructions, of which string work alone (compare, copy, size) is 123G, 22%.

| Family | Measured | Becomes |
|---|---|---|
| Variables by name: `Variables::find` compares text along an object's list | 10.1%, nearly all from the card search (`valueOf` in `meldFits`) | variables keyed by `Name`; a command, condition or rule holds the `Name` it reads |
| Objects and groups by name: `named`, `inGroup`, `Name == std::string`, `getObject` | 4% | commands, rules and timers hold `Name`s, compared as ids |
| Names in a number (`Calc::has`, `evaluateNow`, `refreshLive`) | 2.5% | each name in an expression bound to where it is read, at load |
| Text made while playing: timers joining `owner.name`, a scene asked for by name, looks compared by name | about 1% | made once at load |

`self`, `other` and a spawned copy's name are only known while playing: each gets its id once, when it is known.

The names are the program's, not each game's (the author chose it on a card): one table, each text once, so that a reader, a command made with no game at hand, or a test can make a name. Making a name takes the table's lock, which the computer's thread needs; reading and comparing take none, and nothing makes a name per frame.

After the step (callgrind, same run): string work, copies included, fell from 184G of 555G instructions to 26G of 536G. Left, each small: a value that says `self` is copied and renamed for the object running it (`stepHalf`, `makeNow`); a text that shows a number is made again when the number changes, which is what it is for; a message's text is made only when something fails. Maps the frame iterates in order (`memberThinks`, an object's actions) stay sorted by text, so that play is the same each run; maps only looked up are hashed by id.

Not a name, but found on the way: the card search (`meldDeadwood`, gin rummy's think) is 40% of all play, and builds a `std::vector<float>` for each meld it tries (10%).

## Open

- The names table.
- Whether parts are moved before or after the frame is reshaped; the plan moves storage first.
