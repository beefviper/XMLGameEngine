# XMLGameEngine documentation

A game is one XML file of tags, checked against `games/xgedef.xsd`. The engine reads it and plays it, with no code in the file. This page is the way in: what each part of the docs is for, a short tour, and an index of every word.

## Where to look

| Folder | For | Use it when |
|---|---|---|
| [reference/](reference/) | how the engine works **today**: every tag, what it does, the programs, the build | writing a game, or checking what a tag does. The code is the truth; these pages follow it |
| [designs/](designs/00-designs.md) | one small file per topic: what was looked at, what was chosen and why, what is open | changing the language or the engine |
| [agents/notes.md](agents/notes.md) | gotchas and working rules for anyone (or any AI) changing the code | before a change |

New to it: read the tour below, then [the game file](reference/format.md), [objects](reference/objects.md), [collisions](reference/collisions.md), [input](reference/input.md) and [states](reference/states.md), with a game from [the shipped games](reference/games.md) open beside them.

## A tour in six words

- **Object**: anything drawn, moved or touched. It has a picture, a place, a velocity, and rules. [Objects](reference/objects.md)
- **Rule**: what happens when an object touches another or a screen edge, as a list of commands. [Collisions](reference/collisions.md)
- **Action**: a named thing an object can do (`up`, `fire`). Keys never live on objects. [Input](reference/input.md)
- **State**: a mode of the game (title, playing, paused) that binds keys to actions and holds the state's own objects. [States](reference/states.md)
- **Condition**: a test checked every frame ("no aliens left", "lives at 0") that runs commands when it holds. [Conditions](reference/states.md#conditions)
- **Value**: a number, a name, or arithmetic on names (`window.right - margin`), worked out at load. [Values](reference/format.md#values)

```xml
<game xmlns:xsi="http://www.w3.org/2001/XMLSchema-instance" xsi:noNamespaceSchemaLocation="../xgedef.xsd">
  <window name="Bounce"><width>640</width><height>480</height></window>
  <states>
    <state name="playing">
      <objects>
        <object name="ball">
          <sprite><circle><radius>8</radius></circle></sprite>
          <position><x><centered /></x><y><centered /></y></position>
          <velocity><x>3</x><y>2</y></velocity>
          <collisions>
            <collision edge="vertical"><bounce /></collision>
            <collision edge="horizontal"><bounce /></collision>
          </collisions>
        </object>
      </objects>
      <inputs>
        <input button="space"><reset /></input>
      </inputs>
    </state>
  </states>
</game>
```

A ball bounces round the window, and Space puts it back. Run a game with `output/Debug/xgecli <name>` ([programs](reference/programs.md)).

## The reference

| Page | What is in it |
|---|---|
| [The game file](reference/format.md) | layout, the one rule, values and arithmetic, names, window, variables, styles, colours |
| [Objects](reference/objects.md) | parts of an object, groups, columns and rows, the kit and `from=`, spawn and despawn |
| [Pictures](reference/pictures.md) | shapes, lines, polygons, bitmaps, SVG, text, animation, looks, poses, turning, light, height |
| [Sounds](reference/sounds.md) | notes, waves, music, playing |
| [Collisions](reference/collisions.md) | rules, filters, how a touch is found, knocks |
| [Commands](reference/commands.md) | every verb, where each may go, refused presses |
| [Input](reference/input.md) | keys, mouse and pads, press/hold/release, held keys |
| [Timers and paths](reference/timers.md) | timers, time left, paths |
| [States and scenes](reference/states.md) | the stack, scenes, conditions, keeps, best tables, turns, demos, computer players |
| [Maps](reference/maps.md) | ASCII maps, mazes, cells for grid puzzles |
| [Board games](reference/boards.md) | moves, the picker, marks, put pieces |
| [Cards](reference/cards.md) | decks, piles, hands, patterns, tricks, the computer |
| [The shipped games](reference/games.md) | the key convention and each game, by kind |
| [Programs](reference/programs.md) | `xgecli`, `xgegui`, generating a program |
| [The engine](reference/engine.md) | loading, each frame, backends, building, source map |
| [Known limitations](reference/limits.md) | what it cannot do yet |

## Word index

| Family | Words | Page |
|---|---|---|
| moving | `move`, `hop`, `jump`, `leap`, `land`, `climb`, `accelerate`, `thrust`, `turn`, `swing`, `reverse`, `stop`, `track` | [moving](reference/commands.md#moving) |
| steering | `aim`, `chase`, `hunt`, `wander`, `follow`, `go` | [steering](reference/commands.md#steering), [paths](reference/timers.md#paths), [mazes](reference/maps.md#walking-a-map) |
| touching | `bounce`, `deflect`, `knock`, `stick`, `wrap`, `ride`, `carry`, `hole` | [touching](reference/commands.md#touching), [knocks](reference/collisions.md#knocks) |
| in and out of play | `die`, `reveal`, `fire`, `release`, `drop`, `spawn`, `despawn`, `reset` | [in and out of play](reference/commands.md#in-and-out-of-play), [spawn](reference/objects.md#spawn-and-despawn) |
| numbers | `inc`, `dec`, `set`, `copy`, `rank`, `formula` | [numbers](reference/commands.md#numbers), [arithmetic](reference/format.md#arithmetic-as-tags) |
| look and sound | `become`, `play`, `silence`, `music` | [look and sound](reference/commands.md#look-and-sound) |
| flow | `push`, `pop`, `enter`, `turns`, `pass`, `out`, `demo`, `trigger` | [flow](reference/commands.md#flow) |
| cells | `settle`, `slide`, `cycle`, `fall`, `grow`, `split`, `send`, `through` | [cells](reference/maps.md#cells) |
| boards | `pick`, `place`, `put`, `flank`, `moves` | [boards](reference/boards.md) |
| cards | `deck` (`sheet`), `deal`, `take`, `lay`, piles, patterns, tricks | [cards](reference/cards.md) |
| computer | `computer`, `hold`, `think` | [computer players](reference/states.md#computer-players) |

## Keeping the docs

- **Docs move with code.** A change to a tag or to behaviour updates its reference page, and the matching design file, in the same commit.
- **Reference pages say what is.** Short sections, a small example each, no history. Why it is so goes in the design file.
- **Design files stay small** (about 6 KB): what was looked for, looked at, chosen and why, what is open. No diaries or test logs.
