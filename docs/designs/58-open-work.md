# Open work

**Status:** a backlog, nothing here is decided. Each item points at the design file that holds the reasoning.

## Language and engine

- **Chasing and aiming** ([chase-and-paths](24-chase-and-paths.md)): aim in 8 ways, lead a target, chase round walls; Galaxian's divers and bombs could `<aim>`. Only the front invader of a column firing ([pools-and-enemies](10-pools-and-enemies.md)).
- **Donkey Kong** ([platforming](23-platforming.md)): sloped girders, barrels down ladders, points for a jumped barrel, the hammer, the rest of the air-control ladder.
- **Galaxian** ([chase-and-paths](24-chase-and-paths.md)): a swaying formation, divers that aim at the ship, the flagship taking escorts, points doubled in a dive, curved path legs. Its speeds and timings were chosen without seeing it played; so were Berserk's. Tune them first.
- **Asteroids**: wrapping and expiring shots, a safe respawn, a saucer, more waves. Lunar Lander could turn with `<heading>` and score by fuel left.
- **Sound** ([sound](33-sound.md)): the march of Space Invaders (four notes in turn, one a step; looping sounds are built and the thrusts use them); an envelope; how the eight new sets sound is unheard by agents, so tune them by ear. Breakout could use `<deflect>`; Pong could serve at a set speed and random angle and speed up per hit.
- **Pictures** ([animation-and-looks](31-animation-and-looks.md)): the rest of the Space Invaders 2 sheet (banking ship and hit flash as looks, explosions as a released pool, the saucer on a timer); looks on animated objects; a palette; per-frame intervals; animation that runs once.
- **Groups** ([groups](06-groups.md)): a bare `<x>` in a member, evenly spaced members.
- **Arithmetic** ([arithmetic](03-arithmetic.md), [vocabulary-map](11-vocabulary-map.md)): move the other games' arithmetic to `<equation>`/`<formula>` if wanted; add `min`, `max`, `negate`, `abs`, `clamp` and a `<random>` step.
- **Repetition** (counted 2026-10-13, before defaults: about 940 lines of the games repeat exactly and 2,200 with only a number or name changed). Done since: [objects](05-objects.md)' defaults, [groups inside a group](06-groups.md), and `<shows>` and `<conditions>` sets ([states-and-conditions](07-states-and-conditions.md)). The numbered bundles (Berserk's rooms ×4, Kaboom's waves ×3) are now scenes inside a state, taking shared pieces from a kit ([scenes](12-scenes.md), 2026-10-15). One rule throughout: take the shared thing, say only what differs.
- **Adventure** ([games-atari](56-games-atari.md)): two rooms are built ([scenes](12-scenes.md)), the carried thing going along; more rooms and castles, the bat.
- **Pac-Man** ([chase-and-paths](24-chase-and-paths.md), [games-classic](54-games-classic.md#pac-man)): scatter waves, a real path search as another verb, points doubling for each ghost on one pellet, the dot counters that release ghosts, fruit and levels, a map taken from a scene kit; tune the speeds by playing it.
- **Games the vocabulary cannot say yet**: see [games-atari](56-games-atari.md).

## Code

- `xgecli --generate`: what is open is under Open in [generator](40-generator.md). `test_generated_games.cpp` checks every generated game's data against the engine's reading and plays it against its file; building the generated programs themselves (GCC and Clang on Linux, no warnings) is still a script to run by hand, and MSVC has not been tried with the current target.

## Known untested

- MSVC: the engine, `xgegui` and the tests build clean on MSVC 19.44 with every backend, and all tests pass; checked by hand, not automatically. Older MSVC versions are untested.
- A default build (`cmake -B build`, no options) and a build with only `-DXGE_WITH_OPENGL=ON` are covered by no automatic check ([backends](50-backends.md)).
- Agents build in a Linux sandbox and cannot watch or listen, so anything about how a game feels is unchecked.
