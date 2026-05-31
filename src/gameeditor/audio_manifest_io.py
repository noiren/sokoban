#!/usr/bin/env python3
"""
Shared JSON schema + validation for Asset/audio/audio_manifest.json
"""

from __future__ import annotations

import copy
import json
import os
import re
from dataclasses import dataclass
from typing import Any

MANIFEST_VERSION = 2
MANIFEST_VERSION_LEGACY = 1

BGM_EXTENSIONS = (".mod", ".s3m", ".xm", ".it")
SE_EXTENSIONS = (".wav",)

_ID_RE = re.compile(r"^[A-Za-z_][A-Za-z0-9_]*$")
_RESERVED = frozenset({"COUNT"})


@dataclass
class AudioEntry:
    id: str
    file: str
    display_name: str


@dataclass
class SeAudioEntry:
    category: str
    number: int
    id: str
    file: str
    display_name: str


class ManifestError(ValueError):
    pass


def manifest_path(project_root: str) -> str:
    return os.path.join(project_root, "Asset", "audio", "audio_manifest.json")


def sanitize_cpp_token(raw: str) -> str:
    """Alphanumeric + underscore; never empty."""
    s = re.sub(r"[^A-Za-z0-9_]", "_", raw.strip())
    if not s:
        return "_"
    if s[0].isdigit():
        s = "_" + s
    return s


def se_cpp_enum_name(category: str, entry_id: str) -> str:
    """Globally unique C++ enumerator for SE (single _). Duplicate tokens rejected in parse_entries."""
    return f"{sanitize_cpp_token(category)}_{sanitize_cpp_token(entry_id)}"


def load_manifest(path: str) -> dict[str, Any]:
    if not os.path.isfile(path):
        raise ManifestError(f"Missing manifest: {path}")
    with open(path, encoding="utf-8") as f:
        data = json.load(f)
    if not isinstance(data, dict):
        raise ManifestError("Root must be an object")
    ver = data.get("version", MANIFEST_VERSION_LEGACY)
    if ver not in (MANIFEST_VERSION_LEGACY, MANIFEST_VERSION):
        raise ManifestError(f"Unsupported version {ver!r}")
    return migrate_manifest_to_v2(data)


def migrate_manifest_to_v2(data: dict[str, Any]) -> dict[str, Any]:
    """Return a deep copy normalized to version 2."""
    out = copy.deepcopy(data)
    ver = out.get("version", MANIFEST_VERSION_LEGACY)
    if ver == MANIFEST_VERSION_LEGACY:
        out["version"] = MANIFEST_VERSION
        cats = out.get("se_categories")
        if not cats:
            out["se_categories"] = ["Default"]
        se_rows = out.get("se", [])
        if not isinstance(se_rows, list):
            raise ManifestError("se must be an array")
        for i, row in enumerate(se_rows):
            if not isinstance(row, dict):
                raise ManifestError(f"se[{i}] must be object")
            if "category" not in row:
                row["category"] = "Default"
            if "number" not in row:
                row["number"] = i
        return out
    if ver == MANIFEST_VERSION:
        se_rows = out.get("se", [])
        if not isinstance(se_rows, list):
            raise ManifestError("se must be an array")
        for i, row in enumerate(se_rows):
            if not isinstance(row, dict):
                raise ManifestError(f"se[{i}] must be object")
            if "category" not in row or "number" not in row:
                raise ManifestError(
                    f"se[{i}]: version 2 requires category and number on each row"
                )
        return out
    raise ManifestError(f"Unsupported version {ver!r}")


def _validate_id(raw: str, kind: str) -> None:
    if not raw or not isinstance(raw, str):
        raise ManifestError(f"{kind}: invalid id")
    if raw in _RESERVED:
        raise ManifestError(f"{kind}: id {raw!r} is reserved")
    if not _ID_RE.match(raw):
        raise ManifestError(
            f"{kind}: id {raw!r} must be a valid C++ enumerator fragment (letters, digits, underscore)"
        )


def _validate_category_key(raw: str, kind: str) -> None:
    if not raw or not isinstance(raw, str):
        raise ManifestError(f"{kind}: invalid category")
    if raw in _RESERVED:
        raise ManifestError(f"{kind}: category {raw!r} is reserved")
    if not _ID_RE.match(raw):
        raise ManifestError(
            f"{kind}: category {raw!r} must match [A-Za-z_][A-Za-z0-9_]*"
        )


def _stem_matches_file(file_rel: str, kind: str, allowed_ext: tuple[str, ...]) -> str:
    if not file_rel or not isinstance(file_rel, str):
        raise ManifestError(f"{kind}: missing file path")
    norm = file_rel.replace("\\", "/").strip("/")
    base = os.path.basename(norm)
    stem, ext = os.path.splitext(base)
    ext_l = ext.lower()
    if ext_l not in allowed_ext:
        raise ManifestError(
            f"{kind}: {file_rel!r} must end with one of {allowed_ext}"
        )
    if not stem:
        raise ManifestError(f"{kind}: empty stem for {file_rel!r}")
    symbol = stem.lower()
    if symbol != stem:
        raise ManifestError(
            f"{kind}: file stem {stem!r} must be lowercase to match bn:: *_items::{symbol}"
        )
    return symbol


def _parse_se_categories(data: dict[str, Any]) -> list[str]:
    raw = data.get("se_categories", [])
    if not isinstance(raw, list) or not raw:
        raise ManifestError("se_categories must be a non-empty array")
    out: list[str] = []
    seen: set[str] = set()
    for i, c in enumerate(raw):
        if not isinstance(c, str):
            raise ManifestError(f"se_categories[{i}]: must be string")
        _validate_category_key(c, f"se_categories[{i}]")
        if c in seen:
            raise ManifestError(f"se_categories: duplicate {c!r}")
        seen.add(c)
        out.append(c)
    return out


def parse_entries(
    project_root: str, data: dict[str, Any], check_files_exist: bool
) -> tuple[list[AudioEntry], list[str], list[SeAudioEntry]]:
    data = migrate_manifest_to_v2(data)
    audio_root = os.path.join(project_root, "Asset", "audio")
    bgm_raw = data.get("bgm", [])
    se_raw = data.get("se", [])
    if not isinstance(bgm_raw, list) or not isinstance(se_raw, list):
        raise ManifestError("bgm and se must be arrays")

    se_categories = _parse_se_categories(data)
    cat_set = set(se_categories)

    def parse_bgm_list(raw_list: list[Any]) -> list[AudioEntry]:
        seen: set[str] = set()
        out: list[AudioEntry] = []
        for i, item in enumerate(raw_list):
            if not isinstance(item, dict):
                raise ManifestError(f"bgm[{i}]: must be object")
            eid = item.get("id", "")
            file_rel = item.get("file", "")
            display = item.get("display_name", eid)
            _validate_id(eid, f"bgm[{i}]")
            _stem_matches_file(file_rel, f"bgm[{i}]", BGM_EXTENSIONS)
            if eid in seen:
                raise ManifestError(f"bgm: duplicate id {eid!r}")
            seen.add(eid)
            if check_files_exist:
                abs_path = os.path.normpath(os.path.join(audio_root, file_rel))
                if not abs_path.startswith(os.path.normpath(audio_root) + os.sep):
                    raise ManifestError(f"bgm[{i}]: path escapes Asset/audio")
                if not os.path.isfile(abs_path):
                    raise ManifestError(f"bgm[{i}]: file not found: {file_rel}")
            out.append(
                AudioEntry(
                    id=eid,
                    file=file_rel.replace("\\", "/"),
                    display_name=str(display) if display is not None else eid,
                )
            )
        return out

    def parse_se_list(raw_list: list[Any]) -> list[SeAudioEntry]:
        seen_num: set[tuple[str, int]] = set()
        seen_id: set[tuple[str, str]] = set()
        seen_cpp: set[str] = set()
        out: list[SeAudioEntry] = []
        for i, item in enumerate(raw_list):
            if not isinstance(item, dict):
                raise ManifestError(f"se[{i}]: must be object")
            eid = item.get("id", "")
            file_rel = item.get("file", "")
            display = item.get("display_name", eid)
            cat = item.get("category", "")
            num_raw = item.get("number", None)

            _validate_id(eid, f"se[{i}]")
            if cat not in cat_set:
                raise ManifestError(
                    f"se[{i}]: unknown category {cat!r} (not in se_categories)"
                )
            if num_raw is None:
                raise ManifestError(f"se[{i}]: missing number")
            if not isinstance(num_raw, int) or isinstance(num_raw, bool):
                raise ManifestError(f"se[{i}]: number must be an integer")
            if num_raw < 0:
                raise ManifestError(f"se[{i}]: number must be >= 0")

            _stem_matches_file(file_rel, f"se[{i}]", SE_EXTENSIONS)

            key_num = (cat, num_raw)
            if key_num in seen_num:
                raise ManifestError(
                    f"se: duplicate (category, number) ({cat!r}, {num_raw})"
                )
            seen_num.add(key_num)

            key_id = (cat, eid)
            if key_id in seen_id:
                raise ManifestError(
                    f"se: duplicate (category, id) ({cat!r}, {eid!r})"
                )
            seen_id.add(key_id)

            cpp_name = se_cpp_enum_name(cat, eid)
            if cpp_name in seen_cpp:
                raise ManifestError(
                    f"se: duplicate generated enum name {cpp_name!r}"
                )
            seen_cpp.add(cpp_name)

            if check_files_exist:
                abs_path = os.path.normpath(os.path.join(audio_root, file_rel))
                if not abs_path.startswith(os.path.normpath(audio_root) + os.sep):
                    raise ManifestError(f"se[{i}]: path escapes Asset/audio")
                if not os.path.isfile(abs_path):
                    raise ManifestError(f"se[{i}]: file not found: {file_rel}")

            out.append(
                SeAudioEntry(
                    category=cat,
                    number=num_raw,
                    id=eid,
                    file=file_rel.replace("\\", "/"),
                    display_name=str(display) if display is not None else eid,
                )
            )
        return out

    bgm = parse_bgm_list(bgm_raw)
    se = parse_se_list(se_raw)
    return bgm, se_categories, se


def validate_manifest(
    project_root: str, check_files_exist: bool = True
) -> tuple[list[AudioEntry], list[str], list[SeAudioEntry]]:
    path = manifest_path(project_root)
    data = load_manifest(path)
    return parse_entries(project_root, data, check_files_exist)


def save_manifest_v2(
    project_root: str,
    bgm: list[AudioEntry],
    se_categories: list[str],
    se: list[SeAudioEntry],
) -> None:
    path = manifest_path(project_root)
    os.makedirs(os.path.dirname(path), exist_ok=True)
    if not se_categories:
        raise ManifestError("se_categories must be non-empty")
    payload = {
        "version": MANIFEST_VERSION,
        "se_categories": list(se_categories),
        "bgm": [
            {"id": e.id, "file": e.file, "display_name": e.display_name} for e in bgm
        ],
        "se": [
            {
                "category": e.category,
                "number": e.number,
                "id": e.id,
                "file": e.file,
                "display_name": e.display_name,
            }
            for e in se
        ],
    }
    with open(path, "w", encoding="utf-8") as f:
        json.dump(payload, f, indent=2, ensure_ascii=False)
        f.write("\n")


def manifest_snapshot_dict(
    bgm: list[AudioEntry],
    se_categories: list[str],
    se: list[SeAudioEntry],
) -> dict[str, Any]:
    """Normalized dict for dirty-check / compare."""
    return {
        "version": MANIFEST_VERSION,
        "se_categories": list(se_categories),
        "bgm": [
            {"id": e.id, "file": e.file, "display_name": e.display_name} for e in bgm
        ],
        "se": [
            {
                "category": e.category,
                "number": e.number,
                "id": e.id,
                "file": e.file,
                "display_name": e.display_name,
            }
            for e in se
        ],
    }


def format_validation_errors(errors: list[str]) -> str:
    return "\n".join(errors) if errors else ""


def validate_rows_errors(
    project_root: str,
    bgm: list[AudioEntry],
    se_categories: list[str],
    se: list[SeAudioEntry],
    check_files_exist: bool,
) -> list[str]:
    """Collect human-readable errors (no exception)."""
    errs: list[str] = []
    try:
        tmp = {
            "version": MANIFEST_VERSION,
            "se_categories": se_categories,
            "bgm": [
                {"id": e.id, "file": e.file, "display_name": e.display_name}
                for e in bgm
            ],
            "se": [
                {
                    "category": e.category,
                    "number": e.number,
                    "id": e.id,
                    "file": e.file,
                    "display_name": e.display_name,
                }
                for e in se
            ],
        }
        parse_entries(project_root, tmp, check_files_exist=check_files_exist)
    except ManifestError as e:
        errs.append(str(e))
    return errs


def category_in_use(se: list[SeAudioEntry], category: str) -> bool:
    return any(row.category == category for row in se)

