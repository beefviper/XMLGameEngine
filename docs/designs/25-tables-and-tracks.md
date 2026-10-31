# Tables and tracks

**Status:** stage 1 Built (2026-10-31); stages 2 to 4 Decided, not built.

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
| 3 | `<pivot>` (the point a heading turns about), `<swing>` with a `<rest>`, a turning thing's speed at the point of touch lent by `<knock>` | a pinball table | Decided |
| 4 | `<surface>` on map pieces (drag and grip while over them), `<grip>` turning velocity toward a heading, `<gates>` and laps | a top down racer | Decided |

## Decisions along the way

- **Keys** ([input](08-input.md#a-keys-three-moments-2026-10-31)): Beef's framing of pressed, held, released replaced a `<charge>` verb.
- **Bounce** ([collisions](20-collisions.md#bounce-by-the-normal-2026-10-31)): Beef asked for one rule with no special cases; square things keep the classic flip because their sides are square. A bounce is off a surface standing still; a moving surface lending its speed is `<knock>`'s, so `<kick>` was dropped for `<bounce>` above 1.
- **No `<launch>` yet.** Putt Park sets the ball's `velocity.x` and `.y` from `<sine>` and `<cosine>` of the arrow's heading. A word is added when a second game wants the same.

## Putt Park (stage 1)

- Six holes as scenes of one state; each takes the first hole's and gives its own walls (one picture of `<line>`s, `pixel` collisions), cup, tee, par, and posts or cushions from the kit. A scene condition puts the ball on its tee when the card says it has not been placed.
- The arrow is a bitmap turned by its `<heading>` about its middle, which sits on the ball; its two looks are the same size (all clear while the ball rolls), since `arrow.width` read in the same condition as a `<become>` is still the old look's.
- Space held adds to the arrow's power up to 100, let go strikes; both through `<trigger from="ready">`, so the ball cannot be struck while it rolls. A condition stops the ball dead once its speed is under 0.08 a frame each way and gives the arrow back.
- Open: the cup taking only a slow ball (a rule's condition on speed), water and sand (stage 4's surfaces), more holes, a best round kept.
