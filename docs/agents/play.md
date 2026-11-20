# Notes: how a game runs

Tips for changing values, objects, frames, touches, spawning and loading. Back to [the notes](readme.md).

## Values and variables

- Most numbers are worked out **once at load**. Read again while running: a timer's interval, positions using a measured text size, a command's numbers that name something live (`CmdLive`, remade by `Game::makeNow`), a condition's number (`Condition::live`).
- `<random>` is drawn again when its object comes back into play, not when a scene is entered.
- Variables are `float`. Anything that changes one goes through `Game::setVariable`, so bound texts and expressions see it.
- A value's text is one number or one name (`load/calc`). Arithmetic or a function call in text stops the load: write an operation (`<subtract>`, `<smallest>`, ...). `window.width` is not a name (`window.right` is).
- Live numbers are rows of `liveParts` (`game/object.cpp`); a new one is one row.
- A new operation is a row in `operationShape` (`game/command.cpp`), a case in `evaluateOperation` (`load/expr.cpp`) and the schema types. Each `Answer` has a `late` flag for values not final while loading; keep it.
- A text is compiled once (`game_expr::compiled`); a name is bound into `calc` only when first read. There is no second expression system.

## Objects, states and scenes

- An object's collision rules, actions and timers are `object.kind` (how far its timers have counted is its own, `object.clocks`) (`Kind`, `game/object.h`), shared by every member of its group and by its copies: never change them while playing. What a rule says of `self` or `other` is found as it runs (`Game::makeNow`, `Game::holdsFor`), so a command or a test must never be made for one object by name at load.
- An object's changing numbers are its `Moment` (`game/object.h`), with `start` (a whole `Moment`) and `before` (only its position, `collisionsOn` and `isVisible`, what the next frame reads). A number a reset should put back goes in `Moment`, unless only some objects have it: then it goes in their part, with a `start` the reset puts back (a cell piece's `Object::Cells::Place`). Read a cell place with `Game::place` (an empty one for an object with none) and write it with `placeFor`. `placed` marks a reset or load between frames, whose place is not a move.
- The rest of an object is in parts, one per feature (`object.drawn`, `object.motion`, `object.turning`, ...). Parts few objects use live in a store each (`Game::parts()`, which the frame reads as `FrameRules::parts()`: holders, drives, hands, mazes, cells, tethers, pulls, alofts, rests, carries, pickers), moving there one by one (plan CR3). A new field goes in the part of its feature, or starts one; `struct Object` keeps only what every object has.
- The pieces (top-level `<objects>`) and every `from=` are worked out on the XML first (`load/merge.cpp`); a generated program runs the same code on its tree.
- `layOutScenes` flattens owned objects into `data.objects` named by path and writes every name out in full.
- `isShown` is visible and running in the top state; `isStill` is drawn under a `<show state>`; `isAway` is in a scene not current.
- `Game::find` falls back to the end of one path, so tests may say `object("ball")`; `carry.what` and owners hold full names.
- **`<reset object="x" />` resets every variable of x;** a rule's bare `<reset />` only its own object. Never put `<reset object="player" />` in someone else's rule; keep score and lives on an object no state shows.
- A bare `<reset />` in an object's timer only moves it back; it does not restart timers or variables.
- `collisionData.others` is the object-against-object rule list.

## Frames and touches

- Timers count frames. A push starts a state's timers over; a pop back does not.
- `Game::applyPaths` runs after timers, before acceleration. Anything ending a path clears `path.name`.
- An object mid-jump (`isAirborne()`) is in no pair. A resting object stays touching its platform only because its pull moves it each frame, so `<stop />` ends that.
- **Touches at one moment run in file order, and a still pair is skipped:** a `<land />` that runs first can hide a hazard met at the same moment (Pitfall's jaws are written before the crocodiles).
- `sprite=` on a rule is the look as the touch happened, taken once.
- **Shared pictures:** `drawn.picture` and a look's `picture` are a `HeldPicture`, one between an object and its looks. Read through `->`; write through `edit()`, which copies a shared one first. A write that skips `edit()` will not compile, so a look is never changed by accident.
- **Swings:** a swing's turn is `turning.spin`; `moveObjects` (its `FrameSweep`) turns it through the frame in `advanceTo`, and `sweepTurning` tests the pictures at each whole degree. The heading follows half a cosine over its `Swing` part's `seconds` (`Parts::swings`, given by the first `<swing>`). `knock` uses `velocityAt(touch.point)`.
- **Resting:** pixel contact depth is measured by pixels; `knockedApart`'s settle (four frames of pull) stops a resting ball hopping; `<friction>` (`rub`) may set `rest.resting`, which `applyAcceleration` wakes when the thing or a support moves.
- Drag and grip in a frame are `surface.groundDrag`/`surface.groundGrip` from `findGround`; use those, not `drag`/`grip`.
- Edge rules: `sprite=`/`unless=` are an `EdgeGuard`; `class=` or `object=` on one is a load error.
- `Game` never touches audio: `<play>` queues a name.

## Spawning

- A verb's own numbers (`RawSpawn`, `RawHop`...) are renamed for `self`, a copy's name and a scene's names only through `forEachValue`: a new number goes in its struct's `eachValue`, and nowhere else.
- Copies wait in `Game::spawned` and join, and despawned ones go, in `settleSpawns()`, which calls `rosterChanged()` and drops every cache.
- A spawned piece lands beside its spawner, so copies are named by that list (`playing.wave1.bomb.1`).
- New state that points into `objects` must be rebuilt in `rosterChanged()`.

## Caches

Each rests on something that never changes while a game runs: `FrameRules::partners` and `mazeGrid`; `find`'s answers and `matching`; `game_expr::compiled`; `isShown`, kept until `stageChanged()`. Anything new that changes the state stack, a scene or an object's owner calls `stageChanged()`.

`xgetest-long "[speed]"` prints every game's load and frame times to compare builds.

## Loading

- The schema checks shape and where commands go; the loader checks names.
- A verb is one file in `xge/lib/verbs/`: its name, places, `read`, `make`, `check`, `run`, `hold` and `print`, and the work it does. Its table lists only what it has; `run` takes its own `Cmd*` through `runAs<runX>`.
  - A new verb needs that file, a line in `verbs/all.cpp` and in `games/generators/engine-cpp/engine.xml`, its `Cmd*` struct in `game/command.h` and the schema's command group; `xgetest "[verbs]"` says which is missing.
  - The folder is compiled as one unit, so name a file's functions for its verb (`makeBounce`), never in an unnamed namespace.
  - Use `didYouMean` and `listOf` in any name error.
- `on="hold"` and `on="release"` answer only a press made in the current state.
- Every name a rule, condition, show or holder uses must match something (`xge/tests/engine/test_names.cpp`), so a test's game must have the classes, objects and variables its rules name: give it a far-off object of that class rather than a rule about nothing. Suggestions compare names as written in their state too (`log` for `playing.log`).
