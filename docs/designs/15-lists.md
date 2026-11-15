# Lists

**Status:** Open. Two trials made with today's words; the author picks the way.

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
  <actions><action name="read"><copy variable="self.seconds" into="mode.for" /></action></actions>
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

## Recommendation

**A holder plus a few general words, not a `<list>`.** Both trials worked, and every awkward part is a small gap that would serve cards and boards as well. In order of how much they cut:

1. **Live `top.<variable>` on a holder** (`bag.top.shape`, `schedule.top.seconds`), the same name `<accepts>` already uses. It removes the `drawn` holder, the trigger and the copy in both trials.
2. **`<spawn>` by member number:** `<spawn object="pieces" into="next"><member>bag.top.shape</member></spawn>`, in the way `<random />` picks one. It removes the seven conditions.
3. **`<top sprite>` kept after every `<send>` and `<order>`.** A fix, not a word.
4. **`<starts>` deals in pack order,** shuffling only where an `<order><random /></order>` says so. The card games already write one, so none changes.

With 1 and 2 the bag is about fifteen lines: the tokens, `bag`, `used`, and a land action of `<spawn>`, `<send>` and the refill condition. The schedule loses its `drawn` holder and its `read` action.

Simon and Mastermind are a second step, decided when one of them is built: place-by-place reading (a `place=` on a condition about a holder) and a spawn into a holder. Rejected for now: a separate `<list>` of bare numbers. It would repeat `<send>`, `<order>` and `held` under new names, and two ways to hold an order is the kind of emerged assumption to reduce.

## Open

- The author's choice: these four, a `<list>`, or leave the two trials as they read today.
- The frame-end spawn in trial 1 is general: a `<send>` cannot follow a `<spawn>` in one list. Not part of this choice.
- The trial files are kept outside the repository for reference.
