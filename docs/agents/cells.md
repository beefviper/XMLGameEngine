# Notes: cells and boards

Tips for changing grid puzzles, board games and the computer's search. Back to [the notes](readme.md).

## Cells and boards

- A free cell is one `cellHolder` finds nothing in, so a picker or marks on the board's own map would block it: keep them on a map of their own.
- `countMoves` fills live `moves`; it is 0 while the piece is not shown.
- `<put>` turns a trapped piece by despawning it and spawning the putter's template; copies sit in `spawned` until the frame ends, so live `moves` leaves out pieces put this frame.
- `<think>` copies every kind it reads (`Kind`) and never points into the game, since in a window it runs on its own thread (`thinkInBackground`). Tests have no `loop`, so they search in the frame. Random choice among equal moves is made on the game's side.
- `Engine::runAsPress` judges each trigger's `from=` by the look at the press. An axis `<trigger from=>` is honoured: give the look, or it runs in every look.
- `cellHolder` answers from a snapshot while a `Game::CellsStill` guard lives. Never move a piece on cells while one is held.
- Maps with the same origin and tile are layers: `piecesAbout` (here/around) looks at every layer, a step meets only its own map. `triggerAbout` reaches each piece once per outermost trigger (`aboutReached`), which is what stops a flood.
- `runAction` does nothing for an object `isAway` (in a scene not current): a state's key naming `worker` would otherwise press every room's worker.
- Undo snapshots are kept only when the game has an `<undo />` key (`keepsUndo`), and only for a press that moved a cell piece (`pressBegins`/`pressEnds` around `Engine::runAsPress`). A press made any other way keeps nothing to undo.
