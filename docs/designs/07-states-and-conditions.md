# States and conditions

**Status:** Built, including states `from=` others. Open: object-level conditions, copying a state with scenes, taking things out of a copy. Reference: [states and scenes](../reference/states.md).

## States

A state is a **mode**: menu, play, pause, game over. It says what is shown, which keys run which commands, which conditions to watch, and its own timers.

- **A stack.** `<push>`/`<pop>`; pause pushes `paused` and Space pops it. A bare `<reset />` from a state collapses the stack to the first state, so menu, play, game over, menu does not grow it.
- **`<pop state="x" />`** replaces the current state ("this one goes, that one takes its place"). Chosen over a `<goto>` verb or `<reset to=>`: no new verb, and it reads as what happens.
- **Shared sets, gone:** named `<keys>`, `<conditions>` and then `<shows>` sets at the top of `<states>` went once `from=` shared a state's keys, conditions and shows: the last two games with a `<shows>` set (Air-Sea Battle, Galaxian) used each in one state only.
- **A state from another, `from=`:** the same word and change rule as scenes and [pieces](13-pieces.md): keys replace by key, conditions and timers by name. Done in shared C++ (`takeStates`). First built for 17 games whose win and lose screens differed in one text; those are now one end state with a look per outcome (below), so `from=` is left for states that really are another one changed (Galaxian's high-score letters, Pac-Man's ready).
- **One set of state names:** every game has `title`, `playing`, `paused` and `over`, the end screen of a win, a loss or a draw alike. Other states are moments inside play (`ready`, `point`, `caught`, `blasted`, `landed`, `destroyed`, a player's turn, a high-score letter). Chosen so a reader, a test or a generator finds the same states in every file.
- **States own objects** ([scenes](12-scenes.md)); before, every shown object with a velocity moved, so invaders could not stand still behind the menu.
- **Passing data between states is a look:** the end state owns one object with a look per outcome, and the winning condition `<become>`s it before pushing. Numbers go by path. Shelved: a reserved name for the object that triggered it (a name whose meaning changes while running).

## Conditions

- A declarative file has no place to end a game at 15 points, so some conditional is needed: call it a **trigger**, not an `if`.
- Checked each frame while the state is current; **the first that holds runs and no more that frame.** Tests are named for what they test: `<atleast>`, `<atmost>`, `<above>`, `<below>`, `<equal>`, `<remaining>`. `<atmost>` because a variable can skip zero; `<above>` is strict since a tie is a third outcome.
- A filter matching nothing warns at load, since "none left" would be true at once.
- **The number is content**, read as the game runs when it names something that changes (`<above>gun2.score</above>`, `Condition::live`). It replaced `object=`/`variable=` on the test tag, which read ambiguously.
- **`<any>`** of `<condition>`s, one test each, since the tests of a real "or" are about different things. An "and" is a condition in a condition (below), so `<all>` went: first from a condition's head and a rule's checks, where it said the same thing twice, then from `<any>` too, where its one use (Tennis) read as well as two conditions. Rejected: `and=`/`or=` chains, `<not>` (each test has its opposite).
- **`total=`** (count, sum, highest, lowest) makes one number over everything matched, so "at least three bullets in flight" needs nothing new.
- **`distance=`** measures to the nearest of a name; with `total="lowest"`, "nothing near" is one test (Asteroids' respawn).
- **Conditions in conditions** (the author: the card games' rules read as "a random list of tags"): a condition holds conditions, so a shared test is written once above what it governs, the reuse rule applied to rules. The engine flattens them (`flattened()`) into the plain first-that-holds list, so nothing else changed. Counted before: Blackjack, Spades, Hearts and Gin repeated the test before in 8 to 11 of 14 to 16 conditions.
- **`<first>` and `<otherwise>`** (the author's idea): the holder's commands were first loose after its conditions, meaning "when none holds", which was easy to misread (two games were written wrong that way), and what every case did was written in each. Now `<first>` runs before the cases, `<otherwise>` when none holds, and loose commands beside conditions stop the load. Flattened as `<first>` put before each case's commands. `<run>` was the author's first word, but a pattern's `<run>` is cards in a row; `<always>` and `<none>` were the other candidates.
- **A look is the phase:** `sprite=` on a condition tests what an object shows. Games kept a `phase` number beside a look saying the same thing; the number goes. `sprite=` over `from=` (which reads as changing from).
- **One rule for every player:** a class condition's commands name `self` (each one it holds for) and `others`, as rules and path steps already name theirs; `<lowest />`/`<highest />` pick one. Hearts wrote its win and its moon four times each, once a player. Made at load as one command list per object it may match (`Condition::each`); what names neither runs once. Rejected: a loop verb (programmy).
- **One word, `<condition>`:** the test inside `<any>` and a rule was once `<check>`; the schema gives one word two shapes by where it stands.

## Keeps, tables, turns, demos

- **`<keeps>`** lists objects a full reset passes over; `<copy variable into>` sets one variable from another. Taken "for now"; rejected a flag on each object (scatters what survives a reset).
- **A best table from three small pieces,** not a table element: a text of `<content>`, `<number>` and `<letter>` parts (initials are three numbers); a group's text naming the group's variable means each member's; `<rank variable into>` takes the first place it beats. A tie goes below, as on the arcade machines.
- **On disk:** kept objects are written to `saves/<game>.xml` when a new game starts and on close, read a line at a time without an XML library (a generated program has none). Nothing in the game file asks for it.
- **Turns swap the play,** as arcade boards swapped RAM: each player has a whole game of their own (every object not kept, every timer, the current scenes), so any one-player game becomes two-player in a few lines. Swaps happen only from a state's commands, never mid-frame.
- **Attract mode is recorded play:** the same seed and the same keys on the same frames give the same game, so a `<demo>` is a seed, a length and key changes. Rejected: a scripted computer player, replaying positions.

## Players as a group

- **Wanted:** board games, card games, Cannon Hill and Hold'em share one world, and the players take turns in it. Before, each game kept a turn variable, looks gating every key, a second hand object for the computer and an F-key for it to press (Crazy Eights: four turn values, F12, a `cpuhand`).
- **Chosen (the author agreed):** the players are a `<group>` with `<turns />`, in the order written. Keys reach only the member on its turn; a member with a `<think>` is the computer's, and answers only the presses the computer makes for it, on the same keys a person uses. `<pass />` (the word arcade turns use) hands the turn on. Live `turn` is 1 or 0 on each member, so a live text can say whose turn it is.
- A member may give its own `<variables>`, `<owns>` and `<think>`: the same take-and-change rule as sprites.
- **Board games on it:** each member `<owns class>` its pieces, so one cursor and class-less `<put />`, `<pick />`, `<think>` and marks play for the member on its turn. A `<computer>` `<think>` walking the picker keeps people's keys and pointer off it while its condition holds, and taps its key, so a person's next press is never lost. Rejected: a cursor per player (two of everything again).
- **A member played by `<computer>` conditions** (Hold'em) has no `<think>`, so it answers any press on its turn; its own keys (F9 to F12) trigger it by name. Shared actions written once on the group say "the other" by a condition on each member's `turn`; a word for the other player stays open.
- A verb naming the group changes the member on its turn; `class=` still means every member.
- Off its turn a member shows `<turns sprite>`; the one whose turn comes shows its first look, unless it was given another while it waited (Reversi's "again", set before the `<pass>`).
- Kept apart: arcade `<alternate><player state>`, where each player has a world of their own.
- `<pass object member>` gives the turn to a member by number, or nobody (0): a trick's winner leads, the first leader goes round. Numbers, not names, because the trick's `winner` is a seat number and seats are the members' order; a name would need a string value.
- A turn change waits the computer a moment, so a computer does not play onto a full trick before the conditions that take it run.
- Open: live `computer` to switch a member between person and computer (one player or two from the title); `others`; a member's name in a text; points and tricks held by members rather than the pack. A member's `<team>` stands beside `<owns>` rather than inside a `<hand>`: partners are players, not hands.

## Open

- Object-level conditions (a colour above a speed, a death at 0 health); win and lose as first-class terms.
- A total or distance as a value inside an expression; a condition that holds once until it stops holding, without a flag.
- Comparing each object of a class with its own other variable.
- Demos recording the mouse and pads, following game-level `<random>`s, noticing an older recording.
- All players' scores shown at once; place numbers in a best table.
- A settings vocabulary; "no legal move" as a loss in action games.
