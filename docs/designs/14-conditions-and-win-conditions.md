# 14. Conditions and win conditions

**Status:** implemented in a minimal form (state-level threshold, "has fallen to" and "none left" triggers)

## The problem

A purely declarative file has no natural place for ending the game when someone reaches 15 points. Something has to watch a value and react when it changes. The author's conclusion: in the strictest sense a conditional is unavoidable, so name it a **trigger** or `condition`, not an `if`.

## Decision

```xml
<state name="playing">
  ...
  <conditions>
    <condition class="paddle" variable="score"><atleast>15</atleast><push state="gameover" /></condition>
  </conditions>
</state>
```

The engine evaluates conditions during its update; the file only declares what situation triggers what response. This matches how event and trigger systems in other engines work (a trigger that fires when a threshold is crossed) and stays declarative in spirit.

A first version placed the condition in the `paused` state, which would only have been checked while paused. It belongs in `playing`.

## How it works now

Once per frame, while the state is current, for each condition: find objects matching the `class` and/or `object` filter that have the named variable at or above `<atleast>`; the first match runs the action and stops checking for that frame. `reset()` on the game-over screen restores every variable so a leftover condition does not fire again immediately.

A second form covers the win that is not a score: `<condition class="aliens"><remaining>0</remaining><push state="gameover" /></condition>`. It counts the matching objects that are still visible (`die()` hides an object) and fires when that count is no more than `remaining`. Space Invaders uses it: the aliens are all cells of one `grid()`, `class="aliens"` matches every cell, and when the last one dies the game goes to `gameover`, where a bare `reset()` restores them.

A third form reads a variable from above: `<condition object="frog" variable="lives"><atmost>0</atmost><push state="gameover" /></condition>` fires when the variable is at that value or below. Frogger counts its lives *down* with `dec()`, and "lives is 0" reads far better in the file than a condition that fires when a counter of deaths reaches 3, which would hide the starting number of lives in the condition instead of in the frog's own `<variable>`. It is `atmost` and not an operator for the same reason `remaining` is: a name that says what it means. It fires at or below, not only at equal, because a variable may go below zero (a life lost twice in one frame).

Choices made: a name that says what it counts (`remaining`) rather than an operator or an expression; it counts *visible* objects, not enabled ones, because an alien that has died is both hidden and out of collisions while a text object with collisions off is still very much there; and a filter that matches nothing warns at load, since "none left" would otherwise be true from the first frame. Alternatives not built: a general comparison (`remaining="<3"`), or counting any variable across a class (`sum`, `count`), which is where a real expression language for conditions would start.

## Limits and options

| Limit | Options |
|---|---|
| Only a `>=` or `<=` threshold on one variable, or "no more than N left" | Comparison operators (`atmost` and the plain threshold are two of them, spelled as names); conditions on two variables; conditions on expressions; counting by a variable's value |
| Only state-level conditions | Object-level conditions (change color when speed exceeds X; play a death animation at health 0) |
| Thresholds (`value`, `atmost`, `remaining`) limited to 0-255 by the XSD (`xs:unsignedByte`) | Widen to a float/integer type |
| Target action is a state change | Fire any command; win/lose as first-class terms (compare VGDL's TerminationSet, which lists win and lose conditions) |

Open: do conditions belong to states, objects, or both?

## Second batch: alternatives

- **Unwinnable starts.** Some games can deal a losing board no matter the skill. Alternatives: guarantee solvability at setup, detect a state with no legal moves as a loss condition, or leave it.
- **Performance-driven conditions.** Rules that read how well the player is doing and change stats (help for failure, taxes for excess) need conditions that can compare a rolling measure ([28](28-game-ideas-and-test-games.md)).
- **State-based predicates.** No moves left, nothing remaining, or a timer, next to the threshold form.

## Sources

- "Engine design conversation A (a long general chat; only the project segment was used)" (2026-05-23): the `pong_full.xml` version with a condition, and the discussion of trigger vs `if`.
- "Pong game XML structure" (2026-09-22): a proposed extension using `transitions` and `on-enter` elements; those were guesses about where the engine could go, not accepted design.
- Second batch: "Unwinnable Game States" (2025-05-23), "Power-Up Mockery Concept" (2025-02-27).
