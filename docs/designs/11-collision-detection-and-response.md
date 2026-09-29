# 11. Collision detection and response

**Status:** implemented (edge-based, with known rough spots)

## Decision

Separate the three things that were tangled in the first prototype:

1. **Detection:** what happened, and where? Pure geometry, no side effects (`CollisionDetector`).
2. **Dispatch:** what did the XML declare should happen for that event? (`Game`, via typed `Command`s.)
3. **Execution:** do it (`CommandExecutor`).

In the prototype a single function both detected an edge hit and executed the action, and it parsed action tokens by walking a vector of strings with an iterator, which was fragile and hard to extend. Now `parseCommands()` is the only place that turns exprtk's token output into typed commands (`CmdBounce`, `CmdStick`, `CmdMove`, and so on), and responses are one `std::visit` per context.

The declarative form of the idea: the XML does not spell out an if-the-ball-touches-the-edge test. It says the ball *has* a collision with the vertical edge and the response is `bounce()`. The conditional lives in the engine's detection, not in the data.

## Current algorithm

- **Screen edges:** an object touches an edge when its position crosses the window bound (accounting for its size).
- **Two rectangles:** axis-aligned overlap; the edge reported is the one on the axis with the smaller overlap.
- **Circle and rectangle:** compare the circle's center with the nearest point on the rectangle and report an edge.
- Each pair is tested once per frame, only if at least one moved. The detector reports which edge of B was hit by A; each side of the pair then converts that to its own point of view.
- Movement is `position += velocity` once per frame, after collisions.

## Known weaknesses

- The circle-rectangle edge choice uses tests like `midpoint.y > rectTop - midpoint.y`, which subtracts a coordinate from a bound that already includes it. This was flagged in the first prototype's review and is still there.
- No swept (continuous) collision: a fast object can pass through a thin one in a single frame. The C++ reference Pong ([17](17-reference-pong.md)) solved this with a swept test against a moving frame of reference; the engine has not adopted that.
- No broad phase (every unordered pair is tested).
- `stick()` always zeroes horizontal velocity, even for a top or bottom contact.
- Only the four edges are reported; there is no contact normal, penetration depth, or corner handling.

## Design notes worth keeping

- The prototype's `TODO` about colliding with invisible objects: today only shown objects take part.
- Acceleration is a stated future need (currently only position and velocity).
- The C++ is scaffolding; the language design is the real project. Collision code should follow the vocabulary, not lead it. See [12](12-collision-escalation.md) for how the vocabulary might scale.

## Sources

- "Video game collection value in CAD" (2026-05-23): prototype `game.cpp` and its review.
- "Refactoring SFML Pong game code" (2026-07-11) and "Fixing pong collision and ball sticking issues" (2026-07-13).
