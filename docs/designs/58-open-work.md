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

## Waiting for a look on Windows

What only eyes, ears or MSVC can check. Whoever lands a change that needs one adds its line; the author deletes it once seen.

1. The build at MSVC `/W4` with no warnings, and `games-run-all.bat`.
2. Pace: every game at the same speed under `-w sfml3`, `sdl2`, `raylib` and `opengl`, also on a screen faster than 60 Hz; no tearing.
3. xgegui: Press Start 2P games show their font; the size bar and -/100%/+ keep the window's size.
4. Frostbite: hopping between floes; Donkey Kong, Mario Bros. and Pitfall jump as before.
5. Mario Bros.: a flipped enemy keeps its sideways way; POW spares those in the air; creepers turn when they meet; a shellcreeper that gets up by itself turns red.
6. Q*bert: the disc sets Q*bert down and the top cube lights; Donkey Kong's scale and arcs.
7. Tetris: E and C turn, lines clear.
8. Columns: jewels cycle and lines of three clear, chains fall.
9. Snake: grows on eating, dies on itself.
10. Centipede: heads never stick; a shot centipede splits; the player stays in the bottom zone.
11. Capsules: only pills fall, germs hold things up; the first match does not clear the screen.
12. Putt Park: the ball bounces off angled walls and drops in the cup.
13. Pinball: flippers swing and back; the ball rests on a raised flipper without hopping; the plunger launches on release.
14. Cannon Hill: aim, charge and fire; wind bends the shot; holes in the ground.
15. Speedway: the car grips on road, slides on grass; laps count.
16. Pong: Space against the computer (A and D for easy and smart), 2 for two players.
17. Checkers: the computer answers on Space; X or Backspace or right click puts back; the mouse picks and places; marks show the held piece and its moves.
18. Reversi, Four in a Row, Tic-Tac-Toe, Dots and Boxes: a game against the computer to the end, the window answering while it thinks.
19. Klondike and Pyramid: deal, move, win.
20. Crazy Eights: an eight calls a suit; the computer players finish a game.
21. Blackjack, Texas Hold'em, Hearts, Spades, Gin Rummy: one hand each to the end.
22. Crates, Doubles, Minefield: a level each, with undo in Crates.

## Known untested

- Sanitizers: the whole of `xgetest` is clean under `-DXGE_SANITIZE=address` (address and undefined behaviour), as are `xgetest-long`'s random keys and damaged files; the board-game search on its own thread is clean under `-DXGE_SANITIZE=thread`. The damaged files found one fault: a line out at 1e9 or a text of a huge size asked for hundreds of gigabytes. Pictures and texts are now at most 4096 pixels a side (`refusePictureSize`). Generated programs and `xgegui` have not been run under one.
- MSVC is checked by hand, not automatically; older versions are untested.
- A default build (no options) and one with only `-DXGE_WITH_OPENGL=ON` have no automatic check ([build-and-layout](51-build-and-layout.md)).
- Agents build in a Linux sandbox and cannot watch or listen, so how a game feels and sounds is unchecked.
