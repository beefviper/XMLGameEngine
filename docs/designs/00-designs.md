# XMLGameEngine design index

These files record **why**: what we were looking for, what we looked at, what we chose, and what is still only an idea. How the engine works **today** is [../readme.md](../readme.md); when the two disagree, the code and the readme win and the design file should be fixed.

Verbs are sometimes written `bounce()` for short; in a game file they are always tags.

Status key: **Reference** = a map or summary, nothing to decide. **Built** = in the code. **Decided** = chosen, not built. **Leaning** = a direction, not final. **Open** = undecided. **Idea** = floated, not adopted.

| # | Topic | Chosen / status |
|---|---|---|
| [01](01-vision-and-format.md) | Vision, file format, the VGDL landscape, attributes vs content, arithmetic as text or tags | Declarative + `condition` triggers; XML + XSD, one file; function syntax removed; arithmetic written as text, or as `<equation>` / `<formula>` tags beside it. **Built.** XSLT code generation: **built**, plain for pong_min (`--generate windows-cpp`) and engine-faithful for Pong (`windows-cpp-full`); the rest **open** |
| [02](02-values-variables-and-names.md) | Values (`<random>`), evaluation order, variables, an object's size in expressions, names and classes | Two-pass registration; polling floats by `owner.variable`; `name.width`; `name` + `class`. **Built.** `Value` variant **Decided**; selectors, inheritance **Idea** |
| [03](03-objects-groups-and-storage.md) | What an object is, storage and handles, grids, lockstep, groups | `vector<Object>` and name search; `<group>` with members that take what they leave out. **Built.** Handles **Decided**; opt-in parts, shapes **Idea** |
| [04](04-states-conditions-and-input.md) | States, conditions, input and held keys | State stack; `atleast`/`atmost`/`remaining`; objects name actions, states bind keys; live held keys. **Built.** Per-state behavior, `goto`, data between states **Open** |
| [05](05-collisions.md) | Detection/dispatch/execution, swept collision, pixel tests, riding, `unless`, speed filters, `<deflect>`, escalation, reference Pong | **Built.** Escalation (verb protocol) **Idea** |
| [06](06-motion-and-verbs.md) | The verb principle, what each game forced, gravity and thrust, headings, hop/jump/facing, motion models, arcing jump, AI, paths | **Built** (constant velocity, acceleration, thrust, headings, straight jump). Paths of straight steps **built** (Galaxian). Arcing jump, AI targeting, curved paths **Idea** |
| [07](07-pictures-and-text.md) | Engine-drawn pictures: lines, bitmaps, SVG parts, flip, turning, animation, looks, built-in font | One `Bitmap` path for every picture the engine draws. **Built** |
| [08](08-timers-and-enemy-behavior.md) | Timers, commands in every context, pools instead of spawning, enemies firing back, key sets | Timers on objects and states, pools, key sets. **Built.** Aiming, event queue **Open** |
| [09](09-sound.md) | Sound as notes made into samples by the engine | `<sounds>`, `<play>`, an `Audio` backend. **Built** |
| [10](10-games-as-tests.md) | What each shipped game tested and forced; the games another AI wrote; Berserk | **Built** |
| [11](11-backends-build-and-layout.md) | Backend interfaces and factories, library + programs, CMake modules, `output/`, code layout | **Built.** Folder layout of `lib/` **Open** |
| [12](12-front-ends.md) | `xgecli` options, `xgegui` (Qt renderer, tree, Options, one or two windows, OpenGL backend) | **Built** |
| [13](13-ideas.md) | Scrolling, targets and capability profiles, state export, game ideas, small ideas | **Idea** |
| [14](14-vocabulary-map.md) | Every word the language knows by grammar (noun, verb, adjective, adverb, preposition) and by engine layer; the categories still missing | A reference, not a decision: collision is the richest layer, values the thinnest; frames the math words of the open arithmetic question in 01. **Reference** |

## Reading order

New to the project: [readme](../readme.md), then 01, 04, 05. Working on the language: 14, 01, 02, 06, 08. Working on pictures or sound: 07, 09. Working on the C++: 03, 11, 12.

Add a new topic to the closest existing file before creating a new one; keep this table and the files in step.
