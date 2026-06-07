#!/usr/bin/env python3
import os
import json
import sys
SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))
PROJECT_ROOT = os.path.dirname(os.path.dirname(SCRIPT_DIR))

# Add gameeditor to path for fix_data_io
sys.path.insert(0, os.path.join(PROJECT_ROOT, "src", "gameeditor"))
import fix_data_io

def escape_str(s: str) -> str:
    if s is None: return "nullptr"
    return json.dumps(s, ensure_ascii=False)

def to_cpp_enum_name(face_id: str) -> str:
    """ 'normal_1' -> 'Normal_1' (末尾 _<数字> を variant とみなす) """
    base, _, var = face_id.rpartition('_')
    return f"{base.capitalize()}_{var}"

def to_cpp_face_id(face_id: str) -> str:
    """ 'normal_1' -> 'FdFaceId::Normal_1' """
    if not face_id or face_id not in fix_data_io.FACE_IDS:
        return "FdFaceId::None"
    return f"FdFaceId::{to_cpp_enum_name(face_id)}"

def build_face_enum_lines() -> list:
    """感情カタログから FdFaceId enum の本体行を構築。"""
    lines = []
    for c in fix_data_io.FACE_CATEGORIES:
        cap = str(c["id"]).capitalize()
        names = [f"{cap}_{i}" for i in range(1, int(c.get("variants", 3)) + 1)]
        lines.append("    " + ", ".join(names) + ",")
    lines.append("    None = 255")
    return lines

def to_cpp_position(pos: str) -> str:
    if pos.upper() == "RIGHT": return "FdPosition::Right"
    return "FdPosition::Left"

def main():
    characters = fix_data_io.load_characters(PROJECT_ROOT)
    texts = fix_data_io.load_all_texts(PROJECT_ROOT)
    events = fix_data_io.load_all_events(PROJECT_ROOT)
    puzzle_events = fix_data_io.load_all_puzzle_events(PROJECT_ROOT)
    still_events = fix_data_io.load_all_still_events(PROJECT_ROOT)
    gallery = fix_data_io.load_gallery(PROJECT_ROOT)
    chapters = fix_data_io.load_story_progression(PROJECT_ROOT)
    unlock_rules = fix_data_io.load_unlock_rules(PROJECT_ROOT)

    out_dir = os.path.join(PROJECT_ROOT, "src", "game", "include", "generated")
    os.makedirs(out_dir, exist_ok=True)
    out_path = os.path.join(out_dir, "generated_fix_data.h")

    face_count = len(fix_data_io.FACE_IDS)

    lines = [
        "// AUTO GENERATED FILE. DO NOT EDIT.",
        "#pragma once",
        "#include <cstdint>",
        '#include "generated/audio_ids.h"',
        "",
        f"static constexpr uint16_t kFaceImageCount = {face_count};",
        "",
        "enum class FdFaceId : uint8_t {",
    ]
    lines += build_face_enum_lines()
    lines += [
        "};",
        "",
        "enum class FdPosition : uint8_t {",
        "    Left,",
        "    Right",
        "};",
        "",
        "struct FdCharacterEntry {",
        "    const char* id;",
        "    const char* name_ja;",
        f"    const char* face_images[{face_count}];",
        "};",
        "",
        "struct FdTextEntry {",
        "    const char* id;",
        "    const char* category;",
        "    const char* ja;",
        "};",
        "",
        "struct FdEventLine {",
        "    const char* speaker_id;",
        "    FdFaceId face_id;",
        "    FdPosition position;",
        "    const char* image_id;",
        "    const char* text;",
        "    BgmId bgm_id;",
        "    SeId se_id;",
        "    bool stop_bgm;",
        "    const char* emotion_id;",
        "    const char* center_image_id;",
        "};",
        "",
        "struct FdEventEntry {",
        "    const char* id;",
        "    const char* title_ja;",
        "    const FdEventLine* lines;",
        "    uint16_t line_count;",
        "};",
        "",
        "struct FdStillEventMessage {",
        "    const char* text;",
        "    SeId se_id;",
        "    BgmId bgm_id;",
        "    bool stop_bgm;",
        "};",
        "",
        "struct FdStillEventPage {",
        "    const char* still_image_id;",
        "    uint16_t fade_in_frames;",
        "    uint16_t fade_out_frames;",
        "    const FdStillEventMessage* messages;",
        "    uint16_t message_count;",
        "};",
        "",
        "struct FdStillEventEntry {",
        "    const char* id;",
        "    const char* title_ja;",
        "    const FdStillEventPage* pages;",
        "    uint16_t page_count;",
        "};",
        "",
        "struct FdGalleryEntry {",
        "    const char* category;",
        "    const char* resource_id;",
        "    const char* ja;",
        "    int16_t     unlock_flag; // -1=\u5e38\u306b\u89e3\u7981\u3001>=0=\u30d5\u30e9\u30b0\u53c2\u7167",
        "};",
        "",
        "struct FdUnlockRule {",
        "    const char* event_id;   // \u3053\u306e\u30a4\u30d9\u30f3\u30c8ID\u306e\u518d\u751f\u5b8c\u4e86\u6642",
        "    int16_t     flag_id;    // \u3053\u306e\u30d5\u30e9\u30b0\u3092\u7acb\u3066\u308b",
        "};",
        "",
        "enum class FdStoryStepType : uint8_t {",
        "    STILL_EVENT = 0,",
        "    EVENT       = 1,",
        "    PUZZLE      = 2,",
        "};",
        "",
        "struct FdStoryStep {",
        "    FdStoryStepType type;",
        "    int16_t         puzzle_ref;      // PUZZLE時のみ使用（-1=無効）",
        "    const char*     event_ref;       // EVENT/STILL_EVENT時のみ使用（nullptr=無効）",
        "    const char*     intro_event_ref; // PUZZLE専用：パズル前イベントID（nullptr=なし）",
        "};",
        "",
        "struct FdStoryChapter {",
        "    const char*        id;",
        "    const char*        title_ja;",
        "    const FdStoryStep* steps;",
        "    int16_t            num_steps;",
        "};",
        ""
    ]

    # --- Characters ---
    lines.append("static constexpr FdCharacterEntry g_characters[] = {")
    for c in characters:
        face_images = []
        for fid in fix_data_io.FACE_IDS:
            img = c.faces.get(fid, "")
            face_images.append(escape_str(img) if img else "nullptr")
        faces_array = "{" + ", ".join(face_images) + "}"
        lines.append(f"    {{{escape_str(c.id)}, {escape_str(c.name_ja)}, {faces_array}}},")
    lines.append("};")
    lines.append(f"static constexpr uint16_t kCharacterCount = {len(characters)};\n")

    # --- Texts ---
    lines.append("static constexpr FdTextEntry g_texts[] = {")
    for t in texts:
        lines.append(f"    {{{escape_str(t.id)}, {escape_str(t.category)}, {escape_str(t.ja)}}},")
    lines.append("};")
    lines.append(f"static constexpr uint16_t kTextCount = {len(texts)};\n")

    # --- Events ---
    for e in events:
        if not e.lines: continue
        lines.append(f"static constexpr FdEventLine lines_{fix_data_io.sanitize_cpp_token(e.id)}[] = {{")
        for ln in e.lines:
            bgm_val = f"BgmId::{ln.bgm_id}" if ln.bgm_id else "BgmId::COUNT"
            se_val = f"SeId::{ln.se_id}" if ln.se_id else "SeId::COUNT"
            stop_val = "true" if ln.stop_bgm else "false"
            emotion_val = escape_str(ln.emotion_id) if hasattr(ln, 'emotion_id') else "nullptr"
            lines.append(f"    {{{escape_str(ln.speaker_id)}, {to_cpp_face_id(ln.face_id)}, {to_cpp_position(ln.position)}, {escape_str(ln.image_id)}, {escape_str(ln.text)}, {bgm_val}, {se_val}, {stop_val}, {emotion_val}, {escape_str(ln.center_image_id)}}},")
        lines.append("};")

    lines.append("static constexpr FdEventEntry g_events[] = {")
    for e in events:
        line_ptr = f"lines_{fix_data_io.sanitize_cpp_token(e.id)}" if e.lines else "nullptr"
        lines.append(f"    {{{escape_str(e.id)}, {escape_str(e.title_ja)}, {line_ptr}, {len(e.lines)}}},")
    lines.append("};")
    lines.append(f"static constexpr uint16_t kEventCount = {len(events)};\n")

    # --- Puzzle Events ---
    for e in puzzle_events:
        if not e.lines: continue
        lines.append(f"static constexpr FdEventLine pze_lines_{fix_data_io.sanitize_cpp_token(e.id)}[] = {{")
        for ln in e.lines:
            bgm_val = f"BgmId::{ln.bgm_id}" if ln.bgm_id else "BgmId::COUNT"
            se_val = f"SeId::{ln.se_id}" if ln.se_id else "SeId::COUNT"
            stop_val = "true" if ln.stop_bgm else "false"
            emotion_val = escape_str(ln.emotion_id) if hasattr(ln, 'emotion_id') else "nullptr"
            lines.append(f"    {{{escape_str(ln.speaker_id)}, {to_cpp_face_id(ln.face_id)}, {to_cpp_position(ln.position)}, {escape_str(ln.image_id)}, {escape_str(ln.text)}, {bgm_val}, {se_val}, {stop_val}, {emotion_val}, {escape_str(ln.center_image_id)}}},")
        lines.append("};")

    lines.append("static constexpr FdEventEntry g_puzzle_events[] = {")
    if not puzzle_events:
        lines.append("    {nullptr, nullptr, nullptr, 0}")
    else:
        for e in puzzle_events:
            line_ptr = f"pze_lines_{fix_data_io.sanitize_cpp_token(e.id)}" if e.lines else "nullptr"
            lines.append(f"    {{{escape_str(e.id)}, {escape_str(e.title_ja)}, {line_ptr}, {len(e.lines)}}},")
    lines.append("};")
    lines.append(f"static constexpr uint16_t kPuzzleEventCount = {len(puzzle_events)};\n")

    # --- Still Events ---
    for e in still_events:
        for p_idx, p in enumerate(e.pages):
            if not p.messages: continue
            lines.append(f"static constexpr FdStillEventMessage msgs_{fix_data_io.sanitize_cpp_token(e.id)}_p{p_idx}[] = {{")
            for m in p.messages:
                bgm_val = f"BgmId::{m.bgm_id}" if m.bgm_id else "BgmId::COUNT"
                se_val = f"SeId::{m.se_id}" if m.se_id else "SeId::COUNT"
                stop_val = "true" if m.stop_bgm else "false"
                lines.append(f"    {{{escape_str(m.text)}, {se_val}, {bgm_val}, {stop_val}}},")
            lines.append("};")

        if not e.pages: continue
        lines.append(f"static constexpr FdStillEventPage pages_{fix_data_io.sanitize_cpp_token(e.id)}[] = {{")
        for p_idx, p in enumerate(e.pages):
            msgs_ptr = f"msgs_{fix_data_io.sanitize_cpp_token(e.id)}_p{p_idx}" if p.messages else "nullptr"
            lines.append(f"    {{{escape_str(p.still_image_id)}, {p.fade_in_frames}, {p.fade_out_frames}, {msgs_ptr}, {len(p.messages)}}},")
        lines.append("};")

    lines.append("static constexpr FdStillEventEntry g_still_events[] = {")
    if not still_events:
        lines.append("    {nullptr, nullptr, nullptr, 0}")
    else:
        for e in still_events:
            pages_ptr = f"pages_{fix_data_io.sanitize_cpp_token(e.id)}" if e.pages else "nullptr"
            lines.append(f"    {{{escape_str(e.id)}, {escape_str(e.title_ja)}, {pages_ptr}, {len(e.pages)}}},")
    lines.append("};")
    lines.append(f"static constexpr uint16_t kStillEventCount = {len(still_events)};\n")


    # --- Event Flags (Auto-assign to ALL events) ---
    # FLAG_GALLERY_ITEM_BASE = 32 以降を自動割り当て
    FLAG_GALLERY_ITEM_BASE = 32
    event_to_flag: dict[str, int] = {}  # イベントID -> フラグ番号
    next_flag = FLAG_GALLERY_ITEM_BASE

    # 全イベント・スチルイベントに一意のフラグを割り当て
    for e in events:
        event_to_flag[e.id] = next_flag
        next_flag += 1
    for e in still_events:
        event_to_flag[e.id] = next_flag
        next_flag += 1

    # ギャラリーの unlock_event_id への紐付け
    gallery_flags: list[int] = []  # gallery[i] -> unlock_flag
    for g in gallery:
        eid = (g.unlock_event_id or "").strip()
        if not eid:
            gallery_flags.append(-1)
        elif eid in event_to_flag:
            gallery_flags.append(event_to_flag[eid])
        else:
            event_to_flag[eid] = next_flag
            gallery_flags.append(next_flag)
            next_flag += 1

    lines.append("static constexpr FdGalleryEntry g_gallery[] = {")
    for i, g in enumerate(gallery):
        lines.append(f"    {{{escape_str(g.category)}, {escape_str(g.resource_id)}, {escape_str(g.ja)}, {gallery_flags[i]}}},")
    lines.append("};")
    lines.append(f"static constexpr uint16_t kGalleryCount = {len(gallery)};\n")

    # --- Unlock Rules (gallery自動生成 + 手動指定分の合算) ---
    # gallery自動生成: event_to_flagから生成
    all_rules: list[tuple[str, int]] = list(event_to_flag.items())  # (event_id, flag_id)
    # 手動指定のunlock_rules.json分も追加（モード解禁等）
    for r in unlock_rules:
        if r.event_id and r.flag_id >= 0:
            all_rules.append((r.event_id, r.flag_id))

    lines.append("static constexpr FdUnlockRule g_unlock_rules[] = {")
    if all_rules:
        for eid, fid in all_rules:
            lines.append(f"    {{{escape_str(eid)}, {fid}}},")
    else:
        lines.append("    {nullptr, -1}  // \u30c0\u30df\u30fc")
    lines.append("};")
    lines.append(f"static constexpr int16_t kUnlockRuleCount = {len(all_rules)};\n")

    # --- Story Progression ---
    for i, ch in enumerate(chapters):
        steps = ch.get("steps", [])
        safe_id = ch["id"].lower().replace("-", "_")
        lines.append(f"static constexpr FdStoryStep steps_{safe_id}[] = {{")
        for step in steps:
            stype = step["type"]
            if stype == "STILL_EVENT":
                lines.append(f"    {{FdStoryStepType::STILL_EVENT, -1, {escape_str(step['ref'])}, nullptr}},")
            elif stype == "EVENT":
                lines.append(f"    {{FdStoryStepType::EVENT, -1, {escape_str(step['ref'])}, nullptr}},")
            elif stype == "PUZZLE":
                intro = step.get("intro_event")
                intro_str = escape_str(intro) if intro else "nullptr"
                lines.append(f"    {{FdStoryStepType::PUZZLE, {step['ref']}, nullptr, {intro_str}}},")
        lines.append("};")

    lines.append("static constexpr FdStoryChapter g_story_chapters[] = {")
    for ch in chapters:
        safe_id = ch["id"].lower().replace("-", "_")
        steps = ch.get("steps", [])
        lines.append(f"    {{{escape_str(ch['id'])}, {escape_str(ch.get('title_ja', ''))}, steps_{safe_id}, {len(steps)}}},")
    lines.append("};")
    lines.append(f"static constexpr int16_t kStoryChapterCount = {len(chapters)};\n")

    with open(out_path, "w", encoding="utf-8", newline="\n") as f:
        f.write("\n".join(lines) + "\n")
    print(f"Generated {out_path}")

if __name__ == "__main__":
    main()