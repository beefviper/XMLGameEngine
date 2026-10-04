# 01. Vision, file format and the VGDL landscape

**Status:** built. Arithmetic can be written as text, as an `<equation>` or as a `<formula>` ([arithmetic](#arithmetic-in-text-and-as-tags)); generating code from a game is the open item.

## What we want

- A video game description language in XML plus a C++ engine that runs it. One `.xml` file is the whole game.
- Target: 2D non-scrolling games of the late 1970s and early 1980s, described with a small closed vocabulary of verbs. Scrolling games later, with the same approach.
- The vocabulary grows only when a real game cannot otherwise be described. Like number systems: each extension is small and made because something real could not be said. The test for a good verb: it names a behavior (`carry` says an object rides another), not an implementation or one game's name for it.

## Declarative, with triggers

| Option | Verdict |
|---|---|
| A. Purely declarative (no loops, no conditionals) | Cleanest, but cannot say "game over at 15 points" |
| B. Embedded scripting / expressions | Expressive, but the file becomes a programming language in XML |
| **C. Declarative plus named triggers** | **Chosen.** Loops are the engine's per-frame update; conditionals are collision detection and `<condition>` ([04](04-states-conditions-and-input.md)) |

- Keep a **verb language** (pick known behaviors, give them parameters), not an **expression language** (compute anything). Plain arithmetic for positions across window sizes is accepted; function-call strings are not.
- Recognition over completeness: shared words (bounce, die) let a reader with no programming background follow a game. Tags read as nouns, attributes as descriptions, nesting as containment.
- Finiteness comes from a few mechanism families with parameters, not one verb per game. Difficulty grows with the number of contexts a mechanic appears in (elastic or not, solid or trigger), not the number of mechanics.
- The lineage argument: every game is a variation of Pong (Breakout = turn it 90 degrees, a grid of blocks that die; Space Invaders = blocks that step sideways and a ball that fires; Galaxian = invaders that fly in patterns). Small tweaks, repeated, rebuild the 1970s vocabulary. A written Pong-to-Galaxian chain of "which verb each step adds" is an unwritten idea.
- Origin: Pong in plain C++, pieces moved into XML one at a time. The current code is a ground-up rewrite that keeps the XML design; the prototype's collision code (detection and response tangled, hand-parsed action strings) is what [05](05-collisions.md) replaced.

## File format: why XML

- **Chosen: XML + XSD, one file.** Reasons: familiarity (a non-programmer who has seen HTML can follow `pong.xml`; `name` and `class` are borrowed from HTML), validation (structure, required attributes, types before the game runs), and transformation (XSLT could turn a game into docs or native source).
- Looked at: academic text VGDL (SpriteSet / LevelMapping / InteractionSet / TerminationSet plus an ASCII level file: compact, unfamiliar, no schema tooling), YAML (Griddly's GDY: compact, weaker validation), JSON (awkward for comments and authors).
- One file, not rules + level: grids are laid out with `<grid>`, `<group>` and `<bitmap>` rather than an ASCII map. A separate level file for larger games is open ([13](13-ideas.md)).
- The schema fixes child order; the engine looks elements up by name and does not depend on it. A game with no `xsi:noNamespaceSchemaLocation` skips validation. `<framerate>` is an `xs:unsignedByte`; every other number is a [value](02-values-variables-and-names.md).
- **The schema checks shape, not meaning.** It accepts commands where the engine does nothing with them, unknown key names, and variables with no owner. A game written by another AI from the schema alone passed validation and showed a blank screen ([10](10-games-as-tests.md)). The loader rejects unknown commands and names that do not exist, but is not strict about where a command goes. Making it refuse unknown button names and misplaced commands is a decision for the author.

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

- Named actions stay (`<action name="up">` bound by a state): the indirection is good design regardless of syntax ([04](04-states-conditions-and-input.md)).
- A list of commands is accepted because it is sequential with no control flow. The line to hold: no conditionals, loops, variables or dynamic dispatch inside a command list.
- Rejected elsewhere: statements as a mini language (MML strings for sound, [09](09-sound.md)), one multi-line text block for bitmap rows ([07](07-pictures-and-text.md)).

### Arithmetic in text and as tags

Arithmetic (`window.width.center - ball.radius`) was the last thing hidden from XSD and XSLT: a string a tool cannot see into. What we looked for: a way to write it that a schema can check and a transformation can walk, that does not cost every game its readability, and that does not need a second tool in the pipeline.

| Option | Notes |
|---|---|
| E1. Paste the string through | Works for C++ targets (infix is valid C++), not assembly |
| E2. XSLT parses it | Possible with `analyze-string`, awkward precedence parsing |
| E3. Parse once in C++ into AST-as-XML, XSLT walks it | Keeps the fragile part testable; adds a step |
| E4. Remove arithmetic: computation as elements | Fully toolable, verbose, needs a math vocabulary. **Built, in two spellings, beside the text** |

**What was built (E4).** Two value tags, siblings of `<random>`, so they go anywhere a number does, and the exprtk text stays valid everywhere (a game is not forced to change: every other game still uses text):

- **`<equation>`**, flat: steps, one operation each, operands as attributes, a step able to `name` its answer for the later ones; the last step is the answer. Three-address code, and the closest to an assembly target. The test game is Pong's title.
- **`<formula>`**, nested: one operation whose operands are elements that hold a name, a number, a `<random>` or another operation. Precedence is the nesting, and a chain (`a - b - c`) is one operation with several second operands. Closest to a C++ expression tree. The test game is Breakout's title.

Both use four operations named for their operands (`augend`/`addend`, `minuend`/`subtrahend`, `multiplicand`/`multiplier`, `dividend`/`divisor`); one table, `operationShape` in `command.cpp`, drives the loader, the evaluator and the printer.

**Why two forms, and two names.** The author saw them as different enough in taste (flat steps with names against one nested tree: `printf` against `cout`) that people would have strong opinions, so both exist. Different names (`<equation>`, `<formula>`) keep them from sharing an element: the step `<divide dividend=".." />` and the operation `<divide><dividend>..</dividend></divide>` are different types in different places, so neither can be mistaken for the other and each says so when written the wrong way.

**Decisions inside it.**

- *An operand is a name or a number, never an expression.* Otherwise `<dividend>a + b</dividend>` brings the hidden string back. It also keeps the format's rule (an attribute names or picks something) honest for the attribute operands: a name or a plain number is that, and anything computed is an operation. The price is verbosity: roughly 380 expressions in the games have two or three operators each, so converting all of them would give roughly a thousand operation lines (a rough count).
- *Step names are local to the equation*, with no dots, so they cannot be mistaken for `object.variable` or collide with the global namespace; a step name is used before a variable of the same name.
- *Evaluated at load* like every value, in the written order; a size-dependent position is finished once the window has measured, the same as for text.

**Rejected on the way.** Operators as empty tags between operands (`<divide>window.width<by/>2</divide>`, and with a `<divided-by/>` between named operands): mixed content the schema cannot check, with operands still strings, or a separator that carries no information. One element accepting both operand spellings: harder to validate and to explain than two names. Output names per operation (`quotient=`, `difference=`): a word and a schema type for every operation; `name` already means "this thing's label". Dotted outputs written into an object (`object.middle`): they would collide with real names.

**Schema and validators.** A formula's operands hold operations whose operands are operations, so the schema has types that contain themselves. Xerces handles that; `xsd_lite` did not, and now follows a named type that refers to itself (and frees the cycle when the model goes). A group that contains itself is still refused there.

**Open.** More operations (`min`, `max`, `negate`, `abs`, `clamp`, `sign`, `pick`) are not built: add one when a game cannot be written without it, as for verbs ([06](06-motion-and-verbs.md)), until then the exprtk text has them. A `<random>` step in an `<equation>`. Moving the other games over, a game at a time. And the reason for all of it: generating code for another target (XSLT to C++ or assembly) has no generator yet ([11](11-backends-build-and-layout.md)), so the tags are ready for it, but nothing yet reads them except the engine. [14](14-vocabulary-map.md) lists every word the language has.

## The VGDL landscape (reference only)

- Academic VGDL (Ebner et al. 2013; Schaul's `py-vgdl`) is declarative and text-based for 2D arcade games: hierarchical sprite sets with inheritance, a level map with a legend, collision-only interaction rules, win/lose termination sets. No scripting. It became the base of the GVGAI competition (survey: arXiv 1802.10363) and was largely replaced in AI research by Griddly (arXiv 2011.06363, YAML GDY, C++ core). LLMs have since been used to write VGDL directly (arXiv 2404.08706).
- Cousins: GDL (logic-based, verbose, nothing built in; the opposite philosophy), RBG and Ludii (board games), PuzzleScript (find-and-replace rules on a grid; the closest in spirit). Entity-component systems are the nearest structural neighbor.
- Why the field stayed small: built for one job (benchmark games for AI), a closed vocabulary means a closed language (a new dynamic needs engine code), and each generation is renamed and re-implemented.
- "No loops or conditionals" is really a production-rule system: the engine's tick is the loop and collision matching is the conditional. The price is the same everywhere: the language stops where its vocabulary runs out. XMLGameEngine makes the same bet with exprtk carrying per-rule arithmetic.
- Only the arXiv pages for GVGAI, Griddly and the two GitHub repositories were confirmed to exist; other links from the research were never re-checked.
- Open research angle: how small can a vocabulary be and still describe a class of games.

## Open

- Where the vocabulary boundary sits: scrolling as camera, world or verb; gravity as a world setting or a per-object force (built per object, [06](06-motion-and-verbs.md)).
- A `generate` step (XSLT or code from the description) does not exist.
