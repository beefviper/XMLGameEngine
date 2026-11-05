# Values, variables and names

**Status:** Built. A `Value` variant and anchors are ideas. Reference: [the game file](../reference/format.md#values).

## Values

- A **value** is any number in a game: text arithmetic (`+ - * /`, brackets, names, numbers; `load/calc`) or **one value tag**: `<formula>` ([arithmetic](03-arithmetic.md)) or `<random>`. A value holds text or a tag, not both.
- **`<random>`** is drawn at load and again whenever its object comes back into play (a reset, `<reveal>`, `<release>`, `<fire>`; `Game::drawStartAgain`), so every Pong serve is new. A random size with a random sign is `<sign>` of a random.
- **What is worked out when:** once at load, except a timer's interval (each round), positions that depend on a measured text size, a condition's number, and a command's values that name something that changes. An object's variables are worked out before its position and velocity, so those can use them.
- Ideas, each one tag and one case: `<pick>` from a list, an integer `<random>`, a `<count>` of a class.

## Values read when used

- "A variable should always be able to be changed": an `<inc>` amount was fixed at load, so a fish could not add its own worth.
- **Chosen: a command remembers what it was written as.** One whose values name a live number, and every `<set>` and `<spawn>`, is kept raw (`CmdLive`) and made again by `makeCommand` each time it runs, so no verb needs to know. Rejected: each verb evaluating its own fields (every verb twice), every command live (wasted work on every bounce).
- **Live numbers** (`x`, `y`, `velocity.x`, `heading`, `altitude`, `light`, `blink`, and `touch.*` in a rule) are bound into the arithmetic before a value is worked out. Only a command's value may read them; anywhere else would read 0, so the load stops.
- `self.` is written out to the object's own name before evaluation (`nameSelf`).
- Open: a group's live number by its name (which member?), an object's middle as a name of its own.

## Evaluation order and dotted names

- XML has no definition order or scope. **Chosen: collect, then evaluate:** register every symbol (globals, `window.*`, every `owner.variable`), then evaluate, so `score1` can read `paddle1.score` written later. Globals go in written order.
- `ball.radius` is a symbol with a dot in it. Owned objects are named by path, so a reference splits at its **last** dot. `layOutScenes` writes every name out in full first, inside out ([scenes](12-scenes.md)).
- Chains of references need no sort: only simple reads exist.

## Variables

- Global `<variable>`s are named constants. **Changing state lives on objects** (`paddle1.score`); a game-wide changing variable was never needed.
- **Polling by name:** each object has a map of numbers and a snapshot for reset; a text whose `<number>` is one `owner.variable` redraws when it changes.

| Option | Verdict |
|---|---|
| **Polling** | Chosen: simple, fits a declarative file |
| Listeners | Cleanup and wiring do not look declarative |
| Copying the value | Goes stale |
| Raw pointers | Dangle when storage moves |

- Idea: a small `Value` variant (int, float, bool, string); references as `{handle, variable}` resolved after the first pass.

## An object's own size

- Centring a text needs a width only a backend can measure; games used hand-tuned offsets that broke on any wording change.
- **Built:** `name.width` and `.height` in positions. A text's position is unknown until the window measures it, then worked out again whenever a size it names changes.
- **Wart:** an object's own variable `width` shadows its size (the variable wins, keeping old games working).

## Reference points and axes (idea)

- `window.width.center - name.width / 2` was written 330 times in 23 games, because positions name an object's top-left corner.
- The idea: let a game say **which point of an object its position names** (top-left, centre, any of nine), and perhaps where the origin is and which way y runs.
- Forms, none chosen: `anchor="center"` on a position (an attribute: it picks a point); one setting in `<window>`; a game-wide default a position can change. It must stay an element or attribute, never parsed from a string.
- Open: whether velocity follows a flipped y; what the generator writes.

## Names and classes

- A unique `name` and an optional `class`, borrowed from HTML `id` and `class`: one thing, or every thing of a kind. One condition on `class="paddle"` covers both paddles.
- Every group member or cell is its own object with its own name; the group's name means all of them.
- Ideas: compound selectors, class inheritance, wildcards over variables, several tags per object.
