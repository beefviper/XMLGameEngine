# Arithmetic in text and as tags

**Status:** Built: text arithmetic of `+ - * /` (`load/calc`), and `<formula>` tags for everything more. Reference: [arithmetic as tags](../reference/format.md#arithmetic-as-tags).

## What we looked for

Arithmetic (`window.width.center - ball.radius`) was the last thing hidden from XSD and XSLT: a string a tool cannot see into. Wanted: a form a schema can check and a transformation can walk, that does not cost every game its readability, and that needs no second tool.

| Option | Verdict |
|---|---|
| E1. Paste the string through | Fine for C++ targets, not assembly |
| E2. XSLT parses it | Awkward precedence parsing |
| E3. Parse in C++ to an AST as XML for XSLT | Adds a step |
| **E4. Computation as elements** | Built as `<formula>`, beside plain text for sums |

## `<formula>`

- **One value tag, nested:** one operation whose operands hold a name, a number, a `<random>` or another operation. Precedence is the nesting; a chain (`a - b - c`) is one operation with several second operands.
- **Operands named for what they are:** `augend`/`addend`, `minuend`/`subtrahend`, `multiplicand`/`multiplier`, `dividend`/`divisor`. Operations with no classic names take `<value>`s. One table, `operationShape` in `game/command.cpp`, drives loader, evaluator and printer.
- **An operand is a name or a number, never an expression,** or `<dividend>a + b</dividend>` brings the hidden string back.
- **A divisor of 0** is the engine's to catch, since XSD cannot see what a name holds. Certainly 0 at load stops the load; 0 only because an input is not final yet (a size not yet measured) gives 0 and is worked out again; while running it gives 0 and one warning. A `late` flag tells these apart.
- **One form, not two.** A flat `<equation>` of steps (three-address code) was built beside it, then dropped: two structured spellings of one thing are one too many, and its operands as attributes broke the format's rule.
- **`<smallest>`, `<largest>`, `<absolute>`, `<clamp>`;** clamp reuses the condition words (`<atleast>`, `<atmost>`). Rejected: `<min>`/`<max>`, which already mean a `<random>`'s ends.
- **`<compare>`** gives 1 or 0: `<compare><value>mode.waves</value><atleast>4</atleast></compare>`, the condition words again, so there is one comparing vocabulary and the test says which side is which. Rejected: five two-operand comparisons (which side?), a test as an empty tag between bare operands (mixed content the schema cannot check), an `<if>` (multiplying by 1 or 0 says it).
- Also `<sign>`, `<ceiling>`, `<floor>`, `<round>`, `<negate>`, `<sine>` and `<cosine>` in degrees (so no `pi`), `<remainder>` from 0 up to the divisor so a count wraps.
- Rejected on the way: operators as empty tags (`<divide>a<by/>b</divide>`), output names per operation, dotted outputs written into an object.

## The text cut down

- The text was once worked out by exprtk, so it had functions (`max(...)`, `cos(... * pi / 180)`) and comparisons: a second, hidden maths vocabulary beside the tags.
- **Chosen: one small library of our own,** `load/calc`: numbers, names, `+ - * /`, a leading sign, brackets, nothing else. It compiles a text once into steps that read each name's number where it lives, and works left to right exactly as written. A function call stops the load and says to write a `<formula>`.
- **A text divides by 0 as a `<divide>` does:** 0, stopping the load when the divisor is certainly 0, and a value that comes out as infinity at load stops it too. Before, text gave infinity and an object went nowhere.
- **Open, the author's call: no arithmetic in text at all.** A text would hold one number or one name, every calculation tags, with `<formula>` made short for the common shapes. Plan task R12 surveys the shapes first.
- Rejected: an expression backend behind a factory with exprtk as an option. Nothing a game writes would differ between them, and a game would fail on one the first time someone used a function.

## Schema

A formula's operands hold operations, so the schema's types contain themselves. Xerces handles that; `load/xsd` was taught to follow a named type that refers to itself. The operations are listed twice (formula and operand), since a group containing itself is still refused there.

## Open

`tangent`, square root, power and `pick`: added when a game needs one. A schedule (a list of lengths, one per wave) would say Pac-Man's timings plainly.
