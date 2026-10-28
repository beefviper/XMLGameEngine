# Standing, leaping and climbing

**Status:** Built for Donkey Kong; air control and the arcing jump beyond the fixed arc are ideas.

## Standing, leaping and climbing (Donkey Kong)

**What we looked for.** Donkey Kong: Jumpman and the barrels fall and stand on girders, a barrel rolls off a girder's open end onto the next, Jumpman jumps a barrel in a fixed arc he cannot steer, and climbs ladders through the girders.

**What we looked at.** (a) Standing as a property of the object (a "solid" flag on the platform, every falling thing stopped by it): a world rule, the same objection as a world gravity. (b) Standing as a collision verb, `<land />`, on the faller's rule for what it stands on. (c) Reusing `<stop />`: it takes the pull away, so nothing walks off an edge and falls. For the jump: (d) the straight `<jump>` with more children; (e) a new verb that gives an upward speed under the object's own pull. For ladders: (f) a collision rule with the ladder (it only runs while something moves, so a man resting on a ladder would lose it); (g) a held action that looks for the ladder itself.

**Chosen: (b), (e), (g).**
- *`<land />`* acts only on the faller's bottom edge against the other's top, and only when not going up; it puts it on the top, stops the fall and marks it on the ground until the next frame's landing. The pull keeps acting, so the object is in a touch every frame and walks off an open end on its own. Platforms are solid from above only (the usual one-way platform): walked past at the ends, jumped up through.
- *`<leap>height</leap>`*: the speed up is `sqrt(2 * pull * height)`, so the author gives the height, not a speed. Only from the ground. Air control is the first rung of the ladder below, *none*: the way across is kept until the landing, which takes up whatever keys are held then. The loader refuses a leap on an object with no pull down or no `<land />` rule.
- *`<climb direction class>step</climb>`*: held like a move; `Game::applyClimbing` finds an object of the class under the climber's middle with its feet between that object's top and bottom, gets on (from the ground only), lines it up, and moves it; no pull, no landing, no walking off until an end. A ladder's top is the upper platform's top and its bottom the lower platform's top.
- A move key now changes only its own axis (it used to recompute both, which stopped a fall whenever a walking key was pressed).
- `<acceleration>` in a `<group>`, so a pool of barrels can fall.

**The game (2026-10).** Rewritten closer to the arcade: truss girders as one bitmap in a group of girder rows (`<columns>` gives each length), ladders as `<line>` drawings, rolling barrels, an oil drum, Kong and Pauline as bitmaps. The hammer is a look plus a variable a barrel's rule `<condition>`s, worn off by a timer of that look; the bonus is a variable a state timer counts down, scored on a rescue and costing a life at zero. Jumpman has a `<hitbox>` of his body: at twice the arcade's size his full sprite could not leap a barrel.

**Open.** Sloped girders (a platform at an angle: land on a line, not a box top). Barrels that take a ladder at random. Points for jumping a barrel (something has to notice the man passed over it). Fireballs from the oil drum. Dying from too long a fall. Other rungs of air control (steer, brake, reverse) and a held, variable-height jump.


## Arcing jump (idea; the fixed arc is built as `<leap>`)

- **Two types** in the first plan: *static* (same height, length, arc and time every time; Castlevania; parameters height, width, time) and *dynamic* (height from how long the button is held, length from run speed, steering in the air; Super Mario Bros.; parameters max hold time, max height, upward acceleration, whether the player can slow, stall or go backward in the air).
- **Refinement (second batch): a jump model plus an air-control ladder**, per axis. The model is the vertical profile at takeoff (fixed, variable with button held, cut short on release). Horizontal air control is a ladder: *none* (trajectory fixed at takeoff), *steer*, *accelerate*, *brake* (reduce but not reverse), *reverse* (the Mario ledge trick: jump out, come back, land on the ledge above), *full*. Mega Man can stall but not reverse.
- **Movement as capabilities:** describe the ground (how input becomes speed: instant, or acceleration with friction) and the air (horizontal control from the ladder, vertical control fixed or variable, reversal allowed) rather than naming a jump type. A fixed-arc game says the air has no control; a free game says full.
- Open: are height, distance and duration separate inputs or consequences of a profile (start speed and pull: better for fixed, unavoidable for variable); jump as a verb or part of a movement component ([objects](05-objects.md)); how an animation-locked jump interacts with states. Grounded vs airborne and a jump impulse now exist (`<land />`, `<leap>`); the rest of the ladder does not.

## Arc and grip (2026-10-23)

- Need: Q*bert's timed hops slid in a straight line ("the perfect example of one of those completely static timed jumps"); Mario started and stopped dead, with no slide or skid.
- **`<arc>`** after a timed `<hop>`'s or `<jump>`'s `<seconds>`: the object's altitude follows `4 * arc * t * (1 - t)` over the flight, so it is drawn up and over while its position (what it touches) still moves in a straight line. This is the *static* jump type above, said with the parts a timed hop already had. Chosen over a real pull for hops (Q*bert's hops must land on a cube exactly, every time) and over moving the position up and down (the cube under him would stop being what he touches).
- **`<grip>`** (above 0, up to 1) eases the speed across toward what the held keys ask for, that part of the gap a frame; up and down too when nothing pulls. It is the ground half of "how input becomes speed" above. Skidding is read from it: keys asking one way, speed still going the other. Grip below 1 also brakes an object with no keys held, so it belongs on the steered object only. The arcade Mario has no run button; a grip gives his slow start and slide.
- Open: different grip in the air, a top speed apart from the step, ice (a grip set by what is stood on).
