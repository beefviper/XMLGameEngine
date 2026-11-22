# Lists

**Status:** Built: a holder plus four general changes (the author agreed). Simon and Mastermind are open.

## What we looked for

Four games want one thing: a sequence the game adds to and reads by place.

| Game | What it does with the sequence |
|---|---|
| Tetris | a bag of seven shapes, shuffled, dealt one at a time, refilled when empty |
| Pac-Man | the wave lengths (scatter 7, chase 20, ...), read one after another, then chase for good |
| Simon | a colour added at random each round, the presses compared with it in order |
| Mastermind | a hidden row compared with a guess, place by place |

A [holder](27-cards.md) is already an ordered collection: `<send>` moves the top, `<order><random /></order>` shuffles, `<order><by>` keeps it sorted, live `held` counts it. Before adding a `<list>`, the fraction rule says try the holder: a pack of small pieces no one sees, each carrying one variable.

## Trial 1: the bag of seven

A scratch copy of Tetris, no engine change. It works: over many runs every seven pieces are the seven shapes, then a new shuffle.

```xml
<group name="tokens" class="token">
  <columns name="shape">7</columns>
  <rows>1</rows>
  <sprite name="waiting" />
  <sprite name="up" />
  <position><x>0</x><y>0</y></position>
  <actions>
    <action name="deal">
      <condition variable="shape"><equal>1</equal><spawn object="i" into="next" /></condition>
      ... one for each of the seven
    </action>
  </actions>
</group>
<object name="bag"> ... <holds class="token" sprite="waiting" />
  <actions>
    <action name="draw">
      <send from="bag" into="drawn">1</send>
      <trigger object="tokens" from="up" action="deal" />
      <send from="drawn" into="used">1</send>
      <condition variable="held">
        <equal>0</equal>
        <send from="used" into="bag"><all /></send>
        <order group="bag"><random /></order>
      </condition>
    </action>
  </actions>
</object>
```

What read badly:

- **About 45 lines where `<random />` was one.** Three holders (`bag`, `drawn`, `used`), each with an empty `<sprite />` and a position no one sees.
- **A number cannot pick a piece.** Seven `<condition>`s turn the token's number into a shape. A member cannot carry an action of its own, and `<spawn>` has no way to take "member N" of a group.
- **The top cannot be read.** Nothing outside `<accepts>` reads the top token's variable. The workaround sends it into a holder of its own (`drawn`, laid with look `up`) so that a `<trigger from="up">` reaches only it.
- **`<top sprite>` is not kept up by `<send>` or `<order>`.** It turns only after a deal or a picker's laying, so it could not single out the top. This looks like a gap in the engine, not a design choice.
- **A spawn lands at the end of the frame,** so the start needed two timers: the first piece is dealt, then a later timer sends it to the well and deals the next.

## Trial 2: Pac-Man's schedule

A scratch copy of Pac-Man, no engine change. Its scatter-and-chase test passes on every seed tried. The mode's two timers now read `mode.for`, and each switch draws the next length:

```xml
<group name="waves" class="wave">
  <columns>6</columns>
  ...
  <actions><action name="read"><copy who="self" variable="seconds" into="mode.for" /></action></actions>
  <variables><variable name="seconds">20</variable></variables>
  <column><number>1</number><number>3</number><variables><variable name="seconds">5</variable></variables></column>
  <column><number>5</number><variables><variable name="seconds">7</variable></variables></column>
</group>
<object name="schedule"> ... <holds class="wave" sprite="waiting"><order><by variable="column" /></order></holds></object>
```

What read well and badly:

- **Good: "for good" came free.** When the schedule is empty the chase timer's `<send>` is refused, so the rest of the timer (going back to scatter) never runs. The game's arithmetic trick (`20 + 100000 × (waves ≥ 4)`) goes, and the file is 8 lines shorter.
- **Written last first.** The last one laid is the top, so the columns hold the waves backwards.
- **`<starts>` always shuffles.** Its `<send>` of a pack into a holder deals at random even with no `<order>`. The schedule kept its order only by `<order><by variable="column" />` on the holder.
- **The same reading dance as the bag:** a `drawn` holder, a trigger from its look, a `<copy>`.

## What Simon and Mastermind would still need

Neither can be said today, even badly.

- **Reading by place.** Simon compares the fifth press with the fifth colour, so it needs the member at place N, not only the top.
- **Comparing two holders place by place.** Mastermind counts places that match, then colours that match anywhere. This is a count over two sequences, which no word does.
- **Adding without a fixed pack.** Simon's sequence grows each round. A spawn into a holder (a new token laid on top) would do it. Today a pack is fixed at load.

## What we chose

**A holder plus a few general words, not a `<list>`.** Both trials worked, and every awkward part was a small gap that serves cards and boards as well:

1. **Live `top.<variable>` on a holder** (`bag.top.shape`, `schedule.top.seconds`), the name `<accepts>` already uses. A holder has one for each variable of its held class, 0 while it is empty. It removed the `drawn` holder, the trigger and the copy from both trials. A name like `bag.top.shape` resolves to its owner by the longest start that names an object.
2. **`<spawn>` by member number:** `<member>bag.top.shape</member>` beside `<random />`, counting from 1, refused past the last (so an empty bag's 0 makes nothing). It removed the seven conditions.
3. **`<top sprite>` kept after the deal and every `<send>` and `<order>`,** as it already was after a picker laid. A fix.
4. **`<starts>` deals in pack order, the first member on top,** shuffling only where an `<order><random /></order>` says. Gin and Spades, which relied on the old shuffle, now say it. The schedule is written first to last.

Also: a held member may have one look or none (a token no one sees); two were required before.

The bag is now about twenty lines: the tokens, `bag`, `used`, and in the land action a `<spawn>`, a `<send>` and the refill. Pac-Man's mode lost its variables and the arithmetic, its timers reading `schedule.top.seconds`, each ending by sending the top wave off.

Rejected: a separate `<list>` of bare numbers. It would repeat `<send>`, `<order>` and `held` under new names, and two ways to hold an order is the kind of emerged assumption to reduce.

Simon and Mastermind are a second step, decided when one of them is built: place-by-place reading (a `place=` on a condition about a holder) and a spawn into a holder.

## Open

- The frame-end spawn in trial 1 is general: a `<send>` cannot follow a `<spawn>` in one list. Not part of this choice.
- The trial files (before the four changes) are kept outside the repository for reference.
