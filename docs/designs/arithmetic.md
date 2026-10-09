# Arithmetic in text and as tags

**Status:** Built.

## Arithmetic in text and as tags

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

**Open.** More operations (`min`, `max`, `negate`, `abs`, `clamp`, `sign`, `pick`) are not built: add one when a game cannot be written without it, as for verbs ([motion](motion.md)), until then the exprtk text has them. A `<random>` step in an `<equation>`. Moving the other games over, a game at a time. The reason for all of it, generating code for another target, now has its first reader: the windows-cpp generator writes an `<equation>` or a `<formula>` back out as ordinary C++ arithmetic ([below](generator.md#generating-a-program-with-xslt)). [vocabulary-map](vocabulary-map.md) lists every word the language has.
