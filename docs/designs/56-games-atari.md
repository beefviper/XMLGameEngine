# Games as tests: the Atari 2600 list and after

**Status:** Built. The first five needed no new verb; the later ones were each written to try one.

## How the list started

The author listed about thirty non-scrolling Atari 2600 games (2026-10-07) and asked for five that could be written with the engine as it was. Already built from the list: Kaboom!, Asteroids, Space Invaders, Demon Attack, Frogger, Freeway, Frostbite, Berzerk, Super Breakout (as Breakout). The five (Pitfall!, Missile Command, Combat, Air-Sea Battle, Megamania) were picked for variety and because no new verb was needed. The games after them were each written for a word the engine had just gained.

| Game | Written for | Left out |
|---|---|---|
| [Pitfall!](#pitfall) | scenes from a kit, leaps, ladders | vines, quicksand, the tunnel, the 255 screens |
| [Missile Command](#missile-command) | aiming, tethers, waves as scenes | limited counter-missiles, city bonus, splitting missiles, bombers, smart bombs |
| [Combat](#combat) | headings, thrust, knocks | guided and bouncing shells, the plane games, invisible tanks |
| [Air-Sea Battle](#air-sea-battle) | shells at an angle, wrapping lanes | planes falling as they are hit, the other games on the cartridge |
| [Megamania](#megamania) | waves by from=, an energy timer | faster waves after the eighth, a blaster per 10,000 |
| [Atlantis](#atlantis) | named timers and conditions | the bandit bomber, rebuilding the city |
| [Adventure](#adventure) | carrying, rooms as scenes | most of the kingdom, Rhindle, the bat, the magnet |
| [Trick Shot](#trick-shot) | `<knock>` and `<mass>` | a cue to aim, spin |
| [Mario Bros.](#mario-bros) | a knock through a floor | sidesteppers, fighter flies, coins, fireballs, a second player |
| [Q*bert](#qbert) | diagonal hops, a map pyramid | Slick and Sam, Ugg and Wrongway, more rounds |
| [Yars' Revenge](#yars-revenge) | `<seek>` | the ghost of Yar, harder levels |
| [Tennis](#tennis) | altitude, shadows, a computer player | serving, the net stopping a low ball |
| [Haunted House](#haunted-house) | light and dark | the scepter, the urn as one thing, the nine variations |
| [Fishing Derby](#fishing-derby) | values read when used, tethers | reeling by itself, the shark's dive |
| Space Invaders shields | `<hole>`: four bitmap shields, each shot or bomb cutting a ragged hole | the aliens eating the shields |

## The games

### Pitfall!

- Screens are the `<scenes>` of the playing state, built from a kit of pieces (path, logs, tar, pond and crocodiles, fire, scorpion, treasure; each screen's treasure worth its scene variable `worth`). Each is entered through an unseen strip down each side (`<enter>`, and Harry's rule for that side `<move>`s him to the other).
- Harry, the jungle and the HUD are the state's own; `<leap>`, `<land />`, `<climb>`.
- A rolling log knocks Harry back the way it rolls (a collision `<move>`) with a thud and 50 points, so only a leap gets past it.
- A never-drawn `status` object holds score, lives, clock and treasures found (a `<remaining>` condition sees only the current screen), so `<reset object="harry" />` costs nothing.
- Crocodiles have two looks, shut and open, each with a timer turning it to the other; open ones bite.
- Redrawn 2026-10-19 in several inks, the ladder as `<line>`s, the 2600's death tune, a kept best score.

### Missile Command

- The base `<aim>`s at the sight on a timer and `<fire>`s along the aim; each counter-missile trails a `<tether>` back to the base.
- Meeting the sight it `<release>`s a burst whose `<animation>` swells and shrinks, tested by pixel, that `<die />`s on a timer.
- Missiles are one-line streaks with a `<heading>`, `<chase>`ing the nearest city; one done with is `<reset />` above the sky with new `<random>`s and counted.
- The waves are scenes, each `from=` the one before with its speed, total, colour and `way`.

### Combat

- `<heading>`, `<turn>`, `<thrust>` (a negative one backs up) and `<drag>` for the tanks; a shell fired from the nose; pixel tanks `<bounce />` off box walls; two players.
- One group of tanks with an inner group per player; shells `<knock>` the tank hit back; the engine is a looping sound; 2:16 of play, the most hits winning.

### Air-Sea Battle

- Guns with a `<heading>`, so a shell flies at any angle, pointed one of three ways by `<set variable="self.heading">`.
- Lanes of planes are groups that `<wrap />`, starting off the side they fly in from, so a `<reveal>`ed plane comes back from there.
- A state timer is the clock (2:16), a drone is the battle's `<music>`, and the winner screen comes from conditions comparing the two scores.

### Megamania

- Eight waves as a `from=` chain, each its picture, rows, points and velocity; groups that `<wrap />` (and `<bounce />` for the cookies), each member dropping bombs on a `<random>` timer.
- Energy is a named timer on its own object (`energy.level` its time left), drawn as a bar; the energy left is scored ×10 on a clear.
- The shot is steered by a laser timer setting `laser.x` every frame.

### Atlantis

- Three waves as `<scenes>`, each `from=` the one before, the third with its own `launch` timer and `quota` condition in place of the copied ones.
- The ships are a kit group of four lanes, revealed at the top by the wave's timer; each lane's right edge `<die />`s one and `<reveal>`s one in the lane below; the lowest `<wrap />`s and fires death rays.
- Three posts with fixed `<heading>`s fire along them.
- Left out also: a ship leaving the screen before it drops a lane (it vanishes at the edge, since `<wrap />` is the only edge rule that waits until it is off).

### Adventure

- The hero `<carry />`s what he walks into (one rule for class `item`); Space `<drop />`s it.
- Five rooms as scenes from one kit, with four unseen ways out; castles are pixel bitmaps; each portcullis dies on its own key.
- Yorgle and Grundle sleep until near (an `<all>` of a distance and an `awake` flag), `<chase>` at their room's `dragonspeed`, and die on the sword. The chalice carried into the gold castle's hall wins.
- A slain dragon vanishes rather than lying there; a scene can only replace a whole sprite, so a room's walls in another colour are written out again.

### Trick Shot

- Six balls racked against the white one, ball against ball `<knock />`, the cloth a `<drag>`, the cushions the window's edges, six pockets.
- The cue tip has no mass, so its knock sends the white ball off at up to twice its speed, and it passes over the other balls.

### Mario Bros.

- **The knock through a floor:** Mario and the floors have no mass, so his head against a floor tile from below is a `<knock>0</knock>` that goes through the tile to the shellcreeper standing on it, whose rule `side="bottom" through="floor"` flips it. Floors are tiles in groups inside a group, so a bump reaches only its tile. Through-only rules mean Mario jumping into a creeper hanging off a ledge is a touch (deadly or a kick), not a flip.
- **Coming in:** the shellcreepers start out of play and state `<after>` timers `<reveal>` them inside the top pipes; on the ground deep in a bottom pipe they are `<reset />` into their top pipe.
- **A flip** is a hop that keeps its sideways way, so one bumped near a floor's end comes down a floor lower; it stops sliding once down. A second bump or the 8-second timer gets it up; one that gets up by itself turns red, a palette swap (`<become style="angry">`). Two walking into each other `<bounce />` apart.
- **The POW block's** looks count its bumps left; it triggers an action on the whole group that starts with a `<leap>`, so one in the air is spared. Its rules are written last look first.
- **Drawn** at the arcade's 256 by 224 with `<scale>3</scale>` and Press Start 2P: Mario's `<poses>` with `<mirror>`, a `<grip>` of 0.15 so he gets going and slides, a pull of his own so his jump is floaty; after a hit he blinks (`mario.blink`) and is safe for two seconds.
- Open: a third colour (blue) for a shellcreeper that gets up twice.

### Q*bert

- Four keys each a diagonal `<hop>` with its own `<x>` and `<y>`, taking `<seconds>` with an `<arc>`.
- The pyramid is a `<map>`: 28 cubes in seven rows, each one bitmap of three inks, touching only on their tops (`<solid char>`); a cube's rule for its unlit look lights it and scores once. The void around it is unseen tiles that cost a life.
- Q*bert and Coily touch by their feet (`<hitbox>`); Coily hatches at the bottom and `<hunt>`s Q*bert by hops. The discs are map pieces whose looks say whether they are used.
- Won by counting cubes lit (`player.cubes`); drawn at 256 by 240 with `<scale>3</scale>`.

### Yars' Revenge

- The destroyer missile `<seek>`s Yar a few degrees a frame; the neutral zone is an `unless="zone"` on Yar's rule about the missile, drawn as animated bands.
- The shield is a `<map>` in the cartridge's shape with the Qotile as its one `Q`. A bite or a touch of the Qotile `<reveal>`s the Zorlon cannon, whose action fires the blast (an object out of play does not act on a key).
- The Swirl is `<spawn>`ed at the Qotile, waits a second, then `<chase>`s where Yar is.

### Tennis

- The ball falls and bounces with its `<shadow>` under it; each racket's rule on it has a `<condition>` of its `altitude` and `<set>`s it back up and away, angled by where it was hit.
- The far racket is a `<computer>` on J and L; points are said with `<choose>`; deuce and advantage by `<all>`/`<any>` conditions; a six-game set.

### Haunted House

- The eyes light a little, a match far (`<set variable="player.light">`, burnt down by a timer), every match counted.
- Four floors as `<scenes>`, each a `<map>`, stairs as ways `up` and `down`. The master key carried opens the locked doors, a door's rule checking the key is near (`<condition distance>`).
- Kit creatures each floor places: the tarantula `<hunt>`s and the bat `<wander>`s along the halls (`<maze>`), the ghost `<chase>`s through walls. The win is all three pieces and the eyes back at the front door.

### Fishing Derby

- The lines are `<tether>`s from the rod tips to the hooks; you on the left pier, a `<computer>` on the right.
- A free hook `<carry />`s the fish it touches. The hooks are written before the fish, so the hook's rule runs first: its `line` goes 0 → 1 (bite) → 2 (claimed by the fish's own rule), so a hook on its way up takes no second fish.
- A fish at the surface adds `self.worth` to its fisherman; the shark eats a fish on a line; first to 99.
- Left out also: a `<move>` key sets the hook's velocity on every key change, so the hook rises only while up is held.

## Found on the way

- **A rule can stop the touch that would have come next.** When the jaws open over the crocodile Harry stands on, `<land />` on the crocodile and the jaws' hazard meet at the same moment. Touches at one moment run in file order, `<land />` stops him, and a pair where neither moves is not looked at again that frame, so the jaws were never met. The jaws are written before the crocodiles.
- **A variable cannot be named like an exprtk function** (`floor`); see the [readme](../readme.md#variables).
- **The tar pit is 6 pixels tall**, sitting 1 pixel into the path: a taller one was still touched by a leap that had only just left the ground beside it.

## Near misses from the list

Each was short of something the engine did not have (none started):

- **Seaquest**: `<facing>` follows the last move, up and down too, so the sub's torpedo would leave upwards after rising; it needs a fire that keeps to left and right.
- **Dodge 'Em**: cars turning at the corners of a track (a path could drive the computer's car; the player's needs turns at given places).
- **Warlords**: shields that slide round the corner of a castle.
- **Ms. Pac-Man**: could now be written on Pac-Man's maze verbs. **Keystone Kapers** scrolls sideways. **Starmaster** is a first-person view.
- Close to one that was written: Solar Storm (Megamania with a paddle), Dragonfire and Montezuma's Revenge (Pitfall's screens, jumps and ladders, with keys and doors), Boxing (`<aim>` a punch).
