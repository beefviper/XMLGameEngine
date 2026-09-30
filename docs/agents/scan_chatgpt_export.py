#!/usr/bin/env python3
"""Scan a ChatGPT data export for conversations about this project or game design.

Usage:
    python3 scan_chatgpt_export.py EXPORT_DIR [--top N] [--min SCORE]
    python3 scan_chatgpt_export.py EXPORT_DIR --dump IDX,IDX,... --out DIR

EXPORT_DIR holds conversations-000.json, conversations-001.json, ... Each file is
a list of conversations; each conversation is a tree of messages (`mapping`).
The script follows the branch that ends at `current_node`, which is the visible
thread, and flattens it to plain text.

Index numbers are "file-number:position", for example 7:42, and are only stable
for one export. Without --dump the script prints a ranked list. With --dump it
writes one text file per listed index so they can be read or grepped.

Ranking is only a hint. Read the titles before trusting a score: a strong
keyword can appear in an unrelated chat, and design talk often uses none.
See sources.md for what was used and what was deliberately skipped.
"""
import argparse
import glob
import json
import os
import re

STRONG = re.compile(
    r"xmlgameengine|xml game engine|\bxge\b|vgdl|game description language|"
    r"video game description|xsd|xerces|exprtk|tinyxml|pugixml|rapidxml",
    re.I,
)
DESIGN = re.compile(
    r"\b(game ?design|level ?design|game ?engine|game ?loop|game ?state|"
    r"state (?:stack|machine)|sprite|tile ?map|hit ?box|collision|physics|"
    r"platformer|roguelike|side.?scroller|shoot.?em.?up|metroidvania|"
    r"pong|breakout|space ?invaders|frogger|pac.?man|tetris|asteroids|"
    r"donkey kong|mario|zelda|galaga|arcade|game ?mechanics?|"
    r"power.?ups?|hud|npc|enemy ai|pathfinding|procedural|entity.component|"
    r"ecs|scene graph|sfml|raylib|sdl2|xml game|declarative|"
    r"rule ?set|win condition|lose condition|game rules?|board game|"
    r"turn.based|card game|game jam|retro game|8.bit|nes|snes|atari)\b",
    re.I,
)


def thread(convo):
    """Messages on the visible branch, oldest first, as (role, text)."""
    mapping = convo.get("mapping") or {}
    node = convo.get("current_node")
    chain = []
    while node:
        entry = mapping.get(node)
        if not entry:
            break
        chain.append(entry)
        node = entry.get("parent")
    out = []
    for entry in reversed(chain):
        message = entry.get("message")
        if not message:
            continue
        role = (message.get("author") or {}).get("role")
        if role not in ("user", "assistant"):
            continue
        parts = (message.get("content") or {}).get("parts") or []
        text = "\n".join(p for p in parts if isinstance(p, str)).strip()
        if text:
            out.append((role, text))
    return out


def load_all(export_dir):
    for path in sorted(glob.glob(os.path.join(export_dir, "conversations-*.json"))):
        number = int(re.search(r"-(\d+)\.json$", path).group(1))
        with open(path, encoding="utf-8") as handle:
            for position, convo in enumerate(json.load(handle)):
                yield "%d:%d" % (number, position), convo


def rank(export_dir, top, minimum):
    rows = []
    for key, convo in load_all(export_dir):
        messages = thread(convo)
        user = " ".join(t for r, t in messages if r == "user")
        full = " ".join(t for r, t in messages)
        strong = len(STRONG.findall(full))
        design = len(DESIGN.findall(user))
        design_all = len(DESIGN.findall(full))
        score = strong * 5 + design * 3 + design_all
        if score < minimum:
            continue
        stamp = convo.get("create_time")
        date = "" if not stamp else __import__("datetime").datetime.utcfromtimestamp(stamp).strftime("%Y-%m-%d")
        rows.append((score, strong, design, design_all, key, date, len(messages), convo.get("title") or ""))
    rows.sort(reverse=True)
    print("score strong userD allD key      date        msgs title")
    for score, strong, design, design_all, key, date, count, title in rows[:top]:
        print("%5d %6d %5d %4d %-8s %s %5d  %s" % (score, strong, design, design_all, key, date, count, title))
    print("\n%d conversations at or above the minimum score" % len(rows))


def dump(export_dir, keys, out_dir):
    wanted = set(keys)
    os.makedirs(out_dir, exist_ok=True)
    for key, convo in load_all(export_dir):
        if key not in wanted:
            continue
        lines = ["# [%s] %s" % (key, convo.get("title") or "")]
        for role, text in thread(convo):
            lines.append("\n## %s\n%s" % (role.upper(), text))
        target = os.path.join(out_dir, key.replace(":", "_") + ".txt")
        with open(target, "w", encoding="utf-8") as handle:
            handle.write("\n".join(lines) + "\n")
        print("wrote", target)


def main():
    parser = argparse.ArgumentParser(description=__doc__.split("\n")[0])
    parser.add_argument("export_dir")
    parser.add_argument("--top", type=int, default=80)
    parser.add_argument("--min", type=int, default=6)
    parser.add_argument("--dump", help="comma-separated keys such as 7:42,12:3")
    parser.add_argument("--out", default="dump")
    args = parser.parse_args()
    if args.dump:
        dump(args.export_dir, args.dump.split(","), args.out)
    else:
        rank(args.export_dir, args.top, args.min)


if __name__ == "__main__":
    main()
