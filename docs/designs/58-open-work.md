# Open work

**Status:** a backlog, nothing here is decided. Each item points at the design file that holds the reasoning.

## Language and engine

- **Chasing and aiming** ([chase-and-paths](24-chase-and-paths.md)): aim in 8 ways, lead a target, chase round walls, a condition on distance; Galaxian's divers and bombs could `<aim>`. Only the front invader of a column firing ([pools-and-enemies](10-pools-and-enemies.md)).
- **Donkey Kong** ([platforming](23-platforming.md)): sloped girders, barrels down ladders, points for a jumped barrel, the hammer, the rest of the air-control ladder.
- **Galaxian** ([chase-and-paths](24-chase-and-paths.md)): a swaying formation, divers that aim at the ship, the flagship taking escorts, points doubled in a dive, curved path legs. Its speeds and timings were chosen without seeing it played; so were Berserk's. Tune them first.
- **Asteroids**: wrapping and expiring shots, a safe respawn, a saucer, more waves. Lunar Lander could turn with `<heading>` and score by fuel left.
- **Sound** ([sound](33-sound.md)): looping music, which the thrust of Asteroids and Lunar Lander and the march of Space Invaders wait for; an envelope; how the eight new sets sound is unheard by agents, so tune them by ear. Breakout could use `<deflect>`; Pong could serve at a set speed and random angle and speed up per hit.
- **Pictures** ([animation-and-looks](31-animation-and-looks.md)): the rest of the Space Invaders 2 sheet (banking ship and hit flash as looks, explosions as a released pool, the saucer on a timer); looks on animated objects; a palette; per-frame intervals; animation that runs once.
- **Groups** ([groups](06-groups.md)): nested groups, a bare `<x>` in a member, evenly spaced members.
- **Arithmetic** ([arithmetic](03-arithmetic.md), [vocabulary-map](11-vocabulary-map.md)): move the other games' arithmetic to `<equation>`/`<formula>` if wanted; add `min`, `max`, `negate`, `abs`, `clamp` and a `<random>` step.
- **Games the vocabulary cannot say yet**: see [games-atari](56-games-atari.md).

## Code

- Sort `lib/` into folders; rename `collisionData.basic` ([build-and-layout](51-build-and-layout.md)).
- `xgecli --generate`: gaps and drift risks are listed under Open in [generator](40-generator.md). Every generated game builds on GCC and Clang on Linux and on MSVC 19.44 (Visual Studio 2022, `/W4`) with no warnings; nothing checks that automatically, so it is a script to run by hand when the generator changes. A generated game's behavior against the engine's has not been compared on Windows.

## Known untested

- MSVC: the engine, `xgegui` and the tests build clean on MSVC 19.44 with every backend, and all tests pass; checked by hand, not automatically. Older MSVC versions are untested.
- A default build (`cmake -B build`, no options) and a build with only `-DXGE_WITH_OPENGL=ON` are covered by no automatic check ([backends](50-backends.md)).
- Agents build in a Linux sandbox and cannot watch or listen, so anything about how a game feels is unchecked.
