# Cards

Cards are made by a `<deck>`, lie in objects that are **piles**, and are carried between piles by a **hand**. Rules of a card game (what is a pair, who wins a trick) are written in the game from general parts; there is no tag for any one game.

## Deck

```xml
<deck name="cards" pile="stock">
  <sheet><path>assets/cards.png</path><back>1</back></sheet>
  <face><width>96</width><height>140</height></face>
  <suit name="spades"><color>color.black</color></suit>
  <suit name="hearts"><color>color.red</color></suit>
  <suit name="clubs"><color>color.black</color></suit>
  <suit name="diamonds"><color>color.red</color></suit>
  <rank name="A"><worth>14</worth></rank>
  <rank>2</rank><rank>3</rank> ... <rank>K</rank>
</deck>
```

- **The cards.** A card for each `<suit>` and `<rank>`, in the order written, `<copies>` times (1 when left out): objects `cards.1`, `cards.2` ... with variables `suit` and `rank` (from 1, as written) and two looks, `face` and `back`. `class=` gives them a class. Any deck works the same way (an Italian 40).
- **A suit** gives its name and `<color>` (what `<othercolor />` compares); **a rank** its name and its `<worth>`s ([points](#points)).
- **`<sheet>`**: one picture file (PNG, JPEG, BMP) holding every face, **a row for each suit and a column for each rank, in the order written, then one row of backs**. `<back>` picks a back, from 1 (default 1). The engine reads the file itself and makes each face and the back `<face>`'s size (times `<scale>`), each pixel the average of the ones it covers, so the sheet can be far bigger than the cards; a whole-number shrink gives pixel art back exactly. The file is held only while it is cut. A sheet that does not split evenly into those rows and columns, or an empty or missing back, stops the load.
- **`games/assets/cards.png`** is the shared sheet: 4992 by 2800, cells of 384 by 560, the four suits in the order spades, hearts, clubs, diamonds, ranks A to K, and eight backs (blue, red, green, black and gold, purple, then blue, red and green lattices). All eight card games use it.

### A drawn deck

Without a `<sheet>`, the engine puts each face together from parts the game draws as rows, so every window library draws them:

```xml
<deck name="cards" pile="stock">
  <face><width>63</width><height>93</height><scale>1</scale><color>color.white</color><edge>color.grey</edge></face>
  <back><color>color.darkblue</color><edge>color.lightgrey</edge><pattern><row>*.....*.</row> ... </pattern></back>
  <color char="y">color.gold</color>                 <!-- colours for the court pictures -->
  <suit name="hearts">
    <color>color.red</color>
    <row>.***...***.</row> ...                       <!-- the pip -->
    <index><row>.**.**.</row> ... </index>           <!-- the corner pip -->
    <large><row>....******....</row> ... </large>    <!-- the ace's pip -->
  </suit>
  <rank name="3">
    <index><row>.*****.</row> ... </index>
    <pips><row>.*.</row> <row>...</row> ... <row>.v.</row></pips>
  </rank>
  <rank name="K"><index> ... </index><picture><half /><row>***...</row> ... </picture></rank>
</deck>
```

- `<face>` is the card's size in bitmap pixels (63 by 93 by default) and its colours (`@` face and `#` edge in any part's rows).
- **A suit**: its pip as rows (`*` its colour, `.` the face); `<index>` the corner pip (default: the pip), `<large>` the ace's (default: the pip twice the size).
- **A rank**: just a name (drawn in a small 3x5 font with the pip big in the middle), or `<rank name>` with an `<index>` and either **`<pips>`** (a grid: `*` a pip, `v` one turned round, `@` the large pip, spread between the corners) or a **`<picture>`** (with `<half />` first, the rows are the top half and the bottom is them turned round).
- **`<back>`**: a frame round its `<pattern>`, or a lattice of diamonds. A deck has a `<sheet>` or a `<back>`, not both.

## Piles

A `<pile>` on an object (or each member of a group) makes it a place that holds cards.

```xml
<object name="stock">
  <sprite name="spot"> ... </sprite>
  <pile faces="down"><deals into="waste">1</deals><refill from="waste" /></pile>
</object>
<group name="columns">
  ...
  <pile>
    <fan><y>21</y><hidden><y>6</y></hidden></fan>
    <dealt>1<more>1</more></dealt>
    <takes><othercolor /><down /><empty><rank>K</rank></empty></takes>
    <run />
    <showtop />
  </pile>
</group>
```

| Part | Means |
|---|---|
| `faces="up"`, `"down"` | every card laid there turns that way (none: each keeps its own) |
| `<fan>` | each card's offset from the one under it; `<hidden>` a face-down card's; `<width>` the most it may spread across |
| `<dealt>N</dealt>` | takes N in the deal, a card a round, the last face up; `<more>1</more>` on a group deals each member after the first one more (Klondike's staircase) |
| `<takes>` | what it takes on top: `<one />`, `<samesuit />`, `<othercolor />`, `<up />`, `<down />`, `<matches />` (same suit or rank), `<wild>` ranks, `<sum>13</sum>`, `<empty>` with the ranks and suits it starts with. No `<takes>`: takes nothing; an empty `<takes />`: takes anything |
| `<run />` | a face-up card may be taken with all those on it |
| `<showtop />` | a face-down card left on top turns up |
| `<sorted />` | keeps its cards suit by suit in rank order |
| `<lift>16</lift>` | the card a hand points at is drawn lifted by that much |
| `<deals into="waste">1</deals>` | taking from it deals that many onto that pile (no `into=`: into the taking hand's own pile) |
| `<refill from="waste" />` | taken from empty, turns that pile back into itself; `<shuffle />` shuffles instead, `<leave>1</leave>` leaves that many |

- **The deal.** The shuffled deck goes to its `pile=`, then each pile takes its `<dealt>`. A new game (a full [reset](commands.md#in-and-out-of-play)) shuffles and deals again.
- **Covering.** A pile's top card can be taken only while no card of a later pile overlaps it. Piles that only touch are never covered.
- **Drawing order.** Cards are drawn above everything, in their piles' order, held cards above them and the hand above all.
- **Live numbers** of a pile, readable by conditions and a text's `<number>`: `cards`, `points`, `calling`, `called`, `pattern`, `strength`, `odds`, `lead`, `winner`, `tricks`, `deadwood` (below). `<condition object="foundations" variable="cards" total="sum"><atleast>52</atleast></condition>` is Klondike's win.

## The hand

An object with `<hand />` points at a pile (at a card of it) and carries cards. Its verbs are self verbs:

| Verb | Does | Refused when |
|---|---|---|
| `<take />` | takes the card pointed at and those on it; on a stock, deals | it is not a run the pile gives, or face down |
| `<lay />` | lays what it holds on the pile pointed at | the pile does not take it (laying back where they came from is always allowed) |
| `<lay pile="foundations" />` | lays on the first of those piles that takes it | none takes it |
| `<drop />` | puts what it holds back | |
| `<deal />`, `<deal pile="stock" faces="down" />` | deals one card from the pile pointed at, or from that pile into its own, turned that way | |
| `<redeal />` | a new hand: every card gathered, shuffled, dealt by each pile's `<dealt>` | |
| `<reveal pile="dealer" />` | turns that pile's cards face up | |
| `<shuffle pile="stock" />` | gathers every card into that pile face down and shuffles | |
| `<collect pile="trick" into="stock" />` | moves a pile's cards onto another face down, out of play until the next deal | |

**Moving the hand.** [`<hop>`](commands.md#moving) goes left and right to the nearest pile at the same height, and up and down through a fanned pile's face-up cards, then to the next row of piles. [`<track />`](input.md#pointing-controls) points at the top card under the pointer; held cards follow it, so press, drag and release (`on="release"`) move cards as Space twice does. Moved by the mouse, the hand takes and lays only over a pile or card.

## Players' cards

Crazy Eights gives each player a pile of their own:

```xml
<object name="mine">
  <pile faces="up"><fan><x>56</x><width>768</width></fan><dealt>7</dealt><sorted /></pile>
</object>
<object name="stock">
  <pile faces="down"><deals>1</deals><refill from="discard"><shuffle /><leave>1</leave></refill></pile>
</object>
<object name="discard">
  <pile faces="up"><dealt>1</dealt><takes><matches /><wild><rank>8</rank></wild></takes></pile>
</object>
<object name="hand"> ... <hand pile="mine" /> ... </object>
```

- **`<hand pile="mine" />`** takes only from its own pile, any card, face up or down, and keeps pointing into it.
- **Wild cards.** After a `<wild>` card the pile waits for a call: it takes nothing until a hand's `<call pile="discard" />` names the suit of the card it points at in its own pile. Then `<matches />` follows the suit called. Live `calling` is 1 while it waits; `called` is the suit (from 1; 0 none).

## Points

- A rank's **`<worth>`**s: what it counts for (`<worth>1</worth><worth>11</worth>` for an ace; none: its number as written). Each card gets a variable `worth`, its lowest.
- A pile's live **`points`** adds up its face-up cards' worths. `<points><atmost>21</atmost></points>`: a card that may count higher does while the total stays within 21.
- **By card**: `<points><each suit="hearts">1</each><each suit="spades" rank="Q">13</each></points>`. `rank=` alone each card of that rank; neither, every card. The first match counts; a card none matches counts its worth.

## Patterns

What a set of cards can make is written once in the game, beside `<styles>`, lowest first. Each `<pattern>` is parts that must all hold: `<same>N</same>` (N of one rank; two `<same>`s: two ranks), `<run>N</run>` (N worths in a row, any of a card's worths counting), `<suited>N</suited>` (N of one suit).

```xml
<patterns name="poker">
  <pattern name="pair"><same>2</same></pattern>
  <pattern name="twopair"><same>2</same><same>2</same></pattern>
  <pattern name="three"><same>3</same></pattern>
  <pattern name="straight"><run>5</run></pattern>
  <pattern name="flush"><suited>5</suited></pattern>
  <pattern name="fullhouse"><same>3</same><same>2</same></pattern>
  <pattern name="four"><same>4</same></pattern>
  <pattern name="straightflush"><run>5</run><suited>5</suited></pattern>
</patterns>
```

A pile scores by them with `<score patterns="poker"><with pile="board"><full>5</full></with><best>5</best></score>`: with that pile's cards (`<full>`: how many it holds once dealt), the best five of all. Live numbers:

- `pattern`: the highest made (from 1; 0 none);
- `strength`: orders any two the same size (pattern, then the run from its top, else rank groups bigger first, then the rest), so the showdown is `<above>theirs.strength</above>`;
- `odds`: in percent, how often the cards beat as many unseen ones (a thousand deals, the same each time for the same cards).

### Melds

`<score patterns="rummy"><melds /><best>10</best></score>` splits the cards into groups that each make a pattern with all their cards, leaving the fewest points over. The pile is laid out in its groups. Live `deadwood` is the points left over by the best `<best>` cards.

```xml
<patterns name="rummy">
  <pattern name="set"><same>3</same></pattern>
  <pattern name="run"><run>3</run><suited>3</suited></pattern>
</patterns>
```

## Tricks

```xml
<object name="trick">
  <pile faces="up"><fan><x>64</x></fan>
    <trick><breaks><suit>hearts</suit></breaks><trumps><suit>spades</suit></trumps></trick>
    <points><each suit="hearts">1</each><each suit="spades" rank="Q">13</each></points>
  </pile>
</object>
<object name="hand"> ... <hand pile="mine"><seat>1</seat><team>1</team></hand> ... </object>
```

- **`<trick>`** (in place of `<takes>`): the first card leads any suit; after it, a hand holding the suit led must follow.
- **`<breaks>`**: suits that may not lead until one has been laid since the deal, unless the leader holds nothing else.
- **`<trumps>`**: a card of those suits beats every other suit.
- A hand's **`<seat>`** is its place; **`<team>`** its side (partners share one).
- Live **`lead`** (the suit led) and **`winner`** (the seat of the highest card).
- Live **`tricks`** on a hand's own pile: how many tricks its cards should take, counted as a bidder does (high cards, trump length, short suits).

## Covering and pairs

Pyramid: a hand's **`<holds>2</holds>`** may take the top card of another pile while holding, from that many piles in all; `<drop />` puts each back. A pile's **`<takes><sum>13</sum></takes>`** takes all the cards held when their worths add up to that. So a pick is `<take />` then `<lay pile="pairs" />`, else `<drop />`.

## The computer

[`<think object="cpuhand" button="f12" />`](states.md#computer-players) on a hand with its own pile plays cards by rules of thumb, not a search: every half second it points the hand at what it will do and presses the button, and the game's own actions do the move.

- **Shedding** (Crazy Eights): when a call is due, the suit it holds most of; otherwise a card some pile takes, eights last, then its longest suit, then the highest; else it draws.
- **Melds** (Gin Rummy): it draws the discard when that leaves less deadwood, else the stock; it lets go of the card that leaves least.
- **Tricks**: `<think>` takes tricks: it leads an ace, else its lowest side card; lets a partner's winner stand; else plays the lowest card that wins. With **`<avoid />`** it keeps out of them: leads its card of fewest points, ducks with its highest loser, sheds the most points when void.
