# Notes: looks and pictures

Tips for changing how things are drawn. Back to [the notes](readme.md).

## Looks and pictures

- Every engine-drawn picture is `ShapeKind::Line`, built in `buildPicture`. Never draw or turn in a backend.
- Looks that move come **before** still sprites in the look list, so look 0 is the first animation; an object with `<poses>` starts as standing.
- A pose is picked only while the current look is a pose look.
- `<become object="group">` changes members out of play too.
- `<stop />` takes the pull away until a reset, so it is no landing for something that can be knocked up again.
- `<grip>` below 1 brakes an object with no keys held: never give it to a thing that moves by velocity alone.
- A hop's `<arc>` is drawn through `altitude`; tests compare positions, not drawn spots.
- A `RawObject` made in code (a deck's cards) must have `sprite` equal to `looks.front()`.
- Cards draw by `drawOrder`: 0 first in file order, then the rest sorted.
- A `<become>` does not change an object's size until the frame ends, so `name.width` later in the same list is the old look's. Give looks placed by size the same size.
