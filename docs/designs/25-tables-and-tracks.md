# Tables and tracks

**Status:** stages 1 and 3 Built (2026-10-31); stages 2 and 4 Decided, not built.

## Why this class

- Asked for (2026-10-31): after the grid games, "another class of games we can't easily describe" that would expand the language. The survey (project files, `notes/tables-and-tracks-2026-10-31.md`) sized seven classes still out of reach: tables and tracks, moves as timelines (fighting), claimed area, pointer games, turn puzzles on cells, team sports, rhythm and words.
- **Chosen: tables and tracks**, games of a body moving over a shaped surface: pinball, top down racing, mini golf, artillery over hills, angled blocks in a paddle game. Every one was out of reach for the same four reasons, all already known limitations: nothing faced a way (no bounce off a slope), nothing solid turned about a point, no key acted on being let go, and the ground never changed how a thing moved.
- Runner-up: moves as timelines (one-shot animations, a hitbox per frame, key sequences), which also helps games already here. Claimed area and turn puzzles are smaller follow-ons to the cells.

## Stages

Each ends with an original game, as the grid stages did.

| Stage | Words | Game | Status |
|---|---|---|---|
| 1 | `<bounce>` about the touch's normal with a share kept; a real normal for `pixel` touches; a key's three moments (`on="press"`, `"hold"`, `"release"`) | Putt Park, six holes of mini golf | Built |
| 2 | turns and wind for a shot built by holding (`<turns>` and `<pull>` exist); hills and craters with `<hole>` | an artillery duel | Decided |
| 3 | `<pivot>` (the point a heading turns about), `<swing>` back to where it started when let go, the turn swept through the frame, a turning thing's speed at the point of touch lent by `<knock>` | Pinball, one original table | Built |
| 4 | `<surface>` on map pieces (drag and grip while over them), `<grip>` turning velocity toward a heading, `<gates>` and laps | a top down racer | Decided |

## Decisions along the way

- **Keys** ([input](08-input.md#a-keys-three-moments-2026-10-31)): Beef's framing of pressed, held, released replaced a `<charge>` verb.
- **Bounce** ([collisions](20-collisions.md#bounce-by-the-normal-2026-10-31)): Beef asked for one rule with no special cases; square things keep the classic flip because their sides are square. A bounce is off a surface standing still; a moving surface lending its speed is `<knock>`'s, so `<kick>` was dropped for `<bounce>` above 1.
- **Swept turns, no sub-steps** (Beef, 2026-10-31): every game here is worked out a frame at a time; over one a flipper sweeps a wedge and the ball a fat line, so the touch is found in the frame's one sweep (the pair walked together, half a pixel of the furthest-moving point at a time, then bisected), never by running the frame in smaller steps. Only a `<swing>`'s turn is swept: it is the one turn whose speed matters. A `<turn>` or `<set>` of a heading jumps, as before.
- **Lent speed** (Beef: "analogous to a ball bouncing off a paddle at a different angle based on where it hit"): `<knock>` takes each side's speed at the touch point, velocity plus spin times the arm from the pivot. No new word: a flipper's rule is an ordinary `<knock>`.
- **Settling, not hopping** (Beef, after playing: the ball was "a rubber ball bouncing crazily" round the flippers and "friction would dominate" at low speeds): two faults. A pixel touch's depth was the boxes' overlap, so a `<knock>` threw a ball lying on a flipper (whose turning square is far bigger than its bar) tens of pixels; depth is now measured by the pixels. And slow knocks gave back their share: now a thing closing on an immovable surface slower than four frames of its own pull into it does not bounce (a share up to 1). Scaled by the pull, so walkers meeting walls (no pull into them) and pool balls (both with mass) are untouched; no friction along the surface, so a ball still rolls down a slope.
- **Turning in three layers, friction, rest** (Beef, after playing again: the cradled ball still jittered "ever so slightly", the flipper should gain power through its stroke, and "position, velocity, acceleration, and friction" should describe it): a turn has heading, turn a frame and how that changes; `<friction>` on the moving thing rubs speed off along a still surface it meets, and brings it to a true rest (not moved, not pulled) when its pull cannot slide it or it is wedged between two surfaces, until something moves it or a support moves. A rolling ball should not be held on a real slope, so pinball's ball has 0.05 (holds under about 3 degrees) and rests only wedged.
- **The stroke is one eased curve** (Beef: a flipper moves like a sine wave, accelerating to half way and decelerating, since it must overcome its own resting mass and its coil maxes out at the end): `<swing direction="counterclockwise"><degrees>60</degrees><seconds>0.1</seconds></swing>`, the heading following half a cosine wave from start through the turn, so position, speed and acceleration of the turn all come from it and a strike half way is hardest. Replaced a first form, `<swing>16<to>330</to><gain>4</gain></swing>` (a top rate as bare content and a gain to it), which Beef rejected: the bare number looked naked next to `<to>`, and the stroke should slow at the end too. Time rather than a rate, so a game says how long a flip takes, which is how a flipper is felt.
- **How far, not where to** (Beef: the start is not written, so neither should the end be): a swing says the turn, `<degrees>` with `direction="clockwise|counterclockwise"`, not a `<to>` heading, so the two flippers differ only in their way. Beef offered `<value>` and `<duration>`; kept `<degrees>` and `<seconds>`, since `<value>` is the formula operand word and `<hop>` and `<jump>` already say `<seconds>` for how long a move takes. Over 180 degrees is allowed: the way is said, not the shorter one.
- **Clockwise, not left and right** (Beef, a pet peeve: a rotation has no left or right): `<turn>` and `<swing>` take `direction="clockwise"` or `"counterclockwise"`. Moves keep up, down, left, right.
- **Thick lines are round-ended bars** of true width, not rows of square stamps: square stamps turned with the flipper left a flat ledge at its hinge that a ball balanced on. The bar sits in the squares the stamps covered, so pictures stay where they were; no other game's tests moved.
- **The turning sweep tests the pictures as drawn** (whole degrees, kept once made), not a rotation of the upright one, so the sweep and the touch agree.
- **Not added**: a ball's own spin (english), angular mass for things that turn freely when hit (spinners, gates), rolling as distinct from sliding. Each waits for a game that needs it.
- **No `<rest>`.** A swing goes back to the heading the object started with (its `start`); a word is added if a game wants it to rest elsewhere.
- **No `<launch>` yet.** Putt Park sets the ball's `velocity.x` and `.y` from `<sine>` and `<cosine>` of the arrow's heading. A word is added when a second game wants the same.

## Putt Park (stage 1)

- Six holes as scenes of one state; each takes the first hole's and gives its own walls (one picture of `<line>`s, `pixel` collisions), cup, tee, par, and posts or cushions from the kit. A scene condition puts the ball on its tee when the card says it has not been placed.
- The arrow is a bitmap turned by its `<heading>` about its middle, which sits on the ball; its two looks are the same size (all clear while the ball rolls), since `arrow.width` read in the same condition as a `<become>` is still the old look's.
- Space held adds to the arrow's power up to 100, let go strikes; both through `<trigger from="ready">`, so the ball cannot be struck while it rolls. A condition stops the ball dead once its speed is under 0.08 a frame each way and gives the arrow back.
- Open: the cup taking only a slow ball (a rule's condition on speed), water and sand (stage 4's surfaces), more holes, a best round kept.

## Pinball (stage 3)

- One table on one screen: walls as one `pixel` picture of `<line>`s (a rounded top, the shooting lane, slopes down to the flippers), two slingshot lines and three bumpers that `<bounce>` above 1, three rollover lanes the ball passes over (the lane's own rule, from its `dark` look, so it lights once).
- Flippers are a kit `<line>` with `<pivot>` at the hinge end, a `<swing>` through 60 degrees in 0.1 seconds, and the rule `<knock>0.3</knock>` on the flipper; the ball has `<mass>`, `<friction>0.05</friction>` and `<acceleration>` for the table's tilt. The slopes above the flippers end just above the hinge, so a ball rolls from slope onto flipper with no bump.
- The plunger is Space held (`on="hold"` pulls it back, up to 100) and let go (`on="release"` sets the ball's speed), both `from="loaded"`, the look the ball's rule gives it when the ball sits on it.
- All three lanes lit: a bonus, and the lanes show `flash` for half a second before going dark, so the ball still over the last one does not light it again at once.
- Open: multiball (`<spawn>` a ball), drop targets, a tilt, a ball saver, ramps (needs a layer above the table, like `<altitude>`), a high-score table, nudging the table.

