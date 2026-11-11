# Timers and paths

## Timers

```xml
<timers>
  <timer>
    <every><random><min>0.3</min><max>1.4</max></random></every>
    <reverse />
  </timer>
  <timer name="fuse">
    <after>3</after>
    <despawn />
  </timer>
</timers>
```

A `<timer>` goes off `<every>` so many seconds, again and again, or once `<after>` that long, and runs the commands after it.

- **Whose clock.** An object's timers (a group's are every member's) count while it is in play; a state's while it is current; a scene's while it is current. So a pause stops them.
- **Starting over.** A reset starts a timer over, and so does its object coming back into play (revealed, or a new copy fired or spawned). A push starts a state's timers over, so a state that pops itself after a while says `<after>`; a pop back lets them carry on.
- **The interval is worked out again each round**, after the commands, so a `<random>` waits a new time each time (Kaboom's bomber turns whenever he likes).
- Seconds become frames by the `<framerate>`; a timer never waits less than a frame, and `<after>0</after>` goes off on the first frame it counts.
- Conditions run last in a frame, so a scene a condition puts back is drawn once before its timers count: what must happen first goes in that condition.
- **Look timers.** `<timer sprite="blue">` counts only while its object shows that look, and starts over whenever a `<become>` gives it that look. Pac-Man's blue ghosts turn back `<after>fright</after>`, and a second pellet starts the fright over.
- A `name=` on a state's or scene's timer lets a [`from=`](states.md#scenes) replace it.

### Time left

A named object timer is also a number of its object: `energy.level` is the seconds the timer `level` on `energy` has left, rounded up (3, 2, 1, then 0).

```xml
<text><number>energy.level</number></text>
```

- Anything that reads a variable reads it: a text, a condition, a command.
- It keeps its value while not counting; a reset gives it its whole time; a `<set>` on it is undone the next frame it counts. The object cannot also have a variable of that name.
- A state's or scene's timer has no object to hold it; a clock everyone sees is a timer on a pictureless object of that state.

## Paths

```xml
<paths>
  <path name="dive">
    <speed>3</speed>
    <step><x>-24</x><y>-24</y><play sound="dive" /></step>
    <step><x>80</x><y>120</y><fire object="bomb" /></step>
    <step><x>-56</x><y>568</y></step>
    <home />
  </path>
</paths>
```

A `<path>` is a route written once in the game's `<paths>`, flown by any object that [`<follow>`](commands.md#steering)s it.

- A `<speed>` (pixels a frame), an optional `<start>` (where it is put when it sets off), then its legs.
- A `<step>` moves it by `<x>` across and `<y>` down from where the step began, in a straight line, running its commands as it sets off. In them `self` is whichever object flies it.
- `<home />` flies it back to its `<position>` (moved by any `<shift>` since): a formation is just the objects' places. At the end of the path it comes to rest.
- A path sets the velocity each frame, so the follower moves, wraps and collides as anything does. A `<wrap />` on the way leaves the rest of the step alone: Galaxian's dive adds up to the window's height plus the alien's, so a diver that misses comes back from the top to its place.
- A step's `<x>` or `<y>` naming something live, or `self`, is worked out as the step starts: `<x>ship.x - self.x</x>` dives toward the ship. Every other number on a path is fixed at load.
- An object on a path finishes it first; a reset, `<die />`, `<stop />` or `<reveal>` ends it. A timer `<every>0.1</every>` with `<follow path="lap" />` sends it round again whenever it finishes.
- One with a `<heading>` faces the way it flies.
