# 09. Sound

**Status:** built (`sound.h`, `audio.h`, `audio_*.cpp`, Pong's five sounds). Not listened to on a real sound card or on Windows.

## What we wanted

The 8-bit machines these games come from played bleeps, bloops and little tunes from a sound chip, not recordings. A sound written as notes in the game file fits the format the way `<bitmap>` does: part of the description, readable, diffable, no file beside it.

## The language

A game has an optional `<sounds>` after `<variables>`. Each `<sound name wave>` has an optional `<volume>` (0 to 1, a value, default 0.3), then `<note>`s and `<rest>`s in order. `<note pitch to wave>seconds</note>`: `pitch` is a note name with octave (`C4` is middle C, `A4` 440 Hz, `F#3`, `Bb5`) or hertz; `to` slides evenly in semitones; `wave` is `square` (default), `triangle`, `sawtooth`, `sine`, `noise`. `<play sound="name" />` is an ordinary command (collision, action, key, condition, timer). Length and volume are values (a variable can set the tempo); `pitch`, `to` and `wave` pick something, so they are attributes ([01](01-vision-and-format.md)).

## Decisions

- **Notes, not sound files.** Files need a loader per library, shipping, and are opaque in the XML. A file tag can come later beside notes.
- **The engine makes the samples; a backend only plays them.** `synthesize()` (`sound.cpp`) produces 16-bit mono at 44100 Hz, the same for every library; a backend hands them over (`sf::SoundBuffer`, raylib `LoadSoundFromWave`, an SDL audio callback with a small mixer of its own). Each library's own synthesis does not exist in SFML or SDL2, and would make a sound depend on the library. The synthesizer is tested with no sound device.
- **Waves of a chip:** square, triangle, sawtooth tone channels; noise as percussion, pitched like a chip (a new random level every half cycle, seeded per note, so it sounds the same every time and in every library); sine kept as a soft option. Plucked string, piano and horn from the author's waveform tester were left out as not of the era.
- **Slides by semitones** (exponential in hertz) so a bend sounds even; phase accumulates per sample.
- **A short fade (about 2 ms), no envelope.** An attack/decay/release would be a child of `<sound>` or `<note>`.
- **One voice per sound,** like a chip channel: replaying restarts it, different sounds overlap. A sound asked for twice in one frame plays once. A voice pool would let a sound overlap itself; not needed.
- **The game asks, the engine plays.** `<play>` queues a name (`Game::requestSound`); `Engine` owns the `Audio` and plays the queue at the end of `step()`. `Game` never touches audio, so the tests drive whole games with no device and read the requests (`takeSoundRequests()`). `Engine(game, window)` with no audio is `NullAudio`.
- **Checked at load,** with messages that say where: unusable wave, pitch or length, volume outside 0 to 1, a sound with no notes, duplicate names, `<play>` naming no sound. A note or rest is at most 10 seconds.
- **The sound library is its own choice** (`AudioBackend`: SFML3, Raylib, SDL2, None; `-a`/`--audio`, an Options entry), not tied to the window, so an OpenGL window (GLFW has no sound) can have sound, and `-a none` plays silently. Default SFML 3.
- **No sound is not an error:** a window that will not open stops the game; a sound device that will not open prints a warning and uses `NullAudio` (XGEGUI says so and sets the choice to None).
- **SDL is shared** by `SDL2Window` and `SDL2Audio`: each starts and stops only its own subsystem (`SDL_InitSubSystem`/`QuitSubSystem`), the last one out calls `SDL_Quit()`. raylib opens its sound device apart from its window. SFML 3 and raylib both carry miniaudio and `stb_image` (link clashes when both are built static from source; [11](11-backends-build-and-layout.md)).
- **Pong is the test:** low square blip (A3) off walls, higher (A4) off a paddle, a falling triangle bloop for a point, a rising arpeggio on start, a falling four-note tune with a hiss of noise when someone reaches 15.

## Rejected

Sound files as the only form; a tracker or MML string (`"t120 o4 c8 e8 g8"`: a little language inside a string, which [01](01-vision-and-format.md) took out); each library's own synthesis; a voice pool; sound tied to the window library.

## Not done

Looping music behind a state, stopping a sound from the game file, chords inside one sound, a volume envelope, stereo/panning, a sound-file tag, notes that follow a variable at runtime (a sound is made once when the window opens), sounds for the other games (Breakout's bricks, Invaders' march and shots, Asteroids' thrust and explosions are the obvious ones).
