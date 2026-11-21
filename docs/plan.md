# Plan

What is next, and nothing else. The ordered tasks come first, each written to be picked up cold: what to do, why, which files, what "done" is, and how to check it. Under them is the backlog, which is not ordered and not decided.

- **Working a task:** do its steps in order, one commit each, and tick the step's box in that commit. When every box is ticked, delete the task from this file and from [the order](#order-of-work). Anything it leaves open goes in [the backlog](#backlog).
- **Goals with no date** are under [long term](#long-term).
- **Why something was decided** is in [the design files](designs/00-designs.md), not here. This file started as a review of the whole project; its findings became these tasks, and its settled arguments moved to the design file each belongs to.
- **When the plan and the code disagree, the code wins.** Check that a file or function still exists before changing it, and correct this file.

## Order of work

One task at a time, top to bottom. A task's steps are done in their own order, and the tests are green after every step. Where a line says "steps 1 to 4", the rest of that task comes back further down.

| # | Task | Why here |
|---|---|---|
| 1 | [F8](#f8-dice): dice | Waits on the hold on new games |
| 2 | [A7](#a7-the-next-backend-is-cheap): more backends after step 7 | Never finishes: a backend at a time |

**On hold:** [A5](#a5-a-game-written-out-as-if-by-hand), the stand-alone generator (the author: nice-looking generated code is a hard problem that may need several things to change at once). It stands at `pong_min` written, and `pong.xml` written by hand as the next example.

What must come before what:

| Task | Needs first |
|---|---|
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

## Tasks

Every task has the same shape: why, then numbered steps with a box each, then the files and what "done" means.

- **A step is one commit.** It says what to do and has a **Check** that proves it. Tick its box (`[x]`) in the same commit.
- **Every check also includes** `ctest` passing and zero warnings; a step that touches the reader, a `Raw*` struct or the generator also runs `xgetest-generated`. The check written on a step is what to run on top of that.
- **Sizes:** **S** under two hours, **M** half a day, **L** more than a day. An L step should be split further by whoever starts it, before starting.
- **A step marked "the author" stops for the author:** do not go past it without an answer.

### Architecture

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

Goals the author has set, beyond the tasks above. None is scheduled.

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
- Pinball at 10 frames a second (the author, after DT4): the ball never fully settles, and once a flipper went on turning round and round on the game over screen, as if it started turning as the ball went out and never stopped.

### Code

- **xgegui's Qt window on Windows** (the author, after DT5): slow and uneven on some games, docked more than popped out, while SFML2 and SDL2 play them smoothly. On Linux every game draws a frame in under 1.5 ms. Left for now; a readout of step, draw and paint times would show where it goes.

- `Engine::handleKeyPressed`/`handleKeyReleased` are public only because the tests and `played.h` press keys through them; a test seam would let them go private.
- Objects not shown (`isShown`) are left out of the swept collision pass in `play/frame.cpp`, so nothing hidden can touch or be touched; a game with invisible things that collide would need them in it.
- Astrosmash's test "a crude player who shoots the lowest rock in the sights scores" fails for about 1 Catch2 seed in 250 (`--rng-seed 179`, `235`, `349`): no score in twenty seconds. The test's player or the game's rocks, not the engine; it fails the same before and after A4.
- Kaboom's test "each wave's bomber is quicker and drops faster bombs more often" fails for about 3 Catch2 seeds in 200 (`--rng-seed 9`, `48`, `106`): 4 bombs falling at 85 frames, not 5, one already caught or landed. Seen after DT4; not looked into.
- Building generated programs is by hand: `scripts/shell/windows/games-run-all.bat` generates, builds at MSVC `/W4` and tests them all.

## Waiting for a look on Windows

What only eyes, ears or MSVC can check. Whoever lands a change that needs one adds its line; the author deletes it once seen.

- **The schema's documentation in an editor (A6), not urgent (the author works mostly from the command line):** hovering `<deflect>` (or any tag) in VS Code with the XML extension shows its sentence, and completion lists show them too.

## Known untested

- Sanitizers: the whole of `xgetest` is clean under `-DXGE_SANITIZE=address` (address and undefined behaviour), as are its hidden long tests' random keys and damaged files; the board-game search on its own thread is clean under `-DXGE_SANITIZE=thread`.
  - The damaged files found one fault: a line out at 1e9 or a text of a huge size asked for hundreds of gigabytes.
  - Pictures and texts are now at most 4096 pixels a side (`refusePictureSize`).
  - Generated programs and `xgegui` have not been run under one.
- MSVC is checked by hand, not automatically; older versions are untested.
- A default build (no options) and one with only `-DXGE_WITH_OPENGL=ON` have no automatic check ([build-and-layout](designs/51-build-and-layout.md)).
- Agents build in a Linux sandbox and cannot watch or listen, so how a game feels and sounds is unchecked.
