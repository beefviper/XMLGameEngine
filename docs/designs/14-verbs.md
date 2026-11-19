# Verbs

**Status:** Built: each command tag is a file in `xge/lib/verbs/` holding its name, the places it may be written, how its tag is read, how its numbers are worked out, the names it checks, what it does and how it is printed; `verbs/all.cpp` lists them. Reference: [commands](../reference/commands.md), [the engine](../reference/engine.md#source-map).

## What we looked for

One place for everything the engine knows about a verb.

- A verb used to be spread over about ten: the schema, a list of tag names and a branch of `readCommand` in `load/xml.cpp`, a branch of `makeCommand`, a `Cmd*` struct, a lambda in `CommandExecutor::run`, a case of the load-time check, a table of places, a second table of tag names for messages, and the generator.
- Adding one widened every list, and removing one cost as much, so none were removed.

## The shape

- **A `Verb` record a file** (`verbs/verb.h`): `name`, up to three `forms` (how a message names it, the places it may go, the kind of command it makes), `onRelease` (it acts when a key is let go), and the functions `read`, `make`, `check`, `run`, `hold` (it goes on while its key is still down as a state begins) and `print`, each left out when the verb has none. `bounce.cpp` holds `<bounce>` and nothing else.
- **The command processor is a lookup.** `readCommand` finds the verb by its tag, reads the names any command may have (its attributes) and calls `read` for the rest; `makeCommand` calls `make`; the load-time check calls `check`; `CommandExecutor::run` finds the verb of a command (`verbOf`, a table by the command's kind) and calls `run` with a `Doer`: who is running it, what it touched, and the game (`touching()`, `atEdge()`, `game()`). `run`, `print` and `check` take the verb's own `Cmd*` (`runAs<runBounce>` picks it out), so a file reads: the tag, its numbers, the work. The place check reads the forms.
- **Forms** are for the few verbs written more than one way: `<reset />`, `<reset object>` and `<reset scene>` make three kinds of command; `<despawn />` is about the object running it and goes where that means something, `<despawn object>` anywhere a change does.
- **The work is the verb's file.** What one verb does is a function in its file, given the game: `bounceOff` and `bounceOffEdge` above `runBounce`, the spawn's placing, the hop's flight, the knock's trade of speed.
  - What several share stays shared: `rub` (bounce and knock) and the object helpers in `play/verbs.cpp`, the arithmetic in `play/geometry.cpp`, `hopObject` (a hop, and a hunt by hops) in `hop.cpp`, declared in `verb.h`.
  - `CommandExecutor` runs lists of commands and holds what they share: the trigger's flood guard and the held share of an `<inc>`.
- **Printing is the verb's too.** `operator<<` for a `Command` asks its verb's `print`; a `RawCommand` prints its tag. Only the two kinds that are no verb's, a condition in a list (`CmdWhen`) and a command worked out as it runs (`CmdLive`), are printed in `game/command.cpp`. The tests of generated games compare these printouts, so the text is part of what a verb is.
- **The `Cmd*` structs and the `Command` variant stay in `game/command.h`**: tests and the generator make them.

## Choices

- **A written list, not files that add themselves.** A file-level object that registers itself is the usual way, and reads well.
  - But the engine is a static library, and a linker takes from a library only the files something names: a self-registering verb would be left out, silently, unless every program linking the engine passed a whole-archive flag, spelt differently by each compiler.
  - `all.cpp` names each verb once.
  - A test holds the list against the schema's groups of commands, and against the generator's list of files, so a forgotten line fails.
- **Places as the engine has them, narrower than the schema's.** The schema sorts commands into groups and each place takes some groups; the engine also knows that a `<wrap />` does nothing against another object. Each form names its places from the same groups (`places::changes`, `places::self`...), and the test checks that no verb goes where the schema would refuse it.
- **A condition in an action is not a verb.** It is the list's own flow, run and checked by the code that walks the list.
- **Compiled as one.** Seventy-five small files each including the game's headers would more than double the time a generated program takes to build, so CMake compiles the folder as one unit (`UNITY_GROUP`). Hence a verb's functions are named for it (`makeBounce`, `runBounce`), not hidden in an unnamed namespace where two files' would collide.
- **A generated program reads with them too.** It carries its game as a tree of C++ data read by the engine's own reader (`load/xml.cpp` through `xml/tree.h`), so a verb's reader is the one it runs, with no XML library.
- **Its own data.** `RawCommand` was one struct of about fifty fields shared by all, each meaning something different per verb (`amount` was a bounce's share, a hop's distance and a spawn's count). Now it holds the verb, its line, the names every command may have (`object`, `direction`, `into`...) and `own`, a `std::variant` of the verb's own data as written (`RawBounce`, `RawHop`, `RawSpawn`... beside `RawValue` in `game/command.h`). Each part is named for what it is there and is a `std::optional` when it may be left out; each struct lists its numbers for `RawCommand::forEachValue`, the one walk renames and live numbers go by.
  - Two verbs share one struct where they read alike: a hunt by hops reads a `RawHop`, a `<distribute>` a `RawSend`. A verb with nothing to work out keeps what it read as made (the cells of a `<trigger>` or a `<count>` are a `CellPlace`).
  - `RawCommand` went from 3,328 to 2,496 bytes; a spawn's eleven numbers are most of it. It is load-time data only.
- Rejected: one table of all verbs in one file (the first plan): it centralises the list but leaves each verb's code where it was.

## Open

- **A verb added in one place.** A new verb still touches its file, the list in `all.cpp`, the generator's list in `engine.xml`, its `Cmd` and `Raw` structs and their variants in `game/command.h`, and the schema. The author wants to approach "a `.h` and `.cpp` per verb and one list" at some point: the structs move to the verb's header, one list builds both variants and the table, and the generator copies the folder. The schema keeps its element.
