# Rooms

**Status:** Idea, written up 2026-10-14 for the author to look over; nothing is built. The name is to be workshopped.

## What we looked for

A game made of several places on one screen each: Berserk's four rooms, Pitfall's jungle screens (four of the 255), Kaboom's three waves, Adventure's castles and rooms (one is built, `adventure.xml`). Today every one of them is a `<state>`, and the states do two jobs:

- **Modes** of the program: title, playing, paused, game over. A stack of these is what a state machine is for, and `<push>` and `<pop>` say it well ([states-and-conditions](07-states-and-conditions.md)).
- **Places**: room1 to room4, screen1 to screen4, wave1 to wave3. They are states only because there is nowhere else to put a place.

What the second job costs, read from the games on `working` at 771ae06:

- **Copies.** Every object is global, so a room's walls, robots and tally are written once per room under new names (`walls1` to `walls4`, `robots1left`, `tally3`), and each room state lists them, resets them and pops to the next (`berserk.xml`, the states from line 1398). Kaboom writes its bomber, its bombs, its label and its state three times, differing in a speed and a drop rate. Roughly 600 of Berserk's 1,613 lines and 200 of Kaboom's 654 are the second and later copies (estimated from the line ranges, not counted tag by tag).
- **No pause over a place.** A paused state cannot show the room behind it without the room moving, because being shown is what makes an object move and collide; so pause shows only the HUD.
- **Nothing goes along.** A carried object (`<carry />`) cannot follow the hero into the next room, because the next room is another state showing other objects. This is what stops Adventure at one room.
- **Nothing remembers.** Leaving a room and coming back can only mean starting it again (a reset) or never having left it; there is no "the sword is still where you dropped it".

This is the "numbered bundles" item of [open work](58-open-work.md). The author ruled that it must not need XSLT to load and run a game (2026-10-14), and that nothing is packed into strings.

## What we looked at

| Option | Verdict |
|---|---|
| A **template** with parameters, expanded before load (by XSLT or by the reader) | Copies stay copies, only shorter; nothing goes along or remembers. Needs XSLT or a second expansion language. The author asked for neither |
| A **state that takes another state** and changes parts | Shrinks the states but not the objects, which are the bulk; still one state per place, so pause and carrying stay unsolved |
| Objects **owned by a state** | Mixes the two jobs further: paused would own nothing and show nothing |
| **Rooms beside states**: a place owns its objects; states stay modes | Proposed below |

## Proposed

Places become a second axis beside the modes. A sketch (all names to be workshopped):

```xml
<rooms>
  <room name="room1">
    <variables><variable name="robotspeed">1</variable></variables>
    <objects>
      <object name="walls" class="wall">...</object>
      <group name="robots" class="robot">... velocity from room.robotspeed ...</group>
    </objects>
  </room>
  <room name="room2" from="room1">
    <variables><variable name="robotspeed">2</variable></variables>
    <objects>
      <object name="walls">... only what differs ...</object>
    </objects>
  </room>
</rooms>
```

- **A room owns its objects.** They are in play only while it is the current room. Objects outside any room are global as today: the player, the HUD, shot pools, the jungle every Pitfall screen shares.
- **A room can take another room and change parts** (`from=`): the rule of groups, `<shows>` and `<conditions>` sets once more, "take the shared thing, say only what differs". An object given again by name changes the taken one as a member changes its group (a sprite of the same shape changes only what it gives); a new name adds one. No way to take an object out, as with show sets.
- **A room's own variables are its parameters.** Its objects read them as `room.robotspeed`. Each room's objects are real objects made at load, so values worked out at load (all of them except positions and timer intervals) see their own room's numbers with no change to evaluation. This is what makes a template unnecessary: Kaboom's waves become one wave and two that change `pace` and `drop`.
- **Going to a room is a command, `<enter room="room2" />`**, run wherever commands run: Berserk's exit objects in a collision rule, Pitfall's edge rules (with `<wrap />`, which already puts Harry on the far side), Kaboom's score condition, Adventure's gate. A verb rather than a map, because every game here already has the place where the move happens; a declared map of exits is left open below.
- **What is carried goes along.** On `<enter>`, an object carried by a global object moves to the new room with it.
- **A room keeps its state when left.** Coming back finds it as it was (Adventure's dropped sword). `<reset room="room1" />` starts one again (Berserk's fresh robots), and the bare `<reset />` of a game over starts every room again and makes the first room current.
- **States stay modes.** A playing state shows the current room with the HUD, so there is one playing state, one paused state and one game over however many rooms there are. Conditions and timers that belong to a place go in the room (Berserk's "every robot shot" bonus, Pitfall's screen clock), the rest stay in the state.

## What it touches

- **Engine.** An object gets the room it belongs to; the game gets a current room; "in play" becomes shown and (global or in the current room). The reader builds each room's objects as it builds a group's members. `enter` and `reset room` are two verbs in `verbs.cpp`.
- **Shown and in play.** Pause drawing the room without moving it needs a state to say "draw the current room" apart from "run it". This is the open "per-state object behavior" of [states-and-conditions](07-states-and-conditions.md) and the "hidden but in play" limitation in the readme: rooms make it unavoidable, so the two are one piece of work.
- **Schema, generator, tests.** `<rooms>` in `xgedef.xsd`; `data.xsl` writes a room as it writes a group (a function per room, one taking another by value, as inner groups do); `test_generated_games.cpp` compares the rooms field by field. Berserk, Kaboom and Pitfall rewritten, and Adventure given more rooms, as the games that prove it.

## Open

- **The name.** `room` fits Berserk and Adventure, `screen` Pitfall, `wave` Kaboom; `level`, `stage` and `scene` are the general words. One word for all, since they are one idea.
- **Names inside a room.** Two rooms may each have `robots`. Proposed: a plain name in a room's own rules and in conditions means the current room's; a global object may not share a name with a room object; from outside, the room is named with an attribute (`<reset object="robots" room="room2" />`), not a dotted name, since dots already mean `object.variable`.
- **Drawing without running**: how a state says it (an attribute on a `<show>` of the room, or a separate tag), and whether a room's timers count while it is drawn and still.
- **A map of exits** on the room (`<exit edge="right" room="room2" />`), as a shorthand for the edge-rule-plus-`<enter>` pattern, if Pitfall's 255 screens or Adventure's map make it worth a second spelling. Grid-map level data ([ideas](57-ideas.md)) would sit inside a room.
- **Passing data into a room** (who entered, from which side) beyond the variables the global objects already carry.
- Whether a room that is never left (a one-place game) should be written as a room at all, or the global objects are simply the only room. Proposed: the latter; every game today keeps working unchanged.
