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

## Sources

- "Video game collection value in CAD" (2026-05-23): the named-action design and the remapping benefit.
