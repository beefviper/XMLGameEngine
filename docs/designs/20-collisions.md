# Collisions

**Status:** Built: detection, dispatch, execution; swept object collision; pixel tests; riding; exceptions; deflect.

## Structure

- The prototype had one function that detected an edge hit *and* executed the action, parsing action tokens by hand. Split into three:
  1. **Detection**: pure geometry, no side effects (`CollisionDetector`).
  2. **Dispatch**: what the XML declared for that event (`Game`, typed `Command`s from `makeCommand()` in `command.cpp`).
  3. **Execution**: do it (`CommandExecutor`, one `std::visit` per context).
- Declaratively: the XML does not spell out "if the ball touches the edge"; it says the ball *has* a collision with the vertical edge and the response is `<bounce />`. The conditional lives in detection.
- Rules: `<collision edge="...">` (left/right/top/bottom/vertical/horizontal/all) or against another object by `class` and/or `object` (no selector = anything). Command lists run in order.


## Algorithm (as built)

- **Screen edges** are checked by position before the move (not swept). An object faster than a window is wide could skip them.
- **Swept object-against-object.** Reduce both motions to one (hold B still, move A by the difference). Rectangle vs rectangle: slab method against B grown by A's size. Circle vs rectangle: the centre against the rectangle grown by the radius, with rounded corners solved as a ray against a circle. Returns the time (0 to 1) and B's touched edge. Touching and moving apart is not a hit. Static overlap (`overlap`, `rectangleRectangle`, `circleRectangle`) covers pairs already overlapping at frame start.
- **Frame loop (`Game::moveObjects`).** Candidate pairs: one has a rule answering to the other, they are not in lockstep with each other, neither is in the air mid-`<jump>`, one is moving. (Members of a group without lockstep do collide with each other if a rule answers.) Each round every pair is swept for the time left; the earliest hit anywhere wins; all shown objects advance to that moment; that pair's rules run; the rest of the frame plays with whatever velocities they left. A pair reacts at most once per frame (bounds the loop and stops a bounce re-triggering).
- Each object is swept alone ([objects](05-objects.md) lockstep).
- **Hops and jumps land before collisions.** A queued hop is made at the start of the move and judged where it lands, not swept: swept, a frog hopping onto a log would first touch the *water*. A jump in the air is in no pair; the landing frame counts as moving.
- **Riding (`<ride />`).** Lends the other's velocity for the frame (`Object::riding`), added in movement, the sweep and edge checks; cleared each frame and recomputed from current touches, so stepping off needs no undo. Alternatives rejected: copy the velocity into the object's own (must be undone, fights hop and keys), attach as a child (an ownership model the language lacks), a position offset (bypasses the sweep). It was first called `<carry />`; it was renamed so the frog *rides* the log, and `carry` is kept for the other thing, one object taking another along (Adventure's key), which is not built.
- **`unless="class"`.** A rule is passed over while the object touches something of that class right now (`Game::isTouchingClass`, plain overlap), so it does not depend on pair order. Frogger's river: the water rule, unless on a log. Rejected: rule order (separate pairs do not compose), a "safe" class overriding a "hazard", water only in the gaps between logs, an overlap-counting condition. An edge rule takes it too (and `sprite=`), kept beside the edge's flat list of commands as an `EdgeGuard`. `class=` and `object=` pick the other of a touch, and an edge is no object, so on an edge rule they stop the load; before, the engine read all three on an edge rule and dropped them.
- **`<stick />`** is axis-aware (position and velocity corrected on the touched axis only, so a pushed-into-wall object still slides) and re-applied after the move so a stopped object never ends a frame past the wall.
- **Verbs in object rules.** `reset`, `move`, `inc`, `dec`, `ride`, `stop`, `reverse`, `become`, `reveal`, `release`, `play` and `reset object` all work in rules about another object (they used to be ignored); `executeObjectCollision` is given the other object.
- A dead object means the same everywhere: not drawn, moved or collided with (an older special case parked a dead circle at -100, -100).


## Pixel collisions (Lunar Lander)

- Need: a jagged moon in a bounding box is mostly empty; the same need as a fighting game.
- **Chosen:** `<type>pixel</type>` is opt-in per object. The box/circle sweep runs first; only a pair it reports touching is walked in half-pixel steps and bisected to the first step where a drawn pixel of one lies on a solid pixel of the other. Any other shape counts as solid over its shape (a circle analytically). Refused on text and images (their pixels belong to a backend). The edge reported is the one the motion came in through, so `stick` and `bounce` work unchanged. Any alpha above 0 counts as solid.
- Rejected: a list of boxes (a second description of one picture that drifts); pixel for everything (costly for no gain in Pong); a threshold for alpha (changes "solid" for all sprites).
- Limits: half-pixel steps; lines meant to be tested should be at least 2 pixels thick; no surface normal, so no bounce off a slope.


## Speed filters

- `<slower>N</slower>` / `<faster>N</faster>` before a rule's commands: run only while the object's speed (its velocity plus what it rides) is under N / at least N. The speed is read once per touch, so two rules for the pad (landing and crash) can never both run or both be skipped. Not allowed on screen-edge rules. Chosen over a rule on the key or a world variable.


## Deflect (Pong)

- Need: the ball only ever kept its random slope. The arcade rule: middle of the paddle sends it straight back, ends send it back at 45 degrees. No command could say where along the side the hit was.
- **Chosen: `<deflect>angle</deflect>`**, a new verb in a rule about another object (angle is a value, 0 up to 90). Measured from the side's middle: half the side's length is the full angle, in proportion between; leaves away from the side touched; **speed kept**. Pong uses 45.
- Rejected: `<bounce>45</bounce>` (bounce also runs on screen edges and in lockstep, keep every bounce unchanged); measuring to the ball's own edge (gave about 40 degrees at the top corner).
- Open: serve at a set speed and random angle, speeding up per hit with a cap, adding the paddle's motion to the angle (velocity transfer). Breakout's paddle does not use it yet (plain `<bounce />`).
