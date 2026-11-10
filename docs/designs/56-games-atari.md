# Games as tests: the Atari 2600 list and after

**Status:** Built. The first five needed no new verb; each later one was written to try one.

## How the list started

The author listed about thirty non-scrolling Atari 2600 games and asked for five the engine could already say. Already built from the list: Kaboom!, Asteroids, Space Invaders, Demon Attack, Frogger, Freeway, Frostbite, Berzerk, Super Breakout. The five were Pitfall!, Missile Command, Combat, Air-Sea Battle and Megamania; the games after them each tried a new word.

| Game | Written for | Left out |
|---|---|---|
| Pitfall! | scenes from pieces, leaps, ladders | vines, quicksand, the tunnel, 255 screens |
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

## Decisions in the games

- **Pitfall!** Screens are scenes built from pieces; each is entered through an unseen strip at each side. A never-drawn `status` object holds score, lives and clock, so resetting Harry costs nothing. A rolling log knocks Harry back, so only a leap passes. Crocodiles are two looks with a timer each.
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

## Mario Bros.

- **The knock through a floor:** Mario and the floors have no mass, so a bump from below is a `<knock>0</knock>` passed through the tile to the creeper on it, whose `side="bottom" through="floor"` rule flips it. A creeper hanging off a ledge touched directly is not flipped.
- **Six phases** as scenes, each counting kicks up to its `upto`.
- **One piece per kind from one `pest`** that holds what they share. A sidestepper goes `mad` on the first bump; a fighter fly hops, so a bump reaches it only between hops. Rejected: one group of all kinds (bump rules differ by kind, and rules are a group's).
- **A flip** is a hop that keeps its sideways way. One that gets up by itself turns red by a palette swap; a second time, blue, by a rule on the floor. Rejected: `from=` on a style-only `<become>` (a second meaning for `from=`).
- **Two players at once:** Mario and Luigi are one piece taken twice; Luigi wears green by `<become style="luigi">`. A pest's rules about a brother are written once per brother, since a rule cannot name "whoever touched me". A word for that (`other`) would be worth having if more two-player games need it.
- Drawn at 256 by 224 with `<scale>3</scale>`; `<poses>` with `<mirror>`, a `<grip>` of 0.15.

## Found on the way

- **A rule can stop the touch that would come next.** Harry's `<land />` on a crocodile and its jaws meet at one moment; touches at one moment run in file order and a still pair is not looked at again, so the jaws are written before the crocodiles.
- **The tar pit is 6 pixels tall,** 1 pixel into the path: a taller one was touched by a leap just leaving the ground.

## Near misses

- **Seaquest:** `<facing>` follows up and down too, so a torpedo would leave upward; it needs a fire kept to left and right.
- **Dodge 'Em:** cars turning at given corners. **Warlords:** shields sliding round a castle corner.
- **Keystone Kapers** scrolls; **Starmaster** is first person.
- Close to one already written: Solar Storm (Megamania with a paddle), Dragonfire and Montezuma's Revenge (Pitfall's pieces with keys and doors), Boxing.
