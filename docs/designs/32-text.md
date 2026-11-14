# Text and the built-in font

**Status:** Built.

## Text and the built-in font

- Text is drawn by the engine from a font file (`drawText`, `picture/font.cpp`), `assets/tuffy.ttf` unless a `<font>` says otherwise (below), relative to the working directory. Running from the wrong folder used to draw nothing, silently.
- **Drawn by the engine, not the window library:** each library's own font code (SFML, SDL2_ttf, raylib, Qt) drew text before, and widths differed between windows and from the built-in font. Now every window shows the same bitmap, and a text's size needs no window.
  - plutovg draws it: already linked for SVG, it reads fonts with stb_truetype and fills glyphs with a rasterizer taken from FreeType's, smoothed.
  - Rejected: stb_truetype alone (the same reader, but glyph filling to write), FreeType or HarfBuzz (hinting and shaping, not needed for score text, at the cost of large dependencies). The cost: no hinting, kerning or shaping ([limits](../reference/limits.md)).
  - The bitmap is the box the ink covers, spaces at the ends counted across, as the libraries measured it, so layouts made against them hold. Edges within a 64th of a pixel of a whole one count as on it, so Press Start 2P at multiples of 8 has no part-covered pixel.
  - In a window bigger than the game, text is drawn again at the window's size once a resize settles, so it stays sharp ([backends](50-backends.md#scale)).
- **Fallback:** an 8x8 glyph per printable ASCII (32 to 126), 760 bytes in `picture/font.cpp` (public-domain `font8x8`, credit kept), drawn into a `Bitmap` by `rasterizeText`.
  - When a font load fails the engine prints one error line and draws that font's text from that bitmap.
  - Size: height scale `size / 16` (min 1), cell 8x that tall, half as wide but never under 8 (size 48 gives 12 by 24).
  - Non-ASCII is `?`; newline starts a line; tab is a space.
  - Text is chunky and monospaced, so width differs from the real font; centering with `title.width` still works.
  - The first version used one scale for both axes and ran off the screen.
- Rejected: embedding `tuffy.ttf` (about 600 KB of source, a generation step since C++20 has no `#embed`), an embedded PNG atlas (needs a decoder in a fallback), each library's default font (only raylib has one).
- **Font files:** a game picks a font file with `<font>` in a text or, usually, its style; the file is the text's `Picture::font` (empty for the standard `assets/tuffy.ttf`), opened once and kept by file.
  - Shipped: Press Start 2P (`assets/pressstart2p.ttf`, SIL OFL, its licence beside it as `pressstart2p-ofl.txt`, which `--generate` copies with any font it copies), letters 8 by 8 at size 8, used by Mario Bros. and Q*bert (at their own 256 by 224, size 8 and 16), Pac-Man and Donkey Kong.
  - Looked at: switching the built-in font on by name (its letters are stretched half as wide again, it was drawn as a fallback), a bitmap font as rows in the game file (each game standalone would carry the alphabet).
- The fix for the cause rather than the symptom: the data-folder search that puts `assets/` in the working directory ([cli](52-cli.md)).


## Styles and centering

- Need: a text took about ten lines (sprite, text, content, size, color, position), its size and color repeated across a game's title, pause and end screens, and 209 of the games' positions said `window.width.center - name.width / 2`, naming the object inside its own position.
- **Styles, chosen:** a game-level `<styles>` of `<style name>`s, each an optional `<size>` and `<color>`; `<text style="...">` takes what it leaves out, and its own wins.
  - A style's size is worked out with the game's names, after `layOutScenes`, so a scene's variable of the same name does not change it.
  - Resolved in shared C++ (`applyStyles`), so the generator only writes the style down.
  - Now a style also holds a bitmap's scale and colors ([pictures](30-pictures.md)).
  - Looked at: a style on the state or scene (each game file is one standalone screen set, and one list is enough), and a style holding a position too (a place is not a look).
- **`<centered>`, chosen:** a value tag of a position only, holding where the middle goes (empty: the window's middle on that axis).
  - `centerOnOwnSize` turns it into `(where) - name.width / 2` once the object's name is final, so groups, members and scene changes need nothing of their own.
  - Looked at: a `centered` keyword in the expression (a string to parse), an anchor or origin per object (the parked "anchors" idea in [values-and-names](04-values-and-names.md); centering is the common case), and `self.width` (a name whose meaning changes with the object).
- The shipped games were rewritten on both (styles named by role: `banner`, `prompt`, `help`, `keys`, `hud`). Pong's and Breakout's titles keep their operations as the examples of those tags.
- A centered text that changes width (a score gaining a digit) is centered again, as any position naming a size is (`resolveSizeDependentPositions`).

- **Lines of text** (the author: help lines were each an object repeating their style and centring): a group in `<rows>` alone, the group's `<text style>` with no words, each row its words; cells named by row (`help.2`).
  - Text rows are their padding apart top to top (chosen when only a window could measure text, and kept: lines of words read best spaced evenly); a row's padding is a paragraph step.
  - Looked at: a `<line>` piece inside one `<text>` (every window backend, the 8 by 8 font and the GUI would have to centre each line themselves), and members stacking by the group's padding (a second meaning for padding).
  - The games' title help lines moved onto it; lines of different styles (a banner, the start prompt) stay objects.


## Words

- Need: the author asked for "some stuff for strings and words": tennis's LOVE 15 30 40, a dragon asleep or awake, a rank's name.
- **Chosen: a `<choose>` piece of a `<text>`**, `<by>` a value then its `<word>`s, showing the word the number picks (the first for 0; a fraction goes down; past either end the end word), live like `<number>`. The words stay in the file as elements; nothing is packed in a string.
- Looked at: string variables (a second kind of value through the arithmetic and every command), a game-level list of words named and indexed (one more thing to name for one text), formatting in the content (`{score}`; a string to parse).
- Open: changing the words of a `<content>` at run time other than by a `<choose>` or a look.
