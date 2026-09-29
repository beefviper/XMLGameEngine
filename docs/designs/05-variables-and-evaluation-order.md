# 05. Variables, references and evaluation order

**Status:** implemented for globals and per-object variables (two-pass, up-front symbol registration); lazy or dependency-ordered evaluation of chained references is not needed yet

## The problem

In `<position x="screenWidth / 2 - ball.radius" .../>` nothing in XML says whether `screenWidth` has been defined yet. XML has no scope and no definition order; a parser hands back strings. Whether a name is defined is purely the engine's own state.

## Decision: separate collecting definitions from evaluating expressions

The whole file is in memory before any expression is evaluated, so document order does not need to matter:

1. **Parse** builds the whole tree as raw strings.
2. **Collect symbols** in one pass: global variables, `window.*` values, and every object's own variables.
3. **Evaluate** each expression with all symbols already registered.

No regex and no "has this been declared yet" checks are needed at runtime. In the code (`game_expr.cpp`) globals and the `window.*` values are registered as exprtk constants, and every object variable is pre-registered under `ownerName.variableName` (initially 0) before any expression is compiled. This is why `score1` can read `paddle1.score` even though `paddle1` is declared later in the file. The real value is filled in when the object loop reaches that object.

## Dotted names

Exprtk has no member-access syntax, so `ball.radius` is simply a symbol whose name contains a dot. That works because each such name is registered explicitly. A more general rule (any `object.variable`) needs the up-front registration above.

## Options considered for chained references

If entity A's expression refers to B's variable and B's variable is itself an expression, there is a dependency chain.

| Option | Notes |
|---|---|
| Topological sort with cycle detection | Fixed order decided at load; errors on cycles. More machinery. |
| Lazy on first access, cache the result, error loudly on a cycle | Simpler; fine when most cross-references are static readouts (scores, positions). |
| Resolve names to references once at load, not each frame | Applies to either; avoids a string lookup on every access (see [07](07-object-variables-and-references.md)). |

The suggestion made in conversation was lazy evaluation with caching, which is enough for the games so far. Nothing beyond the simple cross-object read exists in the engine.

## Related caveat

Exprtk needs symbols registered before it compiles an expression. Name-to-object resolution must therefore run before expression strings are handed to it, not interleaved with parsing.

## Sources

- "XML variable references in game engine" (2026-09-25).
- "Stack vs heap allocation in C++" (2026-08-07): two-pass resolution of `paddle1.score`.
