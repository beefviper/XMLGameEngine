# Notes for future Claude sessions

Working notes for whoever picks this project up next. This folder is Claude's own scratch space in the repo; everything the project itself relies on is in `docs/readme.md` and `docs/design/`.

## What exists

- `docs/readme.md`: description of the engine as it is now. Written from the source on 2026-09-29. Update it whenever a verb, attribute, or file changes.
- `docs/design/00-designs.md`: index of design topics with options and status. One file per topic, numbered `NN-name.md`. Keep the table and the file list in step.
- `docs/claude/sources.md`: which conversations fed which entry.
- `docs/claude/scan_export.py`: rank and dump conversations from a claude.ai export.

## Conventions the author asked for

- C++ files start with a header comment: filename, "XML Game Engine", "author: beefviper", and the date, in the form used by the existing files.
- Files end with exactly one trailing newline.
- Prefers direct answers and clear pushback over hedging.

## Facts that are easy to get wrong

- The root `readme.md` is out of date: it says collisions, scoring and win condition are missing, and lists only Xerces/exprtk/SFML. Not edited yet; the author has not asked.
- The repo has three branches: `master`, `rewrite`, `claude` (working branch here). The active engine is on the `claude` branch.
- The sample games still use function-call syntax in attributes ([design 03](../design/03-expression-syntax.md)); the author has said that will go, so do not treat it as settled.
- No handle system, no `Value` variant, and no swept collision exist yet, despite being decided or discussed ([07](../design/07-object-variables-and-references.md), [08](../design/08-entity-storage-and-handles.md), [11](../design/11-collision-detection-and-response.md)).
- `CollisionDetector::circleRectangle` has suspicious edge tests; do not "fix" it without adding a test in `tests/test_collision_geometry.cpp` first.
- The XSD limits: `variable/@value` is an integer, `condition/@value` is 0 to 255.
- Backends are only selectable in C++ (constructor arguments), not on the command line.

## Suggested next steps (not started)

1. Refresh the root `readme.md` (or point it at `docs/readme.md`).
2. Add tests that pin the current behavior of `circleRectangle` before touching it.
3. Decide design 03/04 (function syntax and arithmetic) since it changes every game file.
4. Give every `grid()` cell a unique name so single objects can be addressed.
5. Add a jump verb (design 13) as the first test of whether the vocabulary approach extends.

## Privacy rule for these docs

This repository is public. Nothing personal from the chat export goes in it: no health, identity, contact or business details, and no names beyond the `beefviper` author handle already in the code headers. Conversation titles are listed only for design-relevant chats.
