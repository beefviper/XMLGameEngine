# Collisions: weaknesses and ideas

**Status:** Known weaknesses are open; escalation is an idea.

## Known weaknesses

- Only the four edges of a rectangle are reported; no penetration depth or contact normal (`bounce` flips one component).
- No broad phase: a double loop over shown objects. A game with thousands would want a spatial grid, or sorted swept boxes (wrap each path in a box, keep sorted along an axis, stop at the first that cannot overlap).
- A pair where nothing moves is not looked at, so a rule against something standing still runs only when the object moves, rides something or hops. A `watch` flag on a rule that must be checked every frame is one way out.
- Only the first hit of a pair per frame is handled.
- Hidden objects do not move or collide; there is no invisible trigger zone or off-screen enemy still moving. Visibility and "in play" would have to become separate flags.
- Other directions looked at: smaller time steps than the frame (the swept test avoids them), deferred handling (detect, then resolve, then update sprites), the world bounds as invisible collidable objects so a rule reads "touching the left bound", separate attributes for "what was hit" and "what to do".


## Response alternatives (ideas)

- Classic reversal (built); contact-point angle (built as `<deflect>`); velocity transfer from a moving paddle (energy Pong: moving away absorbs speed, toward adds it); a mapping from contact (where, how fast, with what) as the rule itself.
- Whose property is reflection: the ball, the paddle, the pair or the rule? That is the open question behind escalation.


## Escalation (idea)

- Problem: rules grow as n types × m types. It is the binary method problem (double or multiple dispatch needs n² cases somewhere); no source calls it "collision escalation". VGDL's hierarchical SpriteSet shrinks n to categories; Magic: The Gathering's layer system composes effects by *kind* with a fixed global algorithm, at the price of designing the taxonomy up front.
- Idea: rules keyed on `(classA, classB, phase)` instead of sprite pairs, and a verb protocol, as in Super Mario Bros.: **declare** (Mario moving down sends `stomp`, chosen from his own state), **respond** (a goomba returns `dead`, a koopa `shell`, a spiny `hit`), **counter** (a `hit` is resolved against Mario's state: star or damage), **bookkeeping**. A new entity costs about v rules (verbs it answers) instead of n.
- Caveats from stress-testing: a four-stage machine is enough (a stack with priority only if one resolution must interrupt another); the entity in the more active state should declare, not always the player (a sliding shell hitting a standing Mario); the stomp check can stay the cheap proxy (downward speed) or use real contact geometry, a one-place decision. The taxonomy of tags and phases becomes the hard artifact and is painful to retrofit.
- Today's engine is at the first rung: `class` and `object` filters on a rule.


## Reference Pong (C++ prototype)

- A single-file SFML 3 Pong kept as the yardstick: named tunables at the top (they became `<variables>`), generic per-object helpers instead of per-object functions, one test point for optional output (text with no font).
- Techniques that fed the engine: swept collision against a moving frame of reference (slab method), sweeping *both* objects, clamping back inside after a wall bounce (otherwise it re-triggers and jitters), a frame-time cap, variable bounce by contact point (now `<deflect>`). Not yet in the engine: speed-up per hit with a cap, paddle motion adding to the angle, push-out along a contact normal.
- A hand-tuned nudge (shifting a score glyph a few pixels) is the polish a declarative text vocabulary would need a per-glyph offset for.
