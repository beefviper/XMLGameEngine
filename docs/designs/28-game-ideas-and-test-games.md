# 28. Game ideas and test games

**Status:** idea (candidates for games, and for features they would force)

These are game and mechanic ideas that came up; each is listed with what it would demand of the language. None is built.

## Pong variants

- **Contact-point Pong.** The vertical hit position on the paddle sets the bounce angle, and the paddle's movement adds some of its speed to the ball, so nicking a corner can send the ball off at a steep angle. Needs a response that maps contact position to angle, and a velocity transfer ([11](11-collision-detection-and-response.md)).
- **Energy Pong.** Paddles that can move a little toward or away from the ball. Moving away when it hits absorbs speed; moving toward adds speed. The horizontal range should stay small (a fraction of the court) so it still reads as Pong.
- **Third paddle.** A middle paddle driven by both players. Ways to combine two inputs: same direction moves it that way; the opposite mapping (both up sends it down) so a player can sabotage the other; sum of the inputs; sum with momentum so it feels heavy. Ideas to keep it fair: make it shorter than the others, or make it solid only for the defending player. Needs many-to-one input bindings ([10](10-input-and-actions.md)).

## Mechanics with a twist

- **A game that mocks bad play.** Doing badly earns power-ups with sarcastic messages; doing too well gets things taken away (fewer bombs next round). It is dynamic difficulty made visible. Needs conditions that read player performance and change stats ([14](14-conditions-and-win-conditions.md)).
- **Games that can be unwinnable from the start.** Some solitaire and mine-clearing boards cannot be won regardless of skill. A design choice for the language: an option to guarantee a solvable start, or to flag one that is not.

## Combat resolution styles

Three ways of resolving two groups fighting appear in strategy games, and they are candidates for a battle verb:

- **Pairwise.** Units are matched one to one; the weaker dies.
- **Aggregate / attrition.** Each side loses a share depending on the ratio of strengths; the stronger loses less but never nothing.
- **Dominance.** The stronger side wins with near-zero losses (only chance or crits cost anything).

RPG variants: an ability that uses the target's stat against it, damage as a fraction of maximum health, and reflecting a share of the incoming damage.

## Placement and packing

A grid game where large bases occupy squares and cannot overlap raised a small design analysis: with a gap of one cell between neighbors, destroying one base leaves a hole that fits only one enemy base; with a gap of two, it fits four. The takeaway for a description language is that footprint size and spacing are game parameters worth exposing, and that failure modes depend non-linearly on spacing.

## Balance math toolbox

Common ways to shape a value without branching, which could become expression helpers or value tags ([03](03-expression-syntax.md)): clamp to a range, saturate to zero-to-one, floor and ceiling, hard cap, diminishing returns, smooth steps, and switching a term off by multiplying with a comparison result.

## Genre observations

- A successful game's new mechanics tend to become a genre named after it, because games copy technology and mechanics, not only themes.
- Simulators kept adding fidelity; a language with a physics layer may later be asked to model a machine, not only a toy.
- Some games hide a very different game under a simple opening: a shallow surface, then layers of roles and coordination. Relevant to how much of a game a single XML file should hold.

## Sources

- "Pong vs Donkey Kong Physics" (2026-01-29) and "Pong with Third Paddle" (2026-07-12): Pong variants.
- "Power-Up Mockery Concept" (2025-02-27).
- "Unwinnable Game States" (2025-05-23).
- "Combat resolution types" (2025-10-25) and "RPG Battle Mechanics" (2026-07-10).
- "Game Mechanics Tricks" (2026-07-06).
- "Game Genres and Innovation" (2026-06-27) and "Simulator Genre Evolution" (2026-03-14).
