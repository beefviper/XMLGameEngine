# Pictures

**Status:** Built: lines, polygons, bitmaps with inks and styles, palette swaps, SVG parts, picture sheets, tethers, flips, turning, holes, light and dark. Reference: [pictures](../reference/pictures.md).

## The one decision behind all of it

**The engine draws every picture it can, once, at load, into its own `Bitmap`; backends only show that bitmap.** One path, `buildPicture` (`load/expr.cpp`): read, draw, flip, keep a `Turnable` for objects with a heading. What an object shows is a typed `Picture`, never strings a backend parses. So:

- adding a picture kind changes no window backend, and every library shows the same pixels;
- an object's size is known without a window (`name.width` works in tests);
- a `pixel` collision tests exactly the pixels drawn;
- cells drawing the same picture share it.

Text and `<image>` files are the exception: only a backend can measure them, so they cannot be `pixel`, and a size reads 0 until measured. Keep new kinds on this path.

## Lines and polygons

- **`<line>`** (from, to, colour, thickness), drawn as round-ended bars of true width. Lunar Lander is drawn entirely from lines. Lines have no fill.
- **`<polygon>`** of `<point>`s, filled even-odd, its own kind (not mixed with lines), drawn once so `pixel`, `<hole>` and turning all work. Rejected: a `<fill />` on a closed run of lines (lines are redrawn when turned, losing holes).
- Rejected: a box with a list of parts (a second description that drifts from the picture).

## Bitmaps

- Chunky arcade sprites are small grids; an image file is not in the XML (cannot be read or diffed with the game).
- **`<bitmap>`** of `<row>`s, `.` clear and `*` solid, a whole `<scale>`, a `<color>`. Strict: an unnamed character or unequal rows stop the load. One element per row, since whitespace in text differs between the four XML libraries.
- **Inks:** more `<color char="w">`s, so a ghost's eyes follow its body's looks. Rejected: two or three objects on top of each other.
- **Styles as palettes:** `<bitmap style="mario">` takes the style's scale and inks, its own winning. About 150 lines repeated another bitmap's colours. Rejected: a separate `<palettes>` container, taking another sprite's colours by name.
- **Palette swap:** `<become style="angry">` puts a style's colours on. Each palette an object can reach is drawn at load and shared by copies, so a swap only points at other pictures. Rejected: recolouring at run time (a pixel does not know its character). Not for an object with a heading.

## SVG parts

- A vector sprite sheet. `<image>` hands a file to the backend, and only some read SVG, so pictures would differ per library.
- **`<svg>`**: a path, an optional part (all four of x, y, width, height, or none), `<scale>`, `<hide>` by id (a missing id is not an error).
- **lunasvg is not a backend:** no factory or choice, since a choice would only make a sprite differ. Only `picture/svg.cpp` includes it. It supports `<use>`; nanosvg does not.
- Cut the sheet close round what is drawn, since non-pixel touches use the whole box.
- Rejected: SDL_image in backends, PNG at build time, rasterizing at window size.

## Sheets

- Beef: sprite animations and card sheets are the same idea, so one neutral word for both, dead simple for a plain grid and able to say groups, margins, gaps, split runs and scattered pictures when needed.
- **A game-level `<sheets>`, each named once:** a path, a grid (`<columns>`/`<rows>` or a `<cell>` size), `<margin>` and `<gap>` (0 when left out), `<hide>` for SVG. Sprites take `<cell sheet>` by column and row or by x, y, width, height; animations take `<cell>`s and runs of `<cells>` (`<count>`, `<down />`); a deck takes `sheet=` and optional `<faces>` runs. One reader (`picture/image.cpp`, plutovg for PNG, JPEG, BMP; lunasvg for SVG), one cutter, area-average shrink.
- **Runs mirror group layout:** start, count, along or down. A run never wraps at the row's end; a split animation is two runs, as Beef described it.
- **Animation cells need no sprites:** each is a sprite named `walk.N`, so `<become>`, looks and `from=` treat it like any other.
- A cell is a whole sprite: a member changing one replaces it. Open: a row or member changing only a cell's column or row (what Space Invaders 2's SVG parts do), which would let that game move onto a sheet.
- Rejected: a sheet per sprite (the path and grid said again and again); a list of cells in one string (the format's rule); wrapping runs (a hidden rule about where a row ends).

## Flip and turning

- `<flip>` is applied right after drawing, before turning, so the size is exact.
- **Turning by heading, on demand:** an object keeps the original (`Turnable`) and one current bitmap, redrawn when the whole-degree heading changes. Nearest sample keeps hard edges; lines are redrawn turned. Every heading is the same square. Rejected: 72 pictures made at load.
- Not done: smooth turned edges, fractions of a degree.

## Tethers

- A line between two things that move (a fishing line). **`<tether from to>`** is a sprite redrawn after each frame's move between the two middles; in play only while both ends are. Rejected: a `<line>` with live ends (a sprite is drawn once).
- Open: other end points, a rope that limits distance, touches along the line.

## Holes

- Shields that shots wear away, craters. **`<hole>`** in a rule cuts a round hole (`<rough>` for a ragged edge) in the object's own picture, so the next shot goes deeper.
- Centred on the solid pixel nearest the striker's middle, not the touch point, which stays on the unchanging box edge.
- Copy on write; the whole picture is kept for a reset. Only for pictures that stay as they are (no heading, animation or second look).
- Open: decals, holes in SVG or images, a hole the shape of a bitmap (`<hole sprite>`: the arcade's shields were cut by a fixed pattern).

## Light and dark

- Haunted House: walls seen only by a match. **`<light>`** is a radius; **`<dark>`** `unlit` draws only while lit, `unseen` from the first time lit. Only drawing changes, in one place (`drawObject`), so every library and generated game gets it.
- Rejected: darkening the screen and cutting a circle (a backend effect four times), clipping pixels to the circle, a map of seen tiles.
- Open: lights that fade at the edge, line of sight.
