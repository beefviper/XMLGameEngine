# 23. Jump and air control

**Status:** idea, refines the plan in [13](13-verb-vocabulary.md); not built

## Where [13](13-verb-vocabulary.md) left it

A jump is either static (same height, length, arc, time and animation every time, as in the early Castlevania games) or dynamic (height from how long the button is held, length from run speed, steering in the air, as in Super Mario Bros.). One verb with two models and a few parameters each.

## An alternative split: jump model plus air control

The second batch of conversations argued for separating two things that the first plan bundles:

- **The jump model** covers what happens at takeoff: the vertical profile (fixed, or variable with the button held, or cut short on release).
- **Air control** covers what the player can do while off the ground, per axis.

Horizontal air control is not one switch. A small ladder of behaviors:

| Behavior | Meaning |
|---|---|
| none | trajectory is decided at takeoff |
| steer | the player can bend the path |
| accelerate | input changes horizontal velocity |
| brake | input can reduce existing velocity but not reverse it |
| reverse | the player can build velocity the other way |
| full | horizontal motion is essentially player-controlled |

They combine, and the ladder explains a set of games without naming any of them: a fixed-arc game has none; a game where the player can stall but not turn around has brake; the Mario ledge trick (jump out, come back and land on a ledge directly above) needs reverse.

## Movement as capabilities

The broader form of the same idea is to describe movement as a list of capabilities on the ground and in the air rather than as a named jump type:

- ground: how input turns into speed (instant, or acceleration with friction).
- air: horizontal control (from the ladder), vertical control (fixed, variable), and whether reversal is allowed.

An author of a fixed-arc game says the air has no control on either axis; an author of a free-control game says full. The engine offers no type called after a specific game.

## Test for the vocabulary

A useful check, repeated across the conversations: can the language describe the jump of game A, game B and game C using only the shared words and different values, without a word that means behave like one of them. If a game needs its own type name, the underlying property has not been found yet.

## Open

- Whether height, distance and duration are separate inputs or consequences of a profile (start speed and pull). For the fixed jump the second reads better; for the variable one it is unavoidable.
- Whether jump is a verb on an object or part of a movement component ([27](27-object-composition-and-shape.md)).
- How an animation-locked jump interacts with the state model ([22](22-motion-models.md)).

## Sources

- "Jump Behavior Models" (2026-09-23): air control ladder, capability form.
- "VGDLs and Technology" (2026-05-09): static and dynamic as a behavior catalog with parameters.
- "Collision detection techniques" (2026-07-19): jump and physics modes sharing the same static/dynamic split.
