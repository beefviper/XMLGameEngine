# File format

**Status:** Built.

## File format: why XML

- **Chosen: XML + XSD, one file.** Reasons: familiarity (a non-programmer who has seen HTML can follow `pong.xml`; `name` and `class` are borrowed from HTML), validation (structure, required attributes, types before the game runs), and transformation (XSLT could turn a game into docs or native source).
- Looked at: academic text VGDL (SpriteSet / LevelMapping / InteractionSet / TerminationSet plus an ASCII level file: compact, unfamiliar, no schema tooling), YAML (Griddly's GDY: compact, weaker validation), JSON (awkward for comments and authors).
- One file, not rules + level: blocks are laid out with a `<group>` in columns and rows, and pictures with `<bitmap>`, rather than an ASCII map. A separate level file for larger games is open ([ideas](57-ideas.md)).
- **One list of object parts.** Everything an object may have after its sprites, animations and position (velocity through timers) is one `xs:group`, `objectParts`, used by an object, a group, an object on a map and a group on a map, so they cannot drift apart: before (2026-10-25) there were four copies, and the map groups had lost heading, drag, mass, pull and seek and put the maze before the light. What differs stays on each type: a group's layout and members, a map piece's `char=` and no position, a member's or cell's short list.
- **In play, three flags, three jobs** (2026-10-27). `<active>false</active>` is out of play until a `<reveal>`, `<release>` or `<fire>` (not drawn, touching nothing, not counted); `<drawn>false</drawn>` is in play but unseen (an exit, a trigger box); `<enabled>false</enabled>` in `<collisions>` is seen but touching nothing. `active` was `hidden` until then: the old name read as "not drawn", which is `drawn`'s job. `exists` went with the [kit](13-kit.md): a thing written but not yet complete or in play is a piece.
- **A verb's one number is its content** (2026-10-27, the author's call). `<chase object="hero">2</chase>`, `<wander>2</wander>`, `<jump direction="up">40</jump>`, `<hop>`, `<hunt>` and `<hole>` the same; what else it has follows as named tags (`<seconds>`, `<arc>`, `<near>`, `<ahead>`, `<rough>`), so a verb with extras mixes its number and tags: `<jump direction="up">rowgap<seconds>leap</seconds></jump>`. Before, these wrapped the number in `<speed>`, `<distance>` or `<radius>`, and `<hop>` took either; one rule is easier to remember than which verbs wrap. A diagonal `<hop>` keeps `<x>` and `<y>`: two numbers, so two names.
- The schema fixes child order; the engine looks elements up by name and does not depend on it. A game with no `xsi:noNamespaceSchemaLocation` skips validation. `<framerate>` is an `xs:unsignedByte`; every other number is a [value](04-values-and-names.md).
- **The schema checks shape, not meaning.** It accepts commands where the engine does nothing with them, unknown key names, and variables with no owner. A game written by another AI from the schema alone passed validation and showed a blank screen ([games-classic](54-games-classic.md)). The loader rejects unknown commands and names that do not exist, but is not strict about where a command goes. Making it refuse unknown button names and misplaced commands is a decision for the author.


## Attributes name, content computes

The rule, reached by removing a small scripting language that had grown inside attribute strings (`src="shape.circle(r,'color.white')"`, `action="inc('p.score');reset()"`):

- An attribute **names or picks** something: `name`, `class`, `object`, `variable`, `state`, `action`, `button`, `edge`, `direction`, `unless`, and for sounds `sound`, `wave`, `pitch`, `to`. Everything else is element content. A command list is an ordered list of command tags.
- Why: XSD cannot validate the inside of a string; XSLT sees opaque text; `;` hides sequencing that document order shows.

| Option | Verdict |
|---|---|
| A. Function-call strings | Compact, opaque to XSD and XSLT. Removed |
| B. One element per call | Validatable, verbose |
| C. Structure only where a string stood in for a tree | Same benefits applied selectively |
| **D. Functions become elements and value tags, plain arithmetic stays text** | **Chosen**, done all at once in the games, schema, loader and tests |

- Named actions stay (`<action name="up">` bound by a state): the indirection is good design regardless of syntax ([states-and-conditions](07-states-and-conditions.md)).
- A list of commands is accepted because it is sequential with no control flow. The line to hold: no conditionals, loops, variables or dynamic dispatch inside a command list.
- Rejected elsewhere: statements as a mini language (MML strings for sound, [sound](33-sound.md)), one multi-line text block for bitmap rows ([pictures](30-pictures.md)).

### Lists and parts still packed into attribute strings

**Status:** Open (noted 2026-10-13). Beef: several values in one string break the rule above, even though each value is only a name; the schema sees one string and the reader splits it.

- `<input button="space p escape">`: several keys in one attribute (56 inputs in the games repeat another's commands and could use it, which is why it came up).
- `<inputs keys="player menu">`: several `<keys>` sets.
- `number="2 4 6"` (and `odd`, `even`) on a group's `<row>` and `<column>`: several rows or columns.
- `pitch="F#4"` and `to=`: a letter, a sharp or flat and an octave in one word, or a number of hertz ([sound](33-sound.md)).
- The XSD can check these only with a pattern; XSLT has to split them (the windows-cpp generator walks the words one by one).
- Beef (2026-10-16): leave these, and the arithmetic still written as text, as they are for now; we will probably change both at some point.
- Possible forms, none chosen: one element per value (`<input><button>space</button><button>p</button>...`), an attribute per part for notes (`<note letter="F" sharp="true" octave="4">`, or `<note><pitch>...</pitch>`), or keep a single name per attribute and repeat the element. Shared `<shows>` sets (2026-10-13) avoid the problem: a state takes one set per `<show set="..."/>`.
