#!/usr/bin/env python3
"""Best-effort audio preview via ffplay (ffmpeg)."""

from __future__ import annotations

import os
import shutil
import subprocess
import sys

_preview_proc: subprocess.Popen | None = None


def _find_ffplay() -> str | None:
    return shutil.which("ffplay")


def stop_preview() -> None:
    """Terminate the ffplay instance started by play_file(), if still running."""
    global _preview_proc
    proc = _preview_proc
    _preview_proc = None
    if proc is None:
        return
    try:
        if proc.poll() is None:
            proc.terminate()
            try:
                proc.wait(timeout=2.0)
            except subprocess.TimeoutExpired:
                proc.kill()
    except OSError:
        pass


def play_file(path: str) -> bool:
    stop_preview()
    if not os.path.isfile(path):
        print(f"[preview] missing file: {path}", file=sys.stderr)
        return False
    ffplay = _find_ffplay()
    if not ffplay:
        print(
            "[preview] ffplay not found (install ffmpeg). Cannot preview.",
            file=sys.stderr,
        )
        return False
    global _preview_proc
    try:
        _preview_proc = subprocess.Popen(
            [
                ffplay,
                "-nodisp",
                "-autoexit",
                "-loglevel",
                "quiet",
                path,
            ],
            stdout=subprocess.DEVNULL,
            stderr=subprocess.DEVNULL,
        )
        return True
    except OSError as e:
        print(f"[preview] failed to start ffplay: {e}", file=sys.stderr)
        _preview_proc = None
        return False
