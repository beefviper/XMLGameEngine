# States and conditions

**Status:** Built, states from= others too. Object-level conditions, a state from= a state with scenes, and taking things out of a copy are open.

## States

A `<state>` is a **screen**: menu, playfield, pause, game over. It says what is shown, which keys run which commands, which conditions to watch, and its own timers. A state is a template; which one is current is decided while running.

### A stack

- **Chosen: a stack.** `<push>` / `<pop>`; pause is Space in `playing` pushing `paused` and Space in `paused` popping.
- A bare `<reset />` from a state input or condition collapses the stack to the first state, so menu → play → game over → menu does not grow. The first state is never popped.
- The state stack holds copies, so state timers live in `Game::stateTimers` by name.

### Moving on without growing the stack: `<pop state>`

- Need (2026-10-07): Kaboom's waves and Berserk's rooms used to `<push>` the next, so the stack grew by one each time until a reset.
- Looked at: a `<goto state>` verb; `<reset to="...">` or `<reset>wave2</reset>` (the author's first ideas); an attribute on `<pop>`.
- **Chosen:** `<pop state="wave2" />`, "this state goes, and that one takes its place": no new verb, the attribute picks the state as `push`'s does, and it reads as what happens. It replaces the first state too rather than leaving the stack empty.
- Kaboom and Berserk have since moved their waves and rooms into [scenes](12-scenes.md) (2026-10-15), moved on by `<enter scene>`.

### Show sets: `<shows name>`

- Need (2026-10-13): Pitfall's four screens listed the same nine objects each, and several games' play and end screens the same; 191 `<show>` lines repeated another state's.
- **Chosen:** a named `<shows>` set at the top of `<states>`, taken with `<show set="name" />`, one set to a `<show>`; a set can take a set written before it. The reader (and the generator) expands it in place.
- Why one set per `<show>` and not `sets="a b"`: a list of names packed in one attribute is data hidden in a string ([file-format](02-file-format.md)).
- Since states own their objects (2026-10-15) most sets went away again; Air-Sea Battle and Galaxian still use one.
- Not built: taking an object out of a set (`<hide>`).

### A state from another: `from=`

- Need (2026-10-16): since states own their objects, 17 games had a win and a lose screen that differed in one text.
- Looked at: one end state holding win, continue and lose, a condition revealing one (needs a hide verb and still repeats the conditions that choose); passing a variable to the state (shelved, a name whose meaning changes).
- **Chosen:** `<state name="gameover" from="youwin">`, the same word and the same change rule as a scene's and a [kit](13-kit.md) piece's: a copy of an earlier state; objects named like copied ones are changes, the rest its own; keys by key, conditions and timers by name, shows added. Done in shared C++ (`takeStates`, before `layOutScenes`), so a copied object is named by the new state.
- Not built: copying a state with scenes (a load error says so), and taking something out (a copy cannot drop a key or a shown object).

### Objects per state

- Done 2026-10-15: states and scenes own objects, which run only while their state (and scene) is current, and `<show state>` draws a state still under another ([scenes](12-scenes.md)).
- Before, a state only chose which objects were *shown*, and every shown object with a velocity moved: the invaders could not stand still behind the menu.

### Passing data between states: looks

- Settled 2026-10-15. Space Race, Combat and Freeway had two win screens that differed only in their words, Air-Sea Battle three.
- What the transition carries is *which* outcome, and a look is exactly that: the end state owns one object with a look per outcome, and the condition that ends the game says `<become object="wins.winner" sprite="player2" />` before its `<push>`. A text's size is measured again when its look changes, so centring follows. Numbers go by path (`playing.rocket1.score`).
- Shelved: a condition handing the state the object that triggered it, as a reserved name (`trigger`, `winner`). It would be the one name in a file whose meaning changes while running, and it saves one condition per player.

## Conditions

- Problem: a declarative file has no place to end a game at 15 points. Some conditional is unavoidable, so call it a **trigger**, not an `if`. It belongs in `playing`, not `paused`.
- **Built:** checked each frame while the state is current; the first that holds runs its commands and stops checking for the frame. Filter by `class` and/or `object`, then exactly one test:

| Test | Holds when |
|---|---|
| `<atleast>N</atleast>` | the variable has reached N |
| `<atmost>N</atmost>` | it has fallen to N (fires at or below, since a variable can skip zero) |
| `<above>`, `<below>`, `<equal>` | more than, less than, the same (2026-10-14) |
| `<remaining>N</remaining>` | no more than N matching objects are still in play |

- A condition's commands are the changes and flow groups ([file-format](02-file-format.md#commands-in-groups)). One that changes the variable it reads can count and start over (Frostbite turns four blue rows white and takes 4 off the count).
- Choices: a name that says what it tests rather than an operator; `remaining` counts objects in play, not enabled ones (a text with collisions off is still there); a filter matching nothing warns at load because "none left" would be true at once; `<reset />` on the game-over screen restores variables so a leftover condition does not fire again.

### The number is read as the game runs

- 2026-10-28, Beef agreed. A test's number is content like every other number: `<above>gun2.score</above>`. One that names something that changes is worked out again each time the condition is tested (`Condition::live`); one that does not is worked out once.
- Before: comparing with another variable took `object=` and `variable=` on the test tag, the same two attributes the condition uses for what it tests, which read ambiguously (built 2026-10-14 for Air-Sea Battle's winner).
- Given up: comparing each object a class condition tests with its own other variable (no game used it; a state's condition has no `self`).
- `<above>`/`<below>` are strict where `<atleast>`/`<atmost>` are not, since a tie is a third outcome with two variables.

### And, or, totals, distance

Built 2026-10-16: the three things conditions could not say.

- **And, or:** a condition holds `<all>` or `<any>`, each a list of `<condition>`s, each a test written exactly as a condition's but with no commands; they nest. One test per inner condition, because the tests of a real "and" are about different things (the hero near the dragon, and the dragon still asleep). Set aside: `and=`/`or=` attributes chaining named conditions, several test tags meaning "and" (no "or"), `<not>` (each test already has its opposite).
- **Totals:** `total="count|sum|highest|lowest"` turns what is tested into one number over everything matched and in play, so "at least three bullets in flight" needs nothing new. An attribute, because it picks what is measured. `<remaining>` stays, as every shipped game says it that way.
- **Distance:** `distance="name"` measures each matched object to the nearest in play of that name or group (middle to middle, as `<chase>`), so "near" is a `<below>`.
- Example: Adventure's dragon sleeps until the hero is within 160 pixels and wakes once, an `<all>` of a distance and its own `awake` flag (needed because a condition fires every frame it holds).
- **One word, `<condition>`** (2026-10-28, Beef's call): the test inside `<all>`, `<any>` and a collision rule was a `<check>`. The schema now gives `<condition>` two shapes by where it stands, so a reader learns one word for one idea.

### Condition sets: `<conditions name>`

- Built 2026-10-13, the same rule as `<shows>` a third time: 25 conditions were written word for word in more than one state. A state takes a set with `<condition set="name" />`, and the reader (and the generator) put its conditions in its place. Order matters, since only the first condition that holds runs, so the set stands where its conditions stood.
- No shipped game uses one since the rewrite on scenes: what the places shared became the playing state's own conditions.
- Looked at and left: one dotted name (`variable="status.lives"`) instead of `object` plus `variable`; a `class` condition cannot be written that way.

## Keeping, turns, attract mode

### Keeping a value and copying one: `<keeps>`, `<copy>`

- Built 2026-10-15. A best score needs a value that outlives `<reset />` and a command that sets one variable to another's number as it is now.
- `<keeps>`, after the game's `<objects>`, lists `<keep object="..." />`s that a full reset passes over (variables, place, looks); `<reset object>` still resets one by name.
- `<copy variable="ship.score" into="best.score" />` names both as owner.variable. Galaxian shows its best this way: a condition copies the score into `best` while the score is `<above>` it. (`<set>` with a live value has since come too, [values-and-names](04-values-and-names.md#values-read-when-used-2026-10-17).)
- The author took `<keeps>` "for now", open to something more elegant later. Looked at: a flag on the object (`<kept>true</kept>`), which scatters what survives a reset across the file.

### A table of the best, initials, keeping on disk

Built 2026-10-16: three small general pieces rather than one high-score element.

1. **A text is its pieces:** `<content>`, `<number>` and `<letter>` in any order. A letter is a number shown as one (0 is A, wrapping both ways), so initials are three variables that `<inc>`/`<dec>` already change.
2. **A group's text that names the group's variable means each member's own** (`table.score` in the group `table`), so one sprite serves every row.
3. **`<rank variable="entry.score" into="table" />`:** the table is a group whose members, in order, are the places; the entry takes the first place it beats, and the rest move down. A tie stays below, as on the arcade machines. Whether a score makes the table is an ordinary condition (`<above>table.5.score</above>`).

- **On disk:** the engine writes the kept objects to `saves/<game>.xml` in the games folder whenever a new game starts and when it closes, and reads it at start (`Engine::keepIn`; tests never call it). Nothing in the game file asks for it. Read a line at a time without an XML library, since a generated program has none.
- Set aside: a `<table>` element with columns, variables holding text, a file name in the game file.
- Open: a place's number (1. 2. 3.) needs a text per member; columns of numbers do not line up in a proportional font.

### Players taking turns: swap the play

- Built 2026-10-16. The author chose that each player has a whole game of their own and the engine swaps them, as the arcade boards swapped RAM: any one-player game becomes two-player in a few lines.
- A player's game is every object the game does not keep, every state's and scene's timers and the current scenes; kept objects are everyone's.
- `<turns>` with a `<player state=>` each starts it; the number of players is the number of `<player>`s. The player's state is pushed when that turn begins.
- `<pass />` and `<out />` end a turn; when the turns are over, `<out />` lets the rest of its list run, so `<out /><reset />` says "next player, or a new game". A pass takes off what is above the play state, so the next player lands in play. Only a state's commands can swap, never a collision mid-frame.
- Open: showing every player's score at once; "whose turn" beyond the player's own state.

### Attract mode: recorded play

- Built 2026-10-16. A game is the same every time from the same seed and the same keys on the same frames, so a `<demo>` is a seed, a frame count and the key changes (`<press button>frame</press>`, `<release>`), after `<states>`.
- `<demo />`, a state command, plays it as a new game; a key or the end of the recording starts a new game with what is kept as it was (a demo never sets the best). `xgecli --record` writes the recording into the game file in place of the old one.
- Set aside: a scripted computer player for the attract mode, a state run under the title, replaying positions instead of keys.
- Open: the mouse, pads and paddles are not recorded; game-level `<random>` variables are drawn at load and do not follow the seed; a demo does not notice it was recorded with an older version of the game.

## Open

- Object-level conditions (change color above a speed, a death animation at 0 health), states as predicates (no moves left), win/lose as first-class terms (compare VGDL's TerminationSet).
- A total or distance as a value inside an expression (`<inc>` by the count); a condition that holds once until it stops holding, without a flag.
- Settings screens exist as states but there is no settings vocabulary.
- Guarantee a solvable start, or treat "no legal move" as a loss; rules that read how well the player is doing ([ideas](57-ideas.md)).
