# Sources of the design notes

The first design write-ups (2026-09-29, 2026-09-30) were mined from the author's own exported chat history with `scan_export.py` (a Claude export of about 470 conversations) and `scan_chatgpt_export.py` (a ChatGPT export of about 3,400). Roughly 25 of the first and 110 of the second were read; about a dozen of the first held real design content. Only paraphrased ideas were kept. The JSON itself stays in the author's folder and is **not** copied into the repository; no conversation titles are listed here (see the privacy rule in [notes.md](notes.md)).

## What was and was not used

- Used: the long project discussions (the vision and the "every game is Pong" lineage, XML versus other formats, function-call syntax, variables and evaluation order, collisions, input, verbs and jump, backends and project layout, the C++ reference Pong, build setup), a research conversation on video game description languages, and ChatGPT chats on motion models, jumps, collisions, paths and formations, targets, scrolling, game mechanics and balance.
- Not used: build, linker, compiler and IDE chats beyond what [11](../designs/11-backends-build-and-layout.md) records; hardware, personal, professional and non-game chats that only matched a keyword; the personal parts of long chats; the export's `memories` and `projects` folders (the assistant's own starter documents).

## Caveats

- Exports flatten some assistant turns (web searches, tool calls and thinking are missing), so a few replies show no text.
- Design chats mix the author's statements with the assistant's suggestions; an idea that came only from the assistant is marked as a suggestion in the design file.
- Some chats describe the earlier prototype (2020 to 2021 headers), not the current rewrite.
- Everything written since 2026-10-01 comes from the author's requests in the project thread and from the code, not from exports.
