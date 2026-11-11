# Plan: review of the whole project, and the work that follows

This is a review of everything on `working` (schema, loader, engine, generator, 58 games, tests, docs) and a list of tasks that follows from it. Each task is written to be picked up cold: what to do, why, which files, what "done" is, and how to check it. Nothing in the code was changed by the review.

How to use this file: read [The short version](#the-short-version) and [What I would do differently](#what-i-would-do-differently), settle the [decisions](#decisions-for-the-author), then work the [order of work](#order-of-work) from the top. Tick a task off by deleting it here and moving anything still open into [open-work](designs/58-open-work.md). When this file is empty, delete it.

## The short version

**State.** The project is healthy. A build with every backend has no warnings on GCC. All 1,135 test cases pass, the generated-games test passes (every game's data matches field by field, then 900 frames side by side), and 60 further whole runs with different seeds found one flaky test, which is a real fault in one game (Crazy Eights can fail to end). Every one of the 58 games survived 20,000 frames of random key presses with two seeds: no throw, no crash, no number going to infinity. The loader was given 2,320 damaged copies of the games (a tag or attribute removed, doubled or changed, a number swapped for nonsense): it never crashed and never hung. The schema, the reader, the command list, the checker and the generator agree on all 74 commands and all 327 element names.

**The four findings that matter most.**

1. **A misspelt name can load and silently do nothing.** `<collision class="padle">`, `<condition object="bal" ...>`, `<condition ... variable="scor">` and `<show object="computr">` all load; the rule just never fires. Two objects of one name in the same `<objects>` are silently merged into one, and two states of one name are accepted. This is the largest correctness gap, because the project's promise is "mistakes stop the load" (B2, done; B3, done).
2. **The vocabulary is growing faster than it is being reduced.** 327 element names; 77 are used by exactly one game, 15 by none. Several concepts have two or three spellings: three jumps, two ways into and out of play, three ways to take turns, two ways to end a game, five booleans in a language that otherwise has none. Every simplification the author found so far (kit into objects, grid into group, pools into copies, phase numbers into looks, card nouns into holders) left the older form alive somewhere. The list is in [Simplification and consistency](#simplification-and-consistency).
3. **A new word costs about ten edits in ten places.** Schema, `isCommandTag`, `readCommand`, `RawCommand`, `makeCommand`, the `Cmd*` struct and variant, `CommandExecutor::run`, `ruleFor`, `data.xsl`, the describer in the generated test, two doc pages. That is why words are cheap to add and expensive to remove, and it is the debt that grows with every game ([A1](#a1-a-verb-is-one-file)).
4. **Frame pacing is each window library's own business.** SDL2 paces by vsync only, so on a 144 Hz monitor a game runs 2.4 times too fast; SFML sleeps; raylib and OpenGL count time themselves. Movement is per frame, so pacing is game speed (B5, done).

**What I would stop doing:** adding games and genres for now. See the next section.

## What I would do differently

Direct answers, as asked. Each is argued once here and turned into tasks below.

**1. Stop adding genres until the vocabulary has been reduced once, as a whole.** Fifty-eight games in about six weeks is proof the approach works; a fifty-ninth proves nothing more. Each recent game added three to eight words, and the vision file's own research question is "how small can a vocabulary be". The honest number today is 327 and rising. The author's rule ("test assumptions that only emerged; reduce like a fraction") has so far been applied one feature at a time, when a suggestion happened to reveal it. Apply it once across the whole language, on purpose, with the usage counts in this file as the starting evidence.

**2. The backlog's "next classes of game" are mostly the wrong next step.** From [open-work](designs/58-open-work.md):

| Listed next | My view |
|---|---|
| Dice, then backgammon | Demote. Backgammon is a large rule set that will want board-specific words (bar, bearing off, doubling). If dice are wanted, a dice-and-patterns game (the Yahtzee kind) reuses `<patterns>` and `<random>` and costs almost nothing; do that, and leave backgammon |
| Moves as timelines (fighting) | Promote, but for a different reason: "an animation that runs once, has its own frame times, and can be started by a rule" is a known limitation that eight or more existing games work around. Build that, prove it on existing games and a Boxing-kind game, and stop before combos and hit-stun |
| Lookahead for action games' computers | Cut. It is a research project, the arcade enemies are better on their simple rules, and nothing shipped needs it |
| A deck of single cards | Cut until a game cannot be written without it (the project's own rule) |
| More card games, more rooms, more levels | Cut. Depth in one game teaches the language nothing new |

**3. Spend the next weeks in this order instead:** correctness holes (small, high value), then the reduction pass, then one file per verb (so later words are cheap to add *and* remove), then tools that let an agent see and check a game without a window, and only then new capabilities, chosen because several existing games want them.

**4. The generator should write games that read as hand-written, and today it does the opposite.** `data.xsl` (3,806 lines), `merge.xsl` (748) and `game_parts.h` (502) re-implement `load/xml.cpp` and `load/merge.cpp` in XSLT, and what they write is the engine with one game's data baked in: nobody would write that over a weekend. My first advice was to make that target cheaper still (XSLT writes the XML tree, the engine reads it). The author's answer changes the aim: the point of the descriptions is to free a game from its platform, so a target should write *the game*, standing alone, in that platform's own terms. That is the first generator again (the one that drifted), and it can work now for three reasons it could not then: the frame order is one shared function (`play/frame.cpp`), the verbs' work is shared functions with no game file in them (`play/verbs.cpp`, `play/geometry.cpp`), and arithmetic is the engine's own exact `load/calc`. So: keep `windows-cpp` as it is, as the target that covers every game exactly, stop investing in it, and build a second target beside it that grows verb by verb and is tested frame by frame against the engine ([A5](#a5-a-game-written-out-as-if-by-hand)). One file per verb ([A1](#a1-a-verb-is-one-file)) is what makes that affordable: a verb's C++ and its template for each target sit side by side.

**5. The backends are a feature, so make the next one cheap rather than cutting any.** I called twelve libraries a tax. The author's answer: wide support for libraries is part of what the project is (as RetroArch does it), lower-level ones are wanted next (OpenGL is there; DirectX and Vulkan to come), and Xerces with a weaker built-in validator behind the lighter parsers is the intended design. Accepted. What follows from it is different work: a new window backend today has to draw text and load image files itself, which is most of its code and the reason no two backends measure a text the same. If the engine drew text and decoded images into bitmaps, as it already does every other picture, a backend would be "open a window, show bitmaps, read keys", and a Direct3D or Vulkan one would be a few hundred lines ([A7](#a7-the-next-backend-is-cheap)).

**6. "No CI" is costing more than it saves.** More than a dozen features are still waiting for a first look on Windows. MSVC-only warnings (C4702, C4459, C4458) and Windows header macros (`near`, `far`) have each broken the build more than once. A build-only job on a Windows runner (no tests, no gate) would catch those without changing how the author tests play. If the answer is still no, do T4 (done) instead ([D3](#decisions-for-the-author)).

**7. Stop writing arithmetic two ways.** Text arithmetic and `<formula>` both do `+ - * /`. Pong's title is centred with a twelve-line formula where every other game writes `<centered />`. **The author's call:** no arithmetic in strings. A text value holds one number or one name; every calculation is tags, with `<formula>` made short enough for the everyday cases ([R12](#r12-no-arithmetic-in-text)). The review's first advice (text for `+ - * /`, `<formula>` for the rest) is withdrawn: hiding a calculation in a string is the thing the format otherwise avoids, and it is a second parser to keep.

## What was run

| Check | Result |
|---|---|
| Configure and build, every backend, GCC 13, Release, Ninja (`-DBUILD_TESTING=ON -DFORCE_LOCAL_GLFW=ON -DBUILD_SHARED_LIBS=ON`) | 513 steps, 0 warnings |
| `ctest -j4` | 1,135 of 1,136 pass. The one failure is not ours: `xmltest`, registered by the fetched TinyXML2, "Not Run" (B1, done) |
| `xgetest` whole, 60 runs, `--rng-seed 7001` to `7060` | 58 of 60 clean. Seeds 7029 and 7036 fail the same Crazy Eights test, every time (B9, done: the games were only long, 326 and 329 turns; the bound is now 2,000). Nothing else varied |
| `xgetest-generated` (all 58 games generated, compiled, compared, played) | pass, 0 warnings |
| Random keys: each game, 20,000 frames, 2 seeds, 25 keys toggled at random | 116 of 116 clean |
| Damaged files: 40 per game, 2,320 in all, 3,000 frames each when they load | 0 crashes, 0 hangs; 1,580 refused with a message (451 by the schema, the rest by the loader); 699 loaded; 22 loaded with a position or size of infinity (B4, done) |
| The engine rebuilt with the address sanitizer (18 of its 47 files with the undefined-behaviour one too), every game played 6,000 frames of random keys | 58 of 58 clean. The test files were not run under it: with the strict warnings on, GCC took over fifteen minutes a file (T3, done) |
| Plain build (no options) and `xgegui` with Qt 6 | 277 steps, 0 warnings; `xgecli` and `xgegui` link (Qt 6.4). Not run: no display |
| Agreement scripts: every schema element against the reader, the generator, the reference docs, the tests and the games; every command against `makeCommand`, the schema, `data.xsl`, `commands.md` | All agree. Gaps: 15 elements no game uses, 45 no test names in XML, `call`, `clear`, `collapse`, `fall`, `order` are not in `commands.md` (they are on the maps and cards pages) |

Not checked, because it cannot be from a Linux container: MSVC, Windows, how any game looks, sounds or feels.

The scripts used are small and are described in T1, done (now `xge/tests/engine/test_vocabulary.cpp`; the damaged files and random keys are now `xgetest-long`, T2, done) so they can be rebuilt as permanent tests.

## Findings

### Bugs

| # | What | Evidence | Task |
|---|---|---|---|
| 1 | `ctest` exits with a failure on a clean tree | `xmltest (Not Run)` from `build/_deps/tinyxml2-build` | B1, done |
| 2 | A class, object, variable or look named in a filter or test is not checked | In `pong.xml`: `<collision class="padle">`, `<condition class="padle" variable="score">`, `<condition class="paddle" variable="scor">`, `<condition object="bal" variable="x">`, `<condition object="ball" variable="xx">`, `<show object="computr" />` all load and play | B2, done |
| 3 | Two objects of one name in the same `<objects>` become one, silently; two states of one name are accepted; one key bound twice in a state is accepted | `score2` renamed `score1` in `pong.xml`: 28 objects instead of 29, no message. `mainmenu` renamed `playing`: loads | B3, done |
| 4 | Text arithmetic that divides by zero gives infinity, which becomes a position or a size | `<x>1 / 0</x>` in 22 damaged files: loads, object at infinity | B4, done |
| 5 | Game speed depends on the window library and the monitor | `window/sdl2.cpp`: `SDL_RENDERER_PRESENTVSYNC` and no other pacing. `sfml.cpp`: `setFramerateLimit`. `raylib.cpp`, `opengl.cpp`: their own clocks. `gui/game_session.cpp`: a fifth | B5, done |
| 6 | Nonsense sizes and times are accepted | `<width>0</width>` on the window, a text `<size>0</size>`, `<after>-1</after>`, `<framerate>0</framerate>` all load | B6, done |
| 7 | Load errors from the reader say which tag but not which line | Only Xerces' schema errors carry `line N column M`; the reader's own `fail(where, ...)` has a path of tags | [B7](#b7-line-numbers-in-load-errors) |
| 8 | Stale text | `ruleFor` in `load/check.cpp` still lists `<reveal pile>`, `<shuffle>`, `<redeal />`, `<collect>`; the schema's comment at the patterns type names `<suited>`; `RawCommand::pile`'s comment lists removed verbs; `docs/agents/notes.md` says 55 games; `docs/reference/engine.md` says `<sheets>`; `docs/designs/27-cards.md` line 13 gives a name where the privacy rule in the notes says "the author" | B8, done |
| 9 | One test is flaky, and behind it Crazy Eights can fail to end | `crazyeights.xml: a whole game against the computer, playing the first card that goes, ends` (`xge/tests/engine/test_cards.cpp:807`) fails on 2 seeds of 60: `xgetest --rng-seed 7029` and `7036`. The game is still `playing` after 300 turns | B9, done: not a deadlock, the bound was too tight |

Known and documented, not counted as bugs: screen edges are tested by position and can be skipped by something very fast; a pair where nothing moves is not looked at; only the first hit of a pair per frame is handled ([limits](reference/limits.md)). [F6](#f6-rules-that-are-looked-at-when-nothing-moves) covers the second.

### Simplification and consistency

Each row is one concept done more than one way. Counts are games using each form, from the 58.

| # | Concept | The forms today | Reduce to | Task |
|---|---|---|---|---|
| 1 | Into and out of play | Pools: `<active>false</active>` (14 games) with `<reveal>` (11) and `<die />` (24). Copies: `<spawn>` (12), `<despawn>` (21). In between: `<fire>` (16) and `<release>` (9), which take "a copy, or from a pool" | Copies only for things that come and go; `<die />` and `<reveal>` only for fixed things that return (bricks, aliens). `<release>` is a `<spawn>` at the spawner's middle | R3, done |
| 2 | A jump | `<hop>` (20 games; instant, or timed with `<seconds>` and `<arc>`), `<jump>` (Frostbite only; a timed hop), `<leap>` (3; under a pull) | `<hop>` and `<jump>` (the old `<leap>`) | R2, done |
| 3 | Whose turn | A group with `<turns>` (Crazy Eights, Hearts, Spades, Gin). A `turn` variable on a cursor or table, tested everywhere (Checkers, Reversi, Four in a Row, Tic-Tac-Toe, Dots and Boxes, Hold'em, Cannon Hill). A state's `<turns>` (Galaxian: each player their own game), which is a different idea under the same word | Players as a group for every shared-world game. Rename the arcade one | R4, done |
| 4 | The end of a game | One end state whose object has a look per outcome (the form chosen in October). Separate win and lose states: Atlantis, Breakout, Demon Attack, Donkey Kong, Doubles, Frogger, Frostbite, Haunted House, Kaboom, Mario Bros., Megamania, Minefield, Pitfall, Q\*bert, Tennis | One end state with looks, or a lose state `from=` the win state | R5, done |
| 5 | State names | `title` (34) or `mainmenu` (23); `gameover` (27), `over` (13) or `lost` (6); `youwin` (11), `won` (9) or `wins` (4) | `title`, `playing`, `paused`, `over` | R5, done |
| 6 | Yes or no | `xs:boolean` content: `<active>`, `<drawn>`, `<enabled>`, `<lockstep>`, `<fullscreen>`. Empty tags elsewhere (`<loop />`, `<forced />`, `<cells />`). A pick elsewhere (`update="live"`) | Empty tags: `<hidden />`-style words chosen by the author | R6, done |
| 7 | The middle of the window | `<centered />` (433 uses), `<centered>window.height.center + 70</centered>` (289 of the 297 with content name the window's own middle), `window.width.center - title.width / 2` by hand (4 games), a `<formula>` (Pong) | `<centered />` and `<centered>` holding only the offset | R7, done |
| 8 | Names of the window | `window.right`, `window.bottom`, `window.width.center`, `window.height.center`; `window.width` is not a name | `window.width`, `window.height`, `window.middle.x`/`.y` or the author's choice; one scheme | R7, done |
| 9 | Arithmetic | Text and `<formula>` both do `+ - * /` (about 1,170 texts in all 58 games calculate) | Tags only: a text is one number or one name (the author's call) | [R12](#r12-no-arithmetic-in-text) |
| 10 | Where bitmaps live | `<pictures>` with `<bitmap from>` (36 games); inline `<bitmap>` rows in a sprite (14 games, 65 bitmaps) | `<pictures>` whenever a bitmap is used twice or is more than a few rows | R8, done |
| 11 | All of several tests | Nested conditions (everywhere); `<all>` inside `<any>` (Tennis only, 3 uses) | Decide whether Tennis can nest; if so remove `<all>` | R8, done |
| 12 | A speed test on a touch | `<slower>` (Lunar Lander), `<faster>` (no game): a second comparison vocabulary beside `<above>`/`<below>`/`<atleast>`/`<atmost>` | A rule's `<condition variable="speed"><below>` | done (R1) |
| 13 | Shared shows | `<shows name>` sets (Air-Sea Battle, Galaxian). The matching `<keys>` and `<conditions>` sets were removed as "state `from=` covers them" | Remove if `from=` covers these two as well | R8, done |
| 14 | Pause, title and game-over boilerplate | 57 `paused` states, 835 lines, 18 variants that differ by accident (which keys, whether the pad's Start is bound, the colour). Pad Start is bound in 24 games | One spelling, copied exactly; a test that holds it | R9, done |
| 15 | Either player | Rules written once per player: Pong's left and right edge rules, `score1`/`score2`, `paddle1`/`paddle2` keys; Combat, Cannon Hill, Space Race, Freeway the same | A word for the object touched and for "the other member", already in the backlog as `other` | R10, done (Pong's edge rules stay) |
| 16 | A place that holds things | 11 card games give a holder a dummy `<rectangle>` of 4 by 4 in the background colour, because a holder needs a size | A holder with no sprite | [R11](#r11-card-words-that-are-still-card-words) |
| 17 | Card words | The rule is "tags name mechanics, never one game". Still card-shaped: `<call pile=>`, `<with pile=>`, `<trick>`, `<trumps>`, `<breaks>`, `<melds />`, `<wild>`, and `suit`/`rank` as fixed variable names | See the task | [R11](#r11-card-words-that-are-still-card-words) |
| 18 | Steering | `<chase>`, `<aim>`, `<hunt>`, `<wander>`, `<go>`, `<follow>`, `<track>`, `<seek>` (1 game), `<pull>` (0 games) | Review as one family; at least decide `<seek>` and `<pull>` | R1, done |
| 19 | Words nobody uses | In the schema, in no game: `accelerate`, `pull`, `rise`, `image`, `gap`, `margin`, `near`, `faster`, `framerate`, `fullscreen`, `absolute`, `floor`, `negate`, `remainder`, `odd`. `touch.` is a documented name no game reads | Use in a game, keep with a reason, or remove | R1, done |
| 20 | Where a command may go | Written twice: the schema's command groups and `ruleFor` in `load/check.cpp` | Each verb's own file, checked against the schema ([A1](#a1-a-verb-is-one-file)) | A1 |
| 21 | What a tag may hold | Written three times: the schema, hand-written "unknown `<x>`; expected ..." checks through `load/xml.cpp`, and `data.xsl` | The schema, plus [A5](#a5-a-game-written-out-as-if-by-hand) | A1, A5 |

**The pattern in the history.** Reading the log from the first tag-based commit to now, the same thing happened at least nine times: a first form was built, grew by additions, and was later replaced by something smaller, usually on the author's suggestion.

| First form | Grew into | Replaced by | What is left of the old form |
|---|---|---|---|
| Function-call strings | `<equation>`, exprtk | Tags, `<formula>`, `load/calc` | Arithmetic still in text (row 9, R12) |
| `<grid>` | | Folded into `<group>`; later cells folded into `<map>` | Two grids remain: a group's columns and rows, a map's rows of characters |
| `hidden`, pools of spare objects | `<reveal>`, `<release>`, `<fire>` | `<spawn>`, `<despawn>`, copies | Row 1 |
| `<exists>`, scene kits | `<kit>` | The top-level `<objects>` | The C++ class is still `Kit`; `<active>` |
| `<check>`, `<all>`, `<any>` | sets of conditions | Conditions in conditions, `<first>`, `<otherwise>` | Row 11 |
| Phase numbers | 20 to 30 per card game | A look, tested by `sprite=` | Row 3: `turn` variables are phase numbers |
| `<deck>`, `<pile>`, `<hand>` | fifteen card verbs | Groups, holders, general verbs | Row 17, and the whole of [A2](#a2-holders-in-the-engine-not-piles) |
| Two win screens | | One end state with looks | Row 4 |
| Six verb dispatch tables, string pictures, hand-kept resets | | One dispatcher, `Picture`, `Moment` | `RawCommand`, `makeCommand`, `readCommand` are still one big list each ([A1](#a1-a-verb-is-one-file)) |

Two lessons for whoever works this plan. First, **when a simpler form lands, finish the job in the same change**: move every game, remove the old word from the schema, reader, generator, checker and docs, and rename the C++ to match. Second, **a suggestion that "worked and was added to" is the warning sign**: when a feature needs its third addition, stop and ask which assumption made it necessary.

### Architectural debt

| # | Debt | Why it gets worse with time | Task |
|---|---|---|---|
| 1 | A verb is described in about ten places | Each new verb widens every list; removing one is as costly as adding it, so none are removed | [A1](#a1-a-verb-is-one-file) |
| 2 | `RawCommand` is one struct of about 50 fields shared by 74 verbs, each field meaning something different per verb (`seconds` is a move's wait; `distance` is a swing's degrees; `positionX` is a hop's `<x>`) | A reader has to know which verb borrows which field; a mistake compiles | [A1](#a1-a-verb-is-one-file) |
| 3 | The engine underneath the holder words is still the old card engine: `Object::Pile`, `RawPile` with `dealt`, `refillShuffle`, `isCard`, `suit`, `rank`, and `settleHolders` mapping the new words onto it. `game/cards.cpp` is 2,167 lines | The language says "no card nouns" and the code says the opposite. Every non-card use of holders will fight it | [A2](#a2-holders-in-the-engine-not-piles) |
| 4 | Very long functions: `makeObject` 575 lines, `game_expr::init` 453, `makeCommand` 367, `readCommand` 358, `readGroup` 343, `checkCommand` 316, `checkReferences` 259, `game_xml::init` 247 | Each new feature adds a branch to the middle of one of these | [A3](#a3-split-the-long-functions) |
| 5 | `Object` has about 200 fields in one struct; `game.h` is 934 lines | Same | [A4](#a4-object-in-parts) |
| 6 | The generator is a second reader (4,500 lines of XSLT and helper) that writes the engine's data, not a game anyone could read | Every reader change is made twice, for output that is not what the generator is for | [A5](#a5-a-game-written-out-as-if-by-hand) |
| 7 | The schema has no `xs:annotation`; 165 attributes are plain `xs:string`; the word index in `docs/readme.md` is kept by hand | The XSD is the definition of the language but an editor shows nothing on hover, and the docs can drift | [A6](#a6-the-schema-documents-itself) |
| 8 | A command with a live number (`CmdLive`) is rebuilt from its `RawCommand` through `makeCommand`'s chain of string compares each time it runs | Fine at today's sizes (measured in October); it is the first thing a busier game will hit | [A1](#a1-a-verb-is-one-file) removes the string chain |
| 9 | Each window backend draws text and loads image files itself | Every new backend repeats it, and the author wants several more | [A7](#a7-the-next-backend-is-cheap) |

What is sound and should be left alone: the layering (`play/frame.cpp` decides order, `play/verbs.cpp` and `play/geometry.cpp` do the work, `Game` decides what to call); `Moment` with `start` and `before`; the one dispatcher; the load-time checks with "did you mean"; the seeded tests; the reference docs, which matched the code everywhere I compared them.

### Missing features

Measured against what a description language of this kind should do, within version 1 (one screen, no camera).

| # | Missing | Who wants it | Task |
|---|---|---|---|
| 1 | A way to check a game without a window, and to read a running game as data | Authors, tests, and any agent, which today cannot see a game at all. This is also what the research descendants of VGDL offer (a forward model: step the game, read its state) | T2, done, [F1](#f1-a-game-you-can-step-and-read-from-the-command-line) |
| 2 | An animation that runs once, with a time per frame, started by a command | Explosions, deaths, flips and bursts in a dozen games use look timers to fake it; fighting games need it | [F2](#f2-animations-that-run-once) |
| 3 | A search of a maze | Pac-Man's ghosts can circle behind a wall; Lode Runner kind; Berserk's robots | [F3](#f3-a-maze-search) |
| 4 | A list the game builds and compares | Simon, Mastermind, key sequences, a bag of seven for Tetris, Pac-Man's schedule | [F4](#f4-a-list-of-numbers) |
| 5 | A test of sight | Light through walls (Haunted House), stealth, robots that fire only when they see | [F5](#f5-line-of-sight) |
| 6 | A rule looked at when nothing moves; a condition on an edge rule | Listed limits; several games add a dummy motion to wake a rule | [F6](#f6-rules-that-are-looked-at-when-nothing-moves) |
| 7 | A sound envelope; notes that follow a number | Plucked and bell sounds; Space Invaders' march | [F7](#f7-sound-an-envelope-and-notes-that-follow-a-number) |
| 8 | Dice | The agreed next step; see my view above | [F8](#f8-dice) |
| 9 | Typed text, a word as data | Hangman, initials beyond `<letter>` | Not planned: small genre, large addition |
| 10 | A camera and scrolling | Version 2, on hold by the author's choice | Not planned here |

## Decisions for the author

Each has a default the plan assumes, so work is not blocked. Changing one changes only the tasks named.

| # | Decision | Default assumed | Affects |
|---|---|---|---|
| D1 | What the generator is for | **Settled by the author:** stand-alone games that read as hand-written, for any platform. `windows-cpp` (engine plus data) stays as the exact, complete target and gets no new design work; `data.xsl` is still updated with each language change until A5 covers enough games to retire it | A5, all R tasks |
| D2 | Drop XML, window and sound libraries | **Settled by the author:** keep all, and add lower-level ones (DirectX, Vulkan). See A7 | A7 |
| D3 | A Windows build-only job, or none | None (the author); T4 done | |
| D4 | The words for R6 (booleans) and R7 (window names, centred offset) | **Settled by the author:** `<waiting />` (and `<ready />` to turn it back on) where `<active>false` is still needed, `<hidden />` for `<drawn>false`, `<off />` for a collision's `<enabled>false`, `<lockstep />`, `<fullscreen />`; `<centered><offset>70</offset></centered>`; `window.width`, `window.height` become names and `window.width.center`, `window.height.center` stay (cheap engine constants) | R6, done; R7, done |
| D5 | Whether pools go entirely (R3) | **Settled by the author:** yes, pools were a stand-in from before spawning; `<die />` and `<reveal>` stay for fixed members | R3, done |
| D6 | Whether the standard pause and title screens become a built-in default | **No.** A hidden default breaks "one file is the whole game". Standardise the spelling instead | R9 |
| D7 | Pause on new games and genres until phases 1 to 3 are done | Yes | Order of work |
| D8 | The short tag forms that replace arithmetic in text | **Settled in part by the author:** no arithmetic in strings. The forms are proposed after R12's survey and the author picks | R12 (R7, done) |

## Rules for every task

These repeat what `AGENTS.md` and `docs/agents/notes.md` say, so a task can be done from this file alone.

- Branch from `working`. Never commit a build folder: build in `build/` (ignored) or outside the repository, and read `git status` before `git add`.
- Build and test before and after: `cmake -S . -B build -G Ninja -DBUILD_TESTING=ON -DCMAKE_BUILD_TYPE=Release -DFORCE_LOCAL_GLFW=ON -DBUILD_SHARED_LIBS=ON`, `cmake --build build --parallel`, `cd build && ctest -j4`. Zero warnings. After any change to the reader, `RawObject`, `RawCommand` or the generator: `cmake --build build --target xgetest-generated && output/Release/xgetest-generated`.
- A language change is made everywhere in one commit: `games/xgedef.xsd`; `xge/lib/load/xml.cpp`; `xge/lib/load/merge.cpp` and `games/generators/windows-cpp/merge.xsl` if it touches pieces or `from=`; `games/generators/windows-cpp/data.xsl` and `modules/game_parts.h`; the describer in `xge/tests/generated/test_generated_games.cpp`; every game that uses the old form; the tests; `docs/reference/<page>.md`; the word index in `docs/readme.md`; the matching `docs/designs/<file>.md`.
- When a word is removed, the reader must refuse it with a message that names the new form, and a test must hold that message.
- Each step of a task leaves the tests green. If a step cannot, it is too big: split it.
- Games keep the key convention (the test "every shipped game follows the shared key conventions", `xgetest "[engine_input]"`), colours are `color.` names, files end in one newline, every C++ file starts with the four-line header.
- One game is moved first as the proof, then the rest in groups of about ten, running that group's tests after each.
- When the plan and the code disagree, the code wins: check a file or function still exists before changing it, and correct this file.

## Tasks

Sizes: **S** under two hours, **M** half a day, **L** more than a day, to be split into its substeps. "Blocks" and "needs" give the dependencies.

### Phase 1: correctness

#### B7. Line numbers in load errors

- **Size** M. **Needs** nothing.
- **Why.** In a 1,500-line game "object 'playing.ground.1.1' > `<position>` > `<y>`" is slow to find, and a member of a group is named by a path that does not appear in the file.
- **Files.** `xge/lib/xml/document.h` (`XmlNode`: add `int line() const`, 0 when unknown), the four files `xge/lib/xml/xerces.cpp`, `tinyxml2.cpp`, `pugixml.cpp`, `rapidxml.cpp`; `xge/lib/load/xml.cpp` (`fail`), `xge/lib/load/merge.cpp` (the merged tree must carry each element's line from the file); `Raw*` structs that are checked later (`RawObject`, `RawCommand`, `RawCondition` each get an `int line`); messages thrown in `load/expr.cpp` and `load/check.cpp`.
- **Do, in steps.** (1) `XmlNode::line()` for Xerces (it already reports lines for schema errors; the DOM needs a user-data handler or a locator, see `xerces.cpp` near line 130) and TinyXML2 (`GetLineNum()`); PugiXML and RapidXML give an offset into the buffer: count newlines to it. (2) `fail()` in the reader appends "(line N)". (3) Carry `line` on `RawObject` and `RawCommand` and use it in `check.cpp` and `expr.cpp`. (4) The generated program has no file: `line` stays 0 and nothing is printed; `data.xsl` need not write it, and the describer in `test_generated_games.cpp` must skip the field.
- **Done.** Every message from a misspelling in B2's tests (`xge/tests/engine/test_names.cpp`) ends with the right line under each of the four XML libraries.
- **Verify.** `ctest`, `xgetest-generated`.

### Phase 2: a safety net for the reduction

### Phase 3: the reduction pass

R1 is done (its verdicts are in `docs/designs/11-vocabulary-map.md`). The rest are independent of each other unless noted, and each is one branch.

#### R11. Card words that are still card words

- **Size** M. **Needs** nothing; best done with A2.
- **Do, each a decision then a sweep.** A holder with no `<sprite>` (remove the dummy 4 by 4 rectangles in 11 games; a holder needs a place, not a size: find where `settleHolders` or the spread reads the holder's size and use the held members' size). `<call pile="discard" />` and `<with pile="board">` use `pile=` where every other tag says `object=`. `suit` and `rank` are fixed names for "row" and "column" of the pack: a pattern's `<alike variable="rank">` could be `<alike variable="column">` so a pack is any grid. `<trick variable="suit">` "for now follows the suit": lift the restriction or say why not. `<trumps>`, `<breaks>`, `<melds />`: keep (they are mechanics with those names in every rule book), and say so in `docs/designs/27-cards.md`.
- **Done.** No attribute or fixed variable in the schema is named for a card thing except those the design file lists as kept on purpose.

#### R12. No arithmetic in text

- **Size** L, in substeps. **Needs** D8; R7 is done (the 266 `window.height.center + N` in `<centered>` are `<offset>`s; the 63 uses of `window.*.center` left are card layouts and offsets of a top-left corner, which are this task's).
- **Why.** Finding 7, the author's call: a calculation hidden in a string is data encoded in a string, which the format otherwise never allows, and `load/calc` is a second language inside the first. About 1,170 texts in all 58 games calculate (Mario Bros. 70, the card games 40 to 53 each). Tags are what the schema, the generator and any later tool can see.
- **Do.**
  1. **Survey (S).** Count the shapes the 1,170 take: a name plus or minus a number, a name times a number, `a - b / 2` (centring by hand), a name's half, longer ones. Write the counts into `docs/designs/03-arithmetic.md`.
  2. **Decide the short forms (D8).** `<formula>` as it is takes several lines for `paddle.y + 10`. Propose to the author, with the survey's top shapes written each way, what makes the common ones short without strings: for example operand tags that read in one line (`<add><value>paddle.y</value><value>10</value></add>`), an `<offset>` child where a position is "that name, moved by" (`<x>ball.x<offset>4</offset></x>` breaks the one rule, so it is a child of a value tag, not mixed content), or named halves as names (`title.width.half`) the way `window.width.center` is today. The author picks; nothing is built before.
  3. **The reader (M).** A text value is one number (with its sign) or one name. Anything else stops the load, naming the text and showing the tag form. `load/calc` keeps only what `<formula>` needs (it may go entirely: `<formula>` is evaluated by `load/expr`); `data.xsl` and the describer follow.
  4. **The games (L).** One game first as the proof (Pong), then groups of about ten. Values must not change: for each game dump every object's start position, size and variables before and after, and compare; then the group's tests.
  5. **Docs.** `docs/reference/format.md` (values), `docs/designs/03-arithmetic.md`, `docs/readme.md`'s tour and the root `readme.md` if their examples calculate in text.
- **Done.** No game text holds an operator; `load/calc`'s text parser is gone or used by nothing a game writes; the text half of B4's check (a `/` in text by 0) goes with it, the `<divide>` half stays.

### Phase 4: architecture

#### A1. A verb is one file

- **Done so far.** Each verb is a file in `xge/lib/verbs/` with its name, forms and places, `onRelease`, `read`, `make`, `check` and `run`; `verbs/all.cpp` lists them; `readCommand`, `makeCommand`, `CommandExecutor::run`, the place check and the load-time check look the verb up; `xgetest "[verbs]"` holds the list against the schema. Why a written list and the other choices: [verbs](designs/14-verbs.md). What is left, each independent:
- **Size** M, S, L. **Blocks** nothing; A5 needs only what is done.
- **Substeps, each green.**
  1. **Printing moves (M).** The cases of `operator<<` for `Command` and for `RawCommand` (`game/command.cpp`) into each verb, as `Verb::print`. The generated-games test compares these dumps, so the text must not change by a character.
  2. **A verb's own helpers move beside it (S).** `hop`, `startFlight`, `huntByHop`, `spawnProjectile`, `leap`, `aim`, `chase`, `land`, `knock` in `play/executor.cpp` are each used by one verb (check with grep first); move each into its verb's file as a function there. Those several verbs use stay.
  3. **Optional: each verb owns its data (L).** `Command` becomes `{ const Verb* verb; data }` and `RawCommand`'s 50 shared fields become the verb's own. This is where "`seconds` means a move's wait" ends. It changes `data.xsl` and the describer in the generated test heavily, so do it only after A5 has made `data.xsl` retire, or not at all.
- **Careful.** The folder is compiled as one unit: name a file's functions for its verb (`readBounce`), never in an unnamed namespace. A refused verb returns false and ends its list: keep each `run`'s answer exactly. The generated programs compile the same files: after each substep, `cmake --build build --target xgetest-generated`.
- **Files.** `xge/lib/verbs/*`, `xge/lib/game/command.cpp`, `xge/lib/play/executor.*`; `docs/designs/14-verbs.md` ("Open" shrinks), `docs/agents/notes.md` "Loading".
- **Done.** To add a verb: one file in `xge/lib/verbs/`, one line in `all.cpp` and `engine.xml`, its `Cmd*` struct, the schema element, the docs.
- **Verify.** After each commit: `ctest`, `xgetest-generated`, and `xgetest-long "[speed]"` before and after (load and frame times must not rise).

#### A2. Holders in the engine, not piles

- **Size** L. **Needs** nothing; do R11 alongside.
- **Why.** Debt 3. The words were generalised in November; the engine was not. `settleHolders` translates `<holds>` into the old pile fields.
- **Substeps.**
  1. **Dead fields out (S).** In `RawObject::RawPile` and `Object::Pile` (`xge/lib/game/object.h` near lines 648 to 700), find every field no reader writes any more since the old words were removed (`dealt`, `dealtMore`, `dealsInto`, `deals`, `refillFrom`, `refillShuffle`, `refillLeave`, `faces`, `sorted`, ...): for each, `grep` for a writer in `load/xml.cpp` and `game/cards.cpp`; if only `settleHolders` writes it from a holder word, keep it for step 3; if nothing does, delete it and the code that reads it.
  2. **Rename (M).** `Pile` to `Holder`, `RawPile` to `RawHolder`, `isCard` to `held` (or `inHolder`), `game/cards.cpp` and `cards.h` to `game/holders.cpp` and `holders.h` (three comments name `game/cards.h` as the home of `settleHolders`: in `load/xml.cpp`, `game/states.h` and `game/object.h`), `thinkCards` to `thinkHeld`. Mechanical; one commit; update `engine.xml`, `ENGINE_SOURCES` in the top `CMakeLists.txt`, `data.xsl` field names and the describer.
  3. **Remove the translation (L).** Make the holder fields the holder words themselves (`holdsClass`, `spread`, `orderBy`, `counts`, `accepts` as conditions) and delete `settleHolders`' mapping, so the engine reads what the game wrote.
  4. **`suit` and `rank` (M).** Wherever the engine reads a held member's `suit` or `rank`, read its row and column in its group, under the names R11 chose.
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
- **Do.** Leave `Moment` (position, velocity and the rest that tests name) exactly as it is. Group the rest by feature into members that most objects never use and tests rarely name: `object.maze` (walker state), `object.cell` (cells and chains), `object.holder`, `object.moves` and `object.marks` (boards), `object.swing`, `object.path`, `object.jump`. One group per commit; a group that is unused could later become `std::optional` or a pointer, which also shrinks every object (58 games load thousands).
- **Done.** `struct Object` itself declares under 60 fields; `xgetest-long "[speed]"` no slower.

#### A5. A game written out as if by hand

- **Size** L, open-ended; the first three substeps are a few days. **Needs** A1 as it stands (done).
- **Why.** The author's aim for the generator: a stand-alone version of a game that looks hand-coded over a weekend, as the route to any platform. `windows-cpp` writes the engine with the game's data built in (complete and exact, and unreadable as a game). The first generator wrote readable programs and drifted from the engine; a hand-written Pong on the shared frame later matched the engine frame for frame over ten seeds of 3,600 frames, and was set aside (`git show 7163ea2^:games/prototypes/pong_min/source/pong_min.cpp`; `docs/designs/40-generator.md`, "A readable game on the shared frame").
- **The rule that keeps it honest.** A generated game never re-implements behaviour. It is written out plainly (its numbers as constants, its objects as variables, its rules as functions and `switch`es) and *calls* the same small work functions the engine calls: `bounce`, `deflect`, `stick`, the sweep, the frame order. Those travel with it as a small runtime folder (`play/`, `picture/`, `audio/sound`, one window file): no `Game`, no reader, no XML. A person writing Pong over a weekend would also have a `bounce()` helper; what they would not have is an interpreter, and neither does this.
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
  4. **Grow by game, simplest first (M each).** `pong.xml` (states, conditions, text, sounds, a computer player), Breakout (groups in columns and rows, looks), Space Race, Kaboom, Freeway, Frogger (`ride`, lanes), Space Invaders (lockstep, timers, spawn). Each new game adds a few templates and a line in the covered list. Stop and review with the author after Pong and after Breakout: the look of the output is the point, and only the author can judge it.
  5. **The side-by-side test (M).** `xgetest-generated` gains, per covered game, the same proof `windows-cpp` has: generated and engine play with the same seed and keys, every object compared each frame. The generated program needs a `step(keys)` and a way to list its objects for this; write them into `program.xsl` behind a macro so they vanish from a normal build.
  6. **Later, another platform (L, not planned in detail).** A second code-writing target is a folder of templates plus that platform's runtime (the work functions ported). The templates are the cheap half; the runtime is a port. `docs/designs/57-ideas.md` "Targets and capability profiles" is the design to pick up then.
- **Not doing.** Cards, the board-game search and cell puzzles in this target: they are large engines in themselves (`game/cards.cpp`, `think.cpp`, `cells.cpp`), and a "weekend" version would be those files copied. They stay on `windows-cpp` until the author asks otherwise.
- **Files.** `games/prototypes/pong_min/`, `games/generators/standalone-cpp/`, `xge/cli/source/generate.cpp` (targets are folders already; add the coverage check), `xge/tests/generated/test_generated_games.cpp`, `scripts/cmake/assets.cmake` (`data_generators`), `scripts/shell/windows/games-*.bat` (a target argument), `docs/reference/programs.md`, `docs/designs/40-generator.md`.
- **Done, first milestone.** `xgecli pong --generate standalone-cpp` writes a program under 300 lines of its own code that builds with no warnings at `/W4` and GCC's strict set, and matches the engine frame for frame for ten seeds.

#### A6. The schema documents itself

- **Size** M, then S. **Needs** nothing (R1 is done).
- **Why.** Debt 7, and the project's own point: use XML's tools. The XSD has good comments, but as `<!-- -->`, which no tool reads.
- **Substeps.**
  1. **Comments become `xs:annotation`/`xs:documentation` (M).** For every element and type. An XML editor then shows the sentence on hover and in completion. Check the weak validator (`xge/lib/load/xsd.cpp`) skips annotations, and that Xerces still validates all games.
  2. **`docs/reference/words.md` made from the schema by XSLT (M).** `games/generators/docs/words.xsl`: every element, its documentation, what it may hold, grouped by the schema's own groups. Run by `xgecli --generate docs` (the generator already runs stylesheets with libxslt) or by `xsltproc`. A test regenerates it and compares with the file in the repository, so the page cannot go stale. The hand-kept word index in `docs/readme.md` then links to it and can shrink.
  3. **Tighter attribute types (S).** Of the 165 `xs:string` attributes, those that hold one name become `xs:NMTOKEN` (63 already are), and those that pick from a list (`on=`, `side=`, `total=`) an enumeration. Dotted names (`variable="ball.hits"`) stay strings or get a pattern.
- **Done.** Hovering `<deflect>` in VS Code with the XML extension shows its sentence; `words.md` is generated and tested.

#### A7. The next backend is cheap

- **Size** M, then a backend at a time. **Needs** nothing.
- **Why.** The author wants more backends, lower-level ones included (Direct3D 9, 11, 12, Vulkan), in the spirit of programs that support every library. Today each window backend is 500 to 650 lines, and most of it is the two jobs the engine does not do itself: drawing text with the library's own font code, and loading image files with the library's own decoder. That is also why text widths differ between backends and from the built-in font (a listed limit).
- **Substeps.**
  1. **Write the contract down (S).** In `docs/reference/engine.md` "Backends" and at the top of `window/window.h`: exactly what a `Window` must do, as a numbered list, and what it may assume (pictures arrive as bitmaps).
  2. **Text drawn by the engine (M).** Glyphs into a bitmap with one font library for every backend (FreeType through plutovg, which is already linked for SVG, or `stb_truetype`, a single header), in `picture/font.cpp` beside the 8x8 fallback. A text object then reaches the window as a bitmap like any other picture, and its size is known before a window exists (so sizes no longer wait for `Engine`, and tests measure real widths). Each backend loses its font code. Check every game's text positions before and after, since widths will change slightly: compare with the SFML widths and tune the one font path until titles still fit.
  3. **Image files decoded by the engine (S).** `picture/image.cpp` already cuts sheets with plutovg; use it for `<image>` too and delete each backend's loader (and the SDL2_image dependency).
  4. **One conformance test for every window backend (M).** `xge/tests/engine/test_backends.cpp`: for each backend built, open a small window (under Xvfb on Linux), show three known bitmaps, read the pixels back where the library allows, feed a key, close. A new backend passes this and is done.
  5. **Then the backends, in this order:** Direct3D 11 (the simplest modern one: a window from Win32, a textured quad per object), Direct3D 9 (older machines), Vulkan (the most code for the same quad), Direct3D 12 last. Each is `xge/lib/window/<name>.cpp` behind `XGE_WITH_<NAME>`, a row in `scripts/cmake/dependencies.cmake` and the factory. **The Direct3D ones cannot be built or run in a Linux container:** they are written blind there and compiled, fixed and tested by the author on Windows, so keep each to one small file and expect a round or two.
- **Also gains.** [F1](#f1-a-game-you-can-step-and-read-from-the-command-line)'s picture of a frame becomes trivial once text is a bitmap, and `xgegui`'s Qt renderer shrinks the same way.
- **Done.** A window backend's file holds no font or image-file code; the conformance test runs over each; the first new low-level backend is under 300 lines.

### Phase 5: additions

Order by how many existing games gain. Each ends with the docs, a game or two moved to the new word, and the limitation removed from `docs/reference/limits.md`.

#### F1. A game you can step and read from the command line

- **Size** L. **Needs** nothing (B5 is done).
- **Why.** Missing feature 1. Agents build this project and cannot see it; the notes say so in three places. The engine already plays with no window in tests, draws every picture itself into pixels (a window library only shows a bitmap), and can copy a whole board to search it. What is missing is the front door.
- **Substeps.**
  1. **`xgecli <game> --frames N [--keys file] [--seed S]` (M).** Plays N frames with no window and no sound (a null `Window` like `played::FakeWindow`, moved from the tests into `xge/lib/window/none.cpp` and offered as `-w none`), then exits. The keys file is the `<demo>` format (`<press button="space">1005</press>`, `<release button="space">1009</release>`: a button and the frame), so a recording made with `--record` replays.
  2. **`--dump file` (M).** Every K frames, or at the end, write the state as XML: the current state and scene, and for each object in play its full name, class, look, position, velocity and variables. XML because the project's tools are XML: XSLT can turn it into anything, and a small XSD for it goes in `games/`. One function `Game::describe()` in `game/game.cpp`; the inspector in `xgegui` shows the same data and could later read it from there.
  3. **`--picture file.png` (L).** A software `Window` that composes the frame from the engine's own bitmaps into one image and writes a PNG (plutovg, already linked through lunasvg, writes PNG). Text needs glyphs: use the built-in 8x8 font first (already in `picture/font.cpp`), then FreeType through plutovg if the widths matter. This is the piece that lets an agent look at Capsules' Doc or Pinball's table and see whether it is right.
  4. **Use it (S).** One test per game kind that plays a recorded input and compares the dump with a stored one is *not* wanted (it would freeze every game). Instead: `docs/agents/notes.md` gains "how to look at a game", and the review's damaged-file script uses `--frames`.
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
- **Do.** A part of `<hunt>` (suggest `<search />`): the walker takes the first step of the shortest way through the maze's tiles (breadth-first over `mazeGrid`, which is already cached; the grid is at most a few thousand tiles, and the result can be cached per target tile until the target changes tile). Pac-Man's ghosts keep the arcade rule (no `<search />`): that circling is authentic. Use it in Haunted House (the ghost) or Berserk, and write the Lode Runner kind of chase into open-work as now possible.
- **Files.** `xge/lib/play/maze.cpp`, `game/command.h` (`CmdHunt`), `load/xml.cpp`, schema `huntType`, `data.xsl`, `xge/tests/engine/test_steering.cpp`, `docs/reference/maps.md` "Walking a map", `docs/designs/24-chase-and-paths.md`.
- **Done.** A test maze with the target behind a wall: without the word the walker circles for 600 frames, with it it arrives.

#### F4. A list of numbers

- **Size** L. **Needs** a design agreed with the author first (write `docs/designs/14-lists.md`: what was looked for, options, choice).
- **Why.** Missing feature 4. Four separate wants are one thing: a sequence the game adds to and reads by place. Simon (add a random colour each round, compare presses in order), Mastermind (a hidden list compared with a guess), Tetris' bag of seven (a list shuffled and drawn from), Pac-Man's schedule (a list of lengths, one per wave, in the backlog as "a schedule").
- **Sketch to start the design from.** A holder already *is* an ordered collection with `<send>`, `<order><random />`, `<top>` and counts. Before inventing `<list>`, test whether a holder of small unseen pieces carrying one variable says all four: the bag of seven is a holder shuffled and sent from; a schedule is a holder read from the top. If two of the four read naturally that way, the list is a holder and the addition is only "compare place by place". This is the fraction rule applied before building.
- **Done.** The design file exists with a verdict; if built, a Simon-kind game ships and Tetris draws from a bag.

#### F5. Line of sight

- **Size** M. **Needs** nothing.
- **Why.** Missing feature 5; listed limit "light is not blocked by walls".
- **Do.** A condition test `<sees object="player">` (or `class=`) true when the straight line between the two middles crosses nothing of a named class (`<past class="wall" />`), using the swept-line code in `play/geometry.cpp`. Then `<light>` may take the same `<past>` so walls shadow it (Haunted House). Berserk's robots fire only when they see.
- **Files.** `play/geometry.cpp`, `game/states.h` (`Condition`), `load/xml.cpp` (`readCondition`), schema `conditionTest`, `data.xsl`, `xge/tests/engine/test_conditions.cpp`, `test_dark.cpp`; `docs/reference/states.md` "Conditions", `pictures.md` "Light".

#### F6. Rules that are looked at when nothing moves

- **Size** M. **Needs** nothing.
- **Why.** Two listed limits: "a pair where nothing moves is not looked at" and "an edge rule cannot have a `<condition>`".
- **Do.** (1) A `<collision>` may hold `<always />` (the backlog calls it a watch flag): its pairs are looked at every frame while they overlap. `FrameRules::partners` already knows pairs per rule; add the flag to the pair. Find the games that fake motion to wake a rule and remove the fakes. (2) Edge rules take the same `<condition>` children object rules take (`EdgeGuard` in the notes already carries `sprite=` and `unless=`; add the condition list). Centipede's "falling and rising are looks since edge rules take no condition" can then be simplified.
- **Files.** `xge/lib/play/frame.cpp` (`moveObjects`, the `isMoving` test in `sweep`), `game/game.cpp` (`partners`), `load/xml.cpp` (`readCollisions`), schema `collisionType`, `data.xsl`, `xge/tests/engine/test_collision_rules.cpp`; `docs/reference/collisions.md`, `docs/designs/21-collisions-open.md`.

#### F7. Sound: an envelope, and notes that follow a number

- **Size** M. **Needs** nothing. Cannot be judged in a container: the author listens.
- **Do.** (1) `<note>` or `<part>` takes `<decay>seconds</decay>` (volume falls to nothing over that time) in `audio/sound.cpp` `synthesize()`; tests compare sample amplitudes at two times. (2) `<play sound="march"><pitch>...</pitch></play>` or a faster route: a sound made at several pitches at load and picked by a number when played, so Space Invaders' march follows the fleet. Design first in `docs/designs/33-sound.md`.

#### F8. Dice

- **Size** M. **Needs** D7 lifted.
- **Do.** A die is an object with six looks and a `<random>` variable drawn again by a command: check whether `<become>` to a look picked by a number plus `<set variable>` with a `<random>` already says it (it may: "values read when used"). If so the addition is nothing but a game. Write the dice-and-patterns game first (five dice, keep some, roll three times, score by `<patterns>` with `<alike>` and `<run>`), which tests whether patterns work off a holder of dice the way they do off cards. Backgammon only after, and only if the author still wants it once its word list is written down.

## Order of work

Small, safe steps; the tests are green after every line. A line is one branch off `working`, merged when its checks pass. Lines in the same block can be done in any order or by different sessions at once.

```
Blocks 1 and 2 are done.

Block 3  (reduction, any order; R1 and R4 are done)
  R11 card words
  R12 no arithmetic in text            (survey, then D8, then the sweep)

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
| R12 | D8 |
| A3 (`readCommand`, `makeCommand`) | A1 |
| A1 substep 3 | A5 far enough along to retire `data.xsl` |
| A4 | A2 |
| F8 | D7 lifted |

If only a few more things are ever done from this file, start with R11 (B2, B3, B5, R1, R3, R4, R5, R6, R7, R8, R9, R10 and T1 are done).
