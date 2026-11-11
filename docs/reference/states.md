# States and scenes

A **state** is a mode of the game. Every game names its four the same: `title`, `playing`, `paused` and `over`, the one end screen, whose message has a look per outcome picked by the condition that ends the game (`<become object="over.message" sprite="won" />` then `<push state="over" />`). Other states are moments inside play (`ready`, `point`). A state played in several places holds them as **scenes**. **Conditions** are tests checked every frame.

## States

`<states>` holds, in order, the game's own `<objects>` (in play the whole game, in each state that shows them), then the states. The first state in the file is where the game starts. `name` is required; `from` copies an earlier state. Children, in this order:

| Element | What it does |
|---|---|
| `<objects>` | the state's own objects, in play whenever it is current, in every scene of it |
| `<scenes>` | the places it is played in, one current at a time ([scenes](#scenes)) |
| `<shows>` | `<show object="best" />` runs another's object as its own; `<show state="playing" />` draws that state's objects **still** underneath |
| `<music sound>` | a looping sound plays while it is current (restarting after a pause); one that does not loop plays as it begins |
| `<inputs>` | the keys ([input](input.md)). Needed unless the state is `from=` another |
| `<conditions>` | tests checked every frame ([conditions](#conditions)) |
| `<computer>` | a player the game plays itself ([computer players](#computer-players)) |
| `<starts>` | done as it begins and on a full reset: the deal of a card game ([holders](cards.md#holders)) |
| `<timers>` | timers that count while it is current |

```xml
<state name="paused">
  <objects>
    <object name="paused"> ... </object>     <!-- the PAUSED text -->
  </objects>
  <shows>
    <show state="playing" />                 <!-- the game, still, under it -->
  </shows>
  <inputs>
    <input button="space"><pop /></input>
  </inputs>
</state>
```

- **The stack.** `<push state="paused" />` puts a state over the current one, `<pop />` goes back, `<pop state="result" />` replaces the current one. The first state is never popped. A `<reset />` from a state's commands is a new game, and the stack goes back to the first state.
- **Drawing order.** First what is shown still, then what is in play, each in file order (a later one over an earlier). A state's own objects come after its scenes', so they draw over them.

### A state from another

`<state name="initial2" from="initial1">` is a copy of an earlier state with what it writes over it, by the [one change rule](objects.md#the-one-change-rule). Its objects are named by the new state (`initial2.cursor`). Its `<input>`s replace the copied ones of the same key, conditions and timers those of the same `name=`, music the copied. A state with scenes cannot be copied yet.

```xml
<state name="initial2" from="initial1">     <!-- Galaxian: the second letter of a high score -->
  <objects>
    <object name="cursor"><position><x>letters + letterstep</x></position></object>
  </objects>
  <inputs>
    <input><button>space</button><pop state="initial3" /></input>
  </inputs>
</state>
```

## Scenes

One scene is current, and only its objects are in play, beside the state's own.

```xml
<state name="playing">
  <objects>
    <object name="hero"> ... </object>              <!-- in every scene -->
  </objects>
  <scenes>
    <scene name="room1">
      <ways><way name="out" scene="room2" /></ways>
      <variables><variable name="pace">2</variable></variables>
      <objects>
        <object name="topwall" />                   <!-- the piece, as it is -->
        <group name="robots"><velocity><x>pace</x></velocity></group>   <!-- the piece, changed -->
        <object name="exit"> ... </object>          <!-- its own -->
      </objects>
      <conditions> ... </conditions>
      <timers> ... </timers>
    </scene>
    <scene name="room2" from="room1">               <!-- room1, but its pace and way -->
      <ways><way name="out" scene="room1" /></ways>
      <variables><variable name="pace">3</variable></variables>
    </scene>
  </scenes>
</state>
```

- **Each scene has its own copies.** Two scenes that take the robots each have their own.
- **Moving between scenes** is a command where the move happens: `<enter scene="room2" />` in an exit's rule, a condition, a timer. What an object of the state carries goes along.
- **A scene left stays as it was** until entered again. `<reset scene="room1" />` puts it back as it started; a new game starts every scene again.
- **Ways** let one piece lead somewhere different in each scene: `<enter way="east" />` looks the way up in the current scene as it runs.
- A scene's **`<variables>`** are its own numbers, read by its objects.
- Its **conditions and timers** count only while it is current, after the state's own.
- **`from="room1"`** copies a scene (objects, variables, ways, conditions, timers). What it writes is added; one **of the same name** replaces the copied one in its place. A copied condition is read in the new scene, so its plain names are that scene's.
- **What a condition counts.** A class, or a name every scene shares (`robots`), counts what is in play: the state's, the current scene's and the game's. A path (`room2.robots`) counts that scene's, wherever the game is.

## Shared shows, keys and conditions

What several states share comes with [`from=`](#a-state-from-another); each state writes its own `<show>`s.

## Conditions

```xml
<condition object="frog" variable="lives">
  <atmost>0</atmost>
  <become object="over.message" sprite="lost" />
  <push state="over" />
</condition>
```

A condition is checked every frame while its state (or scene) is current. It holds when an object matching `class=` and/or `object=` passes its test, and then runs its commands. **The first condition that holds runs, and no more that frame**, so order matters. A `name=` lets a scene's `from=` replace it.

| Test | Holds when the number is |
|---|---|
| `<atleast>N</atleast>` | N or more |
| `<atmost>N</atmost>` | N or less (lives counting down) |
| `<above>N</above>`, `<below>N</below>`, `<equal>N</equal>`, `<unequal>N</unequal>` | more than, less than, equal to, not equal to N |
| `<remaining>N</remaining>` | no more than N of the matched objects are in play (`class="aliens"` with 0: all shot) |

- **What is tested.** `variable="score"` is the object's variable or, if none, one of its live numbers: `x`, `y`, `velocity.x`, `velocity.y`, `heading`, `altitude`, `rise`, `light`, `blink`; on cells `column`, `row`, `moves`; on a holder its [card numbers](cards.md#holders).
- **The number is read as the game runs** when it names something that changes: `<above>gun2.score</above>`.
- **`total=`**: one number over everything matched and in play: `count` (no variable), `sum`, `highest`, `lowest`.
- **`distance="hero"`**: how far each matched object is from the nearest `hero`, middle to middle. With a total, over them all: `class="hazard" distance="centre" total="lowest"` with `<above>150</above>` is "nothing within 150 of the middle".
- **Tests that must all hold** are a [condition in a condition](#conditions-in-conditions): the first test on the outer, the next inside it.

```xml
<condition object="dragon" distance="hero"><below>160</below>
  <condition object="dragon" variable="awake"><equal>0</equal>
    <inc variable="dragon.awake" />
    <become object="dragon" sprite="awake" />
  </condition>
</condition>
```

- **`<any>`** holds when one of its tests does. Inside it a `<condition>` is a test alone, with no commands; an `<any>` can hold another. There is no `<all>`: an "or" whose choice needs two tests is two conditions, one for each way (Tennis: five points, or four against thirty or less).

```xml
<condition>                                  <!-- Asteroids: the field clear, or the rocks far from the middle -->
  <any>
    <condition class="hazard"><remaining>0</remaining></condition>
    <condition class="hazard" distance="centre" total="lowest"><above>150</above></condition>
  </any>
  <reveal object="ship" />
</condition>
```

- **`sprite="settling"`** looks only at the matched objects showing that look. With no test tag, showing it is the test: `<condition object="table" sprite="settling">` holds while the table shows `settling`. A look none of them has stops the load. Where a game is in its turn (betting, playing, settling) is a look of one object, changed with `<become>`, not a number.

- **`<lowest />` and `<highest />`** are tests too: they hold for the one matched whose variable is lowest (or highest) of all those matched, the first of a tie.
- **`self` and `others`.** In a condition matching a class (or a group), `self` is each one it holds for and `others` the rest of those it matches: `<inc variable="self.score">self.got</inc>`, `<inc variable="others.score">26</inc>`, and `sprite="self"` in a `<become>` is the look named after it. What names `self` or `others` runs for each one it holds for, in order; the rest of its commands once, after. With class= and no test at all, it holds for each one there. A total (`total=`, `<remaining>`) holds for no one object, so its commands cannot name them. In a condition in a condition, they are what the innermost test matches.

```xml
<!-- the hand over and someone at 100: the lowest score wins -->
<condition object="table" sprite="done">
  <condition class="player" variable="score" total="highest"><atleast>100</atleast>
    <condition class="player" variable="score"><lowest />
      <become object="over.winner" sprite="self" />
      <push state="over" />
    </condition>
  </condition>
</condition>
```

### Conditions in conditions

A condition holds conditions after its test. They are checked only while it holds, in order, the first that holds running. So the test they share is written once. Its commands then go in two blocks, either or both left out: **`<first>`** runs as soon as it holds, before the conditions it holds are checked (what every case does), and **`<otherwise>`** runs when none of them holds. Commands loose beside the conditions it holds stop the load, so which run when is always written.

```xml
<condition object="table" sprite="settling">
  <first><become object="table" sprite="done" /></first>
  <condition object="dealer" variable="worth"><above>21</above>
    <inc variable="table.bank">table.bet * 2</inc>
    <become object="result" sprite="dealerbust" />
  </condition>
  <condition object="player" variable="worth"><equal>dealer.worth</equal>
    <inc variable="table.bank">table.bet</inc>
    <become object="result" sprite="push" />
  </condition>
  <otherwise>
    <become object="result" sprite="lose" />
  </otherwise>
</condition>
```

The case is chosen before `<first>` runs, so what `<first>` changes does not choose it. A condition holding no conditions writes its commands directly. In `<computer>`, where every condition that holds counts, one holding conditions may have a `<first>` but no `<otherwise>`.

They nest to any depth. One with no `<first>` or `<otherwise>`, none of whose conditions holds, lets the next condition be checked.

- A filter matching nothing is a warning, since "none left" would hold at once. A missing object or variable stops the load.
- A state's condition has no `self`, so it names what it compares; a rule's condition has `self`.

## Keeps

A new game puts every object back as it started; the objects in `<keeps>` keep their variables, place and looks.

```xml
<keeps>
  <keep object="best" />
</keeps>
...
<condition object="ship" variable="score">
  <above>best.score</above>
  <copy variable="ship.score" into="best.score" />
</condition>
```

**On disk.** The programs write each kept object's variables and look to `saves/<game>.xml` in the games folder when a new game starts and when they close, and read it at load. A game that keeps nothing writes nothing; tests write nothing unless asked. Delete the file to start again.

### A table of the best

A group whose members are the places, best first, and `<rank variable="entry.score" into="table" />`: the entry takes the first place whose `score` is below its own, all its variables going in, and the rest move down. A tie goes below. Whether a score makes it is a condition, `<above>table.5.score</above>`.

```xml
<group name="table">
  <sprite><text><letter>table.first</letter><letter>table.second</letter><letter>table.third</letter><number>table.score</number></text></sprite>
  ...
</group>
...
<input button="space"><rank variable="entry.score" into="table" /><reset /></input>
```

## Players as a group

Board games, card games and any game whose players share one world (Cannon Hill's field, Hold'em's table) take it in turns this way. The players are a group with `<turns />`:

```xml
<group name="players" class="player">
  <sprite name="free"> ... </sprite>
  <sprite name="calling"> ... </sprite>
  <sprite name="waiting"> ... </sprite>
  <actions>
    <action name="play"><pick class="card" /><place into="discard" /><play sound="card" /><pass /></action>
    ...
  </actions>
  <turns sprite="waiting" />
  <member name="you"><position> ... </position><owns object="mine" /></member>
  <member name="computer"><position> ... </position><owns object="theirs" /><think button="space" /></member>
</group>
```

- **The members take turns in the order written**, the first first. Live **`turn`** on each is 1 on its turn, else 0 (`you.turn`). `sprite=`: the look a member shows off its turn; the one whose turn comes shows its first look.
- **Keys reach only the member on its turn.** A `<trigger object="players">` (and a pointing control) goes to that one alone, and to a member with a `<think>` only as the computer presses for it, so a person's keys never play the computer's member. A trigger run by a rule or condition is not held back.
- **`<pass />`** in a member's action hands the turn to the next, round to the first, and ends what the press would still do. It is refused while a holder waits for a [wild card's call](cards.md#holders).
- **`<pass object="players" member="N" />`**, from anywhere (a condition, the table's action), gives the turn to member N, counted from 1 in the order written, or to nobody with 0 (a full trick while it shows). `member=` is a number or a name for one (`trick.winner`, `table.first`). A member's number is also its seat, so a trick's `winner` names a member.
- **When the turn changes**, the computer waits a moment before it plays, and the game's conditions see the change first.
- **A group variable in a verb** (`<inc variable="players.got">trick.points</inc>`) is the member on its turn.
- **A member's `<think button>`**: the computer plays it on its turn, pointing as it thinks best and pressing that key (`<depth>`, `<avoid />` and `<worth>`s as a [computer's](#computer-players)). No `<computer>` condition is needed.
- **A verb naming the group** (`<become object="players" sprite="calling" />`) changes the member on its turn.
- **`<owns object>`**: the holder of its own, a player's cards ([holders](cards.md#holders)).
- **`<owns class>`**: the pieces of a [board game](boards.md) that are this player's (`<owns class="x" />`). [`<put />`](boards.md#the-picker), `<pick />`, a `<think>` and the marks `<movable>` and `<put>` that leave out `class=` take the class of the member on its turn, so one cursor serves every player.
- **A look given while it waits stays**: a member that comes onto its turn showing its off-turn look shows its first; one given another look meanwhile (Reversi's "again") keeps it.
- **Played by `<computer>` conditions.** A member with no `<think>` answers any press on its turn, so a `<computer>` condition on its live `turn` can play it by holding keys whose inputs trigger it (Hold'em's F9 to F12 on `cpu`).
- **`<team>`** on a member: its side; partners share one, and a computer's `<think>` lets a partner's winning card stand (Spades).

## Alternating games

Players take turns as the arcade machines did: each plays a game of their own, and the machine swaps them. (Players sharing one field, a board or a table, are a [players group](#players-as-a-group) instead.)

```xml
<input button="num2">
  <push state="playing" />
  <alternate>
    <player state="player1" />
    <player state="player2" />
  </alternate>
</input>
```

- Each player's game starts as the game is when `<alternate>` runs: every object but the kept ones, every timer, the current scenes. A `<player>`'s `state=` is pushed when that turn begins.
- **`<pass />`**: the turn goes to the next player still in; this game is put away as it is, theirs comes back.
- **`<out />`**: this player is out and the turn passes. When no one is left, what follows `<out />` runs, so `<out /><reset />` is "the next player, or a new game". With no turns it does nothing, so the file plays one player too.
- They go where a state's commands go, so a swap never happens mid-frame. A full reset ends the turns.

## Attract mode

A game's `<demo>`, after `<states>`, is a recorded game: the random seed, how many frames, and each key pressed and released on its frame.

```xml
<demo>
  <seed>20261016</seed>
  <frames>1200</frames>
  <press button="space">30</press>
  <release button="space">34</release>
  ...
</demo>
```

`<demo />` plays it as a new game from the seed, so it plays out exactly as recorded. A key pressed, or the recording running out, ends it with a new game, kept things as they were (a demo never sets the best). `xgecli <game> --record` writes one into the file, keeping its line endings.

## Computer players

A state's `<computer>` is a player the game plays itself, pressing the state's keys as a person would: conditions whose commands are `<hold button>`s. **Every** condition is tested every frame; the keys of those that hold are held, the rest let go. Give it keys of its own (Pong's F11 and F12) where a person's keys for the same thing should do nothing while it plays.

```xml
<computer>
  <condition object="cpu" variable="x"><below>ball.x</below><hold button="l" /></condition>
  <condition object="cpu" variable="x"><above>ball.x</above><hold button="j" /></condition>
</computer>
```

A condition can hold conditions, so a mode the title sets (`players`, `level`) is written once above what it governs, and one state serves one player and two: Pong's levels are each a condition holding its two moves. Since every one that holds counts, one holding conditions has no buttons of its own.

### Thinking

For a [board game](boards.md), a condition may hold a `<think>` instead of `<hold>`s:

```xml
<computer>
  <condition object="match" variable="players"><equal>1</equal>
    <condition object="whites" variable="turn"><equal>1</equal>
      <think object="cursor" button="space">
        <depth>4</depth>
        <worth sprite="king">3</worth>
      </think>
    </condition>
  </condition>
</computer>
```

- **It searches.** It copies the board and plays out every move, each side in turn, `<depth>` moves deep (default 4, 1 to 9), by the pieces' own `<moves>`.
- **What it wants is inferred**: its pieces less the other side's, each 1 or its look's `<worth>`; a side with no move loses (or passes, in games of puts); a `<line>` check of the board is a win for whoever makes it; a put that `<close>`s keeps the turn. Equal moves are chosen at random.
- **It plays as a person would**: it walks the picker `object=` to each cell of its move and taps `button=` there, eight frames apart. While its condition holds, a person's keys and pointer do not reach that picker, so one cursor and one key serve both sides. With no `class=` it plays the pieces the member on its turn [owns](#players-as-a-group).
- **It thinks beside the game.** In a window the search runs on a thread of its own, so the game keeps drawing; one no longer wanted is stopped. With no window (the tests) it searches in the frame, so a run is the same every time.
- **On a hand of cards** (`object=` a hand with its own pile, no `class=`), it [plays cards](cards.md#the-computer).

A `<hold>` or `<think>` anywhere else, or any other command in a `<computer>`, stops the load.
