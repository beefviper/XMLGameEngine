# The game file

The shape of a game file, how numbers and names are written, and the game-wide parts: window, variables, styles, colours.

## Layout

One `<game>` root, its children in this order (the schema enforces it):

```xml
<game xmlns:xsi="http://www.w3.org/2001/XMLSchema-instance"
      xsi:noNamespaceSchemaLocation="../xgedef.xsd">
  <window name="Pong"> ... </window>   <!-- size, scale, background, frame rate -->
  <variables> ... </variables>         <!-- numbers the whole game uses -->
  <paths> ... </paths>                 <!-- optional: routes objects fly -->
  <styles> ... </styles>               <!-- optional: named text and bitmap looks -->
  <patterns> ... </patterns>           <!-- optional: what cards may make -->
  <objects> ... </objects>             <!-- optional: pieces, written once, never in play themselves -->
  <keeps> ... </keeps>                 <!-- optional: what a new game passes over -->
  <states>                             <!-- title, playing, paused, game over ... -->
    <objects> ... </objects>           <!-- optional: what every state shares -->
    <state> ... </state>
  </states>
  <sounds> ... </sounds>               <!-- optional: notes and music -->
  <pictures> ... </pictures>           <!-- optional: bitmaps sprites take, sheets -->
  <demo> ... </demo>                   <!-- optional: a recorded game -->
</game>
```

- **How it plays comes first; what it sounds and looks like last.** Sounds and pictures are long and say little about the rules, so they sit under the states, and a sprite takes a picture by name: `<bitmap from="ship" />` ([bitmaps](pictures.md#bitmaps)).

- **Where an object is written says how long it lasts,** and the deeper it is written, the shorter: the top-level `<objects>` never (they are [pieces](objects.md#pieces-and-from), taken below or only spawned), `<states>`' own `<objects>` for the whole game, a [state](states.md#states)'s while it is current, a [scene](states.md#scenes)'s while that scene is current. An object is only a definition; where it stands says whether, and for how long, it is in play.
- A game with no `xsi:noNamespaceSchemaLocation` is not validated. The schema checks shape and where each command may go; the loader checks names and meaning once everything is read.

## The format's one rule

**An attribute names or picks something; everything else is element content.**

```xml
<inc variable="paddle1.score" />                          <!-- names -->
<collision edge="left"> ... </collision>                  <!-- picks -->
<radius>10</radius>                                       <!-- a number: content -->
<chase object="hero">2</chase>                            <!-- a verb's one number: content -->
<jump direction="up">rowgap<seconds>leap</seconds></jump> <!-- more numbers: named tags -->
```

- Attributes are labels that cannot be computed: `name`, `class`, `object`, `variable`, `state`, `action`, `button`, `edge`, `direction`, `unless`, `sound`, `wave`, `pitch`, `to`.
- What something *does* is a list of command tags, run in the order written. No `;`-separated strings.
- An attribute holds one name. Several are an element each, written first: `<button>a</button><button>left</button>`, a row's `<number>`. A note's `pitch="F#4"` is one standard name, like a colour's.

## Values

Every number in a game (a position, a speed, a size, a threshold) is a **value**, written one of three ways. A value holds text or one tag, never both.

| Form | Example |
|---|---|
| arithmetic as text | `window.width.center - title.width / 2` |
| `<random>` | `<random><min>-7</min><max>7</max></random>` |
| `<formula>` | the same arithmetic as tags ([below](#arithmetic-as-tags)) |

- **Text** is numbers, names, `+ - * /`, a sign in front and brackets, worked out by the engine's own `load/calc`. Nothing more: a function call (`max(6, x)`) stops the load and says to write a `<formula>`.
- **`<random>`** draws from min to max (either order; each end text or a `<formula>`).

### When a value is worked out

Once, at load, except:

- a timer's `<every>` and `<after>`, worked out again each time it starts over;
- a position naming a text's or image's size, finished once the window measures it and again whenever that size changes;
- a command's values and a condition's test, read as they run ([below](#values-read-when-used));
- an object's number written `update="live"` ([below](#live-numbers-of-an-object)).

A `<random>` is drawn at load and again whenever its object comes back into play: a reset (of it, its scene or the game), a `<reveal>`, `<release>` or `<fire>`, or a new copy. So each Pong serve is new. A game-level `<random>` is drawn once; entering a scene draws nothing.

### Names a value can use

| Name | Value |
|---|---|
| `window.top`, `window.left` | 0 |
| `window.bottom`, `window.right` | the window's height, its width |
| `window.width.center`, `window.height.center` | half the width, half the height |
| a game `<variable>` | its value (worked out in the order written) |
| `object.variable` | that object's variable, wherever the object is written |
| `object.width`, `object.height` | its size (its sprite's footprint); meant for `<position>` |
| `object.timer` | a named object timer's [time left](timers.md#time-left) |
| `object.x`, `.y`, `.velocity.x`, `.velocity.y`, `.heading`, `.altitude`, `.rise`, `.light`, `.blink` | **live numbers**: its top left, how it goes, its [height](pictures.md#height-and-shadows), [light](pictures.md#light-and-dark), [blinking](pictures.md#poses-mirror-grip-and-blinking) |
| `self.` before any of these | in an object's own rules, actions, timers and numbers, the object running it |
| `touch.x`, `.y`, `.along`, `.depth`, `.normal.x`, `.normal.y` | in a rule about another object: the point, where along the object's side (0 to 1), how deep, the way out of the other |

An object's own `<variable>` named like a size or live number (`width`, `x`) wins over it.

### Values read when used

A command whose numbers name something that changes is worked out each time it runs:

```xml
<inc variable="player.score">self.worth</inc>                     <!-- the worth of the fish just landed -->
<set variable="shark.velocity.x">shark.velocity.x * 1.1</set>     <!-- speed the shark up -->
<above>gun2.score</above>                                         <!-- a test following gun 2's score -->
```

Every other value is fixed at load, and one that is not a command's (a position, a size) naming a live number stops the load, since nothing is anywhere yet, unless it is `update="live"`.

### Live numbers of an object

`update="live"` on an object's `<variable>`, a half of its `<position>` or its `<heading>` works it out again every frame from what it reads, after the move and before the conditions, and as the game starts. Left out (or `update="once"`), it is worked out once, as ever.

```xml
<position><x update="live">player.x + 18</x><y>0</y></position>      <!-- Demon Attack's shot, steered by the cannon -->
<heading update="live">self.level - self.side * self.elevation</heading>  <!-- Cannon Hill's barrel follows its elevation -->
<variable name="tipx" update="live"><formula>...<sine><value>self.heading</value></sine>...</formula></variable>
```

- It may read anything a command can: live numbers, other variables, `self`, a `<formula>`. A group's live half is worked out for each member, its place in the group added.
- No command can change it: a `<set>`, `<inc>`, `<dec>` or `<copy into>` naming it stops the load. Change what it reads instead. A live half is put back each frame whatever moved it; the other half moves as ever.
- Its heading and position are worked out before its variables, so a variable can read them. One reading another live number gets it as it was worked out this frame if that came first, else as it was last frame.
- Use it for what follows something (a steered shot, a part riding a turret, a number made from others). A snapshot (what the bet was, the power just fired) or a start that then changes (fuel that burns) stays once.

### Arithmetic as tags

A `<formula>` is one operation whose operands are elements, each a name, a number, a `<random>` or another operation. Precedence is the nesting, so nothing is parsed. Plain sums stay text; what text cannot say is a formula.

```xml
<x>
  <formula>
    <subtract>
      <minuend>window.width.center</minuend>
      <subtrahend><divide><dividend>title.width</dividend><divisor>2</divisor></divide></subtrahend>
    </subtract>
  </formula>
</x>
```

| Operation | Operands | Gives |
|---|---|---|
| `<add>` | `<augend>`, `<addend>`s | sum |
| `<subtract>` | `<minuend>`, `<subtrahend>`s | difference |
| `<multiply>` | `<multiplicand>`, `<multiplier>`s | product |
| `<divide>` | `<dividend>`, `<divisor>`s | quotient |
| `<remainder>` | `<dividend>`, one `<divisor>` | what is left, from 0 up to the divisor (-1 of 4 is 3) |
| `<smallest>`, `<largest>` | two or more `<value>`s | the smallest, the largest |
| `<absolute>`, `<negate>`, `<sign>` | one `<value>` | without its sign; turned over; -1, 0 or 1 |
| `<floor>`, `<ceiling>`, `<round>` | one `<value>` | down, up, nearest (a half away from 0) |
| `<sine>`, `<cosine>` | one `<value>` | of an angle in degrees |
| `<clamp>` | `<value>`, `<atleast>`, `<atmost>` | the value kept between the two |
| `<compare>` | `<value>`, then one of `<above>`, `<below>`, `<atleast>`, `<atmost>`, `<equal>` | 1 if it passes, else 0 |

- An operand is a **name or a number**, never an expression.
- Several second operands chain left to right: `a - b - c` is one `<subtract>` with two `<subtrahend>`s.
- `<compare>` uses a [condition's test words](states.md#conditions). Pac-Man chases for ever from wave four: `20 + 100000 ×` a `<compare><value>mode.waves</value><atleast>4</atleast></compare>`.
- **Dividing by 0.** A divisor certainly 0 at load stops the load, naming the `<divide>`. One that is 0 only because it is not known yet (another object's variable, an unmeasured text) gives 0, and a position or interval made from it is worked out again later. While running, it gives 0 and one warning per place.

## Names

Names are dotted, and every dot means "inside":

| Name | Is |
|---|---|
| `ball.radius` | the ball's variable `radius` |
| `logrow3.2`, `aliens.3.2`, `maze.dots.17` | a [group](objects.md#groups)'s member, a cell (column 3, row 2), a [map](maps.md)'s piece |
| `playing.hero` | the state `playing`'s own object |
| `playing.room2.robots.3` | a member of a scene's group |
| `playing.room2.pace` | a scene's variable |
| `shot.1` | a [copy](objects.md#spawn-and-despawn) made while the game runs |

- **Inside out.** A name is looked for from where it is written: the scene's own, the state's, the game's, then as the end of exactly one path. So inside a scene `robots` means its own, and `hero` means `playing.hero` anywhere.
- **Every scene's.** A scene's object also answers to `state.name` (`playing.robots`), the robots of whichever scene is met. Where a short name could mean two things the shortest path wins; two as short stop the load.
- A name given twice where both can be seen, or two scenes of one name, stop the load.
- A reference splits at its **last** dot into owner and variable.
- `class="..."` puts an object in a kind that rules and conditions match, as in CSS: a name picks one object, a class all of its kind.

## Window

`<window name="Pong">` (the title), then in this order:

| Element | Default | Meaning |
|---|---|---|
| `<width>`, `<height>` | required | the game's size in pixels, plain whole numbers |
| `<scale>` | 1 | each game pixel drawn this many window pixels across. Mario Bros. is 256 by 224 at `<scale>3</scale>`. The mouse is read back in game pixels |
| `<background>` | `color.black` | a [colour](#colours) |
| `<fullscreen>` | `false` | |
| `<framerate>` | 60 | frames a second, up to 255. It is how often the whole engine runs, keys and timers too, so a slow game keeps 60 and steps on a timer |

## Variables

```xml
<variables>
  <variable name="margin">30</variable>
  <variable name="paddle">margin * 4</variable>
  <variable name="start"><random><min>1</min><max>3</max></random></variable>
</variables>
```

Game-level variables are **named constants**, worked out in the order written. Declaring one twice keeps the later. Numbers that change belong to objects (`paddle1.score`); an object with no picture can hold them ([numbers only](objects.md#numbers-only)).

## Styles

A style is a named look taken by `<text style="...">` or `<bitmap style="...">`. What a text or bitmap gives itself wins.

```xml
<styles>
  <style name="banner"><size>64</size><color>color.red</color><font>assets/pressstart2p.ttf</font></style>
  <style name="mario">
    <scale>3</scale>
    <color>color.red</color>              <!-- the * pixels -->
    <color char="b">color.blue</color>    <!-- each b pixel -->
  </style>
</styles>
```

| Part | For | Meaning |
|---|---|---|
| `<size>` | text | a value, worked out with the game's names |
| `<scale>` | bitmap | pixels per character |
| `<color>`, `<color char="x">` | text takes the plain one; a bitmap all | colours |
| `<font>` | text | a font file under the games folder. Default `assets/tuffy.ttf`; `assets/pressstart2p.ttf` is 8 by 8 at size 8 (use multiples of 8), under the SIL OFL in `pressstart2p-ofl.txt` |

A `style=` naming no style, or a text left with no size, stops the load. The games name styles by role: `banner`, `prompt`, `help`, `keys`, `hud`.

### Palette swap

`<become style="angry" />` puts another style's colours on an object's bitmaps.

- Each bitmap that takes a style takes that one instead, for the colours it took from its style. Colours it gives itself stay, and so does a colour the new style leaves out, so a swap style names only what changes.
- The look stays (with `sprite=` too, both change); a reset puts the first colours back.
- Every palette a `<become>` can give is drawn at load, so a swap costs nothing in play.
- A style naming nothing, an object with no styled bitmap, or one with a `<heading>`, stops the load.

Mario Bros.' pests are drawn once per kind; one that gets up by itself does `<become style="angry" />`, and one `angry` style recolours every kind.

## Colours

Every colour is written `color.<name>`: `black`, `white`, `red`, `green`, `blue`, `yellow`, `magenta`, `cyan`, and the muted `grey`, `darkgrey`, `lightgrey`, `brown`, `orange`, `purple`, `darkblue`, `darkgreen`, `forestgreen`, `gold`, `peach` (`picture/color.cpp`). Any other name stops the load.
