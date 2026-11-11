# Scenes

**Status:** Built. First drafted as "rooms" beside states, then reshaped as scenes inside a state. Reference: [states and scenes](../reference/states.md#scenes).

## What we looked for

Games made of several places, one screen each: Berserk's rooms, Pitfall's screens, Kaboom's waves, Adventure's castles. Before scenes each place was a `<state>`, so states did two jobs: **modes** (title, game, pause), which a stack suits, and **places**, which were states only because there was nowhere else to put them. That cost:

- **Copies.** Every object was global, so each room's walls and robots were written again under new names. About 600 of Berserk's 1,613 lines were second and later copies.
- **No pause over a place,** since being shown was what made an object move.
- **Nothing goes along:** a carried key could not follow the hero into the next room.
- **Nothing remembers:** coming back meant starting the room again.

It had to need no XSLT to load, and pack nothing into strings.

## What we looked at

| Option | Verdict |
|---|---|
| A template with parameters, expanded before load | Copies stay copies; needs XSLT or a second language |
| A state that copies another state | Shrinks states, not objects (the bulk) |
| Rooms beside states at the top level | Two parallel lists; whose room it is goes unsaid |
| **Scenes inside a state; states and scenes own objects** | Chosen |

`scene` is the general word for room, screen and wave, and it sharpens what a state is: a mode, which may hold scenes.

## Chosen

- **States and scenes both own objects.** A state's are in play in every scene of it; a scene's only while it is current. The game's own objects are the outermost owner.
- **A scene can copy another** (`from=`) and its `<variables>` are its parameters. Each copy is a real object made at load, so values see their own scene's numbers.
- **Going to a scene is a command,** `<enter scene>`, run where the move happens (an exit's rule, an edge rule, a condition). A verb rather than a map, since every game already has that place.
- **What is carried goes along; a scene left keeps its state.** `<reset scene>` starts one again; a new game starts every scene again.
- **Names are dotted paths** (`playing.room1.walls`). A plain name is looked for inside out: the scene's, the state's, the game's. A name that could be read two ways (shadowing) stops the load. Where a short name ends several paths, the shortest wins.
- **Being in play is split:** running (moves, times, collides), drawn (running, or under a state that shows it still), and seen (`<hidden />` runs unseen, so an exit zone is an ordinary object).
- **Information** between scenes goes through the state's own objects; between states, a look of the end state's object picked by the condition that pushes it.

## How it was built

- **One layout pass before evaluation** (`layOutScenes`, `game/scenes.cpp`) turns owned objects into ordinary objects with owners and full names. Everything after it, generated programs included, sees plain objects. Takes and `from=` are worked out on the XML first ([pieces](13-pieces.md)).
- **Ways, for exits written once.** A scene names its ways (`<way name="east" scene="screen2" />`) and an exit piece says `<enter way="east" />`, looked up in the current scene as it runs. Rejected: an edge map on the scene (fits Pitfall's sides, not Berserk's gaps), letting a take rewrite rules, a computed scene name (a string to parse).
- **`from=` copies everything** (objects, variables, ways, conditions, timers), read in the new scene. A `name=` on a condition or timer lets the copy replace it in place (`mergedByName`). Rejected: a scene's own list replacing the copied one whole (cannot keep one and change another). Atlantis's third wave is written this way.
- **What a condition counts:** a class, or a name every scene shares, counts what is in play; a path (`room2.robots`) counts that scene wherever the game is. Rejected: a `scene=` attribute (a second way to say the path), counting every scene for a class (breaks every wave game).
- `<random>` is drawn again when its object comes back into play, not on entering a scene, which finds the scene as it was left.

## Open

- Whether entering a scene fresh should draw its `<random>`s again.
- Maps of many screens: ways are enough for four, maybe not for Pitfall's 255.
- Who entered, and from which side.
- Taking a copied condition or timer away.
