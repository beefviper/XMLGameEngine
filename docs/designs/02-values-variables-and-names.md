# 02. Values, variables and names

**Status:** built. Handle references and a `Value` variant are decided but not built ([03](03-objects-groups-and-storage.md)).

## Values

- A **value** is any number in a game file (position, velocity, size, threshold, step, interval). It is **expression text** (exprtk arithmetic over named values) or **one value tag**.
- Built value tags: `<equation>` and `<formula>`, arithmetic as tags over four operations, so no string is left to parse ([01](01-vision-and-format.md#arithmetic-in-text-and-as-tags)); and `<random min max />`, drawn at load and again when its object is reset (`Game::drawStartAgain`: the object's random variables, then its velocity and position worked out again), so every Pong serve is new. Ideas, each one new tag in the XSD and one case in the evaluator: `<pick>` (one of a list), an integer `<random>`, `<clamp>`, `<count>` of a class,; a random magnitude with a random sign is written with `sgn()` of a random (Pong: `sgn(ball.side) * ball.speed * cos(ball.angle * pi / 180)`, so the serve is never near-vertical or slow). Other balance helpers (saturate, floor, cap, diminishing returns, multiply by a comparison) are candidates too ([13](13-ideas.md)).
- A value holds text or a tag, not both.
- **What is worked out when.** Everything is evaluated once at load, except a timer's `<every>`/`<after>` (worked out again each round) and positions that depend on a text's or image's measured size. A condition's `<atleast>`/`<atmost>`/`<remaining>` and an `<inc>`/`<dec>` amount are load-time constants, so they cannot follow a variable (Berserk's "extra man every 2000" needed a variable of its own; [10](10-games-as-tests.md)). An object's variables are worked out before its position and velocity, so those can use them; `pi` is a constant.

## Evaluation order and dotted names

- Problem: XML has no definition order or scope, so `<x>screenWidth / 2 - ball.radius</x>` says nothing about whether `screenWidth` exists yet.
- **Chosen: collect, then evaluate.** Parse the whole tree as raw strings; register every symbol (globals, `window.*`, every object's variables as `owner.variable`, initially 0); evaluate. So `score1` can read `paddle1.score` even though `paddle1` comes later in the file. Globals are worked out in written order.
- Exprtk has no member access, so `ball.radius` is just a symbol whose name contains a dot, registered up front.
- Chained references (A reads B's variable that is itself an expression): a topological sort with cycle detection, or lazy evaluation with caching and a loud cycle error. Lazy was suggested; not needed, since only simple cross-object reads exist.
- Expressions read object variables live (`Game::refreshBoundTexts` also writes `expr.objectVariables`), which timers needed.

## Variables

- Global `<variable>`s are named constants. Mutable state lives on **objects** (`paddle1.score`); a variable with no owner is "no such object" and does nothing. A mutable game-wide variable was never needed: an object's own variables were enough in every shipped game.
- **Chosen: polling by named reference.** Each object has `map<string, float>`, plus a snapshot used by `reset`. A text whose `<number>` is exactly one `owner.variable` is bound to it and redraws when `<inc>`/`<dec>`/`setVariable` changes it. Any other `<number>` expression is worked out once.

| Option | Verdict |
|---|---|
| **Polling** (reader looks the value up) | Chosen: simple, fits a declarative file |
| Listeners / observers | Better when many things react to rare changes; stale subscriber cleanup and "subscribe" wiring do not look declarative. A second batch of conversations argued for observers throughout, for swappable parts; middle path: references for simple local dependencies, observers where decoupling pays |
| Copy the value | Goes stale. Rejected |
| Raw pointers | Dangle when storage moves. Rejected in favor of handles |

- Decided, not built: a small `Value` variant (`int`, `float`, `bool`, `string`) so "score" and "doubled" are game-author data; a reference form `{handle, variable}` resolved after the first pass (a dangling one returns a default and can be reported); per-entity variables bound into exprtk before a condition runs. Profile before optimizing the string lookups.

## An object's own size in expressions

- Need: centering a text needs a width only a backend can measure. Games used hand-tuned offsets (`window.width.center - 350`) that broke on any wording or font change.
- **Built:** `objectName.width` / `.height` in expressions (meant for `<position>`); an object names itself like any other (an earlier `self.width` was dropped). A circle/rectangle/bitmap size is known at load; a text/image position is marked unknown (`printGame` says so) until the window measures it, then recomputed whenever a size it names changes (`Game::resolveSizeDependentPositions`). Other expressions see those sizes as 0 until measured.
- **Wart:** an object's own `<variable name="width">` shadows its size. Precedence (variable wins) keeps old games working. Alternatives, none built: a reserved spelling (`title.size.width`, `title.@width`), a separate namespace, rejecting the clash at load, a load warning.
- Alternatives to arithmetic for placing by size, none built: an alignment attribute (`align="center"`), an origin at the center, a per-axis anchor.

## Names and classes

- Objects have a unique `name` and an optional `class`, borrowed from HTML `id` and `class`: target one thing or all things of a kind. One condition on `class="paddle"` covers both paddles.
- Used by `<condition class object>` and `<collision class object>` (both combine with AND; a `<collision>` with neither is "anything"); `class="projectile"` starts an object invisible.
- Every `<group>` member or cell is its own object with a unique name (`logrow3.2`, `aliens.3.2`); the group name still means all of them ([03](03-objects-groups-and-storage.md)).
- Ideas, not built: compound CSS-style selectors (a two-player win vs either-player win), class inheritance (`platform` extends `solid`), wildcard queries over variables (`player1.*`, a map scan), several tags per object, formation names (`group1[x]`).
