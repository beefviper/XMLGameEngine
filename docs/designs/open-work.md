# Open work

**Status:** a backlog, nothing here is decided. Each item points at the design file that holds the reasoning.

## Language and engine

- **Chasing and aiming** ([chase-and-paths](chase-and-paths.md)): aim in 8 ways, lead a target, chase round walls, a condition on distance; Galaxian's divers and bombs could `<aim>`. Only the front invader of a column firing ([pools-and-enemies](pools-and-enemies.md)).
- **Donkey Kong** ([platforming](platforming.md)): sloped girders, barrels down ladders, points for a jumped barrel, the hammer, the rest of the air-control ladder.
- **Galaxian** ([chase-and-paths](chase-and-paths.md)): a swaying formation, divers that aim at the ship, the flagship taking escorts, points doubled in a dive, curved path legs. Its speeds and timings were chosen without seeing it played; so were Berserk's. Tune them first.
- **Asteroids**: wrapping and expiring shots, a safe respawn, a saucer, more waves. Lunar Lander could turn with `<heading>` and score by fuel left.
- **Sound** ([sound](sound.md)): the eight silent games (Breakout, Frogger, Space Race, Freeway, Depth Charge, Astrosmash, Lunar Lander, Asteroids); looping music; an envelope. Breakout could use `<deflect>`; Pong could serve at a set speed and random angle and speed up per hit.
- **Pictures** ([animation-and-looks](animation-and-looks.md)): the rest of the Space Invaders 2 sheet (banking ship and hit flash as looks, explosions as a released pool, the saucer on a timer); looks on animated objects; a palette; per-frame intervals; animation that runs once.
- **Groups** ([groups](groups.md)): nested groups, a bare `<x>` in a member, evenly spaced members.
- **Arithmetic** ([arithmetic](arithmetic.md), [vocabulary-map](vocabulary-map.md)): move the other games' arithmetic to `<equation>`/`<formula>` if wanted; add `min`, `max`, `negate`, `abs`, `clamp` and a `<random>` step.
- **Games the vocabulary cannot say yet**: see [games-atari](games-atari.md).

## Code

- Sort `lib/` into folders; rename `collisionData.basic` ([build-and-layout](build-and-layout.md)).
- `xgecli --generate`: gaps and drift risks are listed under Open in [generator](generator.md). Verified only with GCC and Clang on Linux; a generated game has never been built on Windows with Visual Studio.

## Known untested

- MSVC warnings: the `/W4` set and system-header handling were written from the documentation and never built there, so a new MSVC warning is likely ours.
- A default build (`cmake -B build`, no options) and a build with only `-DXGE_WITH_OPENGL=ON` are covered by no automatic check ([backends](backends.md)).
- Agents build in a Linux sandbox and cannot watch or listen, so anything about how a game feels is unchecked.
