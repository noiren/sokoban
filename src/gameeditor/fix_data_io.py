#!/usr/bin/env python3
import json
import os
import re
from dataclasses import dataclass, field
from typing import Optional

# ---------------------------------------------------------
# 定数定義 (感情カタログ駆動)
# ---------------------------------------------------------
# 感情カテゴリは Asset/fixdata/face_catalog.json を単一の真実とする。
# カタログが無い/壊れている場合は従来の 7カテゴリ×3 にフォールバックする。
_THIS_DIR = os.path.dirname(os.path.abspath(__file__))
_PROJECT_ROOT_FOR_CATALOG = os.path.dirname(os.path.dirname(_THIS_DIR))

_DEFAULT_FACE_CATEGORIES = [
    {"id": "normal", "ja": "通常", "variants": 3},
    {"id": "smile", "ja": "笑顔", "variants": 3},
    {"id": "sad", "ja": "悲しみ", "variants": 3},
    {"id": "angry", "ja": "怒り", "variants": 3},
    {"id": "surprised", "ja": "驚き", "variants": 3},
    {"id": "happy", "ja": "喜び", "variants": 3},
    {"id": "think", "ja": "考え", "variants": 3},
]


def face_catalog_path(project_root: str) -> str:
    return os.path.join(project_root, "Asset", "fixdata", "face_catalog.json")


def load_face_categories(project_root: str) -> list[dict]:
    """[{'id','ja','variants'}] を返す。失敗時はデフォルト。"""
    path = face_catalog_path(project_root)
    if os.path.exists(path):
        try:
            with open(path, "r", encoding="utf-8") as f:
                data = json.load(f)
            cats = data.get("categories", [])
            norm = []
            for c in cats:
                cid = str(c.get("id", "")).strip()
                if not cid:
                    continue
                norm.append({
                    "id": cid,
                    "ja": c.get("ja", cid),
                    "variants": max(1, int(c.get("variants", 3))),
                })
            if norm:
                return norm
        except Exception:
            pass
    return [dict(c) for c in _DEFAULT_FACE_CATEGORIES]


def save_face_categories(project_root: str, categories: list[dict]) -> None:
    data = {"version": 1, "categories": categories}
    save_json(face_catalog_path(project_root), data)


def build_face_ids(categories: list[dict]) -> list[str]:
    ids = []
    for c in categories:
        for i in range(1, int(c.get("variants", 3)) + 1):
            ids.append(f"{c['id']}_{i}")
    return ids


# モジュールロード時にカタログを読み込んで定数を構築(各エディタはこれを参照)
FACE_CATEGORIES = load_face_categories(_PROJECT_ROOT_FOR_CATALOG)
FACE_BASE_TYPES = [c["id"] for c in FACE_CATEGORIES]
FACE_IDS = build_face_ids(FACE_CATEGORIES)

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

def puzzle_events_dir(project_root: str) -> str:
    return os.path.join(fixdata_root(project_root), "puzzle_events")

def still_events_dir(project_root: str) -> str:
    return os.path.join(fixdata_root(project_root), "still_events")

def gallery_path(project_root: str) -> str:
    return os.path.join(fixdata_root(project_root), "gallery.json")

def story_progression_path(project_root: str) -> str:
    return os.path.join(fixdata_root(project_root), "story_progression.json")

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
    bgm_id: str = ""
    se_id: str = ""
    stop_bgm: bool = False
    emotion_id: str = ""
    center_image_id: str = ""

@dataclass
class EventEntry:
    id: str
    title_ja: str
    lines: list[EventLine] = field(default_factory=list)

@dataclass
class StillEventMessage:
    text: str
    se_id: str = ""
    bgm_id: str = ""
    stop_bgm: bool = False

@dataclass
class StillEventPage:
    still_image_id: str
    fade_in_frames: int = 16
    fade_out_frames: int = 16
    messages: list[StillEventMessage] = field(default_factory=list)

@dataclass
class StillEventEntry:
    id: str
    title_ja: str
    pages: list[StillEventPage] = field(default_factory=list)

@dataclass
class GalleryEntry:
    category: str # "tachi-e", "still", "event", "bgm", "se"
    resource_id: str
    ja: str
    unlock_event_id: str = ""  # "" = 常時解禁、以外 = そのイベント再生完了後に解禁

@dataclass
class UnlockRule:
    event_id: str   # このイベントIDの再生完了時
    flag_id: int    # このフラグを立てる

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
# 各種リソース読み書き
# ---------------------------------------------------------
def find_chara_sprite_path(project_root: str, image_id: str) -> Optional[str]:
    if not image_id: return None
    base_dir = sprite_chara_dir(project_root)
    if not os.path.exists(base_dir): return None
    candidates = [f"spr_ch_{image_id}.bmp", f"{image_id}.bmp"]
    for root, _, files in os.walk(base_dir):
        for f in files:
            if f in candidates:
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
    ses = [f"{item.get('category', '')}_{item.get('id', '')}" for item in data.get("se", [])]
    return sorted(bgms), sorted(ses)

# キャラクター
def load_characters(project_root: str) -> list[CharacterEntry]:
    path = characters_path(project_root)
    data = load_json(path)
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
    path = characters_path(project_root)
    data = {
        "version": 2,
        "characters": [{"id": c.id, "name_ja": c.name_ja, "faces": c.faces} for c in characters]
    }
    save_json(path, data)

# イベント
def load_all_events(project_root: str) -> list[EventEntry]:
    edir = events_dir(project_root)
    if not os.path.exists(edir): return []
    result = []
    for fname in sorted(os.listdir(edir)):
        if fname.startswith("evt_") and fname.endswith(".json"):
            result.append(_parse_event_file(os.path.join(edir, fname)))
    return result

def load_all_puzzle_events(project_root: str) -> list[EventEntry]:
    pdir = puzzle_events_dir(project_root)
    if not os.path.exists(pdir): return []
    result = []
    for fname in sorted(os.listdir(pdir)):
        if fname.startswith("pze_") and fname.endswith(".json"):
            result.append(_parse_event_file(os.path.join(pdir, fname)))
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
            text=ln.get("text", ""),
            bgm_id=ln.get("bgm_id", ""),
            se_id=ln.get("se_id", ""),
            stop_bgm=ln.get("stop_bgm", False),
            emotion_id=ln.get("emotion_id", ""),
            center_image_id=ln.get("center_image_id", ""),
        ) for ln in data.get("lines", [])
    ]
    return EventEntry(id=data.get("id", ""), title_ja=data.get("title_ja", "名称未設定"), lines=lines)

def save_event_file(project_root: str, filename: str, event: EventEntry) -> None:
    path = os.path.join(events_dir(project_root), filename)
    _save_event_impl(path, event)

def save_puzzle_event_file(project_root: str, filename: str, event: EventEntry) -> None:
    path = os.path.join(puzzle_events_dir(project_root), filename)
    _save_event_impl(path, event)

def _save_event_impl(path: str, event: EventEntry) -> None:
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
                "text": ln.text,
                "bgm_id": ln.bgm_id,
                "se_id": ln.se_id,
                "stop_bgm": ln.stop_bgm,
                "emotion_id": ln.emotion_id,
                "center_image_id": ln.center_image_id,
            } for ln in event.lines
        ]
    }
    save_json(path, data)

def event_filename_from_id(event_id: str) -> str:
    lower_id = event_id.lower()
    if lower_id.startswith("evt_"):
        return lower_id + ".json"
    return "evt_" + lower_id + ".json"

def puzzle_event_filename_from_id(event_id: str) -> str:
    lower_id = event_id.lower()
    if lower_id.startswith("pze_"):
        return lower_id + ".json"
    return "pze_" + lower_id + ".json"

# スチルイベント
def load_all_still_events(project_root: str) -> list[StillEventEntry]:
    edir = still_events_dir(project_root)
    if not os.path.exists(edir): return []
    result = []
    for fname in sorted(os.listdir(edir)):
        if fname.startswith("sevt_") and fname.endswith(".json"):
            result.append(_parse_still_event_file(os.path.join(edir, fname)))
    return result

def _parse_still_event_file(path: str) -> StillEventEntry:
    data = load_json(path)
    pages = []
    for p in data.get("pages", []):
        msgs = [
            StillEventMessage(
                text=m.get("text", ""),
                se_id=m.get("se_id", ""),
                bgm_id=m.get("bgm_id", ""),
                stop_bgm=m.get("stop_bgm", False)
            ) for m in p.get("messages", [])
        ]
        pages.append(StillEventPage(
            still_image_id=p.get("still_image_id", ""),
            fade_in_frames=p.get("fade_in_frames", 16),
            fade_out_frames=p.get("fade_out_frames", 16),
            messages=msgs
        ))
    return StillEventEntry(id=data.get("id", ""), title_ja=data.get("title_ja", "名称未設定"), pages=pages)

def save_still_event_file(project_root: str, filename: str, event: StillEventEntry) -> None:
    path = os.path.join(still_events_dir(project_root), filename)
    data = {
        "version": 1,
        "id": event.id,
        "title_ja": event.title_ja,
        "pages": [
            {
                "still_image_id": p.still_image_id,
                "fade_in_frames": p.fade_in_frames,
                "fade_out_frames": p.fade_out_frames,
                "messages": [
                    {
                        "text": m.text,
                        "se_id": m.se_id,
                        "bgm_id": m.bgm_id,
                        "stop_bgm": m.stop_bgm
                    } for m in p.messages
                ]
            } for p in event.pages
        ]
    }
    save_json(path, data)

def still_event_filename_from_id(event_id: str) -> str:
    lower_id = event_id.lower()
    if lower_id.startswith("sevt_"):
        return lower_id + ".json"
    return "sevt_" + lower_id + ".json"

# テキスト
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
    path = os.path.join(text_dir(project_root), filename)
    data = {"version": 1, "category": category, "entries": [{"id": e.id, "ja": e.ja} for e in entries]}
    save_json(path, data)

# ギャラリー
def load_gallery(project_root: str) -> list[GalleryEntry]:
    path = gallery_path(project_root)
    data = load_json(path)
    result = []
    for e in data.get("entries", []):
        result.append(GalleryEntry(
            category=e.get("category",""),
            resource_id=e.get("resource_id",""),
            ja=e.get("ja",""),
            unlock_event_id=e.get("unlock_event_id", e.get("unlock_flag_compat","")) or ""
        ))
    return result

def save_gallery(project_root: str, entries: list[GalleryEntry]) -> None:
    path = gallery_path(project_root)
    data = {
        "version": 2,
        "entries": [
            {"category": e.category, "resource_id": e.resource_id, "ja": e.ja, "unlock_event_id": e.unlock_event_id}
            for e in entries
        ]
    }
    save_json(path, data)

# ストーリー進行
def load_story_progression(project_root: str) -> list[dict]:
    path = story_progression_path(project_root)
    data = load_json(path)
    return data.get("chapters", [])

def save_story_progression(project_root: str, chapters: list[dict]) -> None:
    path = story_progression_path(project_root)
    data = {"chapters": chapters}
    save_json(path, data)

# 解禁ルール
def unlock_rules_path(project_root: str) -> str:
    return os.path.join(fixdata_root(project_root), "unlock_rules.json")

def load_unlock_rules(project_root: str) -> list[UnlockRule]:
    path = unlock_rules_path(project_root)
    data = load_json(path)
    return [UnlockRule(event_id=r.get("event_id",""), flag_id=int(r.get("flag_id", -1))) for r in data.get("rules", [])]

def save_unlock_rules(project_root: str, rules: list[UnlockRule]) -> None:
    path = unlock_rules_path(project_root)
    data = {"version": 1, "rules": [{"event_id": r.event_id, "flag_id": r.flag_id} for r in rules]}
    save_json(path, data)