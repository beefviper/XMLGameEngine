# 43. Sound

**Status:** built (`lib/include/sound.h`, `lib/include/audio.h`, `lib/source/sound.cpp`, `lib/source/audio*.cpp`, `games/pong.xml`, `tests/test_sound.cpp`); new tags: `<sounds>`, `<sound>` (with `<volume>`, `<note>`, `<rest>`), and the command `<play sound="..." />`; a new backend kind, `Audio`, chosen with `-a` / `--audio` and in XGEGUI's Options dialog

## Why

Every game so far was silent. The machines these games come from were not: a bounce was a blip, a point a bloop, the end of a game a little tune, and all of it came from a sound chip playing notes, not from recordings. A sound written as notes in the game file fits the format the same way a `<bitmap>` does ([42](42-bitmap-sprites-and-animation.md)): it is part of the description, readable and diffable, with no file next to it.

## The language

A game has an optional `<sounds>` after its `<variables>`. Each `<sound name="...">` has an optional `<volume>` (a value, 0 to 1) and then `<note>`s and `<rest>`s, played one after the other:

```xml
<sound name="score" wave="triangle">
  <volume>0.4</volume>
  <note pitch="E5" to="E3">0.35</note>
</sound>
```

A note's content is how long it lasts, in seconds; `pitch` is a note name with an octave (`C4` is middle C, `A4` 440 Hz, `F#3`, `Bb5`) or a number of hertz; `to` is a pitch to slide to; `wave` picks `square` (the default), `triangle`, `sawtooth`, `sine` or `noise`, on the sound for all its notes or on one note for itself. A rest's content is seconds of silence. `<play sound="score" />` is a command like any other, and works in a collision rule, an object's action, a key's input or a condition.

This follows the format's rule ([03](03-expression-syntax.md)): `pitch`, `to` and `wave` pick something (a pitch is a name on a keyboard), and the length and volume are values in content, so a variable can set the tempo (`<note pitch="C5">beat / 2</note>`).

## Decisions

- **Notes, not sound files.** The request was for the bleeps and bloops of an 8-bit machine and simple tunes; a file format would bring a loader per library, files to ship and find, and nothing to read in the XML. A sound file tag can come later beside notes.
- **The engine makes the samples; a backend only plays them.** `synthesize()` (`sound.cpp`) turns a sound into 16-bit mono samples at 44100 a second, the same for every library, and a backend hands them to its library (`sf::SoundBuffer`, raylib's `LoadSoundFromWave`, an SDL audio callback). The alternative, each library's own tone generator, does not exist in SFML or SDL2, and would make a sound depend on the library. The synthesizer is tested without any sound device.
- **The waves of a sound chip.** Square, triangle and sawtooth are the tone channels of the era, noise its percussion; sine was kept from the waveform tester the engine's synthesizer started from, as a soft option. Noise is pitched the way a chip's noise channel is: a new random level every half cycle, held in between, so a high pitch hisses and a low one rumbles. Its random numbers are seeded per note, so a noise sounds the same every time and in every library. The tester's plucked string, piano and horn (additive and Karplus-Strong synthesis) were left out as not of the era; they would be more `wave` names.
- **Slides by semitones.** `to` slides the pitch evenly in semitones (exponentially in hertz), which is how a pitch bend sounds even; the phase is added up a sample at a time so a slide stays smooth.
- **A short fade, no envelope.** Every note fades in and out over about 2 ms (less for a very short note) so it starts and stops without a click. A real attack, decay and release was not needed for blips; it would be a child of `<sound>` or `<note>`.
- **One voice per sound.** Like a channel of a sound chip: playing a sound that is still playing starts it again, and different sounds play over each other. A pool of voices per sound would let a sound overlap itself, which a fast ball against a wall does not need and which gets loud.
- **The game asks; the engine plays.** `<play>` only queues the name on `Game` (`requestSound`); `Engine` owns the `Audio` and plays the queue at the end of `step()`, after keys, collisions and conditions. `Game` and everything under it never touch audio, so the tests drive whole games with no sound device, and can read what was asked for. A sound asked for twice in one frame (two bricks at once) plays once.
- **Checked when the game loads.** A wave, pitch or length the engine cannot use, a volume outside 0 to 1, a sound with no notes, two sounds of one name, and a `<play>` naming no sound are load errors that say where, like a `<push>` naming no state. A note or rest is at most 10 seconds, so a slip (60 for 0.60) is an error rather than a minute of noise held in memory.
- **The sound library is its own choice.** `AudioBackend` (SFML3, Raylib, SDL2, None) is picked apart from `WindowBackend`, so the OpenGL window (GLFW has no sound) can have sound, and a game can be played silently anywhere (`-a none`). The default is SFML 3, like the window's.
- **No sound is not an error.** A window that will not open stops the game; a sound device that will not open does not. `Engine` prints a warning and uses `NullAudio`; XGEGUI says so in a message and sets the choice to None.
- **SDL2 shares SDL.** `SDL2Window` used to call `SDL_Quit()` when it closed, which would stop the SDL2 sound too. Both now start and stop only their own part (`SDL_InitSubSystem` / `SDL_QuitSubSystem`) and the last one out calls `SDL_Quit()`.
- **raylib's device is its own.** raylib opens its sound device apart from its window (`InitAudioDevice`), so its sound goes with any window, raylib's included.

## Pong as the test

Pong has five sounds: a low square blip (A3) off the top and bottom walls, a higher one (A4) off a paddle, a triangle bloop falling two octaves for a point, a rising arpeggio when a game starts from the menu, and a falling four-note tune with a hiss of noise at the end when someone reaches 15. Each is one `<play>` added to a rule Pong already had. The tests play Pong frame by frame and check that each kind of bounce and a point ask for their sound once, and that `Engine` hands them to its `Audio`.

On Linux the three libraries were run under a virtual X server, with Space pressed by `xdotool` and the sound recorded (SDL2 through its disk driver, SFML 3 and raylib through a PulseAudio null sink): each played the start arpeggio, wall and paddle blips at their pitches, and the score bloop. Nobody has listened to it on a real sound card yet.

## Options considered

- **Sound files** (`<sound file="bounce.wav">`): what most engines do; not what was asked for, and opaque in the XML. Could live beside notes later.
- **A tracker or MML string** (`"t120 o4 c8 e8 g8"`): compact for music, but a little language inside a string, which [03](03-expression-syntax.md) took out of the format.
- **Each library's own synthesis**: SFML and SDL2 have none, and the same game would sound different per library.
- **A voice pool per sound**: overlap of one sound with itself; not needed yet.
- **Sound tied to the window library** (SFML window means SFML sound): simpler to choose, but OpenGL has no sound, and the point of the backends is that they are interchangeable.

## Not done

Music that loops behind a state, stopping a sound from the game file, chords within one sound, a volume envelope, stereo and panning, a sound file tag, notes that follow a variable while the game runs (a sound is made once, when the window opens), and the other shipped games' sounds. Not listened to on Windows.
