#!/usr/bin/env python3
"""Load/save puzzle levels (JSON) and regenerate levels.h."""
import json
import os
import re
from dataclasses import dataclass, field
from typing import Optional

MAP_H = 10
MAP_W = 15

BG_TILES = [
    (0, "FLOOR"),
    (1, "WALL"),
    (2, "GOAL"),
    (3, "SWITCH"),
    (4, "ICE"),
    (5, "CRACKED_1"),
    (6, "CRACKED_2"),
    (7, "HOLE"),
    (8, "EXIT"),
    (9, "ARROW_UP"),
    (10, "ARROW_DOWN"),
    (11, "ARROW_LEFT"),
    (12, "ARROW_RIGHT"),
    (13, "TOGGLE_SWITCH"),
    (14, "WARP_RED_A"),
    (15, "WARP_RED_B"),
    (16, "WARP_BLUE_A"),
    (17, "WARP_BLUE_B"),
]

FG_OBJS = [
    (0, "NONE"),
    (1, "PLAYER"),
    (2, "BARREL"),
    (3, "SHADY"),
]

# エディタ表示用（日本語）
BG_TILES_JA = {
    0: "床",
    1: "壁",
    2: "ゴール扉",
    3: "スイッチ",
    4: "氷",
    5: "ヒビ床1",
    6: "ヒビ床2",
    7: "穴",
    8: "出口",
    9: "矢印↑",
    10: "矢印↓",
    11: "矢印←",
    12: "矢印→",
    13: "回転SW",
    14: "赤WA",
    15: "赤WB",
    16: "青WA",
    17: "青WB",
}

FG_OBJS_JA = {
    0: "なし",
    1: "プレイヤー",
    2: "樽",
    3: "シェイディ",
}

BG_NAMES = {v: n for v, n in BG_TILES}
FG_NAMES = {v: n for v, n in FG_OBJS}

# グリッドプレビュー用 BMP（ゲームと同じ素材）
PUZZLE_MAP_BMP_REL = os.path.join("Asset", "graphics", "stills", "still_puzzle_map.bmp")


def puzzles_json_path(project_root: str) -> str:
    return os.path.join(project_root, "Asset", "fixdata", "puzzles", "levels.json")


def levels_h_path(project_root: str) -> str:
    return os.path.join(project_root, "src", "game", "src", "game", "levels.h")


def puzzle_map_bmp_path(project_root: str) -> str:
    return os.path.join(project_root, "Asset", "graphics", "stills", "still_puzzle_map.bmp")


@dataclass
class LevelEntry:
    title: str = "Untitled"
    bg: list[list[int]] = field(default_factory=lambda: _empty_bg())
    fg: list[list[int]] = field(default_factory=lambda: _empty_fg())
    player_x: int = 1
    player_y: int = 1
    shady_x: int = -1
    shady_y: int = -1


def _empty_bg() -> list[list[int]]:
    grid = [[0] * MAP_W for _ in range(MAP_H)]
    for y in range(MAP_H):
        for x in range(MAP_W):
            if x == 0 or x == MAP_W - 1 or y == 0 or y == MAP_H - 1:
                grid[y][x] = 1
    return grid


def _empty_fg() -> list[list[int]]:
    return [[0] * MAP_W for _ in range(MAP_H)]


def _parse_row(line: str) -> list[int]:
    m = re.search(r"\{([^}]*)\}", line)
    if not m:
        return []
    inner = m.group(1).split("//")[0]
    vals = [int(x.strip()) for x in inner.split(",") if x.strip()]
    return vals


def _parse_int_array(line: str) -> list[int]:
    m = re.search(r"\{([^}]*)\}", line)
    if not m:
        return []
    return [int(x.strip()) for x in m.group(1).split(",") if x.strip()]


def _split_level_blocks(section: str) -> list[str]:
    parts = re.split(r"\n\s*// --- Level \d+.*?---\s*\n", section, flags=re.DOTALL)
    blocks = [p for p in parts if re.search(r"\{\s*\d", p)]
    return blocks


def _rows_from_block(block: str) -> list[list[int]]:
    rows = []
    for line in block.splitlines():
        if "{" in line and re.search(r"\d", line):
            row = _parse_row(line)
            if row:
                rows.append(row)
    return rows


def parse_levels_h(path: str) -> list[LevelEntry]:
    with open(path, encoding="utf-8") as f:
        text = f.read()

    num_match = re.search(r"#define NUM_LEVELS (\d+)", text)
    num_levels = int(num_match.group(1)) if num_match else 0

    titles = re.findall(r"// --- Level \d+ \([^)]*\): (.+?) ---", text)
    if not titles:
        titles = re.findall(r"// --- Level \d+: (.+?) ---", text)

    bg_start = text.find("level_bg_data")
    fg_start = text.find("level_fg_data")
    if bg_start < 0 or fg_start < 0:
        raise ValueError("levels.h: missing level_bg_data or level_fg_data")

    bg_section = text[bg_start:fg_start]
    fg_section = text[fg_start : text.find("// Player coordinates")]

    bg_blocks = _split_level_blocks(bg_section)
    fg_blocks = _split_level_blocks(fg_section)

    px = _parse_int_array(
        re.search(r"level_player_x\[NUM_LEVELS\] = \{([^}]+)\}", text).group(0)
    )
    py = _parse_int_array(
        re.search(r"level_player_y\[NUM_LEVELS\] = \{([^}]+)\}", text).group(0)
    )
    sx = _parse_int_array(
        re.search(r"level_shady_x\[NUM_LEVELS\] = \{([^}]+)\}", text).group(0)
    )
    sy = _parse_int_array(
        re.search(r"level_shady_y\[NUM_LEVELS\] = \{([^}]+)\}", text).group(0)
    )

    n = num_levels or max(len(bg_blocks), len(fg_blocks), len(px))
    levels: list[LevelEntry] = []
    for i in range(n):
        title = titles[i] if i < len(titles) else f"Level {i}"
        bg_rows = _pad_grid(_rows_from_block(bg_blocks[i]) if i < len(bg_blocks) else _empty_bg())
        fg_rows = _pad_grid(_rows_from_block(fg_blocks[i]) if i < len(fg_blocks) else _empty_fg())
        levels.append(
            LevelEntry(
                title=title.strip(),
                bg=bg_rows,
                fg=fg_rows,
                player_x=px[i] if i < len(px) else 1,
                player_y=py[i] if i < len(py) else 1,
                shady_x=sx[i] if i < len(sx) else -1,
                shady_y=sy[i] if i < len(sy) else -1,
            )
        )
    return levels


def _pad_row(row: list[int], y: int) -> list[int]:
    row = list(row[:MAP_W])
    while len(row) < MAP_W:
        x = len(row)
        if y == 0 or y == MAP_H - 1 or x == 0 or x == MAP_W - 1:
            row.append(1)
        else:
            row.append(0)
    return row


def _pad_grid(rows: list[list[int]]) -> list[list[int]]:
    out = []
    for y in range(MAP_H):
        if y < len(rows):
            out.append(_pad_row(rows[y], y))
        else:
            out.append(_pad_row([], y))
    return out


def level_to_dict(level: LevelEntry) -> dict:
    return {
        "title": level.title,
        "bg": level.bg,
        "fg": level.fg,
        "player_x": level.player_x,
        "player_y": level.player_y,
        "shady_x": level.shady_x,
        "shady_y": level.shady_y,
    }


def level_from_dict(d: dict) -> LevelEntry:
    return LevelEntry(
        title=d.get("title", "Untitled"),
        bg=_pad_grid(d.get("bg", _empty_bg())),
        fg=_pad_grid(d.get("fg", _empty_fg())),
        player_x=int(d.get("player_x", 1)),
        player_y=int(d.get("player_y", 1)),
        shady_x=int(d.get("shady_x", -1)),
        shady_y=int(d.get("shady_y", -1)),
    )


def load_levels(project_root: str) -> list[LevelEntry]:
    json_path = puzzles_json_path(project_root)
    h_path = levels_h_path(project_root)
    if os.path.exists(json_path):
        with open(json_path, encoding="utf-8") as f:
            data = json.load(f)
        levels = [level_from_dict(x) for x in data.get("levels", [])]
        for lv in levels:
            sync_actor_positions(lv)
        return levels
    if os.path.exists(h_path):
        levels = parse_levels_h(h_path)
        for lv in levels:
            sync_actor_positions(lv)
        save_levels(project_root, levels)
        return levels
    return [LevelEntry(title="Level 0")]


def save_levels(project_root: str, levels: list[LevelEntry]) -> None:
    json_path = puzzles_json_path(project_root)
    os.makedirs(os.path.dirname(json_path), exist_ok=True)
    payload = {"version": 1, "map_size": {"width": MAP_W, "height": MAP_H}, "levels": [level_to_dict(lv) for lv in levels]}
    with open(json_path, "w", encoding="utf-8", newline="\n") as f:
        json.dump(payload, f, ensure_ascii=False, indent=2)
        f.write("\n")
    write_levels_h(levels_h_path(project_root), levels)


def _format_row(vals: list[int], trailing_comment: str = "") -> str:
    inner = ",".join(str(v) for v in vals)
    line = f"        {{{inner}}}"
    if trailing_comment:
        line += f" // {trailing_comment}"
    return line + ","


def write_levels_h(path: str, levels: list[LevelEntry]) -> None:
    n = len(levels)
    lines: list[str] = [
        "#ifndef LEVELS_H",
        "#define LEVELS_H",
        "",
        '#include "puzzle_types.h"',
        "",
        f"#define NUM_LEVELS {n}",
        "",
        "// Background layouts for each level",
        f"static const unsigned char level_bg_data[NUM_LEVELS][10][15] = {{",
    ]

    for i, lv in enumerate(levels):
        lines.append(f"    // --- Level {i}: {lv.title} ---")
        lines.append("    {")
        for y in range(MAP_H):
            lines.append(_format_row(lv.bg[y]))
        lines[-1] = lines[-1].rstrip(",")
        lines.append("    },")

    lines.append("};")
    lines.append("")
    lines.append("// Foreground starting layouts (Player, Barrels, Shady)")
    lines.append(f"static const unsigned char level_fg_data[NUM_LEVELS][10][15] = {{")

    for i, lv in enumerate(levels):
        lines.append(f"    // --- Level {i}: {lv.title} ---")
        lines.append("    {")
        for y in range(MAP_H):
            lines.append(_format_row(lv.fg[y]))
        lines[-1] = lines[-1].rstrip(",")
        lines.append("    },")

    lines.append("};")
    lines.append("")
    lines.append("// Player coordinates")
    px = ", ".join(str(lv.player_x) for lv in levels)
    py = ", ".join(str(lv.player_y) for lv in levels)
    lines.append(f"static const int level_player_x[NUM_LEVELS] = {{ {px} }};")
    lines.append(f"static const int level_player_y[NUM_LEVELS] = {{ {py} }};")
    lines.append("")
    lines.append("// Shady coordinates")
    sx = ", ".join(str(lv.shady_x) for lv in levels)
    sy = ", ".join(str(lv.shady_y) for lv in levels)
    lines.append(f"static const int level_shady_x[NUM_LEVELS] = {{ {sx} }};")
    lines.append(f"static const int level_shady_y[NUM_LEVELS] = {{ {sy} }};")
    lines.append("")
    lines.append("#endif // LEVELS_H")
    lines.append("")

    os.makedirs(os.path.dirname(path), exist_ok=True)
    with open(path, "w", encoding="utf-8", newline="\n") as f:
        f.write("\n".join(lines))


def sync_actor_positions(level: LevelEntry) -> None:
    """Align player/shady coords and fg_map markers."""
    for y in range(MAP_H):
        for x in range(MAP_W):
            if level.fg[y][x] == 1:
                level.fg[y][x] = 0
            if level.fg[y][x] == 3:
                level.fg[y][x] = 0
    if 0 <= level.player_x < MAP_W and 0 <= level.player_y < MAP_H:
        level.fg[level.player_y][level.player_x] = 1
    if level.shady_x >= 0 and level.shady_y >= 0:
        if level.shady_x < MAP_W and level.shady_y < MAP_H:
            level.fg[level.shady_y][level.shady_x] = 3


def validate_level(level: LevelEntry) -> list[str]:
    issues: list[str] = []
    players = sum(1 for y in range(MAP_H) for x in range(MAP_W) if level.fg[y][x] == 1)
    shadys = sum(1 for y in range(MAP_H) for x in range(MAP_W) if level.fg[y][x] == 3)
    if players != 1:
        issues.append(f"プレイヤーが {players} マスあります（1マス必要）")
    if shadys > 1:
        issues.append(f"シェイディが {shadys} マスあります（0または1）")
    if level.player_x < 0 or level.player_x >= MAP_W or level.player_y < 0 or level.player_y >= MAP_H:
        issues.append("プレイヤー座標が範囲外です（X:0〜14, Y:0〜9）")
    for y in range(MAP_H):
        if len(level.bg[y]) != MAP_W or len(level.fg[y]) != MAP_W:
            issues.append(f"行 {y} の幅が {MAP_W} ではありません")
    return issues
