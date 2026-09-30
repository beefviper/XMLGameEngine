# 12. The collision escalation problem, and trading-card-game style resolution

**Status:** idea under design; not implemented

## The problem

The more entity types and abilities a game has, the more collision rules must be written: roughly n x m for n types and m other types (n squared in the worst case). The author thought this was named in the VGDL literature as a scaling problem. A literature search found no source that calls it "collision escalation", but the mathematics is a well-known one: the **binary method problem**, handled by double dispatch or multiple dispatch, where even the tidiest mechanism still needs n-squared cases somewhere. A relative, the expression problem, has the same shape for types crossed with operations.

## What existing systems do

- **VGDL's SpriteSet is hierarchical.** Rules written against a parent type apply to every child, so adding `flying_enemy` under `enemy` needs no new pairwise entry. That shrinks n from sprites to categories but the shape stays category x category.
- **Magic: The Gathering's layer system.** A new card does not get rules against every other card. Each continuous effect declares what *kind* of thing it does (power and toughness, type, color, ability grants, and so on), and a fixed global algorithm (seven layers, ordered by timestamp and dependency) composes all active effects. The cost is that the fixed taxonomy has to be designed right up front, and the resulting algorithm is regarded as the hardest part of the rules.
- A patent titled "Interaction management for virtual environments" was reported in the research conversation as describing the same pattern (tags, expressions and a shared interaction matrix). Not independently verified.

## The author's idea

Replace (sprite x sprite) rules with (class x class) or (verb x state) resolution over a small, fixed vocabulary and fixed steps, the way a trading card game resolves a new card against existing ones through shared vocabulary and phases.

## Worked model: Super Mario Bros.

Mario has states (small, large, fire, dead; or 3, 2, 1, 0 hit points) and direction. The steps:

1. **Declare.** Mario, moving down, sends the verb `stomp` chosen purely from his own state.
2. **Respond.** Each thing answers the verb from its own class and state alone: a goomba returns `dead` and the interaction ends; a koopa returns `shell` and changes its own state; a spiny returns `hit`.
3. **Counter phase.** `hit` moves the engine to the next phase, where Mario resolves it against his own state: Invincibility if the star is active, otherwise he takes the hit.
4. **Bookkeeping.** Balance any values and apply the next rule set.

Neither side needs to know the other's concrete type, so a new entity costs about v rules (one per verb it responds to or emits) instead of n. v (the verb alphabet) must stay small and roughly fixed while n grows. This is better than classic double dispatch, which still routes through concrete type pairs.

Points raised while stress-testing it:

- **Do not over-borrow from MTG.** A fixed four-stage machine (declare, resolve, counter, bookkeeping) is enough for Mario, because nothing modifies a third object's resolution. A stack-and-priority system is only worth adding if one resolution must interrupt another.
- **Symmetry.** If the rule is hard-wired so that the player always declares and the enemy always responds, a sliding koopa shell hitting a stationary Mario needs special cases, which brings back the pairwise problem. The entity in the more specific or active motion state should declare, not the player.
- **Proxy or geometry?** In the real game the stomp check is only whether Mario's vertical speed is downward, and never his relative position, which is why a falling Mario can stomp a goomba falling across from him. Option A keeps that cheap proxy (authentic, quirks included). Option B derives the verb from real contact geometry (kills the quirk, costs a computation). This is a one-place decision that does not affect the architecture.
- The original 1985 code did not do this; it was a chain of compare-and-branch on enemy IDs. The verb protocol is an improvement the hardware never had room for, not a reconstruction.

## Translation to XMLGameEngine

Entities declare classes or tags (the existing `class` attribute is the anchor). Interaction rules key on `(classA, classB, phase)` rather than `(spriteA, spriteB)`. A new entity tagged `Hazard` and `Pushable` inherits every rule already written for that pair of tags. Honest cost, as with MTG: the taxonomy of tags and phases becomes the hard design artifact and is painful to retrofit.

The engine today is at the first rung only: `class` and `object` filters on a collision rule ([06](06-names-and-classes.md), [11](11-collision-detection-and-response.md)).

## Second batch: alternatives

- **Response as mapping.** Instead of a verb for reflect, describe how the outcome depends on the contact (where, how fast, with what) and let the mapping be the rule ([11](11-collision-detection-and-response.md)).
- **Interactions as constraints.** The declarative framing: an object reflects off surfaces with a response profile, so the data says what a surface does and what an object does to surfaces. This raises the question of which side owns the reflection.

## Sources

- "Video game description languages: overview and research" (2026-09-22).
- Second batch: "VGDL Design Challenges" (2026-01-28), "Pong vs Donkey Kong Physics" (2026-01-29).
