# Sources: which conversations fed which design entry

The export from claude.ai (4 folders: conversations, light metadata, memories, projects) held 466 conversations. Roughly 25 mention the project by keyword, and about a dozen contain real design content. Conversation titles and dates are listed here; the JSON itself stays in the author's own folder and is **not** copied into the repository.

Index numbers refer to position in `conversations.json` at the time of the scan (2026-09-29) and are only useful with `scan_export.py`.

## Used

| Index | Title | Date | Feeds |
|---|---|---|---|
| 7 | Engine design conversation A (a long general chat; only the project segment was used) | 2026-05-23 | 01, 02, 03, 06, 10, 11, 13, 14 |
| 417 | Video game description languages: overview and research | 2026-09-22 | 01, 12, 13, 19 |
| 441 | XML variable references in game engine | 2026-09-25 | 03, 04, 05 |
| 415 | Pong game XML structure | 2026-09-22 | 14 (proposal only) |
| 135 | C++ design conversation A | 2026-08-07 | 05, 06, 07, 08 |
| 210 | C++ design conversation B | 2026-09-05 | 08 |
| 73 | C++ interface conversation A | 2026-07-27 | 15 |
| 62 | C++ interface conversation B | 2026-07-21 | 15 |
| 56 | C++ interface conversation C | 2026-07-19 | 15 |
| 61 | XML game engine project structure review | 2026-07-20 | 09, 15, 16 |
| 60 | Project layout conversation B | 2026-07-20 | 09, 16 |
| 434 | SFML Pong game displaying blank screen | 2026-09-24 | 17 |
| 422 | Building a Pong game with SFML 3 | 2026-09-23 | 17 |
| 45 | Refactoring SFML Pong game code | 2026-07-11 | 11, 17 |
| 46 | Fixing pong collision and ball sticking issues | 2026-07-13 | 11, 17 |
| 449, 452 | SFML 3 intersects migration; default object origin | 2026-09-26 | 17 |
| 1, 58, 55, 54, 447 | Build setup conversations | 2026-02 to 2026-09 | 18 |
| 438 | Training a local LLM to play classic games | 2026-09-25 | 20 |
| 196 | Simulation idea conversation | 2026-08-20 | 20 |

## Read and deliberately not used

- Conversations that only matched the author's online handle or a keyword in a list of words (for example a word-list export, and aider/LM Studio tool setup). They contain nothing about the engine's design.
- Personal material in the long conversation of 2026-05-23 (everything outside the project discussion), and other personal chats. None of it belongs in a public repository, so none of it was carried over.
- The `memories` and `projects` folders of the export: the project files there are the assistant's own starter documents, and the memory export only restates what is in the conversations.

## Caveats about the record

- The export flattens some assistant turns: web searches, tool calls and thinking are missing, so a few replies show no text. The VGDL research conversation ends on a message about jump verbs with no reply.
- The design conversations are a mix of the author's statements and the assistant's suggestions. Where an idea came only from the assistant it is labeled a suggestion in the entry.
- Some conversations describe the earlier prototype (2020 to 2021 header dates), not the current rewrite. Entries say which is which.

## Second batch: ChatGPT export

A second export (about 3,400 conversations across many files, each a list of conversations with a tree of messages) was scanned with `scan_chatgpt_export.py`. The scan ranked conversations by keyword score, a title sweep added others, and roughly 110 were read; the rest were build, hardware, or unrelated chatter. Only game-related and design-related content was carried over, as paraphrased ideas. Keys are `file:position` in that export, useful only with the script. Titles that did not begin as game or engine discussions are replaced by generic ones.

| Key | Title | Date | Feeds |
|---|---|---|---|
| 33:50 | Jump Behavior Models | 2026-09-23 | 13, 22, 23, 26 |
| 22:19 | Pong vs Donkey Kong Physics | 2026-01-29 | 11, 12, 22, 28 |
| 22:2 | Pong Mechanics Vocabulary | 2026-01-28 | 02 |
| 22:4 | VGDL Design Challenges | 2026-01-28 | 01, 12, 13, 27 |
| 28:37 | VGDLs and Technology | 2026-05-09 | 01, 13, 19, 23 |
| 33:51 | Declarative Pong XML redesign | 2026-09-23 | 01, 03, 06, 08, 19, 27 |
| 32:54 | Collision definition suggestions | 2026-07-23 | 11 |
| 32:22 | Collision detection techniques | 2026-07-19 | 11, 23 |
| 7:5 | Deferred Collision Handling | 2025-04-22 | 11 |
| 1:85 | Vertical Collision Calculation | 2024-12-24 | 11 |
| 21:30 | Game Engine Command Queue | 2026-01-17 | 03, 10 |
| 22:21 | Observer Pattern vs Reference | 2026-01-29 | 07 |
| 22:11 | Engine and Game Separation | 2026-01-29 | 15 |
| 32:28 | Engine Abstraction Design | 2026-07-20 | 15, 25 |
| 32:34, 32:36 | XML architecture and object pipeline reviews | 2026-07-21 | 15, 16, 18 |
| 32:45 | Name Attribute in XML | 2026-07-22 | 02, 06 |
| 32:47 | Chibi Akumas vs XMLGameEngine | 2026-07-22 | 25 |
| 32:44 | Movement as Puzzle Design | 2026-07-22 | 22 |
| 32:4 | Asteroids Shape Variations | 2026-07-17 | 27 |
| 9:65 | Enemy Path Creation Methods | 2025-06-12 | 24 |
| 31:70 | Pong with Third Paddle | 2026-07-12 | 10, 28 |
| 14:7 | Data-driven tables versus code | 2025-09-04 | 16, 20, 25 |
| 5:44, 12:91 | NES sprite techniques | 2025-03-30, 2025-08-18 | 25, 26 |
| 9:18 | Sonic vs Mario Debate | 2025-06-01 | 26 |
| 3:99 | Power-Up Mockery Concept | 2025-02-27 | 14, 28 |
| 8:89 | Unwinnable Game States | 2025-05-23 | 14, 28 |
| 16:86, 31:59 | Combat and RPG resolution | 2025-10-25, 2026-07-10 | 28 |
| 31:41 | Game Mechanics Tricks | 2026-07-06 | 03, 28 |
| 31:11, 25:9 | Genre and simulator chats | 2026-06-27, 2026-03-14 | 28 |
| 18:71 | Game controls and sluggishness | 2025-11-23 | 10 |
| 7:19, 3:37, 0:7 | VGDL overview and paper lists | 2025-02 to 2025-04 | 19 |
| 11:73 | General chat A (engine segment only) | 2025-07-25 | 01, 19, 26 |
| 7:2, 7:13 | General chats B and C (engine segments only) | 2025-04 | 01 |

### Second batch: read and deliberately not used

- Build, linker, compiler-flag and IDE chats (many are on CMake, Xerces and SFML setup, which the first batch already covered in [18](../designs/18-build-system.md)).
- Hardware, personal, professional, mobile-game progress and strategy chats, and non-game topics that only matched a keyword. None of it is design material.
- Long conversations that started off-topic; only the segments about the engine or game design were read.
