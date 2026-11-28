# Writing a game out

**Status:** a running record, kept as each game is written out. The suggestions are ideas. Reference: [generating a program](../reference/programs.md#generating-a-program).

## Why this matters most

- The description is the point. A game file should play on any platform. The engine is for testing the games and playing them, and is not the only way to run them (the author, after `pong.xml` was written out).
- So a change to the language is also weighed by how easily `windows-cpp` and `web-js` can write the game out as plain code, and not only by how easily the engine can run it.
- The gap list from `covers.xsl` is the measure: a part that is hard to write out shows up there first.

## What made Pong easy to write out

Pong had been written by hand on the engine's frame a month earlier, at about 530 lines that read as engine code. These changes since then are what let XSLT write it in the shape a person would:

| Change | What it gave the written game |
|---|---|
| Every number per second ([time](29-time.md)) | moving is `position += velocity * seconds`, with no frame counts to carry |
| No arithmetic in strings ([arithmetic](03-arithmetic.md)) | a value is a tree, so XSLT writes `a + b * c`, bracketing only where precedence needs it, with no parser |
| One name per attribute; no reserved names | each covered part is a plain match pattern (`collision[@edge]/bounce`), with nothing split or guessed |
| Conditions holding conditions | already shaped like `if` inside `if`: `computerHolds()` came out as written |
| Seats, `<people>`, `<computer seat>` | "the computer holds seat 2's keys while fewer play" is exactly two bools and one function |
| An object's start values; objects in states | an object that starts again becomes a `startBall()` function |
| A file per verb | a target's `verbs/` mirrors the engine's, so a new verb is a new file in each |

The largest single step was not a language change. It was dropping the frame-for-frame match with the engine: once the written game only had to do the same things, it could be plain code.

## What was still hard

- **What a part is for.** The file does not say which objects move or meet, which are only drawn, and which need a struct of their own. `kinds.xsl` works this out from use. It held for Pong and for Breakout's groups and looks.
- **Centred words.** A centre needs the words' size, which only the window knows, so `main.cpp` measures before drawing.
- **What the engine does without being told.** A target has to copy each of these, and nothing in the file shows them:
  - a state's conditions stop at the first that holds, but every matching touch rule runs;
  - a refused press stops the verbs after it;
  - live numbers are worked out once a frame;
  - a reset also clears what is in flight.

## What Breakout added

| Part of the file | What the written game does with it |
|---|---|
| A group of rows and columns | a list of bricks (`std::vector`, a JS array), built by a `startWall()` loop; a brick hit is removed from the list |
| Looks on the members | a brick's kind is a number into one colour table, not a type each |
| `<self>` in a group's touch rule | the brick being looped over (`cell.worth`) |
| Conditions inside conditions | `&&`, or an `if` inside an `if` |
| An action run by a touch rule | a function of its own, called from the rule |
| Pitch names (`A4`) | hertz, worked out by XSLT at writing time |
| Words in a row | each row measured on its own before centring |

- **Names the platform owns.** A plain page script shares the window's names, so `const top` clashed with `window.top`. The page is now a module script, which keeps its names to itself. Each new target will have its own such names to keep clear of. The game's own names clash the same way with the code's: a `box(..., size)` parameter hid pong_min's `size` constant, which only MSVC flags (C4459), so `test_generate.cpp` now checks that no parameter or local takes a constant's name.
- **The gap list held.** Every part Breakout needed showed up in `covers.xsl` first, and each was a few lines to cover.

## What Space Race added

| Part of the file | What the written game does with it |
|---|---|
| `<row>`s naming their `<number>`s, each a speed | a table of speeds by row (`rowSpeeds[row]`) in the start loop |
| `<wrap />` at the sides | one `if` per side, testing for itself: right off, then a window and its size back |
| A group of members with touch rules | a loop over the members (`for (Rocket* rocket : { &rocket1, &rocket2 })`) |
| A bare `<reset />` in that group's rule | back to a `start` kept in the member's record, its score kept |
| `<type>pixel</type>` and a `<bitmap>` | the picture as rows of text, `touchingPixels()` over its lit pixels, drawn a pixel at a time |
| Conditions in a state that waits for keys | tested at the top of `frame()` while that screen is on top |
| Several `<note>`s, a noise wave | the notes one after another; noise a held random level |

- **Names clash both ways.** Space Race's own constant `seconds` met `frame(float seconds)` and the tone's `seconds`: the written names are now `elapsed` and `duration`, words a game is unlikely to use.
- **Two more hidden rules.** A `<wrap />` waits until the thing is right off the edge and moves it a window and its own size, so a lane keeps its spacing. A bare `<reset />` puts a thing back where it started and keeps its numbers; `<reset object>` starts it again whole.
- **Pixels were small.** The one subsystem-shaped part, a touch by lit pixels, is about a dozen lines once the picture is rows of text.

## What Kaboom added

| Part of the file | What the written game does with it |
|---|---|
| `<scenes>` each `from=` the one before, with the same numbers | a table, a row a scene (`waves[]`), `waveAt`, `wave().pace`, and `startWave()` starting the pieces the scenes take |
| A condition copied down the scenes, one replaced by name in the last | the tests merged by name, then one test for each run of scenes they come out the same in (`waveAt < 7`, `waveAt == 7`) |
| `<timers>` with `<every>` | a countdown in the record (`untilFire`), and a `while` that runs it as often as the time owes |
| `<fire object>`, `<most>` | a list of the piece (`bombs`), a new one pushed from under the thing it leaves, none past the most |
| `<despawn object>`, a bare `<despawn />` | the list emptied (and out of the loop over it); the one met erased |
| `<animation>` on a fired piece | an `age` on each, the picture picked by `age / seconds` |
| `<become from= sprite=>` | an `if` on the look, the look's picture size set with it |
| `<color char>` on a picture or its style, `<background>` | a palette a picture (`Colors`), each lit pixel its letter's colour; the window cleared to the background |
| `axis="pad1.x"` | the stick's moves sent to `mouse()` |

- **More hidden rules.** The pieces a scene takes draw first, the screen's own objects next, what is fired last, so the score sits over the rooftop. A timer's wait is drawn again each time it is done, and the time over carries. A look of another size keeps the thing's top left. A shot leaves from the middle of the side its thing faces.
- **The scenes were a table all along.** Kaboom's eight waves differ only in six numbers and one test; written out, they are eight rows and two `else if`s. A scene that changes more than numbers would need more, and this target says so rather than guessing.
- **Names clash with the game's again.** The tone's `enum class Wave` met the game's `Wave` (a scene), so the sound's is now `Waveform`. In JavaScript, a start function that reads its own thing's size (`bomber.height`) reads it before the thing is made, so the size is written as the number it is.

## What Freeway added

| Part of the file | What the written game does with it |
|---|---|
| Kinds in a grid each giving a place, a speed, a count, a spacing or a picture | one `addTraffics(look, first, size, speed, count, gap)` call a lane, the loop counting to `count` |
| Kinds each a picture, or a colour on the grid's picture | a table by look of the rows and colours (`trafficDrawings`), a kind's colour in place of its picture's own |
| `<hop>` in an action a key triggers | a function the press calls with the member (`up(chicken1)`): a refused hop `return`s before the move, so its sound never plays |
| `<move>` in a rule on meeting | that many pixels that way, once (`chicken->position.y += cell`) |
| `<timers>` in the played state | a countdown of the game's own (`untilTick`), started again with a new game |
| A `<sound>` of `<part>`s | the parts made one by one and added together (`together()`); in JavaScript, each part started at the same moment |
| A member's `<position>` with only `x` | its `y` the group's |

- **Numbers keep their kind.** `800 / 3` is two whole numbers in C++, and cut to 266; whole numbers divided are written `800.0f / 3`. A countdown starting at a game constant is a `float` like the constant, and is shown as the whole number it holds.
- **Names clash again.** The game's `cell` is also the word for a grid's cell, so the loop calls one `one`. Its `seconds` started the clock's own `seconds`, so that is written `freeway::seconds`. Actions named `up` and `down` met `key(..., bool down)`, so a key's state is now `pressed`.
- **One more hidden rule.** A kind's picture (lane 9's `<bitmap from="lane9" />`, rows only) takes what it leaves out, its scale and colours, from the grid's picture, so the trucks are 88 by 28, not 22 by 7. The written game works it out (`kind-scale`, `kind-colors` in `kinds.xsl`).

## What Frogger added

| Part of the file | What the written game does with it |
|---|---|
| Groups in groups (the river's logs and turtles, each with kinds) | made plain before writing (`prepare.xsl`): one list of logs, its looks each kind's and each turtle look's |
| A kind with several looks, a `<timer sprite= after>` on one | a `look` per cell and a `lookTime`; each frame, an `if` per timed look moves it on |
| A rule on a kind by `sprite=` (a diving turtle drowns the frog) | an `if` on the cell's look inside the loop |
| A grid with no kinds and several looks (the pads) | each cell its look, drawn by an `if` on it |
| `<die />` in an action, `<reveal>` later | an `inPlay` the frog's actions, meetings and drawing wait on |
| `<spawn>` with `<most>`, `<animation><once />` and `<then>` | a list of splats, pushed centred on the frog; each picture until its time is up, then what it does and erased |
| `<ride />` | the frog moved by the log's speed for the frame |
| `<hitbox>` | a body pulled in from the picture's sides (`inset`), used for every touch |
| `unless=` | the rules wrapped in `if (!touchingAny(frogBody, pads))` |
| A bare `<reset />` in an action | back to the `start` kept in the record, standing still |

- **A third level was a shape, not a rule.** Every template knew grids of kinds; the river added groups of grids. Flattening the game first (`prepare.xsl`, run by `xgecli` before `generate.xsl`) kept the rest unchanged, and the six games before it come out byte for byte the same.
- **Hidden rules again.** An object out of play is not moved, drawn or touched, and its actions are refused, so a key pressed while the splat plays does nothing. A spawned piece with no place of its own is centred on what spawns it. A refused spawn (one splat at most) stops the rest of the action, so the frog dies once.
- **Names.** A class word ends in an s more often than not (`logs`, `hazards`); one kind is `log`, but `debris` stays `debris`. A plural of a word ending in s takes `es` (`splashes`).

## What Prism Fall added

| Part of the file | What the written game does with it |
|---|---|
| `<lockstep>` on a group | one clock for the list; every `pace.beat` seconds each cell steps on, a picture and a sound a step |
| A grid at a side, `<bounce />` and a `<move>` down | all of the list turned round, back a step and down |
| Rows by `<number>` with their own looks and worth | tables by row (`rowLooks[row]`, `rowWorths[row]`) |
| `<timers>` in a grid, every so often at random, `<fire>` from a cell | a countdown in each cell (`untilFire`), a spark pushed onto its list |
| `<fire>` with one in flight at most | a beam list, the press refused while it holds one |
| `<hole>` | `cutHole(pylon, beam, 9, 0.5)`, clearing lit pixels in the pylon's own copy of its picture |
| `<waiting />`, and a timer's `<reveal>` | `inPlay: false` until `untilComet` runs out |
| A screen with an `<animation>` | its own time (`crackedTime`), the picture by `crackedTime / 0.12` |
| `<set group= variable=>` | a loop over the list (`for (const shard of shards) { shard.velocity.x *= 1.25; }`) |
| `<remaining>` and `total=` on a class | `shards.length === 0`, `shards.length <= pace.next`, `shards.some(...)` |
| A `<hitbox>` uneven on its sides | `inset(lens, 5, 10, 5, 0)`, pulled in by so much from each side |

- **Frames share a palette.** Pictures named `diamond-a` and `diamond-b` with the same colours share one `diamondColors`, not one each.
- **Names clash again.** The game's `top` and `pixel` met the inset's `top` and the drawing's `pixel`, which MSVC flags; the locals are now `fromTop` and `pixelSize`.
- **One hidden rule.** A picture's hole belongs to that one pylon, so each pylon keeps a copy of the rows.

## Suggestions

Each is an idea, to be weighed when the game that needs it is written out.

- **A page of what a target must do.** This would sit beside "What a window must do" in the [engine reference](../reference/engine.md), and list every behaviour above that a written game must copy. It would turn hidden rules into a checklist and show which ones are worth making explicit in the file.
- **Anchors** ([values-and-names](04-values-and-names.md), idea). `anchor="center"` would replace the common `windowWidth / 2 - size.x / 2` with a word that each platform writes in its own way.
- **A new verb should be easy to write out.** When adding one, ask whether it can be written as about ten lines of plain code. If it needs a subsystem (sweeps, pixel collisions, the board search), say so in its design. Those stay `engine-cpp` and `web-asm` only until a target needs them.
- **Regular groups.** Rows times columns become one loop over an array. Per-member exceptions and numbered names (`x.N`) each need special code, so prefer a look or a variable on the member.
- **Count the gaps.** A long test could run every game through `covers.xsl` and count how many games each gap blocks, giving a ranked to-do for the readable targets.

## Open

- Whether `kinds.xsl`'s guesses should become words in the file. So far, inferring them has been enough, and that keeps the file free of code-shaped hints.
