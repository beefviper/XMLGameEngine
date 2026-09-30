# 11. Collision detection and response

**Status:** implemented (swept object-against-object collision, edge-based screen collision, riding and rule exceptions; rough spots listed)

## Decision

Separate the three things that were tangled in the first prototype:

1. **Detection:** what happened, and where? Pure geometry, no side effects (`CollisionDetector`).
2. **Dispatch:** what did the XML declare should happen for that event? (`Game`, via typed `Command`s.)
3. **Execution:** do it (`CommandExecutor`).

In the prototype a single function both detected an edge hit and executed the action, and it parsed action tokens by walking a vector of strings with an iterator, which was fragile and hard to extend. Now `parseCommands()` is the only place that turns exprtk's token output into typed commands (`CmdBounce`, `CmdStick`, `CmdMove`, and so on), and responses are one `std::visit` per context.

The declarative form of the idea: the XML does not spell out an if-the-ball-touches-the-edge test. It says the ball *has* a collision with the vertical edge and the response is `bounce()`. The conditional lives in the engine's detection, not in the data.

## Current algorithm

- **Screen edges:** an object touches an edge when its position crosses the window bound (accounting for its size).
- **Static overlap** (`overlap`, `rectangleRectangle`, `circleRectangle`): two rectangles use axis-aligned overlap and report the edge on the axis with the smaller overlap; a circle uses the distance from its centre to the nearest point of the rectangle, reports the side it is furthest past, and for a centre inside the rectangle the nearest side. Used for objects that already overlap when a frame starts (a hit at time 0).
- **Swept** (`CollisionDetector::sweep`): the two objects' motions over the frame are reduced to one by holding B still and moving A by the difference. Rectangle against rectangle: A's top-left corner against B grown by A's size, entering by the slab method. Circle against rectangle: the circle's centre against the rectangle grown by the radius, which is exact along the flat sides; if it enters beside a corner, the corner is solved as a ray against a circle of that radius, which gives the rounded corner and a time. It returns the time (0 to 1 of the frame) and the edge of B that was hit. Touching and moving apart is not a hit.
- **Frame loop** (`Game::moveObjects`): the pairs worth testing are those where one object has a rule that answers to the other (class/object or `basic`), neither is a group-mate of the other, and one is moving. Each round, every pair is swept for the time left in the frame; the earliest hit anywhere wins, all shown objects advance to that moment, that pair's rules run, and the round repeats for the time that remains, with whatever velocities the rules left. A pair reacts at most once per frame, which bounds the loop and also stops a bounce from immediately re-triggering. With no hits, every object simply moves by its velocity once, as before.
- **Each object is swept on its own.** Groups exist for behaviour (the block marches and turns together) and never for geometry; there is no bounding box around a group, so as aliens die the remaining ones are the only ones tested. Grid cells are individually named (`aliens.3.2`) to make that possible, and for games (Galaga, Galaxian) where members will leave a formation one by one.
- **Rules about another object can do more than bounce and die.** `reset()`, `move.*`, `inc()`, `dec()` and `carry()` work there too (they used to be ignored), so an object can say what touching a hazard or a goal costs or scores. `CommandExecutor::executeObjectCollision` is handed the other object as well as the edge, which is what `carry()` needs.
- **Riding (`carry()`).** A rule with `carry()` gives the object the other's velocity for the frame (`Object::carry`), added to its own in movement, in the sweep and in the edge checks. It is cleared at the start of every frame's move and worked out again by that frame's touches, so stepping off is automatic and nothing has to be undone. Pairs are judged with the carried velocity, so an object sitting still on something moving is a moving pair.
- **Hops land before collisions.** A queued hop is made at the start of the move, before pairs are built, and the object counts as moving for that frame. It is judged where it lands, not swept along the way: swept, a frog hopping onto a log from the grass would first touch the *water* at the edge of the river, before reaching the log, and be lost on the way. A hop is one lane, so nothing is skipped over.
- **`unless` on a rule.** A rule can be passed over while the object is also touching something of another class (`Game::isTouchingClass`, a plain overlap test against everything in play). It is a test of where things are now, so two rules for the same moment do not depend on the order the pairs were found in. This is what lets a wide, static river kill the frog anywhere except where a log is.
- `stick()` is axis-aware: it corrects position and cancels velocity only on the touched edge's axis, so an object pushed into the bottom wall while holding left or right keeps sliding (the alternative, where any push into a wall freezes the object, feels bad to play). It is also re-applied after the move, because the pre-move edge checks only run for a moving object and would otherwise leave a stopped object overshooting the wall by up to one frame of velocity.

## Known weaknesses

- Screen edges are still checked by position before the move, not swept. An object cannot get far past one (the check runs every frame and bounce/stick/die all act on it), but an object faster than a window is wide could skip the check.
- Only the four edges of a rectangle are reported; a corner hit is reduced to the side the contact normal points towards most, and there is no penetration depth or true contact normal for the response to use (`bounce()` just flips one velocity component).
- The candidate pairs are built with a plain double loop over the shown objects each frame (cheap next to the sweeps, but it is not a broad phase). A game with thousands of interacting objects would want a spatial grid.
- A pair in which nothing is moving is not looked at, so a rule against something that stands still does not run for an object at rest. Hopping, being carried and moving all count, and that covers Frogger (a frog that hops into the river is judged, and a frog on a log is being carried), but an object that has been standing still in a place that *becomes* dangerous is not noticed until it moves. A `watch`-style flag on a collision that must be looked at every frame is one way out, if a game needs it.
- Only the first hit of a pair each frame is handled. Two objects meeting again in the same frame after a bounce (a ball trapped between two close walls) will not react a second time until the next frame.
- The engine used to park a dead circle at (-100, -100) and zero its velocity; that special case is gone (`die()` now means the same everywhere: not drawn, moved or collided with), because a dead object is no longer part of anything.
- What it replaced: a per-frame overlap check after which the object moved anyway, which let a small or fast object tunnel through a thin one (the smaller ball and the Space Invaders bullet), and a `circleRectangle` edge choice that compared a coordinate against a bound minus the same coordinate. The C++ reference Pong ([17](17-reference-pong.md)) had the swept test against a moving frame of reference; this is that idea, kept behind the same detector interface.
- `basic="basic"` was a stopgap spelling for "the one general object-against-object rule". It is gone now: a `<collision>` with no `edge`, `class` or `object` means "anything".

## Design notes worth keeping

- The prototype's `TODO` about colliding with invisible objects: today only shown objects take part, and a hidden object does not move at all. That is what lets a projectile wait unseen and still until `fire()` shows it; it also means there is no hidden-but-moving-and-colliding object (a trigger zone, an enemy still off-screen). If a game needs one, visibility and "in play" have to become separate flags; `fire()` and `die()` would then flip the second.
- Acceleration is a stated future need (currently only position and velocity).
- The C++ is scaffolding; the language design is the real project. Collision code should follow the vocabulary, not lead it. See [12](12-collision-escalation.md) for how the vocabulary might scale.

## Second batch: alternatives

**Detection alternatives.**

- **Sorted swept boxes.** Take last and current positions, wrap the swept path in a box, keep objects sorted along an axis, and stop testing at the first that cannot overlap. A cheap "what might collide" pass, then an expensive "how" pass. The built solution has no broad phase (see the notes above); this is the likely shape if one is needed.
- **Smaller time steps.** Running the physics faster than the frame rate is the other cure for tunneling; the swept approach avoids the extra steps.
- **Deferred handling.** Detect first and collect the touches; then resolve in a separate pass; then update sprites. Alternatives for the response hook: a virtual method on the object, a collision component, or data-driven actions (the direction taken).

**Naming and targets.**

- **Screen edges as objects.** Treat the world bounds as invisible collidable planes the engine creates, so a rule says touching the left bound rather than testing coordinates. Rules could then read as sentences: on touching a target, do something.
- **What versus how.** An attribute that says what was hit (edge, object, trigger) is a different concept from one that says what to do; mixing them in one attribute name was the confusing part of an early form. A separate attribute for the target kind was proposed.
- **Edge orientation.** A hit on the top or bottom edge is a vertical collision, because the surface normal is vertical.
- **Grouped edges.** Grouping names such as vertical, horizontal and all are shortcuts over the four sides.

**Response alternatives.**

- **Classic.** Reverse a velocity component.
- **Contact-point mapping.** The hit position on the paddle sets the outgoing angle.
- **Velocity transfer.** The moving paddle adds a share of its speed; a paddle that can also move along the ball's path changes the rebound speed ([28](28-game-ideas-and-test-games.md)).
- **Two rules at once.** When several rules apply, is reflection a property of the ball, the paddle, the pair, or the rule? The open question that [12](12-collision-escalation.md) tries to answer.

## Sources

- "Engine design conversation A (a long general chat; only the project segment was used)" (2026-05-23): prototype `game.cpp` and its review.
- "Refactoring SFML Pong game code" (2026-07-11) and "Fixing pong collision and ball sticking issues" (2026-07-13).
- Second batch: "Collision detection techniques" (2026-07-19), "Deferred Collision Handling" (2025-04-22), "Collision definition suggestions" (2026-07-23), "Vertical Collision Calculation" (2024-12-24), "Pong vs Donkey Kong Physics" (2026-01-29).
