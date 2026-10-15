# Scenes

**Status:** Idea, nothing built. First written 2026-10-14 as "rooms"; renamed and reshaped with the author's direction on 2026-10-15.

## What we looked for

A game made of several places, one screen each: Berserk's four rooms, Pitfall's jungle screens (four of the 255), Kaboom's three waves, Adventure's castles and rooms (one is built, `adventure.xml`). Today every one of them is a `<state>`, and the states do two jobs:

- **Modes** of the program: an attract screen, an options menu, the main game, pause, game over. A stack of these is what a state machine is for, and `<push>` and `<pop>` say it well ([states-and-conditions](07-states-and-conditions.md)).
- **Places**: room1 to room4, screen1 to screen4, wave1 to wave3. They are states only because there is nowhere else to put a place.

What the second job costs, read from the games on `working` at 771ae06:

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

## Proposed

```xml
<state name="playing">
  <objects>
    <object name="harry">...</object>             <!-- in play in every scene -->
  </objects>
  <scenes>
    <objects>                                     <!-- the kit: pieces scenes take -->
      <group name="logs" class="log">...</group>
      <object name="tar" class="hazard">...</object>
      <group name="crocs" class="croc">...</group>
    </objects>
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
- **The kit sits at the top of `<scenes>`**, as named `<keys>`, `<shows>` and `<conditions>` sets sit at the top of `<states>`: the shared part named higher up. Kit pieces are not in play by themselves. A scene takes one by naming it in its own `<objects>`: with nothing inside, as it is; with parts inside, changed as a member changes its group (a sprite of the same shape changes only what it gives). A name the kit does not have must be complete. Each scene that takes a piece gets its own copy, so Pitfall's logs on screen 1 and screen 3 roll on their own.
- **A scene can take another scene and change parts** (`from=`), and a scene's own `<variables>` are its parameters (`screen3.logspeed`, read by the objects it holds). Each copy is a real object made at load, so values worked out at load see their own scene's numbers with no change to evaluation. Kaboom's waves become one wave and two that change the pace and the drop.
- **Going to a scene is a command, `<enter scene="room2" />`**, run wherever commands run: Berserk's exit objects in a collision rule, Pitfall's edge rules (with `<wrap />`, which already puts Harry on the far side), Kaboom's score condition, Adventure's gate. A verb rather than a map, because every game here already has the place where the move happens.
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

Between scenes, it goes through the state's own objects (the score, the lives, what the hero carries belong to `playing`, which every scene sees). Between states (who won, for one end screen instead of two), see Open.

### The game's own objects

Kept, since every game depends on them. With states and scenes owning objects, they become the outermost owner: what every state sees. Whether a game still needs any once states own their objects is a question for later, not a reason to change the 24 games now.

## What it touches

- **Engine.** An object gets its owner (game, state, scene); the game gets a current scene per state; "running" and "drawn" are worked out from the owner and the state stack instead of one flag. The reader builds owned objects as it builds a group's members, and resolves names inside out. `enter` and `reset scene` are verbs in `verbs.cpp`.
- **Schema, generator, tests.** `<objects>` and `<scenes>` in a state in `xgedef.xsd`, with a kit piece taken in a scene allowed to be partial (as `innerGroupType` is); `data.xsl` writes a scene as it writes a group (a function per scene, one taking another by value); `test_generated_games.cpp` compares owners and scenes field by field. Berserk, Kaboom and Pitfall rewritten, and Adventure given more scenes, as the games that prove it.

## Open

- **Drawing what is below**: how a state says it (a tag in its `<shows>`, or an attribute), and whether a still scene's timers count.
- **Passing a value between states**: a condition handing the state it pushes the object that triggered it (Space Race's two win screens, high-score entry), or the destination reading a variable of the state it came from by path (`playing.score`). The second needs nothing new once states own objects.
- **`<random>` drawn again more easily.** Today it is drawn at load and again when its object is reset (`<reset />`, `<reset object>`, a game reset); a timer's interval is drawn every round. Entering a scene draws nothing. Open: whether entering a scene fresh (or `<reset scene>`) draws again, and whether a command should draw again without moving the object back.
- **A map of exits** on the scene (`<exit edge="right" scene="screen2" />`), as a shorthand for the edge-rule-plus-`<enter>` pattern, if Pitfall's 255 screens or Adventure's map make it worth a second spelling. Grid-map level data ([ideas](57-ideas.md)) would sit inside a scene.
- **Who entered, and from which side**, beyond what the state's objects already carry.
- A state with no `<scenes>` is a game as today; every shipped game keeps working unchanged.
