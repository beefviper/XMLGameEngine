# Cards

**Status:** Deck, piles and a hand Built (2026-11-01) with Klondike; Crazy Eights (hands, a computer) and decks of single cards Open.

## What we looked for

- Asked for (Beef, 2026-11-01): "we definitely need to make a deck of cards a first class element", agreed as the next class after the board games, before backgammon (dice).
- Beef: the game should say **how the deck is built**: an Italian deck has coins, cups, swords and batons and only 40 cards; some games use two packs. And later, games like trading card games, where each card is its own (a picture and numbers), not a suit and a rank.
- What a card game needs that the board games do not: **many cards in one place** (a cell holds one piece), stacks that are **fanned** or squared, cards **face up or down**, **shuffling and dealing**, and moving **a run of cards** at once by rules about the card below.

## What we chose and why

- **Three things, kept apart:** a `<deck>` makes the cards; a `<pile>` on an object is a place that holds cards, with its rules; a `<hand />` object is the player's cursor that points at piles and carries cards between them. Piles and the hand know nothing about suits; suits times ranks is only the first way a deck is made, so a deck of single cards can come later with the same piles.
- **A card is an ordinary object** (`cards.1` to `cards.52`), with variables `suit` and `rank` (numbers from 1, in the order written) and two looks, `face` and `back`. So conditions, sounds and looks work on cards as on anything.
- **The game draws its suits, the engine draws the card.** Each `<suit>` gives its colour and its pip as bitmap rows; ranks are labels (`A`, `2`, ... `K`, or `F C R` for the Italian court) drawn in a small built-in pixel font. The engine lays out each face (rank and pip in two corners, a big pip in the middle) as a bitmap, so every window library draws it as it draws any bitmap. Writing 52 faces by hand was the alternative, and is what a deck of single cards will do.
- **Faces at full detail, from parts** (2026-11-01, Beef: "splurge a little... a lot of image data in the XML is fine as long as it's organized"): the game draws each part once, as rows: a suit's pip, corner pip and ace pip; a rank's corner index, its pips as a grid of characters (`*`, `v` turned, `@` the ace's), or a court `<picture>` drawn as its top half (`<half />`) and completed turned round. The engine only places them, so 52 faces come from 4 suits and 13 ranks, and a card is 63 by 93 pixels at scale 1 (three times the first deck's detail). The grid was chosen over pixel positions so a layout reads as the card looks; thirteen rows put both three and four pips down a side on it. Court pictures are shared by the suits, `*` taking the suit's colour. Added `color.gold` and `color.peach` for them.
- **Pile rules are data, like `<moves>`:** what a pile `<takes>` (`<one />`, `<samesuit />`, `<othercolor />`, `<up />`, `<down />`, and `<empty><rank>K</rank></empty>` for the first card), whether a `<run />` can be taken from it, whether a face-down card left on top turns up (`<showtop />`), and `faces="up|down"` for piles that turn every card one way. The stock's behaviour is two properties: `<deals into="waste">1</deals>` and `<refill from="waste" />`.
- **The deal is the piles':** `<deck pile="stock">` puts the shuffled deck in one pile, and each pile's `<dealt>N</dealt>` takes N from it, in rounds, as a dealer does. A group of piles can say `<dealt>1<more>1</more></dealt>` (one, then one more each), which is Klondike's staircase without writing seven piles. A new game (a full reset) shuffles and deals again.
- **The hand works like Checkers' picker:** `<take />`, `<lay />` (refused when the pile does not take the cards, so what follows does not happen), `<drop />` puts back, `<deal />` deals from the pile pointed at. `<lay pile="foundations" />` lays on the first of those piles that takes the cards (the right click sends a card home). Laying back on the pile the cards came from is always allowed: it is putting back.
- **Moving the hand:** `<hop>` on a hand goes left and right to the next pile in the row (piles at the same height), up and down through the face-up cards of a fanned pile and then to the row above or below; `<track />` points at the top card under the pointer, and held cards follow the pointer. So a press, drag and release with the mouse, and Space twice with the keys, are both a move. Moved by the mouse, the hand takes, deals and lays only with the pointer over a pile or a card (Beef: a click on the table must do nothing and make no sound); with the keys it always points at a pile.
- **Drawing order:** cards are drawn in pile order, held cards above them and the hand above all, by a drawing order the engine keeps on objects (`drawOrder`, 0 for everything else, which keeps the file's order).
- **Live number `cards`** on a pile: how many it holds, so "all 52 on the foundations" is an ordinary condition with `total="sum"`.

## Looked at and left

- **Cards as pieces on a map of cells:** a fanned column could be cells, but a stock or a foundation holds many cards in one place, and a run moves as one.
- **A key per pile** (press 1 to 7): not the key convention, and the mouse needs pointing anyway.
- **Turning the top card up by a press**: most computer versions of Klondike turn it at once; `<showtop />` says that.

## Open

- A deck of single cards (each its own picture and variables), for games where every card is different.
- Hands of cards held by players, and a computer playing a card game (Crazy Eights is next).
- Dealing three at a time from the stock, undo, scoring, an automatic finish.
