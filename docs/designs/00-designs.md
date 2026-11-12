# Design index

These files record **why**: what we looked for, what we looked at, what we chose, and what is still only an idea. How the engine works **today** is the [reference](../reference/); when the two disagree, the code and the reference win and the design file should be fixed.

Status words: **Built** is in the code. **Decided** is chosen, not built. **Open** is undecided. **Idea** is floated, not adopted. Each file is one topic: scan the table and open only the one you need. The backlog is [open-work](58-open-work.md).

## Language

| File | Topic | Status |
|---|---|---|
| [vision](01-vision.md) | What we want; declarative with triggers; the VGDL landscape | Built |
| [file-format](02-file-format.md) | Why XML; attributes name, content computes; commands in groups | Built |
| [arithmetic](03-arithmetic.md) | Text arithmetic and operation tags | Built |
| [values-and-names](04-values-and-names.md) | `<random>`, values read when used, evaluation order, variables, names | Built; anchors Idea |
| [objects](05-objects.md) | What an object is, storage, layered values, lockstep | Built |
| [groups](06-groups.md) | Groups, columns and rows, groups inside a group, maps, cells for grid puzzles | Built |
| [states-and-conditions](07-states-and-conditions.md) | The stack, `from=`, conditions, keeps, best tables, turns, demos | Built |
| [input](08-input.md) | Actions, refused presses, held keys, key moments, mouse and pads | Built |
| [timers](09-timers.md) | Timers, look timers, time left | Built |
| [pools-and-enemies](10-pools-and-enemies.md) | Spawn and despawn, enemies firing back | Built |
| [vocabulary-map](11-vocabulary-map.md) | How the words fit together, and what kinds are missing | Map |
| [scenes](12-scenes.md) | Scenes inside a state, ways, what conditions count | Built |
| [pieces](13-pieces.md) | Pieces (the top-level `<objects>`), `from=` and the one change rule | Built |
| [verbs](14-verbs.md) | A file per verb, the list, and why it is written out | Built; reading open |

## Behaviour

| File | Topic | Status |
|---|---|---|
| [collisions](20-collisions.md) | Detection, sweeps, pixels, riding and carrying, contacts, knocks, bounce by the normal | Built |
| [collisions-open](21-collisions-open.md) | Weaknesses, response alternatives | Open |
| [motion](22-motion.md) | The verb principle, gravity, thrust, headings, hops, pulls, altitude | Built |
| [platforming](23-platforming.md) | Standing, jumping, ladders, arc and grip | Built; air control Idea |
| [chase-and-paths](24-chase-and-paths.md) | Chasing, aiming, mazes, paths, computer players | Built; curves Idea |
| [tables-and-tracks](25-tables-and-tracks.md) | Mini golf, artillery, pinball, racing | Built |
| [board-games](26-board-games.md) | Moves, the picker, puts, `<think>` | Built; action lookahead Open |
| [cards](27-cards.md) | Decks, piles, hands, patterns, tricks, melds | Built; single cards Open |
| [cell-puzzles](28-cell-puzzles.md) | Pushing, merging, counting around, undo, conditions in actions | Built |

## Pictures and sound

| File | Topic | Status |
|---|---|---|
| [pictures](30-pictures.md) | One bitmap path; lines, polygons, bitmaps, SVG, turning, holes, light | Built |
| [animation-and-looks](31-animation-and-looks.md) | Animation, looks, poses, mirror, blinking | Built |
| [text](32-text.md) | Text, fonts, styles, `<choose>` | Built |
| [sound](33-sound.md) | Notes made into samples, loops, music | Built |

## Generating, engine, tools, games

| File | Topic | Status |
|---|---|---|
| [generator](40-generator.md) | XSLT writes the game as the engine's data | Built; readable output Open |
| [backends](50-backends.md) | Interfaces and factories, scale, the library | Built |
| [build-and-layout](51-build-and-layout.md) | CMake, output, folders, pipeline | Built |
| [cli](52-cli.md), [gui](53-gui.md) | `xgecli`; `xgegui` | Built |
| [games-classic](54-games-classic.md) | What each early game forced | Built |
| [games-written-by-ai](55-games-written-by-ai.md) | Three games from the schema alone | Built |
| [games-atari](56-games-atari.md) | The 2600 list and after | Built |
| [ideas](57-ideas.md) | Scrolling, target profiles, state export, game ideas | Idea |
| [open-work](58-open-work.md) | Backlog and what is untested | Open |

## Keeping these files

- Numbered by section (0x and 1x language, 2x behaviour, 3x pictures and sound, 4x generating, 5x engine, tools and games), with gaps between.
- Add to the closest file before making a new one, and keep this table in step.
- Each file holds what we looked for, what we looked at, what we chose and why, and what is open. Keep it near 6 KB. No progress diaries, dates or test logs.
