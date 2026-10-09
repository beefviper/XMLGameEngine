# Text and the built-in font

**Status:** Built.

## Text and the built-in font

- Text is drawn by each backend from `assets/tuffy.ttf`, relative to the working directory. Running from the wrong folder used to draw nothing, silently.
- **Fallback:** an 8x8 glyph per printable ASCII (32 to 126), 760 bytes in `builtin_font.cpp` (public-domain `font8x8`, credit kept), drawn into a `Bitmap` by `rasterizeText`. When a font load fails each backend prints one error line and draws all text from that bitmap. Size: height scale `size / 16` (min 1), cell 8x that tall, half as wide but never under 8 (size 48 gives 12 by 24). Non-ASCII is `?`; newline starts a line; tab is a space. Text is chunky and monospaced, so width differs from the real font; centering with `title.width` still works. The first version used one scale for both axes and ran off the screen.
- Rejected: embedding `tuffy.ttf` (about 600 KB of source, a generation step since C++20 has no `#embed`), an embedded PNG atlas (needs a decoder in a fallback), each library's default font (only raylib has one).
- The fix for the cause rather than the symptom: the data-folder search that puts `assets/` in the working directory ([cli](cli.md)).
