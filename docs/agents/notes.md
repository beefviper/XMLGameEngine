# Notes for AI coding agents

Gotchas, untested areas and next steps. The engine itself is in [../readme.md](../readme.md); reasons and rejected options are in [../designs/00-designs.md](../designs/00-designs.md). Rules and layout are in [../../AGENTS.md](../../AGENTS.md).

## State of the project (2026-10-04)

- 15 games in `games/` (list and keys in [../readme.md](../readme.md)), 37 test files. The newest work (sound, `<deflect>`, timers, facing, jumps, looks, key sets, SVG sprites, Berserk) landed on branch `working` ahead of `master`; check `git branch -r` and `git log` for the freshest branch before starting.
- The author builds and runs on Windows (Visual Studio, vcpkg); the `output/` layout with `libraries/` and its manifest was confirmed there on 2026-10-03. Agents build in a Linux sandbox, often without some libraries (no Qt, no window or sound backends), and cannot watch or listen. Berserk's speeds, sizes and waits were chosen without seeing it played: tune them first.

## Gotchas (easy to get wrong)

**Engine**
- Everything numeric is evaluated **once at load**, except a timer's `<every>`/`<after>` and positions that use a text's or image's measured size. A condition's `<atleast>`/`<atmost>`/`<remaining>` and an `<inc>`/`<dec>` amount cannot follow a variable. `<random>` is drawn once, so `<reset />` repeats it ([design 02](../designs/02-values-variables-and-names.md)).
- `<reset object="x" />` resets **every variable** of x, not just its position. A rule's bare `<reset />` puts only its own object back. Never put `<reset object="player" />` in someone else's rule.
- No handle system and no `Value` variant exist; variables are `float`. `collisionData.basic` is the old name for the list of object-against-object rules (rename pending).
- `ShapeKind::Line` means any picture the engine draws itself (`<line>`, `<bitmap>`, `<svg>`): all become a `Bitmap`, params `{"line", w, h}`, built in one block, `game_expr::buildSpriteParams` (read, draw, flip, keep a `Turnable`). Keep new picture kinds on that path; never rotate or draw in a backend ([design 07](../designs/07-pictures-and-text.md)).
- Looks are `Object::looks` (`showLook`, `showLookNamed`); a rule's `sprite=` reads `lookName()`. Animation counts frames (`Object::advanceAnimation`, top of `Game::updateObjects`); a turning object redraws in `Object::showHeading` on a whole-degree change.
- Timers count frames (`Game::framesFor`); a repeating timer re-evaluates its interval the frame after it goes off (`framesLeft = -1`); object timers count only while `isShown`; state timers live in `Game::stateTimers` by name (the stack holds copies). Expressions read object variables live: anything that changes a variable goes through `Game::refreshBoundTexts`.
- An object mid-`<jump>` (`Object::isAirborne()`) is in no collision pair and ignored by `unless=`; it lands in `Game::applyHops`.
- `Game` never touches audio: `<play>` queues a name (`requestSound`, once per name per frame) and `Engine::step()` plays the queue. `Engine(game, window)` with no audio is `NullAudio`. Tests that build a `Game` without an `Engine` must call `setCurrentState(0)` and measure sizes (`measureShapeSize`) or nothing is shown and every size is 0.
- `<fire>` centers the projectile on the shooter's top edge (or the middle of its facing side). Depth Charge and Astrosmash tests expect that.
- Frogger and Space Race are `<group>`s of lanes read as one object per member (`logrow3.2`, `pads.1`); tests address members by those names. `tests/invaders_fixture.h` is a frozen copy of the first Space Invaders for grid/swept/lockstep tests; Space Invaders itself is tested in `test_bitmap_sprites.cpp`.
- Add a test to `tests/test_collision_geometry.cpp` before touching `CollisionDetector::circleRectangle`. `tests/test_xml_format.cpp` pins what both validators must reject; `xsd_lite` covers only the XSD subset the schema uses (named types may contain themselves, as a `<formula>`'s operands do; groups may not). `RawValue` is a tree now (`Kind::Equation`, `Kind::Formula`, `operations`); the arithmetic words live in one table, `operationShape` in `command.cpp`, and a new operation is a row there plus one case in `game_expr::evaluateOperation` and the schema types. `game_expr::finishLoading()` (called at the end of `Game`'s constructor) is what makes a divisor of 0 a load error before it and a warning after it, and each `Answer` carries a `late` flag for values that read 0 only because they are not final while loading (an object variable, an unmeasured size); keep both when changing `evaluateOperation`. `tests/test_equations.cpp` covers it, and Pong's and Breakout's titles are written with the tags so the size-dependent-position tests exercise them.
- The schema checks shape, not meaning (commands in the wrong place, unknown key names, unowned variables load and silently do nothing). The loader checks names a command uses once everything is built.

**Backends and build**
- Never destroy a library's window after creating the next: raylib and GLFW allow one (`Engine::replaceWindow` destroys first). A front end that pauses the game but keeps a library's window calls `Engine::pump()`.
- raylib has no event queue: `EndDrawing()` reads the keyboard itself. `RaylibWindow::pollEvents()` must not call `PollInputEvents()` (it lost every key press) and reports changes by comparing `IsKeyDown`. Escape is not the exit key (`SetExitKey(KEY_NULL)`). The author's raylib 6.0 is built with `SUPPORT_CUSTOM_FRAME_CONTROL`, so `RaylibWindow::display()` swaps, polls and waits itself (detected once: `GetFrameTime()` still 0 after `EndDrawing()`). A texture drawn into a render texture must stay alive until `EndTextureMode()`.
- SDL is shared by `SDL2Window` and `SDL2Audio`: each starts/stops only its own subsystem, the last out calls `SDL_Quit()`. Never put a bare `SDL_Quit()` back.
- Building SFML and raylib both static from source fails to link on Linux (duplicate `stbi_*`, miniaudio); use shared builds or vcpkg. A CMake target named `m` collides with plutovg's `-lm`. More in [design 11](../designs/11-backends-build-and-layout.md).
- A text falls back to the built-in 8x8 font when `assets/tuffy.ttf` is not found; programs make the folder holding `games/` and `assets/` the working directory first (`data_folder.cpp`).

**xgegui**
- No OpenGL through Qt: `GameView` is an ordinary widget; do not bring back a `QOpenGLWidget` without a guard around every library call ([design 12](../designs/12-front-ends.md)).
- Anything that can run while `GameSession` swaps the game or window (a focus signal, a timer) must check its `changing` guard: the engine has no window for part of a swap.
- Do not style `QSpinBox` borders in the style sheet (arrows vanish). The inspector calls an engine-drawn sprite "drawn" (`ShapeKind::Line`).

## Next steps (not started)

1. Chasing/aiming verb (Berserk's robots, invaders aimed at the player); only the front invader of a column firing ([08](../designs/08-timers-and-enemy-behavior.md), [06](../designs/06-motion-and-verbs.md)).
2. Arcing jump with grounded/airborne and an air-control ladder ([06](../designs/06-motion-and-verbs.md)); `<goto state>` so Kaboom's waves and Berserk's rooms stop growing the stack.
3. Make the loader refuse unknown button names and misplaced commands and warn about unowned variables (author's decision; [design 10](../designs/10-games-as-tests.md)).
4. Sort `lib/` into folders; rename `collisionData.basic` ([design 11](../designs/11-backends-build-and-layout.md)).
5. Sound for the eight silent games (Breakout, Frogger, Space Race, Freeway, Depth Charge, Astrosmash, Lunar Lander, Asteroids); looping music; an envelope. Breakout could use `<deflect>`; Pong serve at a set speed and random angle; speed-up per hit.
6. Use the rest of the Space Invaders 2 sheet (banking ship and hit flash as looks, explosions as a released pool, the saucer on a timer); looks on animated objects; a palette; per-frame intervals; animation that runs once ([07](../designs/07-pictures-and-text.md)).
7. Asteroids: wrapping and expiring shots, a safe respawn, a saucer, more waves. Lunar Lander could turn with `<heading>` and score by fuel left.
8. Groups: nested groups, a bare `<x>` in a member, evenly spaced members ([03](../designs/03-objects-groups-and-storage.md)). Moving the other games' arithmetic to `<equation>`/`<formula>` if wanted, and the operations those two lack (`min`, `max`, `negate`, `abs`, `clamp`; a `<random>` step in an equation): [01](../designs/01-vision-and-format.md#arithmetic-in-text-and-as-tags), and [14](../designs/14-vocabulary-map.md) maps the vocabulary.

## Tools in this folder

`scan_export.py` (Claude chat export) and `scan_chatgpt_export.py` (ChatGPT export, many `conversations-NNN.json`, keys `file:position`) rank and dump conversations by keyword so design material can be mined. `sources.md` says how the exports were used.

## Privacy rule

This repository is public. Nothing personal from a chat export goes in it: no health, identity, contact or business details, and no names beyond the `beefviper` author handle already in the code headers. Do not list conversation titles.
