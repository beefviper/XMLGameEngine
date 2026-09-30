# 06. Names and classes

**Status:** implemented (`name`, `class`, filters, unique names for grid cells); class inheritance and compound selectors are ideas only

## Decision

Objects have a `name` (their unique identifier) and an optional `class` (a group label). This is borrowed from HTML's `id` and `class`, with the same purpose: target one specific thing, or all things of a kind.

```xml
<object name="paddle1" class="paddle"> ... </object>
<condition class="paddle" variable="score"><atleast>15</atleast><push state="gameover" /></condition>
```

A single condition on `class="paddle"` checks either paddle's score, instead of writing one condition per paddle.

## Where it is used today

- `<condition class=".." object="..">`: either filter or both (combined with AND); neither matches any object.
- `<collision class=".." object="..">`: an object-against-object rule that only responds to a kind of other object or to one instance. Without `edge`, naming a class or object is enough to make it an object rule, and a `<collision>` with no selector at all matches anything.
- `class="projectile"` has a built-in meaning: the object starts invisible.

## Unique names for grid cells (built)

The author's view was that every object block needs a unique name anyway. Every cell of a `<grid>` is now its own object, named after the grid with its column and row counted from 1 (`aliens.3.2`). The grid's own name still works where the whole group is meant: `<show object="aliens"/>`, a rule or condition with `object="aliens"`, `<reset object="aliens" />`, while a single cell can be named on its own. This is what lets each cell be swept and killed on its own ([11](11-collision-detection-and-response.md)) and lets a condition watch one alien. Names for formations (`group1[x]`) were not built.

## Ideas discussed, not built

- **Compound selectors** in conditions, in the style of CSS: a condition on any object of class `paddle` belonging to one player, to separate a two-player win from either-player win. Judged premature for Pong; noted so it is not forgotten.
- **Class inheritance**, for example a `platform` class that extends a `solid` class. Related to the collision classes in [12](12-collision-escalation.md).
- **Wildcard queries** over an object's variables (`player1.*`), in the style of mIRC's scripting engine, as a way to work with groups of values. This would sit on top of a per-object variable map ([07](07-object-variables-and-references.md)).

## Second batch: alternatives

- **Tags in rules.** A rule could match by a tag (touching any enemy) instead of a name; this is what classes are for, and the second batch used the word tag for the same thing. Whether an object may carry several is the open part.
- **Names as references.** A required unique name is what lets rules, inputs and conditions point at objects ([02](02-file-format.md)).

## Sources

- "Engine design conversation A (a long general chat; only the project segment was used)" (2026-05-23): the HTML analogy and the class-filtered win condition.
- "C++ design conversation A" (2026-08-07): unique names, `block[x,y]`, wildcards.
- Second batch: "Name Attribute in XML" (2026-07-22), "Declarative Pong XML redesign" (2026-09-23).
