# Arithmetic in text and as tags

**Status:** Built.

## Arithmetic in text and as tags

Arithmetic (`window.width.center - ball.radius`) was the last thing hidden from XSD and XSLT: a string a tool cannot see into. What we looked for: a way to write it that a schema can check and a transformation can walk, that does not cost every game its readability, and that does not need a second tool in the pipeline.

| Option | Notes |
|---|---|
| E1. Paste the string through | Works for C++ targets (infix is valid C++), not assembly |
| E2. XSLT parses it | Possible with `analyze-string`, awkward precedence parsing |
| E3. Parse once in C++ into AST-as-XML, XSLT walks it | Keeps the fragile part testable; adds a step |
| E4. Remove arithmetic: computation as elements | Fully toolable, verbose, needs a math vocabulary. **Built as `<formula>`, beside the text** |

**What was built (E4).** One value tag, a sibling of `<random>`, so it goes anywhere a number does, and the exprtk text stays valid everywhere (a game is not forced to change: every other game still uses text, which stays the everyday form):

- **`<formula>`**, nested: one operation whose operands are elements that hold a name, a number, a `<random>` or another operation. Precedence is the nesting, and a chain (`a - b - c`) is one operation with several second operands. Closest to a C++ expression tree. Pong's and Breakout's titles use it.

It uses four operations named for their operands (`augend`/`addend`, `minuend`/`subtrahend`, `multiplicand`/`multiplier`, `dividend`/`divisor`); one table, `operationShape` in `game/command.cpp`, drives the loader, the evaluator and the printer.

**One form, not two.** A flat `<equation>` (steps with operands as attributes, each able to `name` its answer for the later ones: three-address code) was built beside `<formula>` at first, on the view that the two were different enough in taste (`printf` against `cout`) to keep both. The grammar review of 2026-10 dropped it: two structured spellings of one thing are one too many, its operands as attributes went against the format's rule (an attribute names or picks something), it could not hold a `<random>`, and only Pong's title used it. The nested form stayed, since it can hold a `<random>` and maps straight onto an expression tree. At the same time `<random min= max= />` became `<random><min>..</min><max>..</max></random>`, its ends element content like every other number.

**Decisions inside it.**

- *An operand is a name or a number, never an expression.* Otherwise `<dividend>a + b</dividend>` brings the hidden string back. It also keeps the format's rule (an attribute names or picks something) honest for the attribute operands: a name or a plain number is that, and anything computed is an operation. The price is verbosity: roughly 380 expressions in the games have two or three operators each, so converting all of them would give roughly a thousand operation lines (a rough count).
- *Evaluated at load* like every value, in the written order; a size-dependent position is finished once the window has measured, the same as for text.
- *A divisor of 0 is the engine's to catch, not the schema's:* XSD cannot see what a name holds, and could at best refuse the literal `0` (and not `0.0`, or a name that is 0). The engine refuses a divisor that is certainly 0 at load and names the `<divide>`. It cannot refuse one that is 0 only because its input is not final yet (another object's variable reads 0 until that object is built; a text's width until a window has measured it), so that gives 0 silently and is worked out again later where it can be. While the game is running a throw would end it, so a divisor of 0 gives 0 and one warning for each place. To tell these cases apart each answer carries a `late` flag, set by an operand that is an object variable or an object's size and passed on to everything made from it. The expression text keeps its old behaviour (infinity), so no existing game changes.

**Rejected on the way.** Operators as empty tags between operands (`<divide>window.width<by/>2</divide>`, and with a `<divided-by/>` between named operands): mixed content the schema cannot check, with operands still strings, or a separator that carries no information. One element accepting both operand spellings: harder to validate and to explain than two names. Output names per operation (`quotient=`, `difference=`): a word and a schema type for every operation. Dotted outputs written into an object (`object.middle`): they would collide with real names. Flat `<equation>` steps: built, then dropped (above).

**Schema and validators.** A formula's operands hold operations whose operands are operations, so the schema has types that contain themselves. Xerces handles that; `load/xsd` did not, and now follows a named type that refers to itself (and frees the cycle when the model goes). A group that contains itself is still refused there.

**Smallest, largest, absolute, clamp (2026-10-28).** Four more operations, added together as planned. The four arithmetic words have classic names for their operands; these have none, so their operands are `<of>` (`<smallest><of>a</of><of>b</of></smallest>`, two or more; `<absolute>` exactly one), and `<clamp>` reuses the condition words: `<clamp><of>speed</of><atleast>1</atleast><atmost>5</atmost></clamp>`. Rejected: `<min>`/`<max>` as tags, which already mean a `<random>`'s ends and would read `<random><min><formula><min>`. `operationShape` gained how many operands a tag takes and a different role for the last (clamp's `<atmost>`), and how it prints (`min(a, b)`, `clamp(...)`). At the same time a `<random>`'s `<min>` and `<max>` may each be a `<formula>`: the two ends are then kept as one operation, `random`, of `min` and `max` (`RawValue::operations`), so renaming, scenes and the generator walk them like any formula's operands.

**Open.** More operations (`negate`, `floor`, `sign`, `pick`) are not built: add one when a game needs it; until then the exprtk text has them. Moving the other games over, a game at a time, if wanted. The reason for all of it, generating code for another target: the first windows-cpp generator wrote a `<formula>` back out as ordinary C++ arithmetic; the current one writes it as the engine's data (`formula(...)`) and lets the engine work them out ([generator](40-generator.md)). [vocabulary-map](11-vocabulary-map.md) lists every word the language has.
