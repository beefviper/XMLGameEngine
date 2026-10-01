# 22. Motion models

**Status:** idea (a vocabulary to choose from; only constant-velocity motion is built)

## The question

The engine today moves things by a velocity that a rule can rewrite. Pong needs nothing more. A platformer needs acceleration, gravity and a notion of grounded versus airborne. The question is how to *name* the step up so that an author picks a model instead of writing forces, and so that later games do not force a redesign ([13](13-verb-vocabulary.md), [23](23-jump-and-air-control.md)).

## Orders of motion

One useful ladder, from the early-arcade discussion:

- **Position-driven.** Something is placed directly, for example a paddle set by the input. No velocity is kept.
- **Constant velocity.** Position advances by a fixed amount each frame, and a collision rule *rewrites* the velocity (Pong, Space Invaders bullets). No accumulation. Reflection is a teleport of the velocity, not a force.
- **Accelerated.** Velocity accumulates from a force such as gravity. Motion is asymmetric (up is not the mirror of down), a jump becomes an impulse plus a pull, and states such as grounded and airborne appear (Donkey Kong onward).

The practical point: everything up to the second step fits a rule-driven declarative engine comfortably; the third is now built for one case: an object's `<acceleration>` and a held `<accelerate>` thrust that burns a variable (Lunar Lander, [34](34-lunar-lander.md)). It adds per-object state, continuous updates, and a fixed order inside a frame (gravity, then input, then collision, then response).

## Names for the two camps

Several pairs were weighed for the public vocabulary and for the internal docs:

- Kinematic versus dynamic (the standard terms; the leading candidate for anything an author sees).
- Event-driven versus integrated; rule-based versus state-based; analytic (position computed from time) versus integrated (stepped).
- Constraint-based (Pong is really this: a paddle is confined to an axis and a range) as a sub-label, and ballistic (launch, then only gravity acts) as another.
- Rejected as tempting but misleading: Newtonian versus not; impulse-based as a top-level split.

Suggestion recorded: two public words, kinematic and dynamic, with the finer labels kept for documentation.

## Simulated simulation

Many old games only imitate physics with tables and fixed arcs. A dynamic model therefore probably needs a mode that says *behave as if simulated* (a fixed arc that looks like gravity) next to a mode that integrates for real. The same split appears in the jump discussion ([23](23-jump-and-air-control.md)): fixed versus variable.

## Movement feel as data

Two design cultures showed up. Some games treat the avatar as an animation system that is given commands (press to begin a run cycle, release to begin a stop, press jump to begin the one fixed jump). Movement is a puzzle of preparation: line up with the edge, take the right number of steps. Modern remakes of the same games give free control and forgiving jumps. If the vocabulary can say which of the two a game uses, both can be described without special cases:

- **Animation-locked.** An input starts a fixed sequence; input during it is ignored or queued.
- **Free control.** Input adjusts velocity every frame.
- **Forgiveness** as separate optional tolerances (assistant suggestions, not decisions): a short grace period after leaving a ledge, a buffered jump press, automatic ledge grab, and pulling toward a landing spot.

## Related

- Contact-based response (angle from where the ball hits, speed from how the paddle was moving) is a collision-response question: see [11](11-collision-detection-and-response.md).
- Fixed timestep versus per-frame delta was mentioned when discussing physics rates; see [11](11-collision-detection-and-response.md) for the swept alternative.

## Sources

- "Pong vs Donkey Kong Physics" (2026-01-29): orders of motion, terminology.
- "Jump Behavior Models" (2026-09-23): static and dynamic, simulated physics.
- "Movement as Puzzle Design" (2026-07-22): animation-locked movement and preparation.
