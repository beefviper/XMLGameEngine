# 10. Input and named actions

**Status:** implemented

## Decision

Three decoupled layers:

- An **object** defines what it *can* do, and names it: `<action name="up" value="move.up(step)"/>`.
- A **state** defines which buttons exist and which named action each calls: `<input button="q" action="action('paddle1','up')"/>`. It has no idea what the action does.
- The **key** is only a label: `button="q"`.

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

A key can be held while the state changes underneath it (hold Left, pause, unpause). The engine tracks every key's press and release itself, so the open question is what a state change should mean for a key that is still down. Three ways to answer it:

**A. Latched to the press (built).** A press runs the binding of the state that was active at that moment and remembers those commands. The matching release always goes to the same commands, whatever state is active by then. State changes neither cancel nor re-fire anything.

- Hold Left through pause and unpause: the paddle carries on afterward (it is not shown, so not moved, while paused) and stops when Left is released.
- Release Left while paused: the paddle is stopped, so it is not moving after unpause.
- Press Left while paused (no binding there) and hold it into `playing`: nothing happens until Left is pressed again, because its press ran nothing.
- Cost: the one case above where a key that is physically down does nothing until re-pressed.

**B. Live held keys (not built).** The state only gates what a held key means. On every state change the engine releases the outgoing state's bindings for the keys still down and presses the incoming state's bindings for the same keys.

- Pausing stops the paddle and unpausing starts it again with no re-press, and a key held into a state where it is bound acts at once.
- Cost: bindings that change the state would fire again on entry. Holding Space across `mainmenu` to `playing` would press Space in `playing` and pause immediately. The re-press would have to be limited to held actions (`action(...)`) and skip state-changing commands, or those keys would need to be marked as requiring a fresh press.

**C. Cancel on state change (not built).** Leaving a state releases whatever the held keys had running there, and nothing is re-pressed on return.

- Pausing stops the paddle and unpausing starts from rest, so the key has to be pressed again. Simple, predictable, and how many games behave.
- Cost: a player who never lets go of the key has to release and press it once after every pause or menu.

A and C agree when a key is released during the pause and differ when it is held straight through; A and B agree when a key is held straight through and differ for a key first pressed during the pause. A was kept because it needed the least new machinery and makes the release reliable; B or C can be added in `Engine::handleKeyPressed` / `handleKeyReleased` (plus a hook where the state stack changes) without touching the XML.

## Sources

- "Video game collection value in CAD" (2026-05-23): the named-action design and the remapping benefit.
