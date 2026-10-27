# The kit and from=

**Status:** Built 2026-10-25 (the reference is [readme: The kit and from=](../readme.md#the-kit-and-from)).

## What we looked for

Objects could be written at four levels: the game's `<objects>`, a state's, a scene kit at the top of a state's `<scenes>`, and a scene's own. With spawn and despawn built ([pools-and-enemies](10-pools-and-enemies.md)), that seemed wrong to the author: something only ever spawned (a bomb, a shot) was an object marked `<exists>false</exists>` in whichever list its spawner could see, and a thing shared by several states or scenes had to be written where all of them could take it. Counted on the 33 games (2026-10-24): 453 objects and groups in states, 139 in scenes, 41 in scene kits, 35 at the game level.

The wish: define objects once, up top, and customize them lower down.

## What we looked at

| Option | Verdict |
|---|---|
| Top-level objects as **definitions by `class=`** with no `name=`, an object coming into being where a `name=` with that `class=` is written (the author's first idea) | `class=` is a rule category, shared by unlike things (Asteroids' `hazard` is the rocks, the saucer and its shot), so one definition per class would force every enemy of one class to be written out in full: Galaxian's rows, which all react the same way |
| The **scene kit moved up** to the game, taken by name, with a `from=` for a take under another name | Chosen. It matches what `from=` already means on states and scenes |
| Keeping `<exists>false</exists>` beside the kit | Two ways to say "only spawned"; dropped |

Names looked at for the list: `kit`, `blueprints`, `kinds`. `kit` was already the word the docs and the games used for the scene kit.

## Chosen

- **`<kit>`**, after `<styles>`, holds objects and groups; nothing in it is in play. A piece can be `from=` an earlier piece.
- **In any `<objects>`, `from="piece"` takes a piece**, under the name written; an object or group named after a piece (or after a named member of a group piece) takes it by that name.
- **One change rule** for a take, a state `from=` and a scene `from=`: sprites and animations by name, whole; the x and y halves of position, velocity, acceleration and padding each on their own; variables, actions and timers by name; collision rules added after, solids in place of all; members, rows, columns and cells in place of all; inner groups and objects by name, the same way; anything else in place of its tag. The old take could change only sprites, position and velocity halves, `<hidden>` (now `<active>`), `<drawn>` and variables.
- **Lifetime is where a thing is placed**: the game's for the game, a state's while it is current, a scene's in its scene.
- **`<spawn>`, `<fire>` and `<release>` can name a piece.** The piece comes into the list of what spawns it, as the description its copies are made from. `<exists>` left the schema; `<most>` stays.

Decided while building, not by the author (to be confirmed):

- **A waiting copy sits beside its spawner**, not at the game level, so it reads its scene's variables (Kaboom's bombs fall at their wave's speed, Demon Attack's small demons). Megamania's bombs, a state's own before, are now each wave's.
- **A member taken alone keeps its group**: Berserk, Adventure and Haunted House take named members of a kit group, and the member keeps the group's class, rules and picture.
- **Sprites are replaced whole**, as takes already did, rather than changed part by part as a member changes its group.

## How it was built

- **The merge is done on the XML, before reading** (`load/merge.cpp`). Each `<objects>` is copied into small in-memory elements, laid out (`layOut`: inherited list, then each written element changing one of the same name, a member or inner group of one, taking a piece, or added), and read through an `XmlNode` view of them, so the readers did not change. `XmlNode::getAttributes` was added to every backend for the copy.
- **The generator does the same merge in XSLT** (`games/generators/windows-cpp/merge.xsl`), into a merged tree that `data.xsl` then writes out; `take()`, `own()`, `RawSceneChange`, `RawSceneEntry` and the state and scene kits in the data went away. The rule was rewritten from step-by-step (each written child applied to the list in turn) to a definition that XSLT 1.0 can say directly: base's children in order, each kept or changed by the last one alike, then the written ones that took no place. `xgetest-generated` compares every field of every game, so the two must agree to the order of objects.
- **What is spawned is read from the XML too**: the `<spawn>`, `<fire>` and `<release>` inside an object, or a state's inputs, conditions, computer and timers, or a scene's conditions and timers, and inside a path they follow, a `<keys>` set their inputs take and a `<conditions>` set a condition takes. Waiting copies are added round by round, alphabetically, so a piece whose copies release another (Asteroids' medium rocks the small ones) brings that one in too.
- **The games** (20 used scene kits or `<exists>`) were converted: scene kits and descriptions moved to the game's kit. Asteroids' rocks, which were one group with the medium and small ones inside, became a `rock` piece the three sizes are `from=`, and the wave-cleared condition an `<all>` of the three.

## Open

- **Placing a waiting piece somewhere other than beside its spawner** (Megamania's bombs in the state): no way to say it yet.
- **Generated code size**: each take is written out whole in each scene (Berserk 3,173 lines generated), where the kit was written once before.
- **A map taking pieces from the kit** ([groups](06-groups.md)).
- **Changing a numbered member** (`rocks.2`) without naming it.
