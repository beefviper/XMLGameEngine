# Input

**Keys are bound in states, never in objects.** An object names what it can do; a state binds a key to that name:

```xml
<object name="paddle1">
  <actions>
    <action name="up"><move direction="up">step</move></action>
  </actions>
</object>
...
<state name="playing">
  <inputs>
    <input><button>w</button><button>up</button><trigger object="paddle1" action="up" /></input>
  </inputs>
</state>
```

So remapping a key is one edit in one state, a key can do different things in different states, and one action name can mean different things on different objects.

## Keys

- One key is `button="w"`; several keys for the same commands are a `<button>` each, first.
- **Names** are lowercase: `a`-`z`, `num0`-`num9`, `numpad0`-`numpad9`, `f1`-`f15`, `space`, `enter`, `escape`, `backspace`, `tab`, `left`, `right`, `up`, `down`, `home`, `end`, `pageup`, `pagedown`, `insert`, `delete`, `pause`, `lshift`, `rshift`, `lcontrol`, `rcontrol`, `lalt`, `ralt`, and a few more in `engine/keys.cpp`.
- **The mouse and pads** are keys too: `mouse.left`, `mouse.right`, `mouse.middle`; for two pads `pad1.a`, `.b`, `.x`, `.y`, `.lb`, `.rb`, `.back`, `.start`, `.up`, `.down`, `.left`, `.right` (and `pad2.`), named for an Xbox pad. A pad's directions are its d-pad or its left stick past halfway.
- A mouse button counts only when it goes down over the game's own pixels, not on the title bar, a menu or the bars beside a stretched picture.
- **A key's commands run in order**, and a `<trigger from=>` answers the look its object had at the press, so one trigger changing the look does not let a later one answer the new look too.
- **`<otherwise>` in a trigger**: commands run only when the action it runs was refused (a `<place />` that may not go there, a `<pick>` of nothing). Not showing its `from=` look is not a refusal: then neither runs. For now only a key's trigger holds one.

  ```xml
  <trigger object="hand" action="lay" from="held">
    <otherwise><trigger object="hand" action="back" /></otherwise>
  </trigger>
  ```
- **Keys several states share** come with a state [`from=`](states.md#a-state-from-another) another: its own inputs go over the copied ones, key by key.

## A key's three moments

`on=` picks which moment an `<input>` answers:

| `on=` | Runs its commands |
|---|---|
| `press` (or left out) | as the key goes down; held verbs (`<move>`, `<accelerate>`, `<thrust>`, `<turn>`) stay on until it is let go |
| `hold` | every frame while it is down, each time as one press |
| `release` | once, as it is let go |

```xml
<input button="space" on="hold"><trigger object="arrow" action="charge" from="ready" /></input>
<input button="space" on="release"><trigger object="arrow" action="putt" from="ready" /></input>
```

- A key may have all three, each its own `<input>`. The same key at the same moment twice in one state's `<inputs>` (by `button=` or among `<button>`s) stops the load; a state `from=` another may rebind a key it copied.
- **`hold` and `release` answer only a press made in the current state**: Space held from the title into the game neither charges nor strikes.
- Putt Park builds a putt this way: held, an `<inc>` adds power and a `<smallest>` keeps it at 100; let go, a `<set>` gives the ball its speed.

## Pointing controls

`<input axis="mouse.x">` (or `mouse.y`, `pad1.x`, `pad1.y`, `pad2.x`, `pad2.y`) runs its `<trigger>` whenever that control moves, and a `<track />` in the action puts the object where it points: its middle at the mouse, or as far across as the stick is pushed, kept in the window. That is the Atari's paddle controller.

```xml
<action name="steer"><track /></action>
...
<input axis="mouse.x"><trigger object="bucket" action="steer" /></input>
```

- It runs only when the control moves, so keys can move the same object while the mouse rests.
- An axis input runs only `<trigger>`s, in a state's own `<inputs>`. A trigger's `from=` look is kept.
- An object on a [map of cells](boards.md#the-picker) tracks to the cell under the pointer.
- Every window library reads the mouse and pads; Qt (`xgegui`'s own window) reads the mouse only.

## Held keys and state changes

- **A held `<move>`** records its direction's step while down. On every key change the velocity along that axis is worked out from both directions, so Down held and Up tapped cancel, and two axes make a diagonal. A key on one axis leaves the other alone: walking does not stop a fall.
- **`<accelerate>`, `<thrust>`, `<turn>`** are held the same way but add each frame, so what was gained stays when let go. `<drag>` gives thrust a top speed.
- **One-shot verbs** (`<hop>`, `<jump>`, `<fire>`, a state change) run only on a real press. Two hops in one frame keep the later.
- **Facing.** An object with a `<facing>` faces the last key's way; with keys of both axes held, the diagonal, and a `<fire>` follows it.
- **The current state decides what a held key means.** On a state change, held keys stop driving what the old state bound and start driving what the new one binds. Hold Left in Breakout and pause: the paddle stops; unpause with Left still down and it moves again. Only held bindings resume, so Space held through the title does not pause the game.
