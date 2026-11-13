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
| 1 | [B7](#b7-line-numbers-in-load-errors): line numbers in load errors | Small, and every game writer gains |
| 2 | [A6](#a6-the-schema-documents-itself): the schema documents itself | Small, mechanical, and visible in an editor straight away |
| 3 | [F9](#f9-sound-files) steps 1 to 4: sound files | Small and self-contained; the first new thing a game can do |
| 4 | [A5](#a5-a-game-written-out-as-if-by-hand) steps 1 to 6: the stand-alone generator as far as Breakout | The author's main aim; early enough to learn whether the idea holds |
| 5 | [F2](#f2-animations-that-run-once): animations that run once | The addition the most existing games gain from |
| 6 | [F6](#f6-rules-that-are-looked-at-when-nothing-moves): rules when nothing moves | Removes two limits and some fakes in the games |
| 7 | [F3](#f3-a-maze-search), then [F5](#f5-line-of-sight): maze search, line of sight | Two small steering additions that share games (Berserk, Haunted House) |
| 8 | [A7](#a7-the-next-backend-is-cheap) steps 1 to 5: text and images drawn by the engine | The groundwork for new backends, and it makes F1's picture easy |
| 9 | [F1](#f1-a-game-you-can-step-and-read-from-the-command-line) steps 5 and 6: a picture of a frame | Cheap now that text is a bitmap |
| 10 | [A2](#a2-holders-in-the-engine-not-piles): holders in the engine | Tidying; nothing a player sees depends on it |
| 11 | [A3](#a3-split-the-long-functions), then [A4](#a4-object-in-parts): long functions, `Object` in parts | Tidying. If the [review of the C++](#a-review-of-the-c) starts first, it replaces these |
| 12 | [F7](#f7-sound-an-envelope-and-notes-that-follow-a-number) steps 1 to 3, [F4](#f4-a-list-of-numbers) steps 1 to 3 | Each ends at a design for the author to decide |
| 13 | [F8](#f8-dice): dice | Waits on the hold on new games |
| 14 | [A7](#a7-the-next-backend-is-cheap) step 6 onward, [A5](#a5-a-game-written-out-as-if-by-hand) step 7 onward | The two that never finish: a backend at a time, a game at a time |

What must come before what:

| Task | Needs first |
|---|---|
| A4 | A2 |
| A1 step 4 | A5 far enough along to retire `data.xsl`, and the author's yes |
| F1 step 5 | easier after A7 step 2, not blocked by it |
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

### Correctness

#### B7. Line numbers in load errors

- **Needs** nothing.
- **Why.** In a 1,500-line game "object 'playing.ground.1.1' > `<position>` > `<y>`" is slow to find, and a member of a group is named by a path that does not appear in the file.
- **Steps.**
  - [x] **1. The interface, and the easy library (S).** `XmlNode` in `xge/lib/xml/document.h` gains `int line() const`, 0 when unknown. Implement it for TinyXML2 (`GetLineNum()`); the other three return 0 for now. **Check:** a new test in `xge/tests/engine/test_xml_libraries.cpp` reads a three-line file with TinyXML2 and gets line 3 for the third element.
  - [x] **2. PugiXML and RapidXML (S).** Both give an offset into the buffer: count the newlines up to it. **Check:** the same test passes for both.
  - [x] **3. Xerces (M).** It already reports lines for schema errors; the DOM needs a user-data handler or a locator (see where `xerces.cpp` reports schema errors). **Check:** the same test passes for all four libraries.
  - [x] **4. The merged tree keeps the lines (S).** `xge/lib/load/merge.cpp` copies elements when it works out pieces and `from=`; each copy carries the line of the element it was copied from. **Check:** a test loads a game whose member comes from a piece and reads the piece's line from the merged node.
  - [x] **5. The reader's messages (S).** `fail()` in `xge/lib/load/xml.cpp` appends "(line N)" when the line is not 0. **Check:** a test with an unknown tag on a known line expects that line in the message.
  - [x] **6. The later checks (M).** `RawObject`, `RawCommand`, `RawCondition`, `RawCollisionRule` and `RawState` each get an `int line`, filled by the reader; `load/expr.cpp` gives a command's, condition's or object's line, and `load/check.cpp`, which works on what is made, the line of what holds the name: a rule, a condition, an object or a state (an action's command: its object's). **Check:** a test in `xge/tests/engine/test_names.cpp` misspells one name at a time in a game written over many lines, under each of the four libraries.
  - [ ] **7. The generated program (S).** It has no file: `line` stays 0 and nothing is printed. `data.xsl` need not write the field; the describer in `xge/tests/generated/test_generated_games.cpp` must skip it. **Check:** `xgetest-generated`.
- **Docs.** `docs/reference/format.md` where load errors are described; `docs/reference/limits.md` if it lists this.
- **Done when** all seven boxes are ticked.

### Architecture

#### A1. A verb is one file

- **Needs** nothing. A5 needs only what is already done.
- **Done so far.** Each verb is a file in `xge/lib/verbs/` with its name, forms and places, `onRelease`, `read`, `make`, `check` and `run`; `verbs/all.cpp` lists them; `xgetest "[verbs]"` holds the list against the schema. Why a written list: [verbs](designs/14-verbs.md).
- **Careful.** The folder is compiled as one unit: name a file's functions for its verb (`readBounce`), never in an unnamed namespace. A refused verb returns false and ends its list: keep each `run`'s answer exactly. The generated programs compile the same files.
- **Steps.**
  - [x] **1. A place for printing (S).** `Verb` (`verbs/verb.h`) gains an optional `print` for a `Command` and one for a `RawCommand`. `operator<<` for each (`game/command.cpp`) asks the verb first and falls back to its own cases. Nothing moves yet. **Check:** `xgetest-generated` (it compares these dumps, so the text must not change by a character).
  - [x] **2. Move the printing, ten verbs a commit (M in all).** Each verb's two cases move into its file. Repeat until the cases in `command.cpp` are gone, then remove the fallback. **Check after each commit:** `xgetest-generated`.
  - [x] **3. A verb's own helpers move beside it (S).** `hop`, `startFlight`, `huntByHop`, `spawnProjectile`, `aim`, `chase`, `land`, `knock` in `play/executor.cpp`: for each, `grep` for its callers; one used by a single verb moves into that verb's file, one used by several stays. One helper a commit. **Check:** `xgetest-long "[speed]"` before and after (load and frame times must not rise).
  - [ ] **4. The author: whether each verb owns its data (L if yes).** `Command` would become `{ const Verb* verb; data }` and `RawCommand`'s shared fields the verb's own, which is where "`seconds` means a move's wait" ends. It changes `data.xsl` and the describer heavily, so it waits until A5 has retired `data.xsl`, or is dropped. Ask before starting. With it, each command carries its own line, and B7's name checks can say the command's line rather than its holder's (the author wants that).
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

- **Needs** nothing. Open-ended; steps 1 to 3 are a few days.
- **Why.** The author's aim for the generator: a stand-alone version of a game that looks hand-coded over a weekend, as the route to any platform ([generator](designs/40-generator.md)).
  - `windows-cpp` writes the engine with the game's data built in: complete and exact, and unreadable as a game.
  - A hand-written Pong on the shared frame once matched the engine frame for frame over ten seeds of 3,600 frames, and was set aside (`git show 7163ea2^:games/prototypes/pong_min/source/pong_min.cpp`).
- **The rule that keeps it honest.** A generated game never re-implements behaviour.
  - It is written out plainly (its numbers as constants, its objects as variables, its rules as functions and `switch`es) and *calls* the same small work functions the engine calls: `bounce`, `deflect`, `stick`, the sweep, the frame order.
  - Those travel with it as a small runtime folder (`play/`, `picture/`, `audio/sound`, one window file): no `Game`, no reader, no XML.
  - A person writing Pong over a weekend would also have a `bounce()` helper; what they would not have is an interpreter, and neither does this.
- **What a generated Pong should look like** (the target to write towards; about 150 lines):

  ```cpp
  const float margin = 30, size = 16, width = 20, height = 120, step = 5;      // <variables>

  Object ball, paddle1, paddle2;

  void serve()                                   // the ball's <velocity>, drawn again on a reset
  {
      const float angle = randomBetween(-30, 30), side = randomBetween(-1, 1);
      ball.velocity = { sign(side) * speed * cosine(angle), speed * sine(angle) };
  }

  void ballMeetsEdge(Edge edge)                  // the ball's edge rules
  {
      if (edge == Edge::Top || edge == Edge::Bottom) { bounceScreenEdge(ball, edge); }
      else { reset(ball); serve(); }
  }

  void ballMeets(Object& other, const Touch& touch) { deflect(ball, other, touch.edge, 45); }

  void keys(const Keys& down)                    // the playing state's <inputs>
  {
      if (down[Key::W]) { moveByStep(paddle1, Direction::Up, step); }
      ...
  }
  ```

- **Steps.**
  - [ ] **1. Bring the prototype back (M).** Restore `pong_min.cpp` from the commit above into `games/prototypes/pong_min/` and port it to today's `Object`/`Moment`, `FrameRules` and `load/calc`. Restore its test. **Check:** ten seeds, 3,600 frames, every object equal to the engine's each frame. This file is the specification by example.
  - [ ] **2. A new target folder (M).** `games/generators/standalone-cpp/`: `generate.xsl`, `program.xsl` (main, CMake), `runtime.xml` (the engine files it copies: no `game/game.cpp`, no `load/`), and `verbs/` with one template per verb it can write (`bounce.xsl`: `<xsl:template match="bounce" mode="statement">`). **Check:** `xgecli pong_min --generate standalone-cpp` writes a file whose diff from the hand-written one is a few lines, and it builds with no warnings.
  - [ ] **3. Say what it cannot write (S).** Before running, the generator walks the game and lists every verb or part with no template in this target; if any, it stops and prints them with "use `windows-cpp` for this game". Nothing is silently approximated. **Check:** a test holds the list of games each target covers, and generating an uncovered game fails with the list.
  - [ ] **4. The side-by-side test (M).** `xgetest-generated` gains, per covered game, the proof `windows-cpp` has: generated and engine play with the same seed and keys, every object compared each frame. The generated program needs a `step(keys)` and a way to list its objects; write them into `program.xsl` behind a macro so they vanish from a normal build. **Check:** it passes for `pong_min`.
  - [ ] **5. `pong.xml` (M).** States, conditions, text, sounds, a computer player. **Check:** step 4's test passes for it. **Then the author** reads the output: its look is the point, and only the author can judge it.
  - [ ] **6. Breakout (M).** Groups in columns and rows, looks. **Check:** as step 5. **Then the author** reads the output again.
  - [ ] **7. One game a step after that (M each),** simplest first: Space Race, Kaboom, Freeway, Frogger (`ride`, lanes), Space Invaders (lockstep, timers, spawn). Each adds a few templates and a line in the covered list. **Check:** step 4's test for that game.
- **Not doing.** Cards, the board-game search and cell puzzles in this target: they are large engines in themselves, and a "weekend" version would be those files copied. They stay on `windows-cpp` until the author asks otherwise. Another platform is a later task: [targets](designs/57-ideas.md).
- **Files.** `games/prototypes/pong_min/`, `games/generators/standalone-cpp/`, `xge/cli/source/generate.cpp` (targets are folders already; add the coverage check), `xge/tests/generated/test_generated_games.cpp`, `scripts/cmake/assets.cmake` (`data_generators`), `scripts/shell/windows/games-*.bat` (a target argument), `docs/reference/programs.md`, `docs/designs/40-generator.md`.
- **Done, first milestone:** `xgecli pong --generate standalone-cpp` writes a program under 300 lines of its own code that builds with no warnings at `/W4` and GCC's strict set, and matches the engine frame for frame for ten seeds.

#### A6. The schema documents itself

- **Needs** nothing.
- **Why.** The XSD is the definition of the language, but its comments are `<!-- -->`, which no tool reads, so an editor shows nothing on hover.
- **Steps.**
  - [ ] **1. Make sure annotations are harmless (S).** Add one `xs:annotation`/`xs:documentation` to one element. **Check:** every game still validates with Xerces and with the weak validator (`xge/lib/load/xsd.cpp`); if the weak one trips, teach it to skip annotations, with a test.
  - [ ] **2. Move the comments, a section of the schema a commit (M in all).** Each comment that describes an element or type becomes that element's `xs:documentation`. **Check after each commit:** every game validates; at the end, hovering `<deflect>` in VS Code with the XML extension shows its sentence (the author looks).
  - [ ] **3. Tighter attribute types (S).** Of the `xs:string` attributes, those that hold one name become `xs:NMTOKEN`, and those that pick from a list (`on=`, `side=`, `total=`) an enumeration. Dotted names (`variable="ball.hits"`) stay strings. **Check:** every game validates, and a test file with a bad pick is refused by the schema.
- **Held by the author:** generating `docs/reference/words.md` from the schema. The word index in `docs/readme.md` stays hand made, with its groups, for now.
- **Done when** all three boxes are ticked.

#### A7. The next backend is cheap

- **Needs** nothing. Steps 1 to 4 are the groundwork; step 5 onward is a backend at a time.
- **Why.** The author wants more backends, lower-level ones included ([backends](designs/50-backends.md)).
  - Today each window backend is 500 to 650 lines, and most of it is the two jobs the engine does not do itself: drawing text with the library's own font code, and loading image files with the library's own decoder.
  - That is also why text widths differ between backends and from the built-in font (a listed limit).
- **Steps.**
  - [ ] **1. Write the contract down (S).** In `docs/reference/engine.md` "Backends" and at the top of `window/window.h`: exactly what a `Window` must do, as a numbered list, and what it may assume (pictures arrive as bitmaps). **Check:** docs only.
  - [ ] **2. Text drawn by the engine (M).** Glyphs into a bitmap with one font library for every backend (FreeType through plutovg, which is already linked for SVG, or `stb_truetype`, a single header), in `picture/font.cpp` beside the 8x8 fallback. A text then reaches the window as a bitmap like any other picture, and its size is known before a window exists. **Check:** a test measures a text's width with no window; every game's tests pass.
  - [ ] **3. Each backend loses its font code, one a commit (S each).** SFML, SDL2, raylib, OpenGL, then `xgegui`'s Qt window. **Check:** the file holds no font code. **Then the author** looks at a few titles on Windows, since widths change slightly; add a line to [Waiting for a look on Windows](#waiting-for-a-look-on-windows).
  - [ ] **4. Image files decoded by the engine (S).** `picture/image.cpp` already cuts sheets with plutovg; use it for `<image>` too and delete each backend's loader, and the SDL2_image dependency. **Check:** Pong's paddles (`assets/paddle.jpg`) load under every backend built.
  - [ ] **5. One conformance test for every window backend (M).** `xge/tests/engine/test_backends.cpp`: for each backend built, open a small window (under Xvfb on Linux), show three known bitmaps, read the pixels back where the library allows, feed a key, close. **Check:** it passes for every backend built.
  - [ ] **6. Direct3D 11 (M).** The simplest modern one: a window from Win32, a textured quad per object. `xge/lib/window/direct3d11.cpp` behind `XGE_WITH_DIRECT3D11`, a row in `scripts/cmake/dependencies.cmake` and the factory. **It cannot be built in a Linux container:** it is written blind there and compiled, fixed and tested by the author on Windows, so keep it to one small file and expect a round or two. **Check:** step 5's test, run by the author.
  - [ ] **7. Then, each its own step like 6:** Direct3D 9 (older machines), Vulkan (the most code for the same quad), Direct3D 12 last.
- **Also gains.** [F1](#f1-a-game-you-can-step-and-read-from-the-command-line)'s picture of a frame becomes trivial once text is a bitmap.
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
  - [ ] **5. `--picture file.png` (L; split before starting).** A software `Window` (`xge/lib/window/picture.cpp`, behind no option since it needs no library) that composes the frame from the engine's own bitmaps into one image and writes a PNG with plutovg. Text uses the built-in 8x8 font first. **Check:** `xgecli pacman --frames 600 --seed 1 --picture p.png` writes a picture a person recognises as Pac-Man (the agent reads the image; then the author).
  - [ ] **6. Tell agents (S).** `docs/agents/readme.md` gains "how to look at a game". **Check:** docs only.
- **Not wanted:** a test per game that compares a dump with a stored one. It would freeze every game.
- **Files.** `xge/cli/source/cli.cpp`, `main.cpp`; `xge/lib/window/picture.cpp` (new), the window factory; `xge/lib/game/game.cpp`; `docs/reference/programs.md`, `docs/designs/52-cli.md`, `57-ideas.md` (the "machine-readable view" idea becomes built).
- **Done when** `xgecli pacman --frames 600 --seed 1 --picture p.png --dump p.xml` writes a recognisable picture and a state file that validates.

#### F2. Animations that run once

- **Needs** nothing.
- **Why.** Three listed limits at once: an animation has one interval for all frames, cannot run once and stop, and cannot be started by a collision. A dozen games fake it with look timers.
- **Steps.**
  - [ ] **1. A time per frame (S).** A `<frame>` in an `<animation>` takes its own `<seconds>`; without, the animation's. **Check:** a test animation with frames of 0.1 and 0.5 seconds shows each for that long.
  - [ ] **2. Once (M).** `<animation name="burst"><once />...` plays through and then the object shows the look it had before. **Check:** a test object goes back to its earlier look after the last frame, and a reset in the middle puts it back cleanly (the animation's state belongs in `Moment` if a reset must restore it: see [play notes](agents/play.md)).
  - [ ] **3. What comes after (S).** With `<then sprite="x" />` it shows that look instead; with `<then>` holding commands (`<then><despawn /></then>`) it runs them. **Check:** a test for each form.
  - [ ] **4. Started by a command (S).** `<become sprite="burst" />` already switches looks; switching to a `once` animation starts it from its first frame every time, from a collision rule too. **Check:** a test becomes it twice and sees frame one both times.
  - [ ] **5. Move games, two or three a commit (M in all).** Explosions and deaths in Space Invaders, Galaxian, Asteroids, Missile Command, Demon Attack, Megamania, Astrosmash, Atlantis; Mario Bros.' flip; Q\*bert's fall. **Check after each commit:** those games' tests; note the timers removed in the commit message. Add a line to [Waiting for a look on Windows](#waiting-for-a-look-on-windows).
  - [ ] **6. The author: a hitbox per frame (M if yes).** A `<frame>` may hold a `<hitbox>`, so a reach exists only on some frames, proved with a small original two-player boxing game. Only if the author wants that game; key sequences, blocking and hit-stun are a later decision.
- **Files.** `games/xgedef.xsd` (`animationType`), `load/xml.cpp`, `game/object.h`, `play/frame.cpp` (the step where animations and blinking count), `data.xsl`, `xge/tests/engine/test_bitmap_sprites.cpp`, `test_poses.cpp`; `docs/reference/pictures.md` "Animation", `docs/designs/31-animation-and-looks.md`.
- **Done when** the three limits are gone from `limits.md` and at least eight games lost a timer each.

#### F3. A maze search

- **Needs** nothing.
- **Why.** A maze walker steers by the next tile nearest its target, with no search, so a target behind a wall can leave it circling.
- **Steps.**
  - [ ] **1. The search by itself (S).** A function in `xge/lib/play/maze.cpp` that, given the maze's tiles (`mazeGrid`, already cached), a start tile and a target tile, returns the first step of the shortest way (breadth-first; the grid is at most a few thousand tiles). No game uses it yet. **Check:** a test in `xge/tests/engine/test_steering.cpp` on a small written maze, with the target behind a wall and with no way at all.
  - [ ] **2. The word (S).** `<search />` inside `<hunt>`: the walker takes that first step. Schema `huntType`, the hunt verb's file in `xge/lib/verbs/`, `CmdHunt` in `game/command.h`, `data.xsl` and the describer. **Check:** a test maze with the target behind a wall: without the word the walker circles for 600 frames, with it it arrives.
  - [ ] **3. Remember the answer (S).** Cache the result per target tile until the target changes tile. **Check:** `xgetest-long "[speed]"` is no slower for the game in step 4.
  - [ ] **4. A game uses it (S).** Haunted House's ghost or Berserk's robots. Pac-Man's ghosts keep the arcade rule: that circling is authentic. **Check:** that game's test; add a line to [Waiting for a look on Windows](#waiting-for-a-look-on-windows).
- **Docs.** `docs/reference/maps.md` "Walking a map", `docs/designs/24-chase-and-paths.md`, the limit in `limits.md`, the word index. Note in the [backlog](#backlog) that a Lode Runner kind of chase is now possible.
- **Done when** all four boxes are ticked.

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

#### F5. Line of sight

- **Needs** nothing.
- **Why.** Light is not blocked by walls (a listed limit), and robots fire whether or not they can see.
- **Steps.**
  - [ ] **1. The test by itself (S).** A function in `xge/lib/play/geometry.cpp` that says whether the straight line between two points crosses any object of a given class, using the swept-line code already there. **Check:** tests in `xge/tests/engine/test_collision_geometry.cpp`: a wall between, a wall beside, no wall.
  - [ ] **2. The word (M).** A condition test `<sees object="player"><past class="wall" /></sees>` (or `class=`), true when the line between the two middles crosses nothing of the class in `<past>`. Schema `conditionTest`, `readCondition` in `load/xml.cpp`, `Condition` in `game/states.h`, `data.xsl` and the describer. **Check:** tests in `test_conditions.cpp` for seen, hidden, and a misspelt class stopping the load.
  - [ ] **3. A game uses it (S).** Berserk's robots fire only when they see the player. **Check:** Berserk's test; add a line to [Waiting for a look on Windows](#waiting-for-a-look-on-windows).
  - [ ] **4. Light stopped by walls (M).** `<light>` takes the same `<past>`, so walls shadow it. **Check:** a test in `test_dark.cpp` where a wall leaves the far side dark.
  - [ ] **5. Haunted House uses it (S).** **Check:** its test; the limit is gone from `limits.md`; the author looks.
- **Docs.** `docs/reference/states.md` "Conditions", `pictures.md` "Light", the word index.
- **Done when** all five boxes are ticked.

#### F6. Rules that are looked at when nothing moves

- **Needs** nothing.
- **Why.** Two listed limits: a pair where nothing moves is not looked at, and an edge rule cannot have a `<condition>`. Several games add a dummy motion to wake a rule.
- **Steps.**
  - [ ] **1. `<always />` in a `<collision>` (M).** Its pairs are looked at every frame while they overlap. `FrameRules::partners` already knows the pairs per rule; add the flag to the pair, and let it past the "is anything moving" test in the sweep (`xge/lib/play/frame.cpp`). Schema `collisionType`, `readCollisions` in `load/xml.cpp`, `data.xsl` and the describer. **Check:** a test in `xge/tests/engine/test_collision_rules.cpp`: two still, overlapping objects; the rule runs every frame with the word and never without.
  - [ ] **2. List the fakes (S).** Read the games for motion added only to wake a rule, and write the list in this step before changing any. **Check:** the list is in the commit message.
  - [ ] **3. Remove the fakes, a game a commit (S each).** **Check:** that game's test after each.
  - [ ] **4. Conditions on edge rules (M).** An edge rule takes the same `<condition>` children an object rule takes. `EdgeGuard` already carries `sprite=` and `unless=`; add the condition list. **Check:** a test where an edge rule runs only while a variable is above a number.
  - [ ] **5. Centipede uses it (S).** Its falling and rising are looks only because edge rules took no condition; simplify. **Check:** Centipede's test.
- **Docs.** `docs/reference/collisions.md`, `docs/designs/21-collisions-open.md`, both limits out of `limits.md`, the word index.
- **Done when** all five boxes are ticked.

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

- **Needs** nothing. Cannot be fully judged in a container: the author listens.
- **Why.** Sounds are only notes today; a recorded sound cannot be played. The engine already turns every sound into samples itself (`synthesize()`, `xge/lib/audio/sound.h`: signed 16 bit, one channel, `kSoundSampleRate`) and a sound library is only handed those samples. So a sound file is decoded by the engine into the same samples, and nothing after that point knows the difference.
- **The form.** Written as a sprite names an image file:

  ```xml
  <sound name="roar"><path>assets/roar.wav</path></sound>
  <sound name="hum"><loop /><volume>0.2</volume><path>assets/hum.wav</path></sound>
  ```

  A sound holds notes, parts or a path: one of the three, never two. `<loop />`, `<volume>`, `<play>`, `<silence>` and `<music>` work unchanged.
- **Steps.**
  - [ ] **1. One place makes a sound's samples (S).** Today each sound library calls `synthesize()` itself (`audio/sfml.cpp`, `audio/sdl2.cpp`, `audio/raylib.cpp`). Add `std::vector<std::int16_t> samplesOf(const SoundDesc&)` to `audio/sound.h` and `sound.cpp`, which for now only calls `synthesize()`, and make the three call it. No behaviour changes. **Check:** `synthesize(` is called only inside `sound.cpp` and the tests; `xgetest "[sound]"` passes.
  - [ ] **2. Read a WAV file (S).** A new `xge/lib/audio/wave.cpp` and `wave.h` (add to `ENGINE_SOURCES` and `engine.xml`): `std::vector<std::int16_t> readWave(const std::filesystem::path&)`. It reads uncompressed PCM, 8 or 16 bit, one or two channels, any rate; two channels are averaged into one; another rate is resampled to `kSoundSampleRate` by straight-line interpolation. It throws, naming the file, when the file is missing, not a WAV, compressed, or longer than a limit (suggest 60 seconds, a constant beside `kMaxNoteSeconds`). **Check:** a test in `xge/tests/engine/test_sound.cpp` writes a small WAV itself (named with `scratchName()`), reads it back and compares the samples; one test each for stereo, another rate, and each refusal.
  - [ ] **3. The word (S).** `<path>` in a `<sound>`. Schema `soundType`: its choice of notes or parts gains `<path>`. `RawSound` gains `path`; `readSound` (`load/xml.cpp`); `SoundDesc` gains `path`; `processSound` (`load/expr.cpp`) checks the file is there, so a missing file stops the load before a window opens. `samplesOf` calls `readWave` when the sound has a path, then applies the volume. **Check:** a test game with a path sound loads and its samples are the file's; a missing file stops the load with a message naming the sound and the file.
  - [ ] **4. The generator (S).** The `sounds` template in `games/generators/windows-cpp/data.xsl` writes the path, the describer in `xge/tests/generated/test_generated_games.cpp` compares it, and `--generate` copies the file as it copies an image's (`xge/cli/source/generate.cpp`, where it reads each `path`). **Check:** `xgetest-generated` with the test game from step 3.
  - [ ] **5. The author: a recording and a game.** `path` is already a word of the schema (an image's), so the vocabulary test forces no game. The author picks a recording and a game to use it in; the file's licence goes beside it in `games/assets/`, as the font's does. Add a line to [Waiting for a look on Windows](#waiting-for-a-look-on-windows): the author listens on each sound library.
  - [ ] **6. The author: compressed files (M once picked).** MP3 and Ogg each need a decoder in the engine: one small library, fetched like the others, behind one `readSoundFile` that picks by the file's ending and returns the same samples. Which library, and its licence, is the author's call: ask before fetching one.
- **Docs.** `docs/reference/sounds.md` (its first line says nothing is loaded from a file), `docs/designs/33-sound.md`, `docs/reference/limits.md`. A sampled sound cannot go to a target with only a sound chip: note it under targets in `docs/designs/57-ideas.md`.
- **Done when** steps 1 to 4 are ticked.

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

- **A build that runs in a browser,** so a game can be played from a link without compiling anything. Not hard, but better done once the language has settled.

### Language and engine

- **Steering** ([chase-and-paths](designs/24-chase-and-paths.md)): leading a target, chasing round walls, a real maze search as another verb, curved path legs.
- **Platforms** ([platforming](designs/23-platforming.md)): Donkey Kong's barrels choosing ladders by where Jumpman is, the other three screens, air control.
- **Cells** ([groups](designs/06-groups.md#cells)): gliding between cells; Tetris hold, lock delay, ghost piece, a bag of seven, row scoring; Columns chain scoring; Centipede's flea, spider and scorpion.
- **Cell puzzles** ([cell-puzzles](designs/28-cell-puzzles.md#open)): undo of despawned copies, a weighted spawn, a safe first square, conditions in a condition in an action.
- **Pictures** ([pictures](designs/30-pictures.md), [animation-and-looks](designs/31-animation-and-looks.md)): decals, a hole the shape of a bitmap, light blocked by walls, an interval per frame, an animation that runs once.
- **Sound** ([sound](designs/33-sound.md)): an envelope; notes that follow a variable (Space Invaders' march keeping time).
- **Values** ([arithmetic](designs/03-arithmetic.md)): tangent, square root, power, `pick`, when a game needs one.
- **Conditions** ([states-and-conditions](designs/07-states-and-conditions.md)): object-level conditions, a total inside a value, conditions on edge rules.
- **Scenes** ([scenes](designs/12-scenes.md)): Berserk's larger grid of rooms, which needs maps made while playing.
- **Reserved names:** an object, group, member, map or state may not be called `self`, `other`, `others`, `window` or `color`, nor anything a number, because dotted names mix the game's own names with the engine's words (`self.x`, `other.score`, `window.width`, `color.red`).
  - Variables and sounds are free (only ever after a dot, or named by `sound=`), and `top` is free (`top.rank` is read before any name).
  - Look for a way to say those things that needs no reserved words, so any name is free (an object may already be called `object`).

### The games

- Galaxian: escorts' points. Asteroids: the saucers' shots breaking rocks. Lunar Lander: hard landings.
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

Nothing waiting.

## Known untested

- Sanitizers: the whole of `xgetest` is clean under `-DXGE_SANITIZE=address` (address and undefined behaviour), as are `xgetest-long`'s random keys and damaged files; the board-game search on its own thread is clean under `-DXGE_SANITIZE=thread`.
  - The damaged files found one fault: a line out at 1e9 or a text of a huge size asked for hundreds of gigabytes.
  - Pictures and texts are now at most 4096 pixels a side (`refusePictureSize`).
  - Generated programs and `xgegui` have not been run under one.
- MSVC is checked by hand, not automatically; older versions are untested.
- A default build (no options) and one with only `-DXGE_WITH_OPENGL=ON` have no automatic check ([build-and-layout](designs/51-build-and-layout.md)).
- Agents build in a Linux sandbox and cannot watch or listen, so how a game feels and sounds is unchecked.
