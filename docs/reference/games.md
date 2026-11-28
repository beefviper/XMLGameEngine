# The shipped games

59 games in `games/descriptions/`, each one file. Why each was written and what it left out: [games as tests](../designs/54-games.md).

## The key convention

Every game follows it, and `test_engine_input` checks it:

- **1** starts every game from its title, as the one player button of an arcade; **2** starts the games two people can play, saying how many [people](input.md#seats) play. **Space** plays again (never Enter); in play it jumps or fires.
- The **Pause** key pauses and plays on, in every game (and a pad's Start). Nothing else pauses.
- **Pads** follow the seats: a game for two gives each seat its own pad (`pad.up` in a seat), so player 2 plays on pad 2.
- Player one plays on **W A S D**. The arrows are a second way in one-player games, and player two's keys in two-player ones.
- Colours are the engine's colour names only ([colours](format.md#colours)).

The tables list only keys beyond that.

## Paddle and ball

| Game | Keys | Shows |
|---|---|---|
| `pong` | W/S; on the title 1 plays the computer (A/D easy or smart), 2 two players on Up/Down | [`<deflect>`](commands.md#touching) off the paddles, a [computer](states.md#computer-players) of two levels in [seat](input.md#seats) 2, so Up and Down do nothing against it; `windows-cpp` and `web-js` write it out (`ctest --test-dir build/generated` puts it in [`output/<config>/generated/`](engine.md#building)) |
| `pong_min` | W/S, Up/Down | two paddles and a ball, nothing else; the game [`--generate`](programs.md#generating-a-program) was grown on; `windows-cpp` and `web-js` write it out (`ctest --test-dir build/generated` puts it in [`output/<config>/generated/`](engine.md#building)) |
| `breakout` | A/D, the mouse | eight rows of bricks as one group, a half paddle at the top, the mouse by [`<track />`](input.md#pointing-controls); `windows-cpp` and `web-js` write it out (`ctest --test-dir build/generated` puts it in [`output/<config>/generated/`](engine.md#building)) |
| `spacerace` | W/S, Up/Down | two rockets through drifting debris, the most crossings in a minute; `windows-cpp` and `web-js` write it out |

## Shooters

| Game | Keys | Shows |
|---|---|---|
| `prismfall` | A/D, Space fires | an original marching-fleet game: a [lockstep](collisions.md#lockstep) lattice of crystal shards, sparks on timers, a comet, pylons chipped by [`<hole>`](commands.md#touching); `windows-cpp` and `web-js` write it out |
| `prismfall2` | as above | the same game drawn from its own [SVG](pictures.md#svg-pictures) sprite sheet, the shards turning on three frames |
| `astrosmash` | A/D, Space fires | rocks that split, spinners, a spare base each 1,000 |
| `asteroids` | Left/Right turn, Up thrusts, Space fires, Down hyperspace | headings, rocks that break, two saucers, a flame [pose](pictures.md#poses-mirror-grip-and-blinking), the best score kept |
| `demonattack` | A/D, Space fires | three waves of demons that fire back and split |
| `galaxian` | A/D, Space fires; title 1 or 2 players, taking turns | [paths](timers.md#paths), a swaying fleet, escorts, [alternating games](states.md#alternating-games), a top five |
| `megamania` | A/D, Space fires | eight wrapping waves; energy as a [timer's time left](timers.md#time-left) |
| `atlantis` | A, W, D fire the three posts | waves whose named timers and conditions [`from=`](states.md#scenes) replaces |
| `missilecommand` | W A S D move the sight, Space launches | `<aim>`, counter-missiles trailing a [`<tether>`](pictures.md#sprites), waves as scenes |
| `depthcharge` | A/D, Space drops | charges off both sides, submarines laying mines, 90 seconds |
| `yarsrevenge` | W A S D fly, Space fires | a missile that [`<seek>`](objects.md#parts-of-an-object)s, the shield a moving map |
| `centipede` | W A S D move the wand, Space fires | a [chain](maps.md#cells) that [splits](maps.md#cells) where shot, an unseen line holding the wand in its zone |

## Crossing, catching, duelling

| Game | Keys | Shows |
|---|---|---|
| `frogger` | W A S D hop | [`<ride />`](commands.md#touching), [`unless=`](collisions.md#rule-filters), diving turtles, five bays; `windows-cpp` and `web-js` write it out |
| `freeway` | W/S, Up/Down | two chickens across ten lanes; a hit knocks one back a lane; `windows-cpp` and `web-js` write it out |
| `kaboom` | A/D, the mouse | eight waves as [scenes](states.md#scenes); the bucket stack as looks; `windows-cpp` and `web-js` write it out |
| `frostbite` | W/S hop rows, A/D walk | timed [`<hop>`](commands.md#moving), looks that turn the floes, three nights |
| `combat` | W/S drive, A/D turn, F or Left Shift fires; Up/Down, Left/Right, Right Shift or Right Control | two tanks; a hit [knocks](collisions.md#knocks) the tank back |
| `airseabattle` | A S D point, W fires; Left Down Right, Up | two guns, four lanes of planes, 2:16 |
| `fishingderby` | W A S D move the hook; title 1 plays the computer, 2 two players on the arrows | tethers for lines, values [read when used](format.md#values-read-when-used), a computer fisherman in seat 2 |
| `tennis` | W A S D walk; title 1 plays the computer, 2 two players on the arrows | an [altitude](pictures.md#height-and-shadows) and a shadow, a computer in seat 2 at the far end, points said in words |
| `sparring` | A/D step, W jabs, S guards; Left/Right, Up, Down | two boxers; a jab's middle [frame holds the fist](collisions.md#how-a-touch-is-found), scored by the one hit's `by="fist"` rules, a guard by his look |
| `trickshot` | W A S D move the cue tip | pool: balls that [`<knock>`](collisions.md#knocks), slowed by `<drag>` |

## Platforms and rooms

| Game | Keys | Shows |
|---|---|---|
| `donkeykong` | A/D walk, W/S climb, Space jumps | sloping girders, ladders, barrels that sometimes take one, fireballs |
| `mariobros` | A/D walk, W or Space jumps; title 1 one player, 2 for Luigi on the arrows | a knock through a floor, [`<poses>`](pictures.md#poses-mirror-grip-and-blinking), a window [`<scale>`](format.md#window), [palette swaps](format.md#palette-swap), six phases |
| `pitfall` | A/D run, Space jumps, W/S climb | screens built from [pieces](objects.md#pieces-and-from), logs, tar, crocodiles |
| `berserk` | W A S D walk and face, Space fires | rooms as [maps](maps.md), robots that line up and fire, Evil Otto |
| `adventure` | W A S D walk, Space drops | five rooms, [`<carry />`](commands.md#touching), dragons that wake |
| `hauntedhouse` | W A S D walk, Space strikes a match | a house seen only by [light](pictures.md#light-and-dark) |
| `qbert` | W D S A hop up-right, down-right, down-left, up-left | a pyramid map, timed hops with an `<arc>`, Coily hunting by hops |
| `pacman` | W A S D steer | the arcade maze as a [map](maps.md), four ghosts hunting their own ways ([walking a map](maps.md#walking-a-map)) |

## Grid puzzles

| Game | Keys | Shows |
|---|---|---|
| `tetris` | A/D move, W/E/X turn clockwise, Q counterclockwise, S drops faster, Space drops at once | [cells](maps.md#cells), standard turns and kicks, full rows cleared, the next shape by `<send>` |
| `columns` | A/D move, W/E/X/Space cycle, Q cycles back, S drops faster | lines of three alike, chains by `<fall />` |
| `capsules` | as Tetris, without the drop | an original game of germs and capsules; halves that stay joined ([`<joins>`](maps.md#cells)) |
| `snake` | W A S D or arrows | a [chain](maps.md#cells) for a body, food in a free cell; `windows-cpp` and `web-js` write it out |
| `crates` | W A S D or arrows walk, U/Backspace undo, R restarts the room | four rooms of crates to push onto spots: [`<shove />`](maps.md#pushing-merging-and-counting), a floor layer, `<count><here /></count>`, `<undo />` |
| `doubles` | W A S D or arrows slide | a four by four board of numbers that add up when equal: `<slide class>`, `<merge>`, pairs counted `<around>` |
| `minefield` | W A S D or arrows or the mouse point, Space/left click opens, F/right click flags | nine by nine, ten mines: counts `<around>`, a flood by `<trigger><around /></trigger>` |

## Tables and tracks

All original games.

| Game | Keys | Shows |
|---|---|---|
| `puttpark` | A/D or Left/Right aim, Space held then let go putts | six holes of mini golf; [bounce](commands.md#touching) off sloped walls; a key [held and let go](input.md#a-keys-three-moments) |
| `cannonhill` | W/S or Up/Down aim, Space held then let go fires | an artillery duel for two, the barrels a [players group](states.md#players-as-a-group); [`<polygon>`](pictures.md#polygons) hills blown into craters; wind as a spawn's `<acceleration>` |
| `pinball` | A or Left, D or Right flip; Space held then let go shoots | flippers that [swing about a pivot](pictures.md#turning-about-a-pivot) and lend the ball their speed |
| `speedway` | player one W A S D, player two the arrows | a top-down race; cars with a heading and [`<grip>`](pictures.md#poses-mirror-grip-and-blinking); grass, sand and oil as [`<surface>`](pictures.md#surfaces)s; computer cars on paths |

## Board games

Each plays the computer with 1 on the title, or two players with 2, the computer in [seat](input.md#seats) 2 and the keys shared by both seats. Cursor on W A S D or the arrows, the mouse points and clicks.

| Game | Keys | Shows |
|---|---|---|
| `checkers` | Space picks up and puts down, X or Backspace puts back; M, V, J options | [`<moves>`](boards.md), forced jumps, kings by `<promote>`, [`<marks>`](boards.md#marks), a computer that [`<think>`](states.md#computer-players)s; red and white a [players group](states.md#players-as-a-group) that each `<owns class>` its pieces |
| `reversi` | Space puts a disc; M option | [`<flank>`](boards.md#pieces-put-not-moved), `<put>`, the score by `<count>` |
| `backgammon` | title: 1 plays the computer (white), 2 two players; Space rolls, picks up and puts down; X puts back; C doubles, Y takes, N drops | points as [tall cells](maps.md#tall-cells), each side's [`<route>`](boards.md#along-a-route) with `<steps>` by the dice and `<hit>`, live `along` for the win and gammons; the cube as variables |
| `fivedice` | title: 1 plays the computer, 2 two players; Space rolls (three a turn), 1 to 5 keep a die, W/S along the card, E writes | dice as a held group whose six looks are faces and whose `worth` a `<set>` rolls; the cup [`<counts>`](cards.md#holders) them, and a holder per box [scores `<with>`](cards.md#patterns) the cup by one pattern (three or four alike, full house, runs, five alike); each player's column of boxes a piece taken twice; the computer as [`<computer>` conditions](states.md#computer-players) on live numbers (the dice it wants, each box's pick) holding seat 2's keys |
| `fourinarow` | A/D move, Space, S or Down drops | [`<fall />`](boards.md#pieces-put-not-moved) in the moves; the win by a `<line>` check |
| `tictactoe` | Space marks; A/D on the title for easy or smart | an empty `<moves />`: any free square |
| `dotsandboxes` | Space draws | lines put [`<on>`](boards.md#pieces-put-not-moved) their places, a box claimed by `<close>`, a cursor only on free places (`<onto>`) |

## Card games

Space or a click plays; N in the pause deals again where offered.

| Game | Keys | Shows |
|---|---|---|
| `klondike` | move from pile to pile, Space takes and lays, X puts back, F sends home; the mouse drags | the pack a [group](cards.md#holders) of rows and columns, places that `<holds>` with rules of what they accept, the deal in `<starts>` |
| `crazyeights` | A/D along your cards, W/S to the stock, Space plays or draws, names the suit after an eight | [players as a group](states.md#players-as-a-group) taking turns, a computer member, `<wild>` and a call |
| `blackjack` | A/D bet, Space deals, W hits, S stands | a pack whose columns give `worth` (an ace's `high` 11) and holders that [`<counts>`](cards.md#holders) within 21; `<send sprite="back">` for the dealer's hole card |
| `holdem` | W bets or raises, Space checks or calls, X folds | poker hands as a game's [`<patterns>`](cards.md#patterns) naming their variables, live `pattern`, `strength`, `odds`; a pack group, the ace's `high` 14; you and the computer a players group, the computer played by `<computer>` conditions |
| `hearts` | A/D along your cards, Space plays | a [`<trick>`](cards.md#tricks) broken by hearts, points as card variables a holder `<counts>`, the deal by `<distribute>`; the players a [group in turn](states.md#players-as-a-group), the trick's winner leading |
| `spades` | W/S bid, A/D along your cards, Space bids or plays | [trumps and partners](cards.md#tricks) on a holder's `<trick>`, bids from live `tricks`; the players a [group in turn](states.md#players-as-a-group), partners by `<team>` |
| `gin` | A/D along cards, W/S to the piles, Space draws or discards, K knocks | [melds](cards.md#melds) of sets and runs, live `deadwood`; the players a [group in turn](states.md#players-as-a-group), a turn a draw then a discard (holding eleven turns it to discard) |
| `pyramid` | Space turns the stock, picks and pairs, X puts back | [covering](cards.md#covering-and-pairs), a picker that holds two (`<pick><most>2`), a holder that `<accepts>` a `total="sum"` of 13 |
