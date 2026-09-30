# 24. Paths and formations

**Status:** idea (nothing built); needed for Galaxian-style games

## The problem

Enemies in Galaxian and Galaga fly in on curved entrance paths, hold a formation, and then break off to dive at the player. That is scripted motion which is neither constant velocity nor physics. Space Invaders moves a block by fixed steps and reverses at the wall; this is a different class.

## How the originals did it

- The paths were stored data: a table of positions or of per-frame velocity changes, applied one step per frame.
- Simple loops and waves could come from math instead of tables.
- Some systems had a tiny command language: move so many frames in one direction, then curve.
- Designers sketched on graph paper and converted the result into tables, or used a small plotting tool. ROM was tight, so segments were reused.
- Each entrance was tied to a script, ended in an empty slot of the formation grid, and a dive was another script.

## The four representations worth supporting

The author's stated goal was to support all of them, since each fits different games:

1. **Lookup table.** Positions per frame, or velocity steps (smaller and easier to reuse).
2. **Parametric curve.** x and y as expressions of time (circles, sine waves); fits the existing expression evaluator.
3. **Bezier curve.** A few control points; the facing angle comes from the derivative at each point, so a sprite can turn along the curve.
4. **Command sequence.** A turtle-like script (forward, turn, repeat), the Logo style.

A unifying idea: one interface that returns a position (and heading) for a point in time, with the four kinds as implementations behind it. That keeps [15](15-backend-abstraction.md) style substitution inside the game model too.

## Sketch (not adopted)

A path is a named element defined once; an object refers to it by name and says when to start. Tables would be lists of points or steps, parametric paths would hold two expressions, and a sequence would be a list of move and turn elements. Following the current rule that attributes only name or pick, the numbers all sit in element content ([03](03-expression-syntax.md)).

## Formations

The formation is a second layer: a grid of slots that an entrance path ends in, and a rule that releases some members to dive. Two pieces would be needed: slots that objects can occupy and leave, and a verb that says go to slot. The dive itself can use the AI targeting ideas in [13](13-verb-vocabulary.md), or another path.

## Sources

- "Enemy Path Creation Methods" (2025-06-12): representations, formations, authoring.
