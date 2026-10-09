# Vision and the VGDL landscape

**Status:** Built; the open questions are at the end.

## What we want

- A video game description language in XML plus a C++ engine that runs it. One `.xml` file is the whole game.
- Target: 2D non-scrolling games of the late 1970s and early 1980s, described with a small closed vocabulary of verbs. Scrolling games later, with the same approach.
- The vocabulary grows only when a real game cannot otherwise be described. Like number systems: each extension is small and made because something real could not be said. The test for a good verb: it names a behavior (`ride` says an object rides another), not an implementation or one game's name for it.


## Declarative, with triggers

| Option | Verdict |
|---|---|
| A. Purely declarative (no loops, no conditionals) | Cleanest, but cannot say "game over at 15 points" |
| B. Embedded scripting / expressions | Expressive, but the file becomes a programming language in XML |
| **C. Declarative plus named triggers** | **Chosen.** Loops are the engine's per-frame update; conditionals are collision detection and `<condition>` ([states-and-conditions](states-and-conditions.md)) |

- Keep a **verb language** (pick known behaviors, give them parameters), not an **expression language** (compute anything). Plain arithmetic for positions across window sizes is accepted; function-call strings are not.
- Recognition over completeness: shared words (bounce, die) let a reader with no programming background follow a game. Tags read as nouns, attributes as descriptions, nesting as containment.
- Finiteness comes from a few mechanism families with parameters, not one verb per game. Difficulty grows with the number of contexts a mechanic appears in (elastic or not, solid or trigger), not the number of mechanics.
- The lineage argument: every game is a variation of Pong (Breakout = turn it 90 degrees, a grid of blocks that die; Space Invaders = blocks that step sideways and a ball that fires; Galaxian = invaders that fly in patterns). Small tweaks, repeated, rebuild the 1970s vocabulary. A written Pong-to-Galaxian chain of "which verb each step adds" is an unwritten idea.
- Origin: Pong in plain C++, pieces moved into XML one at a time. The current code is a ground-up rewrite that keeps the XML design; the prototype's collision code (detection and response tangled, hand-parsed action strings) is what [collisions](collisions.md) replaced.


## The VGDL landscape (reference only)

- Academic VGDL (Ebner et al. 2013; Schaul's `py-vgdl`) is declarative and text-based for 2D arcade games: hierarchical sprite sets with inheritance, a level map with a legend, collision-only interaction rules, win/lose termination sets. No scripting. It became the base of the GVGAI competition (survey: arXiv 1802.10363) and was largely replaced in AI research by Griddly (arXiv 2011.06363, YAML GDY, C++ core). LLMs have since been used to write VGDL directly (arXiv 2404.08706).
- Cousins: GDL (logic-based, verbose, nothing built in; the opposite philosophy), RBG and Ludii (board games), PuzzleScript (find-and-replace rules on a grid; the closest in spirit). Entity-component systems are the nearest structural neighbor.
- Why the field stayed small: built for one job (benchmark games for AI), a closed vocabulary means a closed language (a new dynamic needs engine code), and each generation is renamed and re-implemented.
- "No loops or conditionals" is really a production-rule system: the engine's tick is the loop and collision matching is the conditional. The price is the same everywhere: the language stops where its vocabulary runs out. XMLGameEngine makes the same bet with exprtk carrying per-rule arithmetic.
- Only the arXiv pages for GVGAI, Griddly and the two GitHub repositories were confirmed to exist; other links from the research were never re-checked.
- Open research angle: how small can a vocabulary be and still describe a class of games.


## Open

- Where the vocabulary boundary sits: scrolling as camera, world or verb; gravity as a world setting or a per-object force (built per object, [motion](motion.md)).
- Generating covers all twenty-three games ([generator](generator.md)).
