# Plan

What is next, and nothing else. The ordered tasks come first, each written to be picked up cold: what to do, why, which files, what "done" is, and how to check it. Under them is the backlog, which is not ordered and not decided.

- **Working a task:** do its steps in order, one commit each, and tick the step's box in that commit. When every box is ticked, delete the task from this file and from [the order](#order-of-work). Anything it leaves open goes in [the backlog](#backlog).
- **Goals with no date** are under [long term](#long-term).
- **Why something was decided** is in [the design files](designs/00-designs.md), not here. This file started as a review of the whole project; its findings became these tasks, and its settled arguments moved to the design file each belongs to.
- **When the plan and the code disagree, the code wins.** Check that a file or function still exists before changing it, and correct this file.

## Order of work

**[Delta time](#delta-time) comes first, then [the C++ review](#the-c-review);** each has its own section and order. After them, one task at a time, top to bottom. A task's steps are done in their own order, and the tests are green after every step. Where a line says "steps 1 to 4", the rest of that task comes back further down.

| # | Task | Why here |
|---|---|---|
| 1 | [F8](#f8-dice): dice | Waits on the hold on new games |
| 2 | [A7](#a7-the-next-backend-is-cheap): more backends after step 7 | Never finishes: a backend at a time |

**On hold:** [A5](#a5-a-game-written-out-as-if-by-hand), the stand-alone generator (the author: nice-looking generated code is a hard problem that may need several things to change at once). It stands at `pong_min` written, and `pong.xml` written by hand as the next example.

What must come before what:

| Task | Needs first |
|---|---|
| A1 step 4 | A5 far enough along to retire `data.xsl`, and the author's yes |
| F8 | the hold on new games lifted ([open decisions](#open-decisions)) |

Everything else is independent, so the author can move any line up or down.

## Open decisions

- **The hold on new games and genres.** New games were paused until the correctness and reduction work was done, and it is done. Whether to lift the hold now, or after the architecture block, is the author's call. Until then the additions below are chosen because several existing games want them, not to add a genre.

## Rules for every task

The rules for any change are in [AGENTS.md](../AGENTS.md), and what a language change must touch is in [the agent notes](agents/readme.md#changing-the-language). On top of those, for work from this plan:

- Work is on a branch off `working`, merged when a step's checks pass.
- Each step leaves the tests green. If a step cannot, it is too big: split it, and write the smaller steps into this file first.
- One game is moved first as the proof, then the rest in groups of about ten, running that group's tests after each.
- When a word is removed, the reader must refuse it with a message that names the new form, and a test must hold that message.

## Delta time

Its own block of work, done before anything else in this file. **Design and reasons:** [time](designs/29-time.md). Read it first; this section is only the steps.

- **The aim.** Each frame is one step of the time that really passed. A game plays the same at 8, 60, 144 or 240 frames a second and when frames come unevenly. Game files write per-second numbers.
- **The shape of the work.** The engine first (DT1 to DT4), with every game still in its old numbers and still playing as before. Then six games moved for the author to try (DT5), then the rest (DT6 to DT8), then what is no longer needed comes out (DT9).
- **How the old games keep working meanwhile.** A game without `<game time="seconds">` has its motion clock in sixtieths of a second: where a moved game's sums use `t = seconds`, an unmoved one's use `t = seconds * 60`. Its per-frame numbers then mean what they always did, and it already goes by time passed.

### Rules for this block

- **Never a fixed step, never a step cut into smaller ones.** If a sum is wrong over a long step, the sum is fixed. A loop over the ticks or timer firings a step owes is not cutting the physics up, and is right.
- **No count of frames is left in the engine** for anything that means time. A field named `frames...` or `...Every` that holds time becomes seconds.
- **How a step is checked.** Build as [AGENTS.md](../AGENTS.md#build-and-test) says, then `cd build && ctest -j4`, then `output/Release/xgetest-long "[steps]"` once DT2 step 1 has made it, and `xgetest-generated` when the reader, a `Raw...` struct or the generator was touched. A step is done when its **Check** and all of these pass with zero warnings, and its box is ticked in the same commit.
- **"The same at every step length"** in a check means: the game is played for the stated seconds in steps of a tenth, a sixtieth and a 240th of a second and in an uneven seeded mix, and the number named agrees across all four within 2 pixels (or 2 degrees, or 2% for a speed), unless the check gives its own range. A count (timer firings, ticks, pictures passed) agrees exactly.
- **About the same is enough.** When an existing test fails after a step here:
  - by a small amount in a position or speed (a few pixels, a few percent): change that expectation to a range, and say why in the commit;
  - by behaviour (nothing scores, something leaves the window, a state is never reached): it is a bug in the step, not in the test. Never loosen a behaviour check.
- **Speed.** Run `xgetest-long "[speed]"` before and after each task, back to back on one machine. A power (`std::pow`) is worked out once per share per step, never per object per pixel.
- Every other rule is as in [rules for every task](#rules-for-every-task). A step marked "the author" stops for the author.

### Order

| # | Task | Ends with |
|---|---|---|
| 1 | [DT1](#dt1-a-step-has-a-length): a step has a length | nothing changed in play |
| 2 | [DT2](#dt2-what-counts-time-holds-seconds): what counts time holds seconds | timers, animations, jumps and ticks right at any step |
| 3 | [DT3](#dt3-movement-by-the-time-passed): movement by the time passed | every sum right at any step |
| 4 | [DT4](#dt4-the-loop-plays-real-time): the loop plays real time | **the author** tries old games at several rates |
| 5 | [DT5](#dt5-six-games-moved): six games moved | **the author** plays them |
| 6 | [DT6](#dt6-the-rest-of-the-games): the rest of the games | every shipped game in seconds |
| 7 | [DT7](#dt7-the-tests-own-games): the tests' own games | no per-frame number in a test |
| 8 | [DT8](#dt8-the-stand-alone-writers): the stand-alone writers | prototypes and `windows-cpp` in seconds |
| 9 | [DT9](#dt9-take-out-what-is-not-needed): take out what is not needed | one kind of game file again |

### DT1. A step has a length

- **Why.** Everything later needs to be told how long the step is. This task only carries the number about.
- **Steps.**
  - [x] **1. The length is passed down (M).** `Engine::step`, `Game::updateObjects`, `Game::updateTimers` and `playOneFrame` (`play/frame.h`) each take `float seconds`. `Engine::loop` still plays its fixed frames and passes `1 / framerate`. The tests' `frames(n)` (`xge/tests/helpers/played.h`) passes a sixtieth. Nothing uses the number yet. **Check:** every test passes unchanged.
  - [x] **2. One place turns seconds into frames (S).** The copies of `framerate > 0 ? framerate : 60` (`game/game.cpp` three times, `game/cells.cpp` twice, `play/frame.cpp`, `load/pictures.cpp`) become one function. It is temporary: DT2 removes its callers one by one. **Check:** `grep -rn "framerate" xge/lib` shows it only there, in the reader and in `engine/engine.cpp`.
  - [x] **3. The game says its units (M).** `<game time="seconds">` in `games/xgedef.xsd` (the only value; documented as temporary), read by `load/xml.cpp` into `GameData`, carried by `data.xsl`, `modules/game_parts.h` and the describer. `Game` gets `motionTime(seconds)`: `seconds` for such a game, `seconds * 60` otherwise. Nothing calls it yet. **Check:** a test in `test_xml_format.cpp` loads a small game each way and reads the two answers; `xgetest-generated`.
- **Files.** `xge/lib/engine/engine.*`, `xge/lib/game/game.*`, `xge/lib/play/frame.*`, `xge/lib/load/xml.cpp`, `xge/lib/game/data.h`, `games/generators/engine-cpp/`, `xge/tests/helpers/played.h`.

### DT2. What counts time holds seconds

- **Why.** Timers and the rest are written in seconds but kept as whole frames, so they round, and a long step cannot pay what it owes.
- **The rule for each.** It holds seconds left as a `float`. A step takes its length off. When it reaches zero it goes off, and if the step was longer than what was left, the rest counts toward the next round, so it can go off several times in one step, in order.
- **The check for each** is a case tagged `[steps]` in a new `xge/tests/engine/test_step_lengths.cpp` (add it to `scripts/cmake/tests.cmake`; the name is not `test_pace.cpp`, which is the loop's own test in `xge/tests/long/`): a small game written in the test is played for 2 seconds at each step length, and the thing counted comes out the same each way.
- **Steps.**
  - [x] **1. The pacing helper (S).** In `xge/tests/helpers/`: `step_lengths.h` with `playFor(game, seconds, step)`, `playUneven(game, seconds, seed)` (steps between a 240th and a tenth, the last one cut so the total is exact) and `atEveryStepLength(file or text, seconds, measure)`, which loads the game afresh for each of the four, plays it, calls `measure(game)` and returns the four answers. **Check:** a case plays a game of one still object and gets four equal positions.
  - [x] **2. Timers (M).** `Timer::framesLeft` (`game/command.h`) and `Game::updateTimers`; a state's timers and an object's; the time left a game reads (`energy.level`) still rounds up to whole seconds. **Check:** a timer of 0.05 seconds goes off 40 times in 2 seconds at every pacing.
  - [x] **3. Animations (M).** `Object::Animation` (`game/object.h`), the counting in `playOneFrame`, `timeAnimation` (`game/game.cpp`), and the frames worked out in `load/pictures.cpp`. A step that passes several pictures runs each one's commands in order, and a once animation's `<then>` once. **Check:** an animation with a command on its third picture runs it the same number of times at every pacing.
  - [x] **4. Hops, jumps and swings (M).** `Object::Flight` (`framesLeft`, `frames`), `Object::Swing` (`frames`), the stagger of `<follow>` (`game/game.cpp`). A hop that ends inside a step lands at its end point and spends the rest of the step on the ground. **Check:** a hop of half a second lands at the same place and time at every pacing; `test_flippers.cpp` passes.
  - [x] **5. Cell ticks (M).** `cellGoEvery`, `cellGoWait`, `heldEvery`, `heldWait` (`game/cells.cpp`), `itemWait` (`game/game.h`). A step that owes three ticks makes three moves, each with its own touches. **Check:** a piece with a `<go>` of 8 cells a second is 16 cells on after 2 seconds at every pacing; Snake and Tetris tests pass.
  - [x] **6. `<stepped />` (S).** It moves when its picture changes, by what its velocity covered since the last change; with step 3 done this follows the animation's seconds. **Check:** the Space Invaders tests, and its block is in the same place after 5 seconds at every pacing, within a step of its own.
  - [x] **7. The frames function goes (S).** Remove the function from DT1 step 2. **Check:** `grep -rn "framerate" xge/lib` shows only the reader and `engine/engine.cpp`.
- **Files.** `xge/lib/game/command.h`, `game/object.h`, `game/game.cpp`, `game/cells.cpp`, `play/frame.cpp`, `load/pictures.cpp`, `load/expr.*`; `docs/reference/timers.md`, `docs/reference/pictures.md` where they say "frames".

### DT3. Movement by the time passed

- **Why.** This is the change itself. Every sum uses `t = motionTime(seconds)`, so an unmoved game's numbers still mean what they did.
- **The sums** are in [time](designs/29-time.md#the-sums). Each step below adds a case to `test_step_lengths.cpp` that plays 2 seconds at each pacing and finds the object in the same place, within 2 pixels.
- **Steps.**
  - [x] **1. Velocity (M).** The move a frame sweeps (`FrameSweep` and `moveObjects` in `play/frame.cpp`) is `velocity * t`; so is what `<ride />` lends and what a `<stepped />` object saves up. **Check:** a mover at a steady velocity.
  - [x] **2. Speed gained (M).** `<acceleration>`, `<thrust>`, `<pull>` and `<jump>`'s pull: the position gains `acceleration * t * t / 2` as well, then the velocity gains `acceleration * t`. `<rise>` and `<fall>` the same. The rest test ("4 frames of its own pull") becomes that many sixtieths of a second. **Check:** a thing dropped from rest; a `<jump>` reaches the same height at every pacing.
  - [x] **3. Turning (S).** `<turn>` held, `<seek>`'s degrees, a spin: degrees times `t`. **Check:** a turning thing faces the same way.
  - [x] **4. Shares (M).** `<drag>`, a `<surface>`'s, `<friction>` and `<grip>` (the easing in `play/frame.cpp`): kept over a step is `(1 - lost)` to the power `t`. The snap at 0.05 becomes a speed in the game's units. **Check:** a thing coasting under drag; Speedway and Putt Park tests.
  - [x] **5. Held keys (M).** `<move>` held (`Object::Drive`), `<climb>`, and an `<inc>` or `<dec>` on a held key add their number times `t`. **Check:** a paddle held for a second; Cannon Hill's power after a second held.
  - [x] **6. Heading for a point (M).** A path's legs (`Game::applyPaths`), `<chase>`, `<aim>`, `<hunt>`, `<go>` and `<wander>` off cells (`play/maze.cpp`), `<track />`: the distance covered is speed times `t`; one that reaches its point inside a step stops there and spends the rest of the step on the next leg or tile. **Check:** a path of four corners is walked in the same time at every pacing and never cuts or overshoots a corner; Pac-Man tests.
  - [x] **7. Screen edges in the sweep (M).** Today `touchesScreenEdge` (`play/frame.cpp`) is asked after the move. The window's four sides join `FrameSweep` as things a move can cross, so an edge rule runs at the moment of crossing and the rest of the step carries on from there, as a touch between objects does. **Check:** a ball at 2,000 pixels a second in steps of a tenth bounces off all four edges and never leaves the window; `limits.md` loses that line.
  - [ ] **8. Read what is left (S).** `grep -rn "velocity\|heading\|acceleration" xge/lib/play xge/lib/verbs xge/lib/game` and read every line: anything added, compared or scaled per frame that the steps above missed (`<knock>`'s settle, a `<fire>`'s lead, `<land>`) is fixed or noted here as right. **Check:** the list of what was read and decided is in the commit message.
- **Files.** `xge/lib/play/frame.cpp`, `play/verbs.cpp`, `play/executor.cpp`, `play/maze.cpp`, `play/collision.cpp`, `xge/lib/verbs/*`, `game/game.cpp`; `docs/reference/objects.md`, `commands.md`, `collisions.md`.

### DT4. The loop plays real time

- **Steps.**
  - [ ] **1. One step a frame (M).** `Engine::loop`: measure the time since the last frame, cut it to `kLongestStep` (a quarter second), play one step of it, draw. `<framerate>` becomes the most frames a second (the wait between frames), default 60. `kMostFramesOwed` goes. `xge/tests/long/test_pace.cpp` is rewritten for the new loop: with its `TimedWindow` taking 5, 20 and 100 milliseconds to show a picture, a mover at 100 pixels a second is within 5 pixels of 200 after 2 real seconds each time, and the pictures shown differ. **Check:** `xgetest-long "[pace]"`; `test_engine_input.cpp`; the window tests under `xvfb-run`.
  - [ ] **2. The command line (S).** `xgecli --framerate <n>` overrides the game's for a window; `--step <seconds>` sets the length of each of `--frames`' steps (default a sixtieth); each `<moment>` of a dump carries its time in seconds (`games/xgedump.xsd`). **Check:** `test_cli.cpp`; `docs/reference/programs.md`.
  - [ ] **3. Whole games at every step length (M).** `xgetest-long "[steps]"` gains one case a game, each played from a fresh load with the keys named, at 8, 30, 60, 144 and 240 steps a second and unevenly. Every run: no exception, every position finite (`finite` in `random_keys.h`). And one measure a game, in game seconds, which must agree with the 60 run within the range given:
    - Pong: the seconds from the serve to the first point with no key pressed, within 10%.
    - Asteroids: how far the ship has gone 3 seconds after 1 second of thrust, within 5%.
    - Mario Bros.: the top of one jump within 3 pixels, and its time in the air within 5%.
    - Space Invaders: where the block is after 10 seconds, within one of its own steps.
    - Snake: the cells the head has gone after 5 seconds, exactly.
    - Pinball: how high the ball gets from a full plunger, within 5%.
    **Check:** it passes; the measured numbers at each rate are in the commit message.
  - [ ] **4. The recorded demo comes out (S).** The author: drop the demos for now and record new ones later. Remove the recorded `<demo>` from the one game that has one (`grep -l "<demo" games/descriptions/*.xml`) and whatever in that game only shows it; `xgetest "[vocabulary]"` then needs `demo` and its words in `kUnused` with why they stay. Recording and playback stay in the engine, counting steps: `--keys` with `--frames` plays the given steps, so it is still exact, and the tests and "Looking at a game" keep using it. A demo played in a window follows the real steps and so only roughly repeats; that is left open ([backlog](#language-and-engine)). **Check:** that game's tests; `test_cli.cpp`.
  - [ ] **5. The author tries it.** A line in [Waiting for a look on Windows](#waiting-for-a-look-on-windows): `xgecli pong --framerate 8`, `30`, `144`, `240`, and the same for Mario Bros. and Pinball, should each play at the speed 60 does. Do not start DT5 until the author says so.
- **Files.** `xge/lib/engine/engine.*`, `xge/cli/source/`, `games/xgedump.xsd`, `xge/tests/long/`; `docs/reference/engine.md`, `programs.md`.

### DT5. Six games moved

- **Which.** `pong` (velocity, a held move, the computer), `asteroids` (thrust, turning, drag), `mariobros` (a pull, jumps, grip), `spaceinvaders` (a stepped block), `snake` (cell ticks: nothing to multiply, only the attribute), `pinball` (friction, swings, knocks). Between them they use every row of the [units table](designs/29-time.md#units).
- **Moving a game.** Add `time="seconds"` to its `<game>`. Multiply each number by its row of the table. Then read the whole file for speeds kept elsewhere: a `<variable>` that is later copied or added into a velocity, a `<set>` or `<inc>` on `x.velocity.y`, a condition that compares a speed, a spawn's or a `<fire>`'s velocity. Round to a number a person would write (`300`, not `299.99997`).
- **Steps.**
  - [ ] **1 to 6. One game a commit (S to M each),** Pong first. Its test file in `xge/tests/games/` is changed only where it names a speed. **Check:** the game's own tests; its case in `xgetest-long "[steps]"`, whose measure must also be within its range of what it was before the move; `xgecli <game> --frames 600 --seed 1 --dump` before and after lists the same objects, each within 20 pixels of where it was (a game with bounces: the same objects only). No per-frame number is left: read the diff once more for any speed under 20 or pull under 100, and say in the commit why each one left is right.
  - [ ] **7. The reference says seconds (S).** `docs/reference/objects.md`, `commands.md`, `collisions.md`, `format.md`: every "a frame" for a moved word becomes "a second", with the `time="seconds"` attribute explained once in `format.md` as temporary.
  - [ ] **8. The author plays the six.** A line in Waiting for a look on Windows. Do not start DT6 until the author says they feel right; if a sum is changed after that, the six are checked again first.

### DT6. The rest of the games

- **Steps.**
  - [ ] **1. The games with motion, ten a commit (M each).** In the order `ls games/descriptions` gives, skipping the six and those in step 2. Moved as DT5 says. **Check after each commit:** as DT5 step 1 for each game, without the `[steps]` case; and `playRandomKeys` (`xgetest-long`) for those games.
  - [ ] **2. Cards, boards and cell puzzles in one commit (S).** Games whose only numbers are positions and seconds get the attribute and a read for any `<velocity>` or `<move>` (a dealt card's slide). **Check:** their tests.
  - [ ] **3. Every game is moved (S).** `xgetest "[vocabulary]"` gains a check that every file in `games/descriptions/` has `time="seconds"`. **Check:** it passes; `xgetest-generated`.

### DT7. The tests' own games

- **Why.** About 170 lines of small games written inside 33 test files, and `invaders_fixture.h`, still hold per-frame numbers, and about 190 expectations compare a speed.
- **Steps.**
  - [ ] **1. Five test files a commit (M in all).** Each inline game gets `time="seconds"` and its numbers moved; each expectation of a speed is multiplied to match; an expectation of a position after some frames keeps its number, as a range where the new sums moved it slightly. **Check after each commit:** that file's cases.
  - [ ] **2. None left (S).** **Check:** `grep -rL 'time="seconds"'` over the test files that hold a `<game` finds none.

### DT8. The stand-alone writers

- **Steps.**
  - [ ] **1. The prototypes (S).** `games/prototypes/pong_min/` and `games/prototypes/pong/`: `frame()` takes the seconds passed, speeds are per second, and each `main.cpp` measures the time between frames. **Check:** their behaviour checks in `xge/tests/written/`.
  - [ ] **2. `windows-cpp` writes that (S).** `games/generators/windows-cpp/*.xsl` until `xgecli pong_min --generate windows-cpp` writes the prototype again. **Check:** `test_generate.cpp`; `xgetest-generated`.

### DT9. Take out what is not needed

- **Steps.**
  - [ ] **1. The attribute and the old clock go (M).** Remove `time=` from every game and test, from the schema, the reader, `GameData`, `data.xsl`, `game_parts.h` and the describer; `motionTime` goes, and every sum uses the seconds directly. A file that still has `time=` is refused by the schema like any unknown attribute. **Check:** `grep -rn "motionTime\|time=\"seconds\"" .` finds nothing; all tests; `xgetest-generated`.
  - [ ] **2. Words for frames (S).** Rename what still says frames but means steps or seconds in `xge/lib` and the tests' helpers, where it misleads. `--frames` keeps its name: it counts steps. **Check:** zero warnings; all tests.
  - [ ] **3. The docs (S).** `docs/reference/limits.md` loses "Movement is pixels per frame..." and "The window's ... frame rate are read once" is checked; `format.md` loses the temporary attribute; `engine.md` describes the loop as built; `docs/designs/29-time.md` is marked built and its "how the games are moved" becomes history-free; the root `readme.md` if it mentions frames. This section is deleted from the plan, and what is still open goes to the backlog.
  - [ ] **4. The author: tune.** The games whose speeds were never watched (Galaxian, Berserk, Tennis, Haunted House, Pac-Man) are now easy to compare with the originals; ask which the author wants looked at.

## The C++ review

Its own block of work, done **after [delta time](#delta-time)**: both touch every field of an object, and doing them at once would hide which one broke a game. **Design, measurements and reasons:** [engine-structure](designs/16-engine-structure.md). Read it first.

- **The aim.** The engine does the same things with less: a resource is held once, an object carries only the parts it uses, a tag is written in few places, and the code reads from its names.
- **What it must not do** is change what any game does. Unlike delta time, every step here leaves every game playing exactly as before.
- **One tag, twelve homes.** `<drag>` today is in the schema, the object reader, the group reader, `rawDrag` and `hasDrag` in `RawObject`, `load/build.cpp`, `Object::Motion`, the frame, the printer, two hand-kept lists of an object's numbers, `data.xsl` twice and the describer. CR1, CR5, CR6 and CR7 each take homes away.

### Rules for this block

- **No game changes.** `xgetest`, `xgetest-generated` and `xgetest-long` pass unchanged after every step. A test is edited only where it names a C++ type or field that moved.
- **Measure.** `xgetest-long "[speed]"` and `"[size]"` (CR1 step 1) before and after each task, back to back on one machine. Load time, frame time and bytes do not rise; where a task says they fall, the numbers go in the commit message.
- **One move a commit.** A part, a resource kind or a verb's data moves in its own commit, so a break is found by `git bisect`.
- **Names over comments,** in every line this block writes or moves: a comment that says what a good name would say is replaced by the name. One-line comments stay as landmarks over a section or a function, and wherever the code cannot say why.
- **How a step is checked.** Build as [AGENTS.md](../AGENTS.md#build-and-test) says, then `cd build && ctest -j4`, `xgetest-generated`, and `xgetest-long` (random keys, damaged files, `[speed]`, `[size]`). A step is done when its **Check** and all of these pass with zero warnings, and its box is ticked in the same commit.
- A step marked "the author" stops for the author.

### Order

| # | Task | Gains |
|---|---|---|
| 1 | [CR1](#cr1-a-measure-and-the-lists-that-disagree): a measure, and the lists that disagree | sizes on record; suspected bugs settled |
| 2 | [CR2](#cr2-a-resource-manager): a resource manager | one picture, sound and texture each, however many use it |
| 3 | [CR3](#cr3-parts-as-components): parts as components | an object the size of what it uses |
| 4 | [CR4](#cr4-input-update-render): input, update, render | the run reads as it is |
| 5 | [CR5](#cr5-the-reader-in-one-pass): the reader in one pass | no search per tag; the schema trusted |
| 6 | [CR6](#cr6-one-reader-not-two): one reader, not two | about 6,000 lines of second reader gone |
| 7 | [CR7](#cr7-a-verb-owns-its-data): a verb owns its data | a command the size of what it says |
| 8 | [CR8](#cr8-no-special-rules): no special rules | no mechanic bends the flow for itself |
| 9 | [CR9](#cr9-names-and-comments): names and comments | the code reads from its names |

The author's order: the resource manager and the components first, since the rest is easier once they are in.

### CR1. A measure, and the lists that disagree

- **Steps.**
  - [ ] **1. Sizes on record (S).** A hidden case `"[size]"` beside `"[speed]"` (`xge/tests/long/`): prints `sizeof` `Object`, `RawObject`, `RawCommand`, `Command`, and for Pong, Space Invaders, Pac-Man and Klondike the objects, the commands, the pictures (count and bytes of pixels) and the textures a null window is asked for. Today's first four are 4,712, 14,600, 3,328 and 184 bytes. **Check:** it prints; the numbers go into [engine-structure](designs/16-engine-structure.md#what-was-measured).
  - [ ] **2. Do the lists hide bugs (S).** Six places list "every value in a command" by hand (`game/object.cpp` in `nameSelfIn` and the two renames, `load/conditions.cpp`, `game/scenes.cpp`, `load/expr.cpp` for `live`), and only one names `arc` and `below`; the scenes' list of an object's numbers lacks seek, light and altitude. Write tests first: `self` in a `<hop>`'s `<arc>`, a spawned copy's own name in a `<spawn>`'s `<below>`, a scene object's `<light>` and `<seek>` naming another object of its scene, a live number in an `<arc>`. **Check:** each test passes or fails; the failures are the bugs.
  - [ ] **3. One walk (S).** `forEachValue` and `forEachName` for `RawCommand` and for `RawObject` (beside each struct), used by all six. **Check:** step 2's tests pass; nothing else changes.

### CR2. A resource manager

- **Why.** Each object draws its own bitmap (`drawPicture` in `load/pictures.cpp` makes a new one per sprite per object), each window makes a texture per object (`CachedVisual` in `window/sfml.h`, `opengl.h`, `raylib.h` and the rest), and what is shared is shared by `std::shared_ptr`s nobody owns and one static map (`loadPicture` in `picture/image.cpp`).
- **The shape.** One class, `Resources` (`xge/lib/engine/resources.h`, `.cpp`), owned by the `Game`:

  ```cpp
  struct PictureId { std::uint32_t index{}; };          // 0 is "none"; SoundId, FontId, SheetId, SchemaId alike
  class Resources
  {
  public:
      PictureId picture(const PictureKey& description);  // the same description gives the same id
      PictureId turned(PictureId from, int degrees);     // made on request, then kept
      PictureId flipped(PictureId from, bool horizontal, bool vertical);
      PictureId recoloured(PictureId from, const std::string& style);
      PictureId own(Bitmap made);                        // one object's alone (a holed picture)
      void release(PictureId owned);                     // only for what own() gave
      const Bitmap& bitmap(PictureId id) const;
      std::size_t pictureCount() const;                  // and pictureBytes(), for [size]
  };
  ```

  - A `PictureKey` holds everything that decides the pixels and nothing else: the kind, its rows or lines or file and part, colours, scale, flip, style, font and size. Two sprites that would draw the same pixels have equal keys.
  - Ids are never reused while a game is loaded, so an id held anywhere stays right. `own` and `release` are the one exception and are used only by `<hole>`.
  - A window keeps its textures by `PictureId`. It is told which ids are on screen and makes a texture the first time it sees one.
- **Steps.**
  - [ ] **1. The manager, with image files (M).** `Resources` with `picture()` for pictures loaded from files; `Game` owns one; the static map and its mutex in `picture/image.cpp` go. New `xge/tests/engine/test_resources.cpp` (add to `tests.cmake`). **Check:** in it, the same file asked twice gives one id and `pictureCount()` of 1; two files give two; a missing file throws the message it does today.
  - [ ] **2. Drawn pictures (L, one kind a commit).** In this order: bitmap rows, lines, polygons, circles and rectangles, texts, sheet cells, SVGs. For each, `drawPicture` builds the key and asks the manager. **Check for each kind,** a case in `test_resources.cpp`: a game written in the test with a group of 20 members sharing one sprite of that kind has `pictureCount()` of 1; changing one member's colour makes it 2. **Check at the end:** `[size]` shows Space Invaders' pictures fewer than its objects, and its load time in `[speed]` has fallen; both numbers in the commit.
  - [ ] **3. Made pictures (M, one kind a commit).** Turned (`Turnable` in `picture/bitmap.h`, which keeps its own map today), flipped (`flipsOf`), palette swaps (`makePalettes`), redrawn at the window's size, and holed (`own`, released when its object is reset or despawned). **Check:** cases in `test_resources.cpp`: two objects with one picture turned to the same heading share one turned picture; a `<hole>` in one of two alike shields leaves the other's picture untouched, and `pictureCount()` returns to what it was after a reset. `test_holes.cpp`, `test_picture_scale.cpp`, `test_flippers.cpp` pass.
  - [ ] **4. Objects hold ids (M).** `std::shared_ptr<const Bitmap>` leaves `Object` and its parts (`Looks`, `Animation`, `Drawn`, `Mirror`) for `PictureId`; `Object::drawnPicture` asks the manager. **Check:** `grep -rn "shared_ptr<const Bitmap>" xge/lib/game xge/lib/play` finds nothing; all tests.
  - [ ] **5. Textures by picture (M, one backend a commit).** Null window first, with a count of textures asked for; then SFML, SDL2, raylib, OpenGL, Vulkan, Qt (`xge/gui`); the Direct3D windows are changed blind and wait for the author's build. The contract in `window/window.h` and `docs/reference/engine.md` "What a window must do" is rewritten first. **Check:** a case in `test_resources.cpp`: the null window is asked for 1 texture by the 20-member game; `test_backends.cpp` under `xvfb-run` for each backend built; a line in Waiting for a look on Windows for Direct3D.
  - [ ] **6. Sounds, fonts, sheets, schemas (M, one a commit).** Sound samples (`samplesOf` in `audio/sound.cpp`), font files (`picture/font.cpp`), sheets (`SheetPicture`), and the parsed schema (`XsdLiteValidator` in `load/xsd.*`). The schema outlives one game, so it is held by the `Engine`'s manager, which each game's manager asks. **Check:** cases in `test_resources.cpp`: two objects playing one sound share one set of samples; loading two games one after another reads `xgedef.xsd` once (a counter on the manager).
- **Files.** `xge/lib/engine/resources.*` (new, in `ENGINE_SOURCES` and `games/generators/engine-cpp/engine.xml`), `load/pictures.cpp`, `picture/*`, `game/object.h`, `window/*`, `audio/*`, `xge/gui/source/`; `docs/reference/engine.md`, `docs/agents/pictures.md`.
- **Done when** no file but `resources.cpp` makes a `Bitmap` that outlives a function, and `[size]` shows pictures and textures counted by what is different, not by object.

### CR3. Parts as components

- **Why.** An `Object` is 4,712 bytes whatever it is. Its parts (A4) become components: each kind in its own collection, held only by the objects that use it. The author: an object needs only its name; everything else, its place and picture included, is a part it has or has not.
- **The shape.**

  ```cpp
  struct ObjectId { std::uint32_t index{}; std::uint32_t generation{}; };

  template <typename Part> class Store      // xge/lib/game/store.h
  {
  public:
      bool has(ObjectId id) const;
      Part& get(ObjectId id);               // throws for one that has none
      Part* find(ObjectId id);              // nullptr for one that has none
      Part& add(ObjectId id, Part part);
      void remove(ObjectId id);
      template <typename Work> void each(Work&& work);   // in object order: work(ObjectId, Part&)
  };
  ```

  - Parts are packed in one vector, kept in object order, with an index per object slot (none: no part). `Game` holds one `Store` per kind of part.
  - A slot freed by a despawn is used again with its generation raised by one, so an `ObjectId` kept from before no longer matches and `Game::find(ObjectId)` answers nullptr.
  - A part that a reset returns to its start keeps its start value in the same store (`Store::start(id)`); which parts do is in step 1's table.
  - Copying a `Game` copies its stores.
- **Steps.**
  - [ ] **1. The author: the table of parts.** Before any code, list every part and loose field of `Object` and of `Moment` (`game/object.h`) with: its kind (a verb's own, shared, or a family's, by the rule in [engine-structure](designs/16-engine-structure.md#how-the-parts-are-grouped)), the verb family or tag it belongs to, its size, how many objects across the shipped games use it (count with a scratch test over every game), whether a reset restores it, and a proposed name. Mark each to keep, merge, split or rename. It goes in that design file. Stop for the author's changes.
  - [ ] **2. A handle with a generation (M).** `ObjectId`; `Game::find(ObjectId)`; each object knows its own id. Then, one use a commit: `foundObjects`, carriers and riders, a `<chase>`'s and `<hunt>`'s target, a holder's members, the picker's held piece, and any other `Object*` or name kept from one step to the next (find them: `grep -rn "Object\*" xge/lib/game/*.h`). **Check:** new `xge/tests/engine/test_store.cpp`: an id found after a spawn; nullptr after a despawn; a new spawn in that slot has the next generation and the old id still finds nothing. All tests. `docs/agents/play.md` loses "never hold a reference to a copy across frames".
  - [ ] **3. The store (S).** `Store<Part>` as above. **Check:** in `test_store.cpp`: `add`, `has`, `get`, `find`, `remove`; `each` visits in object order after adds and removes in any order; a part of a despawned object is gone; a copied store changes alone; `start` is restored by a reset.
  - [ ] **4. Parts move out, one a commit, largest first (L in all).** `Holder`, `Drive`, `Hand`, `Cells`, `Animation`, `MapPlace`, then the rest of step 1's table. For each part:
    1. it leaves `Object` and gets its store in `Game`;
    2. `load/build.cpp` adds it only for an object whose file has its tag, or that has a command of its verb family;
    3. a verb that can give it to an object while playing adds it on first use;
    4. every reader of `object.<part>` asks the store, with `find` where the object may have none;
    5. spawn copies it, despawn removes it, reset restores it.
    **Check after each commit:** all tests and `xgetest-generated`; a case in `test_store.cpp` that Pong's ball has no such part (for a part Pong does not use); `[size]` and `[speed]` numbers in the commit, neither worse.
  - [ ] **5. The author: what every drawn thing has (M once agreed).** `Moment` (now, start and before: 648 bytes), the picture and the collision rules are used by almost every object and read in thousands of places, tests included (`get("ball").position.x`). Show the author step 1's count of objects without them, the number of call sites, and two forms: they stay in the object, or they move and call sites read `game.place(id).position.x`. Do what the author picks.
  - [ ] **6. Passes over a store (M, one pass a commit).** Where a frame pass asks every object "do you have this" (animations, object timers, paths, flights, pulls, altitude), it walks that part's store with `each`. The order of passes in `playOneFrame` and `Game::updateObjects` does not change. **Check:** all tests; `[speed]` frame time for all games summed is lower than before the task; `xgetest-long` random keys.
  - [ ] **7. Features leave `Game` (L, a feature a commit).** With holders, cells, marks and the search holding their own parts, the functions in `game/holders.cpp`, `cells.cpp` and `think.cpp` become functions that take the game, as verbs are, and leave the class. **Check:** all tests; `wc -l xge/lib/game/game.h` under 500.
  - [ ] **8. The written side (M).** `RawObject` (14,600 bytes) holds its optional parts as `std::optional`, a part a commit. **Check:** `[size]`: `sizeof(RawObject)` under 2,000.
- **Files.** `xge/lib/game/store.h` (new), `game/object.h`, `game/game.*`, `game/*.cpp`, `play/*`, `verbs/*`, `load/build.cpp`; `xge/tests/engine/test_store.cpp` (new); `docs/reference/engine.md`, `docs/agents/play.md`.
- **Done when** `sizeof(Object)` is under 256 bytes if step 5 moved the place out, or under 1,200 if it stayed (held by a `static_assert` in the `[size]` test), and no object holds a part its file never asked for.

### CR4. Input, update, render

- **Why.** `Engine::step` polls the window, plays held keys, runs the computer, steps the game and asks for sounds, and `loop` then renders. The author wants the three stages spelled out so another (post-processing) can go between.
- **Steps.**
  - [ ] **1. `Engine::run` (M).** `run()` is the loop: `input()`, `update(seconds)`, `render()`, in those words. `input` is everything that comes from outside (window events, pads, the demo's keys, the computer's presses); `update` is the game's step and what follows from it (sounds asked for, things kept); `render` draws. `loop` and `step` go, with their callers in `xge/cli`, `xge/gui` and the tests' `played.h`. **Check:** all tests; `xgetest "[window]"` under `xvfb-run`.
  - [ ] **2. The game's own step the same way (S).** `Game::updateObjects` reads as its stages in order (timers, the frame, conditions, spawns settled), each a named call, with no work between the calls. **Check:** all tests; `[speed]`.
- **Files.** `xge/lib/engine/engine.*`, `xge/cli/source/`, `xge/gui/source/`, `xge/tests/helpers/played.h`; `docs/reference/engine.md`.

### CR5. The reader in one pass

- **Why.** `findChild` (`load/reading.cpp`) starts from the first child every time and makes a node object per sibling passed; `readObjectHere` calls it about 40 times. The group reader repeats the object reader tag by tag. And the reader re-checks the file's shape because a file that names no schema loads unchecked.
- **Steps.**
  - [ ] **1. Every game is checked (S).** A file that names no schema is checked against the engine's own `xgedef.xsd`. **Check:** a test loads a wrong file with no schema named and gets the schema's message; `test_schema_checker.cpp`.
  - [ ] **2. The reader trusts the shape (M).** Remove what the schema already refuses: unknown tags, missing required children, wrong enumeration words, numbers that are not numbers (the bare `std::stof` and `std::stoi` in `readWindow` and elsewhere become safe). Keep every check a schema cannot make. One group of readers a commit. **Check:** `test_schema_checker.cpp`'s sweep still refuses every damaged game; the count of `fail(` in `load/xml.cpp` goes in the commit.
  - [ ] **3. An object's simple parts as a table (M).** Rows of tag, where it lands and how it is read, used by the object, group and member readers alike, each walking its children once in order. `hasX` beside a value becomes `std::optional`. **Check:** `xgetest-generated`; `[speed]` load time falls.
  - [ ] **4. No search left (M).** The other readers (states, conditions, sprites, holders, maps) walk their children once. `findChild` stays only where one optional child is asked of a small element. Each `copyElement` in `load/xml.cpp` is shown to be needed or removed. **Check:** a count of `findChild(` before and after in the commit; `[speed]`.
- **Files.** `xge/lib/load/xml.cpp`, `load/reading.*`, `game/object.h`; `docs/agents/play.md` "Loading".

### CR6. One reader, not two

- [ ] **1. The author: yes or no.** `engine-cpp` would write the game's tree (after `from=` is worked out) as a C++ table, and the generated program would run `load/xml.cpp` over it through a small `XmlNode`. `data.xsl` (4,138 lines), `merge.xsl` (768), `modules/game_parts.h` (588) and the describer in `test_generated_games.cpp` (551) go; the generator leaves [changing the language](agents/readme.md#changing-the-language). It stays XSLT, with no XML library in the program. The cost: the program carries the reader and reads its table at start. If no, skip to CR7 and A1 step 4 stays parked.
- [ ] **2. The tree and its node (M).** A static tree type and an `XmlNode` over it (`xge/lib/xml/`), with a test that reads Pong from one.
- [ ] **3. The stylesheet (M).** `games/generators/engine-cpp/tree.xsl` writes it; `xgetest-generated` plays each generated game beside the engine's as it does now.
- [ ] **4. The second reader goes (S).** Delete the four files and their mentions in `engine.xml`, `assets.cmake`, the agent notes and [generator](designs/40-generator.md).

### CR7. A verb owns its data

- **Why.** `RawCommand` is 3,328 bytes of 61 shared fields. This is A1 step 4, and it replaces that step.
- **Needs** CR6, or the author's word to do it with `data.xsl` still there.
- **Steps.**
  - [ ] **1. The shape, on three verbs (M).** `RawCommand` becomes the verb, its line, and the verb's own data (a `std::variant` of small structs beside each verb's `Cmd...` struct, or one struct serving as both where nothing is worked out). `<bounce>`, `<hop>` and `<spawn>` first. **The author** reads those three files before the rest move.
  - [ ] **2. The rest, ten verbs a commit (L in all).** A field two verbs used for different things (`seconds`, `amount`, `distance`) becomes each verb's own, named for what it is there. **Check after each commit:** `xgetest "[verbs]"`, `xgetest-generated`.
  - [ ] **3. The bag goes (S).** `RawCommand` has no shared value fields; CR1's `forEachValue` asks the verb. Load errors name the command's own line. **Check:** `[size]`.
- **Files.** `xge/lib/game/command.h`, `xge/lib/verbs/*`, `load/reading.*`; `docs/designs/14-verbs.md`.

### CR8. No special rules

- **Why.** Places where one mechanic was given its own branch because it was quicker that day.
- **Steps.**
  - [ ] **1. The list (S).** Find them: `grep -rn 'verb == "' xge/lib` (18: `send`, `order`, `trigger`, `set`, `spawn`, `shift` and others), tests of a kind or class by name in `play/` and `game/`, flags set by one mechanic and read by the frame (`goThrough`, `placed`, `hasAim`), and early returns in the frame for one feature. For each: what it is for, and the proposal, either a property on `Verb` or on a part that anything could have, or removal. The list goes in [engine-structure](designs/16-engine-structure.md). **The author** reads it.
  - [ ] **2. One a commit (M in all),** as agreed. **Check:** the grep's count falls to what the list says stays.
  - [ ] **3. Things built twice (M).** Read `play/verbs.*`, `play/executor.*`, `play/geometry.*` and `load/reading.*` for helpers that do one job under two names, and layers that only pass a call on. Each is merged or removed. **Check:** the list of what merged is in the commit messages.
  - [ ] **4. Names as ids while playing (M).** Where a step compares or looks up text (objects, classes, variables, sounds), an id made at load is used. Only where `[speed]` or the code's length gains. **Check:** `[speed]`.

### CR9. Names and comments

- **Steps.**
  - [ ] **1. The author: the names table.** Propose the renames in one table in [engine-structure](designs/16-engine-structure.md#versions-of-one-value): `Raw...`/`raw...` (37 types, 32 fields) to `Written...`, and any type, part or function whose name no longer says what it is after CR2 to CR8. Stop for the author.
  - [ ] **2. The renames, a family a commit (M).** By tool, not by hand; the generator's files and the docs with them. **Check:** all tests; `xgetest-generated`.
  - [ ] **3. Comments, a folder a commit (M).** In `xge/lib`: a comment that repeats the code or stands in for a name goes, the name is fixed instead; one-line landmarks over sections and functions stay; a "why" that the code cannot say stays, short. Comments in `game/` are 19% of its lines today. **Check:** the share per folder before and after in the commit.
  - [ ] **4. The rule is written down (S).** [AGENTS.md](../AGENTS.md) "Rules" gains the names-over-comments line, and [engine](reference/engine.md) describes the engine as it now is: resources, components, input, update, render. This section is deleted from the plan.

## Tasks

Every task has the same shape: why, then numbered steps with a box each, then the files and what "done" means.

- **A step is one commit.** It says what to do and has a **Check** that proves it. Tick its box (`[x]`) in the same commit.
- **Every check also includes** `ctest` passing and zero warnings; a step that touches the reader, a `Raw*` struct or the generator also runs `xgetest-generated`. The check written on a step is what to run on top of that.
- **Sizes:** **S** under two hours, **M** half a day, **L** more than a day. An L step should be split further by whoever starts it, before starting.
- **A step marked "the author" stops for the author:** do not go past it without an answer.

### Architecture

#### A1. A verb is one file

- **Needs** nothing. A5 needs only what is already done.
- **Done so far.** Each verb is a file in `xge/lib/verbs/` with its name, forms and places, `onRelease`, `read`, `make`, `check` and `run`; `verbs/all.cpp` lists them; `xgetest "[verbs]"` holds the list against the schema. Why a written list: [verbs](designs/14-verbs.md).
- **Careful.** The folder is compiled as one unit: name a file's functions for its verb (`readBounce`), never in an unnamed namespace. A refused verb returns false and ends its list: keep each `run`'s answer exactly. The generated programs compile the same files.
- **Steps.**
  - [x] **1. A place for printing (S).** `Verb` (`verbs/verb.h`) gains an optional `print` for a `Command` and one for a `RawCommand`. `operator<<` for each (`game/command.cpp`) asks the verb first and falls back to its own cases. Nothing moves yet. **Check:** `xgetest-generated` (it compares these dumps, so the text must not change by a character).
  - [x] **2. Move the printing, ten verbs a commit (M in all).** Each verb's two cases move into its file. Repeat until the cases in `command.cpp` are gone, then remove the fallback. **Check after each commit:** `xgetest-generated`.
  - [x] **3. A verb's own helpers move beside it (S).** `hop`, `startFlight`, `huntByHop`, `spawnProjectile`, `aim`, `chase`, `land`, `knock` in `play/executor.cpp`: for each, `grep` for its callers; one used by a single verb moves into that verb's file, one used by several stays. One helper a commit. **Check:** `xgetest-long "[speed]"` before and after (load and frame times must not rise).
  - [ ] **4. Moved to [CR7](#cr7-a-verb-owns-its-data).** Was: the author: whether each verb owns its data (L if yes). `Command` would become `{ const Verb* verb; data }` and `RawCommand`'s shared fields the verb's own, which is where "`seconds` means a move's wait" ends. It changes `data.xsl` and the describer heavily, so it waits until A5 has retired `data.xsl`, or is dropped. Ask before starting. With it, each command carries its own line, and the load's name checks can say the command's line rather than its holder's (B7, done; the author wants that).
- **Files.** `xge/lib/verbs/*`, `xge/lib/game/command.cpp`, `xge/lib/play/executor.*`; `docs/designs/14-verbs.md` ("Open" shrinks), `docs/agents/play.md` "Loading".
- **Done when** adding a verb is one file in `xge/lib/verbs/`, one line in `all.cpp` and `engine.xml`, its `Cmd*` struct, the schema element and the docs.

#### A5. A game written out as if by hand

- **On hold** (see [order of work](#order-of-work)).
- **Needs** nothing. Open-ended; steps 1 to 3 are a few days.
- **Why.** The author's aim for the generator: a stand-alone version of a game that looks hand-coded over a weekend, as the route to any platform ([generator](designs/40-generator.md)).
  - `engine-cpp` (renamed from `windows-cpp`) writes the engine with the game's data built in: complete and exact, and unreadable as a game. The new target takes the name `windows-cpp`: a generic, single-purpose looking Windows program, on SFML only for now.
  - A hand-written Pong on the shared frame once matched the engine frame for frame over ten seeds of 3,600 frames, and was set aside (`git show 7163ea2^:games/prototypes/pong_min/source/pong_min.cpp`).
- **What it writes** (the author, after reading the first hand-written `pong.xml`): a stand-alone program with no mention of the engine and as few abstractions as the game needs. A couple of SFML shapes, plain collision code, sounds made from a few lines of square wave; no verbs, no frame order, no `Object`. The verbs are for matching the file to the engine; a stand-alone game is plain functions, and a class where one makes sense.
  - It does the same things the file says, not the engine's exact sums, so it is checked by what it does (serves, bounces, returns, points, screens, the computer), not frame for frame.
  - `games/prototypes/pong_min/` is what the generator writes today: a `Thing` (where, how fast, how big) per object, a start function where one starts again, and one `frame()`: keys held, moves, edges, meetings.
  - `games/prototypes/pong/` is the next example: about 300 lines of game (`pong.h`, `pong.cpp`) and 190 of window (`main.cpp`), linked to SFML only.
- **Steps.**
  - [x] **1. Bring the prototype back (M).** Restore `pong_min.cpp` from the commit above into `games/prototypes/pong_min/` and port it to today's engine. This file is the specification by example (since rewritten stand-alone).
  - [x] **2. A new target folder (M).** `games/generators/windows-cpp/`: `generate.xsl`, `game.xsl`, `program.xsl` (main, CMake), `names.xsl`, `values.xsl` and `verbs/` with one file per verb it can write, a template per mode (`<xsl:template match="bounce" mode="edge">`); since the author's ruling it copies none of the engine. **Check:** `xgecli pong_min --generate windows-cpp` writes the hand-written files but for their date line (`test_generate.cpp` holds it), and it builds with no warnings on GCC and Clang.
  - [x] **3. Say what it cannot write (S).** Before running, the generator walks the game (`covers.xsl`, and a template in each verb's file saying where it is covered) and lists every verb or part with no words in this target, once each with the object it is in; if any, it stops and prints them with "use `engine-cpp` for this game". Nothing is silently approximated. **Check:** `test_generate.cpp` holds the games `windows-cpp` covers (`pong_min`), and every other game fails with its list.
  - [x] **4. The checks (M).** Each covered game has its behaviour checks in `xge/tests/written/<game>.h`, a function template over the game's class: `xgetest` runs them on the hand-written prototype, `xgetest-generated` on the generated copy (`test_written_games.cpp`; the covered games and their classes are listed in `tests.cmake`). First built as an every-frame match with the engine; changed with the author's ruling. **Check:** they pass for `pong_min`.
  - [ ] **5. `pong.xml` (M).** States, conditions, text, sounds, a computer player. First by hand: `games/prototypes/pong/`, stand-alone on SFML (done; rewritten without the engine after the author read the first one, which called the engine's frame and verbs). **Check:** the behaviour checks of step 4 for it. **Then the author** reads the output: its look is the point, and only the author can judge it.
  - [ ] **6. Breakout (M).** Groups in columns and rows, looks. **Check:** as step 5. **Then the author** reads the output again.
  - [ ] **7. One game a step after that (M each),** simplest first: Space Race, Kaboom, Freeway, Frogger (`ride`, lanes), Space Invaders (lockstep, timers, spawn). Each adds a few templates and a line in the covered list. **Check:** step 4's test for that game.
- **Not doing.** Cards, the board-game search and cell puzzles in this target: they are large engines in themselves, and a "weekend" version would be those files copied. They stay on `engine-cpp` until the author asks otherwise. Another platform is a later task: [targets](designs/57-ideas.md).
- **Files.** `games/prototypes/pong_min/`, `games/generators/windows-cpp/`, `xge/cli/source/generate.cpp` (targets are folders already; add the coverage check), `xge/tests/generated/test_generated_games.cpp`, `scripts/cmake/assets.cmake` (`data_generators`), `scripts/shell/windows/games-*.bat` (a target argument), `docs/reference/programs.md`, `docs/designs/40-generator.md`.
- **Done, first milestone:** `xgecli pong --generate windows-cpp` writes a program under 300 lines of its own code that builds with no warnings at `/W4` and GCC's strict set, and does what the file says, by its behaviour checks.

#### A7. The next backend is cheap

- **Needs** nothing. Steps 1 to 4 are the groundwork; step 5 onward is a backend at a time.
- **Why.** The author wants more backends, lower-level ones included ([backends](designs/50-backends.md)).
  - Today each window backend is 500 to 650 lines, and most of it is the two jobs the engine does not do itself: drawing text with the library's own font code, and loading image files with the library's own decoder.
  - That is also why text widths differ between backends and from the built-in font (a listed limit).
- **Steps.**
  - [x] **1. Write the contract down (S).** In `docs/reference/engine.md` "What a window must do" and at the top of `window/window.h`.
  - [x] **2. Text drawn by the engine (M).** `drawText` in `picture/font.cpp`, with plutovg (stb_truetype reads the font, a rasterizer from FreeType's fills it); the null window and SFML use it. **Check:** `test_drawn_text.cpp` measures texts with no window.
  - [x] **3. Each backend loses its font code.** SDL2, raylib, OpenGL and `xgegui`'s Qt window show `drawText`'s bitmap; SDL2_ttf is gone.
  - [x] **4. Image files decoded by the engine.** `loadPicture` in `picture/image.cpp`, read once and shared; every backend shows its bitmap. SDL2_image is gone, and the OpenGL window no longer needs SDL at all.
  - [x] **4b. Every size known at load (M; split before starting).** With text and image files made by the engine, `measureShapeSize` knows every picture, so the engine makes each object's picture when the game loads, not when a window opens. What goes: `sizeKnown`, `sizeNeedsBackend`, the "unknown" sizes in `printGame`, positions waiting on a window (`positionUsesSize`, `resolveSizeDependentPositions`), the per-frame `prepare` in `Engine::step`, and each backend's own measuring. A window then only turns finished bitmaps into textures. **Check:** a test loads Pong with no window and finds its title's true width and the position made from it.
  - [x] **4c. Pictures redrawn sharp at the window's size (M).** The author's idea: a window reports its real pixel size; while it is being dragged the pictures stretch, and once it has held still about a quarter second the engine redraws every picture made from a description (text, SVG, circles and rectangles) at that size and swaps them in. Positions, sizes and hitboxes stay in game pixels, so play never notices. Bitmap rows and image files keep their hard-edged scaling. Games scale freely, so a pixel font at an odd scale has slightly uneven pixels but sharp edges. **Check:** a test asks for the drawing at twice the size and gets twice the pixels with the same game size; **then the author** drags a window on Windows. Built for text, circles and rectangles; an SVG stays a bitmap made at load (open in [backends](designs/50-backends.md#scale)).
  - [x] **5. One conformance test for every window backend (M).** `xge/tests/engine/test_backends.cpp`: for each backend built, open a small window (under Xvfb on Linux), show three known bitmaps, read the pixels back where the library allows, feed a key, close. **Check:** it passes for every backend built.
  - [x] **6. Direct3D 11 (M).** The simplest modern one: a window from Win32, a textured quad per object. `xge/lib/window/direct3d11.cpp` behind `XGE_WITH_DIRECT3D11`, a row in `scripts/cmake/dependencies.cmake` and the factory. Built (`window/direct3d11.cpp`, `-w direct3d11`, a choice in `xgegui`); the author's build and window test passed.
  - [x] **7. Then, each its own step like 6:** Direct3D 9 (older machines), Vulkan (the most code for the same quad), Direct3D 12 last. Direct3D 9 is written (`window/direct3d9.cpp`, `-w direct3d9`, a choice in `xgegui`), on a Win32 window now shared with Direct3D 11 (`window/win32.cpp`); the author's build and window test passed. Vulkan is written (`window/vulkan.cpp` on GLFW, `-w vulkan`, a choice in `xgegui`; its GLFW keys and pads shared with OpenGL in `window/glfw.cpp`), and its window test passes on Linux under the validation layer, and the author's Windows build and test passed. **It cannot be built in a Linux container:** it is written blind there and compiled, fixed and tested by the author on Windows, so keep it to one small file and expect a round or two. **Check:** step 5's test, run by the author. Direct3D 12 is written blind (`window/direct3d12.cpp` on the shared Win32 window, `-w direct3d12`, a choice in `xgegui`; a Debug build turns on the debug layer when Windows has it); the author's build and window test passed. Direct3D 10 is written blind (`window/direct3d10.cpp` on the shared Win32 window, `-w direct3d10`, a choice in `xgegui`), Direct3D 11's window on the older API; the author's build and window test passed.
- **Also gains.** The picture of a frame (F1, done) came cheap once text was a bitmap. plutovg can also test whether a point is inside a shape, if a game ever wants exact shape touches.
- **Done, first milestone:** no window backend's file holds font or image-file code, the conformance test runs over each, and the first new backend is under 300 lines.

### Additions

Ordered by how many existing games gain. Each ends with the docs, a game or two moved to the new word, and the limitation removed from `docs/reference/limits.md`.

#### F8. Dice

- **Needs** the hold on new games lifted ([open decisions](#open-decisions)).
- **Why.** Agreed as the next board or card step. The question is whether it needs any new word at all.
- **Steps.**
  - [ ] **1. Try a die with today's words (S).** A test game, no engine change: an object with six looks, a variable set from a `<random>` by a command, and a `<become>` to the look for that number. **Check:** it loads and shows each face over many rolls. If it cannot be said, write down exactly which part, and stop for the author.
  - [ ] **2. Patterns off dice (S).** A holder of five dice, scored by `<patterns>` with `<alike>` and `<run>`, as cards are. **Check:** a test with fixed rolls gets the right pattern for a pair, three alike and a run.
  - [ ] **3. The game (M).** An original dice-and-patterns game on the key convention: five dice, keep some, roll three times, score. It is added as [AGENTS.md](../AGENTS.md#adding-a-file) says a game is. **Check:** its own test plays a whole game.
  - [ ] **4. The author: backgammon.** Write down the words it would need (bar, bearing off, doubling) in `docs/designs/26-board-games.md`, and stop. It is built only if the author still wants it once that list is in front of them.
- **Done when** the first three boxes are ticked.

#### F9. Sound files

- **Built:** a `<sound>` can be a `<path>` to an uncompressed WAV, decoded by the engine (`audio/wave.cpp`) into the same samples as notes ([sounds](reference/sounds.md#recordings), [sound](designs/33-sound.md)). What is left is the author's.
- **Steps.**
  - [ ] **5. The author: a recording and a game.** `path` is already a word of the schema (an image's), so the vocabulary test forces no game. The author picks a recording and a game to use it in; the file's licence goes beside it in `games/assets/`, as the font's does. Then a line in [Waiting for a look on Windows](#waiting-for-a-look-on-windows): the author listens on each sound library.
  - [ ] **6. The author: compressed files (M once picked).** MP3 and Ogg each need a decoder in the engine: one small library, fetched like the others, behind one `readSoundFile` that picks by the file's ending and returns the same samples. Which library, and its licence, is the author's call: ask before fetching one.

## Long term

Goals the author has set, beyond the tasks above. None is scheduled; [delta time](#delta-time) and [the C++ review](#the-c-review) are being done first and have their own sections.

### A camera and scrolling

- **Version 2, on hold by the author's choice** ([ideas](designs/57-ideas.md)).
- Positions, screen edges and collisions all assume the window is the world, so the design is worth writing before much more code leans on that.

### A version on the language, and a definition of 1.0

- **Held by the author for now:** the language still changes and breaks files too often for a number to mean much. The two go together when the time comes.
- A game file says which version of the language it was written for. Suggested: an attribute on `<game>`, since it picks something (`<game version="1">`), a whole number that goes up with each change that breaks older files.
- Each such change comes with a stylesheet that upgrades older files, which is also how the shipped games would be moved. Add the number and the first upgrade stylesheet together; the number alone is only a label.
- 1.0 says which genres, which vocabulary is frozen and which targets. The plan has no finish line without it.

## Backlog

Not ordered and not decided. Each item points at the design file that holds the reasoning. An item becomes a task above when it is chosen.

### Next classes of game

The review that started this plan argued with this list; its view is beside each.

| Listed | The review's view |
|---|---|
| **Dice**, then backgammon, agreed as the next board or card step ([cards](designs/27-cards.md)) | A dice-and-patterns game reuses `<patterns>` and `<random>` and costs little ([F8](#f8-dice)). Backgammon is a large rule set that will want board-specific words: leave it |
| **Moves as timelines** (fighting games: key sequences, blocking that a fist meets first, hit-stun, rounds), the next class after tables and tracks ([tables-and-tracks](designs/25-tables-and-tracks.md)) | Animations that run once and a hitbox per frame are built (F2, done; `sparring` proves them). Stop before combos |
| **Lookahead for action games' computers** ([board-games](designs/26-board-games.md#open)) | A research project; arcade enemies are better on their simple rules |
| **A deck of single cards**, each its own picture and numbers ([cards](designs/27-cards.md)) | Wait until a game cannot be written without it |
| **Typed text, a word as data** (Hangman, initials beyond `<letter>`) | Small genre, large addition |

Games the vocabulary cannot say yet: [near misses](designs/54-games.md#near-misses).

### The project

- **The reference's word list generated from the schema** (`docs/reference/words.md`), now that each element documents itself (A6, done). Held by the author: the word index in `docs/readme.md` stays hand made, with its groups, for now.
- **A build that runs in a browser,** so a game can be played from a link without compiling anything. Not hard, but better done once the language has settled.

### Language and engine

- **Steering** ([chase-and-paths](designs/24-chase-and-paths.md)): leading a target, chasing round walls, a real maze search as another verb, curved path legs.
- **Platforms** ([platforming](designs/23-platforming.md)): Donkey Kong's barrels choosing ladders by where Jumpman is, the other three screens, air control.
- **Cells** ([groups](designs/06-groups.md#cells)): gliding between cells; Tetris hold, lock delay, ghost piece, row scoring; Columns chain scoring; Centipede's flea, spider and scorpion.
- **Cell puzzles** ([cell-puzzles](designs/28-cell-puzzles.md#open)): undo of despawned copies, a weighted spawn, a safe first square, conditions in a condition in an action.
- **Pictures** ([pictures](designs/30-pictures.md), [animation-and-looks](designs/31-animation-and-looks.md)): decals, a hole the shape of a bitmap.
- **Sound** ([sound](designs/33-sound.md)): the rest of an envelope (attack, sustain, release) beside `<decay>`.
- **Stereo** (the author's wish): every sound is one channel today, and a stereo recording is averaged into it. Some later games, music above all, will want two. A mono sound then plays the same on both sides, so every game so far sounds as it does now.
- **Values** ([arithmetic](designs/03-arithmetic.md)): tangent, square root, power, `pick`, when a game needs one.
- **Conditions** ([states-and-conditions](designs/07-states-and-conditions.md)): object-level conditions, a total inside a value, conditions on edge rules.
- **Scenes** ([scenes](designs/12-scenes.md)): Berserk's larger grid of rooms, which needs maps made while playing.
- **Demos after delta time** ([time](designs/29-time.md#open)): a recording replays as the same game only on the same steps. Choose between keeping every step's length in the demo and a constant step for recording and playback alone, then record the demos again.
- **Reserved names:** an object, group, member, map or state may not be called `self`, `other`, `others`, `window` or `color`, nor anything a number, because dotted names mix the game's own names with the engine's words (`self.x`, `other.score`, `window.width`, `color.red`).
  - Variables and sounds are free (only ever after a dot, or named by `sound=`), and `top` is free (`top.rank` is read before any name).
  - Look for a way to say those things that needs no reserved words, so any name is free (an object may already be called `object`).

### The games

- Galaxian: escorts' points. Asteroids: the saucers' shots breaking rocks. Lunar Lander: hard landings.
- A Lode Runner kind of chase is now possible: a `<hunt>` with `<search />` finds the way round.
- Pac-Man: the later levels' schedules, the other fruit. Adventure: the bat, Rhindle, the rest of the kingdom.
- Putt Park: a cup that takes only a slow ball wholly over it. Cannon Hill: cannons falling into craters.
- Speeds in Galaxian, Berserk, Tennis, Haunted House and Pac-Man were chosen without watching play: tune them first.

### Code

- `data.xsl` reads the file a second time; a new field in `load/xml.cpp` needs the same there ([generator](designs/40-generator.md#open)).
- `Engine::handleKeyPressed`/`handleKeyReleased` are public only because the tests and `played.h` press keys through them; a test seam would let them go private.
- Objects not shown (`isShown`) are left out of the swept collision pass in `play/frame.cpp`, so nothing hidden can touch or be touched; a game with invisible things that collide would need them in it.
- Astrosmash's test "a crude player who shoots the lowest rock in the sights scores" fails for about 1 Catch2 seed in 250 (`--rng-seed 179`, `235`, `349`): no score in twenty seconds. The test's player or the game's rocks, not the engine; it fails the same before and after A4.
- Building generated programs is by hand: `scripts/shell/windows/games-run-all.bat` generates, builds at MSVC `/W4` and tests them all.

## Waiting for a look on Windows

What only eyes, ears or MSVC can check. Whoever lands a change that needs one adds its line; the author deletes it once seen.

- **The schema's documentation in an editor (A6), not urgent (the author works mostly from the command line):** hovering `<deflect>` (or any tag) in VS Code with the XML extension shows its sentence, and completion lists show them too.

## Known untested

- Sanitizers: the whole of `xgetest` is clean under `-DXGE_SANITIZE=address` (address and undefined behaviour), as are `xgetest-long`'s random keys and damaged files; the board-game search on its own thread is clean under `-DXGE_SANITIZE=thread`.
  - The damaged files found one fault: a line out at 1e9 or a text of a huge size asked for hundreds of gigabytes.
  - Pictures and texts are now at most 4096 pixels a side (`refusePictureSize`).
  - Generated programs and `xgegui` have not been run under one.
- MSVC is checked by hand, not automatically; older versions are untested.
- A default build (no options) and one with only `-DXGE_WITH_OPENGL=ON` have no automatic check ([build-and-layout](designs/51-build-and-layout.md)).
- Agents build in a Linux sandbox and cannot watch or listen, so how a game feels and sounds is unchecked.
