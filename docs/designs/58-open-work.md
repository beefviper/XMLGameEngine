# Open work

**Status:** a backlog, nothing here is decided. Each item points at the design file that holds the reasoning.

## Language and engine

- **Chasing and aiming** ([chase-and-paths](24-chase-and-paths.md)): lead a target, chase round walls; Galaxian's divers and bombs could `<aim>`. Only the front invader of a column firing ([pools-and-enemies](10-pools-and-enemies.md)).
- **Donkey Kong** ([platforming](23-platforming.md)): sloped girders, barrels down ladders, points for a jumped barrel, fireballs, the other three screens, the rest of the air-control ladder.
- **Galaxian** ([chase-and-paths](24-chase-and-paths.md)): a swaying formation, divers that aim at the ship, the flagship taking escorts, points doubled in a dive, curved path legs. Its speeds and timings were chosen without seeing it played; so were Berserk's. Tune them first.
- **Asteroids**: wrapping and expiring shots, a respawn that waits for a clear middle, the small saucer beside the large. Lunar Lander: hard landings, and the speeds and altitude on the screen.
- **Sound** ([sound](33-sound.md)): the march of Space Invaders keeping time with the block (it is a fixed loop, the playing state's music); an envelope; how the eight new sets sound is unheard by agents, so tune them by ear. Breakout could use `<deflect>`; Pong could serve at a set speed and random angle and speed up per hit.
- **Pictures** ([animation-and-looks](31-animation-and-looks.md)): the rest of the Space Invaders 2 sheet (banking ship and hit flash as looks); a third colour in a palette swap (Mario Bros.' blue shellcreeper); per-frame intervals; animation that runs once.
- **Groups** ([groups](06-groups.md)): a bare `<x>` in a member, evenly spaced members.
- **Arithmetic** ([arithmetic](03-arithmetic.md), [vocabulary-map](11-vocabulary-map.md)): move the other games' arithmetic to `<formula>` if wanted; `negate`, `floor`, `sign` as tags.
- **Repetition** (counted 2026-10-13, before defaults: about 940 lines of the games repeat exactly and 2,200 with only a number or name changed). Done since: [objects](05-objects.md)' defaults, [groups inside a group](06-groups.md), and `<shows>` and `<conditions>` sets ([states-and-conditions](07-states-and-conditions.md)). The numbered bundles (Berserk's rooms ×4, Kaboom's waves ×3) are now scenes inside a state, taking shared pieces from a kit ([scenes](12-scenes.md), 2026-10-15). One rule throughout: take the shared thing, say only what differs.
- **Berserk** ([games-written-by-ai](55-games-written-by-ai.md)): the arcade's far bigger grid of rooms, each maze worked out from where its room is (needs maps made as the game runs); robots' speed and colour by score rather than by room. Its four room maps take one map of the kit and give only their rows (2026-10-28, [kit](13-kit.md)).
- **Adventure** ([games-atari](56-games-atari.md)): five rooms are built ([scenes](12-scenes.md)), the carried thing going along; the bat, Rhindle, the rest of the kingdom.
- **Pac-Man** ([chase-and-paths](24-chase-and-paths.md), [games-classic](54-games-classic.md#pac-man)): the arcade's schedule of scatter waves, a real path search as another verb, the dot counters that release ghosts, the other fruit, ghosts slowed in the tunnel; tune the speeds by playing it.
- **After the 2600 vocabulary** (2026-10-17): decals painted onto a picture ([pictures](30-pictures.md)); light blocked by walls and fading at its edge; a shadow that shrinks as its thing rises; Tennis's serving. Speeds in Tennis and Haunted House were chosen without playing them.
- **Games the vocabulary cannot say yet**: see [games-atari](56-games-atari.md).

## Code

- `xgecli --generate`: what is open is under Open in [generator](40-generator.md). `test_generated_games.cpp` checks every generated game's data against the engine's reading and plays it against its file; building the generated programs themselves is by hand: `scripts/shell/windows/games-run-all.bat` generates, builds (MSVC `/W4`) and tests them all on Windows.

## Known untested

- MSVC: the engine, `xgegui` and the tests build clean on MSVC 19.44 with every backend, and all tests pass; checked by hand, not automatically. Older MSVC versions are untested.
- A default build (`cmake -B build`, no options) and a build with only `-DXGE_WITH_OPENGL=ON` are covered by no automatic check ([backends](50-backends.md)).
- Agents build in a Linux sandbox and cannot watch or listen, so anything about how a game feels is unchecked.
