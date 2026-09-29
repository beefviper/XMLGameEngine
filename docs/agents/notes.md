# Notes for AI coding agents

Working notes for any AI coding agent (or new contributor) picking this project up. Everything the project itself relies on is in [../readme.md](../readme.md) and [../designs/00-designs.md](../designs/00-designs.md); this folder is for agent-facing bookkeeping.

The first version of these docs (the design write-ups, `sources.md` and the scan script) was written by Claude (Sonnet 5.5) on 2026-09-29, from the author's exported chat history plus a read of the source on the `claude` branch.

## What exists

- `docs/readme.md`: the engine as it works now. Update it whenever a verb, attribute or file changes.
- `docs/designs/00-designs.md`: index of design topics with options and status. One file per topic, numbered `NN-name.md`. Keep the table and the files in step.
- `docs/agents/sources.md`: which conversations fed which design entry.
- `docs/agents/scan_export.py`: ranks and dumps conversations from a chat export.

## Conventions the author asked for

- C++ files start with a header comment: filename, "XML Game Engine", "author: beefviper", and the date, in the form used by the existing files.
- Files end with exactly one trailing newline.
- Direct answers and clear pushback are preferred over hedging.

## Facts that are easy to get wrong

- The root `readme.md` is out of date: it says collisions, scoring and win condition are missing, and lists only Xerces, exprtk and SFML.
- The repo has three branches: `master`, `rewrite`, `claude`. The active engine described in these docs is on `claude`.
- The sample games still use function-call syntax in attributes ([design 03](../designs/03-expression-syntax.md)); the author has said that will go, so do not treat it as settled.
- No handle system, no `Value` variant and no swept collision exist yet, despite being decided or discussed ([07](../designs/07-object-variables-and-references.md), [08](../designs/08-entity-storage-and-handles.md), [11](../designs/11-collision-detection-and-response.md)).
- `CollisionDetector::circleRectangle` has suspicious edge tests. Add a test in `tests/test_collision_geometry.cpp` before changing it.
- The XSD limits `variable/@value` to an integer and `condition/@value` to 0-255.
- Backends are only selectable in C++ (constructor arguments), not on the command line.
- On Windows the repo's `.gitattributes` converts line endings on commit; git prints LF-to-CRLF warnings, which are harmless.

## Suggested next steps (not started)

1. Refresh the root `readme.md` (or point it at `docs/readme.md`).
2. Add tests that pin the current behavior of `circleRectangle` before touching it.
3. Decide designs 03 and 04 (function syntax and arithmetic), since they change every game file.
4. Give every `grid()` cell a unique name so single objects can be addressed.
5. Add a jump verb ([design 13](../designs/13-verb-vocabulary.md)) as the first test of whether the vocabulary approach extends.

## Privacy rule for these docs

This repository is public. Nothing personal from a chat export goes in it: no health, identity, contact or business details, and no names beyond the `beefviper` author handle already in the code headers. Conversation titles are listed only for design-relevant chats.
