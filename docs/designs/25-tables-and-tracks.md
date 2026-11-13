# Tables and tracks

**Status:** All four stages built. Reference: [collisions](../reference/collisions.md), [pictures](../reference/pictures.md#turning-about-a-pivot), [input](../reference/input.md#a-keys-three-moments).

## Why this class

- After the grid games, a survey sized seven classes still out of reach: tables and tracks, moves as timelines (fighting), claimed area, pointer games, turn puzzles on cells, team sports, rhythm and words.
- **Chosen: tables and tracks,** a body moving over a shaped surface: pinball, top-down racing, mini golf, artillery, angled blocks. All were out of reach for the same four reasons: nothing faced a way (no bounce off a slope), nothing solid turned about a point, no key acted on being let go, and the ground never changed how a thing moved.
- Runner-up: moves as timelines (one-shot animations, a hitbox per frame, key sequences).

| Stage | Words | Game |
|---|---|---|
| 1 | `<bounce>` about the touch normal; pixel normals; `on="press|hold|release"` | Putt Park, mini golf |
| 2 | `<polygon>`; `<acceleration>` on a `<spawn>` (wind); the barrels a players group | Cannon Hill, artillery |
| 3 | `<pivot>`, `<swing>`, swept turns, speed at the touch point in `<knock>` | Pinball |
| 4 | `<surface>`, `<grip>` with a heading, path followers facing their way | Speedway, a racer |

## Decisions along the way

- **Keys:** pressed, held and released replaced a planned `<charge>` verb; a charge is hold plus `<inc>` plus a clamp ([input](08-input.md)).
- **Bounce:** one rule with no special cases ([collisions](20-collisions.md#bounce-by-the-normal)). A bumper is `<bounce>1.3</bounce>`, so `<kick>` was dropped.
- **Swept turns, never sub-steps.** Over a frame a flipper sweeps a wedge and the ball a fat line, so the touch is found in the frame's one sweep (walked together, then bisected), never by running the frame in smaller steps. Only a `<swing>` is swept; a `<turn>` or a `<set>` of a heading jumps.
- **Lent speed:** `<knock>` takes each side's speed at the touch point, velocity plus spin times the arm from the pivot, as a ball off a paddle depends on where it hits. No new word.
- **Settling, not hopping.** The ball once bounced wildly round the flippers. Two faults: a pixel touch's depth was the boxes' overlap (now measured by pixels), and slow knocks gave back their share (now, closing on an immovable surface slower than four frames of its own pull does not bounce). Scaled by the pull, so walkers and pool balls are untouched.
- **`<friction>`** rubs speed off along a still surface and brings a true rest when the pull cannot slide the thing or it is wedged. Pinball's ball has 0.05 and rests only when wedged.
- **The stroke is one eased curve:** `<swing direction="counterclockwise"><degrees>60</degrees><seconds>0.1</seconds></swing>`, the heading following half a cosine, hardest half way, as a flipper's coil works. Rejected: a top rate as bare content with a `<gain>` (a naked number, and no slowing at the end).
- **How far, not where to:** the start is not written, so neither is the end. `<degrees>` and `<seconds>`, since `<value>` is an operation's operand and hops already say `<seconds>`.
- **Clockwise and counterclockwise,** never left and right, for `<turn>` and `<swing>`.
- **Thick lines are round-ended bars** of true width; square stamps left a ledge a ball balanced on.
- Not added yet: ball spin, angular mass for free spinners, rolling apart from sliding, a `<launch>` word, a `<rest>` heading for a swing.

## The games

- **Putt Park:** six holes as scenes. Space held adds power, let go strikes, both `from="ready"` so a rolling ball cannot be struck. A condition stops the ball under 0.08 a frame. Open: the cup taking only a slow ball wholly over it, sand and water, a best round.
- **Cannon Hill:** hills are a `<polygon>` (lines have no fill; a bitmap hill is a page of stairs), cut by the existing `<hole>`.
  - Wind is a spawn's `<acceleration>`, so each shell keeps its turn's wind; a `<pull>` was rejected (a pull is toward a place).
  - The barrels are a players group (`<group name="barrels" from="barrel">` with `<turns sprite="idle">`), since both play on one field; where a shot is stays the table's look.
  - One barrel piece serves both sides by `heading = level - side * elevation`.
  - Open: cannons falling into craters, a computer player, a picture-shaped hole (`<hole sprite>`).
- **Pinball:** walls are one pixel picture of lines; bumpers bounce above 1. Flippers are a `<line>` piece with a `<pivot>`, a `<swing>` and `<knock>0.3</knock>`; the slopes end just above the hinge so the ball rolls on with no bump. The plunger is Space held and let go. Open: multiball, drop targets, tilt, ramps (a layer above the table).
- **Speedway:** `<grip>` with a heading is a wheel's grip, the speed across the heading losing that share; reused rather than a new word.
  - Rejected: turning the velocity to the heading (a skid would lose no speed).
  - `<surface>` is the ground under anything with a grip; rejected the car naming the ground it feels.
  - Laps by four gate rules, no `<gates>` word.
  - Path followers with a heading face their way.
  - Open: steering by speed, computer cars that race, lap times.
