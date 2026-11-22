# Input

**Status:** Built, the mouse and pads too. Reference: [input](../reference/input.md).

## Three layers

- An **object** names what it can do (`<action name="up">`); a **state** binds a key to that name (`<input button="w"><trigger object="paddle1" action="up" />`); the **key** is only a label.
- So remapping is one edit in one state, one key does different things in different states, and one action name means different things on different objects, with no class hierarchy.
- Held `<move>`s record each direction's step and the velocity is worked out from all four on every key change, so Down held and Up tapped cancel and two keys make a diagonal. Replaced last-key-wins.
- **Shared keys** come with a state `from=` another, its own inputs going over the copied ones key by key. Named `<keys>` sets did this once; no game used them after `from=`, so they went.

## A press that cannot be done

- A ship played its firing sound with its shot already in flight; Q*bert squeaked when pressed in the air.
- **Chosen: a list stops at the first verb refused** (`<fire>` with none free, a hop in the air, `<jump>` off the ground). Each verb already knew; it now says so. Commands before it still run, which Berserk's `<aim>` before `<fire>` needs. The same holds in rules, keys and conditions; later rules still run.
- Rejected: running verbs first and the rest after (breaks aim-then-fire), a `<try><then>` wrapper (the same order in more tags).
- **An object out of play does not act:** its press or trigger does nothing, though letting go still lets go. Yars' hidden cannon fired from where it waited. Rejected: a test in every action, a state per case.

## Held keys across state changes

| Option | Behaviour |
|---|---|
| Latched to the press | Hold Left through pause and the paddle moves on the pause screen. Built first, replaced |
| **Live held keys** | The current state decides what a held key means. On a change the old bindings are let go and the new state's held verbs resume for keys still down, and a maze walker's `<go>` (the way asked for, as a joystick is). One-shots and state changes need a real press |
| Cancel on change | Press again after every pause. Simple but annoying |

Built by a state change count compared in `Engine::syncHeldKeysToState()` after each key event and frame.

## A key's three moments

- A putt, a plunger, a cannon: "hold to build power, let go to strike". A key could act only on its press.
- **Chosen: `on="press|hold|release"`** (press if left out, so no game changed), rather than a `<charge>` verb; a charge is hold, `<inc>` and `<smallest>`. `release` as a value does not clash with the `<release>` command, and a `<demo>` already writes a let-go key as `<release>`.
- **`hold` and `release` answer only a press made in the current state**, or Space held from the title would charge and strike.
- Open: a repeat interval while held; how long a key has been held as a number.

## Otherwise

- **A trigger's `<otherwise>`** (the author agreed, from the readable-rules work): card and board games tried one action and then another on the same key (lay, then put back; place, then pick), which worked only because the first left the second nothing to do.
  - Now the second is written as what happens when the first is refused.
  - Kept in the key's flat command list (the trigger counts the commands after it), so every walker over commands is unchanged; the engine skips them unless the action was refused (`Engine::runAsPress`).
  - Looked at first and rejected: nesting a key's triggers under the looks they answer, which made nothing shorter.
  - Open: an `<otherwise>` in actions, rules and timers.

## Seats

- **Wanted:** the computer plays a person's side without keys of its own, and the other person's keys do nothing against it, with no rules gating them. Before, Pong's computer held F11 and F12, and Up and Down ran actions that tested `players` first.
- **Chosen:** inputs sit in numbered `<seat>`s, `<people>N</people>` gives seats 1 to N to people, and `<computer seat>` presses the rest virtually. The engine refuses a keyboard press in a computer's seat and a computer's press in a person's, at the one place keys come in (`Engine::handleKeyPressed`).
- **Words by who plays:** `<people />` is a value, so Tennis and Fishing Derby say YOU and THE COMPUTER against the computer and PLAYER 1 and PLAYER 2 between people, by `<choose>` on a live variable holding it (a text's `<by>` reads only a name). A value tag, not a name, as the engine's own things are ([no reserved names](04-values-and-names.md)).
- **Looked at:** a toggle key on the title for person or computer in seat 2. Not needed yet: 1 and 2 on the title say it.
- **Turns:** a players group member's seat is its number, already its seat at a trick, so a turn game says nothing new. Its keys sit in seat 1 and in seat 2 `from="1"`, and answer the seat on turn. This replaced the rule that a person's keys miss a picker while a `<think>` condition holds (`answersPicker`).
- **Hold'em** has no computer keys left: both players wear the same choice looks (act, callonly, allin), set by one group action on whoever is on turn, so your keys trigger `players` and the computer presses them in seat 2. The end of a hand gives you the turn, so Space deals as your own key. No F-keys remain in any game.
- **Open:** pads following the seat; demos playing on their own fixed steps.

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
- **One pause key:** the Pause key (and a pad's Start) pauses and plays on in every game, and nothing else does.
  - Before, games paused on Space, P or Escape in eighteen mixtures, Space only where it did not fire.
  - The author's call: one key, the one named for it; Space is left to play again, and jump or fire.
  - All five window libraries read Pause; P was the fallback had one not.
  - Open: a laptop without a Pause key pauses with the pad or not at all.
- **1 starts every game** (the author): the arcade's one player button, beside 2 for two players where a game has them, so the title key says how many play.
