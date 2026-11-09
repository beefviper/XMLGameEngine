# Vocabulary map

**Status:** A map, not a decision. Every word is listed in the [docs index](../readme.md#word-index) and the [reference](../reference/); this page is how the words fit together, and which kinds of word are missing.

## Why

The vocabulary grew one game at a time (a word is added only when a real game cannot be written without it), so it was never laid out as a whole. Laid out, it shows where the language is rich (collision, motion), where it is thin (values), and what kind of word each gap needs.

## The words as grammar

| Part of speech | What it is | Examples |
|---|---|---|
| Nouns | things that exist | `object`, `group`, `map`, `state`, `scene`, `sound`, `path`, `timer` |
| Verbs | commands | `move`, `bounce`, `fire`, `inc`, `become`, `push` ([commands](../reference/commands.md)) |
| Adjectives | properties of a noun | `velocity`, `heading`, `drag`, `mass`, `grip`, `light`, `type`, `hitbox` |
| Prepositions | who, when, where | `edge=`, `class=`, `object=`, `side=`, `unless=`, `from=`, `into=` |
| Pronouns | names that point | `ball.radius`, `window.right`, `aliens.3.2`, `self.`, `touch.`, `energy.level` |
| Adverbs | how, how much, how long | `every`, `after`, `seconds`, `burn`, a verb's amount |
| Values | numbers | text arithmetic (`+ - * /`), `<random>`, `<formula>` tags |

A collision rule reads like a sentence: *this object*, *when it touches* *that class*, *on its top* (`side`), *unless* *it is on a log*, *do this*.

Adverbs are each verb's own: there is no shared "how" vocabulary. Its likely members would be an amount, a duration and a limit any verb could take.

## The words by engine layer

| Layer | How complete |
|---|---|
| Data model: objects, groups, maps, cells, the kit, states and scenes, keeps | solid |
| Appearance: shapes, bitmaps with inks, SVG, animation, looks, poses, turning, light, shadows, holes | solid |
| Kinematics: velocity, acceleration, heading, drag, mass, pull, seek, altitude, grip, swing | good; no air control in a `<leap>` |
| Steering: chase, aim, paths, maze walking | good; no search of a maze, straight legs only |
| Collision: edges, filters, conditions, pixel tests, the touching verbs | the largest layer, where verbs, adjectives and prepositions meet |
| Time: timers, intervals, seconds | built; counts frames |
| Sound: notes, parts, waves, music | built; no envelope, no stereo |
| Input: keys, axes, the three moments, computer players | small and clean |
| Flow: the stack, scenes, conditions, turns, demos | enough for the shipped games |
| Boards and cards: moves, picker, think, packs, holders, patterns | built for the shipped games |
| Values: text arithmetic, `<random>`, `<formula>` | thinnest: no power, root, tangent or "if" |

## Kinds of word the language does not have

| Missing | Words it would hold | Where |
|---|---|---|
| More maths | power, square root, tangent, an integer `random` | [arithmetic](03-arithmetic.md) |
| Planning | searching a maze, leading a target, curved paths | [chase-and-paths](24-chase-and-paths.md) |
| Queries | line of sight; a total or distance inside a value | [states-and-conditions](07-states-and-conditions.md) |
| Branching and events | `if`, `goto`, an event queue. Kept out on purpose: conditions join tests but have no else | [vision](01-vision.md) |
| Presentation | a camera and scrolling (version 2), per-frame intervals, decals | [ideas](57-ideas.md) |

Time, sound, conditions over many objects, maze walking, spawning, `negate`, `floor` and `sign` were all gaps once. Each closed the same way: a game could not be written, the word was added where commands already go, and the workaround objects were deleted.

## The shape this suggests

Two grammars, one inside the other. The outer is **noun, property, verb, filter**: regular, checked by the schema, visible to XSLT. The inner is the **value**, with a small vocabulary of its own beside plain `+ - * /` text. Each further value word is a tag in the XSD, a row in `operationShape` and one case in the evaluator.

A verb that means "behave like one particular game" shows that its parameters were not found.
