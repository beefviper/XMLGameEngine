# Plan

What is next, and nothing else. The ordered tasks come first, each written to be picked up cold: what to do, why, which files, what "done" is, and how to check it. Under them is the backlog, which is not ordered and not decided.

- **Working a task:** do its steps in order, one commit each, and tick the step's box in that commit. When every box is ticked, delete the task from this file and from [the order](#order-of-work). Anything it leaves open goes in [the backlog](#backlog).
- **Goals with no date** are under [long term](#long-term).
- **Why something was decided** is in [the design files](designs/00-designs.md), not here. This file started as a review of the whole project; its findings became these tasks, and its settled arguments moved to the design file each belongs to.
- **When the plan and the code disagree, the code wins.** Check that a file or function still exists before changing it, and correct this file.

## Order of work

**[The C++ review](#the-c-review) comes first,** in its own section and order. After them, one task at a time, top to bottom. A task's steps are done in their own order, and the tests are green after every step. Where a line says "steps 1 to 4", the rest of that task comes back further down.

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

## The C++ review

Its own block of work, done before the tasks below: it touches every field of an object. **Design, measurements and reasons:** [engine-structure](designs/16-engine-structure.md). Read it first.

- **The aim.** The engine does the same things with less: a resource is held once, an object carries only the parts it uses, a tag is written in few places, and the code reads from its names.
- **What it must not do** is change what any game does. Every step here leaves every game playing exactly as before.
- **One tag, twelve homes.** `<drag>` today is in the schema, the object reader, the group reader, `rawDrag` in `RawObject`, `load/build.cpp`, `Object::Motion`, the frame and the printer. CR5 and CR7 take homes away; CR1 took two hand-kept lists of an object's numbers, CR6 `data.xsl`'s two and the describer's.

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
  - [x] **1. Sizes on record (S).** `xgetest-long "[size]"` (`xge/tests/long/test_size.cpp`) prints `sizeof` `Object`, `RawObject`, `RawCommand` and `Command`, and for Pong, Space Invaders, Pac-Man and Klondike the objects, the pictures they hold (count and bytes) and the textures a window keeping them as the backends do is asked for. The numbers are in [engine-structure](designs/16-engine-structure.md#what-was-measured).
  - [x] **2. Do the lists hide bugs (S).** Done: `test_command_values.cpp`. Five of the six lists missed `<arc>` and `<below>`: self, a copy's own name and a scene's names were never put in them, and a live number in them was read once. Light and seek were walked by the scenes elsewhere; grip by nothing. Was: Six places list "every value in a command" by hand (`game/object.cpp` in `nameSelfIn` and the two renames, `load/conditions.cpp`, `game/scenes.cpp`, `load/expr.cpp` for `live`), and only one names `arc` and `below`; the scenes' list of an object's numbers lacks seek, light and altitude. Write tests first: `self` in a `<hop>`'s `<arc>`, a spawned copy's own name in a `<spawn>`'s `<below>`, a scene object's `<light>` and `<seek>` naming another object of its scene, a live number in an `<arc>`. **Check:** each test passes or fails; the failures are the bugs.
  - [x] **3. One walk (S).** Done: `RawCommand::forEachValue` and `RawObject::forEachValue`, used by all six; Pac-Man's `--check` 2.07G to 1.96G. Names (`object`, `partner`, `variable`...) stay listed where each is renamed, since each place treats them differently (exact, at the start, as a look). Was: `forEachValue` and `forEachName` for `RawCommand` and for `RawObject` (beside each struct), used by all six. **Check:** step 2's tests pass; nothing else changes.

### CR2. A resource manager

- **Why.** Pictures are shared by five caches in four places (`drawOnce` and the flips in `game_expr`, each `Turnable`'s turned pictures, a static map of image files), lines and polygons are drawn per object, circles, rectangles and texts are drawn again whenever a window asks, and each window keeps a texture per object and builds it again on every change of picture ([what was measured](designs/16-engine-structure.md#what-was-measured)).
- **The shape.** One class, `Resources` (`xge/lib/engine/resources.h`, `.cpp`), owned by the `Game` (in its `game_expr`, as `Game::resources()`):

  ```cpp
  struct PictureId { std::uint32_t index{}; };          // 0 is "none"; SoundId, FontId, SheetId, SchemaId alike
  class Resources
  {
  public:
      PictureId pictureFile(const std::string& path);    // read once
      template <typename Draw>
      PictureId picture(const std::string& key, Draw&& draw);   // the same key gives the same id
      PictureId own(Bitmap made);                        // one object's alone (a holed picture)
      void release(PictureId owned);                     // only for what own() gave
      const Bitmap& bitmap(PictureId id) const;
      std::size_t pictureCount() const;                  // and pictureBytes(), for [size]
  };
  ```

  - A key is text holding everything that decides the pixels and nothing else (the kind, its rows or lines or file and part, colours, scale, flip, style, font and size), as `drawOnce`'s keys were. Not a `PictureKey` struct: that would be one more place listing every field of every sprite kind, which this review is removing. Turned, flipped and recoloured pictures are keys too (the key it was made from, then the turn).
  - Numbers in a key are written exactly (`std::to_chars`), so two descriptions share a picture only when they draw the same pixels.
  - Ids are never reused while a game is loaded, so an id held anywhere stays right. `own` and `release` are the one exception and are used only by `<hole>`.
  - A window keeps its textures by `PictureId`. It is told which ids are on screen and makes a texture the first time it sees one.
- **Steps.**
  - [x] **1. The manager, with image files (M).** `Resources` with `pictureFile()`; `Game` owns one; the static map and its mutex in `picture/image.cpp` went (`readPictureFile` reads, the manager keeps). An `<image>`'s pixels are read as the game loads into `Picture::image`. `xge/tests/engine/test_resources.cpp`.
  - [x] **2. Drawn pictures (M, one kind a commit).** `drawOnce` (rows, sheet cells, SVGs) asks the manager; lines and polygons are drawn once a description. Circles, rectangles and texts are drawn at the window's scale when a window asks, so they move with step 5. `test_resources.cpp`: 20 alike share one picture, a colour makes two. The four `[size]` games hold the same pictures as before (none of them draws lines).
  - [x] **3. Made pictures (M, one kind a commit).** Done: turned (keyed by the drawing's key and the degrees), flipped (`Resources::flippedLeftRight`); palettes were already drawn through `drawOnce`; holed went with step 4 and drawn at the window's size goes with step 5. Was:** Turned (`Turnable` in `picture/bitmap.h`, which keeps its own map today), flipped (`flipsOf`), palette swaps (`makePalettes`), redrawn at the window's size, and holed (`own`, released when its object is reset or despawned). **Check:** cases in `test_resources.cpp`: two objects with one picture turned to the same heading share one turned picture; a `<hole>` in one of two alike shields leaves the other's picture untouched, and `pictureCount()` returns to what it was after a reset. `test_holes.cpp`, `test_picture_scale.cpp`, `test_flippers.cpp` pass.
  - [x] **4. Objects hold ids (M).** Done: `PictureId` is one pointer to where the `Resources` keep the picture, read like it, so a collision or a test reads `*object.drawn.bitmap` with no manager at hand; holed pictures and a tether's line are `own`ed, `replace`d and `release`d. Only `Object::drawnPicture`, what windows take, still hands out a shared pointer (step 5). Was: `std::shared_ptr<const Bitmap>` leaves `Object` and its parts (`Looks`, `Animation`, `Drawn`, `Mirror`) for `PictureId`; `Object::drawnPicture` asks the manager. **Check:** `grep -rn "shared_ptr<const Bitmap>" xge/lib/game xge/lib/play` finds nothing; all tests.
  - [x] **5. Textures by picture (M, one backend a commit).** Done: every window keeps one texture a picture id (`Window::Textures`), each object only a sprite or a handle to it; the picture window draws from the picture and keeps none. `[size]` counts textures the same way: Pong 29 to 11, Pac-Man 777 to 43, Space Invaders builds 29 in 600 frames, not 1,109. Direct3D passed a MinGW syntax pass here and waits for a build on Windows. Was: Null window first, with a count of textures asked for; then SFML, SDL2, raylib, OpenGL, Vulkan, Qt (`xge/gui`); the Direct3D windows are changed blind and wait for the author's build. The contract in `window/window.h` and `docs/reference/engine.md` "What a window must do" is rewritten first. **Check:** a case in `test_resources.cpp`: the null window is asked for 1 texture by the 20-member game; `test_backends.cpp` under `xvfb-run` for each backend built; a line in Waiting for a look on Windows for Direct3D.
  - [x] **6. Sounds, fonts, sheets, schemas (M, one a commit).** Done: `Resources::samples` (a recording read once at load, notes when an audio library asks; `Audio::load` takes the game's `Resources`), `Resources::sheet` (let go once loaded), and a schema parsed once for the program (`validateWithKeptSchema`, `load/xsd`), since a game is read before any `Engine` exists. Fonts were already opened once for the program, with a lock, and are read where no game is at hand (text measuring, on the computer's thread too), so they stay. Was: Sound samples (`samplesOf` in `audio/sound.cpp`), font files (`picture/font.cpp`), sheets (`SheetPicture`), and the parsed schema (`XsdLiteValidator` in `load/xsd.*`). The schema outlives one game, so it is held by the `Engine`'s manager, which each game's manager asks. **Check:** cases in `test_resources.cpp`: two objects playing one sound share one set of samples; loading two games one after another reads `xgedef.xsd` once (a counter on the manager).
- **Files.** `xge/lib/engine/resources.*` (new, in `ENGINE_SOURCES` and `games/generators/engine-cpp/engine.xml`), `load/pictures.cpp`, `picture/*`, `game/object.h`, `window/*`, `audio/*`, `xge/gui/source/`; `docs/reference/engine.md`, `docs/agents/pictures.md`.
- **Done when** no file but `resources.cpp` makes a `Bitmap` that outlives a function, and `[size]` shows pictures and textures counted by what is different, not by object.

### CR3. Parts as components

- **Why.** An `Object` is 4,752 bytes whatever it is. Its parts (A4) become components: each kind in its own collection, held only by the objects that use it. The author: an object needs only its name; everything else, its place and picture included, is a part it has or has not.
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
  - A `Game` is not copied (its loaded expressions point at its stores); a player's game put away while another plays (`Turn`) keeps a copy of the stores.
- **Steps.**
  - [x] **1. The author: the table of parts.** In [object-parts](designs/17-object-parts.md). The author chose a store per part; the per-part proposals are applied as each part moves in step 4.
  - [x] **2. A handle with a generation (M).** `ObjectId` (`game/object.h`), `Game::find(ObjectId)`, each object's `id`, slots freed when a copy leaves and used again a generation on (`test_store.cpp`). The `Object*` kept in `game.h` and `play/frame.h` all live within one step or are cleared when the roster changes, and what is kept from frame to frame is kept by name, which never changes (a copy's number is never reused), so none needed moving. Names become ids only where a store needs one (step 4) or speed gains (CR8 step 4).
  - [x] **3. The store (S).** `Store<Part>` (`game/store.h`): `has`, `find`, `get`, `add(id, order, part)`, `remove`, `each` in object order, `keepStart` and `restart`. `order` is where the object stands among the game's objects: objects join in order and keep it, so a number given as each joins is enough. A despawn must `remove` from every store (step 4).
  - [ ] **4. Parts move out, one a commit, largest first (L in all).** `Holder`, `Drive`, `Hand`, `Cells`, `Animation`, `MapPlace`, then the rest of step 1's table. For each part:
    1. it leaves `Object` and gets its store in `Game`;
    2. `load/build.cpp` adds it only for an object whose file has its tag, or that has a command of its verb family;
    3. a verb that can give it to an object while playing adds it on first use;
    4. every reader of `object.<part>` asks the store, with `find` where the object may have none;
    5. spawn copies it, despawn removes it, reset restores it.
    Moved so far: `Holder` (`Game::parts().holders`, `isHolder`, `holderOf`; handles live in `game/parts.h` with the stores); `Drive` (`Game::driveFor` gives it on the first key that drives, `<stop />` and a reset take it away, the frame reads it); `Hand` (`isHand`, `handOf`, for an object whose action picks up held items); `Maze` (`Game::mazeOf`; `<go>`, `<hunt>` and `<wander>` start a walk on the part); `Object::Cells` (`Game::cellPart` reads, an empty part for one with none; `cellPartFor` gives one to a piece sent into a cell map; `isShape` moves to the game); `Tether`, `Pull` and `Aloft`, rare enough to go in one commit; `Rest` (for an object with `<friction>`; `knock` and `bounce` take the game's parts); `Carry` (given by the first `<carry />`, `Game::carrying` reads what is carried); `Picker` with `Marks` folded into it (`Game::pickerOf` reads, `pickerFor` gives). Then `Turns` (`Game::turnsOf`), `Item` (`itemOf` reads, `itemFor` writes) `SizeUse`, what a size-dependent position uses (`usesSize`), and `Climb`, what a `<climb>` asked (the footing keeps only grounded, jumping, climbing): 2,000 to 1,736 bytes. Then a board piece's `Moves` (`Game::pieceMoves`; live `moves` linked only for a piece with them), a flipper's `Swing` (given by the first `<swing>`) and a light's `LightPast`: 1,168 to 1,096 bytes. A map piece's `MapPlace` (`Game::mapOf`, `mapFor`; whether it is on a map of cells stays in the object as `onCells`, which every frame asks): 1,096 to 1,032 bytes. A text's pieces that show variables, `BoundText` (`Game::textPiecesOf`, `showsVariable`): 1,032 to 1,008 bytes. What an object follows (`Follows`, update="live") and the picture a hole was first cut in (`Unholed`, given by `cutHole`, taken by a reset): 872 to 824 bytes. A `<surface>`'s drag and grip (`Surface`; the object keeps only `ground`, what it has under it this frame): 824 to 808 bytes. A `<mirror>`'s flipped pictures are its looks' (`LookSet::flips`), shared with its copies: 808 to 784 bytes. Each object's start `Moment`, read only by a reset, a spawn and a few verbs (`Parts::starts`; `Game::startOf` reads, `startFor` writes as it loads): 784 to 696 bytes. An object's own `<hitbox>`es go with its kind's rules (`CollisionData::hitboxes`, read by `Object::hitboxes()`), as they are written with them: 696 to 672 bytes. Its yes/no fields packed side by side, and a flight's numbers by size: 672 to 640 bytes. Timer clocks, for an object with `<timers>` (`Parts::clocks`, `Game::clocksOf`; a look change goes through `Game::showLook`, which starts the timers that follow it over): 640 to 616 bytes. An animation's numbers by size: 616 to 608 bytes. An object's outer groups are one `NameList` (`game/name.h`), a list held once in the `NameStore` and shared by a group's members: 608 to 592 bytes, under the 600 aimed at. The frame reads every store through `FrameRules::parts()`.
    **Check after each commit:** all tests and `xgetest-generated`; a case in `test_store.cpp` that Pong's ball has no such part (for a part Pong does not use); `[size]` and `[speed]` numbers in the commit, neither worse.
  - [ ] **5. What every drawn thing has (the author: it stays).** `Moment` (place, speed and the rest) and the picture, its `animation` with it (the author), stay in the object: nearly every object has them (the picture 5,722 of 5,926) and most verbs touch them. The picture becomes a `PictureId` (CR2 step 4) and `drawn` keeps only what a text needs besides. `before` shrinks to what is read of it (position, `collisionsOn`, `isVisible`): done, 2,624 to 2,416 bytes. The cell place (column, row, a `<go>`, a chain's links, what a piece is joined with) left the `Moment` for the cell part (`Object::Cells::Place`, `Game::place` reads, `placeFor` gives; a reset puts back its `start`): 2,160 to 2,000 bytes. Live `column` and `row` are linked only for an object with a cell part (`LivePart::has`), which also made the frame cheaper.
  - [ ] **5a. The kind, shared (L, a part a commit).** What is the same for every object made from one piece (collision rules and hitboxes, animation frames, looks, moves, layout, and the rest that no verb changes while playing) moves into one `Kind` record per piece, held by the game; an object points at its kind. Something a verb does change while playing stays in the object, or is copied out of the kind on the first change. **Check:** all tests; Space Invaders' 55 invaders hold one set of collision rules between them.
    Done so far: rules, timers and actions keep `self` and `other`, found as they run (`CmdLive::namesSelf`/`namesOther`, `Game::holdsFor`), and a rule naming other is one rule; `Kind` (`game/object.h`) holds the collision rules, actions and timers (each object keeps only its timers' `clocks`), made and checked for each object, then shared by a group's members (`RawObject::kind`, `game_expr::shareKinds`) and by a copy whose rules do not name its piece (`renameOwn`). Hitboxes stay the object's (they can be about its picture). Its looks, palettes, poses and joins are one `LookSet` behind `Object::looks.set`, shared by a copy whose animations do not name its piece (`Renamed::looks`); a look's seconds changed while playing give the object its own set (`Looks::change`): 1,736 to 1,592 bytes. An animation's frames (pictures, times, hitboxes, commands) are one `Frames` held by pointer (`HeldFrames`, `edit` copies a shared one first), shown by pointing at a look's: 1,592 to 1,480 bytes. Still open: group members each hold their own set.
  - [x] **5b. Names as ids (L).** An object's name, group, class, copy and owner state are small ids into one table of names the game holds, read back as text only for the screen, the debug print and errors. **Check:** all tests; `[speed]` not worse.
    Done so far: `Name` (`game/name.h`) is one pointer into the game's `NameStore` (`game_expr::nameStore`, `Game::nameOf`), each text held once; an object's name, class, group, outer groups, copy, owner state and owner scene are names, and so are the map a piece is on, its home map and the path it follows: 1,480 to 1,240 bytes. A picture's colour, font and file are names too: 1,240 to 1,168 bytes. The picture itself is held by pointer (`HeldPicture`, in `game/command.h`), one between an object and its looks, copied only when something changes it (a text's words, a tether's line): 1,008 to 920 bytes. An object's variables are one short list in name order (`Variables`, `game/variables.h`) in place of a `std::map`, in its `Moment` and its `start`: 920 to 872 bytes.
  - [ ] **6. Passes over a store (M, one pass a commit).** Where a frame pass asks every object "do you have this" (animations, object timers, paths, flights, pulls, altitude), it walks that part's store with `each`. The order of passes in `playOneFrame` and `Game::updateObjects` does not change. **Check:** all tests; `[speed]` frame time for all games summed is lower than before the task; `xgetest-long` random keys.
  - [ ] **7. Features leave `Game` (L, a feature a commit).** With holders, cells, marks and the search holding their own parts, the functions in `game/holders.cpp`, `cells.cpp` and `think.cpp` become functions that take the game, as verbs are, and leave the class. **Check:** all tests; `wc -l xge/lib/game/game.h` under 500.
  - [x] **8. The written side.** Dropped (the author): `RawObject` (14,600 bytes) is mostly the file's text, read at load and on a spawn, never every frame, so its size is a fair trade for not reading the file again.
- **Files.** `xge/lib/game/store.h` (new), `game/object.h`, `game/game.*`, `game/*.cpp`, `play/*`, `verbs/*`, `load/build.cpp`; `xge/tests/engine/test_store.cpp` (new); `docs/reference/engine.md`, `docs/agents/play.md`.
- **Done when** what is the same for every object of a kind is held once (step 5a), an object holds only what changes for it while playing, and no object holds a part its file never asked for; `sizeof(Object)` is then under 600 bytes (held by a `static_assert` in the `[size]` test). The author (after seeing where 2.5 KB went: three copies of the `Moment`, about fifteen empty strings, and per-kind data copied into every object) agreed this goal in place of the first one, under 1,200.

### CR4. Input, update, render

- **Why.** `Engine::step` polls the window, plays held keys, runs the computer, steps the game and asks for sounds, and `loop` then renders. The author wants the three stages spelled out so another (post-processing) can go between.
- **Steps.**
  - [x] **1. `Engine::run` (M).** Done: `run()` is `input()`, `update(seconds)`, `render()`; the computer's presses went into `update`, which they need the step's length for, as held keys do. Was: `run()` is the loop: `input()`, `update(seconds)`, `render()`, in those words. `input` is everything that comes from outside (window events, pads, the demo's keys, the computer's presses); `update` is the game's step and what follows from it (sounds asked for, things kept); `render` draws. `loop` and `step` go, with their callers in `xge/cli`, `xge/gui` and the tests' `played.h`. **Check:** all tests; `xgetest "[window]"` under `xvfb-run`.
  - [x] **2. The game's own step the same way (S).** Done: `Game::updateObjects` is `settle`, `playOneFrame`, `settle`, `showJoins`, `placeByResizedTexts`; `playOneFrame` (`play/frame.cpp`) is named stages only (`advanceAnimations`, `meetEdgesWithoutSweep`, `stickAtEdges`, `rememberWhereAll` came out of it); speed unchanged. Was: `Game::updateObjects` reads as its stages in order (timers, the frame, conditions, spawns settled), each a named call, with no work between the calls. **Check:** all tests; `[speed]`.
- **Files.** `xge/lib/engine/engine.*`, `xge/cli/source/`, `xge/gui/source/`, `xge/tests/helpers/played.h`; `docs/reference/engine.md`.

### CR5. The reader in one pass

- **Why.** `findChild` (`load/reading.cpp`) starts from the first child every time and makes a node object per sibling passed; `readObjectHere` calls it about 40 times. The group reader repeats the object reader tag by tag. And the reader re-checks the file's shape because a file that names no schema loads unchecked.
- **Measured first.** Searching tags is a few percent of Pac-Man's load (777 objects); the rest was the objects handled one by one. Done before the steps: a map's members are copies of its first, scene objects are moved rather than copied, collision partners are asked once per group of alike objects, and live numbers go into `calc` only when a text reads them and are kept up only then. Pac-Man's `--check` went from 3.69G to 2.53G instructions, and the play benchmark (`[speed]`'s four games under callgrind) from 40.6G to 18.0G.
- **Steps.**
  - [x] **1. Every game is checked (S).** Done: one naming none is checked against the `xgedef.xsd` beside or above it, else the working folder's; 70 test cases whose snippets the schema turns away were put in schema order or now expect the schema's message (the loader's friendlier ones, like "`<show set=...>` is gone", are no longer reached). Was: a file that names no schema is checked against the engine's own `xgedef.xsd`. **Check:** a test loads a wrong file with no schema named and gets the schema's message; `test_schema_checker.cpp`.
  - [x] **2. The reader trusts the shape (M).** Done in seven commits: `fail(` in `load/xml.cpp` 251 to 156, `requireAttribute(` 20 to 3 (the keys file, read unchecked, keeps its own). Was: remove what the schema already refuses: unknown tags, missing required children, wrong enumeration words, numbers that are not numbers (the bare `std::stof` and `std::stoi` in `readWindow` and elsewhere become safe). Keep every check a schema cannot make. One group of readers a commit. **Check:** `test_schema_checker.cpp`'s sweep still refuses every damaged game; the count of `fail(` in `load/xml.cpp` goes in the commit.
  - [x] **3. An object's simple parts as a table (M).** Done: `simpleParts` in `load/xml.cpp`, 27 parts, read alike by the object and group readers (members give none of them); `findChild(` 146 to 120; Pac-Man's `--check` barely moved (2.091G to 2.088G), as the measure foretold. `hasX` as `std::optional` waits for CR6: `data.xsl` writes every `hasX`, and goes if the author says yes. Was: rows of tag, where it lands and how it is read, used by the object, group and member readers alike, each walking its children once in order. `hasX` beside a value becomes `std::optional`. **Check:** `xgetest-generated`; `[speed]` load time falls.
  - [x] **4. No search left (M).** Done: `Children` (`load/reading.h`) walks an element once; the object, state, scene, map and top-level readers and `<collisions>` use it, and `findChild(` in `load/xml.cpp` went from 120 to 74, each left asking one or a few optional children of a small element (a sprite's shape, a cell, the window, a style, a sheet). `copyElement`: the file is copied into elements once; layOut, the kit and spawnedBy share them (`elementOf`). Pac-Man's `--check` 2.088G to 2.069G. Was: the other readers (states, conditions, sprites, holders, maps) walk their children once. `findChild` stays only where one optional child is asked of a small element. Each `copyElement` in `load/xml.cpp` is shown to be needed or removed. **Check:** a count of `findChild(` before and after in the commit; `[speed]`.
- **Files.** `xge/lib/load/xml.cpp`, `load/reading.*`, `game/object.h`; `docs/agents/play.md` "Loading".

### CR6. One reader, not two

- [x] **1. The author: yes or no.** **Yes, for `engine-cpp`** (the author): it was only what was left once `windows-cpp` became stand-alone, so it can work the way the engine does. `windows-cpp` (A5) stays small and hand-written-looking, untouched. Was: `engine-cpp` would write the game's tree (after `from=` is worked out) as a C++ table, and the generated program would run `load/xml.cpp` over it through a small `XmlNode`. `data.xsl` (4,138 lines), `merge.xsl` (768), `modules/game_parts.h` (588) and the describer in `test_generated_games.cpp` (551) go; the generator leaves [changing the language](agents/readme.md#changing-the-language). It stays XSLT, with no XML library in the program. The cost: the program carries the reader and reads its table at start. If no, skip to CR7 and A1 step 4 stays parked.
- [x] **2. The tree and its node (M).** Done: `xml/tree.h` (`TreeElement`, `viewOfTree`); `load/xml.cpp` reads a checked `<game>` element (`readGame(root)`) and `load/file.cpp` loads and checks a file; `test_tree.cpp` reads Pong, Pac-Man, Klondike and Mario Bros. from trees. Was: A static tree type and an `XmlNode` over it (`xge/lib/xml/`), with a test that reads Pong from one.
- [x] **3. The stylesheet (M).** Done: `tree.xsl` writes each element as a static `TreeElement` array; the program includes the reader, `load/merge.cpp` and `xml/tree.cpp`, and nothing else changed. Was: `games/generators/engine-cpp/tree.xsl` writes it; `xgetest-generated` plays each generated game beside the engine's as it does now.
- [x] **4. The second reader goes (S).** Done: `data.xsl`, `merge.xsl`, `game_parts.h`, the describer and `<module>` in `generate.cpp` gone. Was: Delete the four files and their mentions in `engine.xml`, `assets.cmake`, the agent notes and [generator](designs/40-generator.md).
- [x] **5. `hasX` beside a value becomes `std::optional` (S).** Done: an object's acceleration, heading, pivot, drag, friction, mass, surface drag and grip, light and grip; a picture's scale, svg part and line thickness; a path's start and a holder's picked sends. `RawObject` stays 14,608 bytes. `hasAnimation` and `hasAltitude` stay: each says a whole part was written, and its values keep their defaults. Was: from CR5 step 3, once `data.xsl` no longer writes them.

### CR7. A verb owns its data

- **Why.** `RawCommand` is 3,328 bytes of 61 shared fields. This is A1 step 4, and it replaces that step.
- **Needs** CR6.
- **Steps.**
  - [x] **1. The shape, on three verbs (M).** Done; the author: "good for now", though still busy and its names not easy to follow. Built: `RawOwn` (`std::variant<std::monostate, RawBounce, RawHop, RawSpawn>`) in `RawCommand::own`, each with `eachValue` for `forEachValue`; a hunt's hop is a `RawHop` (`byHops` gone). `RawCommand` 3,328 to 4,112 bytes until step 3; Pac-Man's `--check` 1.96G to 1.86G. Was: `RawCommand` becomes the verb, its line, and the verb's own data (a `std::variant` of small structs beside each verb's `Cmd...` struct, or one struct serving as both where nothing is worked out). `<bounce>`, `<hop>` and `<spawn>` first. **The author** reads those three files before the rest move.
  - [x] **2. The rest, ten verbs a commit (L in all).** Done, with the work each verb does moved into its file and typed `run`/`print`/`check` (the author: "do them all like that"); `hold` and `point` join the table for held keys and the mouse. Was: A field two verbs used for different things (`seconds`, `amount`, `distance`) becomes each verb's own, named for what it is there. **Check after each commit:** `xgetest "[verbs]"`, `xgetest-generated`.
  - [x] **3. The bag goes (S).** Done: `RawCommand` 3,328 to 2,496 bytes; Pac-Man's `--check` 1.86G to 1.72G; load errors already name the line (`onLineOf`). Was: `RawCommand` has no shared value fields; CR1's `forEachValue` asks the verb. Load errors name the command's own line. **Check:** `[size]`.
- **Files.** `xge/lib/game/command.h`, `xge/lib/verbs/*`, `load/reading.*`; `docs/designs/14-verbs.md`.

### CR8. No special rules

- **Why.** Places where one mechanic was given its own branch because it was quicker that day.
- **Steps.**
  - [x] **1. The list (S).** Done: in [engine-structure](designs/16-engine-structure.md#special-rules-plan-cr8); the author: "sounds good". Was: Find them: `grep -rn 'verb == "' xge/lib` (18: `send`, `order`, `trigger`, `set`, `spawn`, `shift` and others), tests of a kind or class by name in `play/` and `game/`, flags set by one mechanic and read by the frame (`goThrough`, `placed`, `hasAim`), and early returns in the frame for one feature. For each: what it is for, and the proposal, either a property on `Verb` or on a part that anything could have, or removal. The list goes in [engine-structure](designs/16-engine-structure.md). **The author** reads it.
  - [x] **2. One a commit (M in all),** as agreed. Done: verb-name checks 14 to 2. **Check:** the grep's count falls to what the list says stays.
  - [x] **3. Things built twice (M).** Done: in [engine-structure](designs/16-engine-structure.md#special-rules-plan-cr8). Was: Read `play/verbs.*`, `play/executor.*`, `play/geometry.*` and `load/reading.*` for helpers that do one job under two names, and layers that only pass a call on. Each is merged or removed. **Check:** the list of what merged is in the commit messages.
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
  - [ ] **4. Moved to [CR7](#cr7-a-verb-owns-its-data).** Was: the author: whether each verb owns its data (L if yes). `Command` would become `{ const Verb* verb; data }` and `RawCommand`'s shared fields the verb's own, which is where "`seconds` means a move's wait" ends. It changed `data.xsl` and the describer heavily, so it waited for CR6. Ask before starting. With it, each command carries its own line, and the load's name checks can say the command's line rather than its holder's (B7, done; the author wants that).
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

Goals the author has set, beyond the tasks above. None is scheduled; [the C++ review](#the-c-review) is being done first and has its own section.

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

- Sanitizers: the whole of `xgetest` is clean under `-DXGE_SANITIZE=address` (address and undefined behaviour), as are `xgetest-long`'s random keys and damaged files; the board-game search on its own thread is clean under `-DXGE_SANITIZE=thread`.
  - The damaged files found one fault: a line out at 1e9 or a text of a huge size asked for hundreds of gigabytes.
  - Pictures and texts are now at most 4096 pixels a side (`refusePictureSize`).
  - Generated programs and `xgegui` have not been run under one.
- MSVC is checked by hand, not automatically; older versions are untested.
- A default build (no options) and one with only `-DXGE_WITH_OPENGL=ON` have no automatic check ([build-and-layout](designs/51-build-and-layout.md)).
- Agents build in a Linux sandbox and cannot watch or listen, so how a game feels and sounds is unchecked.
