#!/usr/bin/env python3
"""Shared Tk helpers for audio manifest editors."""

from __future__ import annotations

import json
import tkinter as tk
from tkinter import messagebox, ttk
from typing import Any


def snapshot_json(data: dict[str, Any]) -> str:
    return json.dumps(data, sort_keys=True, ensure_ascii=False, indent=2)


def validation_error_dialog(parent: tk.Misc, errors: list[str]) -> None:
    if not errors:
        return
    msg = "\n".join(errors[:40])
    if len(errors) > 40:
        msg += f"\n… and {len(errors) - 40} more"
    messagebox.showerror("検証エラー", msg, parent=parent)


def close_unsaved_dialog(parent: tk.Misc) -> str:
    """Returns 'save', 'discard', or 'cancel'."""
    top = tk.Toplevel(parent)
    top.title("未保存の変更")
    top.transient(parent)
    top.grab_set()
    result: dict[str, str] = {"choice": "cancel"}

    ttk.Label(
        top,
        text="変更が保存されていません。どうしますか？",
        padding=8,
    ).pack(fill="x")

    bf = ttk.Frame(top, padding=8)
    bf.pack(fill="x")

    def pick(val: str) -> None:
        result["choice"] = val
        top.destroy()

    ttk.Button(bf, text="保存して閉じる", command=lambda: pick("save")).pack(
        side="left", padx=4
    )
    ttk.Button(bf, text="保存せず閉じる", command=lambda: pick("discard")).pack(
        side="left", padx=4
    )
    ttk.Button(bf, text="キャンセル", command=lambda: pick("cancel")).pack(
        side="left", padx=4
    )

    top.protocol("WM_DELETE_WINDOW", lambda: pick("cancel"))
    parent.wait_window(top)
    return result["choice"]


def tree_column_index(tree: ttk.Treeview, col_id: str) -> str:
    """Return Tk column identifier e.g. '#1' for logical column name."""
    cols = tree["columns"]
    try:
        idx = cols.index(col_id)
        return f"#{idx + 1}"
    except ValueError:
        return ""


def ask_string(parent: tk.Misc, title: str, prompt: str, initial: str = "") -> str | None:
    """Modal string edit; None if cancelled."""
    top = tk.Toplevel(parent)
    top.title(title)
    top.transient(parent)
    top.grab_set()
    out: dict[str, str | None] = {"v": None}

    ttk.Label(top, text=prompt, padding=6).pack(fill="x")
    var = tk.StringVar(value=initial)
    ent = ttk.Entry(top, textvariable=var, width=48)
    ent.pack(fill="x", padx=8, pady=4)
    ent.focus_set()
    ent.select_range(0, tk.END)

    bf = ttk.Frame(top, padding=8)
    bf.pack(fill="x")

    def ok() -> None:
        out["v"] = var.get()
        top.destroy()

    def cancel() -> None:
        out["v"] = None
        top.destroy()

    ttk.Button(bf, text="OK", command=ok).pack(side="left", padx=4)
    ttk.Button(bf, text="キャンセル", command=cancel).pack(side="left", padx=4)
    ent.bind("<Return>", lambda e: ok())
    ent.bind("<Escape>", lambda e: cancel())
    top.protocol("WM_DELETE_WINDOW", cancel)
    parent.wait_window(top)
    return out["v"]


def ask_int(parent: tk.Misc, title: str, prompt: str, initial: int) -> int | None:
    raw = ask_string(parent, title, prompt, str(initial))
    if raw is None:
        return None
    raw = raw.strip()
    try:
        v = int(raw)
    except ValueError:
        messagebox.showerror("入力エラー", "整数を入力してください。", parent=parent)
        return None
    if v < 0:
        messagebox.showerror("入力エラー", "0 以上の整数を入力してください。", parent=parent)
        return None
    return v


def ask_from_list(
    parent: tk.Misc, title: str, options: list[str], initial: str | None = None
) -> str | None:
    if not options:
        return None
    top = tk.Toplevel(parent)
    top.title(title)
    top.transient(parent)
    top.grab_set()
    out: dict[str, str | None] = {"v": None}

    lb = tk.Listbox(top, height=min(len(options), 12), width=32)
    lb.pack(fill="both", expand=True, padx=8, pady=4)
    for o in options:
        lb.insert(tk.END, o)
    if initial and initial in options:
        idx = options.index(initial)
        lb.selection_set(idx)
        lb.activate(idx)
    elif options:
        lb.selection_set(0)

    bf = ttk.Frame(top, padding=8)
    bf.pack(fill="x")

    def ok() -> None:
        sel = lb.curselection()
        if sel:
            out["v"] = options[int(sel[0])]
        top.destroy()

    def cancel() -> None:
        out["v"] = None
        top.destroy()

    ttk.Button(bf, text="OK", command=ok).pack(side="left", padx=4)
    ttk.Button(bf, text="キャンセル", command=cancel).pack(side="left", padx=4)
    lb.bind("<Double-Button-1>", lambda e: ok())
    top.protocol("WM_DELETE_WINDOW", cancel)
    parent.wait_window(top)
    return out["v"]
