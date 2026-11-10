# Verbs

**Status:** Built: each command tag is a file in `xge/lib/verbs/` holding its name, the places it may be written, how its numbers are worked out, the names it checks and what it does; `verbs/all.cpp` lists them. Open: its reading from XML, its printing, and its own data. Reference: [commands](../reference/commands.md), [the engine](../reference/engine.md#source-map).

## What we looked for

One place for everything the engine knows about a verb. A verb used to be spread over about ten: the schema, a list of tag names and a branch of `readCommand` in `load/xml.cpp`, a branch of `makeCommand`, a `Cmd*` struct, a lambda in `CommandExecutor::run`, a case of the load-time check, a table of places, a second table of tag names for messages, and the generator. Adding one widened every list, and removing one cost as much, so none were removed.

## The shape

- **A `Verb` record a file** (`verbs/verb.h`): `name`, up to three `forms` (how a message names it, the places it may go, the kind of command it makes), `onRelease` (it acts when a key is let go), and the functions `make`, `check` and `run`. `bounce.cpp` holds `<bounce>` and nothing else.
- **The command processor is a lookup.** `makeCommand` finds the verb by its tag and calls `make`; the load-time check calls `check`; `CommandExecutor::run` finds the verb of a command (`verbOf`, a table by the command's kind) and calls `run` with a `Doer`: who is running it and what it touched. The place check reads the forms.
- **Forms** are for the few verbs written more than one way: `<reset />`, `<reset object>` and `<reset scene>` make three kinds of command; `<despawn />` is about the object running it and goes where that means something, `<despawn object>` anywhere a change does.
- **The work stays where it was**: the arithmetic in `play/verbs.cpp` and `play/geometry.cpp`, what needs the game in `CommandExecutor` and `Game`. A verb's file says when each is called.
- **The `Cmd*` structs and the `Command` variant stay in `game/command.h`**: tests and the generator make them, and `run` reads its own with `std::get`.

## Choices

- **A written list, not files that add themselves.** A file-level object that registers itself is the usual way, and reads well. But the engine is a static library, and a linker takes from a library only the files something names: a self-registering verb would be left out, silently, unless every program linking the engine passed a whole-archive flag, spelt differently by each compiler. `all.cpp` names each verb once. A test holds the list against the schema's groups of commands, and against the generator's list of files, so a forgotten line fails.
- **Places as the engine has them, narrower than the schema's.** The schema sorts commands into groups and each place takes some groups; the engine also knows that a `<wrap />` does nothing against another object. Each form names its places from the same groups (`places::changes`, `places::self`...), and the test checks that no verb goes where the schema would refuse it.
- **A condition in an action is not a verb.** It is the list's own flow, run and checked by the code that walks the list.
- **Compiled as one.** Seventy-five small files each including the game's headers would more than double the time a generated program takes to build, so CMake compiles the folder as one unit (`UNITY_GROUP`). Hence a verb's functions are named for it (`makeBounce`, `runBounce`), not hidden in an unnamed namespace where two files' would collide.
- Rejected: one table of all verbs in one file (the first plan): it centralises the list but leaves each verb's code where it was.

## Open

- **Reading.** `readCommand` in `load/xml.cpp` still reads every verb's attributes and children into the shared `RawCommand`. Moving each branch to its verb needs the reading helpers out of `load/xml.cpp`, and a decision: a generated program compiles the verbs but not the XML reader.
- **Printing** (`operator<<` for `Command` and `RawCommand` in `game/command.cpp`) is still one list each.
- **Its own data.** `RawCommand` is one struct of about fifty fields shared by all, each meaning something different per verb. A verb owning its fields would end that; it changes the generator's `data.xsl` heavily.
- The helpers of a single verb still in `CommandExecutor` (`hop`, `huntByHop`, `spawnProjectile`) could move beside it.
