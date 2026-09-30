# 29. Groups: shared description, separate objects

**Status:** idea, with a prototype of the XML (`29-groups/frogger.xml`, `29-groups/spacerace.xml`). The engine cannot load a `<group>` yet, so the shipped games are unchanged.

## The problem

A lane in Frogger is three logs that share a shape, a speed, a row and a rule (`wrap`) and differ only in where they start. A lane in Space Race is three bits of debris that share the same things. Every one is written out as a full `<object>` of about 22 lines, so 42 of Frogger's 59 objects and 27 of Space Race's 37 repeat what their neighbours already said. A `<grid>` cannot help: it has one spacing and one velocity for its cells, and its cells are all the same shape.

## The sketch

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

One rule covers all of it: **whatever a member leaves out, it takes from its group**, and after that it must be complete, the same as an `<object>`. So:

- A member can give its own `<sprite>`, `<position>` or `<velocity>`. The hedges along the top of Frogger are one group of six where the two end hedges give a wider sprite.
- `<position>` can be given in halves: the group gives `<y>` (the row) and each member gives `<x>`. Each coordinate is taken from the member if it has it, otherwise from the group.
- The group's children come in the same order as an object's (sprite, position, velocity, collisions, actions, variables), all optional, then the members. `class` is the group's and its members inherit it.

**Members are ordinary objects.** A group is a way of writing objects, not a new kind of thing at run time: the loader expands it into separate objects and the rest of the engine never sees it. Each log wraps by itself, each pad `die()`s by itself. That is what the games need, and it means the change is contained in the XML reading (`game_xml.cpp`) and the schema.

**Names** follow the grid: the group's name means all of it (`<show object="logrow3" />`, `object="pads"` in a rule, `<reset object="logrow3" />`), and a member is `logrow3.2`, counted from 1 in the order written. A member may carry `name="..."` to be called something else. Drawing order is file order, members in the order written, so a group is drawn where it stands in the file.

## What it does and does not do

- It shares a *description*. It does not make the members move as one block; the `<lockstep>true</lockstep>` flag under `<collisions>` (invaders marching together) is a separate thing that a group member could also switch on.
- It does not space members out. Positions are still written by hand, one per member. Anything regular (Space Race's three per lane are 272 apart) could use a spacing shorthand later, but that is `<grid>` territory and is not part of this.
- It replaces nothing: `<grid>` stays for a uniform block of identical cells, and a lone `<object>` stays for a lone object.

## Is it more compact?

Measured on the prototype files against the shipped ones (same one-element-per-line layout on both sides, and not counting the 8-line proposal note at the top of each prototype). The prototypes were checked by expanding every group back to plain objects: all 69 come out identical to the originals.

| | Lines | Bytes | Tags |
|---|---|---|---|
| `frogger.xml` | 1526 → 1068 (-30%) | 37,614 → 25,744 (-32%) | 1047 → 695 (-34%) |
| `spacerace.xml` | 983 → 695 (-29%) | 23,569 → 16,525 (-30%) | 674 → 458 (-32%) |

About a third smaller, not a fraction of the size, for two reasons:

- **Members are still 5 lines each** (`<member>`, `<position>`, `<x>`, and two closers), about 224 lines in Frogger. Allowing a bare `<x>` in a member (`<member><x>20</x></member>`, three lines) would take another 80 lines off Frogger and 54 off Space Race (both about -35% overall), at the cost of a member's position no longer looking like an object's. Not chosen here; it is a syntax question, not a structural one.
- **Every lane still repeats the same 7-line `<collisions>` block** (`wrap`), ten times in Frogger and nine in Space Race, about 130 lines between them. A group only shares within itself. Sharing between groups (a group of lane groups, the inner ones inheriting from the outer) would remove most of that, but needs a rule for merging a partly-given sprite (the lanes differ only in width) and is a bigger step than this one.

The rest of each file is untouched: in Frogger, 564 of the 1068 lines are scenery, the frog, the text objects and the states.

## Options considered

| Option | Verdict |
|---|---|
| One `<group>` per lane, members override only what differs | Chosen for the prototype: simple, one rule |
| Members give full `<position>` (x and y) and the group shares nothing but the rest | Simpler to explain, but repeats the row in every member |
| Group position as an origin the member offsets from | Avoids repeating anything, but reads worse and makes `wrap` starts (a member starting off-screen at a negative x) less obvious |
| Groups nested, inner inheriting from outer | Bigger saving (the repeated `wrap` block), needs merge rules for a sprite; a follow-up |
| Extend `<grid>` with per-row velocity and a list of positions | Fixes lanes only, and only for one shape per grid; groups also cover the hedges, pads and homes |
| A reusable named template that objects refer to | A different idea (see [27](27-object-composition-and-shape.md)); groups need no second definition to point at |

## Open points

- **The name.** `<group>` used to mean something else in one place: a flag under `<collisions>` on a grid that makes its cells move as a block. That flag is now `<lockstep>true</lockstep>` (Breakout and Space Invaders use it), so `<group>` is free for this tag.
- **Tests** name objects (`logrow6a`, `carrow9a`, `debris5b`); they would become `logrow6.1`, `carrow9.1`, `debris5.2`.
- **Schema.** `objects` would hold a choice of `object` and `group`, and the member type is the object type with everything optional. `xsd_lite` would need the same.

See also [21](21-frogger.md) for the lanes as they are written today, and the note there about a `<grid>` with a velocity per row.
