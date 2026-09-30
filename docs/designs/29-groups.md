# 29. Groups: shared description, separate objects

**Status:** built (`<group>` in `assets/xmlgameengine.xsd` and `game_xml.cpp`; `games/frogger.xml` and `games/spacerace.xml` are written with it; `tests/test_group.cpp`)

## The problem

A lane in Frogger is three logs that share a shape, a speed, a row and a rule (`wrap`) and differ only in where they start. A lane in Space Race is three bits of debris that share the same things. Written as separate `<object>`s each one was about 22 lines, so 42 of Frogger's 59 objects and 27 of Space Race's 37 repeated what their neighbours already said. A `<grid>` cannot help: it has one spacing and one velocity for its cells, and its cells are all the same shape.

## The tag

A `<group>` sits beside `<object>` under `<objects>`. It says what its members share, then lists the members. A member says only what is different.

```xml
<group name="logrow3" class="logs">
  <sprite> ... one brown rectangle ... </sprite>
  <position><y>3 * cell + inset</y></position>
  <velocity><x>-1</x><y>0</y></velocity>
  <collisions>
    <enabled>true</enabled>
    <collision edge="horizontal"><wrap /></collision>
  </collisions>
  <member><position><x>20</x></position></member>
  <member><position><x>272</x></position></member>
  <member><position><x>524</x></position></member>
</group>
```

One rule covers all of it: **whatever a member leaves out, it takes from its group**, and after that it must be complete, the same as an `<object>`; if it is not, the load fails naming the member. So:

- A member can give its own `<sprite>`, `<position>` or `<velocity>`. The hedges along the top of Frogger are one group of six where the two end hedges give a wider sprite.
- `<position>` and `<velocity>` can be given in halves: the group gives `<y>` (the row) and each member gives `<x>`. Each coordinate is taken from the member if it has it, otherwise from the group.
- A member cannot give its own `<collisions>`, `<actions>` or `<variables>`; those are the group's, so members are "orchestrated the same way". A member that needs different rules is an `<object>` beside the group.
- The group's children come in the same order as an object's (sprite, position, velocity, collisions, actions, variables), all optional except `<collisions>`, then the members. `class` is the group's and its members inherit it.

**Members are ordinary objects.** A group is a way of writing objects, not a new kind of thing at run time: `game_xml.cpp` reads it as one raw object per member and the rest of the engine never sees the difference. Each log wraps by itself, each pad `die()`s by itself. The engine only remembers which group a member came from (`Object::groupName`), so the group's name can be used wherever a name is.

**Names** follow the grid: a member is `logrow3.2`, the group's name and its number counted from 1 in the order written, unless the member carries `name="..."`. The group's name means all of it (`<show object="logrow3" />`, `object="pads"` in a rule or condition, `<reset object="logrow3" />`) and finds the first member for a lookup by name. Drawing order is file order, members in the order written, so a group is drawn where it stands in the file.

**Lockstep.** A group whose `<collisions>` says `<lockstep>true</lockstep>` gives all its members one lockstep number, so they move and bounce as one block, like the cells of a `<grid>`. Without it a group only shares a description. (Frogger and Space Race do not use it.)

## What it does not do

- It does not space members out. Positions are still written by hand, one per member. Anything regular (Space Race's three per lane are 272 apart) could use a spacing shorthand later, but that is `<grid>` territory and is not part of this.
- It replaces nothing: `<grid>` stays for a uniform block of identical cells, and a lone `<object>` stays for a lone object.

## Is it more compact?

Measured on the shipped files, before and after they were written with groups (same one-element-per-line layout on both sides):

| | Lines | Bytes | Tags |
|---|---|---|---|
| `frogger.xml` | 1526 → 1073 (-30%) | 37,614 → 26,079 (-31%) | 1047 → 699 (-33%) |
| `spacerace.xml` | 983 → 697 (-29%) | 23,569 → 16,674 (-29%) | 674 → 460 (-32%) |

About a third smaller, not a fraction of the size, for two reasons:

- **Members are still 5 lines each** (`<member>`, `<position>`, `<x>`, and two closers), about 224 lines in Frogger. Allowing a bare `<x>` in a member (`<member><x>20</x></member>`, three lines) would take another 80 lines off Frogger and 54 off Space Race (both about -35% overall), at the cost of a member's position no longer looking like an object's. Not done; it is a syntax question, not a structural one.
- **Every lane still repeats the same 7-line `<collisions>` block** (`wrap`), ten times in Frogger and nine in Space Race, about 130 lines between them. A group only shares within itself. Sharing between groups (a group of lane groups, the inner ones inheriting from the outer) would remove most of that, but needs a rule for merging a partly-given sprite (the lanes differ only in width) and is a bigger step than this one.

The rest of each file is untouched: in Frogger, 569 of the 1073 lines are scenery, the frog, the text objects and the states.

## Options considered

| Option | Verdict |
|---|---|
| One `<group>` per lane, members overriding what differs | Chosen: simple, one rule |
| Members give full `<position>` (x and y) and the group shares nothing but the rest | Simpler to explain, but repeats the row in every member |
| Group position as an origin the member offsets from | Avoids repeating anything, but reads worse and makes `wrap` starts (a member starting off-screen at a negative x) less obvious |
| Groups nested, inner inheriting from outer | Bigger saving (the repeated `wrap` block), needs merge rules for a sprite; a follow-up |
| Extend `<grid>` with per-row velocity and a list of positions | Fixes lanes only, and only for one shape per grid; groups also cover the hedges, pads and homes |
| A reusable named template that objects refer to | A different idea (see [27](27-object-composition-and-shape.md)); groups need no second definition to point at |
| Members able to override collisions, actions and variables | Not allowed: a member that acts differently is not "orchestrated the same way"; write it as an `<object>` |

## Naming note

The flag under `<collisions>` that makes the cells of a `<grid>` move as one block used to be called `<group>`. It is now `<lockstep>`, which is what it does, and `<group>` means only this tag.

## Open points

- **Nested groups** and a bare `<x>` in a member, above.
- **An amount per member.** A group of members spaced evenly (`count` and `gap`) would remove the last hand-placed numbers.
- **Test names.** The tests address members by the new names (`logrow6.1`, `carrow9.1`, `debris5.2`, `hedges.2`).

See also [21](21-frogger.md) for the game these lanes belong to.
