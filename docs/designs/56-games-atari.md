# Games as tests: five Atari 2600 games

**Status:** Built, with no engine change.

## Five Atari 2600 games

The author listed about thirty non-scrolling Atari 2600 games (2026-10-07) and asked for five that could be written with the engine as it is. Already built from the list: Kaboom!, Asteroids, Space Invaders, Demon Attack, Frogger, Freeway, Frostbite, Berzerk, Super Breakout (as Breakout). The five were picked for variety and because no new verb was needed; none was added.

| Game | What it uses | Left out |
|---|---|---|
| Pitfall! | screens as `<state>`s, entered by Harry's own edge rules (`<inc>` a screen number and `<move>` him to the other side; a condition of the screen `<pop state>`s to the next); `<leap>`, `<land />`, `<climb>`; a `status` object no screen shows, holding score, lives, clock and screen, so `<reset object="harry" />` costs nothing; crocodile jaws as a hidden pool the pond `<reveal>`s and that `<die />` on their own timer | vines, quicksand, the tunnel's walls and its three-screens-a-step, the 255 screens |
| Missile Command | the base `<aim>`s at the sight on a timer and `<fire>`s along the aim; a counter-missile meeting the sight `<release>`s a burst (a pool of circles that `<die />` on their own timer); missiles `<chase>` the nearest city still standing and turn for another when it goes; a missile done with is `<reset />` above the sky with new `<random>`s and counted; waves are states | three bases with ten missiles each, bonus for cities left, splitting missiles, bombers |
| Combat | `<heading>`, `<turn>`, `<thrust>` (a negative one backs up) and `<drag>` for the tanks; a shell fired from the nose; pixel tanks `<bounce />` off box walls; two players, first to five | guided and bouncing shells, the plane games, invisible tanks, the spin when hit |
| Air-Sea Battle | guns with a `<heading>` that `<turn>`, so a shell flies at any angle; lanes of planes as `<group>`s that `<wrap />`, starting off the side they fly in from, so a `<reveal>`ed plane comes back from there; a state timer as the two-minute clock | the guns' three fixed angles (they swing freely, even at the sea), the other games on the cartridge, a winner screen (most hits needs a condition comparing two variables) |
| Megamania | waves as `<group>`s that `<wrap />` (and `<bounce />` up and down for the cookies), each member dropping bombs from a `<facing>down</facing>` on a `<random>` timer; energy as a variable of its own object, run down by a state timer and filled by `<reset object="energy" />` | steering a shot, five of the eight waves, the bonus for energy left |

Found on the way:
- **A rule can stop the touch that would have come next.** Two rules on Harry meet at the same moment when the jaws appear over the crocodile he stands on: `<land />` on the crocodile and the hazard of the jaws. Touches at the same moment are handled in the order of the objects in the file, and `<land />` stops him, and a pair where neither moves is not looked at again that frame, so the jaws were never met. The jaws are written before the crocodiles.
- **A variable cannot be named like an exprtk function** (`floor`); see the [readme](../readme.md#variables).
- **The tar pit is 6 pixels tall**, sitting 1 pixel into the path: a taller one was still touched by a leap that had only just left the ground beside it.

Near misses from the list, each short of something the engine does not have yet (none were started; verbs are another thread's to add):
- **Seaquest**: `<facing>` follows the last move, up and down too, so the sub's torpedo would leave upwards after rising; it needs a fire that keeps to left and right (a facing that only some moves change).
- **Adventure**: carrying a key, a sword or the chalice needs one object attached to another (`<ride />` only lends a velocity while touching; the name `carry` is kept for that), and a dragon killed by a carried sword.
- **Dodge 'Em**: cars turning at the corners of a track (a path could drive the computer's car; the player's needs turns at given places).
- **Q*bert**: diagonal hops on a pyramid; `<hop>` has four directions.
- **Warlords**: shields that slide round the corner of a castle, and computer players.
- **Ms. Pac-Man**: walls that stop a walker without killing it, and ghosts that steer through a maze (`<chase>` goes straight). **Keystone Kapers** scrolls sideways through the store. **Starmaster** is a first-person view.
- Could be written the same way but not picked, being close to one that was: Atlantis (Air-Sea Battle from the other side), Solar Storm (Megamania with a paddle), Dragonfire and Montezuma's Revenge (Pitfall's screens, jumps and ladders, with keys and doors), Boxing (`<aim>` a punch).
