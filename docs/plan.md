# Plan

What is next, and nothing else. The ordered tasks come first, each written to be picked up cold: what to do, why, which files, what "done" is, and how to check it. Under them is the backlog, which is not ordered and not decided.

- **Finishing a task:** delete it from this file, and from [the order](#order-of-work). Anything it leaves open goes in [the backlog](#backlog).
- **Goals with no date** are under [long term](#long-term).
- **Why something was decided** is in [the design files](designs/00-designs.md), not here. This file started as a review of the whole project; its findings became these tasks, and its settled arguments moved to the design file each belongs to.
- **When the plan and the code disagree, the code wins.** Check that a file or function still exists before changing it, and correct this file.

## Order of work

Small, safe steps; the tests are green after every line. A line is one branch off `working`, merged when its checks pass. Lines in the same block can be done in any order or by different sessions at once.

```
Block 4  (architecture)
  A1  a verb is one file: what is left (printing)
  A5  a game written out as if by hand, substeps 1 to 3, then a game at a time
  A7  backends: text and images drawn by the engine, then Direct3D 11
  A2  holders, substeps 1 to 4
  A3  long functions, one per commit
  A6  schema annotations, words.md
  B7  line numbers
  A4  Object in parts

Block 5  (additions; each ends by moving existing games onto it)
  F1  step and read from the command line
  F2  animations that run once
  F6  rules when nothing moves, conditions on edges
  F3  maze search
  F5  line of sight
  F4  lists                               (design first)
  F7  sound
  F8  dice
```

What blocks what, in one place:

| Task | Needs first |
|---|---|
| A3 (`readCommand`, `makeCommand`) | A1 |
| A1 substep 3 | A5 far enough along to retire `data.xsl` |
| A4 | A2 |
| F8 | the hold on new games lifted ([open decisions](#open-decisions)) |

## Open decisions

- **The hold on new games and genres.** New games were paused until the correctness and reduction work was done, and it is done. Whether to lift the hold now, or after the architecture block, is the author's call. Until then the additions below are chosen because several existing games want them, not to add a genre.

## Rules for every task

The rules for any change are in [AGENTS.md](../AGENTS.md), and what a language change must touch is in [the agent notes](agents/readme.md#changing-the-language). On top of those, for work from this plan:

- A task is one branch off `working`, merged when its checks pass.
- Each step of a task leaves the tests green. If a step cannot, it is too big: split it.
- One game is moved first as the proof, then the rest in groups of about ten, running that group's tests after each.
- When a word is removed, the reader must refuse it with a message that names the new form, and a test must hold that message.

## Tasks

Sizes: **S** under two hours, **M** half a day, **L** more than a day, to be split into its substeps. "Blocks" and "needs" give the dependencies.

### Correctness

#### B7. Line numbers in load errors

- **Size** M. **Needs** nothing.
- **Why.** In a 1,500-line game "object 'playing.ground.1.1' > `<position>` > `<y>`" is slow to find, and a member of a group is named by a path that does not appear in the file.
- **Files.** `xge/lib/xml/document.h` (`XmlNode`: add `int line() const`, 0 when unknown), the four files `xge/lib/xml/xerces.cpp`, `tinyxml2.cpp`, `pugixml.cpp`, `rapidxml.cpp`; `xge/lib/load/xml.cpp` (`fail`), `xge/lib/load/merge.cpp` (the merged tree must carry each element's line from the file); `Raw*` structs that are checked later (`RawObject`, `RawCommand`, `RawCondition` each get an `int line`); messages thrown in `load/expr.cpp` and `load/check.cpp`.
- **Do, in steps.** (1) `XmlNode::line()` for Xerces (it already reports lines for schema errors; the DOM needs a user-data handler or a locator, see `xerces.cpp` near line 130) and TinyXML2 (`GetLineNum()`); PugiXML and RapidXML give an offset into the buffer: count newlines to it. (2) `fail()` in the reader appends "(line N)". (3) Carry `line` on `RawObject` and `RawCommand` and use it in `check.cpp` and `expr.cpp`. (4) The generated program has no file: `line` stays 0 and nothing is printed; `data.xsl` need not write it, and the describer in `test_generated_games.cpp` must skip the field.
- **Done.** Every message from a misspelling in B2's tests (`xge/tests/engine/test_names.cpp`) ends with the right line under each of the four XML libraries.
- **Verify.** `ctest`, `xgetest-generated`.

### Architecture

#### A1. A verb is one file

- **Done so far.** Each verb is a file in `xge/lib/verbs/` with its name, forms and places, `onRelease`, `read`, `make`, `check` and `run`; `verbs/all.cpp` lists them; `readCommand`, `makeCommand`, `CommandExecutor::run`, the place check and the load-time check look the verb up; `xgetest "[verbs]"` holds the list against the schema. Why a written list and the other choices: [verbs](designs/14-verbs.md). What is left, each independent:
- **Size** M, S, L. **Blocks** nothing; A5 needs only what is done.
- **Substeps, each green.**
  1. **Printing moves (M).** The cases of `operator<<` for `Command` and for `RawCommand` (`game/command.cpp`) into each verb, as `Verb::print`. The generated-games test compares these dumps, so the text must not change by a character.
  2. **A verb's own helpers move beside it (S).** `hop`, `startFlight`, `huntByHop`, `spawnProjectile`, `leap`, `aim`, `chase`, `land`, `knock` in `play/executor.cpp` are each used by one verb (check with grep first); move each into its verb's file as a function there. Those several verbs use stay.
  3. **Optional: each verb owns its data (L).** `Command` becomes `{ const Verb* verb; data }` and `RawCommand`'s 50 shared fields become the verb's own. This is where "`seconds` means a move's wait" ends. It changes `data.xsl` and the describer in the generated test heavily, so do it only after A5 has made `data.xsl` retire, or not at all.
- **Careful.** The folder is compiled as one unit: name a file's functions for its verb (`readBounce`), never in an unnamed namespace. A refused verb returns false and ends its list: keep each `run`'s answer exactly. The generated programs compile the same files: after each substep, `cmake --build build --target xgetest-generated`.
- **Files.** `xge/lib/verbs/*`, `xge/lib/game/command.cpp`, `xge/lib/play/executor.*`; `docs/designs/14-verbs.md` ("Open" shrinks), `docs/agents/play.md` "Loading".
- **Done.** To add a verb: one file in `xge/lib/verbs/`, one line in `all.cpp` and `engine.xml`, its `Cmd*` struct, the schema element, the docs.
- **Verify.** After each commit: `ctest`, `xgetest-generated`, and `xgetest-long "[speed]"` before and after (load and frame times must not rise).

#### A2. Holders in the engine, not piles

- **Size** L. **Needs** nothing; R11 (the card words) is done.
- **Why.** The engine underneath the holder words is still the old card engine (`Object::Pile`, `RawPile`, `isCard`, `suit`, `rank`; `game/cards.cpp` is over 2,000 lines), so the language says "no card nouns" and the code says the opposite. The words were generalised in November; the engine was not. `settleHolders` translates `<holds>` into the old pile fields.
- **Substeps.**
  1. **Dead fields out (S).** In `RawObject::RawPile` and `Object::Pile` (`xge/lib/game/object.h` near lines 648 to 700), find every field no reader writes any more since the old words were removed (`dealt`, `dealtMore`, `dealsInto`, `deals`, `refillFrom`, `refillShuffle`, `refillLeave`, `faces`, `sorted`, ...): for each, `grep` for a writer in `load/xml.cpp` and `game/cards.cpp`; if only `settleHolders` writes it from a holder word, keep it for step 3; if nothing does, delete it and the code that reads it.
  2. **Rename (M).** `Pile` to `Holder`, `RawPile` to `RawHolder`, `isCard` to `held` (or `inHolder`), `game/cards.cpp` and `cards.h` to `game/holders.cpp` and `holders.h` (three comments name `game/cards.h` as the home of `settleHolders`: in `load/xml.cpp`, `game/states.h` and `game/object.h`), `thinkCards` to `thinkHeld`. Mechanical; one commit; update `engine.xml`, `ENGINE_SOURCES` in the top `CMakeLists.txt`, `data.xsl` field names and the describer.
  3. **Remove the translation (L).** Make the holder fields the holder words themselves (`holdsClass`, `spread`, `orderBy`, `counts`, `accepts` as conditions) and delete `settleHolders`' mapping, so the engine reads what the game wrote.
  4. **`suit` and `rank` (M).** Wherever the engine reads a held member's `suit` or `rank`, read its row and column in its group, under the names a pack gives them (`<columns name="rank">`, `<rows name="suit">`, R11, done).
- **Done.** `grep -rniE "pile|isCard|suit|rank|deal|shuffle" xge/lib` finds nothing but comments explaining a card game example. The eight card games' tests pass unchanged.
- **Verify.** `ctest "[cards]"` and the eight games' tests after every commit; `xgetest-generated`.

#### A3. Split the long functions

- **Size** M each, independent. **Needs** A1 for `readCommand`, `makeCommand` and `checkCommand` (it dissolves them).
- **Do.** Moves only; no behaviour change; one function per commit.
  - `game_expr::makeObject` (`xge/lib/load/expr.cpp` lines 762 to 1434) into `load/build.cpp` as a sequence of named steps: `buildNumbers`, `buildLooks`, `buildRules`, `buildActions`, `buildTimers`, `buildHolder`, `buildMoves`, ... each taking the `RawObject` and the `Object` being made. This is step 7 of the October source review, which was only half done (the checks moved; the building did not).
  - `game_expr::init` (lines 116 to 672) into steps named for what `docs/reference/engine.md` "Loading" lists: names, variables, sounds, objects, states, checks.
  - `readGroup` (`load/xml.cpp` lines 2953 to 3340) into "what the group gives", "members", "columns and rows", "row, column and cell changes".
  - `checkCommand` and `checkReferences` (`load/check.cpp`): one small function per command kind, called from a `std::visit`.
- **Done.** No function in `xge/lib` over 150 lines (`python3 -m lizard xge/lib -l cpp` lists none); `expr.cpp` under 1,200 lines.

#### A4. `Object` in parts

- **Size** L. **Needs** A2 (the holder part is then clean). Lowest priority in this phase.
- **Why.** About 200 fields. The October review left it "because it touches nearly every test line that says `object.position`".
- **Do.** Leave `Moment` (position, velocity and the rest that tests name) exactly as it is.
  - Group the rest by feature into members that most objects never use and tests rarely name: `object.maze` (walker state), `object.cell` (cells and chains), `object.holder`, `object.moves` and `object.marks` (boards), `object.swing`, `object.path`, `object.jump`.
  - One group per commit; a group that is unused could later become `std::optional` or a pointer, which also shrinks every object (58 games load thousands).
- **Done.** `struct Object` itself declares under 60 fields; `xgetest-long "[speed]"` no slower.

#### A5. A game written out as if by hand

- **Size** L, open-ended; the first three substeps are a few days. **Needs** A1 as it stands (done).
- **Why.** The author's aim for the generator: a stand-alone version of a game that looks hand-coded over a weekend, as the route to any platform.
  - `windows-cpp` writes the engine with the game's data built in (complete and exact, and unreadable as a game).
  - The first generator wrote readable programs and drifted from the engine; a hand-written Pong on the shared frame later matched the engine frame for frame over ten seeds of 3,600 frames, and was set aside (`git show 7163ea2^:games/prototypes/pong_min/source/pong_min.cpp`; `docs/designs/40-generator.md`, "A readable game on the shared frame").
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

- **Substeps.**
  1. **Bring the prototype back and make it current (M).** Restore `pong_min.cpp` from the commit above into `games/prototypes/pong_min/`, port it to today's `Object`/`Moment`, `FrameRules` and `load/calc` (the note in it about the old expression library regrouping constants no longer applies), and restore its test: ten seeds, 3,600 frames, every object equal to the engine's each frame. This is the specification by example.
  2. **A new target folder (M).** `games/generators/standalone-cpp/`: `generate.xsl`, `program.xsl` (main, CMake), `runtime.xml` (the engine files it copies: no `game/game.cpp`, no `load/`), and `verbs/` with one template per verb it can write (`bounce.xsl`: `<xsl:template match="bounce" mode="statement">`). `xgecli pong_min --generate standalone-cpp` must reproduce the hand-written file closely enough that a diff is a few lines.
  3. **Say what it cannot write (S).** Before running, the generator walks the game and lists every verb or part with no template in this target; if any, it stops and prints them with "use `windows-cpp` for this game". Nothing is silently approximated. A test holds the list of games each target covers.
  4. **Grow by game, simplest first (M each).** `pong.xml` (states, conditions, text, sounds, a computer player), Breakout (groups in columns and rows, looks), Space Race, Kaboom, Freeway, Frogger (`ride`, lanes), Space Invaders (lockstep, timers, spawn).
     - Each new game adds a few templates and a line in the covered list.
     - Stop and review with the author after Pong and after Breakout: the look of the output is the point, and only the author can judge it.
  5. **The side-by-side test (M).** `xgetest-generated` gains, per covered game, the same proof `windows-cpp` has: generated and engine play with the same seed and keys, every object compared each frame. The generated program needs a `step(keys)` and a way to list its objects for this; write them into `program.xsl` behind a macro so they vanish from a normal build.
  6. **Later, another platform (L, not planned in detail).** A second code-writing target is a folder of templates plus that platform's runtime (the work functions ported). The templates are the cheap half; the runtime is a port. `docs/designs/57-ideas.md` "Targets and capability profiles" is the design to pick up then.
- **Not doing.** Cards, the board-game search and cell puzzles in this target: they are large engines in themselves (`game/cards.cpp`, `think.cpp`, `cells.cpp`), and a "weekend" version would be those files copied. They stay on `windows-cpp` until the author asks otherwise.
- **Files.** `games/prototypes/pong_min/`, `games/generators/standalone-cpp/`, `xge/cli/source/generate.cpp` (targets are folders already; add the coverage check), `xge/tests/generated/test_generated_games.cpp`, `scripts/cmake/assets.cmake` (`data_generators`), `scripts/shell/windows/games-*.bat` (a target argument), `docs/reference/programs.md`, `docs/designs/40-generator.md`.
- **Done, first milestone.** `xgecli pong --generate standalone-cpp` writes a program under 300 lines of its own code that builds with no warnings at `/W4` and GCC's strict set, and matches the engine frame for frame for ten seeds.

#### A6. The schema documents itself

- **Size** M, then S. **Needs** nothing (R1 is done).
- **Why.** The XSD is the definition of the language, but an editor shows nothing on hover and the word index is kept by hand. It is also the project's own point: use XML's tools. The XSD has good comments, but as `<!-- -->`, which no tool reads.
- **Substeps.**
  1. **Comments become `xs:annotation`/`xs:documentation` (M).** For every element and type. An XML editor then shows the sentence on hover and in completion. Check the weak validator (`xge/lib/load/xsd.cpp`) skips annotations, and that Xerces still validates all games.
  2. **`docs/reference/words.md` made from the schema by XSLT (M).** `games/generators/docs/words.xsl`: every element, its documentation, what it may hold, grouped by the schema's own groups.
     - Run by `xgecli --generate docs` (the generator already runs stylesheets with libxslt) or by `xsltproc`.
     - A test regenerates it and compares with the file in the repository, so the page cannot go stale.
     - The hand-kept word index in `docs/readme.md` then links to it and can shrink.
  3. **Tighter attribute types (S).** Of the 165 `xs:string` attributes, those that hold one name become `xs:NMTOKEN` (63 already are), and those that pick from a list (`on=`, `side=`, `total=`) an enumeration. Dotted names (`variable="ball.hits"`) stay strings or get a pattern.
- **Done.** Hovering `<deflect>` in VS Code with the XML extension shows its sentence; `words.md` is generated and tested.

#### A7. The next backend is cheap

- **Size** M, then a backend at a time. **Needs** nothing.
- **Why.** The author wants more backends, lower-level ones included (Direct3D 9, 11, 12, Vulkan), in the spirit of programs that support every library.
  - Today each window backend is 500 to 650 lines, and most of it is the two jobs the engine does not do itself: drawing text with the library's own font code, and loading image files with the library's own decoder.
  - That is also why text widths differ between backends and from the built-in font (a listed limit).
- **Substeps.**
  1. **Write the contract down (S).** In `docs/reference/engine.md` "Backends" and at the top of `window/window.h`: exactly what a `Window` must do, as a numbered list, and what it may assume (pictures arrive as bitmaps).
  2. **Text drawn by the engine (M).** Glyphs into a bitmap with one font library for every backend (FreeType through plutovg, which is already linked for SVG, or `stb_truetype`, a single header), in `picture/font.cpp` beside the 8x8 fallback.
     - A text object then reaches the window as a bitmap like any other picture, and its size is known before a window exists (so sizes no longer wait for `Engine`, and tests measure real widths).
     - Each backend loses its font code.
     - Check every game's text positions before and after, since widths will change slightly: compare with the SFML widths and tune the one font path until titles still fit.
  3. **Image files decoded by the engine (S).** `picture/image.cpp` already cuts sheets with plutovg; use it for `<image>` too and delete each backend's loader (and the SDL2_image dependency).
  4. **One conformance test for every window backend (M).** `xge/tests/engine/test_backends.cpp`: for each backend built, open a small window (under Xvfb on Linux), show three known bitmaps, read the pixels back where the library allows, feed a key, close. A new backend passes this and is done.
  5. **Then the backends, in this order:** Direct3D 11 (the simplest modern one: a window from Win32, a textured quad per object), Direct3D 9 (older machines), Vulkan (the most code for the same quad), Direct3D 12 last.
     - Each is `xge/lib/window/<name>.cpp` behind `XGE_WITH_<NAME>`, a row in `scripts/cmake/dependencies.cmake` and the factory.
     - **The Direct3D ones cannot be built or run in a Linux container:** they are written blind there and compiled, fixed and tested by the author on Windows, so keep each to one small file and expect a round or two.
- **Also gains.** [F1](#f1-a-game-you-can-step-and-read-from-the-command-line)'s picture of a frame becomes trivial once text is a bitmap, and `xgegui`'s Qt renderer shrinks the same way.
- **Done.** A window backend's file holds no font or image-file code; the conformance test runs over each; the first new low-level backend is under 300 lines.

### Additions

Order by how many existing games gain. Each ends with the docs, a game or two moved to the new word, and the limitation removed from `docs/reference/limits.md`.

#### F1. A game you can step and read from the command line

- **Size** L. **Needs** nothing (B5 is done).
- **Why.** Missing feature 1. Agents build this project and cannot see it; the notes say so in three places. The engine already plays with no window in tests, draws every picture itself into pixels (a window library only shows a bitmap), and can copy a whole board to search it. What is missing is the front door.
- **Substeps.**
  1. **`xgecli <game> --frames N [--keys file] [--seed S]` (M).** Plays N frames with no window and no sound (a null `Window` like `played::FakeWindow`, moved from the tests into `xge/lib/window/none.cpp` and offered as `-w none`), then exits. The keys file is the `<demo>` format (`<press button="space">1005</press>`, `<release button="space">1009</release>`: a button and the frame), so a recording made with `--record` replays.
  2. **`--dump file` (M).** Every K frames, or at the end, write the state as XML: the current state and scene, and for each object in play its full name, class, look, position, velocity and variables. XML because the project's tools are XML: XSLT can turn it into anything, and a small XSD for it goes in `games/`. One function `Game::describe()` in `game/game.cpp`; the inspector in `xgegui` shows the same data and could later read it from there.
  3. **`--picture file.png` (L).** A software `Window` that composes the frame from the engine's own bitmaps into one image and writes a PNG (plutovg, already linked through lunasvg, writes PNG). Text needs glyphs: use the built-in 8x8 font first (already in `picture/font.cpp`), then FreeType through plutovg if the widths matter. This is the piece that lets an agent look at Capsules' Doc or Pinball's table and see whether it is right.
  4. **Use it (S).** One test per game kind that plays a recorded input and compares the dump with a stored one is *not* wanted (it would freeze every game). Instead: `docs/agents/readme.md` gains "how to look at a game", and the review's damaged-file script uses `--frames`.
- **Files.** `xge/cli/source/cli.cpp`, `main.cpp`; `xge/lib/window/none.cpp`, `picture.cpp` (new; behind no option, since they need no library), `window/factory`; `xge/lib/game/game.cpp`; `docs/reference/programs.md`, `docs/designs/52-cli.md`, `57-ideas.md` (the "machine-readable view" idea becomes built).
- **Done.** `xgecli pacman --frames 600 --seed 1 --picture /tmp/p.png --dump /tmp/p.xml` writes a picture a person recognises as Pac-Man and a state file that validates.

#### F2. Animations that run once

- **Size** L. **Needs** nothing.
- **Why.** Missing feature 2, and three listed limits at once: "an animation has one interval for all frames, cannot run once and stop, and cannot be started by a collision".
- **Substeps.**
  1. **A time per frame (S).** `<frame>` in an `<animation>` takes its own `<seconds>`; without, the animation's.
  2. **Once (M).** `<animation name="burst"><once />...`: plays through and then the object shows the look it had before, or, with `<then sprite="x" />`, that look; with `<then><despawn /></then>` (commands), runs them. This replaces the look-timer idiom "become burst; timer after 0.4: despawn".
  3. **Started by a command (S).** `<become sprite="burst" />` already switches looks; make switching to a `once` animation start it from its first frame every time.
  4. **Move games (M).** Explosions and deaths in Space Invaders, Galaxian, Asteroids, Missile Command, Demon Attack, Megamania, Astrosmash, Atlantis; Mario Bros.' flip; Q\*bert's fall. Count the timers removed.
  5. **A hitbox per frame (M), only if a game is written for it.** `<frame>` may hold a `<hitbox>`, so a reach exists only on some frames. Prove with a small two-player Boxing-kind game (original, on the key convention). Stop there: key sequences, blocking and hit-stun are a later decision.
- **Files.** `games/xgedef.xsd` (`animationType`), `load/xml.cpp`, `game/object.h` (the animation state belongs in `Moment` if a reset must put it back: see the notes), `play/frame.cpp` step 1 ("animations and blinking count"), `data.xsl`, `xge/tests/engine/test_bitmap_sprites.cpp`, `test_poses.cpp`; `docs/reference/pictures.md` "Animation", `docs/designs/31-animation-and-looks.md`.
- **Done.** The three limits are gone from `limits.md`; at least eight games lost a timer each.

#### F3. A maze search

- **Size** M. **Needs** nothing.
- **Why.** Missing feature 3. "A maze walker steers by the next tile nearest a target, with no search, so a target behind a wall can leave it circling."
- **Do.** A part of `<hunt>` (suggest `<search />`): the walker takes the first step of the shortest way through the maze's tiles (breadth-first over `mazeGrid`, which is already cached; the grid is at most a few thousand tiles, and the result can be cached per target tile until the target changes tile).
  - Pac-Man's ghosts keep the arcade rule (no `<search />`): that circling is authentic.
  - Use it in Haunted House (the ghost) or Berserk, and write the Lode Runner kind of chase into open-work as now possible.
- **Files.** `xge/lib/play/maze.cpp`, `game/command.h` (`CmdHunt`), `load/xml.cpp`, schema `huntType`, `data.xsl`, `xge/tests/engine/test_steering.cpp`, `docs/reference/maps.md` "Walking a map", `docs/designs/24-chase-and-paths.md`.
- **Done.** A test maze with the target behind a wall: without the word the walker circles for 600 frames, with it it arrives.

#### F4. A list of numbers

- **Size** L. **Needs** a design agreed with the author first (write `docs/designs/14-lists.md`: what was looked for, options, choice).
- **Why.** Missing feature 4. Four separate wants are one thing: a sequence the game adds to and reads by place. Simon (add a random colour each round, compare presses in order), Mastermind (a hidden list compared with a guess), Tetris' bag of seven (a list shuffled and drawn from), Pac-Man's schedule (a list of lengths, one per wave, in the backlog as "a schedule").
- **Sketch to start the design from.** A holder already *is* an ordered collection with `<send>`, `<order><random />`, `<top>` and counts.
  - Before inventing `<list>`, test whether a holder of small unseen pieces carrying one variable says all four: the bag of seven is a holder shuffled and sent from; a schedule is a holder read from the top.
  - If two of the four read naturally that way, the list is a holder and the addition is only "compare place by place".
  - This is the fraction rule applied before building.
- **Done.** The design file exists with a verdict; if built, a Simon-kind game ships and Tetris draws from a bag.

#### F5. Line of sight

- **Size** M. **Needs** nothing.
- **Why.** Missing feature 5; listed limit "light is not blocked by walls".
- **Do.** A condition test `<sees object="player">` (or `class=`) true when the straight line between the two middles crosses nothing of a named class (`<past class="wall" />`), using the swept-line code in `play/geometry.cpp`. Then `<light>` may take the same `<past>` so walls shadow it (Haunted House). Berserk's robots fire only when they see.
- **Files.** `play/geometry.cpp`, `game/states.h` (`Condition`), `load/xml.cpp` (`readCondition`), schema `conditionTest`, `data.xsl`, `xge/tests/engine/test_conditions.cpp`, `test_dark.cpp`; `docs/reference/states.md` "Conditions", `pictures.md` "Light".

#### F6. Rules that are looked at when nothing moves

- **Size** M. **Needs** nothing.
- **Why.** Two listed limits: "a pair where nothing moves is not looked at" and "an edge rule cannot have a `<condition>`".
- **Do.** (1) A `<collision>` may hold `<always />` (the backlog calls it a watch flag): its pairs are looked at every frame while they overlap.
  - `FrameRules::partners` already knows pairs per rule; add the flag to the pair.
  - Find the games that fake motion to wake a rule and remove the fakes. (2) Edge rules take the same `<condition>` children object rules take (`EdgeGuard` in the notes already carries `sprite=` and `unless=`; add the condition list).
  - Centipede's "falling and rising are looks since edge rules take no condition" can then be simplified.
- **Files.** `xge/lib/play/frame.cpp` (`moveObjects`, the `isMoving` test in `sweep`), `game/game.cpp` (`partners`), `load/xml.cpp` (`readCollisions`), schema `collisionType`, `data.xsl`, `xge/tests/engine/test_collision_rules.cpp`; `docs/reference/collisions.md`, `docs/designs/21-collisions-open.md`.

#### F7. Sound: an envelope, and notes that follow a number

- **Size** M. **Needs** nothing. Cannot be judged in a container: the author listens.
- **Do.** (1) `<note>` or `<part>` takes `<decay>seconds</decay>` (volume falls to nothing over that time) in `audio/sound.cpp` `synthesize()`; tests compare sample amplitudes at two times. (2) `<play sound="march"><pitch>...</pitch></play>` or a faster route: a sound made at several pitches at load and picked by a number when played, so Space Invaders' march follows the fleet. Design first in `docs/designs/33-sound.md`.

#### F8. Dice

- **Size** M. **Needs** the hold on new games lifted ([open decisions](#open-decisions)).
- **Do.** A die is an object with six looks and a `<random>` variable drawn again by a command: check whether `<become>` to a look picked by a number plus `<set variable>` with a `<random>` already says it (it may: "values read when used").
  - If so the addition is nothing but a game.
  - Write the dice-and-patterns game first (five dice, keep some, roll three times, score by `<patterns>` with `<alike>` and `<run>`), which tests whether patterns work off a holder of dice the way they do off cards.
  - Backgammon only after, and only if the author still wants it once its word list is written down.

## Long term

Goals the author has set, beyond the tasks above. Neither is scheduled; delta time comes before the camera.

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

- **A review of the C++, as the language had.** The engine library is about 42,000 lines, and five files hold a third of it (`load/xml.cpp`, `load/expr.cpp`, `game/game.cpp`, `game/cards.cpp`, `game/object.h`). Name the few ideas the engine really has, then ask of each large file which idea it belongs to and why it needs more than that. The reader is the first target: much of it writes out, tag by tag, what the schema already says. A2 to A4 above only tidy what is there.
- **A build that runs in a browser,** so a game can be played from a link without compiling anything.
- **A version on the language.** A game file says which version it was written for, and each change to the language comes with a stylesheet that upgrades older files.
- **A definition of 1.0:** which genres, which vocabulary is frozen, which targets. The plan has no finish line without it.
- **Sampled sound.** Only notes made into samples exist today; a recorded sound file cannot be played ([sound](designs/33-sound.md)).

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
