#!/usr/bin/env python3
"""Scan a claude.ai data export for conversations about this project.

Usage:
    python3 scan_export.py EXPORT_DIR [--top N] [--dump IDX,IDX,... --out DIR]

EXPORT_DIR is the folder that contains conversations-000/conversations.json.
Without --dump it prints a ranked list of conversations by keyword hits.
With --dump it writes one plain-text file per listed conversation index
(code blocks kept, tool output kept) so they can be read or grepped.

Ranking is only a hint. Strong keywords also match unrelated chats that merely
mention the project or the author's handle, so read the titles before trusting
a score. See sources.md for what was used and what was deliberately skipped.
"""
import argparse
import json
import os
import re

STRONG = re.compile(
    r"xmlgameengine|xml game engine|vgdl|xsd|xerces|exprtk|tinyxml|pugixml|"
    r"rapidxml|game description language",
    re.I,
)
WEAK = re.compile(
    r"\b(pong|breakout|space ?invaders|collision|sfml|raylib|sdl2|game engine|"
    r"sprite|state stack|game loop)\b",
    re.I,
)


def message_text(message):
    text = message.get("text") or ""
    for attachment in message.get("attachments") or []:
        text += "\n[ATTACH %s]\n%s" % (
            attachment.get("file_name"),
            attachment.get("extracted_content") or "",
        )
    return text


def load(export_dir):
    path = os.path.join(export_dir, "conversations-000", "conversations.json")
    with open(path, encoding="utf-8") as handle:
        return json.load(handle)


def rank(conversations, top):
    rows = []
    for index, convo in enumerate(conversations):
        full = " ".join(message_text(m) for m in convo["chat_messages"])
        rows.append(
            (
                len(STRONG.findall(full)),
                len(WEAK.findall(full)),
                index,
                convo["name"],
                convo["created_at"][:10],
                len(convo["chat_messages"]),
            )
        )
    rows.sort(reverse=True)
    print("strong  weak  index  date        msgs  title")
    for strong, weak, index, name, date, count in rows[:top]:
        print("%6d %5d %6d  %s  %4d  %s" % (strong, weak, index, date, count, name))


def dump(conversations, indexes, out_dir):
    os.makedirs(out_dir, exist_ok=True)
    for index in indexes:
        convo = conversations[index]
        lines = ["# [%d] %s (%s)" % (index, convo["name"], convo["created_at"][:10])]
        for message in convo["chat_messages"]:
            lines.append("\n## %s\n%s" % (message["sender"].upper(), message_text(message)))
        target = os.path.join(out_dir, "%d.txt" % index)
        with open(target, "w", encoding="utf-8") as handle:
            handle.write("\n".join(lines) + "\n")
        print("wrote", target)


def main():
    parser = argparse.ArgumentParser(description=__doc__.split("\n")[0])
    parser.add_argument("export_dir")
    parser.add_argument("--top", type=int, default=60)
    parser.add_argument("--dump", help="comma-separated conversation indexes")
    parser.add_argument("--out", default="dump")
    args = parser.parse_args()
    conversations = load(args.export_dir)
    if args.dump:
        dump(conversations, [int(i) for i in args.dump.split(",")], args.out)
    else:
        rank(conversations, args.top)


if __name__ == "__main__":
    main()
