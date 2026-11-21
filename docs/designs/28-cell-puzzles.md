# Cell puzzles

**Status:** Built (Crates, Doubles, Minefield). Open items at the end.

## What we looked for

Turn-by-turn puzzles on cells, where nothing moves until a key is pressed: box pushing (Sokoban), sliding numbers (2048) and mine finding (Minesweeper). The cell system from the [grid puzzles](06-groups.md#cells) already had steps, refused steps as touches, spawning into free cells and despawning. The question was how few words the three would add, and the games are original ones of each kind (Crates, Doubles, Minefield), not copies.

## What we chose

- **`<shove />`, a touch verb.** A worker walking into a crate is a refused step, so pushing is the worker's rule for that touch: the crate is stepped a cell on and the worker's step goes on. A crate that cannot go refuses the shove, so one crate is pushed, never two. Named `shove` because `<push>` is the state verb. Rejected: a "pushable" property on the crate, which would be a second way to say a rule.
- **Floors as layers.** The spots a crate goes on are a second map laid in the same place; a piece meets only pieces of its own map. Rejected: a non-solid piece on the same map, which would need a word for "steps over this".
- **`<here />` and `<around>` are places, not verbs.** `<count>` and `<trigger class>` take one: the cell itself, or the eight (`<straight />`: four) round it, with `<alike variable>` to count only equal neighbours. Counting mines, a crate on a spot and "is any move left" (Doubles' equal pairs) are all a count; a flood fill is a trigger to the squares around. Every layer is looked at, so a mine on its own map counts.
- **A flood runs once a piece a press.** A trigger about the cells around remembers whom it reached in the outermost trigger, so a square opening its neighbours, which open theirs, stops by itself. The load's circle check lets such a trigger through.
- **A `<condition>` in an action.** A piece picking its look from a count (a crate on a spot, a square showing 3) needed an "if" inside an action. It is the state condition's own test in a new place, its commands run where it stands in the list. Rejected: a look-by-number word (a table from values to looks), which would only cover looks. Inner conditions are refused for now.
- **`<slide class>` and `<merge variable>`.** Doubles slides every number, the one nearest the wall first, so a row of 2 2 2 2 gives 4 4. Meeting an equal number is a refused step whose rule merges: the other goes, this one takes the sum, the step goes on. A piece merges once a slide. The slide is refused when nothing moved, so the new 2 written after it comes only after a real move.
- **`<undo />` a press at a time.** Before a press, the engine keeps every piece's place, look and numbers, and keeps the snapshot only if a cell piece moved; `<undo />` restores the last one. Kept only in games that have an undo key. A new scene starts with nothing to undo.
- **A state's trigger reaches only the current scene.** Each room of Crates has its own worker; a key pressing `worker` must not walk those of rooms not shown. An action of an object away in another scene does nothing.

## Open

- Undo cannot bring back despawned copies, so it fits pushing puzzles, not Doubles.
- A weighted pick for spawning (Doubles' one 4 in ten); a safe first square in Minefield.
- Conditions inside a condition in an action; a live text as a look.
- More rooms for Crates, and a best moves per room.
