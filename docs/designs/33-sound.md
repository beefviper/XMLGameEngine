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
  - The file is read once at load to check it, so a bad one stops the load before a window opens.
- **The engine makes the samples; a backend only plays them.** `samplesOf()` is the one place a sound becomes samples (today by `synthesize()`): 16-bit mono at 44100 Hz, the same for every library; SFML and SDL2 have no synthesis of their own anyway. The synthesizer is tested with no device.
- **A chip's waves:** square, triangle, sawtooth; noise pitched like a chip (a new random level every half cycle, seeded per note, so it sounds the same everywhere); sine as a soft option.
- **Slides by semitones**, so a bend sounds even.
- **Looping is the sound's, holding is the key's.** `<loop />` sits on the `<sound>`; a key's `<play>` of a loop is held like a `<move>`, on with the press and off with the release.
  - Elsewhere `<play>` turns it on and `<silence>` off.
  - A loop is state, not an event: `Engine` compares the set that is on with what it told the `Audio` each frame, so silencing and asking again in one frame does not restart it.
  - `<silence>` was chosen over `<stop>` (an object's verb).
- **Music is parts and a state's own sound.** A `<sound>` of `<part>`s plays them at once, as a chip's channels did; a sum past full scale is cut off, with no automatic scaling, so `<volume>` stays the author's.
  - `<music sound>` on a state loops while it is current, so a pause silences it with nothing written on the pause key; one that does not loop plays as the state begins.
  - Rejected: a `<music>` element of its own (a tune is a sound), beats as lengths (a length is already a value), a tracker pattern table (a little language), music resuming where it paused (each library would need a pause).
- **A short fade (about 2 ms), no envelope.**
- **One voice per sound,** like a chip channel: replaying restarts it; different sounds overlap.
- **The game asks, the engine plays.** `<play>` queues a name; `Engine` plays the queue at the end of `step()`. `Game` never touches audio, so tests read the requests.
- **Checked at load** with messages that say where. A note is at most 10 seconds.
- **The sound library is chosen apart from the window,** so an OpenGL window (GLFW has no sound) can have sound. **No sound is not an error:** a device that will not open gives a warning and silence.
- **SDL is shared** by window and sound: each starts and stops only its own subsystem.
- **A shot's sound sits after its `<fire>`,** so a gun that is not ready stays silent ([input](08-input.md#a-press-that-cannot-be-done)).

## Rejected

Sound files as the only form; a loader per library; a tracker or MML string (`"t120 o4 c8 e8 g8"`, a little language in a string); each library's own synthesis; a voice pool; sound tied to the window library.

## Open

- A volume envelope (decay for plucked and bell sounds); stereo (a mono sound would play the same on both sides); compressed recordings.
- Notes that follow a variable: a sound is made once when the window opens, so Invaders' march cannot keep time with the fleet. A timer stepping through sounds would.
- Silencing a state's music while it is current.
- A faint pulse where a loop's end meets its start at the fades.
