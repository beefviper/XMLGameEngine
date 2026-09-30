# 13. Verb vocabulary

**Status:** a small set implemented (Frogger added five); the rest is a plan

## Principle

A verb is a named behavior with parameters. A good declarative verb captures the *behavior space* of a concept rather than an implementation. With a well-defined vocabulary the author expects to describe the 2D non-scrolling games of the late 70s and early 80s "pretty easily", and, with verbs like jump, the scrolling ones too.

## Implemented today

the command tags `<bounce />`, `<stick />`, `<die />`, `<reset />`, `<inc />`, `<dec />`, `<wrap />`, `<carry />`, `<move direction=...>`, `<hop direction=...>`, `<push />`, `<pop />`, `<trigger />`, `<fire />`; sprites `<circle>`, `<rectangle>`, `<text>`, `<image>`, `<grid>`; a collision rule filter, `unless`; the value tag `<random>`; and the condition forms `atleast`, `atmost` and `remaining`. See [docs/readme.md](../readme.md) for exact meanings.

Rough coverage by game: Pong is essentially `bounce()` and `stick()`; Breakout adds `die()`; Space Invaders adds `fire()` and formation movement through lockstep bounce; Frogger adds `hop`, `carry`, `wrap`, `dec`, `unless` and `atmost` (see [21](21-frogger.md) for why each is shaped the way it is). Space Race added no verbs: it is `move`, `stick`, `inc`, `reset`, `wrap` and a score condition. Gem Catcher added none either: `move`, `stick`, `inc`, `dec`, `reset`, `<random>` and two variable conditions ([30](30-gem-catcher.md)).

The rule of thumb held: each of those was added because Frogger could not be described without it, and each names a behavior rather than an implementation (`carry` says an object rides another, not how its position is updated).

## Planned: jump

Two types.

| Type | Behavior | Parameters (at minimum) | Model |
|---|---|---|---|
| **Static** | Pressing jump stops walking; the character follows a predefined arc and fully lands before control returns | height, width (length), time | Castlevania |
| **Dynamic** | Height depends on how long the button is held; run speed extends the jump; direction can be changed in the air | maximum hold time, maximum height, upward acceleration, whether the player can slow or stall in the air, whether the player can move backwards in the air | Super Mario Bros. |

`hop` is the tile-step cousin of the static jump, not an arc: one instant step per key press, with no height, width or time. It is what a grid game needs (Frogger, and later Pac-Man-style turns), and an arcing jump would be a separate verb that shares the one-shot key handling.

Notes: in Super Mario Bros. you can jump up and back onto a ledge directly above by moving backwards mid-jump; in Mega Man you can slow or stall but not reverse. Whether backwards movement is allowed is therefore a parameter.

## Planned: AI targeting

Classic maze-game AI looks complex but decomposes into a few strategies with parameters. The Pac-Man ghosts, described verbally:

- Blinky chases the player directly.
- Pinky targets a few tiles ahead of the player's direction.
- Inky uses both the player's position and Blinky's, to flank.
- Clyde chases when far and retreats to a corner when within a threshold distance.

Sketch in the same declarative style:

```xml
<ai targeting="direct" />
<ai targeting="ahead" lookahead="4" />
<ai targeting="flank" reference="blinky" />
<ai targeting="proximity_flee" threshold="8" fallback="corner" />
```

Pathfinding is the engine's job, not the XML's. Other likely targeting modes: direct, predictive, flanking, random, territorial. A survey of more maze games is expected to reveal the basic set.

## Questions about where a verb ends

- Is scrolling a property of the camera or world, or a verb applied to objects?
- Is gravity a global world parameter, or a force verb applied to specific objects?
- The author's rule of thumb: the vocabulary grows only when a real game cannot otherwise be described.

## Related smaller ideas

- **Random speed with a dead zone.** `<random min="-7" max="7" />` can give a near-zero velocity; the workaround so far is to reload. A declarative fix: a random magnitude with a random sign (a random from 3 to 7 times a sign), for example a `<sign>` value tag, avoiding an `if`.
- **Sound.** There is no audio vocabulary yet.
- **Pong to Galaxian, written formally.** The chain of tweaks ([01](01-vision-and-scope.md)) could become a design document showing which verbs each step adds.

## Second batch: alternatives

- **Jump.** See [23](23-jump-and-air-control.md): a jump model plus a per-axis air-control ladder, in place of only static and dynamic.
- **Motion verbs.** [22](22-motion-models.md) for names of the motion models; [24](24-paths-and-formations.md) for scripted paths.
- **Why the verb approach works.** Shared verbs (bounce, die, destruct) name expectations and hide machinery: detection, normals, restitution, clamping. A verb set stays finite when it counts mechanism families, not games.
- **Danger sign.** A verb that exists to mean behave like one particular game shows the parameters were not found.
- **Value helpers.** Clamp, saturate, cap and the other balance tricks in [28](28-game-ideas-and-test-games.md) as candidate value tags.
- **Battle resolution.** Pairwise, aggregate and dominance styles ([28](28-game-ideas-and-test-games.md)) as parameters of one verb.

## Sources

- "Engine design conversation A (a long general chat; only the project segment was used)" (2026-05-23): jump, Pac-Man AI.
- "Video game description languages: overview and research" (2026-09-22): jump parameters (that conversation ended before a reply to the final message).
- Second batch: "Jump Behavior Models" (2026-09-23), "VGDL Design Challenges" (2026-01-28), "VGDLs and Technology" (2026-05-09).
