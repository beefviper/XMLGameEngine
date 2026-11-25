# Written-out games

What the readable generators write for the games they cover, kept so the code can be read and built without running `--generate`. A folder per generator, then a folder per game, each exactly as `xgecli <game> --generate <generator>` writes it, but for the files' date line.

| Folder | Holds |
|---|---|
| `windows-cpp/<game>/` | a stand-alone SFML 3 program: `source/`, its `CMakeLists.txt`, `README.md`, `LICENSE`, and any font in `assets/` |
| `web-js/<game>/` | one page in plain JavaScript, `<game>.html`, with its `README.md` and `LICENSE` |

The games for now: `pong_min`, `pong`, `breakout`.

- **Change both together.** `test_generate.cpp` fails when a generator writes anything other than what is kept here, so a stylesheet change is copied in and shows in git as plain code.
- **Built and checked.** The main build makes each `windows-cpp` game a program (`pong_min`, `pong`, `breakout`); `xge/tests/written/<game>.h` says what it must do, run on the kept copy by `test_autogen_<game>.cpp` and on a fresh one by `test_written_games.cpp`.

To refresh a folder after a stylesheet change, from `games/`:

```
xgecli breakout --generate windows-cpp -o autogen/windows-cpp/breakout
```
