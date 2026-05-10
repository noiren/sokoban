#!/usr/bin/env python3
import json
import os
import re
from dataclasses import dataclass, field
from typing import Optional

# ---------------------------------------------------------
# 定数定義
# ---------------------------------------------------------
FACE_BASE_TYPES = ["normal", "smile", "sad", "angry", "surprised", "happy", "think"]
FACE_VARIANTS = 3
FACE_IDS = [f"{b}_{i}" for b in FACE_BASE_TYPES for i in range(1, FACE_VARIANTS + 1)]

# ---------------------------------------------------------
# パス解決
# ---------------------------------------------------------
def fixdata_root(project_root: str) -> str:
    return os.path.join(project_root, "Asset", "fixdata")

def characters_path(project_root: str) -> str:
    return os.path.join(fixdata_root(project_root), "characters", "characters.json")

def text_dir(project_root: str) -> str:
    return os.path.join(fixdata_root(project_root), "text")

def events_dir(project_root: str) -> str:
    return os.path.join(fixdata_root(project_root), "events")

def gallery_path(project_root: str) -> str:
    return os.path.join(fixdata_root(project_root), "gallery.json")

def audio_manifest_path(project_root: str) -> str:
    return os.path.join(project_root, "Asset", "audio", "audio_manifest.json")

def sprite_chara_dir(project_root: str) -> str:
    return os.path.join(project_root, "Asset", "graphics", "sprites", "chara")

def still_dir(project_root: str) -> str:
    return os.path.join(project_root, "Asset", "graphics", "stills")

# ---------------------------------------------------------
# データクラス定義
# ---------------------------------------------------------
@dataclass
class CharacterEntry:
    id: str
    name_ja: str
    faces: dict[str, str] = field(default_factory=dict)

@dataclass
class TextEntry:
    id: str
    category: str
    ja: str

@dataclass
class EventLine:
    speaker_id: str
    face_id: str
    position: str # "LEFT" or "RIGHT"
    image_id: str
    text: str

@dataclass
class EventEntry:
    id: str
    title_ja: str
    lines: list[EventLine] = field(default_factory=list)

@dataclass
class GalleryEntry:
    category: str # "bustup", "still", "bgm", "se"
    resource_id: str
    ja: str

# ---------------------------------------------------------
# 共通I/O関数
# ---------------------------------------------------------
def sanitize_cpp_token(s: str) -> str:
    tok = re.sub(r"[^A-Za-z0-9_]", "_", s)
    if tok and tok[0].isdigit():
        tok = "_" + tok
    return tok

def load_json(path: str) -> dict:
    if not os.path.exists(path): return {}
    with open(path, encoding="utf-8") as f:
        return json.load(f)

def save_json(path: str, data: dict) -> None:
    os.makedirs(os.path.dirname(path), exist_ok=True)
    with open(path, "w", encoding="utf-8", newline="\n") as f:
        json.dump(data, f, ensure_ascii=False, indent=2)
        f.write("\n")

# ---------------------------------------------------------
# 各種リソース読み書き・探索
# ---------------------------------------------------------
def find_chara_sprite_path(project_root: str, image_id: str) -> Optional[str]:
    if not image_id: return None
    base_dir = sprite_chara_dir(project_root)
    if not os.path.exists(base_dir): return None
    
    candidates = [f"spr_ch_{image_id}.bmp", f"{image_id}.bmp"]
    for root, _, files in os.walk(base_dir):
        for f in files:
            if f.lower() in candidates:
                return os.path.join(root, f)
    return None

def find_still_image_path(project_root: str, resource_id: str) -> Optional[str]:
    if not resource_id: return None
    s_dir = still_dir(project_root)
    if not os.path.exists(s_dir): return None
    
    candidate = f"{resource_id.lower()}.bmp"
    for root, _, files in os.walk(s_dir):
        for f in files:
            if f.lower() == candidate:
                return os.path.join(root, f)
    return None

def scan_still_resources(project_root: str) -> list[str]:
    s_dir = still_dir(project_root)
    if not os.path.exists(s_dir): return []
    resource_ids = []
    for root, _, files in os.walk(s_dir):
        for f in files:
            if f.lower().endswith(".bmp"):
                resource_ids.append(os.path.splitext(f)[0])
    return sorted(resource_ids)

def get_audio_ids(project_root: str) -> tuple[list[str], list[str]]:
    data = load_json(audio_manifest_path(project_root))
    bgms = [item.get("id", "") for item in data.get("bgm", [])]
    ses = [item.get("id", "") for item in data.get("se", [])]
    return sorted(bgms), sorted(ses)

# --- Characters ---
def load_characters(project_root: str) -> list[CharacterEntry]:
    data = load_json(characters_path(project_root))
    result = []
    for c in data.get("characters", []):
        faces_data = c.get("faces", {})
        if isinstance(faces_data, list):
            new_faces = {}
            for i, f in enumerate(faces_data):
                fid = f.get("face_id", "")
                if fid not in FACE_IDS and i < len(FACE_IDS):
                    fid = FACE_IDS[i]
                new_faces[fid] = f.get("image_id", "")
            faces_data = new_faces
        result.append(CharacterEntry(id=c["id"], name_ja=c["name_ja"], faces=faces_data))
    return result

def save_characters(project_root: str, characters: list[CharacterEntry]) -> None:
    data = {
        "version": 2,
        "characters": [{"id": c.id, "name_ja": c.name_ja, "faces": c.faces} for c in characters]
    }
    save_json(characters_path(project_root), data)

# --- Events ---
def load_all_events(project_root: str) -> list[EventEntry]:
    edir = events_dir(project_root)
    if not os.path.exists(edir): return []
    result = []
    for fname in sorted(os.listdir(edir)):
        if fname.startswith("evt_") and fname.endswith(".json"):
            result.append(_parse_event_file(os.path.join(edir, fname)))
    return result

def load_event_file(project_root: str, filename: str) -> EventEntry:
    return _parse_event_file(os.path.join(events_dir(project_root), filename))

def _parse_event_file(path: str) -> EventEntry:
    data = load_json(path)
    lines = [
        EventLine(
            speaker_id=ln.get("speaker_id", ""),
            face_id=ln.get("face_id", ""),
            position=ln.get("position", "LEFT"),
            image_id=ln.get("image_id", ""),
            text=ln.get("text", "")
        ) for ln in data.get("lines", [])
    ]
    return EventEntry(id=data.get("id", ""), title_ja=data.get("title_ja", "名称未設定"), lines=lines)

def save_event_file(project_root: str, filename: str, event: EventEntry) -> None:
    data = {
        "version": 2,
        "id": event.id,
        "title_ja": event.title_ja,
        "lines": [
            {
                "speaker_id": ln.speaker_id,
                "face_id": ln.face_id,
                "position": ln.position,
                "image_id": ln.image_id,
                "text": ln.text
            } for ln in event.lines
        ]
    }
    save_json(os.path.join(events_dir(project_root), filename), data)

def event_filename_from_id(event_id: str) -> str:
    return "evt_" + event_id.lower() + ".json"

# --- Text ---
def load_all_texts(project_root: str) -> list[TextEntry]:
    tdir = text_dir(project_root)
    if not os.path.exists(tdir): return []
    result = []
    for fname in sorted(os.listdir(tdir)):
        if not (fname.startswith("text_") and fname.endswith(".json")): continue
        data = load_json(os.path.join(tdir, fname))
        cat = data.get("category", "unknown")
        for e in data.get("entries", []):
            result.append(TextEntry(id=e["id"], category=cat, ja=e["ja"]))
    return result

def load_text_files(project_root: str) -> list[tuple[str, str, list[TextEntry]]]:
    tdir = text_dir(project_root)
    if not os.path.exists(tdir): return []
    result = []
    for fname in sorted(os.listdir(tdir)):
        if not (fname.startswith("text_") and fname.endswith(".json")): continue
        data = load_json(os.path.join(tdir, fname))
        cat = data.get("category", "unknown")
        entries = [TextEntry(id=e["id"], category=cat, ja=e["ja"]) for e in data.get("entries", [])]
        result.append((fname, cat, entries))
    return result

def save_text_file(project_root: str, filename: str, category: str, entries: list[TextEntry]) -> None:
    data = {"version": 1, "category": category, "entries": [{"id": e.id, "ja": e.ja} for e in entries]}
    save_json(os.path.join(text_dir(project_root), filename), data)

# --- Gallery ---
def load_gallery(project_root: str) -> list[GalleryEntry]:
    data = load_json(gallery_path(project_root))
    return [GalleryEntry(category=e["category"], resource_id=e["resource_id"], ja=e["ja"]) for e in data.get("entries", [])]

def save_gallery(project_root: str, entries: list[GalleryEntry]) -> None:
    data = {"version": 2, "entries": [{"category": e.category, "resource_id": e.resource_id, "ja": e.ja} for e in entries]}
    save_json(gallery_path(project_root), data)