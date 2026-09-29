# Sources: which conversations fed which design entry

The export from claude.ai (4 folders: conversations, light metadata, memories, projects) held 466 conversations. Roughly 25 mention the project by keyword, and about a dozen contain real design content. Conversation titles and dates are listed here; the JSON itself stays in the author's own folder and is **not** copied into the repository.

Index numbers refer to position in `conversations.json` at the time of the scan (2026-09-29) and are only useful with `scan_export.py`.

## Used

| Index | Title | Date | Feeds |
|---|---|---|---|
| 7 | Video game collection value in CAD (only the segment where the project is discussed) | 2026-05-23 | 01, 02, 03, 06, 10, 11, 13, 14 |
| 417 | Video game description languages: overview and research | 2026-09-22 | 01, 12, 13, 19 |
| 441 | XML variable references in game engine | 2026-09-25 | 03, 04, 05 |
| 415 | Pong game XML structure | 2026-09-22 | 14 (proposal only) |
| 135 | Stack vs heap allocation in C++ | 2026-08-07 | 05, 06, 07, 08 |
| 210 | Inheritance vs composition in C++ design | 2026-09-05 | 08 |
| 73 | C++ interface for multiple XML libraries | 2026-07-27 | 15 |
| 62 | Virtual destructor in abstract interface classes | 2026-07-21 | 15 |
| 56 | Loading and printing XML in C++ | 2026-07-19 | 15 |
| 61 | XML game engine project structure review | 2026-07-20 | 09, 15, 16 |
| 60 | Renaming game folder file prefixes | 2026-07-20 | 09, 16 |
| 434 | SFML Pong game displaying blank screen | 2026-09-24 | 17 |
| 422 | Building a Pong game with SFML 3 | 2026-09-23 | 17 |
| 45 | Refactoring SFML Pong game code | 2026-07-11 | 11, 17 |
| 46 | Fixing pong collision and ball sticking issues | 2026-07-13 | 11, 17 |
| 449, 452 | SFML 3 intersects migration; default object origin | 2026-09-26 | 17 |
| 1, 58, 55, 54, 447 | CMake conversations | 2026-02 to 2026-09 | 18 |
| 438 | Training a local LLM to play classic games | 2026-09-25 | 20 |
| 196 | Dynamic tree aging through scaling | 2026-08-20 | 20 |

## Read and deliberately not used

- Conversations that only matched the author's online handle or a keyword in a list of words (for example a word-list export, and aider/LM Studio tool setup). They contain nothing about the engine's design.
- Personal material in the long conversation of 2026-05-23 (everything outside the project discussion), and other personal chats. None of it belongs in a public repository, so none of it was carried over.
- The `memories` and `projects` folders of the export: the project files there are the assistant's own starter documents, and the memory export only restates what is in the conversations.

## Caveats about the record

- The export flattens some assistant turns: web searches, tool calls and thinking are missing, so a few replies show no text. The VGDL research conversation ends on a message about jump verbs with no reply.
- The design conversations are a mix of the author's statements and the assistant's suggestions. Where an idea came only from the assistant it is labeled a suggestion in the entry.
- Some conversations describe the earlier prototype (2020 to 2021 header dates), not the current rewrite. Entries say which is which.
