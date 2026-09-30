# 01. Vision and scope

**Status:** implemented at a basic level (option C below)

## What it is

XMLGameEngine is a video game description language (VGDL) stored in XML, plus a C++ engine that runs it. The whole game (window, variables, objects, screens) lives in one XML file.

The stated target is to describe every 2D non-scrolling game of the late 1970s and early 1980s with a well-defined vocabulary of verbs, each with parameters, and later to describe 2D scrolling games with the same approach (jump is the example that shows how a verb grows parameters; see [13](13-verb-vocabulary.md)).

## Options considered

| Option | Idea | Trade-off |
|---|---|---|
| A. Purely declarative | No loops and no conditionals anywhere; behavior comes only from a fixed vocabulary | Cleanest, easiest to validate and generate from; cannot express a state change such as "game over at 15 points" |
| B. Embedded scripting | Let the file contain expressions or code | Very expressive; the file becomes a scripting language wrapped in XML |
| C. Declarative plus triggers | Declarative, with a named `condition` element for state changes | Keeps the spirit of A while covering the cases that cannot be avoided |

## Decision

**C.** The engine is declarative. "Loops" are the engine's own per-frame update; "conditionals" are implicit in collision detection and in `<condition>` elements. When the game needed to end at 15 points, the conclusion was that some kind of conditional is unavoidable, and that it could be called a trigger instead of an `if`. See [14](14-conditions-and-win-conditions.md).

The expression syntax was a case of option B creeping in through convenience (adding functions through exprtk was so easy that the vocabulary outgrew the design). That is the subject of [03](03-expression-syntax.md).

## The lineage argument

The design rests on a claim the author has long made half as a joke: every video game is a variation of Pong.

- **Pong:** two paddles and a ball. The ball bounces off the paddles and the top and bottom edges, and scores for a player if it touches the other player's side.
- **Breakout:** turn the screen 90 degrees, duplicate one paddle into a static grid, make blocks disappear when touched, and lose the ball at the bottom.
- **Space Invaders:** turn the bricks into aliens that shift sideways and step down when one hits a screen edge, and change the ball from bouncing to firing straight ahead, so both bullet and alien vanish on contact.
- **Galaxian:** keep the Invaders layout, but have the aliens fly in from the side in patterns, with one or two swooping at the player.

Small tweaks, repeated, recreate the vocabulary that game designers found in the 1970s. The engine tries to write that vocabulary down explicitly and declaratively.

A second analogy: vocabulary should grow the way number systems did (natural numbers, then zero, then negatives, rationals, irrationals, imaginary numbers, calculus), each addition a small extension made only because something real could not otherwise be said. The jump verb exists because platformers need it; `condition` exists because screens need to change state. Nothing is added speculatively.

## How the project started

It began as Pong written entirely in C++. Pieces were abstracted into an XML file one at a time until the whole game was in XML. That first version (Xerces, exprtk, SFML, hard-wired together) is the prototype; the current code is a ground-up rewrite that keeps the XML design and replaces the C++. The prototype's collision code was, by the author's own account, poor: detection and response were tangled together, and actions were parsed by walking tokens by hand. That is what [11](11-collision-detection-and-response.md) fixed.

## Open questions

- Where the vocabulary boundary sits: is scrolling a property of a camera or world, or a verb applied to objects? Is gravity a global parameter of the world, or a force applied to specific objects? ([13](13-verb-vocabulary.md))
- How far a closed vocabulary can go before games need engine changes, the ceiling every language in this family hits ([19](19-vgdl-landscape.md)).

## Second batch: alternatives

A later set of conversations restated the vision from a few new angles.

- **The pull toward a programming language.** A declarative format that needs real behavior tends to reintroduce a general expression language and slowly become a programming language again. The defense discussed: keep a *verb* language (choose from known behaviors, give them parameters) rather than an *expression* language (compute anything). Arithmetic for positioning across window sizes was judged acceptable; function-call strings were the thing to remove ([03](03-expression-syntax.md)).
- **A theory of games, not a file format.** Designing this is closer to defining what a collision or an interaction *is* than to serializing data. Each exception becomes a first-class concept, and the difficulty grows with the number of *contexts* a mechanic can appear in (elastic or not, solid or trigger, one-way or not), not with the number of mechanics. The primitives themselves (shapes, motion kinds, forces, interactions, states) are finite.
- **Recognition over completeness.** Shared words such as bounce and die let a reader recognize familiar behavior without knowing the machinery. The goal is a vocabulary that a person with no programming background can read; tags read as nouns, attributes as descriptions, nesting as containment.
- **Mechanism families.** Finiteness comes from a small number of families with parameters, not from one verb per game style ([13](13-verb-vocabulary.md), [23](23-jump-and-air-control.md)).
- **A research angle.** The interesting question a description language raises is how small a vocabulary can be while still describing a class of games; related to entity-component systems in structure ([19](19-vgdl-landscape.md), [27](27-object-composition-and-shape.md)).

## Sources

- "Engine design conversation A (a long general chat; only the project segment was used)" (2026-05-23), the section where the project is first described and the lineage argument is told.
- "Video game description languages: overview and research" (2026-09-22).
- Second batch (ChatGPT export): "VGDL Design Challenges" (2026-01-28), "VGDLs and Technology" (2026-05-09), "Declarative Pong XML redesign" (2026-09-23).
