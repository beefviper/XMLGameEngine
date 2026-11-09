# Input

**Status:** Built, the mouse and pads too. Reference: [input](../reference/input.md).

## Three layers

- An **object** names what it can do (`<action name="up">`); a **state** binds a key to that name (`<input button="w"><trigger object="paddle1" action="up" />`); the **key** is only a label.
- So remapping is one edit in one state, one key does different things in different states, and one action name means different things on different objects, with no class hierarchy.
- Held `<move>`s record each direction's step and the velocity is worked out from all four on every key change, so Down held and Up tapped cancel and two keys make a diagonal. Replaced last-key-wins.
- **Shared keys** come with a state `from=` another, its own inputs going over the copied ones key by key. Named `<keys>` sets did this once; no game used them after `from=`, so they went.

## A press that cannot be done

- A ship played its firing sound with its shot already in flight; Q*bert squeaked when pressed in the air.
- **Chosen: a list stops at the first verb refused** (`<fire>` with none free, a hop in the air, `<leap>` off the ground). Each verb already knew; it now says so. Commands before it still run, which Berserk's `<aim>` before `<fire>` needs. The same holds in rules, keys and conditions; later rules still run.
- Rejected: running verbs first and the rest after (breaks aim-then-fire), a `<try><then>` wrapper (the same order in more tags).
- **An object out of play does not act:** its press or trigger does nothing, though letting go still lets go. Yars' hidden cannon fired from where it waited. Rejected: a test in every action, a state per case.

## Held keys across state changes

| Option | Behaviour |
|---|---|
| Latched to the press | Hold Left through pause and the paddle moves on the pause screen. Built first, replaced |
| **Live held keys** | The current state decides what a held key means. On a change the old bindings are let go and the new state's held verbs resume for keys still down. One-shots and state changes need a real press |
| Cancel on change | Press again after every pause. Simple but annoying |

Built by a state change count compared in `Engine::syncHeldKeysToState()` after each key event and frame.

## A key's three moments

- A putt, a plunger, a cannon: "hold to build power, let go to strike". A key could act only on its press.
- **Chosen: `on="press|hold|release"`** (press if left out, so no game changed), rather than a `<charge>` verb; a charge is hold, `<inc>` and `<smallest>`. `release` as a value does not clash with the `<release>` command, and a `<demo>` already writes a let-go key as `<release>`.
- **`hold` and `release` answer only a press made in the current state**, or Space held from the title would charge and strike.
- Open: a repeat interval while held; how long a key has been held as a number.

## Otherwise

- **A trigger's `<otherwise>`** (Beef agreed, from the readable-rules work): card and board games tried one action and then another on the same key (lay, then put back; place, then pick), which worked only because the first left the second nothing to do. Now the second is written as what happens when the first is refused. Kept in the key's flat command list (the trigger counts the commands after it), so every walker over commands is unchanged; the engine skips them unless the action was refused (`Engine::runAsPress`). Looked at first and rejected: nesting a key's triggers under the looks they answer, which made nothing shorter. Open: an `<otherwise>` in actions, rules and timers.

## Mouse and pads

- **Buttons are keys** (`mouse.left`, `pad1.a`), so binding, held keys and sets all apply. A pad's d-pad and stick past halfway are the same four directions.
- **Backends report state, the engine finds changes** (`Window::controls()` diffed in `Engine::handleControls`), one place for the rules instead of five event loops.
- **A click counts only on the game's own pixels**, decided once in the engine for every library: Klondike answered clicks on the title bar and menus. A button held and brought onto the field is no press.
- **Analog is an `axis=` binding plus `<track />`:** the Atari paddle is a position, not a direction. `<track path>` follows a path's legs. Rejected: a velocity from a stick, a range on `<track>`.
- Qt reads the mouse only (Qt 6 has no gamepad module). Open: right stick, triggers, more pads.

## Ideas

- A frame-scoped **command queue** (input makes intents, update uses them): determinism, replay, network sync. The three layers are the same idea from the XML side.
- Many-to-one bindings (two players steering one object); axis inversion as a binding attribute.
- The shipped games' key convention is in [the shipped games](../reference/games.md#the-key-convention), enforced by `test_engine_input`.
