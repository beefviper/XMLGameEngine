# File format

**Status:** Built. Reference: [the game file](../reference/format.md).

## Why XML

- **Chosen: XML and XSD, one file.** Familiar (a reader of HTML can follow `pong.xml`; `name` and `class` are HTML's), validated before the game runs, and transformable (XSLT makes a program of it, [generator](40-generator.md)).
- Looked at: VGDL (compact, unfamiliar, no schema tooling), YAML (weaker validation), JSON (no comments).
- **One file, not rules plus level:** blocks are a group in columns and rows or an ASCII `<map>`; pictures are bitmaps. No level files, XInclude or shared files across games.
- **One list of object parts** (`objectParts`), used by objects, groups and map pieces, so they cannot drift apart. Before, four copies had drifted.
- **Three settings, three jobs:** `<waiting />` is out of play until revealed; `<hidden />` is in play but unseen; `<off />` in collisions is seen but touches nothing. They were `<active>false</active>`, `<drawn>false</drawn>` and `<enabled>false</enabled>`, true/false content in a language whose other choices are empty tags or picks; the author chose the empty tags. A setting an outer piece makes and a take undoes has its pair (`<ready />` for `<waiting />`), and in the one change rule either written replaces the other. `<lockstep />` and the window's `<fullscreen />` went the same way.
- **A verb's one number is its content:** `<chase object="hero">2</chase>`; extras follow as named tags (`<hop direction="up">40<seconds>0.3</seconds></hop>`). One rule beats remembering which verbs wrap their number.
- **Defaults for boilerplate:** background black, not fullscreen, 60 frames a second, and a written `<collisions>` is on. No `<collisions>` still touches nothing.
- The schema fixes child order; the engine looks elements up by name. A game with no schema location skips validation.

## Attributes name, content computes

Reached by removing a small scripting language that had grown inside attribute strings (`src="shape.circle(r,'color.white')"`, `action="inc('p.score');reset()"`).

- **An attribute names or picks** something (`name`, `class`, `object`, `edge`, `direction`, `pitch`...). Everything else is element content; a command list is an ordered list of tags.
- Why: XSD cannot validate inside a string, XSLT sees opaque text, and `;` hides sequencing that document order shows.

| Option | Verdict |
|---|---|
| Function-call strings | Opaque to XSD and XSLT. Removed |
| One element per call | Verbose |
| **Functions become elements and value tags; plain arithmetic stays text** | Chosen |

- A command list is sequential with no control flow. The line to hold: **no conditionals, loops, variables or dynamic dispatch inside a command list.**
- **One name per attribute.** Lists once packed in strings (`button="space p"`, `keys="a b"`, `number="2 4 6"`) are now an element each, written first: `<button>`, `<number>`, `<odd />`. An attribute is an `xs:NMTOKEN`, so the schema refuses a space. `<button>`, not `<key>`, since it names pad buttons too. Rejected: `<key value="s" />` wrappers, repeating the input per key. `pitch="F#4"` stays: one standard name, like a colour's.

## Commands in groups

- The schema once had one list of all commands, legal everywhere, and the loader turned away misplaced ones. **Chosen:** nine groups by what a command needs (changes, flow, self, steering, touch, edge, bounce, controls, press), each command in exactly one, and each place naming the groups it takes. The table is in [commands](../reference/commands.md#where-a-command-can-go).
- The loader's places are each verb's own, named from the same groups (`places::` in `verbs/verb.h`), and a test holds them against the schema ([verbs](14-verbs.md)).
- Sorting found gaps, filled rather than written in: an action can now stop, reverse, spawn and steer, and a collision rule can fire.
- Open: the two kinds of `<collision>` differ by attribute, which XSD 1.0 cannot tell apart; two tags would let the schema check it.
- The schema checks shape and placement, not names: an unknown key or an unowned variable is the loader's to refuse ([games written by another AI](54-games.md)).
