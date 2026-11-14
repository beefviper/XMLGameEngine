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
| 1 | [F2](#f2-animations-that-run-once): animations that run once | The addition the most existing games gain from |
| 2 | [A7](#a7-the-next-backend-is-cheap) step 7: Direct3D 12 (Vulkan awaiting the author's test) | A backend at a time, each passing the window test |
| 3 | [F1](#f1-a-game-you-can-step-and-read-from-the-command-line) steps 5 and 6: a picture of a frame | Cheap now that text is a bitmap |
| 4 | [A2](#a2-holders-in-the-engine-not-piles): holders in the engine | Tidying; nothing a player sees depends on it |
| 5 | [A3](#a3-split-the-long-functions), then [A4](#a4-object-in-parts): long functions, `Object` in parts | Tidying. If the [review of the C++](#a-review-of-the-c) starts first, it replaces these |
| 6 | [F7](#f7-sound-an-envelope-and-notes-that-follow-a-number) steps 1 to 3, [F4](#f4-a-list-of-numbers) steps 1 to 3 | Each ends at a design for the author to decide |
| 7 | [F8](#f8-dice): dice | Waits on the hold on new games |
| 8 | [A7](#a7-the-next-backend-is-cheap) step 6 onward | Never finishes: a backend at a time |

**On hold:** [A5](#a5-a-game-written-out-as-if-by-hand), the stand-alone generator (the author: nice-looking generated code is a hard problem that may need several things to change at once). It stands at `pong_min` written, and `pong.xml` written by hand as the next example.

What must come before what:

| Task | Needs first |
|---|---|
| A4 | A2 |
| A1 step 4 | A5 far enough along to retire `data.xsl`, and the author's yes |
| F1 step 5 | easier after A7 step 4b, not blocked by it |
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

#### A1. A verb is one file

- **Needs** nothing. A5 needs only what is already done.
- **Done so far.** Each verb is a file in `xge/lib/verbs/` with its name, forms and places, `onRelease`, `read`, `make`, `check` and `run`; `verbs/all.cpp` lists them; `xgetest "[verbs]"` holds the list against the schema. Why a written list: [verbs](designs/14-verbs.md).
- **Careful.** The folder is compiled as one unit: name a file's functions for its verb (`readBounce`), never in an unnamed namespace. A refused verb returns false and ends its list: keep each `run`'s answer exactly. The generated programs compile the same files.
- **Steps.**
  - [x] **1. A place for printing (S).** `Verb` (`verbs/verb.h`) gains an optional `print` for a `Command` and one for a `RawCommand`. `operator<<` for each (`game/command.cpp`) asks the verb first and falls back to its own cases. Nothing moves yet. **Check:** `xgetest-generated` (it compares these dumps, so the text must not change by a character).
  - [x] **2. Move the printing, ten verbs a commit (M in all).** Each verb's two cases move into its file. Repeat until the cases in `command.cpp` are gone, then remove the fallback. **Check after each commit:** `xgetest-generated`.
  - [x] **3. A verb's own helpers move beside it (S).** `hop`, `startFlight`, `huntByHop`, `spawnProjectile`, `aim`, `chase`, `land`, `knock` in `play/executor.cpp`: for each, `grep` for its callers; one used by a single verb moves into that verb's file, one used by several stays. One helper a commit. **Check:** `xgetest-long "[speed]"` before and after (load and frame times must not rise).
  - [ ] **4. The author: whether each verb owns its data (L if yes).** `Command` would become `{ const Verb* verb; data }` and `RawCommand`'s shared fields the verb's own, which is where "`seconds` means a move's wait" ends. It changes `data.xsl` and the describer heavily, so it waits until A5 has retired `data.xsl`, or is dropped. Ask before starting. With it, each command carries its own line, and the load's name checks can say the command's line rather than its holder's (B7, done; the author wants that).
- **Files.** `xge/lib/verbs/*`, `xge/lib/game/command.cpp`, `xge/lib/play/executor.*`; `docs/designs/14-verbs.md` ("Open" shrinks), `docs/agents/play.md` "Loading".
- **Done when** adding a verb is one file in `xge/lib/verbs/`, one line in `all.cpp` and `engine.xml`, its `Cmd*` struct, the schema element and the docs.

#### A2. Holders in the engine, not piles

- **Needs** nothing.
- **Why.** The engine underneath the holder words is still the old card engine (`Object::Pile`, `RawPile`, `isCard`, `suit`, `rank`; `game/cards.cpp` is over 2,000 lines), so the language says "no card nouns" and the code says the opposite. `settleHolders` translates `<holds>` into the old pile fields.
- **Check for every step:** `ctest "[cards]"` and the eight card games' tests pass unchanged.
- **Steps.**
  - [ ] **1. Dead fields out (S).** In `RawObject::RawPile` and `Object::Pile` (`xge/lib/game/object.h`), take each field in turn (`dealt`, `dealtMore`, `dealsInto`, `deals`, `refillFrom`, `refillShuffle`, `refillLeave`, `faces`, `sorted`, ...) and `grep` for a writer in `load/xml.cpp` and `game/cards.cpp`. If only `settleHolders` writes it from a holder word, keep it for step 3. If nothing writes it, delete it and the code that reads it. **Check:** the list of fields deleted is in the commit message.
  - [ ] **2. Rename (M).** `Pile` to `Holder`, `RawPile` to `RawHolder`, `isCard` to `held`, `game/cards.cpp` and `cards.h` to `game/holders.cpp` and `holders.h`, `thinkCards` to `thinkHeld`. Mechanical, one commit. Also `engine.xml`, `ENGINE_SOURCES` in the top `CMakeLists.txt`, the field names in `data.xsl` and the describer, and the three comments that name `game/cards.h` (in `load/xml.cpp`, `game/states.h`, `game/object.h`). **Check:** `grep -rn "Pile\|isCard\|cards\.h" xge/lib` finds nothing.
  - [ ] **3. Remove the translation, one holder word a commit (L in all).** In turn: what it holds, `spread`, `order`, `counts`, `accepts`. For each, the holder's field becomes the word itself as the game wrote it, the engine reads that, and the matching lines of `settleHolders` go. **Check:** after the last, `settleHolders` is gone.
  - [ ] **4. `suit` and `rank` (M).** Wherever the engine reads a held member's `suit` or `rank`, it reads the member's row and column in its group, under the names a pack gives them (`<columns name="rank">`, `<rows name="suit">`). **Check:** `grep -rniE "pile|isCard|suit|rank|deal|shuffle" xge/lib` finds nothing but comments explaining a card game example.
- **Done when** all four boxes are ticked.

#### A3. Split the long functions

- **Needs** nothing. Moves only, no behaviour change.
- **Why.** Each new feature adds a branch to the middle of one of these.
- **Check for every step:** the function is under 150 lines, and `xgetest-long "[speed]"` is no slower.
- **Steps, in any order.**
  - [ ] **1. `game_expr::makeObject` (M)** (`xge/lib/load/expr.cpp`) into a new `load/build.cpp` as named steps: `buildNumbers`, `buildLooks`, `buildRules`, `buildActions`, `buildTimers`, `buildHolder`, `buildMoves`, each taking the `RawObject` and the `Object` being made. Add the file to `ENGINE_SOURCES` and `engine.xml`.
  - [ ] **2. `game_expr::init` (M)** into steps named for what [Loading](reference/engine.md#loading) lists: names, variables, sounds, objects, states, checks.
  - [ ] **3. `readGroup` (M)** (`load/xml.cpp`) into "what the group gives", "members", "columns and rows", "row, column and cell changes".
  - [ ] **4. `checkCommand` (S)** (`load/check.cpp`): whatever is still one long function after the verbs took their own checks becomes one small function per kind.
  - [ ] **5. `checkReferences` (M)** (`load/check.cpp`): one function per thing checked (states, objects, sounds, shows).
- **Done when** no function in `xge/lib` is over 150 lines and `expr.cpp` is under 1,200 lines.

#### A4. `Object` in parts

- **Needs** A2. Lowest priority here.
- **Why.** About 200 fields in one struct. Leave `Moment` (position, velocity and the rest that tests name) exactly as it is.
- **Check for every step:** `xgetest-long "[speed]"` is no slower.
- **Steps, one group a commit.** Each moves the fields of one feature into a member that most objects never use and tests rarely name.
  - [ ] **1.** `object.maze` (a walker's state).
  - [ ] **2.** `object.cell` (cells and chains).
  - [ ] **3.** `object.holder`.
  - [ ] **4.** `object.moves` and `object.marks` (boards).
  - [ ] **5.** `object.swing`.
  - [ ] **6.** `object.path`.
  - [ ] **7.** `object.jump`.
- **Later, not planned:** a group no object uses could become `std::optional` or a pointer, which also shrinks every object.
- **Done when** `struct Object` itself declares under 60 fields.

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
  - [x] **6. Direct3D 11 (M).** The simplest modern one: a window from Win32, a textured quad per object. `xge/lib/window/direct3d11.cpp` behind `XGE_WITH_DIRECT3D11`, a row in `scripts/cmake/dependencies.cmake` and the factory. Built (`window/direct3d11.cpp`, `-w direct3d11`, a choice in `xgegui`); the author's build and window test passed. Vulkan is written (`window/vulkan.cpp` on GLFW, `-w vulkan`, a choice in `xgegui`; its GLFW keys and pads shared with OpenGL in `window/glfw.cpp`), and its window test passes on Linux under the validation layer; awaiting the author's Windows build and test. **It cannot be built in a Linux container:** it is written blind there and compiled, fixed and tested by the author on Windows, so keep it to one small file and expect a round or two. **Check:** step 5's test, run by the author.
  - [ ] **7. Then, each its own step like 6:** Direct3D 9 (older machines), Vulkan (the most code for the same quad), Direct3D 12 last. Direct3D 9 is written (`window/direct3d9.cpp`, `-w direct3d9`, a choice in `xgegui`), on a Win32 window now shared with Direct3D 11 (`window/win32.cpp`); the author's build and window test passed.
- **Also gains.** [F1](#f1-a-game-you-can-step-and-read-from-the-command-line)'s picture of a frame becomes trivial now that text is a bitmap. plutovg can also write PNGs (F1, step 5's test) and test whether a point is inside a shape, if a game ever wants exact shape touches.
- **Done, first milestone:** no window backend's file holds font or image-file code, the conformance test runs over each, and the first new backend is under 300 lines.

### Additions

Ordered by how many existing games gain. Each ends with the docs, a game or two moved to the new word, and the limitation removed from `docs/reference/limits.md`.

#### F1. A game you can step and read from the command line

- **Needs** nothing.
- **Why.** Agents build this project and cannot see it. The engine already plays with no window (`-w none`), checks a game without playing it (`--check`), draws every picture itself into pixels, and can copy a whole board to search it. What is missing is the front door.
- **Steps.**
  - [x] **1. `--frames N` and `--seed S` (S).** `xgecli <game> -w none --frames 600 --seed 1` plays that many frames and exits; `--frames` alone implies no window and no sound. **Check:** a test runs it on Pong and gets exit code 0; a game that throws gives a message and a non-zero code.
  - [x] **2. `--keys file` (S).** The file is the `<demo>` format (`<press button="space">1005</press>`, `<release button="space">1009</release>`: a button and the frame), so a recording made with `--record` replays. **Check:** record ten seconds of Pong with `--record`, replay it with `--keys`, and the final score matches.
  - [x] **3. `Game::describe()` (M).** One function in `game/game.cpp` that writes the state as XML: the current state and scene, and for each object in play its full name, class, look, position, velocity and variables. XML because the project's tools are XML. **Check:** a test calls it on Pong after 60 frames and finds the ball and both paddles with their positions.
  - [x] **4. `--dump file` (S).** Writes `describe()` at the end, or every K frames with `--dump-every K`. A small XSD for the dump goes in `games/`. **Check:** the file validates against that XSD.
  - [ ] **5. `--picture file.png` (L; split before starting).** A software `Window` (`xge/lib/window/picture.cpp`, behind no option since it needs no library) that composes the frame from the engine's own bitmaps (text is `drawText`'s) into one image and writes a PNG with plutovg. **Check:** `xgecli pacman --frames 600 --seed 1 --picture p.png` writes a picture a person recognises as Pac-Man (the agent reads the image; then the author).
  - [ ] **6. Tell agents (S).** `docs/agents/readme.md` gains "how to look at a game". **Check:** docs only.
- **Not wanted:** a test per game that compares a dump with a stored one. It would freeze every game.
- **Files.** `xge/cli/source/cli.cpp`, `main.cpp`; `xge/lib/window/picture.cpp` (new), the window factory; `xge/lib/game/game.cpp`; `docs/reference/programs.md`, `docs/designs/52-cli.md`, `57-ideas.md` (the "machine-readable view" idea becomes built).
- **Done when** `xgecli pacman --frames 600 --seed 1 --picture p.png --dump p.xml` writes a recognisable picture and a state file that validates.

#### F2. Animations that run once

- **Needs** nothing.
- **Why.** Three listed limits at once: an animation has one interval for all frames, cannot run once and stop, and cannot be started by a collision. A dozen games fake it with look timers.
- **Steps.**
  - [x] **1. A time per frame (S).** A `<frame>` in an `<animation>` takes its own `<seconds>`; without, the animation's. **Check:** a test animation with frames of 0.1 and 0.5 seconds shows each for that long.
  - [x] **2. Once (M).** `<animation name="burst"><once />...` plays through and then the object shows the look it had before. **Check:** a test object goes back to its earlier look after the last frame, and a reset in the middle puts it back cleanly (the animation's state belongs in `Moment` if a reset must restore it: see [play notes](agents/play.md)).
  - [x] **3. What comes after (S).** With `<then sprite="x" />` it shows that look instead; with `<then>` holding commands (`<then><despawn /></then>`) it runs them. **Check:** a test for each form.
  - [x] **4. Started by a command (S).** `<become sprite="burst" />` already switches looks; switching to a `once` animation starts it from its first frame every time, from a collision rule too. **Check:** a test becomes it twice and sees frame one both times.
  - [x] **5. Move games, two or three a commit (M in all).** Explosions and deaths in Space Invaders, Galaxian, Asteroids, Missile Command, Demon Attack, Megamania, Astrosmash, Atlantis; Mario Bros.' flip; Q\*bert's fall. Also deaths that no longer reset at once in Pac-Man, Frogger, Donkey Kong, Berserk, Pitfall, Centipede, Frostbite, Haunted House, Yars' Revenge and Space Invaders 2.
  - [ ] **6. The author: a hitbox per frame (M if yes).** Hitboxes pulled in from the sides and several to an object are built ([collisions](reference/collisions.md#how-a-touch-is-found)). A `<frame>` may hold a `<hitbox>`, so a reach exists only on some frames, proved with a small original two-player boxing game. Only if the author wants that game; key sequences, blocking and hit-stun are a later decision.
- **Files.** `games/xgedef.xsd` (`animationType`), `load/xml.cpp`, `game/object.h`, `play/frame.cpp` (the step where animations and blinking count), `data.xsl`, `xge/tests/engine/test_bitmap_sprites.cpp`, `test_poses.cpp`; `docs/reference/pictures.md` "Animation", `docs/designs/31-animation-and-looks.md`.
- **Done when** the three limits are gone from `limits.md` and at least eight games lost a timer each.

#### F4. A list of numbers

- **Needs** a design agreed with the author before any code.
- **Why.** Four separate wants are one thing, a sequence the game adds to and reads by place: Simon (add a random colour each round, compare presses in order), Mastermind (a hidden list compared with a guess), Tetris' bag of seven (a list shuffled and drawn from), Pac-Man's schedule (a list of lengths, one per wave).
- **Where to start.** A holder already *is* an ordered collection with `<send>`, `<order><random />`, `<top>` and counts. Before inventing `<list>`, test whether a holder of small unseen pieces carrying one variable says all four. This is the fraction rule applied before building.
- **Steps.**
  - [ ] **1. Try the bag of seven as a holder (S).** In a scratch copy of Tetris, no engine change: a holder of seven unseen pieces, shuffled, sent from, refilled when empty. **Check:** it loads and deals each shape once per seven. Write down what read badly.
  - [ ] **2. Try a schedule as a holder (S).** The same for Pac-Man's wave lengths, read from the top. **Check:** as step 1.
  - [ ] **3. Write the design (S).** `docs/designs/15-lists.md`: what was wanted, the two trials, what Simon and Mastermind would still need ("compare place by place"), and a recommendation: a holder plus one word, or a new `<list>`. No code.
  - [ ] **4. The author decides.** Then the build steps are written here, in this shape, before anyone starts.
- **Done, for now, when** the design file exists with a verdict.

#### F7. Sound: an envelope, and notes that follow a number

- **Needs** nothing. Cannot be judged in a container: the author listens.
- **Why.** Plucked and bell sounds need a note that dies away; Space Invaders' march should speed up with the fleet.
- **Steps.**
  - [ ] **1. `<decay>` (S).** A `<note>` or `<part>` takes `<decay>seconds</decay>`: its volume falls to nothing over that time, in `synthesize()` (`xge/lib/audio/sound.cpp`). Schema `noteType` and `partType`, `readSound`, `data.xsl`. **Check:** a test in `xge/tests/engine/test_sound.cpp` compares the loudness of the samples at two times.
  - [ ] **2. A game uses it (S).** One sound in one game. **Check:** its test; add a line to [Waiting for a look on Windows](#waiting-for-a-look-on-windows) for the author to listen.
  - [ ] **3. Design notes that follow a number (S).** In `docs/designs/33-sound.md`, two options written out with what a game would say for each: a `<play>` that gives a pitch, or a sound made at several pitches at load and picked by a number when played. No code.
  - [ ] **4. The author picks.** Then the build steps are written here before anyone starts.
- **Done, for now, when** steps 1 to 3 are ticked.

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

Goals the author has set, beyond the tasks above. None is scheduled; delta time comes before the camera.

### Delta time

- **Decided by the author:** movement goes by time passed, not by frames. Being tied to the frame rate was never intended; it is how the first games happened to be written.
- **Today:** a velocity is pixels per frame, while timers are already in seconds, so a game's speed follows its `<framerate>` ([limits](reference/limits.md#movement-and-collisions)).
- **What the design must answer before it is built:**
  - the units a game file writes (pixels per second), and moving every game onto them;
  - how the frame-by-frame tests, the side-by-side proof of generated games, recorded demos and the board-game search stay repeatable when a step's length varies;
  - how a long step is kept from carrying something through a wall.

### A camera and scrolling

- **Version 2, on hold by the author's choice** ([ideas](designs/57-ideas.md)).
- Positions, screen edges and collisions all assume the window is the world, so the design is worth writing before much more code leans on that.

### A review of the C++

- The same pass the language had: name the few ideas the engine really has, then ask of each large file which idea it belongs to and why it needs more than that. Something can be abstract and still simple.
- The engine library is about 42,000 lines, and five files hold a third of it (`load/xml.cpp`, `load/expr.cpp`, `game/game.cpp`, `game/cards.cpp`, `game/object.h`).
- The reader is the first target: much of it writes out, tag by tag, what the schema already says.
- A2 to A4 above only tidy what is there.

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
| **Moves as timelines** (fighting games: one-shot animations, a hitbox per frame, key sequences), the next class after tables and tracks ([tables-and-tracks](designs/25-tables-and-tracks.md)) | Build the part eight or more existing games already work around, an animation that runs once ([F2](#f2-animations-that-run-once)), and stop before combos |
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
- **Cells** ([groups](designs/06-groups.md#cells)): gliding between cells; Tetris hold, lock delay, ghost piece, a bag of seven, row scoring; Columns chain scoring; Centipede's flea, spider and scorpion.
- **Cell puzzles** ([cell-puzzles](designs/28-cell-puzzles.md#open)): undo of despawned copies, a weighted spawn, a safe first square, conditions in a condition in an action.
- **Pictures** ([pictures](designs/30-pictures.md), [animation-and-looks](designs/31-animation-and-looks.md)): decals, a hole the shape of a bitmap, light blocked by walls, an interval per frame, an animation that runs once.
- **Sound** ([sound](designs/33-sound.md)): an envelope; notes that follow a variable (Space Invaders' march keeping time).
- **Stereo** (the author's wish): every sound is one channel today, and a stereo recording is averaged into it. Some later games, music above all, will want two. A mono sound then plays the same on both sides, so every game so far sounds as it does now.
- **Values** ([arithmetic](designs/03-arithmetic.md)): tangent, square root, power, `pick`, when a game needs one.
- **Conditions** ([states-and-conditions](designs/07-states-and-conditions.md)): object-level conditions, a total inside a value, conditions on edge rules.
- **Scenes** ([scenes](designs/12-scenes.md)): Berserk's larger grid of rooms, which needs maps made while playing.
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
- Building generated programs is by hand: `scripts/shell/windows/games-run-all.bat` generates, builds at MSVC `/W4` and tests them all.

## Waiting for a look on Windows

What only eyes, ears or MSVC can check. Whoever lands a change that needs one adds its line; the author deletes it once seen.

- **Animations that play once (F2 step 5):** Missile Command's and Depth Charge's bursts and Space Invaders 2's blast look as before and vanish when their frames end; in Mario Bros. a kicked pest pops up kicking its legs for half a second before it is gone.
- **New bursts and deaths (F2 step 5), new art drawn for each:** bursts for enemies hit in Galaxian, Demon Attack, Megamania, Astrosmash and Atlantis (buildings too), and a wreck for the player's ship in those and Asteroids. Deaths that stand the game still: Space Invaders and Space Invaders 2 (cannon bursts), Pac-Man (shrivels), Donkey Kong (spins, lies flat, halo), Q\*bert (curses; shrinks away off the pyramid), Berserk (flashes, zapped), Pitfall (flickers), Centipede (wand bursts), Frostbite (sinks; freezes blue), Yars' Revenge (bursts), Mario Bros. (flips and drops off the bottom, green for Luigi). Play goes on in Frogger (flat or splash rings, then a skull) and Haunted House (eyes flash red). Worth a look: timings (1 to 2.5 seconds each) and whether a stand-in sits where the player was.
- **Text drawn by the engine (A7):** titles and scores in every window (SFML, SDL2, raylib, OpenGL, `xgegui`) look alike, smoothed, a pixel or two different in width from before; Press Start 2P games (Mario Bros., Q\*bert, Pac-Man, Donkey Kong) stay crisp. Pong's paddle images look as before in every window. On Windows, a configure from scratch no longer fetches SDL2_ttf or SDL2_image, and `-DXGE_WITH_OPENGL=ON` alone no longer brings SDL2.
- **Sizes known at load (A7 step 4b):** every game starts with its titles and scores in their places from the first frame (no jump on the first frame); a score that grows a digit keeps its right edge where it was; `xgegui`'s inspector shows real sizes before the game is run, and editing a text there resizes it.
- **Vulkan (A7 step 7):** a configure from scratch with `BUILD_TESTING` fetches volk (and Vulkan-Headers when none is installed) with no Vulkan SDK; the build has no warnings; `xgetest "[window]"` passes for `vulkan`; Pong, Frogger and Klondike with `-w vulkan` play, resize with the game centred, and quit with the close box; OpenGL keys and pads work as before.
- **Line of sight (F5):** Berserk's robots fire only at a man they can see; Haunted House's eyes and matches no longer light rooms through walls.
- **The tarantula's search (F3):** in Haunted House it now comes the shortest way along the halls to the eyes instead of circling behind walls.
- **Smaller hitboxes:** Mario Bros.' brothers and pests, the frog, Pitfall Harry, Donkey Kong's barrels and fireballs, and the ships of Galaxian, Space Invaders, Demon Attack, Megamania and Astrosmash are touched by their bodies: a near miss on a wingtip, a toe or a hat should be a miss, while floors, walls and bounces feel as before.
- **The schema's documentation in an editor (A6):** hovering `<deflect>` (or any tag) in VS Code with the XML extension shows its sentence, and completion lists show them too.

## Known untested

- Sanitizers: the whole of `xgetest` is clean under `-DXGE_SANITIZE=address` (address and undefined behaviour), as are `xgetest-long`'s random keys and damaged files; the board-game search on its own thread is clean under `-DXGE_SANITIZE=thread`.
  - The damaged files found one fault: a line out at 1e9 or a text of a huge size asked for hundreds of gigabytes.
  - Pictures and texts are now at most 4096 pixels a side (`refusePictureSize`).
  - Generated programs and `xgegui` have not been run under one.
- MSVC is checked by hand, not automatically; older versions are untested.
- A default build (no options) and one with only `-DXGE_WITH_OPENGL=ON` have no automatic check ([build-and-layout](designs/51-build-and-layout.md)).
- Agents build in a Linux sandbox and cannot watch or listen, so how a game feels and sounds is unchecked.
