# Collisions

**Status:** Built: detection, dispatch, execution; swept object collision; pixel tests; riding; exceptions; deflect; side, checks and triggers in rules.

## Structure

- The prototype had one function that detected an edge hit *and* executed the action, parsing action tokens by hand. Split into three:
  1. **Detection**: pure geometry, no side effects (`CollisionDetector`).
  2. **Dispatch**: what the XML declared for that event (`Game`, typed `Command`s from `makeCommand()` in `game/command.cpp`).
  3. **Execution**: do it (`CommandExecutor`, one `std::visit` per context).
- Declaratively: the XML does not spell out "if the ball touches the edge"; it says the ball *has* a collision with the vertical edge and the response is `<bounce />`. The conditional lives in detection.
- Rules: `<collision edge="...">` (left/right/top/bottom/vertical/horizontal/all) or against another object by `class` and/or `object` (no selector = anything). Command lists run in order.


## Algorithm (as built)

- **Screen edges** are checked by position before the move (not swept). An object faster than a window is wide could skip them.
- **Swept object-against-object.** Reduce both motions to one (hold B still, move A by the difference). Rectangle vs rectangle: slab method against B grown by A's size. Circle vs rectangle: the centre against the rectangle grown by the radius, with rounded corners solved as a ray against a circle. Returns the time (0 to 1) and B's touched edge. Touching and moving apart is not a hit. Static overlap (`overlap`, `rectangleRectangle`, `circleRectangle`) covers pairs already overlapping at frame start.
- **Frame loop (`Game::moveObjects`).** Candidate pairs: one has a rule answering to the other, they are not in lockstep with each other, neither is in the air mid-`<jump>`, one is moving. (Members of a group without lockstep do collide with each other if a rule answers.) Each round every pair is swept for the time left; the earliest hit anywhere wins; all shown objects advance to that moment; that pair's rules run; the rest of the frame plays with whatever velocities they left. A pair reacts at most once per frame (bounds the loop and stops a bounce re-triggering).
- Each object is swept alone ([objects](05-objects.md) lockstep).
- **Hops and jumps land before collisions.** A queued hop is made at the start of the move and judged where it lands, not swept: swept, a frog hopping onto a log would first touch the *water*. A jump in the air is in no pair; the landing frame counts as moving.
- **Riding (`<ride />`).** Lends the other's velocity for the frame (`Object::riding`), added in movement, the sweep and edge checks; cleared each frame and recomputed from current touches, so stepping off needs no undo. Alternatives rejected: copy the velocity into the object's own (must be undone, fights hop and keys), attach as a child (an ownership model the language lacks), a position offset (bypasses the sweep). It was first called `<carry />`; it was renamed so the frog *rides* the log, and `carry` is kept for the other thing, one object taking another along (Adventure's key), built 2026-10-14 (below).
- **`unless="class"`.** A rule is passed over while the object touches something of that class right now (`Game::isTouchingClass`, plain overlap), so it does not depend on pair order. Frogger's river: the water rule, unless on a log. Rejected: rule order (separate pairs do not compose), a "safe" class overriding a "hazard", water only in the gaps between logs, an overlap-counting condition. An edge rule takes it too (and `sprite=`), kept beside the edge's flat list of commands as an `EdgeGuard`. `class=` and `object=` pick the other of a touch, and an edge is no object, so on an edge rule they stop the load; before, the engine read all three on an edge rule and dropped them.
- **`<stick />`** is axis-aware (position and velocity corrected on the touched axis only, so a pushed-into-wall object still slides) and re-applied after the move so a stopped object never ends a frame past the wall.
- **Carrying (`<carry />`, `<drop />`; built 2026-10-14).** In the carrier's rule about the thing (`<collision class="item"><carry /></collision>`): the carrier names what it carries (`Object::carrying`) and where it holds it (`carryOffset`, as it was when picked up), so the thing needs no rule of its own and one rule picks up a whole class. Each frame the carried thing is lent its carrier's motion before the move, as a ride is, so it is swept and its own touches run (Adventure's key opening the gate before the hero reaches it), and after the move it is put exactly where it is held (the carrier may have been stopped on the way). Kept on the carrier, not the carried, because the rule runs on the carrier and "one thing at a time" is a fact about the carrier: picking up another puts the first down where it is, as in Adventure, and taking one another object carries takes it from that one. `<drop />` (an action, a rule, a timer) puts it down and remembers it (`dropped`) until the two stop overlapping, or the carry rule would pick it up again at once; Adventure needs the same. Out of play on either side, or a reset of either, ends it. Rejected: an attachment kept on the carried thing (it would need its own rule, and the swap rule is the carrier's), a parent/child tree of objects (an ownership model the language does not have; [objects](05-objects.md)), moving the carried thing by teleport alone (its touches on the way would be missed, since a pair where neither moves is skipped). Not built: carrying several things, a carried thing that hangs where the carrier faces, a carrier taking it from another only when some rule says (Adventure's bat steals; here anyone does).
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


## Side, checks and triggers (2026-10-16)

- Need: Mario Bros. and Joust tell a bump from below or above from a touch from the side; a lap that counts only after the halfway marker is a rule that depends on a variable; six games wrote the same death (three to five commands) in two to four rules.
- **`side=`, chosen:** the side of the rule's own object that was touched, the edge the detector already reported. Same words as `edge=` (top, bottom, left, right, horizontal, vertical), but a separate attribute, since `edge=` already means a screen edge. Looked at: `edge=` doing both (one word meaning two things), and a `<from>` filter naming the other's motion (the touched side already says it).
- **Checks, chosen:** the `<check>`, `<all>` and `<any>` of [conditions](07-states-and-conditions.md) after `<slower>`/`<faster>`, all of which must hold. One test vocabulary, not a second one for rules. A `<check>` with no `class=` or `object=` means the object itself (`nameSelf`, after names are final), since a rule is always about its own object; in a state's condition that same check still means every object with the variable. Not yet on screen-edge rules (`EdgeGuard` would need its own checks); a state condition covers the cases met so far.
- **`<trigger>` in rules, chosen:** a rule runs one of the object's `<action>`s, `object=` left out meaning itself, and an action can now also `<inc>`, `<dec>`, `<copy>`, `<reset object>`, `<reset scene>`, `<enter>` and `<die />`. So a death is one action and the rules that kill each say `<trigger action="lose" />`. Looked at: several classes in one `class=` (a list in a string), and a named rule set (another mechanism for what an action already is). An action run from a rule or a timer is one press: a `<move>` in it stays on, as it did from a timer.
- Open: what a touch knows beyond the side (where along it, the touch point, the normal, the depth), for knocking and pool; the other object as a name in a rule (shelved earlier as a name whose meaning changes).
