# Collisions

`<collisions>` says how an object touches, then its rules:

```xml
<collisions>
  <type>pixel</type>                      <!-- optional: box (default) or pixel -->
  <solid char="*" />                      <!-- optional, pixel only: which bitmap characters touch -->
  <collision edge="vertical"><bounce /></collision>
  <collision class="bricks"><deflect>30</deflect></collision>
  <collision object="paddle" side="bottom"> ... </collision>
</collisions>
```

In order: `<off />` (no touches until a rule turns them on), `<lockstep />`, `<type>`, `<solid>`s, `<hitbox>`es, then the rules. Only objects in play with collisions on take part.

## Rules

- **A screen edge:** `edge="left"`, `right`, `top`, `bottom`, `vertical` (top and bottom), `horizontal` (left and right), `all`. An edge is met in the frame's sweep, at the moment the object reaches that side going out, and the rest of the step carries on from there; one already past it meets it at once, every step it moves. A `<stepped />` object or a piece of a cell map jumps, so it is asked before the move, by where it is.
- **Another object:** `class="..."` and/or `object="..."` (one object, a group, a copy's piece), both meaning AND. A `<collision>` with neither is about anything. An edge rule with either stops the load.
- **Every matching rule runs**, in the order written.
- **A rule holds rules.** What a `<collision>` says (its attributes and checks) every rule it holds says too, so a shared class or look is written once; an attribute it gives is not given again below it. Its `<first>` runs as a rule of its own, before them. Every one that matches runs, so there is no `<otherwise>`, and commands loose beside the rules it holds stop the load.

  ```xml
  <collision class="shot">
    <collision sprite="stub"><despawn /></collision>
    <collision sprite="half"><become sprite="stub" /></collision>
  </collision>
  ```
- What a rule can do is what its place takes ([where commands go](commands.md#where-a-command-can-go)).
- **`other` is what it touched.** In a rule about another object, `other` names the object touched wherever a command or a condition names an object or a variable's owner: `<inc who="other" variable="score">800</inc>`, `<reset who="other"/>`, `<condition who="other" variable="blink">`.
  - So a coin's one `<collision class="brother">` pays whichever brother took it.
  - `other` is whatever the rule touched, found as it runs. At load the rule is checked as if written for each object of the file it can touch (of its class, in its state), so `who="other" variable="score"` stops the load if one of them has no `score`.
  - Nothing it can touch stops the load, and `other` outside such a rule (an edge, an action, a state) names nothing.

## Rule filters

| Filter | Runs only |
|---|---|
| `side="top"` (`bottom`, `left`, `right`, `horizontal`, `vertical`) | when touched on that side of its own object: a ball falling onto a block is touched on its bottom |
| `sprite="cracked"` (edges too) | while its object shows that [look](pictures.md#looks), as the touch happened |
| `unless="logs"` (edges too) | when not also touching something of that class right now. Frogger's river: `<collision class="water" unless="logs">` |
| `hitbox="fist"` | for a touch made with its own [hitbox](#how-a-touch-is-found) of that name: a punch that lands |
| `by="fist"` | for a touch made by the other's hitbox of that name: being punched. With `hitbox="body"` too, a fist on the body, not on the guard |
| `through="floor"` | for a touch felt [through](#knocks) something of that class; without it, only a direct touch |
| `<condition variable="speed"><below>N</below></condition>` | a condition about itself reads `speed` as the touch happened, so a rule that stops it does not change which rules after it run: a landing and a crash on one pad |
| `<condition>`s, `<any>` (after those) | while every one holds, written as a [condition](states.md#conditions); several in a row all must hold, so there is no `<all>` here. One with no `class=` or `object=` is about the object itself. |

## How a touch is found

- **Swept.** Each object moves along its path for the frame, and the moment two first touch is found (boxes, circles, a circle against a rounded box), so a fast object cannot pass through a thin one. The earliest touch in the frame is handled first: everything moves to that moment, the pair's rules run, and the rest of the frame plays with the velocities left. Each pair reacts at most once a frame.
- **Who is looked at.** The pairs some rule answers for, worked out once at load. A pair is skipped unless one of the two has moved or changed: it has a velocity or rides, it is not where the last frame left it (a hop, a `<set>` of `x`, a `<shift>`, a spawn), it shows another picture (a look, an animation's frame, its size, mirrored), or it came into play or had its collisions turned on. A crocodile whose jaws open under a still Harry touches him. Being loaded or reset is not a move. An object mid-jump is in no pair.
- **What a touch is.** Worked out once, before either side's rules (`contactOf`, `play/geometry.h`): the edge of each touched, the point, the way apart, where along each side, how deep, and when in the frame. Rules read it as `side=`, `<bounce>` and `<knock>`.
- **`<type>pixel</type>`.** Swept as boxes first, then walked half a pixel at a time until a drawn pixel of one meets a drawn pixel of the other. Text and images cannot be `pixel`. The way apart is how what is drawn lies at the point (`pixelContactAt`), so a ball meets a drawn slope at its angle.
- **Turning.** A pair where one is [swinging](pictures.md#turning-about-a-pivot) is walked through the frame together, the turning one turned as it would be shown, until they touch. The frame itself is never run in smaller steps.
- **`<solid char="*" />`** after `<type>pixel</type>`: only those characters of the bitmap touch (Q*bert's cubes by their tops).
- **`<hitbox>`**: what touches other objects instead of the whole picture, so a near miss is a miss. Not with `pixel`.
  - **Pulled in from the sides**, named as the screen edges are: `<all>`, `<horizontal>` (left and right), `<vertical>` (top and bottom), `<top>`, `<bottom>`, `<left>`, `<right>`, one number each. A side takes its own word, then its pair's, then `<all>`; none is 0. It follows the picture's size, and left and right swap while the picture is [mirrored](pictures.md#poses-mirror-grip-and-blinking).
  - **Or a shape**: a `<rectangle>` or `<circle>`, then its `<position>` in the picture, which may reach past it (a fist). While the picture is mirrored it is placed from the right instead.
  - **Several** `<hitbox>`es are allowed, and touching any of them is a touch: a body pulled in, and a foot sticking out. A group's hitboxes are replaced, all of them, by a member that writes its own.
  - **On a frame:** a `<frame>` of an [animation](pictures.md#animation) may hold `<hitbox>`es after its `<seconds>`: they touch too, on top of the object's own (or its picture), only while that frame shows. A sword or a fist that is only out mid-swing.
  - **Named:** `<hitbox name="foot">` is what a rule's `hitbox=` and `by=` pick. Which hitboxes touched is looked at where the two meet (just touching counts), even in a pair that touches by pictures. A `hitbox=` its object has not, a `by=` that no object has, or either on an edge rule stops the load.
  - **The whole picture still meets** the window's edges, and anything a rule of either side stands on, stops at, bounces off or rides: a pair whose rules hold `<land />`, `<stick />`, `<stop />`, `<bounce>`, `<ride />`, `<knock>`, `<deflect />` or `<shove />` touches by pictures. So feet pulled in never sink into a floor. Carrying uses the picture too.

```xml
<hitbox><all>2</all><bottom>4</bottom></hitbox>      <!-- 2 in all round, 4 at the feet -->

<hitbox><all>6</all></hitbox>                        <!-- the body -->
<hitbox>                                             <!-- and a foot -->
  <rectangle><width>12</width><height>6</height></rectangle>
  <position><x>6</x><y>18</y></position>
</hitbox>
```

## Lockstep

`<lockstep />` in a group's `<collisions>` makes each innermost group one block: its members never touch each other, a rule's `<move>` moves them all, and one hitting a side with `<bounce />` turns the whole block, once: while the block is already going away from that side, the bounce is refused, so the rest of that rule (a `<move>` down) waits for the next turn. Each cell is still swept on its own, so a bullet meets only the cells alive. This is Prism Fall's march.

## Knocks

`<knock />` in a rule about another object makes the two meet as things with weight:

```xml
<collision class="ball"><knock>0.3</knock></collision>    <!-- on a flipper -->
```

- Along the way apart they share what they carry by their `<mass>`es; across it each keeps its own, so a ball hit off centre goes off at an angle.
- Each brings its speed at the touch point: its velocity, plus for a [swinging](pictures.md#turning-about-a-pivot) thing its spin times how far out the point is.
- The content, 0 to 1 (default 1), is how bouncy: 1 is pool balls, 0 sends them on together. Nothing happens unless they are closing, so a second `<knock />` for one touch does nothing.
- **No `<mass>` is as heavy as can be**: never moved, and its knock sends a thing off at up to twice its speed (Trick Shot's cue tip).
- **Settling.** Against something with no mass, a thing closing slower than four frames of its own pull into it does not bounce: a ball on a flipper lies there. A `<bounce>` up to 1 settles the same way; a bumper's never does.
- **Friction and rest.** With `<friction>`, a thing that knocks or bounces off something with no mass loses speed along the surface. When it settles it **comes to rest** (still, not pulled) if what it lies on is still and either its pull cannot slide it, or it is wedged between two surfaces. It wakes when something moves it or its support moves.
- **Through a thing with no mass.** A knock on something with no mass goes through to whatever with a mass rests on its far side, which then runs its own `through="class"` rules about the knocker. Mario's head under a floor tile is a `<knock>0</knock>` that reaches the shellcreeper on top, whose `<collision class="brother" through="floor">` flips it. One level only.
- A row of balls passes a knock on by itself, each pair meeting in turn within the frame.
