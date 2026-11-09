# Cards

**Status:** Built: deck, piles and a hand (Klondike); players' cards, wild cards and a card-playing computer (Crazy Eights); points, a dealer and bets (Blackjack); patterns and odds (Texas Hold'em); tricks (Hearts), trumps and bids (Spades); melds (Gin Rummy); covering and pairs (Pyramid). Open: decks of single cards. Reference: [cards](../reference/cards.md).

## Cards as groups

- Beef: deck, pile and hand are card nouns; other games will need collections of pieces too. So a pack is a `<group>` in rows (suits) and columns (ranks), a pile anything that `<holds>` a class, and the hand a picker (`<pick>`, `<place>`, `<drop>`). Verbs named for what they do, not for cards: `<send>`, `<distribute>` (Beef's word over deal), `<order>` with `<random />` (over shuffle or randomize: it says what becomes random). Rules on what is placed compare variables (`<same>`, `<differs>`, `<step>`, `<total>`), so no word knows suits or colours.
- **Stage one, built:** Klondike. The new words are read into the existing card engine (`settleHolders`, game/cards.cpp): a held class's cells become cards, a holder's looks become face up or down, `<starts>` becomes where the cards begin and what each holder is dealt. Generated programs build the same raw data and call the same function.
- Open: `<picked>` and `<starts>` take only the forms the deal needs; `<order>` and `<distribute>` as verbs anywhere; the other seven games (Crazy Eights with players as a group); `<any>` in `<accepts>`; points, patterns and tricks on held members.

## What we looked for

- A deck of cards as a first-class element. The game should say **how the deck is built**: an Italian deck has 40 cards in coins, cups, swords and batons; some games use two packs; trading card games have every card its own.
- What board games lack: **many cards in one place**, stacks fanned or squared, **face up or down**, **shuffling and dealing**, and moving **a run of cards** by rules about the card below.
- A rule over all of it: **tags name mechanics, never a specific game.** No `<poker>`.

## The pieces

- **Three things, kept apart:** a `<deck>` makes the cards; a `<pile>` on an object holds cards, with its rules; a `<hand />` object is the player's cursor that carries cards between piles. Piles and hands know nothing of suits, so a deck of single cards can come later.
- **A card is an ordinary object** (`cards.1` to `cards.52`) with `suit` and `rank` and looks `face` and `back`, so conditions, sounds and looks work on cards.
- **Faces come from one picture sheet** (Beef's call), `sheet=` naming one of the game's `<sheets>`: a row per suit, a column per rank, the back the next cell; other layouts are `<faces>` runs ([pictures](30-pictures.md#sheets)). The drawn deck was about 960 of each card game's lines; the shared `assets/cards.png` (2496 by 1400, eight backs) takes it to 10 and is easy to replace. Read by plutovg, shrunk by area averaging: 0.1 s and 45 MB at load (4992 wide: 0.2 s, 130 MB).
- **A drawn deck stays possible:** each suit gives its pips as rows, each rank its corner index and pips as a grid of characters, a court card a `<picture>` drawn as its top half; the engine lays out the faces. The shared sheet was rendered from exactly that deck at twice the size, so the faces are pixel for pixel what they were.
- **Pile rules are data, like `<moves>`:** what it `<takes>` (`<samesuit />`, `<othercolor />`, `<up />`, `<down />`, `<empty>`), whether a `<run />` may be taken, `<showtop />`, `faces=`. The stock is `<deals into>` and `<refill from>`.
- **The deal is the piles':** each pile's `<dealt>N</dealt>` takes from the deck in rounds; `<more>1</more>` makes Klondike's staircase over a group.
- **The hand works like Checkers' picker:** `<take />`, `<lay />` (refused when the pile does not take the cards), `<drop />`, `<deal />`. Laying back where cards came from is always allowed.
- **Moving the hand:** `<hop>` goes to the next pile or card; `<track />` points under the mouse, so press-drag-release and Space twice are both a move. A click on the bare table does nothing.
- Cards draw in pile order, held cards above, by a `drawOrder` kept on objects.

## Players, points and patterns

- **A player's cards are a pile of their own,** fanned, `<sorted />`, and a hand that owns it (`<hand pile="mine" />`) takes only from it and draws into it.
- **Wild cards and a call:** `<matches />`, `<wild>`; a wild card makes the pile wait for a `<call />` of the suit of a card the caller points at. No suit picker was needed; this is narrower than strict rules, chosen for having no extra screen.
- **The turn is the game's** (a variable and looks gating keys), not the engine's.
- **Points:** a rank's `<worth>` (several when it may count either); a pile's `<points><atmost>21</atmost>` adds them, face-up cards only. The dealer is conditions and a timer, bets are plain variables.
- **Patterns, not poker:** hands are written as a `<patterns>` set from three mechanics, `<same>`, `<run>`, `<suited>`, the same parts rummy and dice use. The engine finds the best and orders two by one general tie rule. A pile `<score>`s by a set, `<with>` a shared pile, the `<best>` so many.
- **Melds** are patterns read for a whole group (`<score><melds />`): the engine splits the cards for the least deadwood and lays the pile out that way. `<each>` names exceptions to worth.

## Tricks, covering, pairs

- **A trick is a pile with a rule:** `<trick>` (follow suit), `<breaks>`, `<trumps>`; the pile knows who laid each card by `<seat>` and gives `lead` and `winner`. These moved out of `<takes>` so `<takes>` keeps one meaning.
- Taken tricks go into a pile with `<collect>`; a new hand is `<redeal />`, the same deal a new game makes.
- **Covering is drawn, not written:** a card is free when no card drawn over it overlaps it. Listing 28 covering pairs was the alternative; overlap serves TriPeaks too.
- **A pair is two cards held:** `<holds>2</holds>` and `<takes><sum>13</sum></takes>`.

## The computer

- **Rules of thumb, not a search,** since hidden cards would have to be guessed. Shedding: play the longest suit, high first, wild last. Tricks: `<avoid />` ducks with the highest losing card. Bids count `tricks` by the book's rule of thumb. `<team>` lets a partner's winner stand.
- **Hold'em uses odds:** a thousand random deals of the unseen cards, seeded by the cards seen so runs repeat. Four conditions play it: raise, call by pot odds, all in, fold.
- Rejected: bluffing, opponent reading, counting played cards (for now).

## Looked at and left

- Cards as pieces on a cell map (a stock holds many cards in one place).
- A key per pile (not the key convention; the mouse points anyway).
- Each pattern spelling its own tie rule; partners inferred from seats; select-two verbs.

## Open

- A deck of single cards; a pass when nothing goes and nothing is left to draw.
- Hold'em with more players, no-limit, side pots, blinds.
- Blackjack doubling, splits, insurance, a shoe.
- Hearts' passing and first-trick rules; nil bids; laying off in Gin Rummy; melds on the table.
- Klondike's three-card deal, undo, automatic finish; TriPeaks and Golf.
