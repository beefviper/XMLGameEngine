# Notes for AI coding agents

Gotchas that are easy to get wrong. How the engine works is [../readme.md](../readme.md); why, and the backlog, is [../designs/00-designs.md](../designs/00-designs.md). Rules and layout: [../../AGENTS.md](../../AGENTS.md).

## State

- 23 games in `games/`, one test file per area in `tests/`. Newer work lands on `working` ahead of `master`: check `git branch -r` and `git log` first.
- The author builds on Windows (Visual Studio, vcpkg). Agents usually build on Linux, often without Qt or window and sound backends, and cannot watch or listen.

## Engine

- Numbers are evaluated **once at load**, except a timer's `<every>`/`<after>` and positions using a measured text or image size. A condition's `<atleast>`/`<atmost>`/`<remaining>` and an `<inc>`/`<dec>` amount cannot follow a variable. `<random>` is drawn once, so `<reset />` repeats it ([values-and-names](../designs/04-values-and-names.md)).
- `<reset object="x" />` resets **every variable** of x. A rule's bare `<reset />` resets only its own object. Never put `<reset object="player" />` in someone else's rule; keep score, lives and clock on an object no state shows (Pitfall's `status`).
- Variables are `float`; there is no handle system or `Value` variant. `collisionData.basic` is the old name of the object-against-object rule list.
- Every engine-drawn picture (`<line>`, `<bitmap>`, `<svg>`) is `ShapeKind::Line`, built in `game_expr::buildSpriteParams`. Add new kinds there; never rotate or draw in a backend ([pictures](../designs/30-pictures.md)).
- Timers count frames (`Game::framesFor`); object timers run only while `isShown`; state timers live in `Game::stateTimers`. Anything that changes a variable goes through `Game::refreshBoundTexts`.
- An object mid-`<jump>` (`isAirborne()`) is in no collision pair and ignored by `unless=`. `Object::grounded` is set by `CommandExecutor::land` and cleared in `Game::applyHops`; a resting object stays touching its platform only because its pull moves it every frame, so `<stop />` ends that ([platforming](../designs/23-platforming.md)).
- Edge rules: `sprite=`/`unless=` are an `EdgeGuard`; `class=` or `object=` on an edge rule is a load error.
- Touches in one moment run in file order and a pair where neither moves is skipped: a `<land />` or `<stop />` that runs first can hide a hazard met at the same moment (Pitfall's jaws come before the crocodiles). A variable named like an exprtk function (`floor`, `min`) stops the load.
- `Game` never touches audio: `<play>` queues a name and `Engine::step()` plays it. Tests that build a `Game` without an `Engine` must call `setCurrentState(0)` and `measureShapeSize`, or nothing is shown and every size is 0.
- Frogger and Space Race are groups of lanes addressed as `logrow3.2`, `pads.1`. `tests/invaders_fixture.h` is a frozen copy of the first Space Invaders.
- Before touching `CollisionDetector::circleRectangle`, add a test to `test_collision_geometry.cpp`. `test_xml_format.cpp` pins what both validators must reject; `xsd_lite` covers only the XSD subset the schema uses.
- Arithmetic: a new operation is a row in `operationShape` (`command.cpp`), a case in `game_expr::evaluateOperation` and the schema types. A divisor of 0 is a load error before `finishLoading()` and a warning after; each `Answer` has a `late` flag for values not final while loading. Keep both.
- Load checks: `ruleFor` in `game_expr.cpp` mirrors the dispatchers in `command_executor.cpp`; a new verb needs a line in both. Use `didYouMean` and `listOf` (`spelling.cpp`) in any name error.
- The schema checks shape, not meaning; the loader checks names once everything is built.
- Text falls back to the built-in 8x8 font when `assets/tuffy.ttf` is missing; programs set the folder holding `games/` and `assets/` as working directory (`data_folder.cpp`).

## Backends and build

- Never destroy a library's window after creating the next (raylib, GLFW): `Engine::replaceWindow` destroys first. A front end that pauses but keeps a window calls `Engine::pump()`.
- raylib has no event queue: `RaylibWindow::pollEvents()` must not call `PollInputEvents()`; Escape is not its exit key; with `SUPPORT_CUSTOM_FRAME_CONTROL` `display()` swaps, polls and waits itself. A texture drawn into a render texture must live until `EndTextureMode()`.
- SDL is shared by `SDL2Window` and `SDL2Audio`: each starts only its subsystem, the last out calls `SDL_Quit()`.
- Backend `.cpp` files compile only behind `XGE_WITH_<NAME>`; factories expose `available()`, `availableBackends()`, `defaultBackend()`, `name()`. Build with no options as well as with tests when touching factories, `cli.cpp` or `session_options.cpp`.
- Windows headers define `near`, `far` and `small`: never use them as names (MSVC fails far from the cause; `-Dnear= -Dfar=` on a syntax-only compile catches it).
- SFML and raylib both static from source fail to link on Linux (duplicate `stbi_*`, miniaudio); a CMake target named `m` collides with plutovg's `-lm` ([build-and-layout](../designs/51-build-and-layout.md)).
- `-Wdouble-promotion` is GCC only on purpose.

## Generator (`xgecli --generate windows-cpp`)

Reasons and details: [generator](../designs/40-generator.md), [structure](../designs/41-generator-structure.md), [rules](../designs/42-generator-rules.md).

- The generated rules are a simpler copy of the engine's: a game that plays differently from the engine is a bug in `main.xsl` or `functions.xml`. `check.xsl` lists what is accepted (`$supported`); a tag missing there is refused, so widen it and the writing together.
- Keep in step: `modules/pictures.h` with `rasterizeRows`/`rasterizeLines`/`rasterizeTurned`/`turnBitmap` (`lib/source/bitmap.cpp`); `modules/sound.cpp` with `lib/source/sound.cpp`; `groups.xsl` with `readGroup`/`layOutCells`; `tables.xml` with `keycode.cpp`, `window_sfml.cpp`, `color.cpp`.
- A new module goes in `data_generators` in `assets.cmake`. `generate.cpp` runs a target's `prepare.xsl` first and makes the output folder the working directory (libxslt resolves `exsl:document` hrefs as URIs).
- `tests/test_generate.cpp` checks the text written, not that it builds. Under Xvfb use a screen at least 1400 wide or the right paddle is hidden.
- Galaxian: `Game::applyPaths` runs after timers, before acceleration, and a path ignores pushes on the way. Anything ending a path clears `Object::followPath`. Its dive relies on `<wrap />` taking exactly window height plus the object's height.

## xgegui

- No OpenGL through Qt: do not bring back a `QOpenGLWidget` without a guard around every library call ([gui](../designs/53-gui.md)).
- Anything running while `GameSession` swaps game or window must check its `changing` guard.
- Do not style `QSpinBox` borders (arrows vanish).

## Tools and privacy

`scan_export.py` (Claude chat export) and `scan_chatgpt_export.py` (ChatGPT export) rank and dump conversations by keyword. The first design write-ups were mined from the author's exports; the JSON stays in the author's folder.

This repository is public. Nothing personal from a chat export goes in it: no health, identity, contact or business details, no conversation titles, and no names beyond the `beefviper` handle in the code headers.
