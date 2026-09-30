# Notes for AI coding agents

Working notes for any AI coding agent (or new contributor) picking this project up. Everything the project itself relies on is in [../readme.md](../readme.md) and [../designs/00-designs.md](../designs/00-designs.md); this folder is for agent-facing bookkeeping.

The first version of these docs (the design write-ups, `sources.md` and the scan script) was written by Claude (Sonnet 5.5) on 2026-09-29, from the author's exported chat history plus a read of the source on the `claude` branch. Frogger (`games/frogger.xml`), the verbs it needed and design write-up 21 were added by Claude (Sonnet 5.5) on 2026-09-30, as was Space Race (`games/spacerace.xml`, first to two points, no new verbs). A second export (ChatGPT) was mined on 2026-09-30: it added design docs 22 to 28 and a section headed second batch in most earlier docs; only paraphrased ideas were kept, and titles of chats that did not start as game discussions were made generic.

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

- The root `readme.md` is out of date: it says collisions, scoring and win condition are missing, and lists only Xerces, exprtk and SFML.
- The repo has three branches: `master`, `rewrite`, `claude`. The active engine described in these docs is on `claude`.
- Function-call syntax is gone: attributes only name or pick things, everything else is element content ([design 03](../designs/03-expression-syntax.md)). Arithmetic in values is still exprtk text.
- No handle system and no `Value` variant exist yet, despite being decided or discussed ([07](../designs/07-object-variables-and-references.md), [08](../designs/08-entity-storage-and-handles.md)). Swept collision is built ([11](../designs/11-collision-detection-and-response.md)).
- `CollisionDetector::circleRectangle` has suspicious edge tests. Add a test in `tests/test_collision_geometry.cpp` before changing it.
- The XSD checks structure, not expression text; `xsd_lite` (used by the three non-Xerces backends) covers only the XSD subset the schema uses, and `tests/test_xml_format.cpp` pins what both validators must reject.
- Files in the repo use CRLF line endings; keep them when editing.
- Backends are only selectable in C++ (constructor arguments), not on the command line.
- A new game file has to be added to `data_xml` in `scripts/cmake/assets.cmake`, or the build does not copy it next to the executable; a new test file has to be added to the list in `scripts/cmake/tests.cmake`.
- Frogger's board is built from many named rectangles, drawn in file order (so scenery first, the frog last). It is long because the language has no way yet to say a lane of differently spaced objects; see [21](../designs/21-frogger.md).
- On Windows the repo's `.gitattributes` converts line endings on commit; git prints LF-to-CRLF warnings, which are harmless.

## Suggested next steps (not started)

1. Refresh the root `readme.md` (or point it at `docs/readme.md`).
2. Add tests that pin the current behavior of `circleRectangle` before touching it.
3. Decide design 04 (arithmetic as text vs elements); design 03 is done.
4. Give every `grid()` cell a unique name so single objects can be addressed.
5. Add an arcing jump ([design 13](../designs/13-verb-vocabulary.md)) as the next test of whether the vocabulary approach extends. `hop` (Frogger) was the first: one instant step per press.
6. The ideas listed at the end of [design 21](../designs/21-frogger.md): an amount for `inc`/`dec`, per-row velocity in `grid()`, show and hide verbs.

## Privacy rule for these docs

This repository is public. Nothing personal from a chat export goes in it: no health, identity, contact or business details, and no names beyond the `beefviper` author handle already in the code headers. Conversation titles are listed only for design-relevant chats.
