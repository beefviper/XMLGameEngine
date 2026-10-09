# XMLGameEngine design index

These files record **why**: what we looked for, what we looked at, what we chose, and what is still only an idea. How the engine works **today** is [../readme.md](../readme.md); when the two disagree, the code and the readme win and the design file should be fixed. Verbs are written `bounce()` for short; in a game file they are always tags.

Status: **Built** = in the code. **Decided** = chosen, not built. **Open** = undecided. **Idea** = floated, not adopted. **Reference** = a map, nothing to decide.

Each file is one topic, so scan this table and open only the one you need. The backlog is [open-work](58-open-work.md).

## Language

| File | Topic | Status |
|---|---|---|
| [vision](01-vision.md) | What we want, declarative with triggers, the VGDL landscape | Built |
| [file-format](02-file-format.md) | Why XML + XSD, one file; attributes name, content computes | Built |
| [arithmetic](03-arithmetic.md) | Math as text, `<equation>`, `<formula>` | Built |
| [values-and-names](04-values-and-names.md) | `<random>`, evaluation order, variables, an object's size, names and classes | Built; `Value` variant Decided |
| [objects](05-objects.md) | What an object is, storage, handles, lockstep | Built; handles Decided |
| [groups](06-groups.md) | `<group>`: listed members, cells in columns and rows | Built |
| [states-and-conditions](07-states-and-conditions.md) | State stack, `atleast`/`atmost`/`remaining` | Built; per-state behavior, `goto` Open |
| [input](08-input.md) | Actions, held keys, key sets across states | Built |
| [timers](09-timers.md) | Timers, commands in every context | Built; event queue Open |
| [pools-and-enemies](10-pools-and-enemies.md) | Pools instead of spawning, enemies firing back, key sets | Built |
| [vocabulary-map](11-vocabulary-map.md) | Every word by grammar and by layer; missing categories | Reference |

## Behavior

| File | Topic | Status |
|---|---|---|
| [collisions](20-collisions.md) | Detection, dispatch, swept and pixel collision, speed filters, `<deflect>` | Built |
| [collisions-open](21-collisions-open.md) | Weaknesses, response alternatives, escalation, reference Pong | Open / Idea |
| [motion](22-motion.md) | Verb principle, gravity, thrust, headings, hop, jump, facing | Built |
| [platforming](23-platforming.md) | Standing, leaping, ladders (Donkey Kong); arcing jump | Built; air control Idea |
| [chase-and-paths](24-chase-and-paths.md) | `<chase>`, `<aim>`, paths and formations (Galaxian), AI targeting | Built; curved paths Idea |

## Pictures and sound

| File | Topic | Status |
|---|---|---|
| [pictures](30-pictures.md) | One `Bitmap` path for lines, bitmaps, SVG parts, flip, turning | Built |
| [animation-and-looks](31-animation-and-looks.md) | Animation, named looks | Built |
| [text](32-text.md) | Text and the built-in font | Built |
| [sound](33-sound.md) | Notes made into samples; `<sounds>`, `<play>` | Built |

## Code generation

| File | Topic | Status |
|---|---|---|
| [generator](40-generator.md) | XSLT approach, options, the plain `windows-cpp` target, open questions | Built |
| [generator-structure](41-generator-structure.md) | Program layout: objects, screens, groups, pictures, names, modules | Built |
| [generator-rules](42-generator-rules.md) | Edges, touches, timers, jumps, aim, chase, paths | Built |

## Engine, tools and games

| File | Topic | Status |
|---|---|---|
| [backends](50-backends.md) | Backend interfaces and factories, library and programs | Built |
| [build-and-layout](51-build-and-layout.md) | CMake modules, `output/`, source layout, pipeline | Built; `lib/` folders Open |
| [cli](52-cli.md), [gui](53-gui.md) | `xgecli` options; `xgegui` (Qt renderer, tree, Options, windows) | Built |
| [games-classic](54-games-classic.md) | What each early game tested and forced | Built |
| [games-written-by-ai](55-games-written-by-ai.md) | Berserk, Demon Attack, Frostbite from the schema alone | Built |
| [games-atari](56-games-atari.md) | Pitfall!, Missile Command, Combat, Air-Sea Battle, Megamania | Built |
| [ideas](57-ideas.md) | Scrolling, capability profiles, state export, game ideas | Idea |
| [open-work](58-open-work.md) | Backlog and known untested areas | Open |

Files are numbered by section (01-11 language, 2x behavior, 3x pictures and sound, 4x code generation, 5x engine, tools and games) with gaps left between sections. Add a topic to the closest existing file before creating a new one; keep this table in step. Keep files to: what we looked for, what we looked at, what we chose and why, what is open. No progress diaries or test-run logs.
