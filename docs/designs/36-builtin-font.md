# 36. Built-in font

**Status:** built (`lib/source/builtin_font.cpp`, `lib/include/builtin_font.h`, `tests/test_builtin_font.cpp`)

## Why

Text is drawn with a font file, `assets/tuffy.ttf`, found relative to the directory the program is run from. Run `XGECLI` from anywhere but a directory that has `assets/` next to it and the font is not found. The engine printed a line about it and then drew nothing, so a game loaded and played with every label invisible, which is easy to miss. A game should still be readable when its font is missing, so the engine carries a font inside itself.

## Decision

An 8 by 8 pixel glyph for each printable ASCII character (32 to 126) is stored in the program as a constant table, 760 bytes: eight bytes a character, one per row, with the lowest bit of a row the leftmost pixel. The glyphs are the public-domain `font8x8` set by Daniel Hepper, itself from the IBM VGA font, with the credit kept in the source.

`rasterizeText(text, size, color)` draws a string into a `Bitmap` ([34](34-lunar-lander.md)), the same picture type a sprite of `<line>`s becomes. Each backend, when its font load fails, prints the error once (`error: failed to load font: assets/tuffy.ttf - drawing text with the built-in 8x8 font instead`) and from then on draws every text from that bitmap instead of from the font. So the fallback is one function and one new branch in each of the three backends, and the same code can be tested without a window.

- **Size.** The glyphs are scaled by a whole number, `size / 16` rounded and at least 1, so a `<size>` of 48 is 3x and 128 is 8x, and the pixels stay square. The first version used `size / 8`, which made every letter about twice as wide as the real font's, which averages 0.4 to 0.6 of its size across, and text ran off the screen; a cell of about half the size is the right width. Text is chunky, not smooth; the width of a line is characters times 8 times the scale, so layout that centers with `title.width` still works.
- **Characters.** Anything but printable ASCII is drawn as `?`; a multi-byte UTF-8 character counts as one. A newline starts a new line, a tab is one space.
- **When it is used.** Only when the font file cannot be loaded. With the font in place, nothing changes.

## Options considered

- **Embed `tuffy.ttf` itself** (about 600 KB of source for a 100 KB font, and a generation step in the build, since C++20 has no `#embed`). It would look the same with or without `assets/`, and each backend can load a font from memory, but it is much more to carry and was not what was asked for.
- **An embedded PNG atlas.** Needs an image decoder to read it, in a fallback.
- **Fall back to each library's own default font.** Only raylib has one (an `sf::Font` has no default, SDL_ttf none), so three different looks and one missing.
- **Look for `assets/` next to the program as well.** A separate, smaller fix for the cause rather than the symptom; not done yet, so that running from the wrong directory shows the fallback.

## Approximations

Only ASCII, one style, no kerning, and the line height is the glyph height, so lines of large text touch; there is no way to choose it from a game file. The real font's letters are proportional and the fallback's are not, so a title can be wider or narrower. Checked in the tests and by drawing a sample to a picture; the three window backends were compile-checked only, not run.
