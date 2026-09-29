# 07. Object variables and references between objects

**Status:** per-object numeric variables and `owner.variable` text binding implemented (polling); a general `Value` type and handle-based references are planned

## What is needed

Some state belongs to the entity itself, and some state has to be read by another entity:

- A ship's "doubled" flag (Galaga-style) is the ship's own state.
- In Pong, the score belongs to the paddle, and the scoreboard object displays it.

## Decision

The author's design for the XML: describe the relationship, not the mechanism.

```xml
<object name="paddle1"> ... <variable name="score" value="0" /> </object>
<object name="score1">  ... <variable name="display" value="paddle1.score" /> </object>
```

The XML author says the paddle has a score and the scoreboard shows it. Underneath, the engine polls.

## Options considered

| Option | Notes |
|---|---|
| **Polling** | The reader looks the value up whenever it needs it (each frame it draws). Simple; composes with a declarative file (each frame, read this variable and draw it). One lookup per access. |
| Listeners / notifiers | The owner pushes changes to subscribers. Better when values change rarely and many things react; but the owner can then hold a stale reference to a destroyed subscriber, so cleanup is needed at both ends, and "subscribe" wiring does not look declarative. |
| Copying the value | Goes stale. Rejected. |
| Raw pointers to the other entity | Dangle when storage moves or the entity is removed. Rejected in favor of handles ([08](08-entity-storage-and-handles.md)). |

**Chosen:** polling, with references resolved to something stable once at load.

## Current implementation

- Each `Object` has `std::map<std::string, float> variable` plus `variableOriginal`, a snapshot used by `reset()` so a new game starts clean and a leftover condition does not fire at once.
- Every object variable is also registered in exprtk as `owner.variable` ([05](05-variables-and-evaluation-order.md)).
- `text(paddle1.score, ...)` is bound to that variable (`boundVariableOwner`, `boundVariableName`). `inc('paddle1.score')` increments it and marks the text object's visual dirty so the backend redraws it. This is the current polling path.

## Planned

- **`xge::Value`** as a small variant (for example `std::variant<int, float, bool, std::string>`), so an object's variables are `map<string, Value>` filled from whatever the XML declares. "Score" and "doubled" are then data invented by the game author, not special cases in C++. The current code stores floats only.
- **A reference alternative in `Value`**: instead of a literal, `{handle, variableName}` resolved after the first pass, following the reference (recursively, if that variable is itself a reference) when read. A dangling handle returns a default and can be reported, instead of crashing.
- **Binding into exprtk**: per-entity variables bound into the symbol table before a condition or action tied to that entity is evaluated.
- **Cost note:** with a map of strings, each read is a name-to-handle lookup and then a variable lookup. That is fine at Pong or Galaga scale; profile before optimizing.

## Sources

- "Stack vs heap allocation in C++" (2026-08-07).
