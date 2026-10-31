# Animation and looks

**Status:** Built.

## Animation

- Need: arcade creatures flap between two poses; nothing changed a picture over time.
- An object with several `<sprite name="...">`s and an `<animation>`: `<interval>` seconds (a value above 0) and `<frame sprite="name" />` elements that refer to sprites by name (so a sprite can be reused in the sequence). At least two frames; every sprite must be shown; frames must be pictures of one size. (Several named sprites with *no* animation are looks, below.)
- **Seconds in the file, frames in the engine:** turned into frames with `<framerate>` at load (60 when left out); a slow machine animates slowly with everything else. One count per object, advanced first thing in a frame (`playOneFrame` in `play/frame.cpp`) for objects that are shown and in play; a pause or menu holds the picture; a reset restores frame one. The cells of a group animate together because they start together and share the pictures.
- Groups: a member's own sprites replace the group's; an animation (its own, else the group's) is looked up in the sprites the member ends up with. This is what gives Space Invaders three alien kinds in one lockstep group.
- Frames of a turned object each keep their own `Turnable`; equal-sized bitmaps turn to equal squares.
- Rejected: a global animation clock (flips everything on one tick regardless of state; a per-object count is simpler with pause and reset, at the cost of a group's cells being in step only because they start together).
- Not done: an interval per frame, an animation that runs once or starts on a collision, pictures driven by a variable or hit, an image-to-rows tool. (A color per pixel came as bitmap inks, [pictures](30-pictures.md).)


## Looks

- Need: Frostbite's ice turns blue when landed on and its igloo grows a block at a time; Frogger faked a filled home with a frog under each pad.
- **Several named sprites and no animation are *looks*.** `<become sprite="name" />` (or `object=` for a name or group) switches the picture; a reset shows the first. A collision rule with `sprite="white"` runs only while its object shows that look, so a row gives one block and then no more until it turns white again. **The look as the touch happened** (2026-10-31): taken once before the rules run, as `<slower>`/`<faster>` are. Before, each rule saw what the rules before it left, so games wrote look chains last look first and a look could not be flipped back and forth by one touch; worse, a look changed inside a nested touch (Centipede's turn: a refused hop off the map runs the edge's rules, which turn it to rising) started the next rule of the outer touch, turning it twice.
- **Looks count hits.** Breakout's top row takes two (`whole`, then `cracked`). A per-object variable could count them, but `<die />` has no condition, so the variable could not end the brick; a look can, and the player sees the count. Rules run in the order written and each sees the look as the rule before left it, so the rule for the last look comes first. A row cannot have rules of its own, so the row is a group of its own in the class `bricks`.
- **Edge rules too.** An edge's rules are kept as one flat list of commands per edge (an `edge="all"` rule is copied into four), so a `sprite=` edge rule is noted beside its edge's list as where its commands start and how many there are (`EdgeGuard`), and the look is checked when the run reaches them. A list of rules per edge was the other way; it would have changed every reader of the lists (the checks at load, the inspector, the tests) for one attribute. Before this the engine read `sprite=` on an edge rule and dropped it, so the rule ran whatever the object showed.
- Chosen over class changes (`<become class>`: rules already filter by class, but the change would not be visible and colour would need its own command) and per-object state variables. **The look is the state: what the player sees is what the rule tests.**
- **`<become ... from="look">`** changes only the objects showing that look: a pellet turns every hunting ghost blue but leaves eyes going home as eyes. Without it, the pellet would need a rule per look.
- A look is a `Picture` plus a bitmap swapped in and marked dirty, so no backend changed. With a heading each look keeps its pictures to turn (since 2026-10-29, for Asteroids' flame); a look cannot be a bound number text. Recoloring is a palette, apart from the look (see [pictures](30-pictures.md)).

## Looks that move (2026-10-23)

- Need: Mario walking, Coily an egg that becomes a snake, Q*bert's discs spinning, each an animation that is also one look among others. Before, an object had either one animation or still looks.
- **A named `<animation name="walking">` is a look.** `<become>`, `<poses>` and `from=` pick it like a sprite. The looks are the animations in the order written, then the sprites; the object starts as the first. A named animation need not show every sprite (the rest are still looks), and animations are all named or none, so an old file reads as before.
- Chosen over a `<frames>` element inside `<sprite>` (a second way to say what `<animation>` already says) and over animations as a separate layer on top of looks (two switches for one picture).
- In a group, animations merge by name: a member's animation of a name replaces the group's.

## Poses and mirror (2026-10-23)

- Need: Mario with a walk, a skid, and frames up and down, without a rule for each change; Q*bert and Coily facing the way they hop with one picture a side instead of one per way.
- **`<poses>`** maps how the object moves (standing, walking, skidding, rising, falling, and thrusting while a `<thrust>` is held, falling back to standing) to looks; the engine picks one each frame before the conditions run. Fallbacks (walking to standing, skidding to walking, falling to standing, rising to falling) let a game give only the pictures it has. The pose is picked only while one of the pose looks is showing, so a `<become>` to another look (dying) is not undone the next frame. An object with poses starts as and resets to standing, wherever it is listed.
- Rejected: conditions on velocity doing `<become>` (a rule per pose, each needing the other's look excluded, and no "skidding" without the keys); poses picked by the backend (they are game state, tests must see them).
- **`<mirror>left</mirror>`** names the way the pictures do not face; going that way they are drawn flipped. The way is the keys held, else a jump's way across, else velocity; stopping keeps it. Flipped bitmaps are made once at load and shared, like turned ones. Not allowed with a `<heading>` (that turns the picture instead) or a `<hole>` (holes are cut in one picture).
- **Blinking** is a live number part, `owner.blink` (seconds left), not a verb: setting it starts it, a rule can test it (Mario's shellcreepers leave a blinking Mario alone), and it counts down by itself. A `<blink>` verb would have needed a variable beside it for the rules anyway.
- Open: an `airborne`/`climbing` pose for ladders, a turning pose, poses read by a group's cells from a shared list only (a cell cannot have its own).
