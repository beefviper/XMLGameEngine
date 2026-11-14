# XMLGameEngine documentation

A game is one XML file of tags, checked against `games/xgedef.xsd`. The engine reads it and plays it, with no code in the file. This page is the way in: what each part of the docs is for, a short tour, and an index of every word.

## Where to look

| Folder | For | Use it when |
|---|---|---|
| [reference/](reference/) | how the engine works **today**: every tag, what it does, the programs, the build | writing a game, or checking what a tag does. The code is the truth; these pages follow it |
| [designs/](designs/00-designs.md) | one small file per topic: what was looked at, what was chosen and why, what is open | changing the language or the engine |
| [agents/](agents/readme.md) | tips that are hard to work out from the code, for anyone (or any AI) changing it | before a change |
| [plan.md](plan.md) | what is next: ordered tasks, each written to be picked up cold, then the backlog | choosing what to do next |

New to it: read the tour below, then [the game file](reference/format.md), [objects](reference/objects.md), [collisions](reference/collisions.md), [input](reference/input.md) and [states](reference/states.md), with a game from [the shipped games](reference/games.md) open beside them.

## A tour in six words

- **Object**: anything drawn, moved or touched. It has a picture, a place, a velocity, and rules. [Objects](reference/objects.md)
- **Rule**: what happens when an object touches another or a screen edge, as a list of commands. [Collisions](reference/collisions.md)
- **Action**: a named thing an object can do (`up`, `fire`). Keys never live on objects. [Input](reference/input.md)
- **State**: a mode of the game (title, playing, paused) that binds keys to actions and holds the state's own objects. [States](reference/states.md)
- **Condition**: a test checked every frame ("no aliens left", "lives at 0") that runs commands when it holds. [Conditions](reference/states.md#conditions)
- **Value**: a number, a name, or an operation written as tags (`<subtract>`), worked out at load. [Values](reference/format.md#values)

A small whole game is in the [root readme](../readme.md#a-whole-game). Run a game with `output/Debug/xgecli <name>` ([programs](reference/programs.md)).

## The reference

| Page | What is in it |
|---|---|
| [The game file](reference/format.md) | layout, the one rule, values and arithmetic, names, window, variables, styles, colours |
| [Objects](reference/objects.md) | parts of an object, groups, columns and rows, lines of text, pieces and `from=`, spawn and despawn |
| [Pictures](reference/pictures.md) | shapes, lines, polygons, bitmaps, SVG, sheets, text, animation, looks, poses, turning, light, height |
| [Sounds](reference/sounds.md) | notes, waves, music, recordings, playing |
| [Collisions](reference/collisions.md) | rules, filters, how a touch is found, knocks |
| [Commands](reference/commands.md) | every verb, where each may go, refused presses |
| [Input](reference/input.md) | keys, mouse and pads, press/hold/release, held keys |
| [Timers and paths](reference/timers.md) | timers, time left, paths |
| [States and scenes](reference/states.md) | the stack, scenes, conditions, keeps, best tables, turns, demos, computer players |
| [Maps](reference/maps.md) | ASCII maps, mazes, cells for grid puzzles |
| [Board games](reference/boards.md) | moves, the picker, marks, put pieces |
| [Cards](reference/cards.md) | packs, holders, patterns, tricks, the computer |
| [The shipped games](reference/games.md) | the key convention and each game, by kind |
| [Programs](reference/programs.md) | `xgecli`, `xgegui`, generating a program |
| [The engine](reference/engine.md) | loading, each frame, backends, building, source map |
| [Known limitations](reference/limits.md) | what it cannot do yet |

## Word index

| Family | Words | Page |
|---|---|---|
| moving | `move`, `hop`, `jump`, `land`, `climb`, `thrust`, `turn`, `swing`, `reverse`, `stop`, `track` | [moving](reference/commands.md#moving) |
| steering | `aim`, `chase`, `hunt`, `wander`, `follow`, `go` | [steering](reference/commands.md#steering), [paths](reference/timers.md#paths), [mazes](reference/maps.md#walking-a-map) |
| touching | `bounce`, `deflect`, `knock`, `stick`, `wrap`, `ride`, `carry`, `hole` | [touching](reference/commands.md#touching), [knocks](reference/collisions.md#knocks) |
| in and out of play | `die`, `reveal`, `fire`, `drop`, `spawn`, `despawn`, `reset`; `waiting` (`ready`), `hidden` on an object | [in and out of play](reference/commands.md#in-and-out-of-play), [spawn](reference/objects.md#spawn-and-despawn) |
| places | `centered` (`offset`), `left`, `right`, `top`, `bottom` (`offset` in from the side) in a position | [objects](reference/objects.md#parts-of-an-object) |
| hitboxes | `hitbox` with `all`, `horizontal`, `vertical`, `top`, `bottom`, `left`, `right` (in from the picture's sides), or a `rectangle` or `circle` and `position` | [how a touch is found](reference/collisions.md#how-a-touch-is-found) |
| numbers | `inc`, `dec`, `set`, `copy`, `rank`, the operations (`add`, `subtract`, `multiply`, `divide`, `clamp`, ...); `update="live"` on a variable, position or heading | [numbers](reference/commands.md#numbers), [arithmetic](reference/format.md#arithmetic-as-tags), [live numbers](reference/format.md#live-numbers-of-an-object) |
| look and sound | `become`, `play`, `silence`, `music`; an empty `sprite name` (a look that draws nothing) | [look and sound](reference/commands.md#look-and-sound), [numbers only](reference/objects.md#numbers-only) |
| flow | `push`, `pop`, `enter`, `alternate`, `pass`, `out`, `demo`, `trigger` (in a key, `otherwise`) | [flow](reference/commands.md#flow) |
| rules in rules | a condition holding conditions (`first`, `otherwise`), a collision holding collisions (`first`), `sprite=` tests, `self`/`others`, `other` in a collision rule, `lowest`, `highest`, `unequal` | [conditions in conditions](reference/states.md#conditions-in-conditions), [collisions](reference/collisions.md) |
| cells | `settle`, `slide` (`class=`), `cycle`, `fall`, `grow`, `split`, `send`, `through` | [cells](reference/maps.md#cells) |
| cell puzzles | `shove`, `merge`, `undo`, `here`, `around` (`straight`, `alike`) in `count` and `trigger`; a `condition` in an action | [pushing, merging and counting](reference/maps.md#pushing-merging-and-counting), [conditions in an action](reference/commands.md#conditions-in-an-action) |
| boards | `pick`, `place`, `put`, `flank`, `close`, `moves` | [boards](reference/boards.md) |
| pictures | `pictures` (a `bitmap name` a sprite takes by `bitmap from`, a `drawing name` by `drawing from`, and each `sheet`), `cell`, `cells` (sprites, frames and cards cut from one picture) | [bitmaps](reference/pictures.md#bitmaps), [sheets](reference/pictures.md#sheets) |
| holders | a pack's `columns name` and `rows name`, `holds`, `spread`, `accepts` (`condition` with `top`, `any`, `wild`, `empty`), `carry`, `top`, `order`, `counts`, `trick`, `score`, `picked`, `owns`, `starts`, `distribute`; in actions `send from into`, `order group`, `distribute`, `pick` (`most`), a holder's `become` | [holders](reference/cards.md#holders) |
| players | `turns` on a group, `pass` (`member=`), a member's `think`, `owns` (`object=` or `class=`) and `team`, live `turn` | [players as a group](reference/states.md#players-as-a-group) |
| patterns and tricks | `patterns` (`alike`, `run`), melds, tricks | [patterns](reference/cards.md#patterns), [tricks](reference/cards.md#tricks) |
| computer | `computer`, `hold`, `think` | [computer players](reference/states.md#computer-players) |
