# 44. Games written by another AI

**Status:** built (`games/berserk.xml`, `games/demonattack.xml`, `games/frostbite.xml`, `tests/test_ai_games.cpp`); no new tags

## Why

The author asked another AI to write three games from the schema alone (`assets/xmlgameengine.xsd`), to see whether the language is expressive enough for something that has not read the engine's source or the other games. The three came back as small arcade sketches of Berserk, Demon Attack and Frostbite. All three passed the schema first time, and all three loaded. None of them could be played, and the first thing seen of them was a blank screen.

## What the other AI got wrong, and why the schema did not say so

Every one of these is a command the schema accepts in a place where the engine does nothing with it. The games were corrected, but the gaps are in the language, not only in the games, so they are listed here.

- **Colors the engine does not know.** `white`, `black` and `#00ff00` are valid text to the schema, but a color is one of the `color.` names (`color.white`, `color.green`, ...) and anything else is transparent, so the games loaded and drew nothing: a blank screen. Each is now the nearest `color.` name (the navy of Frostbite's sky is `color.darkblue`), and a test reads every `<color>` and `<background>` in the three files.
- **A key name the engine does not have.** `<input button="fire">`. A button is any string; an unknown name reads as `KeyCode::Unknown` and the input never runs. The games use Space (the shared convention) and W, A, S, D beside the arrows.
- **`<move>` and `<fire>` straight in an `<input>`.** An input runs `<push>`, `<pop>`, `<reset />`, `<reset object>` and `<trigger>`; a move or a fire only means something inside an object's `<action>`. The player gained `<actions>` and the inputs became `<trigger object="player" action="left" />` and so on.
- **`<inc>` and `<dec>` in a `<condition>`.** The schema lets a condition run them; the engine runs only the state commands and `<reset />`. Demon Attack's wave counter had no effect and was removed (nothing read it). Frostbite's cold was meant to be a counter ticking up to 60 and taking a degree: it is now a one-pixel object the colour of the sky that crosses the screen once a second, and its `edge="right"` rule takes a degree and wraps, which is a place `<dec>` does run.
- **`<reset object="player" />` inside a `<collision>`.** Ignored there; a bare `<reset />` is how an object puts itself back. Without it the player stayed on the robot and lost a life every frame.
- **A group named in `object="..."`.** A collision's `object` is one object's name; a group is matched by `class`. The same for a condition's `<remaining>`.
- **A variable with no owner.** `<inc variable="score">` prints "no such object" and does nothing: a variable belongs to an object, and the game's own `<variables>` are constants. Score and lives live on the player (`player.score`) and are drawn by two text objects.
- **A spare projectile that starts switched on.** `<fire>` takes a projectile whose collisions are disabled; one that starts enabled is never fired, and it flew off the screen with nothing to put it away. The bullets start disabled with an `edge="all"` rule that does `<die />`, like the shot in Astrosmash.

## What was left as the other AI wrote it

The games are small on purpose and were not made better. Berserk's robots pace sideways and never chase, and its two rooms are one maze; Demon Attack's demons never shoot back; Frostbite has no water, so only the bird, the fish and the cold can hurt the builder. A game's size and shape are the other AI's.

## Tests

`tests/test_ai_games.cpp` plays each game through a real `Engine` with no window, with the keys a player would press: each loads with what it should, Space starts it, the movement keys move the player, a shot kills what it hits and scores, a touch costs a life and puts the player back, the end screen comes up and Space plays again. The three also load through all four XML libraries (`test_xml_format`) and follow the shared key conventions (`test_engine_input`).

## What this says about the language

The vocabulary was enough: the three games are written with what was already there, and no tag was added. The schema is not strict enough to tell an author when a command is in the wrong place, and an author who has never seen the engine finds that out only by playing. Making the loader refuse an unknown button name and the commands an `<input>`, a `<condition>` or a `<collision>` would not run, and warn about a variable no object owns, would have turned each of the mistakes above into a load error naming the line. Not done; it is a change to the engine and to the schema's content models, for the author to decide.

## Later

On 2026-10-03 ([47](47-timers-facing-jumps-and-looks.md)) Frostbite was rewritten from scratch (the shore and igloo at the top, rows of ice below, jumps between them; `tests/test_frostbite.cpp`), and the two others were changed: Demon Attack's demons fire back, and Berserk's robots fire back, its man shoots the way he faces, and its walls kill (the `<stick />` against an object the other AI wrote did nothing). Two of the gaps above are closed by the same change: a `<condition>` now runs `<inc>` and `<dec>`, and a collision runs `<reset object>`. The schema is still not strict about where a command goes.
