# 42. Bitmap sprites and animation

**Status:** built (`games/spaceinvaders.xml`, `tests/test_bitmap_sprites.cpp`); new tags: `<bitmap>` (with `<row>`, `<scale>`), `<animation>` (with `<interval>`, `<frame>`), a `name` on `<sprite>`; `<fire>` now centers the projectile on the shooter

## Why

Every picture so far was a plain shape, a text, an image file, or a drawing made of straight lines. A chunky arcade sprite (an alien, a ship) is none of those comfortably: lines have no fill, and an image file is not in the XML, so the picture cannot be read, edited or diffed with the rest of the game. Arcade sprites of that era were small grids of on and off pixels, and a text file shows such a grid directly. The second gap was movement without motion: nothing changed an object's picture over time, which the invaders' two-pose flap needs.

## The language

A `<bitmap>` is a sprite written as rows of text. A period is a clear pixel, an asterisk a solid one, and `<scale>` says how many real pixels each character is across, so a handful of characters is a chunky sprite whatever the window size. It sits where `<line>`s sit, as the one thing in a `<sprite>`, and it can also be the shape a `<grid>` repeats.

An object that changes picture has several `<sprite>`s, each with a `name`, and an `<animation>` after them. The animation holds an `<interval>` (seconds per picture) and `<frame sprite="name" />` elements that refer back to the sprites by name, in the order shown. Referring by name, rather than nesting the sprites inside the animation, keeps a sprite an ordinary sprite and lets one be used in more than one place in the sequence.

## Decisions

- **A bitmap becomes the same kind of bitmap a sprite of lines does** (`Object::bitmap`, drawn once when the game loads). Every backend, the Qt renderer and the pixel collision test already draw and test that, so nothing in any window backend changed. The internal shape kind is still called `Line`; it means "a picture the engine drew itself" and was not renamed, to keep the change out of every backend.
- **Strict characters.** Only `.` and `*` are allowed, and every row must be as long as the first. A space or a stray letter is far more likely a typo than a design, and silently reading it as clear or solid would hide it. The message names the row and the character. More characters can be allowed later (see the options below) without breaking files that exist.
- **One color per bitmap**, like one color per `<line>`; a two-color sprite is two objects.
- **Seconds in the file, frames in the engine.** `<interval>` is written in seconds because that is how someone thinks about a flap, and is turned into frames of the game with the window's `<framerate>` when the game loads. The engine still has no clock (speeds are pixels per frame), so a slow machine animates slowly along with everything else. A window with no framerate cannot count seconds, which is a load error.
- **Strict animations.** At least two frames; every sprite of the object must be shown, or it is an error naming the sprite; several sprites with no animation is an error; frames must all be pictures of one size (and one grid layout), since the object's size, its place in a grid and its collision box are measured once.
- **A count per object, advanced by the game loop.** `Game::updateObjects()` counts a frame for every object that is shown and in play; when the count reaches the interval the object's `bitmap` points at the next picture and its visual is marked dirty, the same way a heading changes the picture. A pause or menu holds the picture, and a reset (`resetObjectState`) restores the first picture and the count.
- **The cells of a grid animate together** because they start together and count the same frames; they share the pictures themselves.
- **Groups.** A group, and a member, can each give `<sprite>`s and an `<animation>`. A member's own sprites replace the group's, and an animation (its own, else the group's) is resolved against whichever sprites the member ends up with. This is what lets Space Invaders have three kinds of alien in one group: one lockstep block that marches and steps down together, each member a `<grid>` of its own two sprites.
- **Headings turn the finished picture.** An object with a `<heading>` and a `<bitmap>` has the rows drawn once into a picture, which is kept; the picture shown is that turned to the heading by taking, for every pixel of the result, the one original pixel under it (`turnBitmap`). Turning the image and not the rows keeps the scale, so a pixel of art stays a block of `<scale>` pixels, and nearest-pixel sampling keeps the colors flat and the edges hard, which suits the look. Every heading comes out as one square size, as a turned sprite of lines does, and the object's size is that square. This is the same machinery as [41](41-asteroids.md) (`showHeading`), only filled from an image and not from vectors.
- **Turn on demand, two pictures.** The first version drew a picture for each of 72 headings when the game loaded, which cost 72 pictures per sprite (and per frame of an animation). Turning needs only the original and the picture shown now, and one turn of a few thousand pixels is cheap, so an object keeps a shared `Turnable` (the lines, or the original picture) and its own current `bitmap`, and redraws when its heading, rounded to a whole degree, changes. That also gave 360 headings in 1 degree steps for free. Drawing and collision still read the same `bitmap`, so no backend changed. Letting each graphics library rotate a texture was rejected: the picture drawn and the picture a pixel collision tests would differ by rounding and filtering from library to library.
- **Animation and headings together.** Each frame has its own `Turnable` (`Object::turnables[frame]`); the object turns the frame that is showing, so turning does not restart or skip the animation and an animation step keeps the heading. Frames must therefore turn in the same square size, which equal sized bitmaps do by construction.

## Space Invaders as the test

The aliens are a group of three members (a row of squids, two of crabs, two of octopuses), each a grid of two bitmaps animated on a one-second interval; the ship is one bitmap; the bullet is a thin tall rectangle. Making the bullet thin showed that `<fire>` put the projectile's left edge, not its middle, at the middle of the shooter, so it left from beside the barrel. It is now centered on the shooter's top edge, as the docs said, which also moved the shots of Depth Charge and Astrosmash by half their width.

Moving the game from one grid of identical rectangles to three kinds of alien broke several tests that were only borrowing the game as a fixture for grid naming, swept hits, lockstep spacing and the win. They now load a frozen copy of the first game (`tests/invaders_fixture.h`), so they check the engine and do not change when the game does.

## Options considered

- **One multi-line text block** for the rows. Whitespace inside an XML text node is handled a little differently by each of the four XML libraries; one `<row>` element per row is the same on all of them and reads the same as a list of `<line>`s.
- **A palette** (a letter per color, declared once). Natural next step for multi-colored sprites; not needed yet, and the two-character alphabet leaves room for it.
- **Frames as attributes of a sprite** (`<sprite frames="2">`). Reads shorter but cannot say a different picture sequence, or reuse a picture.
- **A global animation clock** instead of a count per object. Everything would flip on the same tick whatever its state; a per-object count is simpler to reason about with pause and reset, at the cost of cells being in step only because they start together.
- **Flipping a bitmap**, and turning by a fraction of a degree or smoothing the edges of a turned picture.

## Not done

Per-frame intervals, an animation that runs once, animations driven by a variable or by a collision, a color per pixel, and a tool to turn an image into rows of text.
