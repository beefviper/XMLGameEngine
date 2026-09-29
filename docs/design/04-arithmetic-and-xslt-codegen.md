# 04. Arithmetic in XML and XSLT code generation

**Status:** open

## The question

If a complete XML toolchain is going to be used on game files, is arithmetic in attribute strings (`window.width.center - ball.radius`) the last place data is hidden from it? And if so, should it be removed?

The motivating idea is to use XSLT to turn a game description directly into native source for any platform for which someone has written a transformation, with no extra tooling in the pipeline.

## Why it matters per target

- **C++ target:** infix arithmetic is already valid C++, so an expression can be pasted into generated source verbatim and the compiler does the work exprtk does at runtime. No parsing needed, provided dotted names map onto real C++ members or references.
- **Assembly target:** the output is a flat sequence of operations on named locations, for example loading one value into a register, moving it to a second register, loading another value, then subtracting. XSLT would need to understand the expression tree to emit that, so something has to parse the string.

## Options considered

| Option | Idea | Notes |
|---|---|---|
| E1. Paste strings through | Emit the expression text as-is for targets that share its syntax | Works for C++, not for assembly. |
| E2. XSLT parses everything | Tokenise with `analyze-string` (XSLT 2.0+), build the tree with recursive templates, then emit | Possible for short expressions, but precedence parsing is awkward without mutable state. |
| E3. Parse once elsewhere, XSLT emits | A C++ parser turns each expression into an AST written out as XML (its own intermediate format), then XSLT only walks the tree per backend | Keeps the fragile part testable in C++; XSLT does what it is good at (a tree walk, one template per operator). Adds a step outside XSLT. |
| E4. Remove arithmetic from XML | Express computation as elements, so nothing hides in strings | Fully toolable; more verbose; needs vocabulary for math. |

Details noted for the assembly case: a two-register accumulator scheme needs unique temporary names for nested sub-expressions, which can be passed down the recursion (depth counter, or `generate-id()`).

## Where things stand

- The suggestion made in conversation was E3 (AST as XML, XSLT for AST to target text).
- The author would rather not add extra tooling to the pipeline, and is leaning toward asking whether E4 is feasible. No decision.
- Function-call removal ([03](03-expression-syntax.md)) is a prerequisite for any of these.

## Sources

- "XML variable references in game engine" (2026-09-25).
