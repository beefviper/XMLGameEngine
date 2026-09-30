# 06. Names and classes

**Status:** implemented (`name`, `class`, filters); unique per-instance names and class inheritance are ideas only

## Decision

Objects have a `name` (their unique identifier) and an optional `class` (a group label). This is borrowed from HTML's `id` and `class`, with the same purpose: target one specific thing, or all things of a kind.

```xml
<object name="paddle1" class="paddle"> ... </object>
<condition class="paddle" variable="score"><atleast>15</atleast><push state="gameover" /></condition>
```

A single condition on `class="paddle"` checks either paddle's score, instead of writing one condition per paddle.

## Where it is used today

- `<condition class=".." object="..">`: either filter or both (combined with AND); neither matches any object.
- `<collision class=".." object="..">`: an object-against-object rule that only responds to a kind of other object or to one instance. Without `edge`, naming a class or object is enough to make it an object rule; `basic="basic"` remains for the one unfiltered, match-anything rule.
- `class="projectile"` has a built-in meaning: the object starts invisible.

## Ideas discussed, not built

- **Compound selectors** in conditions, in the style of CSS: a condition on any object of class `paddle` belonging to one player, to separate a two-player win from either-player win. Judged premature for Pong; noted so it is not forgotten.
- **Class inheritance**, for example a `platform` class that extends a `solid` class. Related to the collision classes in [12](12-collision-escalation.md).
- **Unique names for every instance.** The author's view is that every object block needs a unique name anyway, so a grid could name its cells `block[x,y]`, and formations `group1[x]`, `group2[x]`. Today all cells of a `grid()` share the grid object's name, so a single brick cannot be addressed (see [07](07-object-variables-and-references.md) for why unique names would also simplify references).
- **Wildcard queries** over an object's variables (`player1.*`), in the style of mIRC's scripting engine, as a way to work with groups of values. This would sit on top of a per-object variable map ([07](07-object-variables-and-references.md)).

## Sources

- "Video game collection value in CAD" (2026-05-23): the HTML analogy and the class-filtered win condition.
- "Stack vs heap allocation in C++" (2026-08-07): unique names, `block[x,y]`, wildcards.
