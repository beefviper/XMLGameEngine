# Scenes

**Status:** Built 2026-10-15 (the reference is [readme: Scenes](../readme.md#scenes)). First written 2026-10-14 as "rooms"; renamed and reshaped with the author's direction on 2026-10-15.

## What we looked for

A game made of several places, one screen each: Berserk's four rooms, Pitfall's jungle screens (four of the 255), Kaboom's waves (eight since 2026-10-19), Adventure's castles and rooms (five are built, `adventure.xml`). Today every one of them is a `<state>`, and the states do two jobs:

- **Modes** of the program: an attract screen, an options menu, the main game, pause, game over. A stack of these is what a state machine is for, and `<push>` and `<pop>` say it well ([states-and-conditions](07-states-and-conditions.md)).
- **Places**: room1 to room4, screen1 to screen4, wave1 to wave3. They are states only because there is nowhere else to put a place.

What the second job cost, read from the games on `working` at 771ae06, before the games were rewritten on scenes:

- **Copies.** Every object is global, so a room's walls, robots and tally are written once per room under new names (`walls1` to `walls4`, `robots1left`, `tally3`), and each room state lists them, resets them and pops to the next (`berserk.xml`, the states from line 1398). Kaboom writes its bomber, its bombs, its label and its state three times, differing in a speed and a drop rate. Roughly 600 of Berserk's 1,613 lines and 200 of Kaboom's 654 are the second and later copies (estimated from the line ranges, not counted tag by tag).
- **No pause over a place.** A paused state cannot show the room behind it without the room moving, because being shown is what makes an object move and collide; so pause shows only the HUD.
- **Nothing goes along.** A carried object (`<carry />`) cannot follow the hero into the next room, because the next room is another state showing other objects. This is what stops Adventure at one room.
- **Nothing remembers.** Leaving a room and coming back can only mean starting it again (a reset) or never having left it.

This is the "numbered bundles" item of [open work](58-open-work.md). The author ruled that it must not need XSLT to load and run a game (2026-10-14), and that nothing is packed into strings.

## What we looked at

| Option | Verdict |
|---|---|
| A **template** with parameters, expanded before load (by XSLT or by the reader) | Copies stay copies, only shorter; nothing goes along or remembers. Needs XSLT or a second expansion language |
| A **state that takes another state** and changes parts | Shrinks the states but not the objects, which are the bulk; still one state per place |
| **Rooms beside states**, at the top level (the first draft) | Two parallel lists; the place a room belongs to (the main game) is left unsaid |
| **Scenes inside a state; states and scenes own objects** | Chosen direction (the author, 2026-10-15) |

The name: `room` fits Berserk and Adventure, `screen` Pitfall, `wave` Kaboom. **`scene`** is the general word, and it sharpens what a state is: a mode of operation, which may hold scenes.

## Chosen

```xml
<state name="playing">
  <objects>
    <object name="harry">...</object>             <!-- in play in every scene -->
  </objects>
  <scenes>
    <scene name="screen1">
      <objects>
        <group name="logs" />                     <!-- taken as it is -->
        <object name="moneybag">...</object>      <!-- this scene's own -->
      </objects>
    </scene>
    <scene name="screen3" from="screen1">
      <objects>
        <group name="logs"><velocity><x>-2</x></velocity></group>  <!-- taken and changed -->
      </objects>
    </scene>
  </scenes>
</state>
```

- **States and scenes both own objects.** A state's `<objects>` are in play whenever the state is current, in every scene of it (Harry, the HUD of the main game). A scene's are in play only while it is the state's current scene. The game's own top-level `<objects>` stay as they are (below).
- **Objects are made the same way everywhere.** Any `<objects>`, in the game, a state or a scene, holds `<object>`s and `<group>`s with all their rules (members, columns and rows, groups inside a group).
- **The kit sat at the top of `<scenes>`** (until 2026-10-25, when it moved to the game, [kit](13-kit.md)), as named `<keys>`, `<shows>` and `<conditions>` sets sit at the top of `<states>`: the shared part named higher up. Kit pieces are not in play by themselves. A scene takes one by naming it in its own `<objects>`: with nothing inside, as it is; with parts inside, changed as a member changes its group (a sprite of the same shape changes only what it gives). A name the kit does not have must be complete. Each scene that takes a piece gets its own copy, so Pitfall's logs on screen 1 and screen 3 roll on their own.
- **A scene can take another scene and change parts** (`from=`), and a scene's own `<variables>` are its parameters (`screen3.logspeed`, read by the objects it holds). Each copy is a real object made at load, so values worked out at load see their own scene's numbers with no change to evaluation. Kaboom's waves become one wave and two that change the pace and the drop.
- **Going to a scene is a command, `<enter scene="room2" />`**, run wherever commands run: Berserk's exit objects in a collision rule, Pitfall's edge rules (with `<wrap />`, which already puts Harry on the far side), Kaboom's score condition, Adventure's gate. A verb rather than a map, because every game here already has the place where the move happens. (Built later: the command can name a way of the current scene instead, below.)
- **What is carried goes along.** On `<enter>`, an object carried by one of the state's objects moves to the new scene with it.
- **A scene keeps its state when left.** Coming back finds it as it was (Adventure's dropped sword). `<reset scene="room1" />` starts one again (Berserk's fresh robots), and the bare `<reset />` of a game over starts every scene again and makes the first one current.

### Names: dotted paths

An owned object is named by its path: `room1.walls`, `playing.room1.walls`, and a member as today, `playing.room1.robots.3`. A variable is still the last part (`playing.room1.tally.kills`, and `paddle1.score` as now). A plain name is looked for from the inside out: the scene's own, then its state's, then the game's, so the rules inside a scene say `robots` and mean their own.

For a path to read only one way, a name may not be given twice where both can be seen (a scene called `logs` in a state with an object called `logs`); the loader refuses it, naming both. Group member numbers are already dotted (`aliens.3.2`), so this extends the existing spelling rather than adding one.

### Being in play, split

Being shown is today what makes an object drawn, moving, timing and colliding at once ([readme](../readme.md#known-limitations), "an object that is not visible is not moved and does not collide"; [states-and-conditions](07-states-and-conditions.md), per-state behavior). Scenes need these apart:

- **Running** (moves, animates, counts its timers, collides): an object of the current state, or of its current scene.
- **Drawn**: running, or under a state that draws what is below it, still (pause over the game, the attract screen over a demo).
- **Seen**: an object can run and collide without being drawn, which makes an exit or trigger zone an ordinary object with no sprite shown, and an enemy on its way in from off-screen possible.

### Information between scenes and states

Between scenes, it goes through the state's own objects (the score, the lives, what the hero carries belong to `playing`, which every scene sees). Between states (who won, for one end screen instead of two), a look of the end state's own object, picked by the condition that pushes it ([states and conditions](07-states-and-conditions.md)), and numbers by path.

### The game's own objects

Kept, since every game depends on them. With states and scenes owning objects, they become the outermost owner: what every state sees. Whether a game still needs any once states own their objects is a question for later, not a reason to change the 24 games now.

## How it was built

- **One layout pass, before anything is worked out.** `layOutScenes` (`game/scenes.cpp`) turns the states' and scenes' objects into ordinary objects among the game's, each with its owner (state, scene) and its path as its name, and writes every name the file uses out in full. Everything after it (evaluation, the frame, the generated programs) sees plain objects with owners, so a generated game gets scenes from the same code. Since 2026-10-25 the takes and `from=` are worked out on the XML first ([kit](13-kit.md)), and `data.xsl` writes each state's and scene's objects whole.
- **Takes change whole sprites.** The note above said a sprite of the same shape changes only what it gives, as in a group. A take replaces the sprite of that name instead: a piece's sprite is already finished, and a whole sprite reads plainly (the field's walls in green are a green wall). As first built only position and velocity halves, `<hidden>`, `<drawn>`, `<variables>` and sprites could change; since 2026-10-25 a take follows the one change rule of the [kit](13-kit.md), collision rules and timers included.
- **`state.name` names a piece in every scene.** Each scene object also answers to `playing.rock`, so a rule of the state (the hero's shot) can say `rock` and mean whichever room's rock it meets. Where a short name is the end of several paths, the shortest wins; two as short stop the load.
- **Shadowing stops the load.** A scene object named like one of its state's, or a state's or scene's like one of the game's, would make a plain name read two ways, so it is refused rather than ruled on.
- **Running, drawn and seen:** running is owned by the current state (and its current scene) or shown by it; `<drawn>false</drawn>` runs unseen; `<show state="...">` draws another state still, under the current one (the first of the open questions, settled: a tag in `<shows>`, and a still state's timers do not count).
- **What is carried belongs to where it is carried.** On `<enter>`, an object a state-level carrier holds moves to the new scene; a reset gives it back to the scene it started in.
- **Ways, for exits written once** (2026-10-15). A take cannot change a piece's collision rules, so as first built Berserk's rooms and Pitfall's screens each wrote their exits out in full, only to change the scene they entered. A scene now names its ways (`<ways><way name="east" scene="screen2" /></ways>`) and a command says `<enter way="east" />` or `<reset way="east" />`, which looks the way up in the state's current scene when it runs. The exits move into the kit; each scene says only where its ways go (Pitfall 925 to 849 lines, Berserk 1236 to 1197). Looked at and not chosen: an edge map on the scene (`<exit edge="right" scene=... />`), which fits Pitfall's sides but not Berserk's gaps in the wall, and is a second way of saying what an object and a rule already say; letting a take change collision rules, which would make a take a partial rewrite of rules, harder to read than a name; a `<goto>` with a computed scene, which is a string to be parsed. A way is a name that picks a scene, so it stays an attribute. Berserk resets the room it goes into (`<reset way="out" />` then `<enter way="out" />`) instead of the one it leaves, since a kit piece cannot name the room it is in; the robots are back either way, and a room left now keeps its state until it is entered again. Since 2026-10-19 Berserk's rooms are `<map>`s and its exits part of each map (a map's pieces are its own, not the kit's), still leading by the way `out`.
- **`from=` copies the whole scene, and conditions and timers have names** (2026-10-15). As first built `from=` copied the objects and variables only, so Kaboom's and Megamania's later waves wrote their conditions out again to change one number and one scene. It now copies the ways, conditions and timers too, read in the new scene (its variables, objects and ways). For a few hours a scene's own conditions (or timers) replaced the copied list whole, since they had no names to be changed by; the author then asked for names. A `<condition>` or `<timer>` takes an optional `name=`, and everything a scene copies follows one rule, the one variables, ways and taken pieces already followed: what the scene writes is added, and one of the same name takes the copied one's place, where it was (`mergedByName` in `game/scenes.cpp`). A whole condition or timer is replaced, not changed part by part, as a take replaces a whole sprite. Atlantis is the game written for it: its third wave keeps everything of the second but writes its own `launch` timer (two lanes at once) and `quota` condition (a win). Looked at and not chosen: a scene's own list replacing the copied one whole (built first; it cannot keep one copied condition and change another); taking a copied one away (`<condition name="x" />` with nothing in it), which no game needs yet; names on an object's timers that a take could change, likewise. The name is only read by `from=`; elsewhere it is a label.
- **What a condition counts** (2026-10-15, settled). A class, or a name every scene shares (`robots`, the state's name for each scene's robots), counts what is in play: the state's own objects, the current scene's, the game's. That is what every game with waves or rooms means ("this wave is cleared"), and counting every scene would never let Megamania's first wave end while the second waits with its enemies. A name by its path (`room2.robots`) names that scene's objects, and is counted wherever the game is, so "no robots left in room 2" can be asked from anywhere. As first built only a single object named in full was counted outside the current scene; a group by its path now is too. Looked at and not chosen: a `scene=` attribute on a condition (a second way to say what the path says) and counting every scene for a class (breaks every wave game).
- **`<random>`** is drawn again whenever its object comes back into play (reset, `<reset scene>`, `<reveal>`, `<release>`, `<fire>`), not on entering a scene, which finds the scene as it was left.

## Open

- **Drawing again on entry**: whether entering a scene fresh should draw its `<random>`s again (today only `<reset scene>` does), and whether a command should draw again without moving the object back.
- **Grid-map level data** ([ideas](57-ideas.md)) would sit inside a scene; with ways, a map of screens is each screen's `<ways>`, which is enough for four screens and may not be for Pitfall's 255.
- **Who entered, and from which side**, beyond what the state's own objects already carry.
- **Taking a copied condition or timer away**: not built, since no game needs it yet. (Changing a piece's named timer in a take is built, [kit](13-kit.md).)
- **The game's own objects**: kept; whether a game still needs any once states own their objects is left for later.
