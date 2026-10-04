# 14. Vocabulary map

**Status:** a reference, not a decision. It sorts every word the language knows today into grammatical categories and engine layers, and lists the categories that are still missing. Nothing here is new behavior; the gaps are **Idea**s that point at the file that discusses them.

## Why

The vocabulary grew one game at a time ([06](06-motion-and-verbs.md): a word is added only when a real game cannot be written without it), so it was never laid out as a whole. Laying it out shows where the language is rich (collision), where it is thin (values) and what kind of word each gap needs. It also frames the arithmetic question in [01](01-vision-and-format.md#arithmetic-in-text-and-as-tags): arithmetic was the one place structure hid in strings, and writing it as tags (option E4, now built beside the text) meant adding a new category of word, not just more verbs.

Read from the readme and `xgedef.xsd` of the `working` branch on 2026-10-04.

## The words today, by grammar

**Nouns: things that exist**

- Containers: `game`, `window`, `variables`, `sounds`, `objects`, `states`.
- Things: `object`, `group` and its `member`s, `grid` cells, `variable`, `state`, `sound`, a named `keys` set, `timer`.
- Sprite shapes: `circle`, `rectangle`, `text`, `image`, `line`, `bitmap`, `svg`. Several named `sprite`s on one object are its **looks** (`become`) or the frames of an `animation` (`frame`).
- Sound parts: `note`, `rest`.

**Verbs: commands**

| Family | Words |
|---|---|
| Motion | `move`, `hop`, `jump`, `accelerate`, `thrust`, `turn`, `reverse`, `stop` |
| Collision responses | `bounce`, `deflect`, `stick`, `wrap`, `carry` |
| Lifecycle | `die`, `reset`, `fire`, `release`, `reveal` |
| Appearance | `become` |
| Bookkeeping | `inc`, `dec` |
| Sound | `play` |
| Flow between screens | `push`, `pop`, `trigger` |

Most of these now work wherever they make sense (a collision, a condition, a key or a timer), not only where they were first needed ([08](08-timers-and-enemy-behavior.md)).

**Adjectives: properties of a noun**

- Look and place: `position`, `velocity`, `acceleration`, `heading`, `facing`, `drag`, `hidden`, `color`, `size`, `radius`, `width`, `height`, `thickness`, `scale`, `flip`, `hide` (parts of an SVG to leave out).
- Collision settings: `enabled`, `lockstep`, `type` (`box` or `pixel`).
- Sound: `wave` (`square`, `triangle`, `sawtooth`, `sine`, `noise`), `pitch`, `volume`.

**Prepositions and conjunctions: who, when, where**

- Targets: `edge`, `class`, `object`, `direction`, `button`, `keys`.
- Filters on a rule: `unless`, `slower`, `faster`, `sprite` (only while the object shows that look).
- Triggers in a condition: `atleast`, `atmost`, `remaining`.

**Pronouns: names that can be pointed at**

- Dotted references (`ball.radius`, `paddle1.score`), built-in names (`window.width.center`), generated names (`aliens.3.2`, `logrow3.2`), colors (`color.red`) and key names (`space`).

**Adverbs: how, how much, how long**

Time words arrived with timers and sound: `every` (again and again), `after` (once), `interval` (seconds a picture shows), `seconds` and `distance` (a `jump`), the seconds in a `note` and `rest`, and `to` (slide to a pitch). Amounts are still one-offs: `burn` on `accelerate` and `thrust`, the amount in `inc`, `dec`, `move`, `hop`, the count in `release` and `reveal`, the angle in `deflect`. There is no shared "how" vocabulary, so each verb invents its own; the likely shared members would be an amount, a duration and a limit that any verb could take.

**Values**

- Text expressions evaluated by exprtk, one value tag that draws a number, `<random min max>`, and two value tags that write arithmetic out as tags: `<equation>` (flat steps, operands as attributes, a step's `name` for the ones after it) and `<formula>` (one operation, operands as nested elements).
- Both are built from four operation words, each with a first and a second operand named for what they are: `add` (`augend`, `addend`), `subtract` (`minuend`, `subtrahend`), `multiply` (`multiplicand`, `multiplier`), `divide` (`dividend`, `divisor`). An operand is a name or a number, never an expression.
- exprtk's own functions work in the text form (Berserk uses `max` to keep a wait above a floor), but the format does not depend on them, and the tags have no `min`, `max` or the like yet.

A note on pictures: an `<image>` is a file loaded by whichever window backend is running, and so can be a JPEG or a PNG (`assets/paddle.jpg` is one; the loader code was not checked for the exact list). An `<svg>` is different: the engine draws it itself into its own bitmap (`svg.cpp`), so no backend loads one. A `<bitmap>` is the ASCII-rows form, and `<line>`s are the command-sequence form. Lines, bitmaps and SVGs share one path and can all be `pixel` collision shapes; an `<image>` cannot ([07](07-pictures-and-text.md)).

## The words today, by engine layer

| Layer | Vocabulary | How complete |
|---|---|---|
| Data model: what exists | objects, groups, grids, variables, states | solid |
| Appearance | seven shapes, `animation`, looks and `become`, `flip`, `heading` | solid; one color per bitmap, no palette |
| Kinematics: how things move | velocity, acceleration, heading, facing, drag, `move`, `hop`, `jump`, `thrust`, `turn`, `reverse` | good; no arcing jump with air control |
| Collision: what touches what, and the response | `edge`, `class`, `object`, `unless`, `sprite`, `slower`, `faster`, `type`, and a dozen response verbs | the largest layer: it is where verbs, adjectives and prepositions meet |
| Time | `timers` with `every` and `after`, animation `interval`, `jump` `seconds` | built; counts frames, shows no time left |
| Sound | `sounds`, `note`, `rest`, `wave`, `pitch`, `to`, `volume`, `play` | built; no looping music, no envelope |
| Input | `input`, `button`, `keys`, `action`, `trigger` | small and clean |
| Game flow | states, `push`, `pop`, conditions | enough for the shipped games; no `goto` |
| Values | expressions, `random`, `equation`, `formula` with four operations | thin; no `min`, `max`, `clamp`, `negate` as tags |

A collision rule reads like a sentence: *this object* (noun), *when it touches* (preposition) *that class* (noun), *unless* (conjunction) *it is on a log*, *do this* (verb). The "when does this apply" words (`class`, `object`, `edge`, `unless`, `sprite`, `slower`, `faster`) are where the language works hardest.

## Categories the language does not have

| Missing category | Words it would hold | Where it is discussed |
|---|---|---|
| **Math**, beyond the four operations | `min`, `max`, `negate`, `abs`, `clamp`, `floor`, `sign`, `pick`, an integer `random`, a `count` of objects; a `<random>` step in an `<equation>` | [01](01-vision-and-format.md#arithmetic-in-text-and-as-tags), [02](02-values-variables-and-names.md), [13](13-ideas.md) (balance helpers) |
| **Creation** | `create`, `destroy`. Nothing makes an object while a game runs; `fire`, `release` and `reveal` bring members of a hidden pool into play | [08](08-timers-and-enemy-behavior.md) |
| **Aiming and chasing** | `aim`, `chase`, `ai targeting=...`, paths and formations | [06](06-motion-and-verbs.md), [08](08-timers-and-enemy-behavior.md) |
| **Arcing jump** | grounded versus airborne, a jump impulse, air control | [06](06-motion-and-verbs.md) |
| **Queries** for conditions | `count`, `distance`, `touching`, `speed`, a timer's time left. Only `remaining` and a variable threshold exist | [04](04-states-conditions-and-input.md), [08](08-timers-and-enemy-behavior.md) |
| **Branching and events** | `if`, `and`, `or`, `goto state`, an event queue. A condition tests one threshold; this is deliberate, to keep control flow out | [01](01-vision-and-format.md), [04](04-states-conditions-and-input.md), [08](08-timers-and-enemy-behavior.md) |
| **Presentation** | a camera and scrolling, a palette for several colors in one bitmap, per-frame animation intervals, looping music | [13](13-ideas.md), [07](07-pictures-and-text.md), [09](09-sound.md) |

Time and sound were gaps in the first version of this map and are filled now; they are the pattern for how a gap closes: a game could not be written (Frostbite's cold, Kaboom's bomber, every silent game), the word was added in the places that already hold commands, and the workaround objects were deleted.

## The shape this suggests

Two grammars, one inside the other. The outer one is **noun / property / verb / filter**, and it is already regular: tags are checked by the schema and every word is visible to XSLT. The inner one is the **value**, which was all exprtk text and now has a vocabulary of its own: `random`, and four arithmetic words in two spellings, with the text still accepted beside them. Each further word (`min`, `max`, `pick`, `count`) is a tag in the XSD and one case in the evaluator ([01](01-vision-and-format.md#arithmetic-in-text-and-as-tags)).

The rule of thumb stands: a word is added only when a real game cannot be described without it, and a verb that means "behave like one particular game" shows that its parameters were not found ([06](06-motion-and-verbs.md)). Of the gaps above, `min` and `max` are the likeliest to pass that test next (a clamped speed, a random sign; Berserk already reaches for `max` in text), and aiming is the one the shipped games work around most.

## Keeping this current

When a verb, tag, attribute or value tag is added, add it to the matching list here as well as to [../readme.md](../readme.md) and the schema. When a gap in the last table is filled, move the word up and say which game needed it.
