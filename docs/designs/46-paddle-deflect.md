# 46. Paddle deflect

**Status:** built (`lib/source/command_executor.cpp` `CommandExecutor::deflect`, `games/pong.xml`, `tests/test_deflect.cpp`); one new command, `<deflect>`

## Why

Pong's ball only ever bounced: `<bounce />` flips one part of the velocity, so the angle the ball left a paddle at was the angle it came in at, set once by the `<random>` velocity it started with. Whatever the players did, a rally kept the same slope. The author asked for the arcade behavior: a hit in the middle of the paddle sends the ball straight back, a hit at the top end sends it back up at 45 degrees, and the bottom end down at 45. The reference Pong in C++ ([17](17-reference-pong.md)) already did this ("variable bounce"), and design [11](11-collision-detection-and-response.md) listed it as the contact-point response. Nothing in the vocabulary could say it: a rule can tell which object and which side was touched, but not where along the side, and no command sets a velocity from an angle.

## The language

```xml
<collision>
  <deflect>45</deflect>
  <play sound="paddle" />
</collision>
```

`<deflect>` goes in a rule about another object. Its content is a value: the widest angle, in degrees, from straight back out (0 up to, not including, 90). The object leaves the side it touched at an angle set by where along that side its middle was: square in the middle is straight out, level with either end (or beyond it, clipping a corner) is the full angle towards that end, and in between is in proportion, so 45 at half way up the paddle is 22.5 degrees up. The speed (the length of the velocity) is kept. In a screen-edge rule it does nothing, as `<carry />` does not.

The content is the angle, not an attribute, by the format's rule ([03](03-expression-syntax.md)): it is a number, and a variable can set it.

## Decisions

- **A new verb, not `<bounce>` with content.** `<bounce>45</bounce>` would have read well too, but `<bounce />` also runs against screen edges and moves the invader block in lockstep; keeping the angle out of it leaves every bounce as it was, and a game can still say plain `<bounce />` against a paddle.
- **Measured along the other object's side, from its middle.** Half the side's length is the full angle, so "the top edge" means what the author said: the ball's middle level with the top of the paddle. Measuring to the ball's own edge (half the paddle plus the ball's radius) was the alternative; it gave about 40 degrees at the top corner of Pong's paddle and needed a hit off the corner itself to reach 45.
- **Which way is the side's, not the ball's.** The ball leaves away from the side it touched (the same side `<bounce />` uses), whatever its velocity was, so a ball coming in steeply still goes the way the hit position says. A hit on a top or bottom side is handled the same way, along that side.
- **The speed is kept.** Speeding up on each hit, and the paddle's own motion adding spin, were part of the reference Pong; neither was asked for. Speeding up would be a second command (or a `<faster>`-style amount); the paddle's motion is the velocity-transfer idea in [11](11-collision-detection-and-response.md).
- **The ball's start is unchanged.** It still starts at a random velocity of up to 7 across and 3 up or down, so a serve can be slow or shallow; after the first paddle hit the hit position decides the slope.

## Open points

- A serve at a set speed and a random angle (needs a value made from an angle, or `<random>` over a heading).
- Speeding up a little on each paddle hit, with a cap.
- Adding the paddle's own vertical motion to the angle.

## Sources

- The author's request in the project thread (2026-10-03).
- The reference Pong's variable bounce ([17](17-reference-pong.md)), and the contact-point response in [11](11-collision-detection-and-response.md).
