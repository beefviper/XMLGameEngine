# Games as tests: the first games

**Status:** Every game in `games/descriptions/` is built; most are played frame by frame in `xge/tests/test_<game>.cpp`.

## At a glance

| Game | Chosen to test | Forced (new) | Left out |
|---|---|---|---|
| Pong, Breakout, Space Invaders | the core: bounce, stick, die, fire, lockstep blocks | `<deflect>` (Pong), bitmaps, animation, SVG sprites, enemy fire (Invaders 1 and 2) | |
| Frogger | hazards, loops, riding, lives, one-step moves | `dec`, `atmost`, `hop`, `wrap`, `ride`, `unless`, verbs in object rules, colors | (rewritten 2026-10-19 with diving turtles, a timer and filled bays; crocodiles, lady frog, flies and speed-up still missing) |
| Space Race | a second game on the same vocabulary | nothing | [rewritten](#the-paddle-and-ball-rewrite) |
| Kaboom | `<random>` in a `<group>`; states differing only in what they show | nothing at first; when reworked, timers, `<facing>` fire, `<reset object>` in a rule | speed-up inside a wave (eight waves stand in) |
| Freeway | Frogger's road is a vocabulary, not a kit; two players | nothing | (timed round and knock-back built in the 2026-10-19 rewrite) |
| Depth Charge | limited ammunition without a countdown; rewritten 2026-10-19 as the arcade game: pairs of charges, mines, a clock | nothing | the time given back for a high score, the charges' depth setting |
| Astrosmash | Kaboom's pieces plus a gun | nothing | levels, guided missiles, the UFO |
| Lunar Lander | gravity, held limited thrust, landing by speed, pixel terrain; rewritten 2026-10-19 with turning, scored pads and lander after lander | lines, pixel, acceleration, accelerate/burn, stop, slower/faster | the scrolling terrain and close-up, abort, hard landings |
| Asteroids | facing, coasting, shots from a nose, wrapping, breaking | heading, turn, thrust, drag, release of kit copies, despawn, amounts | two saucer sizes, wrapping and expiring shots, a safe respawn |
| Pac-Man | a tile maze, steering, four ghosts each chasing its own way | `<map>`, `<maze>`, `<go>`, `<hunt>`, `<wander>`, bitmap inks, look timers, `<become from>` | the arcade's schedule of scatter waves, the dot counters that let ghosts out, the other fruit ([below](#pac-man)) |
| Berserk, Demon Attack, Frostbite (first written by another AI; all three since rewritten) | can the schema alone be enough for an author | (found gaps) then timers, facing, jump, looks, key sets; later chasing and aiming | |
| Pitfall!, Missile Command, Combat, Air-Sea Battle, Megamania | five Atari 2600 games picked because today's verbs already say them | nothing | see [below](56-games-atari.md#how-the-list-started) |


## Frogger

- A 48-pixel grid (13 by 14): score, time and lives row, five bays in a hedge, five river lanes, the bank, five road lanes, the pavement. Pictures are multi-ink bitmaps (frog, logs, turtles, truck, cars, bulldozers). Lanes are `<group>`s inside one `lanes` group that gives `<wrap />` and `<rows>1</rows>`; each lane lays its logs or cars out as `<columns>` with a gap of `(window.right + w) / n - w`, so they are evenly spaced round the loop.
- Needs and what was chosen:

| Need | Chosen | Rejected |
|---|---|---|
| Lives that end the game at zero | `dec` and condition `atmost` | counting deaths up to 3 (hides the number of lives in the condition); comparison operators; a lives object |
| One step per press, however long held or paused | `hop` queued by the press, made at the start of the next move | a held `move` stopped by a timer; a flag on `<input>`; a one-frame velocity (swept along the way) |
| Looping lanes | `wrap`: acts only once fully off screen and still heading that way, keeps overshoot | `reset` at the far edge (pops in whole, snaps to the start); teleport on touching an edge; duplicates at both ends |
| Riding a log | `ride` ([collisions](20-collisions.md)) | |
| River kills unless on a log | `unless="logs"` on the rule ([collisions](20-collisions.md)) | |
| Diving turtles | two looks and a look timer each (`up` after 3.5 s becomes `under`, back after 1.5 s); the turtle's own rule `sprite="under"` triggers the frog's `drown` | a separate hazard object fading in |
| Bays that fill | the hedge is one strip of class `hazard`, passed over on a bay (`unless="pads"`); a bay has looks `empty` and `home`, and its rules (filled first) trigger the frog's `home` or `lose` | a green frog under each pad and the pad `<die />`ing (the first version) |
| Time | the clock object's `time`, a state timer `<dec>`s it, every new frog does `<reset object="clock" />`, at zero the frog's `lose` | a timer on the frog |

- A frog ridden off the side dies at the edge by an ordinary `edge="horizontal"` rule. A frog home scores 50 plus 10 a second left; five home wins.
- Known quirk: a frog put back by the turtle's rule is still moved by its own `ride` that frame (a turtle's speed, under 2 pixels).


## Kaboom

- An 800 by 600 window, a bucket stopped at the sides by `<stick />`, eight waves as on the cartridge. The buckets are one object with three looks (a stack of three, two, one); a miss does `<become from="two" sprite="one">` then `from="three"` to `two`, so the stack and its catching area shrink. The waves are [scenes](12-scenes.md) of the playing state, a `from=` chain each giving only its `<variables>` (`wave`, `upto` score, `pace`, `drop` seconds, `fall`, `points` 1 to 8); the first wave's condition `reached` resets the bombs and enters its way `next`, the eighth writes its own `reached` that wins.
- The Mad Bomber (a multi-ink bitmap) paces the rooftop, `<reverse />`s on a timer with a random wait, and `<fire>`s a bomb, a copy of the kit's bomb, every `drop` seconds; he has `<facing>down</facing>`, so bombs leave from under him and come in trails and clusters that follow him. Bombs are animated (a fizzing fuse). A caught bomb `<despawn />`s. A bomb on the ground does `<reset object="bombs" />`: every falling bomb at once, so one miss costs one bucket. The tests play wave one with an automatic player and check the last wave is harder.
- History: first a looser game (Gem Catcher), then the arcade rules with bombs in six fixed columns, `<random>` speeds drawn once, and a `tally` object a condition turned into the explosion (a rule could not reset other objects then). Reworked with timers, then (2026-10-19) with scenes for all eight waves and the bucket stack as looks.
- Showed: a group's shared part can hold `<random>` and each member still draws its own numbers; keys held across a state change keep moving the bucket.
- Rejected: a new speed on every reset (`<random>` re-drawn at runtime), speed rising within a wave (eight fixed speeds stand in). Open: eight scenes of the same shape are a case for numbered bundles (Beef's call).


## Freeway

- 800 by 600, 50-pixel cells, twelve rows: far side, ten lanes (five driving left on top, five right below, faster towards the middle, two of them trucks), near side. Two chickens are one `<group>` with two members; each scores in its own `self.score`. Two key sets (W/S and Up/Down). A 2:16 round: the global `seconds` read by a `clock` object, a state timer counts it down, at zero `wins` is pushed with its `draw` look; that state's own conditions (`<above object variable>`) pick the winner's look.
- The traffic is one group of all ten lanes, which gives the `<wrap />` and the car bitmap; a lane says its row, speed and colour, truck lanes their wider bitmap, and lays its cars out as `<columns>` evenly round the loop.
- The far side is an object (`class="farside"`) the chicken has a rule for; a hop into that row is judged where it lands, so it scores at once. A car hits a chicken that hops into it because a hopped object counts as moving; two stationary objects are never checked. A hit is `<move direction="down">cell</move>` in the chicken's rule: knocked back one lane, as on the cartridge.
- Rejected: lives (kept different from Frogger); sending a hit chicken to the start (far too costly near the far side).


## Depth Charge

- Rewritten 2026-10-19, closer to the arcade game: a destroyer bitmap that rolls a pair of charges off both sides on Space (two `<spawn>`s of the kit's charge, at most six in the water, at positions read as Space is pressed), ten submarines in four lanes as one group with a group per lane (its color, depth, speed and `worth`, scored with `self.worth`: 20 to 80, deeper is more), mines `<release>`d by the submarines on random timers that rise and cost a ship under the destroyer, a burst where a boat goes down, a 90-second clock and three ships, the sonar as the playing state's `<music>`.
- First version (2026-09-30): one charge at a time and a count of misses (`ship.charges` started at the misses allowed and only the floor rule took one off), nine boats to sink. Replaced because the arcade game is about many charges in the water against the clock.
- Rejected: two keys, one per side, as the arcade's two buttons (the key convention keeps Space as the one fire key); a sunk boat coming back from the side it came in (a reveal brings it back where it started).


## Astrosmash

- Eight rocks (four slow, four fast) fall fixed columns with `<random>` speeds and heights; a ship fires one shot at a time. A shot kills (`<die />` on the shot, `<inc>` then `<reset />` on the rock: `<die />` is for things that stay dead). A rock reaching the ground or the ship costs a life. 20 points win, five lives lost is game over. Both groups share class `rocks`.
- The shot is a copy fired from the kit and despawned when it hits or leaves, so nothing waits on screen in the rocks' way.
- Rejected: splitting rocks (there was no way then; `<release>` of kit copies now does it in Asteroids), drifting (needs wall rules, harder columns), a free miss (unloseable by neglect), differently scored rocks (an amount on `inc` exists now).
- Rewritten 2026-10-19 (418 to 362 lines) nearer the Intellivision game: no win any more, it goes on until the last base; rocks fall at a slant and bounce off the sides; a big rock scores 10 and the shot's rule about `boulders` `<release>`s two chips, copies of a kit piece; small rocks 20, spinners 40; a landed rock takes half its worth off the score (`max(0, ...)` in a `<set>`), a landed spinner or anything on the base costs it and resets every rock (the sky clears); a spare base each 1,000 (a condition comparing the score with the ship's `spare`); the peak score shown. All rocks are one group of two inner groups, sharing their rules through `self.worth`.
- Found: a split rule on the rocks with a `<condition>` seemed to fire for small rocks too, so the split was put on the shot. Looked at again 2026-10-23: a check with no `object=` does look at the member running it, in a group of groups as anywhere (it did on 2026-10-19 too, built again to see); what failed was `self` in a check (`variable="self.worth"`), which commands understood and checks did not. Checks take `self` now, and the split is back on the rocks. Also a test that sets a variable directly must use `Game::setVariable`, or the live numbers a command reads stay stale.
- Not in it: levels (multiplier, speed, sky colour), guided missiles, the UFO, rapid fire by holding Space (a `<fire>` is a press, not a hold).


## Lunar Lander

- An 800 by 600 window; a moon of `<line>`s with three gaps for short thick green pads worth 2X, 3X and 5X (100, 150 and 250 points), narrower for more; gravity from the lander's `<acceleration>`. Decisions are in [collisions](20-collisions.md), [motion](22-motion.md), [pictures](30-pictures.md).
- Rewritten 2026-10-19: the lander turns (`<heading>`, `<turn>`) and fires one engine along its heading (`<thrust burn="fuel">`). A landing is one rule about the pads: slower than `safespeed` and nearly upright, an `<any>` of two `<condition>`s of the heading (0 to 10 or 350 to 360); any other touch of class `ground` is a wreck, which `<release>`s four pieces of debris and costs 200 fuel. Each pad's own rule sets what the landing is worth. The next lander is the same one put back with `<reset object="lander" />`, which starts its variables over too, so the fuel is `<copy>`d out to `status.fuel` and back in around the reset (the score is `status`'s); the game is over when what was copied out is nothing. A landing pushes a state that holds the lander on the pad for 2.5 seconds; a wreck shows a message whose timer says when the next one comes. Looked at: the fuel kept only on `status` (a `<thrust burn>` burns the thrusting object's own variable).
- Not done: the scrolling terrain and the close-up view, the abort button, hard landings (between safe and wrecked), the speeds and altitude on the screen (a text cannot show a value that is not a variable).


## Asteroids

- Rewritten 2026-10-19, closer to the arcade game: a three-line arrowhead ship, four big rocks from the edges at random speeds that break into two mediums, then two smalls (20, 50, 100 points), endless waves (a cleared wave puts the rocks back as they started, their `<random>`s drawn again), hyperspace on S, a ship every 10000 points, one aiming saucer (1000 points) on a random timer, the heartbeat as the playing state's `<music>`, and the best score kept. The ship and its shots share the class `breaker`, so each rock size has one rule for being broken by either, and a ship hit is out of play until a two-second state timer `<reveal>`s it at the middle. Rocks are one group with a group per size and per shape. Left out: two saucer sizes, the flame, shots wrapping and running out, a respawn that waits for a clear middle.

## Space Invaders 1 and 2

- Invaders 1 was a grid of identical rectangles; redrawn with three alien kinds (squid, two rows of crab, two of octopus) as a group of three grids of two named bitmaps animated on a one-second interval (since one group of 11 columns and 5 rows whose rows change the bitmaps, [objects](05-objects.md)), a bitmap cannon and a thin bullet. Making the bullet thin showed `<fire>` put its left edge, not its middle, at the middle of the shooter; now centered (this moved Depth Charge's and Astrosmash's shots by half their width). Invaders 2 is the same game drawn from an SVG sprite sheet with three-frame animations a half second apart ([pictures](30-pictures.md)). In both, and in Demon Attack, the aliens fire back copies of the kit's bomb on random timers, and the cannon has three lives.
- Moving the game to three kinds broke tests that borrowed it as a block fixture, so those load `xge/tests/invaders_fixture.h`, a frozen copy of the first game.
- Rewritten nearer the arcade game (2026-10-19), both files alike, with no new verb: each row's worth a row `<variable>` scored with `self.worth` (30, 20, 10); a mystery ship revealed by a state timer, worth `50 * ceil(self.worth)` of a random `worth`; a blast `<release>`d where a hit was (Invaders 2 uses the sheet's explosion frames and its animated saucer); the march as the playing state's looping `<music>`; a best score `<keeps>`; a green ground line; shots and bombs that meet put each other away; the invasion lost when the block's lowest row (`total="highest"` of `y`) reaches the shields. **The march speeds up** with one condition: `total="count"` of the aliens `<atmost>pace.next</atmost>` sets the whole group's `velocity.x` to a corner cell's times 1.3 (dead cells keep their velocity, as lockstep bounces turn them too) and halves `next`. **A hit holds the game still** for a second: the rule counts `struck`, a condition pushes `blasted`, which shows playing still and pops on a state timer `<every>1</every>`: a state's timers are not started over when it is pushed, so an `<after>` there fires once a game, while an `<every>` counts only while the state is current and starts each visit at nought. Dropped: the settings stub. Left: the march's tempo following the speed (a sound is fixed when made), bombs from the lowest alien of a column only, the cannon's lives as pictures, the aliens wearing the shields down.

## Pac-Man

- The arcade's maze, 28 by 31 tiles of 16 pixels in a 448 by 576 window (the arcade's screen at twice its size), drawn as a `<map>` in wall characters. It was first a smaller maze of its own (19 by 22); in 2026-10 Beef asked for the games to come closer to the originals, and the arcade's own layout is what Pac-Man players know. The pieces were worked out from the arcade's block layout: a line between two wall tiles is drawn unless both sides of it are wall, which outlines every block through the middle of its edge tiles, as the arcade does.
- Ghosts: Blinky hunts Pac-Man, Pinky four tiles ahead of him, Inky doubles the point two ahead away from Blinky, Clyde hunts until within eight tiles and then heads for his corner. They leave the house on paths after 1, 5 and 10 seconds (Blinky starts outside). A pellet turns them blue (they wander, slower) for six seconds; an eaten ghost is eyes that hunt the house through the door and turn normal inside.
- Speeds are the arcade's first level at twice its size (Pac-Man 2 pixels a frame, ghosts 1.875, blue 1). Each time a ghost is itself again it scatters to its own corner (members of a `corners` group off the maze) for seven seconds, then chases; a ghost eaten is worth `status.worth`, which a pellet sets to 200 and each ghost eaten doubles (a `<set>` read as it runs). A cherry (an object of two looks) shows after 70 and 170 dots for 9.5 seconds; a cleared maze is the next round, the ghosts a sixteenth faster up to 2.125 and blue a second less; a life at 10,000; the best score `<keeps>`. READY! is a state over the still maze (`caught`, and `ready`, a copy with the opening tune), popped by an `<every>` timer, since an `<after>` in a state goes off only once a game.
- Kept simple: one scatter after each fright rather than the arcade's waves; ghosts leave the house on timers; one fruit; no slowing in the tunnel; the ghosts' eyes do not look where they go; speeds unplayed by agents.

## The paddle and ball rewrite

- Asked (2026-10-19): rewrite every game with the vocabulary added since it was written, fewer lines, and closer to the original in graphics, sound and play. Pong, Pong (minimal), Breakout and Space Race here; Combat, Trick Shot and Tennis in [the Atari games](56-games-atari.md).
- **Pong** follows Atari's arcade: a square ball, a dashed net (a group of one column), big digits by the net, first to eleven, the speed up at the fourth and twelfth hit (a second `class="paddle"` rule with an `<any>` of two `<condition>`s). Dropped: the settings screen, the logo and the image paddles; image sprites keep their tests in `test_size_expressions`' own game.
- **Breakout** follows the arcade: eight rows of fourteen, two of each colour, worth 7, 5, 3, 1, each with its pitch. The wall is one group whose rule scores `self.worth`, and each colour an inner group giving its rows, worth and sound. The ball speeds up at the fourth and twelfth hit and at the first brick in the back four rows (`<all>` of its y and a flag), and the paddle has a `half` look taken when the ball reaches the top. Three balls, two walls. Dropped: the two-hit grey row and the marching block (looks counting hits and lockstep keep their tests in `test_gameplay_verbs` and the Space Invaders tests).
- **Space Race** follows the arcade: twelve rows of debris as one group of 8 by 12 cells with `<row number="1 5 9">` speeds, a clock bar sinking out of sight in a minute, the most crossings win. The win screen picks its words with its own conditions, since nothing scores the last point.
