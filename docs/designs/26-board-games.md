# Board games

**Status:** Built: Checkers with a computer, marks, put back and the mouse; Reversi, Four in a Row, Tic-Tac-Toe, Dots and Boxes. Open: other board games, lookahead for action games. Reference: [board games](../reference/boards.md), [thinking](../reference/states.md#thinking).

## What we looked for

Turn games on cells. Maps of cells already knew which piece is where; missing were a way to say **how a piece may move**, and for a player to **choose a piece and where it goes** rather than steer it.

## Moves

- **`<moves>` on a piece:** `<step>`s and `<jump class>`es as cell offsets, `<forced />`, `<promote sprite><row>`. A move is data about the piece, like `<poses>`, not a verb; the engine works out what is legal. A set per look (`<moves sprite="king">`), so a king is a look.
- **A jump is over one piece into the cell beyond**, taking it: checkers' capture, and in the same words a halma hop.
- **Forced capture and runs of jumps are built in,** since legality must be known before a press can be refused.
- **Promotion ends the move** (American checkers). `<promote>` names a row, so each side writes its own.
- **Live number `moves`**, so "no move on your turn loses" is an ordinary condition.

## The picker

- **A picker, not a selected piece.** `<pick class>` and `<place />` are self verbs of a cursor using its cell. A refused verb ends its list, so `<place />` then the turn's change passes the turn only when a move is made. Whose turn it is is a variable and the cursor's looks.
- The cursor sits on **a map of its own** over the board, so it walks over pieces without touching them.
- Rejected: a `<turns>`-style word (that swaps whole games), crowning by a condition (conditions have no `self`), marking the held piece by its look (every piece would need held looks).

## Easier play

- **Put back is `<drop />`,** as on a carrier; refused once a run of jumps has begun.
- **`<marks>` on the picker** (held, moves, movable, last), worked out by the engine and drawn as copies on their own map; gated by look and by a variable, so an option on the title is a kept variable.
- **Auto-pick is `<pick><only />`** run by a condition.
- **The mouse is `<track />` on cells**; an axis trigger keeps its `from=`, so the mouse moves only the player's cursor.
- **A key's `from=` is judged by the look when pressed**, not as each trigger runs: one press could otherwise drop both sides' discs.

## Pieces put, not moved

- **`<put class>`** puts a copy of the first piece of the class written, so the board in the file is the start and the pieces are the supply.
- What the piece's `<moves>` say decides where: `<flank class>` (Reversi: a line of the other class ended by one's own), `<fall />` (down a column), nothing (anywhere free), `<on class>` (in place of a piece of that class).
- **Trapped pieces turn by replacement,** so sides stay classes (what `<jump>`, `total=` and `<think>` go by).
- **Passing and the end are conditions** on `moves`. **`<count class into>`** gives a score.
- **Wins by line use the map's `<line>` check.** Checks may not push a state, so a check sets a variable and a condition ends the game.
- **Dots and Boxes** needed `<on class>` (a line only on its place) and `<close class object>` (claims the boxes walled in; the put counts as refused so the turn does not pass). Each side's lines are its own class.
- **`<onto class>` on a hop or a track** moves only between places of those classes, reading the map as rows like a text cursor. No rule of cell distances alone fits both across and down lines.

## The computer

- The ask: a generic player that knows the rules and goals, not a strong one. The rules are already data, so the computer need only play them forward.
- **`<think>`** copies the board into a small table and searches with negamax and alpha-beta, depth 4 by default, by the pieces' own `<moves>`. It presses keys as a person would, walking the picker, so the game's own actions do the move. Its own key and cursor looks keep the player out of its turn.
- **Goals are inferred:** pieces count 1 (or a `<worth sprite>` hint); a side with no move loses (or passes in games of puts); a `<line>` check is a win, and short of that each open run is worth its pieces squared; a closing put keeps the turn.
- **Made cheaper:** boards seen are remembered by a key of their pieces; a move is dropped once plainly worse than the best; closing moves first; the free-place counts read one snapshot (`CellsStill`). In games of claims, a **quiet** put (touching nothing) is searched once and its fellows count as equal; used only for claims, not for lines, where corners and edges differ.
- **It thinks beside the game:** in a window the search runs on a thread on a copied board, so the player's move is drawn first. With no window it runs in the frame, so tests replay the same.
- A piece put in the same frame was once missed from the copied board, and Tic-Tac-Toe's smart level could lose; fixed.
- Pong's levels are plain `<computer>` conditions with no search; a paddle at the top must not reach a corner-to-corner ball.

## Open

- **Lookahead for action games:** play the game forward a second or two on a copy of the objects' numbers for each choice of keys, scored by goals inferred from the conditions.
- A stronger Dots and Boxes (chains) and Reversi (corners, mobility).
- `<onto>` for Checkers' and Reversi's cursors; draws by repetition or forty quiet moves.
- Halma (jumps that do not take), chess (sliding moves of any length, blocking pieces, stalemate).
