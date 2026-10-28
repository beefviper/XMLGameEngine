# Text and the built-in font

**Status:** Built.

## Text and the built-in font

- Text is drawn by each backend from `assets/tuffy.ttf`, relative to the working directory. Running from the wrong folder used to draw nothing, silently.
- **Fallback:** an 8x8 glyph per printable ASCII (32 to 126), 760 bytes in `picture/font.cpp` (public-domain `font8x8`, credit kept), drawn into a `Bitmap` by `rasterizeText`. When a font load fails each backend prints one error line and draws all text from that bitmap. Size: height scale `size / 16` (min 1), cell 8x that tall, half as wide but never under 8 (size 48 gives 12 by 24). Non-ASCII is `?`; newline starts a line; tab is a space. Text is chunky and monospaced, so width differs from the real font; centering with `title.width` still works. The first version used one scale for both axes and ran off the screen.
- Rejected: embedding `tuffy.ttf` (about 600 KB of source, a generation step since C++20 has no `#embed`), an embedded PNG atlas (needs a decoder in a fallback), each library's default font (only raylib has one).
- **Font files (2026-10-28, the author asked):** a game picks a font file with `<font>` in a text or, usually, its style; the file is the fifth of a text's spriteParams (left off for the standard `assets/tuffy.ttf`, so nothing else changed), and every window backend keeps its fonts by file (SDL2 and OpenGL by file and size), unsmoothed. Shipped: Press Start 2P (`assets/pressstart2p.ttf`, SIL OFL, its licence beside it as `pressstart2p-ofl.txt`, which `--generate` copies with any font it copies), letters 8 by 8 at size 8, used by Mario Bros. and Q*bert (at their own 256 by 224, size 8 and 16), Pac-Man and Donkey Kong. Looked at: switching the built-in font on by name (its letters are stretched half as wide again, it was drawn as a fallback), a bitmap font as rows in the game file (each game standalone would carry the alphabet).
- The fix for the cause rather than the symptom: the data-folder search that puts `assets/` in the working directory ([cli](52-cli.md)).


## Styles and centering

- Need (2026-10-16): a text took about ten lines (sprite, text, content, size, color, position), its size and color repeated across a game's title, pause and end screens, and 209 of the games' positions said `window.width.center - name.width / 2`, naming the object inside its own position.
- **Styles, chosen:** a game-level `<styles>` of `<style name>`s, each an optional `<size>` and `<color>`; `<text style="...">` takes what it leaves out, and its own wins. A style's size is worked out with the game's names, after `layOutScenes`, so a scene's variable of the same name does not change it. Resolved in shared C++ (`applyStyles`), so the generator only writes the style down. Since 2026-10-28 a style also holds a bitmap's scale and colors ([pictures](30-pictures.md)). Looked at: a style on the state or scene (each game file is one standalone screen set, and one list is enough), and a style holding a position too (a place is not a look).
- **`<centered>`, chosen:** a value tag of a position only, holding where the middle goes (empty: the window's middle on that axis). `centerOnOwnSize` turns it into `(where) - name.width / 2` once the object's name is final, so groups, members and scene changes need nothing of their own. Looked at: a `centered` keyword in the expression (a string to parse), an anchor or origin per object (the parked "anchors" idea in [values-and-names](04-values-and-names.md); centering is the common case), and `self.width` (a name whose meaning changes with the object).
- The shipped games were rewritten on both (styles named by role: `banner`, `prompt`, `help`, `keys`, `hud`). Pong's and Breakout's titles keep their `<formula>`s as the examples of that tag.
- A centered text that changes width (a score gaining a digit) is centered again, as any position naming a size is (`resolveSizeDependentPositions`).


## Words (2026-10-17)

- Need: the author asked for "some stuff for strings and words": tennis's LOVE 15 30 40, a dragon asleep or awake, a rank's name.
- **Chosen: a `<choose>` piece of a `<text>`**, `<by>` a value then its `<word>`s, showing the word the number picks (the first for 0; a fraction goes down; past either end the end word), live like `<number>`. The words stay in the file as elements; nothing is packed in a string.
- Looked at: string variables (a second kind of value through exprtk and every command), a game-level list of words named and indexed (one more thing to name for one text), formatting in the content (`{score}`; a string to parse).
- Open: changing the words of a `<content>` at run time other than by a `<choose>` or a look.
