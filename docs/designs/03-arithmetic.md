# Arithmetic as tags

**Status:** Built: every calculation is operation tags (`<add>`, `<clamp>`, ...); a value's text is one number or one name (`load/calc`), and sides (`<left>`, `<right>`, `<top>`, `<bottom>`) cover the window edges. Reference: [arithmetic as tags](../reference/format.md#arithmetic-as-tags).

## What we looked for

Arithmetic (`window.width.center - ball.radius`) was the last thing hidden from XSD and XSLT: a string a tool cannot see into. Wanted: a form a schema can check and a transformation can walk, that does not cost every game its readability, and that needs no second tool.

| Option | Verdict |
|---|---|
| E1. Paste the string through | Fine for C++ targets, not assembly |
| E2. XSLT parses it | Awkward precedence parsing |
| E3. Parse in C++ to an AST as XML for XSLT | Adds a step |
| **E4. Computation as elements** | Built: operations written straight in a value |

## Operations

- **One value tag, nested:** an operation written straight in a value, its operands `<value>`s holding a name, a number, a `<random>` or another operation. Precedence is the nesting; a chain (`a - b - c`) is one operation of three values.
- **Chosen by the author, after the survey below: no wrapper and no operand names** (`<x><subtract><value>window.right</value><value>4</value></subtract></x>`). It was a `<formula>` wrapper with operands named for what they are (`augend`/`addend`, `minuend`/`subtrahend`, `multiplicand`/`multiplier`, `dividend`/`divisor`): the order already says it, so they were words to learn. Kept: nesting is precedence, which the author wants for turning a game into another language later. Rejected: a start and steps read in order (`<value>cell</value><times>8</times><plus>16</plus>`), shorter but with no precedence. One table, `operationShape` in `game/command.cpp`, drives loader, evaluator and printer.
- **An operand is a name or a number, never an expression,** or `<value>a + b</value>` brings the hidden string back.
- **A divisor of 0** is the engine's to catch, since XSD cannot see what a name holds. Certainly 0 at load stops the load; 0 only because an input is not final yet (a size not yet measured) gives 0 and is worked out again; while running it gives 0 and one warning. A `late` flag tells these apart.
- **One form, not two.** A flat `<equation>` of steps (three-address code) was built beside it, then dropped: two structured spellings of one thing are one too many, and its operands as attributes broke the format's rule.
- **`<smallest>`, `<largest>`, `<absolute>`, `<clamp>`;** clamp reuses the condition words (`<atleast>`, `<atmost>`). Rejected: `<min>`/`<max>`, which already mean a `<random>`'s ends.
- **`<compare>`** gives 1 or 0: `<compare><value>mode.waves</value><atleast>4</atleast></compare>`, the condition words again, so there is one comparing vocabulary and the test says which side is which. Rejected: five two-operand comparisons (which side?), a test as an empty tag between bare operands (mixed content the schema cannot check), an `<if>` (multiplying by 1 or 0 says it).
- Also `<sign>`, `<ceiling>`, `<floor>`, `<round>`, `<negate>`, `<sine>` and `<cosine>` in degrees (so no `pi`), `<remainder>` from 0 up to the divisor so a count wraps.
- Rejected on the way: operators as empty tags (`<divide>a<by/>b</divide>`), output names per operation, dotted outputs written into an object.

## The text cut down

- The text was once worked out by exprtk, so it had functions (`max(...)`, `cos(... * pi / 180)`) and comparisons: a second, hidden maths vocabulary beside the tags.
- **Then one small library of our own,** `load/calc`, of `+ - * /` and brackets. A function call stopped the load and said to write an operation.
- **Chosen by the author (plan D8): no arithmetic in text at all.** `load/calc` now reads one number (a sign allowed) or one name, bound once to where the number lives. Arithmetic in text stops the load and shows the operation to write (for two terms, the exact tags). A value that comes out as infinity at load stops it too.
- **Sides for the window edges:** `<left>`, `<right>` (in an `<x>`), `<top>`, `<bottom>` (in a `<y>`) put the object's own side on the window's, an optional `<offset>` measured in from the side, so `window.right - paddle.width - 12` is `<right><offset>12</offset></right>`. `<centered>` may hold an operation, a middle placed by a calculation.
- **Long operations are laid out a `<value>` per line** in the games; short ones stay on one line.
- Rejected: an expression backend behind a factory with exprtk as an option. Nothing a game writes would differ between them, and a game would fail on one the first time someone used a function.

## The survey for no arithmetic in text

Plan task R12 counted every game text that calculates: 900, of which 627 are an `<x>` or `<y>`, 109 a size and 93 a number for `<set>`, `<inc>` or `<dec>`. By shape: a name plus or minus a number 382; names added or taken away 157; a name times a number (and a number) 143; a name and a name times a number 44; a number less a name 38; with a division 94; longer 42. 199 start from a `window.` name, and 100 are card layouts on `cardwidth` or `cardheight`. So three in four are one name changed once or twice. All 900 were converted to operations and sides, with every start number unchanged.

## Schema

An operation's operands hold operations, so the schema's types contain themselves. Xerces handles that; `load/xsd` was taught to follow a named type that refers to itself. The operations are listed twice (in a value and in an operand), since a group containing itself is still refused there.

## Open

`tangent`, square root, power and `pick`: added when a game needs one. A schedule (a list of lengths, one per wave) would say Pac-Man's timings plainly.
