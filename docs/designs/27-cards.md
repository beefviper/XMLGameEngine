# Cards

**Status:** Built: Klondike, Crazy Eights, Blackjack, Texas Hold'em, Hearts, Spades, Gin Rummy, Pyramid, all on general words (no card nouns). Open: decks of single cards. Reference: [cards](../reference/cards.md).

## What we looked for

- Cards as a first-class part of the language, saying **how the pack is built** (an Italian pack has 40 cards; some games use two packs; trading card games have every card its own).
- What board games lack: **many pieces in one place**, stacks fanned or squared, **face up or down**, **shuffling and dealing**, and moving **a run** by rules about the one below.
- A rule over all of it: **tags name mechanics, never a specific game.** No `<poker>`.

## Chosen

- **No card nouns** (Beef): other games need collections of pieces too. A pack is a `<group>` in rows (suits) and columns (ranks); a pile anything that `<holds>` a class; the hand a picker (`<pick>`, `<place>`, `<drop>`). The first form had `<deck>`, `<pile>`, `<hand>` and card verbs (`<take>`, `<lay>`, `<deal>`, `<collect>`, `<shuffle>`, `<redeal />`, `<reveal pile>`); all eight games moved, then the old words left the schema, reader and generator.
- **Verbs say what they do:** `<send>`, `<distribute>` (Beef's word over deal), `<order>` with `<random />` (over shuffle: it says what becomes random), `<become>` on a holder turning what it holds. They work in any action, timer or condition; `<starts>` in a state is the deal.
- **`<carry />`**, once `<run />`: a picked card takes those on it, the word Adventure carries with, so "run" means only cards in a row.
- **Placing rules are conditions** on the arriving one, which compare its variables, so no word knows suits or colours. The card under it is `top` (`<equal>top.rank - 1</equal>`, `<unequal>top.color</unequal>`); `total="sum"` is over what is laid at once (Pyramid's pairs), `total="count"` how many. They were once words of their own (`<same>`, `<differs>`, `<step>`, `<total>`, `<one />`) that read as "a random list of tags" (Beef); now anyone who reads conditions reads them. Made at load into the same few comparisons (`PileRule::VarRule`), so the computer's search stays fast. Any comparison takes the top's own variable or another (`<below>top.worth</below>`), plus or minus a number.
- **A card is worth its column** (Beef): an ace at the front or back of the row; a column, row or cell lower down gives another value, and `high` is a second worth counted where it fits within a holder's `<counts><atmost>` (Blackjack).
- **Points are card variables** (the hearts row, the queen's cell) a holder `<counts>` into its own variable; no points table.
- **One engine underneath:** `settleHolders` (game/cards.cpp) reads the words into the card engine: a held class's cells become cards, a holder's looks face up or down, `<starts>` where the cards begin. Generated programs build the same raw data and call the same function.
- **Faces from one sheet** (Beef's call), `assets/cards.png`, a row per suit, a column per rank; a sheet cell leaving out its column or row takes the group cell's own. The drawn deck was about 960 lines per game; the sheet takes it to 10 (0.1 s, 45 MB to load).

## Players, patterns, tricks

- **Players in turn** ([players as a group](07-states-and-conditions.md#players-as-a-group)): a member's `<owns>` is its hand; `<distribute into="players">` deals a card a round by seat; `<pass member="trick.winner">`; `<team>` for partners. Gin's turn is two looks, changed by a condition on holding eleven since a pick on a `<picked>` holder is refused.
- **Wild cards and a call:** `<wild variable>` in `<accepts>` makes the holder wait for `<call pile>` naming a value of that variable from a card the caller points at; no suit picker screen. A wait in the turn says the rule in one place (rejected: a `laid` variable and two conditions).
- **Patterns, not poker:** a `<patterns>` set from `<alike variable>` (the grid games' word for sharing a value; once `<same>`) and `<run variable>`, the same parts rummy and dice use. The engine finds the best and orders two by one general tie rule; a holder `<score>`s `<with>` a shared pile, the `<best>` so many. `<run variable="rank">` runs by column (Gin's court cards are worth 10). **Melds** split the whole holder for least deadwood and lay it out that way.
- **A trick is a holder with a rule:** `<trick variable="suit">`, `<breaks>`, `<trumps>`; it knows who laid each card and gives `lead` and `winner`.
- **Covering is drawn, not written:** a card is free when no card of a later holder overlaps it. Listing 28 covering pairs was the alternative; overlap serves TriPeaks too.
- **The computer plays by rules of thumb,** not a search, since hidden cards would be guessed: shedding, melds, tricks (`<avoid />`), bids by the book's `tricks` count; Hold'em by `odds` (five hundred seeded deals, only for piles something reads them of: the odds were 88% of a Hold'em game's time). Rejected: bluffing, reading opponents, counting played cards (for now).

## Looked at and left

- Cards as pieces on a cell map (a stock holds many cards in one place).
- A key per pile (not the key convention; the mouse points anyway).
- Each pattern spelling its own tie rule; partners inferred from seats; select-two verbs.

## Open

- `<picked>` and `<starts>` take only the forms the deal needs; a trick follows only `suit`, and suits are numbers (named rows would read better); `<call pile>` is still a card word; a held `<send>` gets no holder check at load; points and tricks on held members.
- A deck of single cards; a pass when nothing goes and nothing is left to draw.
- Hold'em with more players, no-limit, side pots, blinds; Blackjack doubling, splits, insurance, a shoe.
- Hearts' passing and first-trick rules; nil bids; laying off in Gin Rummy; melds on the table.
- Klondike's three-card deal, undo, automatic finish; TriPeaks and Golf.
