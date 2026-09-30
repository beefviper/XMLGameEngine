# 02. File format: why XML

**Status:** implemented

## Decision

A game is a single XML file, validated against `assets/xmlgameengine.xsd`.

## Why XML

The author picked XML on purpose, for three reasons:

1. **Familiarity.** Many people know HTML, so a game file does not look like a foreign programming language. Someone who has never programmed but has looked at HTML can read `pong.xml` and roughly follow it. The `name` and `class` attributes are borrowed from HTML `id` and `class` on purpose ([06](06-names-and-classes.md)).
2. **Validation.** XSD can check structure, required attributes and types before the game runs.
3. **Transformation.** XSLT could turn a game file into documentation, another format, or even native source code for a platform ([04](04-arithmetic-and-xslt-codegen.md)).

Reasons 2 and 3 are the ones that the expression syntax currently undermines ([03](03-expression-syntax.md)).

## Options considered

| Option | Notes |
|---|---|
| **XML + XSD** | Chosen. Familiar, validatable, transformable. Verbose. |
| Text VGDL (academic style) | The original VGDL uses compact text files: a game description (SpriteSet, LevelMapping, InteractionSet, TerminationSet) plus an ASCII-art level file. Very compact, unfamiliar, no schema tooling. |
| YAML | What Griddly's GDY uses. Compact and readable; weaker validation story. |
| JSON | Easy to parse everywhere; awkward for comments and for humans to author. |

## One file or two

Academic VGDL splits rules and level layout into two files. XMLGameEngine keeps everything in one XML file, and lays out grids of objects with the `grid()` sprite function instead of an ASCII map. Whether a separate level file will ever be needed for larger games is open.

## Current schema notes

- The schema fixes the order of children (`window`, `variables`, `objects`, `states`; inside an object `sprite`, `position`, `velocity`, `collisions`, then optional `actions` and `variables`). The engine's own element lookup is by name, not position, so it does not depend on this order.
- `variable` values are declared `xs:integer`, and a state `condition` `value` is `xs:unsignedByte` (0 to 255). Both are limits of the schema, not of the engine.
- A game file with no `xsi:noNamespaceSchemaLocation` skips validation. See [15](15-backend-abstraction.md) for how strong (Xerces) and weak (built-in) validation differ.

## Second batch: alternatives

- **What stays an attribute.** The suggestion that held up: only `name` is an attribute, because it is identity, while width, height, color and the like are configuration and go in child elements (already the direction in [03](03-expression-syntax.md)). Alternatives: grouping related properties under a sub-element (size, background, display), or making name an element too.
- **Every object has a unique name.** Other parts of the file act on objects by name, so a required, unique name is the anchor ([06](06-names-and-classes.md)).
- **Structure versus behavior in strings.** A fork was laid out: either a mini scripting language inside attribute values, or a structured vocabulary of mechanics. Recommended path: freeze the string form as version one semantics and lift the highest-value concepts into structure one at a time. The first candidates were collision responses, score and counters, and reset or spawn.

## Sources

- "Engine design conversation A (a long general chat; only the project segment was used)" (2026-05-23), the discussion of why XML.
- "Video game description languages: overview and research" (2026-09-22).
- Second batch: "Name Attribute in XML" (2026-07-22), "Pong Mechanics Vocabulary" (2026-01-28).

The schema also checks the shape of every game: attributes are names and picks, values and commands are elements, and `<condition>` must hold exactly one of `<atleast>`, `<atmost>`, `<remaining>`. Expression text inside a value is still a plain string to the schema.
