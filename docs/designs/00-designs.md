# XMLGameEngine design index

These files record **why**: what we looked for, what we looked at, what we chose, and what is still only an idea. How the engine works **today** is [../readme.md](../readme.md); when the two disagree, the code and the readme win and the design file should be fixed. Verbs are written `bounce()` for short; in a game file they are always tags.

Status: **Built** = in the code. **Decided** = chosen, not built. **Open** = undecided. **Idea** = floated, not adopted. **Reference** = a map, nothing to decide.

Each file is one topic, so scan this table and open only the one you need. The backlog is [open-work](58-open-work.md).

## Language

| File | Topic | Status |
|---|---|---|
| [vision](01-vision.md) | What we want, declarative with triggers, the VGDL landscape | Built |
| [file-format](02-file-format.md) | Why XML + XSD, one file; attributes name, content computes; lists packed in strings | Built; lists in strings Open |
| [arithmetic](03-arithmetic.md) | Math as text, `<formula>` | Built |
| [values-and-names](04-values-and-names.md) | `<random>`, values read when used (`<set>`, `<spawn>`, live numbers), evaluation order, variables, an object's size, names and classes | Built; `Value` variant Decided; reference points and axes Idea |
| [objects](05-objects.md) | What an object is, storage, handles, lockstep | Built; handles Decided |
| [groups](06-groups.md) | `<group>`: listed members, cells in columns and rows, `<map>`s drawn as text, maps of cells (Tetris), groups inside a group | Built; Columns, Snake, Centipede Decided |
| [states-and-conditions](07-states-and-conditions.md) | State stack, `<pop state>`, `<shows>` and `<conditions>` sets, live tests, `<all>`/`<any>`, `total=`, `distance=`, states `from=`, keeps, a table of the best, turns, attract mode | Built; object-level conditions Open |
| [input](08-input.md) | Actions, a refused press, key sets, held keys across states, mouse and pads | Built |
| [timers](09-timers.md) | Timers, look timers, time left, commands in every context | Built; event queue Open |
| [pools-and-enemies](10-pools-and-enemies.md) | Spawn and despawn (`<most>`), what replaced pools, enemies firing back | Built |
| [vocabulary-map](11-vocabulary-map.md) | Every word by grammar and by layer; missing categories (checked 2026-10-28) | Reference |
| [scenes](12-scenes.md) | Scenes inside a state; states and scenes own objects, `<enter>`, dotted paths, `<drawn>`, `<show state>` | Built |
| [kit](13-kit.md) | The game's `<kit>` of pieces, `from=` and the one change rule, lifetime by placement, spawned pieces beside their spawner | Built |

## Behavior

| File | Topic | Status |
|---|---|---|
| [collisions](20-collisions.md) | Detection, dispatch, swept and pixel collision, fewer checks, speed filters, `<deflect>`, `side=`, conditions and `<trigger>` in rules, contacts, knocks and `through=`, hitboxes | Built |
| [collisions-open](21-collisions-open.md) | Weaknesses, response alternatives, escalation, reference Pong | Open / Idea |
| [motion](22-motion.md) | Verb principle, gravity, thrust, headings, hop (diagonals too), jump, facing, pull, seek, altitude and shadows | Built |
| [platforming](23-platforming.md) | Standing, leaping, ladders (Donkey Kong); arcing jump; `arc` and `grip` | Built; air control Idea |
| [chase-and-paths](24-chase-and-paths.md) | `<chase>`, `<aim>`, paths and formations (Galaxian), maze walking (`<go>`, `<hunt>`, `<wander>`), AI targeting, computer players | Built; curved paths Idea |

## Pictures and sound

| File | Topic | Status |
|---|---|---|
| [pictures](30-pictures.md) | One `Bitmap` path for lines, bitmaps, SVG parts, flip, turning; inks, bitmap styles and palette swap; tethers, holes, light and dark | Built; decals Idea |
| [animation-and-looks](31-animation-and-looks.md) | Animation, named looks, looks that move, `poses`, `mirror`, blinking | Built |
| [text](32-text.md) | Text, font files and the built-in font, `<styles>`, `<centered>`, words (`<choose>`) | Built |
| [sound](33-sound.md) | Notes made into samples; `<sounds>`, `<play>`, loops, music in `<part>`s | Built |

## Code generation

| File | Topic | Status |
|---|---|---|
| [generator](40-generator.md) | `windows-cpp`: XSLT writes the game as the engine's data, played by the engine's own code; a readable game on the shared frame (prototype, set aside) | Built; readable output Open |

## Engine, tools and games

| File | Topic | Status |
|---|---|---|
| [backends](50-backends.md) | Backend interfaces and factories, window `scale`, library and programs | Built |
| [build-and-layout](51-build-and-layout.md) | CMake modules, `output/`, source layout and `xge/lib/` folders, pipeline | Built |
| [cli](52-cli.md), [gui](53-gui.md) | `xgecli` options; `xgegui` (Qt renderer, tree, Options, windows) | Built |
| [games-classic](54-games-classic.md) | What each early game tested and forced | Built |
| [games-written-by-ai](55-games-written-by-ai.md) | Berserk, Demon Attack, Frostbite from the schema alone, then rewritten | Built |
| [games-atari](56-games-atari.md) | The 2600 list and after: a section per game, from Pitfall! to Mario Bros. and Q*bert | Built |
| [ideas](57-ideas.md) | Scrolling, capability profiles, state export, game ideas | Idea |
| [open-work](58-open-work.md) | Backlog and known untested areas | Open |

Files are numbered by section (01-12 language, 2x behavior, 3x pictures and sound, 4x code generation, 5x engine, tools and games) with gaps left between sections. Add a topic to the closest existing file before creating a new one; keep this table in step. Keep files to: what we looked for, what we looked at, what we chose and why, what is open. No progress diaries or test-run logs.
