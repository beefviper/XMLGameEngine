# 01. Vision, file format and the VGDL landscape

**Status:** built. Arithmetic can be written as text, as an `<equation>` or as a `<formula>` ([arithmetic](#arithmetic-in-text-and-as-tags)); a game can be generated as a C++ program by XSLT ([generating](#generating-a-program-with-xslt)), so far only for Pong's tags.

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
- *A divisor of 0 is the engine's to catch, not the schema's:* XSD cannot see what a name holds, and could at best refuse the literal `0` (and not `0.0`, or a name that is 0). The engine refuses a divisor that is certainly 0 at load and names the `<divide>`. It cannot refuse one that is 0 only because its input is not final yet (another object's variable reads 0 until that object is built; a text's width until a window has measured it), so that gives 0 silently and is worked out again later where it can be. While the game is running a throw would end it, so a divisor of 0 gives 0 and one warning for each place. To tell these cases apart each answer carries a `late` flag, set by an operand that is an object variable or an object's size and passed on to everything made from it. The expression text keeps its old behaviour (infinity), so no existing game changes.

**Rejected on the way.** Operators as empty tags between operands (`<divide>window.width<by/>2</divide>`, and with a `<divided-by/>` between named operands): mixed content the schema cannot check, with operands still strings, or a separator that carries no information. One element accepting both operand spellings: harder to validate and to explain than two names. Output names per operation (`quotient=`, `difference=`): a word and a schema type for every operation; `name` already means "this thing's label". Dotted outputs written into an object (`object.middle`): they would collide with real names.

**Schema and validators.** A formula's operands hold operations whose operands are operations, so the schema has types that contain themselves. Xerces handles that; `xsd_lite` did not, and now follows a named type that refers to itself (and frees the cycle when the model goes). A group that contains itself is still refused there.

**Open.** More operations (`min`, `max`, `negate`, `abs`, `clamp`, `sign`, `pick`) are not built: add one when a game cannot be written without it, as for verbs ([06](06-motion-and-verbs.md)), until then the exprtk text has them. A `<random>` step in an `<equation>`. Moving the other games over, a game at a time. The reason for all of it, generating code for another target, now has its first reader: the windows-cpp generator turns an `<equation>` into a lambda and a `<formula>` into nested C++ ([below](#generating-a-program-with-xslt)). [14](14-vocabulary-map.md) lists every word the language has.

## Generating a program with XSLT

**What we looked for.** The format was chosen partly so a game could be transformed (above): a way to turn a game file into a program that plays it with none of the engine in it, as a test of whether the description really is the game, and as the start of other targets (another language, another machine).

**What we looked at.**

| Option | Notes |
|---|---|
| G1. C++ in xgecli walks the loaded `Game` and prints code | Reuses the loader, but the generator is engine code: every target is more C++ in the program |
| G2. XSLT on the game file | The format's own tool; a target is a folder of stylesheets, not a rebuild. Needs an XSLT processor |
| G3. A separate generator program | Cleaner split, one more program to build; can come later if xgecli outgrows it |

Processors: libxslt (XSLT 1.0 with EXSLT, C, in vcpkg and every Linux distribution), Xalan-C++ (XSLT 1.0, matches Xerces but barely maintained), Saxon (XSLT 3.0, Java or a C build with a different licence for the full version).

**Chosen: G2 with libxslt, run by `xgecli --generate <target>`.** A target is named platform-language (`windows-cpp`); the backends are assumed (SFML 3 for now; a later option can pick them, since naming every window, sound and XML combination in the target would multiply). `-g` stays the game; the long form only, plus `-o` for the folder. One stylesheet writes several files with `exsl:document` (XSLT 1.0 has one result; 2.0's `xsl:result-document` is the same idea): `main.cpp` is the real output, `CMakeLists.txt` and `README.md` are small fixed texts with the game's name in them. A stylesheet cannot copy a binary file, so its own result is a manifest of what it wrote and of the assets to copy, and xgecli copies them. Choices inside the stylesheets:
- *Static, not interpreted.* The program is the game's rules written out as statements (`deflect(o_ball, o_paddle1, edgeOfFirst, (45.0f));`), not the engine plus the XML as data. Collision pairs, which rules apply to which pair, edge rules and key bindings are worked out by the stylesheet.
- *The verbs' C++ is kept as text* in `runtime.xml`, one part per verb or feature, written only when the game uses the tag (`when=`). It mirrors `command_executor.cpp`, `game.cpp`, `collision_detector.cpp` (the swept tests), `sound.cpp` and `engine.cpp`, so generated Pong plays like the engine's.
- *Expression text is tokenised in XSLT 1.0* (no regular expressions): names become C++ variables (`window.width.center` → `v_window_width_center`), everything else is copied. That is enough because the expression syntax is C-like; it is also the weakest part, and the reason the tags exist.
- *Refuse, never guess.* Every tag the target does not know stops it with `windows-cpp cannot generate <x> yet` and where; xgecli removes a folder it made. A program that silently lacks a rule would be worse than none.

**Rejected.** XSLT 3.0 (Saxon): text processing would be easier, but the C library is the one that fits a C++ program with vcpkg dependencies. The engine as a library inside the generated program: that is the engine, not a generated game. One file per object or state: the C++ is shorter as one file and nobody edits it.

**Open.** Every other game: the first tag each needs is listed in [readme](../readme.md#known-limitations) (`<group>`, `<line>`, `<bitmap>`, `<grid>`, `<wrap>`, `<svg>`, then timers, looks and the rest). The runtime is a second copy of the verbs and can drift from the engine; a test that plays a generated game against the engine frame by frame would catch it, but needs a compiler in the test. Built and played only on Linux with GCC; Windows and Visual Studio not tried. Other targets (another backend, another language, an 8-bit machine) and how backends are named for them.

## The VGDL landscape (reference only)

- Academic VGDL (Ebner et al. 2013; Schaul's `py-vgdl`) is declarative and text-based for 2D arcade games: hierarchical sprite sets with inheritance, a level map with a legend, collision-only interaction rules, win/lose termination sets. No scripting. It became the base of the GVGAI competition (survey: arXiv 1802.10363) and was largely replaced in AI research by Griddly (arXiv 2011.06363, YAML GDY, C++ core). LLMs have since been used to write VGDL directly (arXiv 2404.08706).
- Cousins: GDL (logic-based, verbose, nothing built in; the opposite philosophy), RBG and Ludii (board games), PuzzleScript (find-and-replace rules on a grid; the closest in spirit). Entity-component systems are the nearest structural neighbor.
- Why the field stayed small: built for one job (benchmark games for AI), a closed vocabulary means a closed language (a new dynamic needs engine code), and each generation is renamed and re-implemented.
- "No loops or conditionals" is really a production-rule system: the engine's tick is the loop and collision matching is the conditional. The price is the same everywhere: the language stops where its vocabulary runs out. XMLGameEngine makes the same bet with exprtk carrying per-rule arithmetic.
- Only the arXiv pages for GVGAI, Griddly and the two GitHub repositories were confirmed to exist; other links from the research were never re-checked.
- Open research angle: how small can a vocabulary be and still describe a class of games.

## Open

- Where the vocabulary boundary sits: scrolling as camera, world or verb; gravity as a world setting or a per-object force (built per object, [06](06-motion-and-verbs.md)).
- Generating covers Pong only ([above](#generating-a-program-with-xslt)).
