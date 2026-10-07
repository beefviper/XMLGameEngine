# 04. States, conditions and input

**Status:** built. Per-state object behavior, object-level conditions and passing data between states are open.

## States

- A `<state>` is a **screen**: menu, playfield, pause, game over. It says what is shown, which keys run which commands, which conditions to watch, and its own timers. A state is a template; which one is current is runtime.
- **Chosen: a stack.** `<push>` / `<pop>`; pause is space in `playing` pushing `paused` and space in `paused` popping. A bare `<reset />` from a state input or condition collapses the stack to the first state, so menu → play → game over → menu does not grow. The first state is never popped. The state stack holds copies, so state timers live in `Game::stateTimers` by name.
- **Moving on without growing the stack (2026-10-07): `<pop state="wave2" />`.** Kaboom's waves and Berserk's rooms used to `<push>` the next, so the stack grew by one each time until a reset. Looked at: a `<goto state>` verb; `<reset to="...">` or `<reset>wave2</reset>` (the author's first ideas); and an attribute on `<pop>`. Chosen: `<pop state="...">`, "this state goes, and that one takes its place": no new verb, the attribute picks the state as `push`'s does, and it reads as what happens. It replaces the first state too rather than leaving the stack empty. Kaboom and Berserk use it.
- Open:
  - **Per-state object behavior.** A state only chooses which objects are *shown*; every shown object with a velocity moves, so the invaders cannot stand still behind the menu and march only in play. Ideas: a per-`<show>` override (`velocity="0"`); `<enter>`/`<exit>` commands; an `active` flag states switch; a freeze verb; separate copies per state (possible today, duplicates definitions). Related to where behavior belongs: state, object, or the pair.
  - **Passing data between states.** Space Race has two identical win screens differing only in text, because a transition carries nothing. Idea: a condition hands the destination the object (or its name) that triggered it, via a reserved name such as `trigger`, so one `wins` state can say "`{winner}` wins". Open: object, name or copied value; lifetime. Would also serve high-score entry.
  - Settings screens exist as states but there is no settings vocabulary.

## Conditions

- Problem: a declarative file has no place to end a game at 15 points. The conclusion was that some conditional is unavoidable, so call it a **trigger**, not an `if`. It belongs in `playing`, not `paused`.
- **Built (state-level):** checked each frame while the state is current; the first match runs its commands and stops checking for the frame. Filter by `class` and/or `object`, then exactly one of:
  - `<atleast>N</atleast>` on a variable (reached it),
  - `<atmost>N</atmost>` (fallen to it: Frogger counts lives *down* with `<dec>` so the starting number lives in the frog's own `<variable>`; fires at or below because a variable can skip zero),
  - `<remaining>N</remaining>` (no more than N matching objects still in play, i.e. visible; hidden and dead objects do not count).
- A condition can run state commands plus `<inc>`, `<dec>`, `<become>`, `<reveal>`, `<play>` and `<trigger>`; one that changes the variable it reads can count and start over (Frostbite turns four blue rows white and takes 4 off the count).
- Choices: a name that says what it counts rather than an operator or expression; `remaining` counts visible objects, not enabled ones (a text with collisions off is still there); a filter matching nothing warns at load because "none left" would be true at once; `<reset />` on the game-over screen restores variables so a leftover condition does not fire again.
- Not built: general comparisons (`remaining="<3"`), conditions on two variables or expressions, counting or summing a variable across a class, object-level conditions (change color above a speed, a death animation at 0 health), states-as-predicates (no moves left, a timer), win/lose as first-class terms (compare VGDL's TerminationSet). A condition's numbers are load-time constants ([02](02-values-variables-and-names.md)).
- Second-batch ideas: guarantee a solvable start or treat "no legal move" as a loss (unwinnable boards); rules that read how well the player is doing and change stats ([13](13-ideas.md)).

## Input: three layers

- An **object** names what it can do: `<action name="up"><move direction="up">step</move></action>`. A **state** binds a key to a name: `<input button="w"><trigger object="paddle1" action="up" /></input>`. The **key** is only a label.
- Consequences: remapping is one attribute in one state; the same key can do different things in different states; changing what `up` does is one edit on the object; the same action name can mean different things on different objects (polymorphism with no class hierarchy).
- Built on top: one `button` can name several keys (`"a left"`); named `<keys>` sets at the top of `<states>` that a state takes with `<inputs keys="name">` and overrides key by key ([08](08-timers-and-enemy-behavior.md) for why).
- Held `<move>`s record each direction's step; velocity is recomputed from all four on every key change (replacing an earlier last-key-wins), so Down plus tapping Up cancels, and two keys make a diagonal.
- One-shot verbs (`<hop>`, `<jump>`, `<fire>`) belong to the press.

### Held keys across state changes

A key can stay down while the state changes (hold Left, pause, unpause).

| Option | Behavior |
|---|---|
| A. Latched to the press | A press runs the binding of the state active then; the release goes to the same commands. Hold Left through pause: the paddle keeps moving on the pause screen, and a key first pressed during the pause does nothing after. Built first, then replaced |
| **B. Live held keys (built)** | The current state gates what a held key means. On each state change the engine releases what held keys were driving, then resumes the new state's *continuous* bindings (`move`, `accelerate`, `thrust`, `turn`) for keys still down. Pausing stops the paddle; unpausing restarts it with no re-press. Space held through the menu does not pause (state changes and one-shots run only on a real press) |
| C. Cancel on change | Nothing resumed; press again after every pause. Simple, but annoying. Switching to it means dropping the resume half of `syncHeldKeysToState` |

- Implemented by `Game::stateChangeCount()` (up on every push/pop) compared in `Engine::syncHeldKeysToState()` after each key event and each frame (a condition can change state); `CommandExecutor::executeHeldInput` is the resume path. Releasing always reaches the commands its key is currently driving.
- Repeat-while-held would be a separate verb or a repeat interval, not a behavior of `hop`.

### Ideas

- A frame-scoped **command queue**: input builds intents (command, target, amount), update consumes them, cleared every tick. Gives determinism, replay, an AI that issues the same commands, network sync. The three layers above are the same idea from the XML side. A queue inside every object was rejected.
- Many-to-one bindings (two players' inputs combined into one object: same direction, opposite, sum, sum with momentum; Pong with a third paddle).
- Few actions, context chooses the effect (instead of a button per command); axis inversion as a binding attribute.
- Fixed conventions for shipped games (Space starts/pauses/restarts, W A S D plus arrows) are listed in [../readme.md](../readme.md) and enforced by `test_engine_input`.
