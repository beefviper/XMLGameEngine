# Animation and looks

**Status:** Built.

## Animation

- Need: arcade creatures flap between two poses; nothing changed a picture over time.
- An object with several `<sprite name="...">`s and an `<animation>`: `<interval>` seconds (a value above 0) and `<frame sprite="name" />` elements that refer to sprites by name (so a sprite can be reused in the sequence). At least two frames; every sprite must be shown; frames must be pictures of one size. (Several named sprites with *no* animation are looks, below.)
- **Seconds in the file, frames in the engine:** turned into frames with `<framerate>` at load (a window with no framerate is an error); a slow machine animates slowly with everything else. One count per object, advanced by `Game::updateObjects` for objects that are shown and in play; a pause or menu holds the picture; a reset restores frame one. The cells of a group animate together because they start together and share the pictures.
- Groups: a member's own sprites replace the group's; an animation (its own, else the group's) is looked up in the sprites the member ends up with. This is what gives Space Invaders three alien kinds in one lockstep group.
- Frames of a turned object each keep their own `Turnable`; equal-sized bitmaps turn to equal squares.
- Rejected: a global animation clock (flips everything on one tick regardless of state; a per-object count is simpler with pause and reset, at the cost of a group's cells being in step only because they start together).
- Not done: an interval per frame, an animation that runs once or starts on a collision, pictures driven by a variable, direction or hit, a color per pixel, an image-to-rows tool.


## Looks

- Need: Frostbite's ice turns blue when landed on and its igloo grows a block at a time; Frogger faked a filled home with a frog under each pad.
- **Several named sprites and no animation are *looks*.** `<become sprite="name" />` (or `object=` for a name or group) switches the picture; a reset shows the first. A collision rule with `sprite="white"` runs only while its object shows that look, so a row gives one block and then no more until it turns white again.
- **Looks count hits.** Breakout's top row takes two (`whole`, then `cracked`). A per-object variable could count them, but `<die />` has no condition, so the variable could not end the brick; a look can, and the player sees the count. Rules run in the order written and each sees the look as the rule before left it, so the rule for the last look comes first. A row cannot have rules of its own, so the row is a group of its own in the class `bricks`.
- **Edge rules too.** An edge's rules are kept as one flat list of commands per edge (an `edge="all"` rule is copied into four), so a `sprite=` edge rule is noted beside its edge's list as where its commands start and how many there are (`EdgeGuard`), and the look is checked when the run reaches them. A list of rules per edge was the other way; it would have changed every reader of the lists (the checks at load, the inspector, the tests) for one attribute. Before this the engine read `sprite=` on an edge rule and dropped it, so the rule ran whatever the object showed.
- Chosen over class changes (`<become class>`: rules already filter by class, but the change would not be visible and colour would need its own command) and per-object state variables. **The look is the state: what the player sees is what the rule tests.**
- A look is a spriteParams plus a bitmap swapped in and marked dirty, so no backend changed. Looks do not combine with an animation or a heading; a look cannot be a bound number text. No recoloring.
