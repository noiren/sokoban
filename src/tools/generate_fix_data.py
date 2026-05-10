#!/usr/bin/env python3
import os
import json
import fix_data_io

SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))
PROJECT_ROOT = os.path.dirname(os.path.dirname(SCRIPT_DIR))

def escape_str(s: str) -> str:
    if s is None: return "nullptr"
    return json.dumps(s, ensure_ascii=False)

def to_cpp_face_id(face_id: str) -> str:
    """ 'normal_1' -> 'FdFaceId::Normal_1' """
    if not face_id or face_id not in fix_data_io.FACE_IDS:
        return "FdFaceId::None"
    parts = face_id.split('_')
    return f"FdFaceId::{parts[0].capitalize()}_{parts[1]}"

def to_cpp_position(pos: str) -> str:
    if pos.upper() == "RIGHT": return "FdPosition::Right"
    return "FdPosition::Left"

def main():
    characters = fix_data_io.load_characters(PROJECT_ROOT)
    texts = fix_data_io.load_all_texts(PROJECT_ROOT)
    events = fix_data_io.load_all_events(PROJECT_ROOT)
    gallery = fix_data_io.load_gallery(PROJECT_ROOT)

    out_dir = os.path.join(PROJECT_ROOT, "src", "game", "include", "generated")
    os.makedirs(out_dir, exist_ok=True)
    out_path = os.path.join(out_dir, "generated_fix_data.h")

    lines = [
        "// AUTO GENERATED FILE. DO NOT EDIT.",
        "#pragma once",
        "#include <cstdint>",
        "",
        "enum class FdFaceId : uint8_t {",
        "    Normal_1, Normal_2, Normal_3,",
        "    Smile_1, Smile_2, Smile_3,",
        "    Sad_1, Sad_2, Sad_3,",
        "    Angry_1, Angry_2, Angry_3,",
        "    Surprised_1, Surprised_2, Surprised_3,",
        "    Happy_1, Happy_2, Happy_3,",
        "    Think_1, Think_2, Think_3,",
        "    None = 255",
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
        "    const char* face_images[21];",
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
        "};",
        "",
        "struct FdEventEntry {",
        "    const char* id;",
        "    const char* title_ja;",
        "    const FdEventLine* lines;",
        "    uint16_t line_count;",
        "};",
        "",
        "struct FdGalleryEntry {",
        "    const char* category;",
        "    const char* resource_id;",
        "    const char* ja;",
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
            lines.append(f"    {{{escape_str(ln.speaker_id)}, {to_cpp_face_id(ln.face_id)}, {to_cpp_position(ln.position)}, {escape_str(ln.image_id)}, {escape_str(ln.text)}}},")
        lines.append("};")

    lines.append("static constexpr FdEventEntry g_events[] = {")
    for e in events:
        line_ptr = f"lines_{fix_data_io.sanitize_cpp_token(e.id)}" if e.lines else "nullptr"
        lines.append(f"    {{{escape_str(e.id)}, {escape_str(e.title_ja)}, {line_ptr}, {len(e.lines)}}},")
    lines.append("};")
    lines.append(f"static constexpr uint16_t kEventCount = {len(events)};\n")

    # --- Gallery ---
    lines.append("static constexpr FdGalleryEntry g_gallery[] = {")
    for g in gallery:
        lines.append(f"    {{{escape_str(g.category)}, {escape_str(g.resource_id)}, {escape_str(g.ja)}}},")
    lines.append("};")
    lines.append(f"static constexpr uint16_t kGalleryCount = {len(gallery)};\n")

    with open(out_path, "w", encoding="utf-8", newline="\n") as f:
        f.write("\n".join(lines) + "\n")
    print(f"Generated {out_path}")

if __name__ == "__main__":
    main()