# 14. Conditions and win conditions

**Status:** implemented in a minimal form (state-level threshold triggers)

## The problem

A purely declarative file has no natural place for ending the game when someone reaches 15 points. Something has to watch a value and react when it changes. The author's conclusion: in the strictest sense a conditional is unavoidable, so name it a **trigger** or `condition`, not an `if`.

## Decision

```xml
<state name="playing">
  ...
  <conditions>
    <condition class="paddle" variable="score" value="15" action="state('gameover')" />
  </conditions>
</state>
```

The engine evaluates conditions during its update; the file only declares what situation triggers what response. This matches how event and trigger systems in other engines work (a trigger that fires when a threshold is crossed) and stays declarative in spirit.

A first version placed the condition in the `paused` state, which would only have been checked while paused. It belongs in `playing`.

## How it works now

Once per frame, while the state is current, for each condition: find objects matching the `class` and/or `object` filter that have the named variable at or above `value`; the first match runs the action and stops checking for that frame. `reset()` on the game-over screen restores every variable so a leftover condition does not fire again immediately.

## Limits and options

| Limit | Options |
|---|---|
| Only a `>=` threshold on one variable | Comparison operators; conditions on two variables; conditions on expressions |
| Only state-level conditions | Object-level conditions (change color when speed exceeds X; play a death animation at health 0) |
| Threshold limited to 0-255 by the XSD (`xs:unsignedByte`) | Widen to a float/integer type |
| Target action is a state change | Fire any command; win/lose as first-class terms (compare VGDL's TerminationSet, which lists win and lose conditions) |

Open: do conditions belong to states, objects, or both?

## Sources

- "Video game collection value in CAD" (2026-05-23): the `pong_full.xml` version with a condition, and the discussion of trigger vs `if`.
- "Pong game XML structure" (2026-09-22): a proposed extension using `transitions` and `on-enter` elements; those were guesses about where the engine could go, not accepted design.
