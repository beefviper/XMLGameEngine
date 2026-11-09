# Cards

There are no card words as such. A pack is a `<group>`, a pile is anything that [`<holds>`](#holders) its class, and a hand is a picker; the players are a [group that takes turns](states.md#players-as-a-group). Rules of a card game (what is a pair, who wins a trick) are written in the game from general parts; there is no tag for any one game.

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
      <condition variable="color"><unequal>top.color</unequal></condition>
      <condition variable="rank"><equal>top.rank - 1</equal></condition>
      <empty><condition variable="rank"><equal>13</equal></condition></empty>
    </accepts>
    <carry />
    <top sprite="face" />
  </holds>
</group>
```

- **The pack** is a group in `<columns>` and `<rows>` whose class a `<holds>` names. Each cell is a card: its row is its `suit`, its column its `rank` (variables, from 1), its first look the face and its second the back. A sheet `<cell>` that leaves out its column or row takes the cell's own, so the faces are one sprite. `worth` is the rank (its column) unless a column, row or cell gives one (`<column><number>11</number><number>12</number><number>13</number><variables><variable name="worth">10</variable></variables></column>`); a **`high`** variable is a second value it may count instead where a holder counts within a limit (Blackjack's ace: `high` 11).
- **`<holds class sprite>`** on an object or group makes it a holder: what it holds, in order, the last on top, each showing the look `sprite=` while laid there (none: each keeps its own).
- **`<spread>`**: each one on from the one under it (`<x>`, `<y>`, `<width>` the most it spreads across); a second with `sprite=` for those showing that look.
- **`<accepts>`**: what may be placed on top, as `<condition>`s on the arriving one, all of which must hold. Each compares its `variable=` with `<equal>`, `<unequal>`, `<above>`, `<below>`, `<atleast>` or `<atmost>` a number, or with a variable of the top one, plus or minus a number if wanted: `<equal>top.suit</equal>`, `<unequal>top.color</unequal>`, `<equal>top.rank + 1</equal>`, `<below>top.worth</below>`. `total="sum"` adds that variable up over what is laid at once (`<equal>13</equal>`: Pyramid's pairs); `<condition total="count"><equal>1</equal></condition>` lays one at a time. `<any>` holds conditions of which one must hold; `<empty>` holds conditions on the first, where there is no top. `<accepts />` takes anything; no `<accepts>`: nothing.
- **`<wild variable="suit">`** in `<accepts>`, with `<condition>`s: what keeps them goes on anything. The holder then waits (live `calling` 1) until the one who laid it names a value with `<call pile>`: the `suit` of the one it points at among its own. Live `called` is that value, and the top counts as it until the next one is laid. A wild one dealt on top counts as its own.

  ```xml
  <accepts>
    <any>
      <condition variable="suit"><equal>top.suit</equal></condition>
      <condition variable="rank"><equal>top.rank</equal></condition>
    </any>
    <wild variable="suit"><condition variable="rank"><equal>8</equal></condition></wild>
  </accepts>
  ```
- **`<carry />`**: one may be picked with all those on it, while each follows the rules. **`<top sprite="face" />`**: the top one turns face up. **`<lift>16</lift>`**: the one a picker points at is drawn lifted by that much. **`<order><by variable="suit" /><by variable="rank" /></order>`**: kept in that order, the first first.
- **`<picked>`**: what picking from it does instead, picking nothing up: `<send into="waste">1</send>` (no `into=`: to the picker's own holder), and `<empty>` with `<send from="waste"><all /></send>` (a `<leave>` in it) and `<order><random /></order>`; `<play sound>` sounds as it sends.
- **The picker.** An object whose actions `<pick class>` a held class is a hand: `<pick class="card" />` takes what it points at, `<place />` lays it where it points, `<place into="foundations" />` on the first of those that accepts it, `<drop />` puts it back. **`<owns object="mine" />`** gives it a holder of its own: it picks any of those, face up or down, and stays pointing there. [`<hop>`](commands.md#moving) moves it left and right to the nearest holder at the same height, and up and down through a spread holder's face-up cards, then to the next row of holders. [`<track />`](input.md#pointing-controls) points at the top card under the pointer; held cards follow it, so press, drag and release (`on="release"`) move cards as Space twice does. Moved by the mouse, it picks and places only over a holder or card.
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
- Live **`held`** is how many a holder holds (`cards` as well), and more below: `points`, `calling`, `called`, `pattern`, `strength`, `odds`, `lead`, `winner`, `tricks`, `deadwood`. `<condition object="foundations" variable="cards" total="sum"><atleast>52</atleast></condition>` is Klondike's win.
- **Covering.** A holder's top card can be picked only while no card of a later holder overlaps it. Holders that only touch are never covered.
- **Drawing order.** Cards are drawn above everything, in their holders' order, held cards above them and the picker above all.
- **`games/assets/cards.png`** is the shared [sheet](pictures.md#sheets): 2496 by 1400, cells of 192 by 280 (twice a 96 by 140 card), the four suits in the order spades, hearts, clubs, diamonds, ranks A to K, and a row of eight backs (blue, red, green, black and gold, purple, then blue, red and green lattices; column 2 of row 5 is the red). All eight card games use it. **To change the cards' look**, replace the file with another of the same layout at any size.

## Patterns

What a set of cards can make is written once in the game, beside `<styles>`, lowest first. Each `<pattern>` is parts that must all hold, each naming the variable it compares: `<alike variable="rank">N</alike>` (N of one rank; two of them: two ranks), `<alike variable="suit">N</alike>` (N of one suit), `<run variable="worth">N</run>` (N worths in a row, a card's `high` counting too). `<run variable="rank">` runs by column, the ace low whatever it is worth (Gin's runs, its court cards worth 10); for now only `<melds />` take it.

```xml
<patterns name="poker">
  <pattern name="pair"><alike variable="rank">2</alike></pattern>
  <pattern name="twopair"><alike variable="rank">2</alike><alike variable="rank">2</alike></pattern>
  <pattern name="three"><alike variable="rank">3</alike></pattern>
  <pattern name="straight"><run variable="worth">5</run></pattern>
  <pattern name="flush"><alike variable="suit">5</alike></pattern>
  <pattern name="fullhouse"><alike variable="rank">3</alike><alike variable="rank">2</alike></pattern>
  <pattern name="four"><alike variable="rank">4</alike></pattern>
  <pattern name="straightflush"><run variable="worth">5</run><alike variable="suit">5</alike></pattern>
</patterns>
```

A holder (Hold'em's: the ace's `high` 14) scores by them with `<score patterns="poker"><with pile="board"><full>5</full></with><best>5</best></score>`: with that holder's cards (`<full>`: how many it holds once dealt), the best five of all. Live numbers:

- `pattern`: the highest made (from 1; 0 none);
- `strength`: orders any two the same size (pattern, then the run from its top, else rank groups bigger first, then the rest), so the showdown is `<above>theirs.strength</above>`;
- `odds`: in percent, how often the cards beat as many unseen ones (five hundred deals, the same each time for the same cards), worked out only for a pile something reads them of: a condition, a shown text or an expression.

### Melds

`<score patterns="rummy"><melds /><best>10</best></score>` splits the cards into groups that each make a pattern with all their cards, leaving the fewest points over (each card's `worth`). The holder is laid out in its groups. Live `deadwood` is the points left over by the best `<best>` cards.

```xml
<patterns name="rummy">
  <pattern name="set"><alike variable="rank">3</alike></pattern>
  <pattern name="run"><run variable="rank">3</run><alike variable="suit">3</alike></pattern>
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
- A member of [players in turn](states.md#players-as-a-group) sits at its number in the group, so `<pass member="trick.winner" />` gives the winner the lead (Hearts, Spades); a member's **`<team>`** is its side (partners share one).
- Live **`lead`** (the suit led) and **`winner`** (the number of the player who laid the highest card).
- Live **`tricks`** on a player's own holder: how many tricks its cards should take, counted as a bidder does (high cards, trump length, short suits).

## Covering and pairs

Pyramid: a picker's **`<pick class="card"><most>2</most></pick>`** may take the top card of another holder while holding, from that many holders in all; `<drop />` puts each back. A holder's **`<accepts><condition variable="worth" total="sum"><equal>13</equal></condition></accepts>`** takes what is laid at once when their worths add up to that (a king alone). So a pair is `<pick class="card"><most>2</most></pick>` then `<place into="pairs" />`, else `<drop />`.

## The computer

A players member's [`<think button="space" />`](states.md#computer-players), with a holder it `<owns>`, plays cards by rules of thumb, not a search: every half second it points at what it will do and presses the button, and the game's own actions do the move.

- **Shedding** (Crazy Eights): when a call is due, the suit it holds most of; otherwise a card some holder accepts, eights last, then its longest suit, then the highest; else it draws.
- **Melds** (Gin Rummy): it draws the discard when that leaves less deadwood, else the stock; it lets go of the card that leaves least.
- **Tricks**: `<think>` takes tricks: it leads an ace, else its lowest side card; lets a partner's winner stand; else plays the lowest card that wins. With **`<avoid />`** it keeps out of them: leads its card of fewest points, ducks with its highest loser, sheds the most points when void.
