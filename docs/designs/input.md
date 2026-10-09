# Input

**Status:** Built.

## Input: three layers

- An **object** names what it can do: `<action name="up"><move direction="up">step</move></action>`. A **state** binds a key to a name: `<input button="w"><trigger object="paddle1" action="up" /></input>`. The **key** is only a label.
- Consequences: remapping is one attribute in one state; the same key can do different things in different states; changing what `up` does is one edit on the object; the same action name can mean different things on different objects (polymorphism with no class hierarchy).
- Built on top: one `button` can name several keys (`"a left"`); named `<keys>` sets at the top of `<states>` that a state takes with `<inputs keys="name">` and overrides key by key ([timers](timers.md) for why).
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
