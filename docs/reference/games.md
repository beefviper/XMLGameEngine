# The shipped games

55 games in `games/descriptions/`, each one file. Why each was written and what it left out: [games-classic](../designs/54-games-classic.md), [games-written-by-ai](../designs/55-games-written-by-ai.md), [games-atari](../designs/56-games-atari.md).

## The key convention

Every game follows it, and `test_engine_input` checks it:

- **Space** starts, pauses and plays again (never Enter).
- Player one plays on **W A S D**. The arrows are a second way in one-player games, and player two's keys in two-player ones.
- Where Space fires, **P or Escape** pauses.
- Colours are `color.` names only.

The tables list only keys beyond that.

## Paddle and ball

| Game | Keys | Shows |
|---|---|---|
| `pong` | W/S; on the title Space plays the computer (A/D easy or smart), 2 two players on Up/Down | [`<deflect>`](commands.md#touching) off the paddles, a [computer](states.md#computer-players) of two levels |
| `pong_min` | W/S, Up/Down | two paddles and a ball, nothing else; the game [`--generate`](programs.md#generating-a-program) was grown on |
| `breakout` | A/D, the mouse | eight rows of bricks as one group, a half paddle at the top, the mouse by [`<track />`](input.md#pointing-controls) |
| `spacerace` | W/S, Up/Down | two rockets through drifting debris, the most crossings in a minute |

## Shooters

| Game | Keys | Shows |
|---|---|---|
| `spaceinvaders` | A/D, Space fires | a [lockstep](collisions.md#lockstep) block, bombs, a mystery ship, shields worn by [`<hole>`](commands.md#touching) |
| `spaceinvaders2` | as above | the same game drawn from an [SVG](pictures.md#svg-pictures) sprite sheet |
| `astrosmash` | A/D, Space fires | rocks that split, spinners, a spare base each 1,000 |
| `asteroids` | Left/Right turn, Up thrusts, Space fires, Down hyperspace | headings, rocks that break, two saucers, a flame [pose](pictures.md#poses-mirror-grip-and-blinking), the best score kept |
| `demonattack` | A/D, Space fires | three waves of demons that fire back and split |
| `galaxian` | A/D, Space fires; 2 on the title | [paths](timers.md#paths), a swaying fleet, escorts, [turns](states.md#taking-turns), a top five, an [attract mode](states.md#attract-mode) |
| `megamania` | A/D, Space fires | eight wrapping waves; energy as a [timer's time left](timers.md#time-left) |
| `atlantis` | A, W, D fire the three posts | waves whose named timers and conditions [`from=`](states.md#scenes) replaces |
| `missilecommand` | W A S D move the sight, Space launches | `<aim>`, counter-missiles trailing a [`<tether>`](pictures.md#sprites), waves as scenes |
| `depthcharge` | A/D, Space drops | charges off both sides, submarines laying mines, 90 seconds |
| `yarsrevenge` | W A S D fly, Space fires | a missile that [`<seek>`](objects.md#parts-of-an-object)s, the shield a moving map |
| `centipede` | W A S D move the wand, Space fires | a [chain](maps.md#cells) that [splits](maps.md#cells) where shot, an unseen line holding the wand in its zone |

## Crossing, catching, duelling

| Game | Keys | Shows |
|---|---|---|
| `frogger` | W A S D hop | [`<ride />`](commands.md#touching), [`unless=`](collisions.md#rule-filters), diving turtles, five bays |
| `freeway` | W/S, Up/Down | two chickens across ten lanes; a hit knocks one back a lane |
| `kaboom` | A/D, the mouse | eight waves as [scenes](states.md#scenes); the bucket stack as looks |
| `frostbite` | W/S jump rows, A/D walk | [`<jump>`](commands.md#moving), looks that turn the floes, three nights |
| `combat` | W/S drive, A/D turn, F or Left Shift fires; Up/Down, Left/Right, Right Shift or Right Control | two tanks; a hit [knocks](collisions.md#knocks) the tank back |
| `airseabattle` | A S D point, W fires; Left Down Right, Up | two guns, four lanes of planes, 2:16 |
| `fishingderby` | W A S D move the hook | tethers for lines, values [read when used](format.md#values-read-when-used), a computer fisherman |
| `tennis` | W A S D walk | an [altitude](pictures.md#height-and-shadows) and a shadow, a computer at the far end, points said in words |
| `trickshot` | W A S D move the cue tip | pool: balls that [`<knock>`](collisions.md#knocks), slowed by `<drag>` |

## Platforms and rooms

| Game | Keys | Shows |
|---|---|---|
| `donkeykong` | A/D walk, W/S climb, Space jumps | sloping girders, ladders, barrels that sometimes take one, fireballs |
| `mariobros` | A/D walk, W or Space jumps; 2 on the title for Luigi on the arrows | a knock through a floor, [`<poses>`](pictures.md#poses-mirror-grip-and-blinking), a window [`<scale>`](format.md#window), [palette swaps](format.md#palette-swap), six phases |
| `pitfall` | A/D run, Space jumps, W/S climb | screens built from a [kit](objects.md#the-kit-and-from), logs, tar, crocodiles |
| `berserk` | W A S D walk and face, Space fires | rooms as [maps](maps.md), robots that line up and fire, Evil Otto |
| `adventure` | W A S D walk, Space drops | five rooms, [`<carry />`](commands.md#touching), dragons that wake |
| `hauntedhouse` | W A S D walk, Space strikes a match | a house seen only by [light](pictures.md#light-and-dark) |
| `qbert` | W D S A hop up-right, down-right, down-left, up-left | a pyramid map, timed hops with an `<arc>`, Coily hunting by hops |
| `pacman` | W A S D steer | the arcade maze as a [map](maps.md), four ghosts hunting their own ways ([walking a map](maps.md#walking-a-map)) |

## Grid puzzles

| Game | Keys | Shows |
|---|---|---|
| `tetris` | A/D move, W/X turn, E/C turn back, S drops faster, Space drops at once; P pauses | [cells](maps.md#cells), standard turns and kicks, full rows cleared, the next shape by `<send>` |
| `columns` | A/D move, W/X/Space cycle, S drops faster; P pauses | lines of three alike, chains by `<fall />` |
| `capsules` | as Tetris, without the drop | an original game of germs and capsules; halves that stay joined ([`<joins>`](maps.md#cells)) |
| `snake` | W A S D or arrows; Space or P pauses | a [chain](maps.md#cells) for a body, food in a free cell |

## Tables and tracks

All original games. P or Escape pauses.

| Game | Keys | Shows |
|---|---|---|
| `puttpark` | A/D or Left/Right aim, Space held then let go putts | six holes of mini golf; [bounce](commands.md#touching) off sloped walls; a key [held and let go](input.md#a-keys-three-moments) |
| `cannonhill` | W/S or Up/Down aim, Space held then let go fires | an artillery duel for two; [`<polygon>`](pictures.md#polygons) hills blown into craters; wind as a spawn's `<acceleration>` |
| `pinball` | A or Left, D or Right flip; Space held then let go shoots | flippers that [swing about a pivot](pictures.md#turning-about-a-pivot) and lend the ball their speed |
| `speedway` | player one W A S D, player two the arrows | a top-down race; cars with a heading and [`<grip>`](pictures.md#poses-mirror-grip-and-blinking); grass, sand and oil as [`<surface>`](pictures.md#surfaces)s; computer cars on paths |

## Board games

Each plays the computer, or two players with 2 on the title. Cursor on W A S D or the arrows, the mouse points and clicks; P or Escape pauses.

| Game | Keys | Shows |
|---|---|---|
| `checkers` | Space picks up and puts down, X or Backspace puts back; M, V, J options | [`<moves>`](boards.md), forced jumps, kings by `<promote>`, [`<marks>`](boards.md#marks), a computer that [`<think>`](states.md#computer-players)s |
| `reversi` | Space puts a disc; M option | [`<flank>`](boards.md#pieces-put-not-moved), `<put>`, the score by `<count>` |
| `fourinarow` | A/D move, Space, S or Down drops | [`<fall />`](boards.md#pieces-put-not-moved) in the moves; the win by a `<line>` check |
| `tictactoe` | Space marks; A/D on the title for easy or smart | an empty `<moves />`: any free square |
| `dotsandboxes` | Space draws | lines put [`<on>`](boards.md#pieces-put-not-moved) their places, a box claimed by `<close>`, a cursor only on free places (`<onto>`) |

## Card games

Space or a click plays; P or Escape pauses (N there deals again where offered).

| Game | Keys | Shows |
|---|---|---|
| `klondike` | move from pile to pile, Space takes and lays, X puts back, F sends home; the mouse drags | the pack a [group](cards.md#holders) of rows and columns, places that `<holds>` with rules of what they accept, the deal in `<starts>` |
| `crazyeights` | A/D along your cards, W/S to the stock, Space plays or draws, names the suit after an eight | [players as a group](states.md#players-as-a-group) taking turns, a computer member, `<wild>` and a call |
| `blackjack` | A/D bet, Space deals, W hits, S stands | [`<worth>`](cards.md#points) and live `points`, a dealer's hole card |
| `holdem` | W bets or raises, Space checks or calls, X folds | poker hands as a game's [`<patterns>`](cards.md#patterns), live `pattern`, `strength`, `odds` |
| `hearts` | A/D along your cards, Space plays | a [`<trick>`](cards.md#tricks) broken by hearts, points by card; the players a [group in turn](states.md#players-as-a-group), the trick's winner leading |
| `spades` | W/S bid, A/D along your cards, Space bids or plays | [trumps and partners](cards.md#tricks), bids from live `tricks`; the players a [group in turn](states.md#players-as-a-group), partners by `<team>` |
| `gin` | A/D along cards, W/S to the piles, Space draws or discards, K knocks | [melds](cards.md#melds) of sets and runs, live `deadwood` |
| `pyramid` | Space turns the stock, picks and pairs, X puts back | [covering](cards.md#covering-and-pairs), a hand that holds two, a pile that takes a sum of 13 |
