# Games as tests

Every game was written to try the language: what each one forced, in three parts. The games as they are today, with their keys, are in [the shipped games](../reference/games.md).

| Part | What |
|---|---|
| [The first games](#the-first-games) | what each early game forced |
| [Games written by another AI](#games-written-by-another-ai) | three games from the schema alone |
| [The Atari 2600 list and after](#the-atari-2600-list-and-after) | the 2600 list, Mario Bros., near misses |

## The first games

**Status:** Every game in `games/descriptions/` is built; most are played frame by frame in `xge/tests/games/test_<game>.cpp`. All were rewritten once with the newer vocabulary, closer to the originals in graphics, sound and play. The list: [the shipped games](../reference/games.md).

### What each game forced

| Game | Chosen to test | Forced (new) | Left out |
|---|---|---|---|
| Pong, Breakout, Space Invaders | the core: bounce, stick, die, fire, lockstep | `<deflect>`, bitmaps, animation, SVG sprites, enemy fire | Breakout's two-hit row and marching block (kept in tests) |
| Frogger | hazards, loops, riding, lives, one-step moves | `dec`, `atmost`, `hop`, `wrap`, `ride`, `unless`, verbs in object rules, colours | crocodiles, lady frog, flies, speed-up |
| Space Race, Freeway | a second game on the same words | nothing | lives in Freeway (kept apart from Frogger) |
| Kaboom | `<random>` in a group; states that differ only in what they show | timers, `<facing>` fire, `<reset object>` in a rule | speed-up within a wave |
| Depth Charge | many charges against a clock | nothing | the charges' depth setting |
| Astrosmash | Kaboom's pieces plus a gun | nothing | levels, guided missiles, the UFO |
| Lunar Lander | gravity, limited thrust, landing by speed, pixel terrain | lines, pixel, acceleration, `burn`, `stop`, a condition on `speed` | scrolling close-up, abort, hard landings |
| Asteroids | facing, coasting, wrapping, breaking | heading, turn, thrust, drag, piece copies, despawn, a distance total, a thrusting pose | saucer shots breaking rocks |
| Pac-Man | a tile maze, steering, four ghosts | `<map>`, `<maze>`, `<go>`, `<hunt>`, `<wander>`, look timers, `<become from>` | later levels' schedules, other fruit |

### Frogger

| Need | Chosen | Rejected |
|---|---|---|
| Lives that end the game at zero | `dec` and `atmost` | counting deaths up (hides the number of lives) |
| One step per press | `hop`, queued and made at the start of the move | a held move stopped by a timer; a one-frame velocity (swept on the way) |
| Looping lanes | `wrap`: acts once fully off screen and heading out, keeps overshoot | `reset` at the far edge (pops in whole) |
| River kills unless on a log | `unless="logs"` | rule order |
| Diving turtles | two looks and a look timer; the turtle's `sprite="under"` rule drowns the frog | a separate hazard fading in |
| Bays that fill | the hedge is a hazard passed over on a bay; a bay has looks `empty` and `home` | a pad that dies |
| Time | a clock object's variable, counted down by a state timer | a timer on the frog |

Lanes are groups inside one `lanes` group that gives the `<wrap />`; each lays its cars out as columns spaced `(window.right + w) / n - w` round the loop.

### Kaboom

- The bucket stack is one object with three looks; a miss is `<become from="two" sprite="one">`. The eight waves are scenes, a `from=` chain each giving only its variables.
- The bomber `<reverse />`s on a random timer and `<fire>`s bombs from under him, so they trail him. A bomb on the ground resets every falling bomb.
- Open: eight scenes of one shape are a case for numbered bundles.

### Astrosmash and Lunar Lander

- Astrosmash: a big rock's rule `<spawn>`s two chips; worth is `self.worth` so one rule serves every size. A check in a rule once failed on `variable="self.worth"`; checks take `self` now.
- Lunar Lander: a landing is one rule about the pads, slower than `safespeed` and nearly upright (an `<any>` of two heading tests). The next lander is the same one reset, so fuel is copied out and back around the reset.

### Asteroids

- Rocks are one group with a group per size and shape; ship and shots share class `breaker`, so each size has one rule for being broken.
- A hit ship returns when the nearest hazard is over 150 away: `distance=` with `total="lowest"`.
- Two saucers, the small one `from=` the big with its timers replaced; they are one group piece `ufos` so rocks can break them.
- **The flame** is a `thrusting` pose over a two-frame animation. That needed looks with a `<heading>`: each look now keeps its own turned pictures. Chosen over a tethered flame object (needs its heading kept in step) and a `<become>` on the key (nothing puts it back).

### Space Invaders

- One group of 11 columns and 5 rows, rows changing the bitmaps. Invaders 2 is the same game from an SVG sprite sheet.
- Tests that used the game as a fixture load a frozen copy, `invaders_fixture.h`.
- Making the bullet thin showed `<fire>` placed its left edge at the shooter's middle; now centred.
- **The march speeds up** with one condition: a count of aliens at most `pace.next` sets the group's speed times 1.3 and halves `next`.
- **A hit holds the game** for a second: a state over it, still, popped by an `<after>` timer.
- Left: the march tempo following the speed (a sound is fixed when made), bombs only from a column's lowest alien, shields wearing down.

### Pac-Man

- The arcade's own maze, 28 by 31 tiles of 16 pixels. Walls are outlined through the middle of their edge tiles, as the arcade does.
- Each ghost's chase is a `<hunt>` variant (ahead, doubled from Blinky, shy within eight tiles). They leave the house by dot counts, with a four-second nudge.
- **Scatter and chase are looks of a `mode` object**, whose look timers run the first level's schedule; each switch turns ghosts round with `<trigger from="normal">`. Chosen over a mode verb: looks already are modes, and the schedule stays in the game.
- A ghost eaten is worth `status.worth`, doubled each time. READY! is a state over the still maze.
- Kept simple: one schedule for every level, one fruit, eyes that do not look where they go.

### Paddle and ball

- **Pong** follows Atari's arcade: square ball, dashed net, first to eleven, faster at the fourth and twelfth hit.
- **Breakout**: eight rows of fourteen, worth 7, 5, 3, 1, each colour an inner group with its worth and pitch; a `half` paddle look once the ball reaches the top.
- **Space Race**: twelve rows of debris as one 8 by 12 group; the win screen picks its words by its own conditions.

## Games written by another AI

**Status:** Built. All three games have since been rewritten.

### What the test showed

The author asked another AI to write three games (Berserk, Demon Attack, Frostbite) from `xgedef.xsd` alone. All three passed the schema and loaded; none could be played. Every error was something the schema accepts where the engine does nothing:

- colours like `white` or `#00ff00` (a colour is a `color.` name);
- key names the engine lacks (`button="fire"`);
- `<move>` or `<fire>` in an `<input>`, `<inc>` in a condition, `<reset object>` in a collision (the last since allowed);
- a variable with no owner (`<inc variable="score">`);
- a spare projectile that started in play and flew off.

**Lesson:** the vocabulary was enough (no tag was added), but the schema could not tell an author a command was in the wrong place.

- **Chosen:** the loader refuses, as errors not warnings, unknown keys and colours, a command where it does nothing, and a change to a variable no object has, each naming what to write instead, with a "did you mean".
- Where a command works is `ruleFor` in `load/check.cpp`; later the schema's command groups said the same ([file format](02-file-format.md#commands-in-groups)).

### The rewrites

- **Demon Attack:** three waves; demons that `<spawn>` two small ones when shot; big demons with `whole` and `hurt` looks; the shot steered over the cannon by a timer that sets `laser.x`, as the original does.
- **Frostbite:** three nights as scenes of a `from=` chain giving the drift; the four rows one group whose `sprite="white"` rule turns its row blue and builds a block of the igloo; the igloo built bottom up by `<reveal>` in order.
- **Berserk:** after Stern's 1980 game. Four rooms as scenes, each a `<map>` of the arcade's five-by-three frame on half-cell tiles; robots a group piece with a member per robot per room; Otto out of play until a room timer reveals him.
  - **Every rule that costs a life is the man's own:** `<reset object="player" />` from another's rule would reset his score and lives too, since a reset object resets every variable.
  - He comes in opposite the exit he took, and that doorway shuts behind him.
  - Eight directions by two keys held; robots fire only when lined up (`<aim>` with `<lined>`).
  - Making it load found two slow spots, both fixed: an expression parser built per value (now per distinct text), and in-play checks per pair (now per object).
  - Differences: straight-line chasing, four fixed rooms in a two-by-two loop (the arcade's grid needs maps made at run time), speeds chosen without watching play.

## The Atari 2600 list and after

**Status:** Built. The first five needed no new verb; each later one was written to try one.

### How the list started

The author listed about thirty non-scrolling Atari 2600 games and asked for five the engine could already say. Already built from the list: Kaboom!, Asteroids, Space Invaders, Demon Attack, Frogger, Freeway, Frostbite, Berzerk, Super Breakout. The five were Pitfall!, Missile Command, Combat, Air-Sea Battle and Megamania; the games after them each tried a new word.

| Game | Written for | Left out |
|---|---|---|
| Pitfall! | scenes from pieces, jumps, ladders | vines, quicksand, the tunnel, 255 screens |
| Missile Command | aiming, tethers, waves as scenes | limited missiles, bombers, smart bombs |
| Combat | headings, thrust, knocks | guided shells, the plane games |
| Air-Sea Battle | shells at an angle, wrapping lanes | the other games on the cartridge |
| Megamania | waves by `from=`, an energy timer | faster waves after the eighth |
| Atlantis | named timers and conditions | the bandit bomber |
| Adventure | carrying, rooms as scenes | most of the kingdom, the bat, the magnet |
| Trick Shot | `<knock>` and `<mass>` | a cue to aim, spin |
| Mario Bros. | a knock through a floor | fireballs, icicles, later phases |
| Q*bert | diagonal hops, a map pyramid | Slick and Sam, Ugg and Wrongway |
| Yars' Revenge | `<seek>` | the ghost of Yar |
| Tennis | altitude, shadows, a computer player | serving, the net stopping a low ball |
| Haunted House | light and dark | the scepter, the nine variations |
| Fishing Derby | values read when used, tethers | the shark's dive |
| Space Invaders shields | `<hole>` | aliens eating the shields |

### Decisions in the games

- **Pitfall!** Screens are scenes built from pieces; each is entered through an unseen strip at each side. A never-drawn `status` object holds score, lives and clock, so resetting Harry costs nothing. A rolling log knocks Harry back, so only a jump passes. Crocodiles are two looks with a timer each.
- **Missile Command.** The base `<aim>`s at the sight and fires; each counter-missile trails a `<tether>`. Its burst is an animation tested by pixel. Waves are scenes, each `from=` the one before.
- **Combat.** One group of tanks with an inner group per player; shells `<knock>` the tank back; pixel tanks bounce off box walls.
- **Air-Sea Battle.** Guns with a heading, pointed by `<set variable="self.heading">`. Plane lanes start off the side they fly in from, so a revealed plane comes back from there.
- **Megamania.** Eight waves as a `from=` chain. Energy is a named timer (`energy.level`) drawn as a bar.
- **Atlantis.** The third wave keeps the second's scene but replaces its `launch` timer and `quota` condition by name. Ships drop a lane at the right edge; a ship cannot leave the screen first, since `<wrap />` is the only edge rule that waits until it is off.
- **Adventure.** One `<carry />` rule for class `item`; dragons wake by a distance holding a flag's test (a condition in a condition). A room's walls in another colour must be written again, since a take replaces a whole sprite.
- **Trick Shot.** The cue tip has no mass, so its knock sends the white ball at up to twice its speed and passes over other balls.
- **Q*bert.** Cubes touch only on their tops (`<solid char>`); Q*bert and Coily touch by their feet (`<hitbox>`); Coily `<hunt>`s by hops.
- **Yars' Revenge.** The neutral zone is `unless="zone"`. The Zorlon cannon is out of play until revealed, so its action does nothing before.
- **Tennis.** Each racket's rule checks the ball's `altitude`. Points are said with `<choose>`.
- **Haunted House.** Light is a variable a match sets and a timer burns down. Floors are map scenes with stairs as ways.
- **Fishing Derby.** The hook's `line` goes 0, 1, 2 so a rising hook takes no second fish; hooks are written before fish so their rule runs first.

### Mario Bros.

- **The knock through a floor:** Mario and the floors have no mass, so a bump from below is a `<knock>0</knock>` passed through the tile to the creeper on it, whose `side="bottom" through="floor"` rule flips it. A creeper hanging off a ledge touched directly is not flipped.
- **Six phases** as scenes, each counting kicks up to its `upto`.
- **One piece per kind from one `pest`** that holds what they share. A sidestepper goes `mad` on the first bump; a fighter fly hops, so a bump reaches it only between hops. Rejected: one group of all kinds (bump rules differ by kind, and rules are a group's).
- **A flip** is a hop that keeps its sideways way. One that gets up by itself turns red by a palette swap; a second time, blue, by a rule on the floor. Rejected: `from=` on a style-only `<become>` (a second meaning for `from=`).
- **Two players at once:** Mario and Luigi are one piece taken twice; Luigi wears green by `<become style="luigi">`. A pest's rules about a brother are written once, for class `brother`, naming him `other`.
- Drawn at 256 by 224 with `<scale>3</scale>`; `<poses>` with `<mirror>`, a `<grip>` of 0.15.

### Found on the way

- **A rule can stop the touch that would come next.** Harry's `<land />` on a crocodile and its jaws meet at one moment; touches at one moment run in file order and a still pair is not looked at again, so the jaws are written before the crocodiles.
- **The tar pit is 6 pixels tall,** 1 pixel into the path: a taller one was touched by a jump just leaving the ground.

### Near misses

- **Seaquest:** `<facing>` follows up and down too, so a torpedo would leave upward; it needs a fire kept to left and right.
- **Dodge 'Em:** cars turning at given corners. **Warlords:** shields sliding round a castle corner.
- **Keystone Kapers** scrolls; **Starmaster** is first person.
- Close to one already written: Solar Storm (Megamania with a paddle), Dragonfire and Montezuma's Revenge (Pitfall's pieces with keys and doors), Boxing.
