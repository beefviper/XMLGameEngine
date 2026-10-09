# Generating a program with XSLT

**Status:** Built: `--generate windows-cpp` covers every game. Details of the target: [structure](generator-structure.md), [rules](generator-rules.md).

## Approach

**What we looked for.** The format was chosen partly so a game could be transformed ([file-format](file-format.md)): a way to turn a game file into a program that plays it with none of the engine in it, as a test of whether the description really is the game, and as the start of other targets (another language, another machine).

**What we looked at.**

| Option | Notes |
|---|---|
| G1. C++ in xgecli walks the loaded `Game` and prints code | Reuses the loader, but the generator is engine code: every target is more C++ in the program |
| G2. XSLT on the game file | The format's own tool; a target is a folder of stylesheets, not a rebuild. Needs an XSLT processor |
| G3. A separate generator program | Cleaner split, one more program to build; can come later if xgecli outgrows it |

Processors: libxslt (XSLT 1.0 with EXSLT, C, in vcpkg and every Linux distribution), Xalan-C++ (XSLT 1.0, matches Xerces but barely maintained), Saxon (XSLT 3.0, Java or a C build with a different licence for the full version).

**Chosen: G2 with libxslt, run by `xgecli --generate <target>`.** A target is named platform-language (`windows-cpp`); the backends are assumed (SFML 3 for now; a later option can pick them, since naming every window, sound and XML combination in the target would multiply). `-g` stays the game; the long form only, plus `-o` for the folder. One stylesheet writes several files with `exsl:document` (XSLT 1.0 has one result; 2.0's `xsl:result-document` is the same idea): `main.cpp` is the real output, `CMakeLists.txt` and `README.md` are small fixed texts with the game's name in them. A stylesheet cannot copy a binary file, so its own result is a manifest of what it wrote and of the assets to copy, and xgecli copies them. Choices inside the stylesheets:
- *Static, not interpreted.* The program is the game's rules written out as statements (`deflect(o_ball, o_paddle1, edgeOfFirst, (45.0f));`), not the engine plus the XML as data. Collision pairs, which rules apply to which pair, edge rules and key bindings are worked out by the stylesheet.
- *The verbs' C++ was kept as text* in a `runtime.xml` that mirrored the engine's verbs, sweep and synthesizer, so generated Pong played like the engine's; that is what made the first target too big, and it is gone (below). The plain target keeps small helpers in `functions.xml` instead.
- *Expression text is tokenised in XSLT 1.0* (no regular expressions): names become C++ variables (`window.width.center` → `v_window_width_center`), everything else is copied. That is enough because the expression syntax is C-like; it is also the weakest part, and the reason the tags exist.
- *Arithmetic comes out as a person would write it.* An `<equation>` or `<formula>` becomes one infix expression (`v_window_width_center - v_title_width / 2.0f`), an equation's steps written in where they are used, with brackets only where C++'s precedence needs them (`a - (b - c)`, `(a + b) * c`), and plain expression text keeps no brackets when it is one name or number. The first version wrote an equation as a lambda of its steps and every divide as `xge::divide()`, faithful but unreadable. The cost: a divisor of 0 gives infinity, as the expression text does, not the 0 the engine's `<divide>` gives.
- *Refuse, never guess.* Every tag the target does not know stops it with `windows-cpp cannot generate <x> yet` and where; xgecli removes a folder it made. A program that silently lacks a rule would be worse than none.

**Rejected.** XSLT 3.0 (Saxon): text processing would be easier, but the C library is the one that fits a C++ program with vcpkg dependencies. The engine as a library inside the generated program: that is the engine, not a generated game. One file per object or state: the C++ is shorter as one file and nobody edits it.

### A plain target: windows-cpp (windows-cpp-full removed)

**What we looked for.** The first target worked, but generated Pong was about 1,700 lines (790 of copied engine: object type, swept collision, verbs, synthesizer; 800 of per-object code) where Pong by hand on SFML 3 is about 100. The author wanted the program to read as if written by hand, with nothing of the engine in it, laid out as: the header block, includes (local, third party, standard), global and tunable variables, the objects, forward declarations, `main`, then the definitions; and the stylesheet shaped the same way (`generate-main` calling `generate-window` and `generate-game-loop`, which calls `generate-events`, `generate-update` and `generate-render`).

**What we looked at.** (a) Trimming the first target: its size comes from being faithful to the engine (the sweep, every verb's edge cases), so trimming means changing what it is. (b) A new target that starts from the smallest game and grows, refusing everything else, beside the old one. (c) Replacing the old one outright: loses full Pong until the new one catches up.

**Chosen: (b), then (c).** `windows-cpp` is the new, plain target. The first one was kept as `windows-cpp-full` for a while, then removed at the author's call: it carried too much of the engine. Its choices above (static rules, tokenised text, readable arithmetic, refuse never guess) carry over; its copied runtime does not. `games/pong_min.xml` (two paddles and a ball, no menu, score, text or sound) is the game it was grown on, and comes out at about 230 lines; all of `games/pong.xml` comes out at about 480, plus the modules; Space Race and Freeway, mostly groups, at about 870 and 980; Breakout at about 500; Depth Charge at about 630; Frogger at about 1080; Kaboom, Demon Attack and Megamania at about 1060, 570 and 1240; Asteroids, Combat and Lunar Lander at about 1330, 790 and 520; Frostbite and Air-Sea Battle at about 1170 and 1090; Donkey Kong and Pitfall! at about 690 and 1560. Choices:

- *Simple collision, not the engine's.* Move, then test where it landed; a touch is told apart by the smaller overlap. The engine's sweep is what makes thin walls and fast bullets safe; pixel touches have it, box touches do not yet, as no generated game has needed it.

**Open.** A `<reveal>` draws a `<random>` start again where the engine does not. A rule about a class is one loop per group in it, so Space Race's rockets have nine; a person might keep the lanes in one vector, which would need the generator to see that the groups differ only in numbers. The generated rules are a second, simpler copy of the verbs and can drift from the engine; a test that plays a generated game against the engine frame by frame would catch it, but needs a compiler in the test. A pair is tested every frame, where the engine tests it only when one of the two moves: a rule about two things that both stand still runs in the program and not in the engine (Frostbite with its floes stopped). Built and played only on Linux with GCC (and checked with Clang); Windows and Visual Studio not tried. A sound that cannot be made (no audio device) is silent, as in the engine. Other targets (another backend, another language, an 8-bit machine) and how backends are named for them.
