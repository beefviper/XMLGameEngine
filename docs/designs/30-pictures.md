# Pictures

**Status:** Built: lines, bitmaps, SVG parts, flipping, turning.

## The one decision behind all of it

**The engine draws every picture it can, once, at load, into its own `Bitmap`; backends only show that bitmap.** One path in `buildPicture` (`load/expr.cpp`): read, draw, flip (bitmap, svg), keep a `Turnable` for objects with a heading. What an object shows is a typed `Picture` (`game/command.h`: kind, size or radius, color name, text and its size and font, image file and flip), never a list of strings a backend parses (the old `spriteParams`, gone 2026-10-29). Consequences:

- no window backend, the Qt renderer or the collision detector changes when a picture kind is added, and every library shows identical pixels;
- the object's size is known without a window (`name.width` works in tests);
- a `<type>pixel</type>` collision tests exactly the pixels drawn ([collisions](20-collisions.md));
- cells of a group drawing the same picture share it (drawn once); the internal shape kind is still called `Line` ("a picture the engine drew itself", params `{"line", w, h}`), not renamed, to keep the change out of every backend. The xgegui inspector calls such a sprite "drawn"/"pixels".

Keep new picture kinds on this path. Text and `<image>` files are the exception: only a backend can measure them, so they cannot be `pixel` and expressions see their size as 0 until measured.


## Lines

- Need: Lunar Lander is drawn entirely from lines; the moon is one object of lines, the lander 16, each pad one thick line.
- `<line>` (from/to, `<color>`, `<thickness>` of at least 1) inside a `<sprite>`, in pixels from the sprite's top left. Rasterized with a square brush, no gaps, transparent elsewhere; endpoints rounded. Not mixable with other shapes. Lines have no fill.
- Rejected: a box with a list of parts (a second description that drifts from the picture).


## Tethers (2026-10-17)

- Need: a line between two things that move (a fishing line, a grappling rope, a leash); the author asked for it named, "recalculated every frame".
- **Chosen: a sprite, `<tether from="rod" to="hook">`**, with `<thickness>` and `<color>`: after each frame's move (`drawTethers` in play/frame, shared with generated games) the line between the two middles is drawn as a picture, as `<line>`s are, and the object's position and size follow it. It is in play only while both ends are (2026-10-19: Missile Command's counter-missile trails, a tether from the base to each shot of a pool, vanish with the shot). Attributes, since they name the two ends. Looked at: a verb that draws a line each frame (nothing to name), a `<line>` whose ends read live numbers (a sprite is drawn once at load).
- Open: ends other than the middles, a tether that limits how far apart the two can go (a rope rather than a line), collisions along the line itself (it touches as its box, or by pixels with `<type>pixel</type>`).

## Bitmaps

- Need: chunky arcade sprites are small grids of on/off pixels; lines have no fill, an image file is not in the XML (cannot be read, edited or diffed with the game).
- **`<bitmap>`**: `<row>`s of `.` (clear) and `*` (solid), `<scale>` (whole number of at least 1, default 1), a `<color>`. Strict: only the characters a color names and equal row lengths; the error names the row and character (a stray letter is likelier a typo than a design). More colors came later as inks (below), without breaking existing files.
- One `<row>` element per line, not one text block: whitespace handling in text nodes differs among the four XML libraries.
- Rejected at first: a palette (a two-color sprite was two objects; inks and styles replaced that, below); frames as an attribute (`<sprite frames="2">`, cannot reorder or reuse).
- A bitmap can be the sprite of a group in columns and rows, a row changing its rows of text and color.
- **Inks (Pac-Man's ghosts):** a ghost is a body with white eyes and blue pupils, and as two or three objects on top of each other the eyes would not follow the body's looks. So a `<bitmap>` takes more `<color char="w">color.white</color>`s after its first: each names one more character and its color, the plain `<color>` staying `*`. The rows stay a picture; the strictness stays (a character no color names is an error).
- **A palette shared between bitmaps (2026-10-28):** the game's `<styles>` (until then text's size and color) also hold a `<scale>` and `<color char>`s; `<bitmap style="mario">` takes what it does not say, its own winning, resolved in the same shared `applyStyles` as text. Measured first: about 150 lines in the shipped games repeated another bitmap's scale and colors, most in Mario Bros. (Mario's seven poses, the creepers' four), Q*bert and Berserk; converting the groups where a style saves lines cut about 80. Looked at: a separate `<palettes>` (a second container for the same idea of a named look), and a bitmap taking another sprite's colors by name (ties a look to one object, so two characters could not share it).
- **Palette swap (2026-10-28, the author's idea):** a Mario Bros. shellcreeper turns red when it gets up by itself; drawing each look again in red would double the looks and every rule that checks one. Chosen: `<become style="angry">` (beside or instead of `sprite=`) puts that style's colors on in place of those the object's bitmaps took from their own style; colors a bitmap gives itself stay, and a char the new style leaves out keeps its color, so a swap style says only what changes. Each palette an object can be given (one per `style=` of a `<become>` that can reach it) is drawn as the game loads, kept in `Object::palettes` and shared by copies, so a swap at run time only points at other pictures; a reset puts the first back on. Not for an object with a `<heading>` (it turns its picture from another kept original). Looked at: recoloring pixels at run time (a picture does not know which character a pixel came from), a `<palettes>` list on the object (the style already names the colors; the `<become>`s already say which are needed).


## SVG parts

- Need: a vector sprite sheet (Space Invaders: 6 frames across, 4 rows, 32 units a cell) that `<bitmap>` and `<image>` cannot use. `<image>` hands the file to whichever backend runs, and only SDL2/OpenGL (via SDL_image) can read an SVG; the pictures would differ per library and could not be pixel-tested.
- **`<svg>`**: `<path>`, an optional part (`<x>`, `<y>`, `<width>`, `<height>`: all four or none, so a typo cannot mean the whole sheet), `<scale>` (pixels per unit, may be 1.5), `<hide>` by element id (so the file stays as drawn; a missing id is not an error, so a sheet with its backdrop already removed works with the same game file). The drawing's own colors, straight RGBA, antialiased edges; size rounded up so no edge is lost.
- **lunasvg v3.5.0, not a backend:** no interface, factory, command-line choice or Options entry (a choice would only make one sprite look different in different libraries). `picture/svg.h` speaks only in `Bitmap`; only `picture/svg.cpp` includes the library, linked `PRIVATE`. Fetched pinned by `FetchContent`, or found through `find_package`. It supports `<use>` (the sheet defines the ship once and shows it three times); nanosvg does not. It adds no `stb_image` symbols.
- The sheet is cut close round what is drawn, because non-pixel collisions use the whole picture box; all three frames of an alien share one cut so the animation frames match in size.
- Rejected: SDL_image in backends (pictures differ per library), converting to PNG at build time (a tool, a second file to keep in step, no part or scale in the game file), rasterizing at window size (resize redraws; `<scale>` already says what a unit is), a palette/recolor, caching the drawing by file (cheap if loading the sheet per sprite is ever slow).
- Mistakes stop the load naming object and file. Space Invaders 2 uses the aliens, the idle ship, the player bolt, the enemy bolt (flipped), the four explosion frames (a pool released where a hit was) and the saucer's three frames (the mystery ship). The banking ship, hit flash and charged bolt are unused.


## Flip and turning

- `<flip>` (`horizontal`/`vertical`) on a bitmap or svg is applied to the pixels right after drawing, before they are kept for turning, so the size is exact and pixel tests see flipped pixels. Turning half a round with a heading would also flip, but every heading's picture is a square big enough for any angle (a thin falling bolt would get a box about 38 pixels square).
- **Turning by heading**, done by the engine, on demand: an object keeps a shared `Turnable` (the lines, or the original bitmap; one per animation frame) and its one current `bitmap`, redrawn by `Object::showHeading` only when the heading rounded to a whole degree changes (360 headings, `turnBitmap`, `rasterizeTurned`). Each result pixel takes the one original pixel under it (nearest sample): flat colors, hard edges, a pixel of art stays a block of `<scale>` pixels. Lines are re-drawn turned, not rotated, to stay sharp. Every heading is the same square, which is the object's size. The first version pre-made 72 pictures at load; on demand costs two pictures per object and gave 360 headings free.
- Not done: smooth/anti-aliased turned edges, turning by a fraction of a degree.


## Holes (2026-10-17)

- Need: shields that shots wear away (Space Invaders), craters; the author's words were a transformation of the picture, with a blast radius and noise, or decals.
- **`<hole>`, chosen:** a command of a rule about another object, cutting a round hole (its radius as its content, and `<rough>` for a ragged edge) in the object's own picture. A pixel collision then sees the hole, so the next shot goes deeper. Centered on the solid pixel nearest the striker's middle (within two pixels of its box), not on the touch point: the touch point lies on the box edge, which never moves as the picture is eaten, so holes would stay at the surface; the striker's middle alone lies outside the picture and each hole would only reach a few pixels in.
- Copy on write: the picture is shared by every object made from one definition, so the first hole copies it and keeps the whole one (`Object::unholed`) for a reset.
- Only for a picture that stays as it is (lines, polygons or a bitmap, no heading, animation or second look): a turned or swapped picture is drawn again from its original and would lose its holes, and keeping holes across turns is not needed by any game yet.
- Not done: decals (painting a picture onto another, a scorch mark), holes in an `<svg>` or `<image>` sprite, a hole's shape other than round. Beef (2026-10-31) noted the arcade Space Invaders cut a fixed pattern, not a round blast; if wanted, `<hole sprite="splat" />` cutting the shape of a bitmap.

## Polygons (2026-10-31)

- Need: Cannon Hill's hills, filled and blown into craters. Lines have no fill; a bitmap hill is a page of characters with stairs for slopes.
- **Chosen:** a `<polygon>` of `<point>`s and a `<color>`, a sprite of one or more, its own kind (not mixed with lines). Drawn once at load (`rasterizePolygons`: a pixel is filled when its middle is inside, rows scanned against the edges, even-odd), so it is a finished picture: `pixel` collisions test it, `<hole>` cuts it, a heading turns its pixels as a bitmap's.
- Rejected: a `<fill />` on a closed run of lines (colour per line, and lines are redrawn when turned, which a hole would lose).

## Light and dark (2026-10-17)

- Need: Haunted House (walls seen only by a match), fog of war.
- **Chosen:** an object's `<light>` (a radius from its middle, a live number a `<set>` changes) and `<dark>` (`unlit`: drawn only while some light reaches any part of its box; `unseen`: from the first time until a reset). Only drawing changes, in one place (`drawObject` in `window/window.h`), so every window library and a generated game get it; whether each dark thing is lit is worked out at the end of the frame (`lightAndDark` in `play/frame.cpp`).
- Looked at: darkening the whole screen and cutting a circle out of it (a backend effect in four libraries, and a dark wall would still be drawn under it), a light that reveals only pixels within the circle (needs clipping in every backend), a map of seen tiles (only for maps; `unseen` on any object covers it).
- Open: lights that fade at the edge, light blocked by walls (line of sight).
