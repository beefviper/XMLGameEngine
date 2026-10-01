# Notes for AI coding agents

Working notes for any AI coding agent (or new contributor) picking this project up. Everything the project itself relies on is in [../readme.md](../readme.md) and [../designs/00-designs.md](../designs/00-designs.md); this folder is for agent-facing bookkeeping.

The first version of these docs (the design write-ups, `sources.md` and the scan script) was written by Claude (Sonnet 5.5) on 2026-09-29, from the author's exported chat history plus a read of the source on the `claude` branch. Frogger (`games/frogger.xml`), the verbs it needed and design write-up 21 were added by Claude (Sonnet 5.5) on 2026-09-30, as was Space Race (`games/spacerace.xml`, first to two points, no new verbs). Kaboom (`games/kaboom.xml`, design write-up 30, no new verbs) was added by Claude (Sonnet 5.5) on 2026-09-30 as a game to have fun with, and to try `<random>` inside a `<group>`; it was first a looser game named Gem Catcher, and was renamed and rewritten to follow the arcade rules (bombs only, waves, a miss sets off the wave and costs a bucket) on the same day. Freeway, Depth Charge and Astrosmash (design write-ups 31 to 33, no new verbs) were added the same day, chosen because they needed nothing the vocabulary lacked. Lunar Lander (`games/lunarlander.xml`, design write-up 34) was added by Claude (Sonnet 5.5) on 2026-09-30 at the author's request, with the `<line>` sprite shape, `<type>pixel</type>` collisions, `<acceleration>`, `<accelerate>`, `<stop />` and the `slower`/`faster` filters; the window backends for lines were only compile-checked. A second export (ChatGPT) was mined on 2026-09-30: it added design docs 22 to 28 and a section headed second batch in most earlier docs; only paraphrased ideas were kept, and titles of chats that did not start as game discussions were made generic.

## What exists

- `docs/readme.md`: the engine as it works now. Update it whenever a verb, attribute or file changes.
- `docs/designs/00-designs.md`: index of design topics with options and status. One file per topic, numbered `NN-name.md`. Keep the table and the files in step.
- `docs/agents/sources.md`: which conversations fed which design entry.
- `docs/agents/scan_export.py`: ranks and dumps conversations from a Claude chat export.
- `docs/agents/scan_chatgpt_export.py`: the same for a ChatGPT export (many `conversations-NNN.json` files; keys are `file:position`).

## Conventions the author asked for

- C++ files start with a header comment: filename, "XML Game Engine", "author: beefviper", and the date, in the form used by the existing files.
- Files end with exactly one trailing newline.
- Direct answers and clear pushback are preferred over hedging.

## Facts that are easy to get wrong

- Key conventions for `games/`: Space starts, pauses and plays again (never Enter); player one is W/A/S/D. Where Space fires, pause is P or Escape. Keep new games to this.

- The root `readme.md` is a short overview (games, format, build, status); the detailed description is `docs/readme.md`. Update both when a verb, a game or a dependency changes.
- The repo has three branches: `master`, `rewrite`, `claude`. The active engine described in these docs is on `claude`.
- Function-call syntax is gone: attributes only name or pick things, everything else is element content ([design 03](../designs/03-expression-syntax.md)). Arithmetic in values is still exprtk text.
- No handle system and no `Value` variant exist yet, despite being decided or discussed ([07](../designs/07-object-variables-and-references.md), [08](../designs/08-entity-storage-and-handles.md)). Swept collision is built ([11](../designs/11-collision-detection-and-response.md)).
- `CollisionDetector::circleRectangle` was rewritten (nearest point on the rectangle, and the closest side for a centre inside it); `tests/test_collision_geometry.cpp` pins it. Add a test there before changing it.
- The XSD checks structure, not expression text; `xsd_lite` (used by the three non-Xerces backends) covers only the XSD subset the schema uses, and `tests/test_xml_format.cpp` pins what both validators must reject.
- Files in the repo use CRLF line endings on disk (`.gitattributes` stores LF and checks out CRLF); keep them when editing. A few newer files (design 22 to 28, `scan_chatgpt_export.py`, `sources.md`) are LF on disk; git normalizes them on commit.
- Every cell of a `<grid>` has its own name (`aliens.3.2`, column then row from 1), so a single cell can be addressed; the grid's name still means the whole grid.
- The engine's internal list of unfiltered and filtered object-against-object collision rules is still called `basic` (`collisionData.basic`), a leftover of the old `basic="basic"` spelling; the XML no longer has it.
- Backends are selected in C++ by constructor arguments and on the command line by `-w`/`--window` and `-x`/`--xml` ([37](../designs/37-command-line.md)); `XGEGUI` has no way to choose yet.
- The engine is a library (`XGELIB`, in `lib/`); `cli/` is XGECLI (the command line program) and `gui/` is XGEGUI (a stub); each has its own `source/` and `include/`. Targets: `XGELIB`, `XGECLI`, `XGEGUI`, `XGETEST` (tests) and `XGEDATA` (copies games and assets). New engine source files go in `ENGINE_SOURCES` in the top-level `CMakeLists.txt`; tests link the library, so they no longer need that list. Static by default, `XGE_BUILD_SHARED` for shared ([35](../designs/35-library-and-front-ends.md)).
- Text falls back to a font stored in the program (`lib/source/builtin_font.cpp`, 8x8, public domain) when `assets/tuffy.ttf` cannot be found; the assets are looked for relative to the working directory only, so run from the build directory or the fallback appears ([36](../designs/36-builtin-font.md)).
- A new game file has to be added to `data_xml` in `scripts/cmake/assets.cmake`, or the build does not copy it into the build directory; a new test file has to be added to the list in `scripts/cmake/tests.cmake`.
- Frogger's board is built from many named rectangles, drawn in file order (so scenery first, the frog last); its lanes, pads, homes and hedges are `<group>`s, read as one object per member (`logrow3.2`, `pads.1`), and Space Race's debris lanes the same way. The tests address members by those names. See [21](../designs/21-frogger.md) and [29](../designs/29-groups.md).
- On Windows the repo's `.gitattributes` converts line endings on commit; git prints LF-to-CRLF warnings, which are harmless.

## Suggested next steps (not started)

1. Decide design 04 (arithmetic as text vs elements); design 03 is done.
2. Add an arcing jump (gravity now exists: an object's `<acceleration>`, see [design 34](../designs/34-lunar-lander.md); what is missing is grounded-versus-airborne and a jump impulse) ([design 13](../designs/13-verb-vocabulary.md), and the air-control refinement in [design 23](../designs/23-jump-and-air-control.md)) as the next test of whether the vocabulary approach extends. `hop` (Frogger) was the first: one instant step per press.
3. The ideas listed at the end of [design 21](../designs/21-frogger.md): an amount for `inc`/`dec`, show and hide verbs, and per-row velocity in `<grid>` (the built `<group>` tag, [design 29](../designs/29-groups.md), covers lanes; its open points are nested groups, a bare `<x>` in a member and evenly spaced members).
4. Rename `collisionData.basic` (and the `basic=` label in `printGame()`) to something that says what it is.
5. Sort out the file structure: `lib/source/` and `lib/include/` are flat and growing (23 files each). Move them into folders by responsibility (parse and evaluate, engine loop, collision, window backends, XML backends); the Visual Studio filters are meant to be built from the directories, so this is only about the layout on disk. Not started; design 16 has the history of the earlier, over-layered attempt on the `rewrite` branch and why it was flattened.
6. Watch Lunar Lander in each window backend (SFML, raylib, SDL2) once; only the tests have played it so far. Rotation and scoring by fuel left are the obvious next steps for it.
7. Designs 22 to 28 are ideas from a second batch of conversations, not plans; nothing in them is built.

## Privacy rule for these docs

This repository is public. Nothing personal from a chat export goes in it: no health, identity, contact or business details, and no names beyond the `beefviper` author handle already in the code headers. Conversation titles are listed only for design-relevant chats.
