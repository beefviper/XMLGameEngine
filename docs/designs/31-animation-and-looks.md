# Animation and looks

**Status:** Built: animations, looks, looks that move, poses, mirror, blinking. Reference: [pictures](../reference/pictures.md#animation).

## Animation

- Arcade creatures flap between two poses. An object with several named sprites and an `<animation>`: its `<seconds>` (a picture's time; once `<interval>`, renamed to match hop, swing and timers) and `<frame sprite>`s naming them, so a sprite can be reused. Frames must be one size.
- **Seconds in the file, frames in the engine:** turned into frames at load. One count per object, advanced at the start of a frame while it is in play, so a pause holds the picture and a reset shows frame one. Cells animate together because they start together.
- In a group, a member's sprites replace the group's, and the animation is looked up in what the member ends with: three alien kinds in one lockstep group.
- **Frames cut from a sheet:** `<cell>` and runs of `<cells>` beside `<frame>`s, each a sprite of its own named `walk.1`, `walk.2` ([sheets](30-pictures.md#sheets)), so a sprite sheet needs no sprite per frame.
- Rejected: a global animation clock (awkward with pause and reset).
- A frame may take its own `<seconds>` (a cycle that holds one picture longer); without, the animation's. Only a `<frame>`, not a sheet's cells, which have no place to say it.
- `<once />` plays it through one time, for a burst or a death. A look then goes back to the last look shown that was not itself once, so a flash over a walk returns to the walk without the game naming it; an object's one animation has nothing to go back to and holds its last picture. Going back is a look change like any other, kept as `lookBefore` on the object, and a reset starts over from the first look.
- `<then>` says what comes after: `sprite=` a look in place of the one before, and commands run as an object timer's (the same place, so the same verbs and checks). It is the end of the animation, not a timer, so a game no longer counts the frames out to match. An object's one animation has no looks, so its `<then>` takes commands only. The commands run after every animation has counted its frame, by the object's index, since a spawn may grow the list.
- A once animation may be one frame: a look held for a while, which replaced the games' `<timer sprite="x"><after>` that turned a look back. Once looks come last among the looks, so an object never starts as one.
- `<become>` with a once look starts it from its first frame even when it already shows it, so a second hit is a whole second burst. Any look else is left alone when it already shows, so a walk is not restarted by a rule that names it every frame.

## Looks

- Frostbite's ice turns blue when landed on; Frogger had faked a filled home with a frog under each pad.
- **Several named sprites and no animation are looks.** `<become sprite>` switches; a reset shows the first. A rule with `sprite="white"` runs only while its object shows that look.
- **The look is the state: what the player sees is what the rule tests.** Chosen over `<become class>` (invisible, and colour would need its own command) and per-object state variables.
- **Looks count hits:** Breakout's two-hit row is `whole` then `cracked`. A variable could count, but `<die />` has no condition; a look can end the brick, and the player sees the count.
- **The look as the touch happened** is taken once before the rules run, as the speed is. Before, each rule saw what the rules before it left, so games wrote rules last look first, and a look changed inside a nested touch (Centipede's turn) set off the next rule too.
- **Edge rules too:** an edge's rules are one flat list of commands, so a `sprite=` rule is noted as where its commands start and end (`EdgeGuard`). Rejected: a list of rules per edge (every reader of the lists would change for one attribute).
- **`<become from="look">`** changes only objects showing that look: a pellet turns hunting ghosts blue and leaves eyes as eyes.
- With a heading each look keeps its own pictures to turn. Recolouring is a palette swap, apart from the look ([pictures](30-pictures.md)).

## Looks that move

- Mario walking, Coily an egg then a snake: an animation that is also one look among others.
- **A named `<animation>` is a look,** picked by `<become>`, `<poses>` and `from=` like a sprite. Animations are all named or none, so old files read as before.
- Rejected: frames inside `<sprite>` (a second way to say `<animation>`), animations as a layer over looks (two switches for one picture).

## Poses, mirror, blinking

- **`<poses>`** maps how the object moves (standing, walking, skidding, rising, falling, thrusting) to looks, picked each frame before the conditions. Fallbacks let a game give only the pictures it has. A pose is picked only while a pose look is showing, so a `<become>` to dying is not undone. Rejected: conditions doing `<become>` (a rule per pose), poses picked by the backend (tests must see them).
- **`<mirror>left</mirror>`** names the way the pictures do not face; going that way they are drawn flipped, made once at load. Not with a heading or a `<hole>`.
- **Blinking is a live number,** `owner.blink`: setting it starts it, a rule can test it, it counts down by itself. A `<blink>` verb would have needed a variable beside it anyway.
- Open: climbing and turning poses, a cell with poses of its own.
