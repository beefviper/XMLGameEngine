# Pictures

What an object looks like: its sprites, looks, animation and poses, and how light and height change the drawing.

## Sprites

A `<sprite>` holds one shape. A `name` is needed only when an object has several (as [looks](#looks) or [animation](#animation) frames). A `<color>` left out is `color.white`.

| Shape | Contents | Draws |
|---|---|---|
| `<circle>` | `<radius>`, `<color>` | a filled circle |
| `<rectangle>` | `<width>`, `<height>`, `<color>` | a filled rectangle |
| `<text>` | pieces, then `<size>`, `<color>`, `<font>`; `style=` | [text](#text) |
| `<image>` | `<path>`, `<flip>` | an image file, loaded by the window library |
| `<line>`s | each `<from>`, `<to>`, `<color>`, `<thickness>` | [straight lines](#lines) |
| `<polygon>`s | each three or more `<point>`s, `<color>` | [filled shapes](#polygons) |
| `<bitmap>` | `<row>`s, `<scale>`, `<color>`s, `<flip>`; `style=` | a [picture written as text](#bitmaps) |
| `<svg>` | `<path>`, a part, `<scale>`, `<hide>`s, `<flip>` | part of an [SVG file](#svg-pictures) |
| `<tether from="a" to="b">` | `<thickness>`, `<color>` | a line between two objects' middles, redrawn every frame; in play while both are (a fishing line, a missile's trail) |

**The engine draws its own pictures.** Lines, polygons, bitmaps and SVGs are drawn once at load into the engine's own `Bitmap` (`buildPicture`, `load/expr.cpp`). So every window library shows the same pixels, an object's size is known with no window, and a `pixel` [collision](collisions.md#how-a-touch-is-found) tests exactly what is drawn. Text and image files are the exception: only a window library can measure them.

**Turning.** An object with a `<heading>` shows its picture turned, to the nearest whole degree, redrawn only when that changes. Lines are redrawn turned, so they stay sharp; a bitmap's pixels are turned one for one, so chunky pixels stay chunky. Every heading gives the same square, which is the object's size.

### Lines

```xml
<sprite>
  <line><from><x>10</x><y>2</y></from><to><x>20</x><y>2</y></to></line>
  <line><from><x>20</x><y>2</y></from><to><x>25</x><y>7</y></to><color>color.red</color><thickness>2</thickness></line>
</sprite>
```

- Points are pixels from the sprite's top left, never negative. `<thickness>` is at least 1 (default 1).
- A thick line is a round-ended bar of that width at any angle, so turned it is the same shape turned (what a flipper touches with).
- No fill, and no mixing with another shape. Lunar Lander's moon is one object of fifteen lines.

### Polygons

```xml
<sprite>
  <polygon>
    <point><x>0</x><y>600</y></point>
    <point><x>0</x><y>450</y></point>
    <point><x>390</x><y>270</y></point>
    <point><x>800</x><y>445</y></point>
    <point><x>800</x><y>600</y></point>
    <color>color.forestgreen</color>
  </polygon>
</sprite>
```

- Corners in order, the last joined to the first. Several are drawn in order, a later over an earlier. A pixel is filled when its middle is inside.
- Drawn once like lines, so `pixel` collisions test it and a [`<hole>`](commands.md#touching) cuts it (Cannon Hill's craters). Not mixed with lines or other shapes.

### Bitmaps

```xml
<sprite>
  <bitmap>
    <row>..*...*..</row>
    <row>..*****..</row>
    <row>.**w*w**.</row>
    <row>*********</row>
    <scale>5</scale>
    <color>color.green</color>             <!-- the * pixels -->
    <color char="w">color.white</color>    <!-- the w pixels -->
  </bitmap>
</sprite>
```

- `.` is clear, `*` the plain `<color>`, and each `<color char="x">` names one more character.
- Each character is a `<scale>` by `<scale>` block (whole, default 1): the picture above is 45 by 20.
- Rows must be equally long and hold only named characters; anything else stops the load (`row 3 of a bitmap has 'o' as character 2`).
- Scale and colours can come from a [style](format.md#styles); `<flip>horizontal</flip>` or `vertical` mirrors it.

### SVG pictures

```xml
<sprite>
  <svg>
    <path>assets/Space Invaders Color Sprites.svg</path>
    <x>96</x> <y>64</y> <width>32</width> <height>32</height>
    <scale>2</scale>
    <hide>backdrop</hide>
  </svg>
</sprite>
```

- `<x>`, `<y>`, `<width>`, `<height>` (all four or none) pick a part of the drawing, so one sheet serves many objects. `<scale>` is pixels per unit (need not be whole). Each `<hide>` is an element id to leave out.
- The drawing keeps its own colours; a pixel with anything drawn counts as solid.
- Drawn by the engine with lunasvg (`picture/svg.cpp`). A missing file, a part outside the drawing or a scale of 0 or less stops the load.

## Text

A `<text>` is pieces, in any order, then `<size>`, `<color>` and `<font>` (or a `style=` for those it leaves out):

```xml
<text style="hud"><content>SCORE</content><number>ship.score</number></text>   <!-- SCORE 120 -->
```

| Piece | Shows |
|---|---|
| `<content>PONG</content>` | the words as written |
| `<number>paddle1.score</number>` | a number; one `owner.variable` is live and redraws when it changes, anything else is worked out once |
| `<letter>entry.first</letter>` | a number as a letter, 0 is A and 25 Z, wrapping: initials are three variables |
| `<choose><by>value</by><word>LOVE</word><word>15</word>...</choose>` | one of its words, the first for 0 (past either end, the end word), live like a number |

- Pieces are joined with a space; letters side by side make one word (`ABC 120`).
- In a group, a piece naming the group's own variable (`table.score` in the group `table`) shows each member's own.
- A text whose look or width changes is measured again, so a centred one stays centred.
- Drawn with the style's font, else `assets/tuffy.ttf`, unsmoothed. If a font file is missing, each window library says so once and uses the built-in 8 by 8 font.

## Animation

```xml
<object name="crab">
  <sprite name="open"> <bitmap> ... </bitmap> </sprite>
  <sprite name="closed"> <bitmap> ... </bitmap> </sprite>
  <animation>
    <interval>1</interval>
    <frame sprite="open" />
    <frame sprite="closed" />
  </animation>
</object>
```

- `<interval>` is seconds per picture (above 0); each `<frame>` picks a sprite. At least two frames, all pictures of one size; an unnamed animation shows every sprite.
- It moves on only while its object is in play, so a pause holds it; a reset starts it again. A group's cells start together, so a block of aliens flaps as one.
- In a group, a member, row, column or cell giving sprites changes the group's of those names.

## Looks

An object with several named sprites and no unnamed animation has **looks**. It starts as the first; `<become sprite="name" />` shows another; a reset brings back the first.

```xml
<collision object="bailey" sprite="white">     <!-- only while this floe is white -->
  <become object="row1" sprite="blue" />
  <inc variable="bailey.score">10</inc>
</collision>
```

- **A look is a state the player can see.** A rule with `sprite=` runs only while its object shows that look, as it was when the touch happened (so rule order does not matter, and one touch can turn a look back and forth).
- **A look can count hits**: a brick `whole` then `cracked`, the second hit `<die />`.
- **A look tells the next state something**: an end state's `winner` gets `<become object="winner" sprite="player2" />` before it is pushed.
- `<become object="ghosts" sprite="blue" from="normal" />` changes only those showing `normal`.
- With a `<heading>` every look is kept to turn. A text with a `<number>` or `<letter>` cannot be a look.
- **Looks that move.** A named `<animation name="walking">` is a look, picked by name like a sprite. The looks are the named animations in order, then the sprites they leave out. Animations are all named or none.

## Poses, mirror, grip and blinking

Three parts, after `<facing>`, give a walking, jumping thing its look and feel without a rule for each:

```xml
<poses>
  <standing sprite="stand" />
  <walking sprite="walking" />
  <skidding sprite="skid" />
  <rising sprite="rising" />
  <falling sprite="falling" />
  <thrusting sprite="burning" />
</poses>
<mirror>left</mirror>
<grip>0.15</grip>
```

- **`<poses>`**: the engine picks the look each frame. `standing` is needed; the rest fall back (walking to standing, skidding to walking, falling to standing, rising to falling, thrusting to standing). Thrusting while a `<thrust>` is held; rising then falling in the air; skidding when its grip slides it against its keys. Poses are picked only while a pose look shows, so a `<become>` to another look (Mario's `dead`) holds.
- **`<mirror>left</mirror>`** (or `right`): the way its pictures do not face; going that way they are drawn flipped. The way is the move key held, else a jump's way, else its velocity. Not with a `<heading>` or a `<hole>`.
- **`<grip>`** (above 0 up to 1, default 1): how quickly its speed follows its keys, that share of the way each frame, so `0.15` gets going slowly and slides. It also brakes an object with no keys held, so give it only to something a player steers.
- **With a `<heading>`, `<grip>` is a wheel's**: each frame its sideways speed loses that share, so a car goes where it points and slides when the grip is low.
- **Blinking.** `<set variable="mario.blink">2</set>` blinks it for two seconds, counting down by itself. It is a live number, so a rule can leave a blinking player alone. A reset ends it.

### Surfaces

A piece with a `<surface>` is ground that changes how a gripping thing goes:

```xml
<group name="grass" char="g">
  <sprite><rectangle><width>40</width><height>40</height><color>color.forestgreen</color></rectangle></sprite>
  <surface><drag>0.08</drag><grip>0.3</grip></surface>
</group>
```

While its middle is over a surface (in play, drawn or not), a thing with a `<grip>` has that surface's `<drag>` and `<grip>` instead of its own; over several, the last in the file. Things without a `<grip>` (a shot, a path follower) feel nothing.

### Turning about a pivot

A `<pivot>` is the point of the drawing the object turns about; its `<position>` is where that point sits on the table.

```xml
<object name="leftflipper" from="flipper">
  <position><x>141</x><y>539</y></position>
  <heading>30</heading>
  <pivot><x>0</x><y>0</y></pivot>
  <actions>
    <action name="flip"><swing direction="counterclockwise"><degrees>60</degrees><seconds>0.1</seconds></swing></action>
  </actions>
</object>
```

- **`<swing>`** (held) turns the heading `<degrees>` its way in one stroke `<seconds>` long; let go, it comes back to where it started. Pressed again on the way back, it turns round where it is.
- **The stroke is eased**, as half a wave: from still, fastest half way, still at the end. Pinball's 60 degrees in six frames turns 4, 11, 15, 15, 11, 4.
- **The turn is swept** through the frame with every other move, so a touch is found where the flipper first meets the ball, however fast. Only a swing is swept; a `<turn>` or `<set>` of the heading jumps.
- **A turning thing lends the speed of the point that touches**: a [`<knock>`](collisions.md#knocks) off the tip goes faster than off the hinge.

## Light and dark

What is `<dark>` is drawn only where there is light. Something in play with a `<light>` lights everything within that many pixels of its middle.

```xml
<action name="strike"><set variable="player.light">match</set></action>
```

- `<dark>unlit</dark>`: drawn only while lit; `<dark>unseen</dark>`: from the first time it is lit until a reset, so a map fills in as it is explored.
- Darkness changes only drawing: a dark thing moves and touches as ever.
- The light is a live number, so a `<set>` strikes a match and a timer burns it down.

## Height and shadows

An object with an `<altitude>` is in the air over its position: it touches at the point on the ground, and is drawn raised.

```xml
<object name="shadow"> ... <shadow object="ball" /></object>      <!-- written first, drawn under -->
<object name="ball">
  ...
  <altitude><start>80</start><fall>0.25</fall><bounce>0.7</bounce></altitude>
  <collisions>
    <collision object="player">
      <condition variable="altitude"><below>reach</below></condition>
      <set variable="ball.rise">loft</set>
    </collision>
  </collisions>
</object>
```

- `<start>`, `<rise>` (speed up), `<fall>` (how much the rise drops a frame) and `<bounce>` (0 to 1) are each optional, 0 by default. It comes to rest once a bounce would be less than a frame's fall.
- `altitude` and `rise` are live numbers.
- `<shadow object="ball" />` keeps an object under another, middle to middle, in play while it is.
