# 25. Targets and capability profiles

**Status:** idea (long term); related to [15](15-backend-abstraction.md)

## The idea

The engine already chooses window and XML backends at build or start time. A bigger version of the same thought: one game description, several *targets*, each with its own limits, and the toolchain does its best on each and tells the author what it had to give up.

The comparison that started it: a hobby toolkit for 8-bit machines writes the game logic once and swaps in target-specific code for screen setup, input, graphics, sound and start-up, so the same source builds for several old computers and consoles. The declarative version of that idea is stronger, because the description has no machine code in it at all.

## What a target would carry

A capability profile: resolution, color count and palette rules, sprite count per frame and per scanline, sprite size and colors per sprite, audio channels, input devices, memory. A loader then compares the game to the profile.

## Degradation reports

The output would not fail silently. Examples of the kind of messages the discussion imagined:

- too many objects on screen for this target; some would be dropped or flickered;
- a sprite uses more colors than allowed; it was reduced;
- a file named here could be recreated by hand if the reduced version is not acceptable.

That list of *which assets to redo* is the useful part: the tool approximates, then says exactly what to fix.

## Hardware tricks as a design lesson

Old hardware limits produced techniques the language may want to be able to say, or at least to know about when it targets those machines:

- Reusing the sprite budget mid-frame (draw the top half, then reposition the same sprites for the bottom half) to double what appears to be on screen.
- Using a hit between a marked sprite and the background as a timing signal to split the screen, so a status bar stays fixed while the playfield scrolls ([26](26-scrolling-and-screen-regions.md)).
- Scanline interrupts on some cartridges for the same purpose.

These are *implementation* techniques. The declarative layer would say there are two regions and a limit; a target would decide how.

## Null and fallback backends

Alongside real backends: a software renderer as the last fallback, and null implementations of graphics, audio and input, so the rest of the engine never checks whether a subsystem exists. A null audio backend makes the play-a-sound call always safe. A null input can be scripted, which is the basis for tests and for an AI player ([20](20-ideas-parking-lot.md)).

## Data-driven machine descriptions

A related idea from emulator design: keep a system's instruction or capability table as data, translate it once at start-up into a fast structure (an array of handlers), and only then run. Loading a small table costs almost nothing; what matters is that nothing stays a string lookup in the hot path. The same structure fits a target profile: parse, resolve, then run without further parsing ([16](16-code-layout-and-pipeline.md)).

## Open

- Whether targets belong in the XML at all, or in a separate profile file.
- How much of a degradation report is possible without running the game.

## Sources

- "Chibi Akumas vs XMLGameEngine" (2026-07-22): multi-target idea and degradation messages.
- "Engine Abstraction Design" (2026-07-20): null and software fallbacks, subsystem list.
- "Sprite Multiplexing on NES" (2025-03-30) and "Sprite 0 hit effect" (2025-08-18): hardware techniques.
- "Data-driven tables versus code" (2025-09-04): tables resolved at load.
