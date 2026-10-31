# Board games

**Status:** Checkers Built (2026-10-31); a computer opponent, other board games Open.

## What we looked for

- Asked for (2026-10-31): "turn based on cells... so like, Checkers?", the turn puzzles on cells that the class survey (project files, `notes/tables-and-tracks-2026-10-31.md`) listed as a small follow-on to the maps of cells.
- The maps of cells already knew which piece is in which cell. What was missing was a way to say **how a piece may move**, and a way for a player to **choose a piece and where it goes** rather than steer it.

## What we chose and why

- **`<moves>` on a piece**: `<step>`s and `<jump class>`es as cell offsets, `<forced />`, `<promote sprite><row>`. A move is data about the piece, like `<joins>` or `<poses>`, not a verb: the rules of checkers are a short list per side, and the engine works out which moves are legal. A set per look (`<moves sprite="king">`) reuses looks as the piece's simple state, so a king is a look, not another object.
- **Jumps are over one piece into the cell beyond.** That is checkers' capture, and in the same words a halma hop (a jump over a class of your own). Taking the jumped piece is part of the jump; a game that does not take could later say so.
- **Forced capture and runs of jumps are built in**, since they decide which moves are legal, and that has to be known before a press can be refused. `<forced />` is per set, so a game without the rule leaves it out. A run of jumps is not optional: a piece that has jumped and can jump again must.
- **Promotion ends the move** (American checkers: a man crowned by a jump does not jump on as a king). `<promote>` names a row of the map, not "the far side", so each side writes its own.
- **A picker, not a selected piece.** `<pick class>` and `<place />` are self verbs of the object that points (a cursor), using its cell. A refused verb ends its list (as since 2026-10-18), so `<place />` followed by the turn's change passes the turn only when a move is made and over; a place that jumps but must jump on counts as refused for the same reason. No new idea of "the turn" was needed: Checkers keeps whose turn it is in a variable and in the cursor's looks.
- **The cursor on a map of its own** over the board (same tile, same place), so it walks over pieces without touching them and needs no exception in what counts as a free cell.
- **Live number `moves`**, counted before the conditions each frame, so "no move on your turn loses" is an ordinary condition (`total="sum"`), and so is "no pieces left".

## Looked at and left

- **A `<turns>`-style turn word**: `<turns>` swaps whole games between players; on one board both sides share one game. A variable and looks said it with nothing new.
- **Crowning by a rule** (a condition on the row reached): conditions have no `self`, and a state's condition cannot change "the piece that just moved". `<promote>` inside the moves keeps it with the rule that leads there.
- **Marking the piece held**: the cursor's look shows that something is held, not which piece. A `picked` name usable in `object=` would let the game change the held piece's look; left until a game needs it.

## Open

- A computer opponent (a `<computer>` that presses keys cannot choose a move; it needs a move-choosing verb, perhaps "the move that takes most").
- Draws (forty moves with no capture, a position repeated).
- Other board games on the same words: halma and Chinese checkers (jumps over any class, not taking), reversi (placing new pieces that turn others over), chess (sliding moves of any length: a `<slide>` move, and pieces that block).
