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

## An object's own size in expressions

Centering a text needs its width, which only exists after a backend has measured the font. Before this, the games hand-tuned offsets for each text (`window.width.center - 350`), which broke whenever the wording, size or font changed.

Built: `objectName.width` and `objectName.height` are available in expressions (meant for `<position>`), and an object refers to itself by its own name like any other field, so a text called `title` is centered with `window.width.center - title.width / 2`. (An earlier draft used `self.width`; dropped because ordinary users would just write the name.) An object's own `<variable>` named `width`/`height` wins over the size. A circle's or rectangle's size follows from its sprite, so its position is exact at load. A text's or image's position is marked unknown (`printGame` says so) until the window has measured it, then worked out, and worked out again whenever its size changes (`Game::resolveSizeDependentPositions`, run once after `Window::init()` and each frame). The program prints the game before and after `Engine` so both states are visible.

**Known wart, shadowing:** `width` and `height` are ordinary variable names, so an object's own `<variable name="width">` shadows its measured size, and the size becomes unreachable from expressions. Precedence (variable wins) keeps existing games working, but it is a collision by design. Alternative spellings that could not collide, none built: a reserved suffix or prefix for built-ins (`title.size.width`, `title.@width`, `title.$width`); a separate namespace (`size(title).x`, `title.size.x`); rejecting a `<variable>` named `width`/`height` on objects whose size is measurable, so the clash is an error rather than silent shadowing; or a warning at load time when shadowing happens. Worth deciding before games in the wild depend on the current spelling.

Placing one object by another's size works the same way (`title.width + 10`). Other ways it could be spelled, none built: an alignment attribute on the object instead of arithmetic (`align="center"`, an origin at the center); a per-axis anchor. Limits of what is built: a position is only recomputed when the size of an object it names changes, and `name.width`/`name.height` read 0 in an expression evaluated before a backend exists (velocity, variables) for text and images.

## Planned

- **`xge::Value`** as a small variant (for example `std::variant<int, float, bool, std::string>`), so an object's variables are `map<string, Value>` filled from whatever the XML declares. "Score" and "doubled" are then data invented by the game author, not special cases in C++. The current code stores floats only.
- **A reference alternative in `Value`**: instead of a literal, `{handle, variableName}` resolved after the first pass, following the reference (recursively, if that variable is itself a reference) when read. A dangling handle returns a default and can be reported, instead of crashing.
- **Binding into exprtk**: per-entity variables bound into the symbol table before a condition or action tied to that entity is evaluated.
- **Cost note:** with a map of strings, each read is a name-to-handle lookup and then a variable lookup. That is fine at Pong or Galaga scale; profile before optimizing.

## Sources

- "Stack vs heap allocation in C++" (2026-08-07).
