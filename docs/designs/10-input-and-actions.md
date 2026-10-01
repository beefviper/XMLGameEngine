# 10. Input and named actions

**Status:** implemented

## Decision

Three decoupled layers:

- An **object** defines what it *can* do, and names it: `<action name="up"><move direction="up">step</move></action>`.
- A **state** defines which buttons exist and which named action each calls: `<input button="w"><trigger object="paddle1" action="up" /></input>`. It has no idea what the action does.
- The **key** is only a label: `button="w"`.

Consequences the author values:

- Remapping keys is a change to one attribute in one state.
- The same key can do something different in a different state by naming a different action.
- Changing what `up` does for an object is one edit on the object, and every state that references it follows.
- The same action name can mean different things on different objects, so a state can say `up` to whoever it addresses. This is polymorphism expressed in XML with no class hierarchy.

The author points to this as one of the features that do not depend on the function-call syntax, and want to keep ([03](03-expression-syntax.md)).

## How held keys become movement

While a key bound to `move.*` is held, that direction's step is recorded on the object. Velocity is recomputed from all four directions on every key change:

- holding Down and tapping Up cancels to a standstill; releasing Up resumes Down,
- left/right and up/down are independent axes, so two keys give an 8-way diagonal from a 4-way D-pad.

This replaced an earlier "last key wins" behavior in which a single key event overwrote a whole axis.

## Held keys across state changes

A key can be held while the state changes underneath it (hold Left, pause, unpause). The engine tracks every key's press and release itself, so the question is what a state change should mean for a key that is still down. Three ways to answer it:

**A. Latched to the press.** A press runs the binding of the state that was active at that moment and remembers those commands. The matching release always goes to the same commands, whatever state is active by then. State changes neither cancel nor re-fire anything.

- Hold Left through pause and unpause: the paddle carries on afterward, and stops when Left is released.
- Cost: a state that shows the object but does not bind the key (Breakout's `paused` shows the player and binds only Space) does not stop it. The paddle keeps moving on the pause screen, and a key first pressed during the pause does nothing after unpausing until it is pressed again.
- This was built first, and replaced by B.

**B. Live held keys (built).** The current state only gates what a held key means. On every state change the engine releases whatever the held keys were driving in the outgoing state, then resumes the incoming state's continuous bindings (an action's `move.*`) for the keys still down.

- Pausing stops the paddle (the pause screen binds only Space, so nothing else responds), and unpausing starts it again with no re-press. A key first pressed during the pause starts driving the paddle when `playing` returns.
- Only continuous commands are resumed. State-changing commands and one-shot commands such as `fire` run only on a real press. Otherwise holding Space through `mainmenu` to `playing` would press Space again in `playing` and pause immediately, and a held fire key would shoot on every unpause.
- A release always reaches the commands its key is currently driving, so a key let go during the pause leaves the paddle stopped afterward.
- Implementation: `Game::stateChangeCount()` goes up on every push or pop, and `Engine::syncHeldKeysToState()` compares it after each key event and after each frame's update (a `<condition>` can change the state too). `CommandExecutor::executeHeldInput` is the resume path.

**One-shot verbs are never resumed.** `fire` and, since Frogger, `hop.*` belong to the press. A `hop` in an action is queued when the key goes down and made once; releasing does nothing, holding does nothing more, and coming back from a pause with the key still down does not hop again. That is the same rule that keeps a held Space from pausing straight after the menu, applied to a verb: a game whose input is a discrete step wants a fresh press for every step, and the resume path (`executeHeldInput`) only ever picks up `move.*`. The alternative for a hop, repeating while the key is held, would be a separate verb (or a repeat interval), not a behavior of this one.

**C. Cancel on state change (not built).** Leaving a state releases whatever the held keys had running there, and nothing is resumed on return.

- Pausing stops the paddle and unpausing starts from rest, so the key has to be pressed again. Simple, predictable, and how many games behave.
- Cost: a player who never lets go of the key has to release and press it once after every pause or menu.

B and C differ only on return: B resumes a held key, C waits for a fresh press. Switching to C means dropping the resume half of `syncHeldKeysToState`.

## Second batch: alternatives

- **A frame-scoped command queue.** Input builds a list of intents (a command with a target and an amount); update consumes it, validating and applying. The queue is cleared every tick. Alternative rejected: a queue inside every object. Benefits listed: determinism, recording and replay, an AI that issues the same commands, and network sync later. The three-layer indirection above is the same idea seen from the XML side.
- **Actions as data.** The key, the named action and the effect stay decoupled, so a binding can be swapped without touching physics.
- **Many-to-one bindings.** Two players' inputs combined into one object (sum, same direction, opposite, majority, with momentum) would need a way to name several sources for one action ([28](28-game-ideas-and-test-games.md)).
- **Few actions, not many buttons.** Complaints about games with a button per command and hold-to-interact everywhere point the same way: give the player a small set of named actions and let context choose the effect.
- **Axis inversion** is a binding attribute rather than a game rule, if it is added.

## Sources

- "Engine design conversation A (a long general chat; only the project segment was used)" (2026-05-23): the named-action design and the remapping benefit.
- Second batch: "Game Engine Command Queue" (2026-01-17), "Pong with Third Paddle" (2026-07-12), "Game controls and sluggishness" (2025-11-23).
