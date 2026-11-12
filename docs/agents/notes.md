# Notes for AI coding agents

Gotchas that are easy to get wrong. How the engine works is the [reference](../reference/); why, and the backlog, is [the design index](../designs/00-designs.md). Rules and layout: [AGENTS.md](../../AGENTS.md).

## The repository

- The games are in `games/descriptions/`, one test file per area in `xge/tests/engine/` and per game in `xge/tests/games/`. New work lands on `working` ahead of `master`: check `git branch -r` and `git log` first.
- The author builds on Windows (Visual Studio, vcpkg). Agents build on Linux, often without Qt, and cannot watch or listen.
- Windows headers define macros a Linux build never sees: `near`, `far`, `min`, `max`, `small`, `interface`, `ERROR`. Never name anything one of them (`-Dnear= -Dfar=` on a syntax-only compile catches it).
- MSVC flags what GCC misses, in tests too: C4702 (unreachable code; also a loop whose body always throws, as `fail` does: test the first child with an `if`), C4459 (a local hiding a file-level name).

## Values and variables

- Most numbers are worked out **once at load**. Read again while running: a timer's interval, positions using a measured text size, a command's numbers that name something live (`CmdLive`, remade by `Game::makeNow`), a condition's number (`Condition::live`).
- `<random>` is drawn again when its object comes back into play, not when a scene is entered.
- Variables are `float`. Anything that changes one goes through `Game::setVariable`, so bound texts and expressions see it.
- A value's text is one number or one name (`load/calc`). Arithmetic or a function call in text stops the load: write an operation (`<subtract>`, `<smallest>`, ...). `window.width` is not a name (`window.right` is).
- Live numbers are rows of `liveParts` (`game/object.cpp`); a new one is one row.
- A new operation is a row in `operationShape` (`game/command.cpp`), a case in `evaluateOperation` (`load/expr.cpp`) and the schema types. Each `Answer` has a `late` flag for values not final while loading; keep it.
- A text is compiled once (`game_expr::compiled`); a name is bound into `calc` only when first read. There is no second expression system.

## Objects, states and scenes

- An object's changing numbers are its `Moment` (`game/object.h`), with `start` and `before`. A number a reset should put back goes in `Moment` and nowhere else. `placed` marks a reset or load between frames, whose place is not a move.
- The pieces (top-level `<objects>`) and every `from=` are worked out on the XML first (`load/merge.cpp`), and the same in `merge.xsl`. Change one, change the other step for step: `xgetest-generated` compares the order of objects.
- `layOutScenes` flattens owned objects into `data.objects` named by path and writes every name out in full.
- `isShown` is visible and running in the top state; `isStill` is drawn under a `<show state>`; `isAway` is in a scene not current.
- `Game::tryGetObject` falls back to the end of one path, so tests may say `getObject("ball")`; `carrying` and owners hold full names.
- **`<reset object="x" />` resets every variable of x;** a rule's bare `<reset />` only its own object. Never put `<reset object="player" />` in someone else's rule; keep score and lives on an object no state shows.
- A bare `<reset />` in an object's timer only moves it back; it does not restart timers or variables.
- `collisionData.others` is the object-against-object rule list.

## Frames and touches

- Timers count frames. A push starts a state's timers over; a pop back does not.
- `Game::applyPaths` runs after timers, before acceleration. Anything ending a path clears `followPath`.
- An object mid-jump (`isAirborne()`) is in no pair. A resting object stays touching its platform only because its pull moves it each frame, so `<stop />` ends that.
- **Touches at one moment run in file order, and a still pair is skipped:** a `<land />` that runs first can hide a hazard met at the same moment (Pitfall's jaws are written before the crocodiles).
- `sprite=` on a rule is the look as the touch happened, taken once.
- **Swings:** a swing's turn is `Object::spin`; `moveObjects` turns it through the frame in `advanceTo`, and `sweepTurning` tests the pictures at each whole degree. The heading follows half a cosine over `swingFrames`. `knock` uses `velocityAt(touch.point)`.
- **Resting:** pixel contact depth is measured by pixels; `knockedApart`'s settle (four frames of pull) stops a resting ball hopping; `<friction>` (`rub`) may set `Object::resting`, which `applyAcceleration` wakes when the thing or a support moves.
- Drag and grip in a frame are `groundDrag`/`groundGrip` from `findGround`; use those, not `drag`/`grip`.
- Edge rules: `sprite=`/`unless=` are an `EdgeGuard`; `class=` or `object=` on one is a load error.
- `Game` never touches audio: `<play>` queues a name.

## Cells and boards

- A free cell is one `cellHolder` finds nothing in, so a picker or marks on the board's own map would block it: keep them on a map of their own.
- `countMoves` fills live `moves`; it is 0 while the piece is not shown.
- `<put>` turns a trapped piece by despawning it and spawning the putter's template; copies sit in `spawned` until the frame ends, so live `moves` leaves out pieces put this frame.
- `<think>` copies every kind it reads (`Kind`) and never points into the game, since in a window it runs on its own thread (`thinkInBackground`). Tests have no `loop`, so they search in the frame. Random choice among equal moves is made on the game's side.
- `Engine::runAsPress` judges each trigger's `from=` by the look at the press. An axis `<trigger from=>` is honoured: give the look, or it runs in every look.
- `cellHolder` answers from a snapshot while a `Game::CellsStill` guard lives. Never move a piece on cells while one is held.
- Maps with the same origin and tile are layers: `piecesAbout` (here/around) looks at every layer, a step meets only its own map. `triggerAbout` reaches each piece once per outermost trigger (`aboutReached`), which is what stops a flood.
- `runAction` does nothing for an object `isAway` (in a scene not current): a state's key naming `worker` would otherwise press every room's worker.
- Undo snapshots are kept only when the game has an `<undo />` key (`keepsUndo`), and only for a press that moved a cell piece (`pressBegins`/`pressEnds` around `Engine::runAsPress`). A press made any other way keeps nothing to undo.

## Spawning

- A spawn's position, velocity and acceleration halves are `RawCommand` fields renamed for `self` in `object.cpp`, given scene values in `scenes.cpp`, generated in `data.xsl` and compared in `test_generated_games.cpp`. A new half goes in all of them.
- Copies wait in `Game::spawned` and join, and despawned ones go, in `settleSpawns()`, which calls `rosterChanged()` and drops every cache.
- A spawned piece lands beside its spawner, so copies are named by that list (`playing.wave1.bomb.1`).
- New state that points into `objects` must be rebuilt in `rosterChanged()`.

## Looks and pictures

- Every engine-drawn picture is `ShapeKind::Line`, built in `buildPicture`. Never draw or turn in a backend.
- Looks that move come **before** still sprites in the look list, so look 0 is the first animation; an object with `<poses>` starts as standing.
- A pose is picked only while the current look is a pose look.
- `<become object="group">` changes members out of play too.
- `<stop />` takes the pull away until a reset, so it is no landing for something that can be knocked up again.
- `<grip>` below 1 brakes an object with no keys held: never give it to a thing that moves by velocity alone.
- A hop's `<arc>` is drawn through `altitude`; tests compare positions, not drawn spots.
- A `RawObject` made in code (a deck's cards) must have `sprite` equal to `looks.front()`.
- Cards draw by `drawOrder`: 0 first in file order, then the rest sorted.
- A `<become>` does not change an object's size until the frame ends, so `name.width` later in the same list is the old look's. Give looks placed by size the same size.

## Caches

Each rests on something that never changes while a game runs: `FrameRules::partners` and `mazeGrid`; `tryGetObject`'s answers and `matching`; `game_expr::compiled`; `isShown`, kept until `stageChanged()`. Anything new that changes the state stack, a scene or an object's owner calls `stageChanged()`.

`xgetest-long "[speed]"` prints every game's load and frame times to compare builds.

## Loading

- The schema checks shape and where commands go; the loader checks names.
- A verb is one file in `xge/lib/verbs/`: its name, places, `read`, `make`, `check` and `run`. A new verb needs that file, a line in `verbs/all.cpp` and in `games/generators/windows-cpp/engine.xml`, its `Cmd*` struct in `game/command.h` and the schema's command group; `xgetest "[verbs]"` says which is missing. The folder is compiled as one unit, so name a file's functions for its verb (`makeBounce`), never in an unnamed namespace. Use `didYouMean` and `listOf` in any name error.
- `on="hold"` and `on="release"` answer only a press made in the current state.
- Every name a rule, condition, show or holder uses must match something (`xge/tests/engine/test_names.cpp`), so a test's game must have the classes, objects and variables its rules name: give it a far-off object of that class rather than a rule about nothing. Suggestions compare names as written in their state too (`log` for `playing.log`).

## Tests

- A new schema word needs a game that uses it and a mention in `docs/reference/` (`xgetest "[vocabulary]"`); one no game uses goes in `kUnused` in `test_vocabulary.cpp` with why it stays.
- A test that builds a `Game` without an `Engine` calls `setCurrentState(0)` and `measureShapeSize`, or nothing is shown and sizes are 0.
- A command run between frames leaves its copies waiting: call `settleSpawns()` first. Never hold a reference to a copy across frames; look it up by name.
- The working directory is `games/`, so a test names `descriptions/<game>.xml` and finds the schema with `findSchema("descriptions/pong.xml", "../xgedef.xsd")`. ctest runs from `output/<config>/games`, a plain run from the repository's `games/`: check a new schema test both ways.
- A scratch file is named with `scratchName()`, so `ctest -j` never has two cases writing one file.
- Before touching `circleOverlapsBox`, add a test to `test_collision_geometry.cpp`.
- `invaders_fixture.h` is a frozen copy of the first Space Invaders.

## Backends and build

- Never destroy a library's window after creating the next (raylib, GLFW). A front end that pauses calls `Engine::pump()`.
- Pace is `Engine::loop`'s alone: no window sets a frame rate or waits out a frame (vsync on is fine). `xgetest-long "[pace]"` checks it in real time.
- raylib: `pollEvents()` must not call `PollInputEvents()`; Escape is not its exit key; a texture drawn into a render texture must live until `EndTextureMode()`.
- SDL is shared: each of window and sound starts only its subsystem; the last out calls `SDL_Quit()`.
- Backend files compile only behind `XGE_WITH_<NAME>`: build with no options as well as with tests when touching factories, `cli.cpp` or `session_options.cpp`.
- SFML and raylib both static from source fail to link on Linux; `-DBUILD_SHARED_LIBS=ON` links. A target named `m` collides with plutovg's `-lm`.

## Generator

- `data.xsl` is a second reader: a new tag or `Raw*` field needs the same there (and in the describer in `test_generated_games.cpp`), maybe a helper in `game_parts.h`.
- Keep generated statements that make big temporaries in helpers (`addCell`, `addMember`): a Debug build gives each statement its own stack slots, and Breakout's bricks once overflowed a 1 MB stack. Check under `ulimit -s 1024`.
- XSLT comments cannot hold `--`; libxml2 refuses the stylesheet.
- A new engine source a generated program needs goes in `engine.xml` and `assets.cmake`; one that reads XML or chooses libraries stays out (keep `game/game.cpp` and `engine/engine.cpp` free of `load/xml.h` and the factories).
- After a generator or `Raw*` change, `cmake --build build --target xgetest-generated`.

## xgegui

- No OpenGL through Qt: do not bring back a `QOpenGLWidget` without a guard around every library call.
- Anything running while `GameSession` swaps game or window checks its `changing` guard.
- Do not style `QSpinBox` borders (the arrows vanish).
- Left and right Shift come from the native scan code (`key_queue.cpp`).

## Docs

- Docs move with code: a tag or behaviour change updates its [reference](../reference/) page and the matching design file in the same commit.
- Reference pages say what is, with short sections and small examples. Design files say why, near 6 KB each, with no dates or diaries.
- A change only eyes, ears or MSVC can check adds a line to "Waiting for a look on Windows" in `docs/designs/58-open-work.md`; the author deletes it once seen.

## Privacy

This repository is public. Nothing personal from a chat export goes in it: no health, identity, contact or business details, no conversation titles, and no names beyond the `beefviper` handle in the code headers. Design files call the owner "the author".
