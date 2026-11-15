# Time

**Status:** Open. The author has decided movement goes by time passed; this file is the design to choose from before any code.

## What we want

A game's speed should not follow its frame rate. Today a velocity is pixels per frame, so the same file at `<framerate>120</framerate>` plays twice as fast. Timers, animations, hops and swings are already written in seconds and turned into frames at load; movement is the odd one out.

## Where things stand

- **The loop is already a fixed step** (`Engine::loop`): the clock says how many frames are owed, each is played whole, then the picture is drawn once. A stall of more than four frames is dropped rather than raced through. So a slow machine already keeps the game's pace (up to that limit), and every run plays the same frames.
- **No shipped game sets `<framerate>`:** all 59 run at 60. Moving them is multiplying their per-frame numbers by 60.
- **The per-frame words** a game writes:

| Kind | Words | Per frame today | Per second |
|---|---|---|---|
| speeds | `<velocity>`, `<move>` while held, a path's speed, `<hunt>`/`<go>` pace, `<seek>`'s most | pixels a frame | pixels a second |
| speed gained | `<acceleration>`, gravity, `<thrust>`, `<seek>`, `<pull>` | pixels a frame, each frame | pixels a second, each second |
| turning | `<turn>` while held, a spin | degrees a frame | degrees a second |
| shares | `<drag>`, `<friction>`, `<grip>`, a `<surface>`'s | share lost each frame | share kept each second |
| height | `<rise>`, `<fall>` | as speeds and speed gained | as those |
| counts | an `<inc>` on a held key | added each frame | open (below) |

Live numbers read and set the same units: `ball.velocity.x` would read pixels a second.

## What we looked at

- **A. Variable step:** each frame moves by the time it actually took. Smooth on any screen, but no two runs are alike, so the frame-by-frame tests, recorded demos, the side-by-side proof of generated games and the board-game search all lose repeatability. A long frame also carries things further between sweeps. Rejected.
- **B. Fixed step, per-second units (recommended):** keep the loop as it is; the step's length is `1 / framerate` seconds, and the physics multiplies by it. A game file writes per-second numbers, so 60 or 120 frames a second play at one speed. Tests still count frames, demos stay frame-numbered, and nothing changes about what is repeatable. A step's move is bounded, so the sweeps keep stopping tunnelling as they do today.
- **C. Fixed step, per-frame units kept, a converter at load:** the file says seconds, the loader divides by the frame rate once, and the engine stays in frames. Least engine work, but a variable read or set at run time (`<set variable="ball.velocity.x">`, `pace.beat`) cannot know which units it is in, so every boundary needs a conversion. Rejected for that.
- **Drawing between steps** (interpolating positions for a 144 Hz screen) is separate and optional; B allows it later without changing a game.

## Questions for the author

1. **B, the fixed step with per-second units.** Agreed?
2. **Shares as "kept each second".** `<drag>0.02</drag>` (2% lost each frame) becomes about `0.3` kept each second (0.98 to the 60th power). Is "kept each second" the word, or "lost each second" to stay closer to today's?
3. **Counts on a held key** (Cannon Hill's power, a charge): today `+1` each frame held. Either the `<inc>` stays per frame (it is a count, not a motion), or a held key's numbers become per second (`<inc>30</inc>` adds 30 a second). Recommended: per second, written as today's number times 60, so every held key reads one way.
4. **The frame rate itself.** Once speeds are per second, `<framerate>` only sets smoothness and how fine a step is. Should 60 stay the default, and should the window's refresh pick it?

## How it would be built

One kind of word at a time, each step moving every game and its tests in the same commit, so no file mixes units:

1. The step's length in the engine (`Game::step seconds`), with velocity and acceleration (gravity) per second. Every game's numbers times 60; the tests' expected positions stay, as the frames do.
2. Speeds and speed gained in verbs: `<move>` held, `<thrust>`, `<seek>`, `<pull>`, paths, `<hunt>`/`<go>` pace.
3. Turning, then height.
4. Shares, by the form the author picks.
5. Held-key counts, by the form the author picks.
6. A test that one game played at 120 frames a second is where it is at 60 after the same seconds, within a pixel; `limits.md` loses its line.

Variables that hold speeds (`status.pace`, `pace.speed`) cannot be found by a script: each step greps the games for names read by the words it moves, and the game tests, which check positions after a number of frames, catch any missed.

## Open

- Whether the window's screen refresh should set the frame rate, or the game's `<framerate>` always wins.
- Interpolated drawing, after B.
