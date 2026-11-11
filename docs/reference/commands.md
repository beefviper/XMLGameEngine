# Commands

A command is a tag in a list: a collision rule, an action, a key's `<input>`, a condition, a timer, or a path's step. A list runs in the order written.

## Where a command can go

Commands are sorted by what they need. Each place takes only the groups that mean something there; anything else is a schema error (and, read without the schema, a load error listing what can go there, from each verb's places in `xge/lib/verbs/`).

| Group | Commands | Needs |
|---|---|---|
| changes | `inc`, `dec`, `set`, `copy`, `rank`, `play`, `silence`, `become`, `reveal`, `follow`, `shift`, `enter`, `reset`, `spawn`, `despawn object=`, `trigger`, `count`; for [holders](cards.md#holders) `send`, `distribute`, `order` | nothing: they name what they change |
| flow | `push`, `pop`, `turns`, `pass`, `out`, `demo`, `undo` | the game's running order |
| self | `die`, `drop`, `move`, `release`, `reverse`, `stop`, `fire`, `despawn`; on cells `settle`, `slide`, `cycle`, `grow`, `split`, `pick`, `place`, [`put`](boards.md#pieces-put-not-moved); a hand's [`call`](cards.md#holders) | an object running it |
| steering | `aim`, `chase`, `hunt`, `wander` | an object to steer |
| touch | `carry`, `deflect`, `hole`, `knock`, `land`, `ride`; on cells `shove`, `merge`, `through` | another object touched |
| edge | `stick`, `wrap` | a screen edge |
| bounce | `bounce` | a screen edge or another object |
| controls | `climb`, `go`, `hop`, `jump`, `swing`, `thrust`, `turn`, `track` | a key held or pressed |
| cells | `clear`, `collapse`, `fall` | what a [map's check](maps.md#checks) found |
| press | `hold`, `think` | a [computer player](states.md#computer-players) |

| Place | Takes |
|---|---|
| a `<collision>` with another object | changes, self, steering, touch, bounce |
| a `<collision edge>` | changes, self, edge, bounce |
| an object's `<action>` | changes, self, steering, controls, and `<condition>`s holding those |
| an object's `<timer>`, a path's `<step>` | changes, self, steering |
| a state's or scene's `<input>`, `<condition>`, `<timer>` | changes, flow |
| a `<computer>`'s `<condition>` | press |

## A press that cannot be done

**A verb that is refused ends its list**: what is written after it is not done; what is before it has run. In a collision, only that rule's rest is dropped; later rules still run.

```xml
<fire object="bullet" /><play sound="shoot" />   <!-- sounds only when a shot leaves -->
```

Refused are: a `<fire>` or `<spawn>` at its `<most>`; a `<hop>` or hunt by hops in the air or landing outside the window; a `<jump>` off the ground; an `<aim>` with `<lined>` not lined up; a [`<pick>`, `<place>` or `<put>`](boards.md#the-picker) that is no move; a card [`<send>` or `<distribute>`](cards.md#holders) with nothing to send; a spawn into a taken cell. So write the verb first and its answers after.

An object out of play does nothing when its action is pressed or triggered; letting go still lets go.

## Moving

| Command | Does |
|---|---|
| `<move direction="up">step</move>` | in an action, held: `step` pixels a frame while the key is down ([held keys](input.md#held-keys-and-state-changes)). In a rule: shifts it once |
| `<hop direction="up">distance</hop>` | once a press: a step made before collisions, judged where it lands; refused off the window. Diagonals `up-left` ... `down-right`, by `distance` or `<x>` and `<y>` |
| `<seconds>`, `<arc>` in a `<hop>` | it takes that long through the air, touching nothing until it lands; `<arc>` lifts its [altitude](pictures.md#height-and-shadows) to that height half way |
| `<jump>height</jump>` | a jump up under its own pull to about that height; only from the ground, fixed sideways until a `<land />`. Needs an acceleration down and a `<land />` rule |
| `<land />` | in a rule: coming down onto the other's top, it stands there. Any other side, or going up, does nothing (one-way platforms) |
| `<climb direction="up" class="ladder">step</climb>` | held: at an object of that class, gets on, lines up and climbs; at an end steps off onto what it lands on, or holds there if nothing is. Stacked pieces are one ladder. Run by a rule or timer, it takes one ladder and lets go |
| `<thrust burn="fuel">amount</thrust>` | held: adds to the velocity every frame along its `<heading>`; `burn` takes 1 off its own variable each frame and stops at 0 |
| `<thrust direction="up">amount</thrust>` | held: the same one way across the screen, whatever its heading (it needs none) |
| `<turn direction="clockwise">degrees</turn>` | held: turns the heading that much a frame |
| `<swing direction="counterclockwise"><degrees>60</degrees><seconds>0.1</seconds></swing>` | held: one eased, swept stroke, and back when let go ([turning about a pivot](pictures.md#turning-about-a-pivot)) |
| `<reverse />` | the velocity turns round |
| `<stop />` | still for good (velocity 0, no pull, no held keys) until a reset |
| `<track />`, `<track path="rail" />` | run by a [pointing control](input.md#pointing-controls): goes where it points; with `path=`, that far along the path |

## Steering

| Command | Does |
|---|---|
| `<chase object="hero">1.5<near>100</near></chase>` | heads straight for the nearest of that name at that speed, stopping within `<near>`. It acts once, so a timer repeats it |
| `<aim object="man" />` | its next `<fire>`s go straight at where that one was. `<lined>8</lined>`: only along one of eight ways, within 8 pixels, else refused |
| `<follow path="dive" />` | flies a [path](timers.md#paths); `object="aliens"` sends every one, `<stagger>0.15</stagger>` apart; `<nearest>2</nearest>` only the two nearest |
| `<shift object="aliens"><x>sway.way</x></shift>` | moves those objects, and the places they fly home to; a reset puts them back |
| `<go direction="left">speed</go>` | a [maze walker](maps.md#walking-a-map) turns that way at the first tile it can; on cells, keeps stepping |
| `<hunt object="pacman">2<ahead>4</ahead><with object="blinky" /><retreat object="corner">8</retreat></hunt>` | a maze walker, at every tile, takes the open way whose next tile is nearest a target: the quarry, `<ahead>` tiles in front of it, doubled away from a `<with>` partner, or a corner while within `<retreat>` tiles. Ties go up, left, down, right |
| `<hunt object="qbert"><hop><x>32</x><y>48</y><seconds>0.3</seconds></hop></hunt>` | hunts by hops: of four hops, the one landing nearest the target |
| `<wander>1.25</wander>` | a maze walker takes a random open way at every tile, never back |

## Touching

| Command | Does |
|---|---|
| `<bounce />`, `<bounce>0.8</bounce>` | mirrors the velocity about the way the surface faces, keeping that share of its speed (above 1: a bumper). A box's side or screen edge gives the plain flip; a `pixel` picture its drawn slope |
| `<deflect>45</deflect>` | off another object at up to that angle, by where along the side it hit; speed kept (Pong) |
| `<knock />`, `<knock>0.5</knock>` | the two meet as things with weight ([knocks](collisions.md#knocks)) |
| `<shove />`, `<merge variable="value" />` | on cells: pushes the piece in the way on a cell, or joins two equal ones ([pushing and merging](maps.md#pushing-merging-and-counting)) |
| `<stick />` | stops only the velocity into the edge or side touched, so it slides along |
| `<wrap />` | once wholly off a screen edge, put back in from the opposite one |
| `<through />` | a refused [cell](maps.md#cells) step is made anyway; the two share the cell |
| `<ride />` | lent the other's velocity this frame (a frog on a log) |
| `<carry />` | picks up what it touched and takes it along until a `<drop />`; one at a time |
| `<drop />` | puts down what it carries; on a [picker](boards.md#the-picker) (a [hand](cards.md#holders) too), puts back |
| `<hole>4<rough>0.5</rough></hole>` | cuts a round hole in its own picture where it was struck; a reset mends it. Lines, polygons or a plain bitmap only |

## In and out of play

| Command | Does |
|---|---|
| `<die />` | out of play (not drawn, moved or touched) until a `<reveal>` or reset |
| `<reset />` | in an object's place: back as it started. In a state's place: **a new game** (every object back, copies gone, scenes started again, the stack down to the first state), except what is [kept](states.md#keeps) |
| `<reset object="name" />` | that object or group back as it started, **every variable included**, and its copies gone. So never reset the player from another's rule: keep score on an object of its own |
| `<reset scene="room1" />`, `<reset way="out" />` | a [scene](states.md#scenes) back as it started |
| `<enter scene="room2" />`, `<enter way="east" />` | makes that scene current; the one left stays as it was |
| `<reveal object="blocks">count</reveal>` | brings back the first `count` (default 1) out of play, where they started |
| `<release object="debris">count</release>` | puts `count` into play at the middle of the object running it: copies of a piece, or from a pool |
| `<fire object="shot" />` | launches a copy (or pool member): along an aim; else from its facing side; else from its nose along a heading; else from its top with the shot's own velocity |
| `<spawn object="ball">` | a new copy, placed and moving as its `<position>`, `<velocity>`, `<acceleration>` say, read as it runs: `<x><centered>self.x + 5</centered></x>` |
| `<despawn />`, `<despawn object="bomb" />` | takes it, or every copy of that name, out of the game ([spawn and despawn](objects.md#spawn-and-despawn)) |

## Numbers

| Command | Does |
|---|---|
| `<inc variable="ship.score" />`, `<inc variable="ship.score">self.worth</inc>` | adds 1, or the amount |
| `<dec variable="frog.lives" />` | takes 1, or the amount, off; may go below 0 |
| `<set variable="target">value</set>` | an object's variable or a live number (`x`, `y`, `velocity.x`, `velocity.y`, `heading`, `altitude`, `rise`, `light`, `blink`). A group's name sets every member |
| `<copy variable="ship.score" into="best.score" />` | the second takes the first's number now |
| `<count class="black" into="score.black" />` | how many of that class are in play; with [`<here />` or `<around>`](maps.md#pushing-merging-and-counting), how many in those cells |
| `<rank variable="entry.score" into="table" />` | the entry takes its place in a [table of the best](states.md#a-table-of-the-best) |

The variable must belong to an object; one no object has stops the load.

## Look and sound

| Command | Does |
|---|---|
| `<become sprite="blue" />` | shows that [look](pictures.md#looks). `object=` changes that name or group (out of play too); `from="normal"` only those showing it; `style="angry"` a [palette swap](format.md#palette-swap) |
| `<play sound="zap" />` | a [sound](sounds.md), played once the frame's work is done; on a key a loop plays while held |
| `<silence sound="thrust" />` | stops that sound |

## Flow

| Command | Does |
|---|---|
| `<push state="paused" />` | puts that state over the current one |
| `<pop />` | back to the state below (never past the first) |
| `<pop state="result" />` | this state goes and that one takes its place |
| `<trigger object="ship" action="fire" />` | runs an object's action as one press. On a group, each member. `from="normal"` only those showing that look (on a key, the look at the press). In an object's own places `object=` may be left out. In a key's input it may hold an [`<otherwise>`](input.md#keys). `class="square"` with [`<around />`](maps.md#pushing-merging-and-counting): those of the class in the cells round it |
| `<turns>`, `<pass />`, `<out />` | players [taking turns](states.md#taking-turns) |
| `<demo />` | plays the recorded [`<demo>`](states.md#attract-mode) |
| `<hold button="l" />` | a computer player holds that key |

Actions that trigger one another in a circle stop the load, except through a trigger about the cells around, which reaches each piece once a press.

## Conditions in an action

An action may hold a `<condition>` among its commands. Its test is the same as a state's, with `self` and a bare `variable=` meaning this object; its commands run when it holds, in their place in the list.

```xml
<action name="look">
  <count class="spot" into="self.home"><here /></count>
  <condition variable="home"><equal>1</equal><become sprite="home" /></condition>
  <condition variable="home"><equal>0</equal><become sprite="crate" /></condition>
</action>
```

For now it holds commands only, no conditions.

## Mistakes the loader catches

A game that would play wrong stops loading, saying where and, for a likely typo, what was meant. Names are checked once everything is built, so a name used before what it names is fine.

```
object 'ball' > <collisions> > <collision>: unknown command <explode>
there is no key named 'spcae' (did you mean 'space'?)
```

- an unknown command, or one missing an attribute;
- a state, object, action, projectile, sound, look, scene or way that does not exist;
- a collision rule's `class=`, `unless=` or `through=` that no object has as its class, or an `object=` that names no object or group: such a rule would never run (a piece only ever spawned counts);
- a condition's `class=` or `object=` (in a state, a scene, an action, a rule or a `<computer>`) that matches no object, with the classes there are;
- a condition's `variable=` that nothing it looks at has, as its own variable or as a number the engine keeps (`x`, `moves`, ...);
- a `<show object=>` that names no object or group, and a `<trigger from=>` that is no look of what it names (a condition's or a rule's `sprite=` was already checked);
- a holder's `<by variable=>`, `<counts variable=>`, `<wild variable=>` or `<accepts>` condition, or a cell check's `<alike variable=>`, naming a variable no object has;
- an object, group, member, map or state called `self`, `others`, `touch`, `window` or `color` (the words a dotted name can start with), or anything called by a number. A variable or a sound may be called a word (a card's `color`).
- a key the engine does not have, a colour that is not a `color.` name;
- a command where it does nothing (a `<move>` in an `<input>`), listing what can go there;
- an `<inc>` or `<dec>` of a variable no object has.
