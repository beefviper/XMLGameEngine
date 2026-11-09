# Cards

Cards are made by a `<deck>`, lie in objects that are **piles**, and are carried between piles by a **hand**. Rules of a card game (what is a pair, who wins a trick) are written in the game from general parts; there is no tag for any one game.

The card words are moving to general ones: a pack is a `<group>`, a pile is anything that [`<holds>`](#holders), and the hand is a picker; the players are a [group that takes turns](states.md#players-as-a-group). Klondike and Crazy Eights are written that way; the other card games still use `<deck>`, `<pile>` and `<hand />`, described after it.

## Holders

```xml
<!-- the pack: a row for each suit, a column for each rank -->
<group name="cards" class="card">
  <columns>13</columns>
  <rows>4</rows>
  <sprite name="face"><cell sheet="cards"><scale>0.5</scale></cell></sprite>
  <sprite name="back"><cell sheet="cards"><column>1</column><row>5</row><scale>0.5</scale></cell></sprite>
  <position><x>0</x><y>0</y></position>
  <variables><variable name="color">1</variable></variables>
  <row><number>2</number><number>4</number><variables><variable name="color">2</variable></variables></row>
</group>

<group name="columns" class="place">
  ...
  <holds class="card">
    <spread><y>32</y></spread>
    <spread sprite="back"><y>9</y></spread>
    <accepts>
      <differs variable="color" />
      <step variable="rank">-1</step>
      <empty><condition variable="rank"><equal>13</equal></condition></empty>
    </accepts>
    <run />
    <top sprite="face" />
  </holds>
</group>
```

- **The pack** is a group in `<columns>` and `<rows>` whose class a `<holds>` names. Each cell is a card: its row is its `suit`, its column its `rank` (variables, from 1), its first look the face and its second the back. A sheet `<cell>` that leaves out its column or row takes the cell's own, so the faces are one sprite. `worth` is the rank (its column) unless a column, row or cell gives one (`<column><number>11</number><number>12</number><number>13</number><variables><variable name="worth">10</variable></variables></column>`); a **`high`** variable is a second value it may count instead where a holder counts within a limit (Blackjack's ace: `high` 11).
- **`<holds class sprite>`** on an object or group makes it a holder: what it holds, in order, the last on top, each showing the look `sprite=` while laid there (none: each keeps its own).
- **`<spread>`**: each one on from the one under it (`<x>`, `<y>`, `<width>` the most it spreads across); a second with `sprite=` for those showing that look.
- **`<accepts>`**: what may be placed on top, by the arriving one's variables. `<same variable>`, `<differs variable>`, `<step variable>N` (the top one's plus N), `<total variable>N` (the two add up to N), `<condition variable>` with `<equal>`, `<atleast>` or `<atmost>`; `<one />` one at a time; `<any>` holds rules of which one must hold; `<empty>` holds `<condition>`s for the first. No `<accepts>`: takes nothing.
- **`<wild variable="suit">`** in `<accepts>`, with `<condition>`s: what keeps them goes on anything. The holder then waits (live `calling` 1) until the one who laid it names a value with `<call pile>`: the `suit` of the one it points at among its own. Live `called` is that value, and the top counts as it until the next one is laid. A wild one dealt on top counts as its own.

  ```xml
  <accepts>
    <any><same variable="suit" /><same variable="rank" /></any>
    <wild variable="suit"><condition variable="rank"><equal>8</equal></condition></wild>
  </accepts>
  ```
- **`<run />`**: one may be picked with all those on it, while each follows the rules. **`<top sprite="face" />`**: the top one turns face up. **`<lift>`** as a pile's. **`<order><by variable="suit" /><by variable="rank" /></order>`**: kept in that order, the first first.
- **`<picked>`**: what picking from it does instead, picking nothing up: `<send into="waste">1</send>` (no `into=`: to the picker's own holder), and `<empty>` with `<send from="waste"><all /></send>` (a `<leave>` in it) and `<order><random /></order>`; `<play sound>` sounds as it sends.
- **The picker.** An object whose actions `<pick class>` a held class is a hand: `<pick class="card" />` takes what it points at, `<place />` lays it where it points, `<place into="foundations" />` on the first of those that accepts it, `<drop />` puts it back. Moving it is as for the hand below. **`<owns object="mine" />`** gives it a holder of its own, as `<hand pile>` does: it picks any of those, face up or down, and stays pointing there.
- **`<starts>`** in a state is the deal, done as it begins and on a full reset:

  ```xml
  <starts>
    <send from="cards" into="stock"><all /></send>
    <order group="stock"><random /></order>
    <distribute from="stock" into="columns">1<more>1</more></distribute>
  </starts>
  ```

  For now it sends all of a pack into one holder, orders it at random, then `<distribute>`s N to each holder of a group (one at a time round; `<more>`: each after the first that many more) or `<send>`s N to one.
- **`<counts variable="worth"><atmost>21</atmost></counts>`** in a `<holds>`: the holder's own variable of that name (`player.worth`) is what its face-up members' variables of that name add up to. For `worth`, a member's `high` counts instead while the total stays within `<atmost>`.
- **The verbs, in any action, timer or condition:** `<send from="stock" into="player">1</send>` sends the top one (or `<all />`) from holder to holder; from the pack's name, `<all />` is every card wherever it is. `sprite="back"` lays them showing that look. `<order group="stock"><random /></order>` puts what a holder holds at random. `<distribute from="stock" into="mine">13</distribute>` sends that many to a holder, or, into a group, that many to each member's own holder, a card a round. **`<become object="dealer" sprite="face" />`** on a holder, with a look of what it holds rather than its own, turns everything it holds to that look. Each is refused when there is nothing to send or no such holder or look.

  ```xml
  <action name="next">
    <send from="cards" into="stock"><all /></send>
    <order group="stock"><random /></order>
  </action>
  ```
- Live **`held`** is how many a holder holds (`cards` as well).

## Deck

```xml
<sheets>                                             <!-- at the top of the game -->
  <sheet name="cards"><path>assets/cards.png</path><columns>13</columns><rows>5</rows></sheet>
</sheets>

<deck name="cards" pile="stock" sheet="cards">
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
- **`sheet=`** names one of the game's [`<sheets>`](pictures.md#sheets): every face and the back are cut from it and made `<face>`'s size (times `<scale>`). Left as it is, **the faces are a row for each suit and a column for each rank, in the order written**, and the back is the first cell after them (column 1 of the next row).
- **`<faces>`** says otherwise, before `<face>`: `<faces><down /></faces>` is a column for each suit and a row for each rank; or the faces in the deck's order (suit by suit, rank by rank) as `<cell>`s and runs of `<cells>`, so a suit can be split over two rows:

  ```xml
  <faces>
    <cells><row>1</row><count>7</count></cells>               <!-- spades A to 7 -->
    <cells><row>2</row><count>6</count></cells>               <!-- spades 8 to K -->
    ...
  </faces>
  <back><column>3</column><row>9</row></back>                 <!-- or <x> <y> <width> <height> -->
  ```

  A run starts at its `<column>` and `<row>` (each 1 when left out) and goes `<count>` cells along the row, or down the column with `<down />`. Faces that do not make one for every card, or a cell off the sheet, stop the load. With a sheet, `<back>` picks the back's cell, and the face's colours and drawn parts go unused.
- **`games/assets/cards.png`** is the shared sheet: 2496 by 1400, cells of 192 by 280 (twice a 96 by 140 card), the four suits in the order spades, hearts, clubs, diamonds, ranks A to K, and a row of eight backs (blue, red, green, black and gold, purple, then blue, red and green lattices; `<back><column>2</column><row>5</row></back>` is the red). All eight card games use it. **To change the cards' look**, replace the file with another of the same layout at any size, or describe another layout with the sheet's grid and `<faces>`.

### A drawn deck

Without `sheet=`, the engine puts each face together from parts the game draws as rows, so every window library draws them:

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
- **`<back>`**: a frame round its `<pattern>`, or a lattice of diamonds. A deck with `sheet=` takes its back from the sheet instead.

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

What a set of cards can make is written once in the game, beside `<styles>`, lowest first. Each `<pattern>` is parts that must all hold, each naming the variable it compares: `<same variable="rank">N</same>` (N of one rank; two of them: two ranks), `<same variable="suit">N</same>` (N of one suit), `<run variable="worth">N</run>` (N worths in a row, a card's `high` counting too). `<run variable="rank">` runs by column, the ace low whatever it is worth (Gin's runs, its court cards worth 10); for now only `<melds />` take it. Old forms: no `variable=` (rank, worth) and `<suited>N`.

```xml
<patterns name="poker">
  <pattern name="pair"><same variable="rank">2</same></pattern>
  <pattern name="twopair"><same variable="rank">2</same><same variable="rank">2</same></pattern>
  <pattern name="three"><same variable="rank">3</same></pattern>
  <pattern name="straight"><run variable="worth">5</run></pattern>
  <pattern name="flush"><same variable="suit">5</same></pattern>
  <pattern name="fullhouse"><same variable="rank">3</same><same variable="rank">2</same></pattern>
  <pattern name="four"><same variable="rank">4</same></pattern>
  <pattern name="straightflush"><run variable="worth">5</run><same variable="suit">5</same></pattern>
</patterns>
```

A pile or a holder (Hold'em's: the ace's `high` 14) scores by them with `<score patterns="poker"><with pile="board"><full>5</full></with><best>5</best></score>`: with that pile's cards (`<full>`: how many it holds once dealt), the best five of all. Live numbers:

- `pattern`: the highest made (from 1; 0 none);
- `strength`: orders any two the same size (pattern, then the run from its top, else rank groups bigger first, then the rest), so the showdown is `<above>theirs.strength</above>`;
- `odds`: in percent, how often the cards beat as many unseen ones (a thousand deals, the same each time for the same cards).

### Melds

`<score patterns="rummy"><melds /><best>10</best></score>` splits the cards into groups that each make a pattern with all their cards, leaving the fewest points over (each card's `worth`). The holder is laid out in its groups. Live `deadwood` is the points left over by the best `<best>` cards.

```xml
<patterns name="rummy">
  <pattern name="set"><same variable="rank">3</same></pattern>
  <pattern name="run"><run variable="rank">3</run><same variable="suit">3</same></pattern>
</patterns>
```

## Tricks

```xml
<group name="cards" class="card"> ...
  <row><number>2</number><variables><variable name="points">1</variable></variables></row>
  <column><number>1</number><variables><variable name="worth">14</variable></variables></column>
  <cell row="1" column="12"><variables><variable name="points">13</variable></variables></cell>
</group>
<object name="trick" class="place"> ...
  <holds class="card" sprite="face">
    <spread><x>64</x></spread>
    <trick variable="suit"><breaks>2</breaks></trick>
    <counts variable="points" />
  </holds>
</object>
```

- **`<trick variable="suit">`** in a `<holds>`: the first card leads any suit; after it, a player holding the suit led must follow. The highest `worth` of the suit led wins (an ace-high pack gives column 1 `worth` 14). For now the variable is always `suit`, the row.
- **`<breaks>2</breaks>`**: a suit (its row) that may not lead until one has been laid since the deal, unless the leader holds nothing else.
- **`<trumps>1</trumps>`**: a card of that suit beats every other suit (Spades).
- Points are card variables (`points` on the hearts row and the queen of spades' cell), added up by the trick's **`<counts variable="points" />`** into `trick.points`; `<think><avoid />` sheds the cards it counts.
- The deal, in `<starts>` and between hands: `<send from="cards" into="stock"><all /></send>`, `<order group="stock"><random /></order>` and **`<distribute from="stock" into="players">13</distribute>`**, which deals a card a round to each member's own holder (`<owns>`) in turn.
- Old form, on a `<pile>`: `<trick><breaks><suit>hearts</suit></breaks><trumps><suit>spades</suit></trumps></trick>` with `<points><each suit rank>`.
- A hand's **`<seat>`** is its place; **`<team>`** its side (partners share one). A member of [players in turn](states.md#players-as-a-group) with a `<hand />` sits at its number in the group, so `<pass member="trick.winner" />` gives the winner the lead (Hearts, Spades); its `<team>` is the member's own.
- Live **`lead`** (the suit led) and **`winner`** (the seat of the highest card).
- Live **`tricks`** on a hand's own pile: how many tricks its cards should take, counted as a bidder does (high cards, trump length, short suits).

## Covering and pairs

Pyramid: a hand's **`<holds>2</holds>`** may take the top card of another pile while holding, from that many piles in all; `<drop />` puts each back. A pile's **`<takes><sum>13</sum></takes>`** takes all the cards held when their worths add up to that. So a pick is `<take />` then `<lay pile="pairs" />`, else `<drop />`.

## The computer

[`<think object="cpuhand" button="f12" />`](states.md#computer-players) on a hand with its own pile plays cards by rules of thumb, not a search: every half second it points the hand at what it will do and presses the button, and the game's own actions do the move.

- **Shedding** (Crazy Eights): when a call is due, the suit it holds most of; otherwise a card some pile takes, eights last, then its longest suit, then the highest; else it draws.
- **Melds** (Gin Rummy): it draws the discard when that leaves less deadwood, else the stock; it lets go of the card that leaves least.
- **Tricks**: `<think>` takes tricks: it leads an ace, else its lowest side card; lets a partner's winner stand; else plays the lowest card that wins. With **`<avoid />`** it keeps out of them: leads its card of fewest points, ducks with its highest loser, sheds the most points when void.
