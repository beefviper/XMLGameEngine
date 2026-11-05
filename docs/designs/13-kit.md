# The kit and from=

**Status:** Built. Reference: [the kit and from=](../reference/objects.md#the-kit-and-from).

## What we looked for

Objects could be written at four levels: the game's, a state's, a scene kit at the top of a state's scenes, and a scene's own. With spawn and despawn built, something only ever spawned (a bomb, a shot) was an object marked `<exists>false</exists>` wherever its spawner could see it, and a thing shared by several states had to be written where all could take it.

The wish: define objects once, up top, and customize them lower down.

## What we looked at

| Option | Verdict |
|---|---|
| Top-level definitions by `class=`, an object made where a name with that class is written | `class=` is a rule category shared by unlike things (Asteroids' `hazard` is rocks, saucer and shot), so every enemy of one class would be written out in full |
| **The scene kit moved up to the game,** taken by name or `from=` | Chosen: it matches what `from=` already means on states and scenes |
| Keeping `<exists>false</exists>` beside it | Two ways to say "only spawned"; dropped |

## Chosen

- **`<kit>`** holds objects, groups and maps; nothing in it is in play. A piece can be `from=` an earlier piece.
- **In any `<objects>`, `from="piece"` takes a piece** under the name written; an object named after a piece takes it by that name.
- **One change rule** for a take, a state `from=` and a scene `from=`: sprites and animations by name, whole; x and y halves of position, velocity, acceleration and padding apart; variables, actions and timers by name; collision rules added after; members, rows and cells in place of all; inner groups by name; anything else in place of its tag.
- **Lifetime is where a thing is placed:** the game's, a state's, a scene's.
- **`<spawn>`, `<fire>` and `<release>` can name a piece;** its waiting copies sit beside the spawner, so they read its scene's variables (Kaboom's bombs fall at their wave's speed).
- A member taken alone keeps its group's class, rules and picture.
- **A map is a piece too:** Berserk's four rooms share tile, place, walls and exits, and each gives only its rows. Rejected: a scene `from=` the first room (it copies the robots, and a copy cannot be dropped).

## How it was built

- **The merge is done on the XML, before reading** (`load/merge.cpp`): each `<objects>` is copied into small in-memory elements, laid out, and read through an `XmlNode` view of them, so the readers did not change.
- **The generator does the same merge in XSLT** (`merge.xsl`). The rule was rewritten from step by step to a definition XSLT 1.0 can say directly: base's children in order, each kept or changed by the last one alike, then the written ones that took no place. `xgetest-generated` makes the two agree to the order of objects.
- What is spawned is read from the XML too, round by round, so a piece whose copies release another brings that one in.

## Open

- Placing a waiting piece somewhere other than beside its spawner.
- Generated code size: each take is written out whole in each scene.
- A map's pieces taken from the kit one by one; changing a numbered member (`rocks.2`) without naming it.
