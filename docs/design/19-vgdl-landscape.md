# 19. The VGDL landscape

**Status:** research summary (reference only)

This entry condenses a research conversation on 2026-09-22. Papers and links were gathered by the assistant then. On 2026-09-29 the arXiv pages for the GVGAI survey (1802.10363) and Griddly (2011.06363) and the `schaul/py-vgdl` and `GAIGResearch/GVGAI` repositories were confirmed to exist; the other links were **not** re-checked.

## What a VGDL is

A domain-specific, declarative language for the rules and layout of 2D sprite games, compact enough to be generated and simulated automatically. Academic VGDL has two files, a game description and an ASCII level map. The game description has four parts:

| Part | Purpose |
|---|---|
| SpriteSet | Hierarchical list of object types; children inherit parents' rules and can override them |
| LevelMapping | Maps characters in the level file to sprite types |
| InteractionSet | Rules that fire only on collision between two sprites |
| TerminationSet | Win and lose conditions |

There is no general scripting: behavior comes from picking built-in sprite classes and writing collision rules. There is no dedicated Wikipedia article; the closest coverage is under [General game playing](https://en.wikipedia.org/wiki/General_game_playing), and the similarly named [Game Description Language](https://en.wikipedia.org/wiki/Game_Description_Language) is a different, older language.

## Compared with other approaches

| Approach | Idea | Relation to this project |
|---|---|---|
| GDL (Stanford, Genesereth) | Logic-based (Datalog-like), agents reason over the rules; very verbose because nothing is built in | Opposite philosophy: XMLGameEngine leans on a large built-in vocabulary |
| RBG (Regular Boardgames) | Rules as regular expressions, for finite deterministic turn-based games | Sibling for board games |
| Ludi / Ludii | "Ludemic" decomposition of game components | Sibling for board games |
| PuzzleScript (Lavelle) | Declarative find-and-replace rules over a grid; turn-based; rules are not limited to collisions | Closest spiritual cousin |
| Imperative engines (Unity, Unreal, GML) | Full control flow | Unlimited expressiveness, no generic reasoning about a game |
| Griddly (GDY, YAML) | C++-core rewrite of the GVGAI idea for speed | The successor that mostly replaced VGDL in AI research |

## Papers and code

Origin:

- Ebner, Levine, Lucas, Schaul, Thompson, Togelius. "Towards a Video Game Description Language." Dagstuhl Follow-Ups, 2013. [Dagstuhl](https://drops.dagstuhl.de/entities/document/10.4230/DFU.Vol6.12191.85)
- Schaul. "A Video Game Description Language for Model-based or Interactive Learning." IEEE CIG, 2013. [PDF](https://course.ccs.neu.edu/cs5150f13/readings/schaul_vgdl.pdf). Source: [schaul/py-vgdl](https://github.com/schaul/py-vgdl).

GVGAI competition era:

- Perez-Liebana et al. "General Video Game AI: a Multi-Track Framework..." IEEE Trans. on Games, 2019. [arXiv:1802.10363](https://arxiv.org/abs/1802.10363). The best single survey.
- Khalifa, Green, Perez-Liebana, Togelius. "General Video Game Rule Generation." [arXiv:1906.05160](https://arxiv.org/abs/1906.05160).
- Torrado et al. "Deep Reinforcement Learning for General Video Game AI." CIG 2018. Code: [GVGAI_GYM](https://github.com/rubenrtorrado/GVGAI_GYM).
- The physics and macro-actions extension, IEEE CIG 2017 ([IEEE Xplore](https://ieeexplore.ieee.org/abstract/document/8080443)), one of the few times the language itself was extended.
- Frameworks: [GAIGResearch/GVGAI](https://github.com/GAIGResearch/GVGAI) (Java), [gvgai.net](http://www.gvgai.net), Python 2.0 rewrite [rubenvereecken/py-vgdl](https://github.com/rubenvereecken/py-vgdl).

Applications and successors:

- Broll et al. "AtDelfi: Automatically Designing Legible, Full Instructions For Games." [arXiv:1807.04375](https://arxiv.org/abs/1807.04375).
- "Planning from video game descriptions." [arXiv:2109.00449](https://arxiv.org/abs/2109.00449).
- "Game Generation via Large Language Models." [arXiv:2404.08706](https://arxiv.org/abs/2404.08706) (an LLM writing VGDL directly).
- Bamford, Huang, Lucas. "Griddly: A platform for AI research in games." AAAI 2021. [arXiv:2011.06363](https://arxiv.org/abs/2011.06363); code [Bam4d/Griddly](https://github.com/Bam4d/Griddly); docs [griddly.readthedocs.io](https://griddly.readthedocs.io).
- PuzzleScript: [puzzlescript.net](https://www.puzzlescript.net/); the study of what gives it power, ScriptButler (CWI, 2023): [PDF](https://ir.cwi.nl/pub/32966/32966.pdf).
- Regular Boardgames: [AAAI paper](https://ojs.aaai.org/index.php/AAAI/article/view/3991).

## Why the field stayed small

- It was built for one job: an unlimited supply of benchmark games for AI agents. Success shows up as papers *using* GVGAI, not as papers extending VGDL.
- A closed vocabulary means a closed language: a genuinely new dynamic must be coded into the engine, not written in the description.
- Each generation is reimplemented under a new name (GDL to RBG and Ludii, VGDL to Griddly and MiniHack), so citations restart from zero and the field looks smaller than the idea.

## "No loops or conditionals": does it make sense?

The conditionals and loops are implicit, not absent: the engine's per-tick evaluation is the loop and pattern matching on collisions or grid state is the conditional. This is a production-rule or term-rewriting system, the same paradigm as cellular automata, Datalog and regex-based rule languages. The trade-off is explicit in the literature: expressiveness against compactness and generability. VGDL chose compactness so search and evolution can mutate thousands of games; a Turing-complete layer would kill that. It stops making sense where the sprite and interaction vocabulary runs out. XMLGameEngine makes the same bet, with exprtk carrying per-rule arithmetic rather than general control flow, and pays the same price ([01](01-vision-and-scope.md), [12](12-collision-escalation.md)).

## Sources

- "Video game description languages: overview and research" (2026-09-22).
