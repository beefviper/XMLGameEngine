# Sounds

Sounds are written as notes, the bleeps and tunes of an 8-bit sound chip; nothing is loaded from a sound file. A [`<play>`](commands.md#look-and-sound) or a state's [`<music>`](states.md#states) starts one.

```xml
<sounds>
  <sound name="wall" wave="square">
    <note pitch="A3">0.04</note>
  </sound>
  <sound name="gameover" wave="square">
    <volume>0.4</volume>
    <note pitch="G4">0.2</note>
    <note pitch="E4" to="D#4">0.5</note>       <!-- slides down -->
    <rest>0.1</rest>
    <note pitch="2000" wave="noise">0.15</note>
  </sound>
  <sound name="thrust" wave="noise">
    <loop />                                   <!-- until silenced -->
    <note pitch="150">1</note>
  </sound>
  <sound name="start">
    <part wave="square"> <note pitch="C5">0.1</note> ... </part>   <!-- a tune -->
    <part wave="triangle"> <note pitch="C3">0.2</note> ... </part> <!-- over its bass -->
  </sound>
</sounds>
```

## Parts of a sound

| Part | Meaning |
|---|---|
| `wave=` | `square` (default), `triangle`, `sawtooth`, `sine` or `noise` (a high pitch hisses, a low one rumbles). On a sound, part or note; the nearest wins |
| `<loop />` | first: plays over and over ([playing](#playing)) |
| `<volume>` | a value from 0 to 1; default 0.3 |
| `<note pitch to wave>seconds</note>` | `pitch` is a note and octave (`C4` middle C, `A4` 440 Hz, `F#3`, `Bb5`) or hertz; `to` slides there evenly in semitones. Length above 0, at most 10 |
| `<rest>seconds</rest>` | silence |
| `<part wave>` | a voice of its own: a sound's parts play at once. Parts add up, so keep the volume down. Parts or loose notes, not both |

## Playing

- **Loops.** A `<play>` of a looping sound runs it until a `<silence sound>`, a reset of the game, or (when the `<play>` is on a key) that key being let go: Asteroids' engine hums while thrust is held. A state change lets go of the keys, so a pause is quiet.
- **One voice per sound**, like a chip's channel: playing one that is playing starts it again; different sounds overlap; one asked for twice in a frame plays once.
- Every sound is made into samples by the engine (16 bit, mono, 44100 a second) when the window opens, the same for every sound library, with a short fade so notes do not click.
- These stop the load, saying where: an unusable wave, pitch or length, a volume outside 0 to 1, a loop after anything, a sound with no notes, two sounds of one name, a `<play>`, `<silence>` or `<music>` naming no sound.
