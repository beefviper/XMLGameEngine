# Sound

**Status:** Built (`audio/sound.h`, `audio/audio.h`, a file per library). Reference: [sounds](../reference/sounds.md).

## What we wanted

The 8-bit machines played bleeps, bloops and little tunes from a sound chip, not recordings. A sound written as notes in the game file fits the format as `<bitmap>` does: part of the description, readable, diffable, no file beside it.

## Decisions

- **Notes first, recordings beside them.** Notes are part of the description and readable; a file is opaque. A recording is a `<path>` in a `<sound>`, written as a sprite names an image, in place of notes or parts.
  - The engine decodes it (`audio/wave.cpp`) into the same samples a synthesized sound is, so no library has a loader of its own and nothing after `samplesOf()` knows the difference.
  - Uncompressed WAV only: it needs no library. MP3 or Ogg would each need a decoder, which is the author's call.
  - Stereo is averaged into mono, as every sound is mono; another rate is resampled along straight lines. At most 60 seconds, so a wrong file is a load error and not memory filled.
  - A recording plays as made (volume 1) when no `<volume>` is given, where notes default to 0.3: a recording was already mixed, a full-scale square wave is harsh.
  - The file is read once, at load, by the game's `Resources`: a bad one stops the load before a window opens, and the audio library plays those samples.
- **The engine makes the samples; a backend only plays them.** `samplesOf()` is the one place a sound becomes samples (today by `synthesize()`): 16-bit mono at 44100 Hz, the same for every library, made once a game by `Resources::samples` (sounds that sound the same share them) when a library asks, so a game played with no sound makes none; SFML and SDL2 have no synthesis of their own anyway. The synthesizer is tested with no device.
- **A chip's waves:** square, triangle, sawtooth; noise pitched like a chip (a new random level every half cycle, seeded per note, so it sounds the same everywhere); sine as a soft option.
- **Slides by semitones**, so a bend sounds even.
- **Looping is the sound's, holding is the key's.** `<loop />` sits on the `<sound>`; a key's `<play>` of a loop is held like a `<move>`, on with the press and off with the release.
  - Elsewhere `<play>` turns it on and `<silence>` off.
  - A loop is state, not an event: `Engine` compares the set that is on with what it told the `Audio` each frame, so silencing and asking again in one frame does not restart it.
  - `<silence>` was chosen over `<stop>` (an object's verb).
- **Music is parts and a state's own sound.** A `<sound>` of `<part>`s plays them at once, as a chip's channels did; a sum past full scale is cut off, with no automatic scaling, so `<volume>` stays the author's.
  - `<music sound>` on a state loops while it is current, so a pause silences it with nothing written on the pause key; one that does not loop plays as the state begins.
  - Rejected: a `<music>` element of its own (a tune is a sound), beats as lengths (a length is already a value), a tracker pattern table (a little language), music resuming where it paused (each library would need a pause).
- **A short fade (about 2 ms), and one part of an envelope: decay.** `<decay>` on a sound or a part is the seconds each note takes to fall evenly to nothing from its start, for plucks and bells (Pinball's bumpers ring). It belongs to the voice, not the note, so a note stays one number of seconds (no mixed text and tags). Attack, sustain and release, the other three of the usual four, wait for a game that needs them.
- **One voice per sound,** like a chip channel: replaying restarts it; different sounds overlap.
- **The game asks, the engine plays.** `<play>` queues a name; `Engine` plays the queue at the end of `update()`. `Game` never touches audio, so tests read the requests.
- **Checked at load** with messages that say where. A note is at most 10 seconds.
- **The sound library is chosen apart from the window,** so an OpenGL window (GLFW has no sound) can have sound. **No sound is not an error:** a device that will not open gives a warning and silence.
- **SDL is shared** by window and sound: each starts and stops only its own subsystem.
- **A shot's sound sits after its `<fire>`,** so a gun that is not ready stays silent ([input](08-input.md#a-press-that-cannot-be-done)).

## Notes that follow a number

The arcade Invaders thump once per step of the fleet, so the march quickens as the fleet does. A fixed `<music>` loop drifts apart from a fleet that speeds up, and every sound is made once, when the window opens. Three ways were weighed:

- **A. `<play>` gives a pitch.** `<play sound="thump"><pitch>march.note</pitch></play>`: the engine makes that sound again at the asked pitch (kept once made). Covers a siren rising with speed. Cost: sounds made during play, and the tempo still needs something to play it on time.
- **B. One sound at several pitches, picked by a number.** `<sound name="thump"><pitches><pitch>F2</pitch><pitch>E2</pitch>...</pitches>` made at load; `<play sound="thump"><by>march.step</by></play>` picks one, as `<choose><by>` picks a word. Nothing is made during play. Same tempo problem as A.
- **C. The author's, chosen and built: sound in step with the picture.** Two small, general words:
  - an animation's `<seconds update="live">`, worked out every frame like any [live number](../reference/format.md#live-numbers-of-an-object);
  - a `<frame>` that holds a `<play>`, run as that picture comes up.

  Invaders' aliens become four frames (`a`, `b`, `a`, `b`), each playing its note, with `<seconds update="live">pace.beat</seconds>`, which the speed-up condition shortens as it speeds the fleet. The march then quickens with the fleet, as the arcade's does, with no tempo word and no sound made in play. It also gives footsteps on a walk and a flap on each wingbeat.

A may come later, only if a game needs a pitch that slides with a number. Open: whether the fleet's own steps (a hop per frame of the animation instead of a velocity) should be the third piece, so picture, sound and step all keep one time.

## Rejected

Sound files as the only form; a loader per library; a tracker or MML string (`"t120 o4 c8 e8 g8"`, a little language in a string); each library's own synthesis; a voice pool; sound tied to the window library.

## Open

- The rest of an envelope (attack, sustain, release); a curved rather than straight decay; stereo (a mono sound would play the same on both sides); compressed recordings.
- Silencing a state's music while it is current.
- A faint pulse where a loop's end meets its start at the fades.
