# Values, variables and names

**Status:** Built. A `Value` variant and anchors are ideas. Reference: [the game file](../reference/format.md#values).

## Values

- A **value** is any number in a game: text arithmetic (`+ - * /`, brackets, names, numbers; `load/calc`) or **one value tag**: `<formula>` ([arithmetic](03-arithmetic.md)) or `<random>`. A value holds text or a tag, not both.
- **`<random>`** is drawn at load and again whenever its object comes back into play (a reset, `<reveal>`, a new copy; `Game::drawStartAgain`), so every Pong serve is new. A random size with a random sign is `<sign>` of a random.
- **What is worked out when:** once at load, except a timer's interval (each round), positions that depend on a measured text size, a condition's number, a command's values that name something that changes, and an object's numbers written `update="live"` (every frame, [below](#numbers-that-follow)). An object's variables are worked out before its position and velocity, so those can use them.
- Ideas, each one tag and one case: `<pick>` from a list, an integer `<random>`, a `<count>` of a class.

## Values read when used

- "A variable should always be able to be changed": an `<inc>` amount was fixed at load, so a fish could not add its own worth.
- **Chosen: a command remembers what it was written as.** One whose values name a live number, and every `<set>` and `<spawn>`, is kept raw (`CmdLive`) and made again by `makeCommand` each time it runs, so no verb needs to know. Rejected: each verb evaluating its own fields (every verb twice), every command live (wasted work on every bounce).
- **Live numbers** (`x`, `y`, `velocity.x`, `heading`, `altitude`, `light`, `blink`, and `touch.*` in a rule) are bound into the arithmetic before a value is worked out. Only a command's value may read them; anywhere else would read 0, so the load stops.
- `self.` is written out to the object's own name before evaluation (`nameSelf`), in its commands and in its own numbers (variables, position, heading).

## Numbers that follow

- Asked: can object variables reference each other live? Only a text's `<number>` was bound (redrawn on change); everything else was worked out at load. Games faked following with a timer `<every>0.01</every>` doing `<set>` (Demon Attack's and Megamania's steered shot, Yars' cannon) or the same formula copied into every action that changed its input (Cannon Hill's heading).
- **Both are needed.** Following: a steered shot, a turret part, a number made from others. Once: snapshots (Hold'em's `owed` from `bet`, Cannon Hill's `last` from `power`), starts that then change (Lunar Lander's fuel, lives), a value made from itself (`velocity.x * 1.25`).
- Rejected: "follow unless a command writes it" (adding one `<inc>` would quietly stop a link); a separate `<follows>` element (a second word for a variable). **Chosen (the author's):** an attribute where the number is made, `update="live"`, a pick of `once|live` (the format has no true/false attributes; keys' `on=` is the model). On an object's `<variable>`, `<position>` halves and `<heading>`.
- **How:** `RawValue::live`; the object keeps its `Follow`s (`Object::follows`, made in `makeObject`, so a spawned copy has its own); `Game::follow`, a `FrameRules` step after `holdCarried`, works each out with `evaluateNow`. At load such a value may read live numbers as 0 (`evaluateStart`) and is followed once when the game is made. No command may write one (`load/check`), so a snapshot cannot become a link by mistake. Polling every frame, not listening: a handful of values, each one compiled program.
- Open: a live velocity or size, a live game `<variable>` (they are constants in `load/calc`), a live text `<number>` naming more than one name, ordering live variables that read each other.
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
- **Built: `<centered>`** puts an object's middle at a place (empty: the window's middle), so a game never writes `name.width / 2`. Nine in ten of them said `window.height.center - 60`, so `<centered><offset>-60</offset></centered>` is that far from the window's middle, right or down, a negative number the other way: a child, never a sign in text (no arithmetic in strings). Rejected: an `offset=` attribute (a number is content, not a pick).
- **Window names:** `window.width` and `window.height` are names beside the edges `window.right` and `window.bottom` (the same numbers), so a size reads as a size; `window.width.center` and `window.height.center` stay as cheap constants.

## Reference points and axes (idea)

- `window.width.center - name.width / 2` was written 330 times in 23 games, because positions name an object's top-left corner; `<centered>` (above) took those, and what is left is cards laid out from the middle.
- The idea: let a game say **which point of an object its position names** (top-left, centre, any of nine), and perhaps where the origin is and which way y runs.
- Forms, none chosen: `anchor="center"` on a position (an attribute: it picks a point); one setting in `<window>`; a game-wide default a position can change. It must stay an element or attribute, never parsed from a string.
- Open: whether velocity follows a flipped y; what the generator writes.

## Names and classes

- A unique `name` and an optional `class`, borrowed from HTML `id` and `class`: one thing, or every thing of a kind. One condition on `class="paddle"` covers both paddles.
- Every group member or cell is its own object with its own name; the group's name means all of them.
- Ideas: compound selectors, class inheritance, wildcards over variables, several tags per object.
