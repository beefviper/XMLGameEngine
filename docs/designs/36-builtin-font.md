# 36. Built-in font

**Status:** built (`lib/source/builtin_font.cpp`, `lib/include/builtin_font.h`, `tests/test_builtin_font.cpp`)

## Why

Text is drawn with a font file, `assets/tuffy.ttf`, found relative to the directory the program is run from. Run `XGECLI` from anywhere but a directory that has `assets/` next to it and the font is not found. The engine printed a line about it and then drew nothing, so a game loaded and played with every label invisible, which is easy to miss. A game should still be readable when its font is missing, so the engine carries a font inside itself.

## Decision

An 8 by 8 pixel glyph for each printable ASCII character (32 to 126) is stored in the program as a constant table, 760 bytes: eight bytes a character, one per row, with the lowest bit of a row the leftmost pixel. The glyphs are the public-domain `font8x8` set by Daniel Hepper, itself from the IBM VGA font, with the credit kept in the source.

`rasterizeText(text, size, color)` draws a string into a `Bitmap` ([34](34-lunar-lander.md)), the same picture type a sprite of `<line>`s becomes. Each backend, when its font load fails, prints the error once (`error: failed to load font: assets/tuffy.ttf - drawing text with the built-in 8x8 font instead`) and from then on draws every text from that bitmap instead of from the font. So the fallback is one function and one branch in each backend (the three there were then; the OpenGL backend and XGEGUI's Qt renderer, added later, do the same), and the same code can be tested without a window.

- **Size.** The height and the width are scaled separately. The height scale is `size / 16` rounded and at least 1, and a character's cell is 8 times that tall, so a `<size>` of 48 is a cell 24 tall and 128 is 64 tall. The cell is half as wide as it is tall (4 pixels for each step of the scale) but never under 8, the width of a glyph: sizes under 32 are 8 by 8, 32 is 8 by 16, 48 is 12 by 24, 64 is 16 by 32, 128 is 32 by 64. The real font's letters average 0.4 to 0.6 of the size across and about the size tall, so the cell is about its shape. The first version used one scale for both, `size / 8`, then `size / 16`, and the letters were too wide and ran off the screen. Where the width is an odd multiple of 4 (12 at size 48) the columns of a glyph are alternately 1 and 2 pixels wide, so they still add up to the cell. Text is chunky, not smooth; the width of a line is the characters times the cell width, so layout that centers with `title.width` still works.
- **Characters.** Anything but printable ASCII is drawn as `?`; a multi-byte UTF-8 character counts as one. A newline starts a new line, a tab is one space.
- **When it is used.** Only when the font file cannot be loaded. With the font in place, nothing changes.

## Options considered

- **Embed `tuffy.ttf` itself** (about 600 KB of source for a 100 KB font, and a generation step in the build, since C++20 has no `#embed`). It would look the same with or without `assets/`, and each backend can load a font from memory, but it is much more to carry and was not what was asked for.
- **An embedded PNG atlas.** Needs an image decoder to read it, in a fallback.
- **Fall back to each library's own default font.** Only raylib has one (an `sf::Font` has no default, SDL_ttf none), so three different looks and one missing.
- **Look for `assets/` next to the program as well.** A separate, smaller fix for the cause rather than the symptom; not done yet, so that running from the wrong directory shows the fallback.

## Approximations

Only ASCII, one style, no kerning, and the line height is the cell height (each glyph leaves its last row empty, except for tails, so lines are one row apart); the width and height of a letter cannot be chosen separately from a game file, only from `<size>`. The real font's letters are proportional and the fallback's are not, so a title can be wider or narrower. Checked in the tests and by drawing a sample to a picture; the three window backends were compile-checked only, not run.
