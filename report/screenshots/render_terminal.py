#!/usr/bin/env python3
"""Render a captured terminal session as a PNG screenshot.

Usage: python3 render_terminal.py <input.txt> <output.png> [title]

Produces a dark-terminal-themed image with monospace text. Used to
generate report screenshots from kshell session captures.
"""
import sys
from pathlib import Path
from PIL import Image, ImageDraw, ImageFont


BG       = (24, 26, 33)
FG       = (220, 222, 230)
PROMPT   = (130, 200, 255)
ACCENT   = (170, 220, 130)
TITLE_BG = (40, 44, 52)
TITLE_FG = (200, 200, 210)
PADDING_X = 24
PADDING_Y = 18
LINE_HEIGHT = 22
TITLE_HEIGHT = 36
FONT_SIZE = 15
MAX_LINES = 60


def find_mono_font(size: int) -> ImageFont.FreeTypeFont:
    candidates = [
        "/System/Library/Fonts/Menlo.ttc",
        "/System/Library/Fonts/Monaco.dfont",
        "/Library/Fonts/SF Mono.ttf",
        "/System/Library/Fonts/Courier.dfont",
    ]
    for path in candidates:
        if Path(path).exists():
            try:
                return ImageFont.truetype(path, size)
            except Exception:
                continue
    return ImageFont.load_default()


def colorize(line: str):
    if "kshell --inspect" in line and "-----" in line:
        return ACCENT
    if line.lstrip().startswith("kshell:"):
        return PROMPT
    return FG


def render(input_path: str, output_path: str, title: str = "kshell — Terminal"):
    text = Path(input_path).read_text()
    raw_lines = text.splitlines()
    if len(raw_lines) > MAX_LINES:
        raw_lines = raw_lines[:MAX_LINES]

    font = find_mono_font(FONT_SIZE)
    title_font = find_mono_font(FONT_SIZE - 1)

    width  = 980
    height = TITLE_HEIGHT + PADDING_Y * 2 + LINE_HEIGHT * len(raw_lines)
    img = Image.new("RGB", (width, height), BG)
    draw = ImageDraw.Draw(img)

    draw.rectangle([(0, 0), (width, TITLE_HEIGHT)], fill=TITLE_BG)
    for i, color in enumerate([(255, 95, 86), (255, 189, 46), (39, 201, 63)]):
        cx = 16 + i * 22
        cy = TITLE_HEIGHT // 2
        draw.ellipse([(cx - 7, cy - 7), (cx + 7, cy + 7)], fill=color)
    draw.text((width // 2 - 80, 8), title, font=title_font, fill=TITLE_FG)

    y = TITLE_HEIGHT + PADDING_Y
    for line in raw_lines:
        clean = line.replace("\t", "    ")
        draw.text((PADDING_X, y), clean, font=font, fill=colorize(clean))
        y += LINE_HEIGHT

    img.save(output_path)
    print(f"wrote {output_path}  ({width}x{height})")


if __name__ == "__main__":
    if len(sys.argv) < 3:
        print(__doc__)
        sys.exit(1)
    title = sys.argv[3] if len(sys.argv) > 3 else "kshell — Terminal"
    render(sys.argv[1], sys.argv[2], title)
