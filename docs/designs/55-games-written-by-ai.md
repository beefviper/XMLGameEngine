# Games as tests: games written by another AI

**Status:** Built. All three games have since been rewritten.

## What the test showed

The author asked another AI to write three games (Berserk, Demon Attack, Frostbite) from `xgedef.xsd` alone. All three passed the schema and loaded; none could be played. Every error was something the schema accepts where the engine does nothing:

- colours like `white` or `#00ff00` (a colour is a `color.` name);
- key names the engine lacks (`button="fire"`);
- `<move>` or `<fire>` in an `<input>`, `<inc>` in a condition, `<reset object>` in a collision (the last since allowed);
- a variable with no owner (`<inc variable="score">`);
- a spare projectile that started in play and flew off.

**Lesson:** the vocabulary was enough (no tag was added), but the schema could not tell an author a command was in the wrong place. **Chosen:** the loader refuses, as errors not warnings, unknown keys and colours, a command where it does nothing, and a change to a variable no object has, each naming what to write instead, with a "did you mean". Where a command works is `ruleFor` in `load/check.cpp`; later the schema's command groups said the same ([file format](02-file-format.md#commands-in-groups)).

## The rewrites

- **Demon Attack:** three waves; demons that `<release>` two small ones when shot; big demons with `whole` and `hurt` looks; the shot steered over the cannon by a timer that sets `laser.x`, as the original does.
- **Frostbite:** three nights as scenes of a `from=` chain giving the drift; the four rows one group whose `sprite="white"` rule turns its row blue and builds a block of the igloo; the igloo built bottom up by `<reveal>` in order.
- **Berserk:** after Stern's 1980 game. Four rooms as scenes, each a `<map>` of the arcade's five-by-three frame on half-cell tiles; robots a kit group with a member per robot per room; Otto out of play until a room timer reveals him.
  - **Every rule that costs a life is the man's own:** `<reset object="player" />` from another's rule would reset his score and lives too, since a reset object resets every variable.
  - He comes in opposite the exit he took, and that doorway shuts behind him.
  - Eight directions by two keys held; robots fire only when lined up (`<aim>` with `<lined>`).
  - Making it load found two slow spots, both fixed: an expression parser built per value (now per distinct text), and in-play checks per pair (now per object).
  - Differences: straight-line chasing, four fixed rooms in a two-by-two loop (the arcade's grid needs maps made at run time), speeds chosen without watching play.
