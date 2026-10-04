# 48. Berserk

**Status:** built (`games/berserk.xml`, `tests/test_berserk.cpp`); no new tags

## Why

The first Berserk was one of the three games another AI wrote from the schema alone ([44](44-games-written-by-another-ai.md)): one small maze, four red squares pacing sideways, and "next room" that was the same room again. The author asked for it to be looked up and rewritten, using what the language has learned since (timers, facing, looks, key sets, bitmaps, sounds).

## The arcade game (Stern's Berzerk, 1980)

A man in a maze of electrified walls with one to eleven robots. The walls kill him, and kill robots that touch them. Robots shoot only once the score is high enough, and only when lined up with the man; they walk towards him. He shoots in eight directions, one shot at a time. Every room has exits in all four outer walls; the next room is a different maze and he comes in at the side opposite the one he left by. A robot is 50 points, and leaving a room with every robot shot is worth 10 more for each. Stay too long and Evil Otto, a bouncing smiling face that cannot be shot and passes over walls, comes for him: slowly while robots remain, and as fast as the man (faster, after 5000 points) once they are gone. (From a web search of the arcade-history and Video Game History Wiki pages and a ColecoVision manual, and from memory; the details that were not in the pages are the least sure.)

## What was built

- **Four rooms**, each a maze written as rows of text (a `<bitmap>` of 73 by 49 characters at scale 8, 584 by 392 pixels) with the four exits as gaps in the outer wall, and its own robots (3, 4, 5, 6), coloured by room. A room is a `<state>`, so leaving is a `<push>` of the next, and the fourth leads to the first again. The mazes were laid out by a small program that adds random walls and keeps a wall only if every cell can still be reached, so every robot and every exit can be got to; its output is the file.
- **The man** has four looks (`<become>` in his actions picks the one for the key) and `<facing>`, so the picture and the shot agree. He is a box against the maze's pixels (the walls are `pixel`, so the gaps are real). One shot at a time, as in the arcade game: the one `bullet` object can only be in flight once.
- **Robots** patrol a corridor and turn at a wall or an exit (`<reverse />`), with two animated frames. Each fires from a timer along the way its group faces: a group is the robots of one axis, facing one of the two ways along it, so a robot's shots go the way its group faces whichever way it is walking. Their shots come from one pool of four shared by the room. The wait is a `<random>` whose ends are expressions of `player.depth`, the number of rooms the man has left, so the second lap fires faster.
- **Evil Otto** is a hidden object a room's timer reveals (every 14 seconds less the depth, never under 6), and at once when a condition sees that every robot of the room is gone. He bounces off the window's edges over the walls, kills the man on touch and cannot be shot (the shot is put away). Slow-then-fast was left out: he has one speed.
- **Exits and scoring.** Every rule that costs a life is the man's own (walls, robot shots, Otto, each room's robots), because `<reset />` in a rule puts only that object back at the start, while `<reset object="player" />` in somebody else's rule would put his score and lives back too. Walking into an exit adds to `player.exited`; a condition of the room sees it, resets what the room used, adds to `player.depth` and pushes the next room. A robot is 50; each room has a `tally` object counting the robots shot there, and a condition on it pays the room bonus (10 for each) when the count reaches the room's robots, then takes the count back off. The tally is reset when the man leaves, so shooting two of four and leaving does not count towards the next visit.
- **Extra man** every 2000 points, from a variable (`player.lifepoints`) that is added to with the score and taken 2000 off when it pays.
- Pause is P or Escape in the shared key set; a paused game shows only its message, so nothing moves.

## What the engine could not do, and what it taught

- **No chasing and no aiming.** Robots patrol and fire along the way their group faces; they do not walk towards the man or fire when he is lined up. This is the biggest difference from the arcade game, and needs a verb that sets a velocity from another object's position.
- **Four directions, not eight.** The man faces and shoots up, down, left or right.
- **Entering a room.** The man always comes in at the left exit (his start position), not at the side opposite the one he used.
- **Walls do not kill robots**; robots turn at them. A robot dying on a wall would need a rule for `class="wall"` that does `<die />` and the patrols would all die at once.
- **Conditions and commands read their numbers once, when the game loads.** A condition's `<atleast>` and a command's amount are worked out at load, so `<atleast>player.nextlife</atleast>` (to make a condition that moves its own goalposts) fires every frame, and `<inc>` of `player.kills` adds the load-time value. A timer's `<every>`, including the ends of a `<random>` in it, is worked out again each round, which is what Otto's wait and the robots' waits use. Counts that have to start again are variables of their own, reset by `<reset object>` of a small object that owns them.
- **`<reset object>` resets every variable of the object**, not just where it is (documented, but it cost a morning here: a robot's rule that did `<reset object="player" />` gave the man his lives back).

## Open points

A verb to chase or aim; eight directions (a `<facing>` of eight, and a fire that follows it); entering by the side opposite the exit (a reset to one of several named places); Otto speeding up when the robots are gone; robots dying on the walls and on each other; the arcade's rule that robots do not shoot until the score is high enough (the waits could start long and shorten with score, which needs only another expression).
