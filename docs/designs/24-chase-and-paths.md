# Chasing, aiming and paths

**Status:** Built: `<chase>`, `<aim>`, maze walking, paths of straight steps, computer players. Ideas: curved paths, searching. Reference: [steering](../reference/commands.md#steering), [maps](../reference/maps.md#walking-a-map), [paths](../reference/timers.md#paths).

## Chasing and aiming

- The ask: "chase to get close, then aim to target, then fire". Berserk's Otto should come for the man, its robots shoot at him.
- Looked at: a behaviour property running every frame (needs its own frame order); **verbs that act once, run by a timer or a rule**; an `at=` on `<fire>` (could not face without firing). **Chosen: verbs,** so the timer says how often the chaser looks again, which is also how sluggish it is.
- `<chase object>speed<near>8</near></chase>` sets the velocity at the nearest one of a name; a heading turns to it.
- `<aim object />` stores the way; the next `<fire>`s go along it. `<lined>` refuses unless the target is on one of eight lines.
- Open: chasing round walls, leading a moving target.

## Walking a maze

- Pac-Man and four ghosts on a tile maze. **A `<maze map>` on the walker and three verbs for how it chooses at a tile:** `<go>` (a player's stick, taken at the first open tile), `<hunt>` (arcade targeting), `<wander>` (random). The walking itself is one engine pass, the same for all three (`play/maze.cpp`).
- **Arcade targeting, not a search:** of the open ways other than back, the one whose next tile is nearest the target; ties up, left, down, right. The four ghosts are four targets on one verb (the quarry, `<ahead>`, doubled `<with>` a partner, `<retreat>` within a range). A* was rejected as the default: getting stuck and doubling back is how the game plays.
- **Walls are the walker's own list**, by class and look, so the ghost house door is a wall to a hunter and open to eyes going home.
- **`<slow class>`** in the maze, as the ghosts crawl through the tunnel. Rejected: a rule that changes speed and must change it back.
- Rejected: one `<ai targeting>` attribute with a mode per ghost (packs choices into attributes), a verb per ghost (names a ghost, not a behaviour).

## Paths and formations

- Galaxian's aliens fly in, hold a formation, then dive: scripted motion flown by many objects.
- Looked at: a table of steps, a parametric curve, Bezier curves, a turtle command sequence. **Chosen: steps, the simplest.**
- **Relative steps, not points:** the same dive is flown from any slot, and a `<wrap />` mid-step does not upset it. Galaxian's dive adds up to the window's height plus an alien's, so a diver that misses wraps home.
- **Home is the formation:** `<home />` flies back to the object's position. A formation needs no grid of slots.
- **A step can run commands** as it sets off (bombs on the way down).
- **A path sets the velocity,** so the follower moves, wraps and collides as anything does.
- **`<stagger>`** sends a group one behind another; an object on a path ignores another `<follow>`.
- **Sway** is `<shift>` on a timer, moving objects and their home places together. **Aim** is a step whose `<x>` names something live, worked out as the step starts. **Escorts** are `<follow><nearest>2</nearest>`. A step's `self` is made for each object that can fly it, at load.
- Open: curves, easing, Galaga's capture beam.

## Computer players

- Tennis and Pong need a computer player. **A state's `<computer>`:** conditions whose commands are `<hold button>`; every one is tested each frame and the engine presses keys as a person would (`Engine::playComputer`). A second person could take its place, and no verbs are new.
- What made it possible: a condition reads an object's live numbers (`x`, `velocity.x`) when it has no variable of that name.
- Rejected: AI verbs on the object (a second vocabulary), a script of moves (a program), the demo stream (cannot react).
- Board games search instead ([board games](26-board-games.md#the-computer)). Open: a reaction delay.
