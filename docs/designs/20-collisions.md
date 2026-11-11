# Collisions

**Status:** Built: detection, dispatch and execution apart; swept touches; pixel tests; riding and carrying; `unless=`; deflect; side, checks and triggers in rules; contacts and `<knock>`; solid inks and hitboxes; bounce by the normal. Reference: [collisions](../reference/collisions.md), [commands](../reference/commands.md#touching).

## Structure

- The prototype found an edge hit and acted on it in one function, parsing action tokens by hand. Split into three:
  1. **Detection**: geometry only, no side effects (`play/collision.h`, `play/geometry.h`).
  2. **Dispatch**: what the XML declared for that touch (typed `Command`s from `game/command.cpp`).
  3. **Execution**: `CommandExecutor::run`, one way in for every place a command runs: it calls the command's verb ([verbs](14-verbs.md)), told by a `Doer` what it has (an object, what it touched, a key, an action).
- Declarative: the ball *has* a collision with the vertical edge whose answer is `<bounce />`; the "if" lives in detection.

## The algorithm

- **Screen edges** are checked by position before the move, not swept.
- **Object against object is swept.** Both motions reduce to one; box against box by slabs, circle against box with rounded corners, circle against circle by the two radii. Touching and moving apart is not a hit.
- **The frame loop** (`moveObjects`, `play/frame.cpp`): each round the earliest touch anywhere wins, everything advances to it, that pair's rules run, and the rest of the frame plays on. A pair reacts at most once a frame.
- **What has moved.** A pair is looked at only when one has moved: a velocity, a ride, or being somewhere else than last frame (a hop, a `<set>` of `x`, a teleport, a spawn). Each object keeps last frame's `Moment` in `before`. Put somewhere by a load or a reset between frames is not a move (`placed`); reset in the middle of a frame is. Rejected: a list of verbs that move by place (one would be missed).
- **Hops land before collisions** and are judged where they land: swept, a frog hopping onto a log would first touch the water.

## Fewer checks

Donkey Kong lagged: about 50,000 pairs compared each frame. Kept every touch and its order exactly, and:

1. which pairs a rule answers for is found once per game (`FrameRules::partners`);
2. each pair first checks whether the boxes they cover over the frame meet;
3. a pair too far apart sleeps until a reaction moves one of them;
4. being in play is cached until the stage changes.

A screen grid was rejected: it needs a cell size per game, and what is left after pruning is a few hundred cheap tests. Open: a grid if a game has thousands of movers; `nearest` searches still look at everything.

## Riding, carrying, unless

- **`<ride />`** lends the other's velocity for the frame, recomputed from current touches, so stepping off needs no undo. Rejected: copying the velocity (must be undone), parent/child attachment (an ownership model the language lacks).
- **`<carry />`** is the carrier's: it remembers what and where, lends its motion before the move (so the carried thing's touches run) and places it after. One thing at a time; taking another puts the first down. `<drop />` remembers the dropped thing until they part. Open: several things, a thief that steals only by rule.
- **`unless="class"`** skips a rule while touching that class (Frogger's water unless on a log), so it does not depend on pair order. Rejected: rule order, a "safe" class overriding a "hazard".
- **`<stick />`** corrects only the touched axis, so a pushed object slides along.

## Pixel collisions

- `<type>pixel</type>` is opt-in. The box sweep runs first; only a touching pair is walked in half-pixel steps to the first shared solid pixel. Any alpha counts as solid. Refused on text and images (their pixels are a backend's).
- Rejected: a list of boxes (a second picture that drifts), pixels for everything (costly), an alpha threshold.
- **`<solid char>`** names which inks of a bitmap touch (Q*bert's cube tops); the mask goes with flips and turns. **`<hitbox>`** is a smaller box or circle used for every touch. Rejected: a separate collision picture, polygon hitboxes. Open: a hitbox per look.

## Filters and checks in rules

- **Speed on a touch** is the live number `speed`, held at the speed as the touch happened while the rules run, so a landing rule and a crash rule never both run. `<slower>`/`<faster>` were a second comparison vocabulary beside the conditions, and went.
- **`side=`** is the side of the rule's own object that was touched; same words as `edge=` but a separate attribute, since `edge=` means a screen edge.
- **Checks** reuse the condition tests (`<condition>`, `<any>`; several in a row all hold, so no `<all>`); one with no `object=` means the object itself. Not yet on edge rules.
- **`<trigger>` in rules** runs one of the object's actions, so a death is one action that every killing rule names.
- **Rules in rules** (the author asked, with conditions in conditions): a rule holds rules that say what it says, so Centipede's shot or a creeper's look is written once over the rules it governs. Made at load into the plain rule list (attributes and checks passed down, `<first>` a rule before them), so detection is untouched. Saying an attribute twice stops the load rather than overriding: a rule below one about the shot about the ball reads as a mistake. No `<otherwise>`: every matching rule runs, so "none ran" would need the engine to track the rules of one touch.

## Deflect

Pong's rule: the paddle's middle sends the ball straight back, its ends at 45 degrees. `<deflect>45</deflect>` measures from the side's middle and keeps the speed. Rejected: `<bounce>45</bounce>` (bounce also runs on edges).

## Contacts and knocks

- **A contact is worked out once per touch** (`contactOf`): edges, point, normal, where along each side, depth, time. Each side's rules see it from their own side; commands read `touch.x`, `touch.normal.x` and so on. Chosen over every sweep returning it, or each verb measuring for itself.
- **`<knock>`** is a one-dimensional collision along the normal by `<mass>`, its content the bounce. Nothing happens unless closing, so both sides may say it. No mass is as heavy as can be. Chains of balls pass a knock on with no extra mechanism.
- **Through a thing with no mass** (Mario Bros.' bumped floor): the knock goes to what rests on its far side, which then runs its rules as touched from below. A rule's `through="class"` takes only such felt-through touches.
- Open: spin, rolling friction apart from `<drag>`.

## Bounce by the normal

- Pinball, golf and track walls are not square. **`<bounce>` mirrors the velocity about the touch normal**, as off a surface standing still, keeping its content's share of speed (above 1, a bumper). A box side faces straight out, so every box game plays as before; the tests passed unchanged.
- **Off a still surface, not a moving one.** Counting the other's velocity made two Mario Bros. creepers meeting fling each other. A moving surface is `<knock>`'s job.
- **The pixel normal** (`pixelContactAt`): round the touch point, each side's solid pixels are measured for how they spread; the flattest is the surface. Otherwise middle to middle, which handles the end of a gun barrel.
- Rejected: a normal written on the object, the gradient alone (a thin line gives none). Open: push-out along the normal.
