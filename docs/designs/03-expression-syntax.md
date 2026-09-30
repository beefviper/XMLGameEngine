# 03. Expression syntax: function calls in attributes vs XML structure

**Status:** option D **built**: function syntax is gone from every game file, the XSD and the loader; plain arithmetic stays as text

## The problem

Attributes such as `src="shape.circle(ball.radius,'color.white')"`, `action="bounce()"` and `action="inc('paddle2.score');reset()"` are readable, but they are a small scripting language living inside XML strings. Exprtk made adding functions easy, and the vocabulary grew faster than the design. The result is very readable but reads as a custom scripting language embedded in XML, which is weaker engineering.

Consequences:

- **XSD cannot validate the contents.** It can check that `src` is a string, not that the string is a well-formed call with the right arguments. A typo is a runtime error, not a schema error.
- **XSLT sees opaque strings.** A transformation cannot reason about a call it cannot see.
- **Sequencing hides in delimiters.** The `;` in `inc('paddle2.score');reset()` encodes "do this, then that" as text. Tree structure (document order) is what a toolchain can see.

## Options considered

| Option | Example | Notes |
|---|---|---|
| **A. Function-call strings** (current) | `<sprite src="shape.circle(ball.radius,'color.white')"/>` | Compact and readable; opaque to XSD and XSLT. |
| B. One element per call | `<sprite><shape.circle radius="ball.radius" color="color.white"/></sprite>` and `<collision edge="vertical"><action.bounce/></collision>` | XSD can validate arguments; XSLT can transform; more verbose. |
| C. Structure for the parts that are really structure | `<velocity><random min="-7" max="7" axis="x"/><random min="-3" max="3" axis="y"/></velocity>`; several actions as ordered child elements | Same benefits as B, applied where a string was standing in for a tree. |
| D. Functions become elements; plain arithmetic stays as text | Keep `window.width.center - ball.radius`, drop `shape.circle(...)` | Removes most hidden structure; arithmetic remains ([04](04-arithmetic-and-xslt-codegen.md)). |

## Decision and status

The author asked for the function-like syntax to be removed (conversation of 2026-09-25) and not left half-done. It was removed (2026-09-30), all at once: the five sample games, the XSD, the loader and the tests.

**Rule of thumb.** An attribute names or picks something: `name`, `class`, `object`, `edge`, `button`, `state`, `variable`, `direction`, `unless`. Everything else is element content. A *value* is either expression text (`window.width.center - ball.radius`, plain arithmetic, still exprtk) or exactly one value tag. A command list is an ordered list of command tags.

| Was | Now |
|---|---|
| `x="random.range(-7,7)"` | `<x><random min="-7" max="7" /></x>` |
| `src="shape.circle(r,'color.white')"` | `<sprite><circle><radius>r</radius><color>white</color></circle></sprite>` |
| `action="inc('p.score');reset()"` | `<inc variable="p.score" /><reset />` |
| `action="move.up(step)"` | `<move direction="up">step</move>` |
| `action="state('x')"` / `state()` | `<push state="x" />` / `<pop />` |
| `value="15"` on a condition | `<atleast>15</atleast>` (or `<atmost>`, `<remaining>`) |
| `basic="basic"` | a `<collision>` with no selector |

Anywhere a value is allowed, a value tag is allowed. Only `<random min max>` exists today (drawn once at load, as `random.range` was). Ideas for others: `<pick>` (one of a list), an integer `<random>`, `<clamp>`, `<count>` of objects in a class. Each is a new tag in the XSD and one case in the evaluator; none is built.

## Worked example from the conversations

`pong.xml` was reviewed for anything that needs more than a symbol-table lookup to interpret:

- **Not a problem:** numeric literals, `edge="vertical"`, `enabled="true"`, `class="paddle"`, and color names such as `color.black`. Each is a flat string that maps to a constant by table lookup.
- **A problem:** the arithmetic in positions, and the multi-step action strings (`inc(...);reset()`), which were the same kind of problem (structure flattened into text).
- Fix for the second (now built):

```xml
<collision edge="left">
  <inc variable="paddle2.score" />
  <reset />
</collision>
```

- The attribute-per-line AI examples in [13](13-verb-vocabulary.md) follow the same style.

## Parts that were kept on purpose

Objects have named actions (`<action name="up"><move direction="up">step</move></action>`) and states bind keys to those names. That indirection is good design regardless of how the call is written ([10](10-input-and-actions.md)).

## Second batch: alternatives

- **Statements as a command list.** Several actions in one rule (bounce, then move down) were accepted because the list is sequential, deterministic and has no control flow: a command list, not a script. The condition for staying safe is to keep out conditionals, loops, variables and dynamic dispatch.
- **Responses as elements.** Two shapes for the reaction to a collision: one element with named effects, or a list of response elements each with a command and a target. The first reads more like a game description; the second is more regular for a schema ([27](27-object-composition-and-shape.md)).
- **Named parts of an expression.** A window size expressed as a dotted name, and an object's own size in expressions, were kept as readable text; sign and magnitude for random speeds (see [13](13-verb-vocabulary.md)) can be value tags.
- **A helper set for shaping values.** Clamp, saturate, cap, floor, diminishing returns and multiply-by-a-comparison are the common tricks for balancing without branches; each is a candidate value tag if the arithmetic stays text ([28](28-game-ideas-and-test-games.md)).

## Sources

- "Engine design conversation A (a long general chat; only the project segment was used)" (2026-05-23): first statement of the concern.
- "XML variable references in game engine" (2026-09-25): the `pong.xml` review and the sequencing point.
- Second batch: "Game Engine Command Queue" (2026-01-17), "Declarative Pong XML redesign" (2026-09-23), "Game Mechanics Tricks" (2026-07-06).
