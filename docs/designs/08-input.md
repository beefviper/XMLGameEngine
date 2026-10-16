# Input

**Status:** Built, the mouse and gamepads too.

## Input: three layers

- An **object** names what it can do: `<action name="up"><move direction="up">step</move></action>`. A **state** binds a key to a name: `<input button="w"><trigger object="paddle1" action="up" /></input>`. The **key** is only a label.
- Consequences: remapping is one attribute in one state; the same key can do different things in different states; changing what `up` does is one edit on the object; the same action name can mean different things on different objects (polymorphism with no class hierarchy).
- Built on top: one `button` can name several keys (`"a left"`); named `<keys>` sets at the top of `<states>` that a state takes with `<inputs keys="name">` and overrides key by key ([timers](09-timers.md) for why).
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

### Mouse and gamepads (2026-10-16)

- **Buttons are keys.** A mouse or pad button is one more `KeyCode` (`mouse.left`, `pad1.a` ... `pad2.right`, an Xbox layout, two pads), so binding, held keys, key sets and the state rules above all apply unchanged. A pad's d-pad and its stick past halfway are the same four directions, so a game binds `pad1.left` once.
- **Backends report state, the engine finds changes.** `Window::controls()` gives the mouse's place and buttons and each pad's buttons and stick once a frame; `Engine::handleControls` diffs it with the last frame and presses and releases. One place for the rules (the halfway stick, the first look that only notes) instead of five event loops. Qt has the mouse only (Qt 6 has no gamepad module).
- **Analog is an `axis=` binding plus `<track />`.** The Atari paddle is a position, not a direction, so it binds to an action like a key does (the three layers stay: state binds, object acts) and `<track />` names the behavior: follow where it points, across the window, kept inside. It runs only when the control moves, so the keys and the mouse share an object. Rejected: a velocity from a stick (the d-pad already moves), a range on `<track>` (the window is every shipped game's range; add `<from>`/`<to>` when one is not).
- Open: a range for `<track>`, the right stick and triggers, a stick's push as a speed, a third and fourth pad, pads in Qt.

### Ideas

- A frame-scoped **command queue**: input builds intents (command, target, amount), update consumes them, cleared every tick. Gives determinism, replay, an AI that issues the same commands, network sync. The three layers above are the same idea from the XML side. A queue inside every object was rejected.
- Many-to-one bindings (two players' inputs combined into one object: same direction, opposite, sum, sum with momentum; Pong with a third paddle).
- Few actions, context chooses the effect (instead of a button per command); axis inversion as a binding attribute.
- Fixed conventions for shipped games (Space starts/pauses/restarts, W A S D plus arrows) are listed in [../readme.md](../readme.md) and enforced by `test_engine_input`.
