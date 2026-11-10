# Open work

**Status:** A backlog; nothing here is decided. Each item points at the file that holds the reasoning.

The [plan](../plan.md) reviews this backlog, argues with parts of it, and orders the work as tasks.

## Next classes of game

- **Dice** (backgammon) was agreed as the next board or card step ([cards](27-cards.md)).
- **Moves as timelines** (fighting games: one-shot animations, a hitbox per frame, key sequences) is the next class after tables and tracks ([tables-and-tracks](25-tables-and-tracks.md)).
- **Lookahead for action games' computers** ([board-games](26-board-games.md#open)).
- **A deck of single cards**, each its own picture and numbers ([cards](27-cards.md)).
- Games the vocabulary cannot say yet: [games-atari](56-games-atari.md#near-misses).

## Language and engine

- **Steering** ([chase-and-paths](24-chase-and-paths.md)): leading a target, chasing round walls, a real maze search as another verb, curved path legs.
- **Platforms** ([platforming](23-platforming.md)): Donkey Kong's barrels choosing ladders by where Jumpman is, the other three screens, air control.
- **Cells** ([groups](06-groups.md#cells)): gliding between cells; Tetris hold, lock delay, ghost piece, a bag of seven, row scoring; Columns chain scoring; Centipede's flea, spider and scorpion.
- **Cell puzzles** ([cell-puzzles](28-cell-puzzles.md#open)): undo of despawned copies, a weighted spawn, a safe first square, conditions in a condition in an action.
- **Pictures** ([pictures](30-pictures.md), [animation-and-looks](31-animation-and-looks.md)): decals, a hole the shape of a bitmap, light blocked by walls, an interval per frame, an animation that runs once.
- **Sound** ([sound](33-sound.md)): an envelope; notes that follow a variable (Space Invaders' march keeping time).
- **Values** ([arithmetic](03-arithmetic.md)): tangent, square root, power, `pick`, when a game needs one.
- **Conditions** ([states-and-conditions](07-states-and-conditions.md)): object-level conditions, a total inside a value, conditions on edge rules.
- **A word for the object touched** (`other`), so Mario Bros.' rules about either brother are written once ([games-atari](56-games-atari.md#mario-bros)).
- **Scenes** ([scenes](12-scenes.md)): Berserk's larger grid of rooms, which needs maps made while playing.
- **Reserved names:** an object, group, member, map or state may not be called `self`, `others`, `touch`, `window` or `color`, nor anything a number, because dotted names mix the game's own names with the engine's words (`self.x`, `window.width`, `color.red`). Variables and sounds are free (only ever after a dot, or named by `sound=`), and `top` is free (`top.rank` is read before any name). Look for a way to say those things that needs no reserved words, so any name is free (an object may already be called `object`).

## The games

- Galaxian: escorts' points. Asteroids: the saucers' shots breaking rocks. Lunar Lander: hard landings.
- Pac-Man: the later levels' schedules, the other fruit. Adventure: the bat, Rhindle, the rest of the kingdom.
- Putt Park: a cup that takes only a slow ball wholly over it. Cannon Hill: cannons falling into craters.
- Speeds in Galaxian, Berserk, Tennis, Haunted House and Pac-Man were chosen without watching play: tune them first.

## Code

- `data.xsl` reads the file a second time; a new field in `load/xml.cpp` needs the same there ([generator](40-generator.md#open)).
- `Engine::handleKeyPressed`/`handleKeyReleased` are public only because the tests and `played.h` press keys through them; a test seam would let them go private.
- Objects not shown (`isShown`) are left out of the swept collision pass in `play/frame.cpp`, so nothing hidden can touch or be touched; a game with invisible things that collide would need them in it.
- Building generated programs is by hand: `scripts/shell/windows/games-run-all.bat` generates, builds at MSVC `/W4` and tests them all.

## Known untested

- MSVC is checked by hand, not automatically; older versions are untested.
- A default build (no options) and one with only `-DXGE_WITH_OPENGL=ON` have no automatic check ([build-and-layout](51-build-and-layout.md)).
- Agents build in a Linux sandbox and cannot watch or listen, so how a game feels and sounds is unchecked.
