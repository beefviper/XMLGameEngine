# Vocabulary map

**Status:** A reference, not a decision. Read from `xge/xgedef.xsd` and [../readme.md](../readme.md) on `working`, 2026-10-23.

## Why

The vocabulary grew one game at a time ([motion](22-motion.md): a word is added only when a real game cannot be written without it), so it was never laid out as a whole. Laying it out shows where the language is rich (collision, motion), where it is thin (values), and what kind of word each gap needs.

## The words today, by grammar

**Nouns: things that exist**

- Containers: `game`, `window`, `variables`, `sounds`, `paths`, `styles`, `objects`, `keeps`, `states`, `scenes`, `ways`, `timers`, `actions`, `inputs`, `conditions`, `shows`.
- Things: `object`, `group` and its `member`s or cells (`columns`, `rows`, `padding`, changed by `row`, `column`, `cell`), `map` (`tile`, `row`s of characters, pieces picked by `char=`), `variable`, `state`, `scene`, `way`, `sound`, `path`, `style`, `timer`, `action`, `keep`, `demo`, `computer`, `turns` and its `player`s.
- Sprite shapes: `circle`, `rectangle`, `text`, `image`, `line`, `bitmap`, `svg`, `tether`. Text pieces: `content`, `number`, `letter`, `choose` (`by`, `word`). Several named `sprite`s on one object are its **looks** (`become`) or the frames of an `animation` (`frame`); a named animation is a look that moves. `poses` (`standing`, `walking`, `skidding`, `rising`, `falling`) picks a look by how the object moves, `mirror` flips it for the other way. The `window` has a `scale`.
- Sound parts: `note`, `rest`, `part` (one voice of music), `loop`.
- Path parts: `step`, `start`, `home`, `speed`.
- Recorded play: `demo` with `seed`, `press`, `frames`.

**Verbs: commands**

| Family | Words |
|---|---|
| Moving | `move`, `hop` and `jump` (with `arc`), `leap`, `land`, `climb`, `accelerate`, `thrust`, `turn`, `reverse`, `stop`, `track` |
| Steering | `chase`, `aim`, `follow`; in a maze `go`, `hunt`, `wander` |
| Touching | `bounce`, `deflect`, `knock`, `stick`, `wrap`, `ride`, `carry`, `drop`, `hole` |
| In and out of play | `die`, `reset`, `fire`, `release`, `reveal`, `spawn`, `despawn`; `exists`, `most` on an object |
| Numbers | `inc`, `dec`, `set`, `copy`, `rank` |
| Look and sound | `become`, `play`, `silence` |
| Flow | `push`, `pop`, `enter`, `trigger`, `turns`, `pass`, `out`, `demo`, `hold` |

Most verbs work wherever they make sense (a collision, an action, a key, a condition, a timer, a path step), not only where they were first needed ([timers](09-timers.md)). In an action, timer or path step, a verb that is refused (a `fire` with no free projectile, a `hop` in the air) ends the list there.

**Adjectives: properties of a noun**

- Place and motion: `position`, `velocity`, `acceleration`, `heading`, `facing`, `drag`, `mass`, `pull`, `seek`, `altitude` (`start`, `rise`, `fall`, `bounce`), `shadow`, `maze` (`wall`), `centered`.
- Being in play: `hidden`, `drawn`, `light`, `dark` (`unlit` or `unseen`).
- Look: `color`, `size`, `radius`, `width`, `height`, `thickness`, `scale`, `flip`, `hide` (parts of an SVG left out), `background`, `style=`.
- Collision settings: `enabled`, `lockstep`, `type` (`box` or `pixel`), `solid` (which bitmap characters touch), `hitbox`.
- Sound: `wave` (`square`, `triangle`, `sawtooth`, `sine`, `noise`), `pitch`, `to`, `volume`, `music` (a state's own sound).

**Prepositions and conjunctions: who, when, where**

- Targets: `edge`, `class`, `object`, `variable`, `into`, `direction`, `button`, `axis`, `keys`, `sound`, `state`, `scene`, `way`, `path`, `from`.
- Filters on a collision rule: `side`, `unless`, `sprite` (only in that look), `slower`, `faster`, and `check`s.
- Tests in a condition or check: `atleast`, `atmost`, `above`, `below`, `equal` (each a number, or `object=`/`variable=` naming another variable), `remaining`; over many objects `total=` (`count`, `sum`, `highest`, `lowest`); `distance=` to a name or group; `all` and `any` to join them.
- Steering details: `ahead`, `with`, `retreat` (`hunt`), `near` (`chase`), `stagger` (`follow`), `rough` (`hole`).

**Pronouns: names that can be pointed at**

- Dotted references (`ball.radius`, `paddle1.score`), built-in names (`window.right`, `window.width.center`), generated names (`aliens.3.2`, `dots.17`), scene paths (`playing.room2.robots`), live numbers (`x`, `y`, `velocity.x`, `heading`, `altitude`, `light`, `blink`), `self.` (the object running a rule) and `touch.` (the touch a rule answers), colors (`color.red`) and key names (`space`, `mouse.left`, `pad1.a`).

**Adverbs: how, how much, how long**

Time words: `every` and `after` (timers), `interval` (an animation's pictures), `seconds` (a `jump` or a timed `hop`), the seconds in a `note` and `rest`. Amounts are still each verb's own: `burn` on `accelerate` and `thrust`, the amount in `inc`, `dec`, `move`, `hop`, the count in `release` and `reveal`, the angle in `deflect`, the strength of `pull` and the turn of `seek`. There is no shared "how" vocabulary; the likely members would be an amount, a duration and a limit any verb could take.

**Values**

- Text expressions evaluated by exprtk; one value tag that draws a number, `<random min max>`; and arithmetic as tags, `<equation>` (flat steps, operands as attributes, a step's `name` for the ones after it) and `<formula>` (one operation, operands as nested elements).
- Both arithmetic forms use four operation words, each with operands named for what they are: `add` (`augend`, `addend`), `subtract` (`minuend`, `subtrahend`), `multiply` (`multiplicand`, `multiplier`), `divide` (`dividend`, `divisor`). An operand is a name or a number, never an expression.
- A command's values are read when it runs, so they can name live numbers; every other value is worked out once at load ([values-and-names](04-values-and-names.md)).
- exprtk's own functions work in the text form (Berserk uses `max` to keep a wait above a floor), but the format does not depend on them, and the tags have no `min`, `max` or the like.

A note on pictures: an `<image>` is a file loaded by the window backend; an `<svg>` is drawn by the engine itself (`picture/svg.cpp`). Lines, bitmaps and SVGs share one bitmap path and can be `pixel` collision shapes; an `<image>` cannot ([pictures](30-pictures.md)).

## The words today, by engine layer

| Layer | Vocabulary | How complete |
|---|---|---|
| Data model: what exists | objects, groups, maps, variables, states (`from=`), scenes (kit, `from=`, ways), keeps | solid |
| Appearance | eight shapes, multi-color bitmaps, animation, looks (moving ones too) and `become`, `poses`, `mirror`, blinking, the window's `scale`, `flip`, `heading`, `light`/`dark`, `shadow`, `hole` | solid; no recolouring a look |
| Kinematics | velocity, acceleration, heading, facing, drag, mass, pull, seek, altitude, grip, a hop's or jump's arc, and the moving verbs | good; `leap` has no air control |
| Steering | `chase`, `aim`, paths with `follow`, maze walking with `go`, `hunt`, `wander`, `hunt` by hops | good; no search of a maze, straight path legs only |
| Collision | edges, `class`, `object`, `side`, `unless`, `sprite`, `slower`, `faster`, checks, `type`, `solid`, `hitbox`, and the touching verbs | the largest layer: where verbs, adjectives and prepositions meet |
| Time | timers with `every` and `after` (on objects, states, scenes, looks), animation `interval`, `seconds` | built; counts frames, no time left shown |
| Sound | `sounds`, `note`, `rest`, `part`, `wave`, `pitch`, `to`, `volume`, `loop`, `play`, `silence`, `music` | built; no envelope, no stereo |
| Input | `input` (`button`, `axis`), `keys` sets, `trigger`, `track`, mouse and pads, `computer` with `hold` | small and clean |
| Game flow | states, `push`, `pop`, scenes with `enter` and `reset scene`/`way`, `show state`, conditions with `all`/`any`, `turns`, `demo`, `keeps`, `rank` | enough for the shipped games |
| Values | expressions, `random`, `equation`, `formula` with four operations, live numbers in commands | thin; no `min`, `max`, `clamp`, `negate` as tags |

A collision rule reads like a sentence: *this object* (noun), *when it touches* (preposition) *that class* (noun), *on its top* (`side`), *unless* (conjunction) *it is on a log*, *do this* (verb).

## Categories the language does not have

| Missing category | Words it would hold | Where it is discussed |
|---|---|---|
| **Math** beyond the four operations | `min`, `max`, `negate`, `abs`, `clamp`, `floor`, `sign`, `pick`, an integer `random`; a `<random>` step in an `<equation>` | [arithmetic](03-arithmetic.md), [values-and-names](04-values-and-names.md) |
| **Planning** | searching a maze, leading a target, curved paths, paths that aim | [chase-and-paths](24-chase-and-paths.md) |
| **Queries** | a timer's time left, what is touching what, line of sight; a condition's threshold that follows a variable | [states-and-conditions](07-states-and-conditions.md), [timers](09-timers.md) |
| **Branching and events** | `if`, `goto`, an event queue. Conditions join tests with `all`/`any` but have no else; keeping control flow out is deliberate | [vision](01-vision.md), [timers](09-timers.md) |
| **Presentation** | a camera and scrolling (version 2), recolouring a look, per-frame animation intervals, decals | [ideas](57-ideas.md), [pictures](30-pictures.md) |

Time, sound, music, conditions over many objects, maze walking and making objects while a game runs (`spawn`/`despawn`, 2026-10-23) were gaps in earlier versions of this map and are filled now. They show how a gap closes: a game could not be written (Frostbite's cold, Pac-Man's ghosts, Adventure's sleeping dragon), the word was added in the places that already hold commands, and the workaround objects were deleted.

## The shape this suggests

Two grammars, one inside the other. The outer one is **noun / property / verb / filter**, and it is regular: tags are checked by the schema and every word is visible to XSLT. The inner one is the **value**, which has a small vocabulary of its own (`random` and four arithmetic words in two spellings) beside exprtk text. Each further word (`min`, `max`, `pick`) is a tag in the XSD and one case in the evaluator.

A word is added only when a real game cannot be described without it, and a verb that means "behave like one particular game" shows that its parameters were not found ([motion](22-motion.md)). Of the gaps above, `min` and `max` are the likeliest to pass that test next (Berserk already reaches for `max` in text).

## Keeping this current

When a verb, tag, attribute or value tag is added, add it here as well as to [../readme.md](../readme.md), the root readme's vocabulary tables and the schema. When a gap in the last table is filled, move the word up and say which game needed it.
