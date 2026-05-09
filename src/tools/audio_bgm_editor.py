#!/usr/bin/env python3
"""Tkinter editor for BGM entries in Asset/audio/audio_manifest.json"""

from __future__ import annotations

import os
import sys
import tkinter as tk
from tkinter import filedialog, messagebox, ttk

SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))
if SCRIPT_DIR not in sys.path:
    sys.path.insert(0, SCRIPT_DIR)

import audio_editor_shell as shell
import audio_manifest_io as am
import audio_preview

PROJECT_ROOT = os.path.dirname(os.path.dirname(SCRIPT_DIR))

COL_PLAY = "play"
COL_STOP = "stop"
COL_TRASH = "trash"
COL_ID = "id"
COL_FILE = "file"
COL_PICK = "pick"
COL_DISP = "display"


class BgmEditorApp:
    def __init__(self) -> None:
        self.root = tk.Tk()
        self.root.title("GBA Sokoban — BGM manifest")
        self.audio_root = os.path.join(PROJECT_ROOT, "Asset", "audio")

        self.bgm: list[am.AudioEntry] = []
        self.se_categories: list[str] = []
        self.se: list[am.SeAudioEntry] = []
        self._baseline = ""

        tb = ttk.Frame(self.root)
        tb.pack(fill="x", padx=4, pady=4)
        ttk.Label(tb, text="BGM（マニフェストの bgm のみ編集）").pack(side="left")
        self.btn_plus = ttk.Button(tb, text="+", width=3, command=self.add_via_dialog)
        self.btn_plus.pack(side="right")

        cols = (COL_PLAY, COL_STOP, COL_TRASH, COL_ID, COL_FILE, COL_PICK, COL_DISP)
        self.tree = ttk.Treeview(
            self.root,
            columns=cols,
            show="headings",
            height=16,
            selectmode="browse",
        )
        self.tree.heading(COL_PLAY, text="▶")
        self.tree.heading(COL_STOP, text="■")
        self.tree.heading(COL_TRASH, text="🗑")
        self.tree.heading(COL_ID, text="id")
        self.tree.heading(COL_FILE, text="file")
        self.tree.heading(COL_PICK, text="…")
        self.tree.heading(COL_DISP, text="display_name")
        self.tree.column(COL_PLAY, width=28, anchor="center")
        self.tree.column(COL_STOP, width=28, anchor="center")
        self.tree.column(COL_TRASH, width=28, anchor="center")
        self.tree.column(COL_ID, width=90)
        self.tree.column(COL_FILE, width=220)
        self.tree.column(COL_PICK, width=28, anchor="center")
        self.tree.column(COL_DISP, width=160)

        vsb = ttk.Scrollbar(self.root, orient="vertical", command=self.tree.yview)
        self.tree.configure(yscrollcommand=vsb.set)
        self.tree.pack(side="left", fill="both", expand=True, padx=(4, 0), pady=4)
        vsb.pack(side="left", fill="y", pady=4)

        bf = ttk.Frame(self.root)
        bf.pack(fill="x", padx=4, pady=6)
        ttk.Button(bf, text="保存", command=self.save_clicked).pack(side="right")

        self.tree.bind("<ButtonRelease-1>", self.on_tree_click)
        self.tree.bind("<F2>", self.on_f2)

        self._last_col: str | None = None

        self.load_disk()
        self.root.protocol("WM_DELETE_WINDOW", self.on_close)
        self.refresh_tree()

    def snapshot(self) -> str:
        return shell.snapshot_json(
            am.manifest_snapshot_dict(self.bgm, self.se_categories, self.se)
        )

    def is_dirty(self) -> bool:
        return self.snapshot() != self._baseline

    def load_disk(self) -> None:
        path = am.manifest_path(PROJECT_ROOT)
        try:
            data = am.load_manifest(path)
            self.bgm, self.se_categories, self.se = am.parse_entries(
                PROJECT_ROOT, data, check_files_exist=False
            )
        except am.ManifestError as e:
            messagebox.showerror("読み込みエラー", str(e), parent=self.root)
            self.bgm = []
            self.se_categories = ["Default"]
            self.se = []
        self._baseline = self.snapshot()

    def refresh_tree(self) -> None:
        for iid in self.tree.get_children():
            self.tree.delete(iid)
        for i, e in enumerate(self.bgm):
            self.tree.insert(
                "",
                "end",
                iid=str(i),
                values=("▶", "■", "×", e.id, e.file, "…", e.display_name),
            )

    def row_idx(self, iid: str) -> int | None:
        try:
            return int(iid)
        except ValueError:
            return None

    def on_tree_click(self, event: tk.Event) -> None:
        row = self.tree.identify_row(event.y)
        col = self.tree.identify_column(event.x)
        self._last_col = col
        if not row:
            return
        self.tree.selection_set(row)
        idx = self.row_idx(row)
        if idx is None or idx < 0 or idx >= len(self.bgm):
            return
        e = self.bgm[idx]

        if col == shell.tree_column_index(self.tree, COL_PLAY):
            audio_preview.play_file(os.path.join(self.audio_root, e.file))
        elif col == shell.tree_column_index(self.tree, COL_STOP):
            audio_preview.stop_preview()
        elif col == shell.tree_column_index(self.tree, COL_TRASH):
            if messagebox.askyesno(
                "削除",
                f"この行を削除しますか？\n{e.id}",
                parent=self.root,
            ):
                del self.bgm[idx]
                self.refresh_tree()
        elif col == shell.tree_column_index(self.tree, COL_PICK):
            self.browse_row(idx)

    def browse_row(self, idx: int) -> None:
        path = filedialog.askopenfilename(
            title="音声ファイル",
            initialdir=self.audio_root,
            filetypes=[
                ("Tracker / module", "*.mod *.xm *.s3m *.it"),
                ("All", "*.*"),
            ],
        )
        if not path:
            return
        try:
            rel = os.path.relpath(path, self.audio_root).replace("\\", "/")
        except ValueError:
            messagebox.showerror("パスエラー", "Asset/audio 以下を選んでください。", parent=self.root)
            return
        stem = os.path.splitext(os.path.basename(rel))[0]
        if stem != stem.lower():
            messagebox.showwarning(
                "ファイル名",
                "ビルドでは stem は小文字である必要があります。",
                parent=self.root,
            )
        self.bgm[idx].file = rel
        self.refresh_tree()

    def on_f2(self, _evt: tk.Event | None = None) -> None:
        sel = self.tree.selection()
        if not sel:
            return
        idx = self.row_idx(sel[0])
        if idx is None:
            return
        e = self.bgm[idx]
        col = self._last_col or shell.tree_column_index(self.tree, COL_ID)

        if col == shell.tree_column_index(self.tree, COL_ID):
            v = shell.ask_string(self.root, "id", "C++ 用 id（英数字・_）", e.id)
            if v is not None:
                e.id = v.strip()
                self.refresh_tree()
        elif col == shell.tree_column_index(self.tree, COL_FILE):
            v = shell.ask_string(self.root, "file", "Asset/audio からの相対パス", e.file)
            if v is not None:
                e.file = v.strip().replace("\\", "/")
                self.refresh_tree()
        elif col == shell.tree_column_index(self.tree, COL_DISP):
            v = shell.ask_string(self.root, "display_name", "表示名", e.display_name)
            if v is not None:
                e.display_name = v.strip()
                self.refresh_tree()
        else:
            v = shell.ask_string(self.root, "id", "C++ 用 id（英数字・_）", e.id)
            if v is not None:
                e.id = v.strip()
                self.refresh_tree()

    def add_via_dialog(self) -> None:
        path = filedialog.askopenfilename(
            title="追加する BGM ファイル",
            initialdir=self.audio_root,
            filetypes=[
                ("Module", "*.mod *.xm *.s3m *.it"),
                ("All", "*.*"),
            ],
        )
        if not path:
            return
        try:
            rel = os.path.relpath(path, self.audio_root).replace("\\", "/")
        except ValueError:
            messagebox.showerror("パスエラー", "Asset/audio 以下を選んでください。", parent=self.root)
            return
        stem = os.path.splitext(os.path.basename(rel))[0]
        if stem != stem.lower():
            messagebox.showwarning(
                "ファイル名",
                "stem は小文字にしてください（リネーム推奨）。",
                parent=self.root,
            )
        cid = stem[0].upper() + stem[1:] if stem else "Bgm"
        cid = "".join(c if c.isalnum() or c == "_" else "_" for c in cid)
        if cid and cid[0].isdigit():
            cid = "_" + cid
        disp = stem
        self.bgm.append(am.AudioEntry(id=cid, file=rel, display_name=disp))
        self.refresh_tree()

    def save_clicked(self) -> None:
        errs = am.validate_rows_errors(
            PROJECT_ROOT,
            self.bgm,
            self.se_categories,
            self.se,
            check_files_exist=True,
        )
        if errs:
            shell.validation_error_dialog(self.root, errs)
            return
        try:
            am.save_manifest_v2(
                PROJECT_ROOT, self.bgm, self.se_categories, self.se
            )
        except OSError as exc:
            messagebox.showerror("保存エラー", str(exc), parent=self.root)
            return
        self._baseline = self.snapshot()
        messagebox.showinfo("保存", "audio_manifest.json を保存しました。", parent=self.root)

    def on_close(self) -> None:
        if not self.is_dirty():
            audio_preview.stop_preview()
            self.root.destroy()
            return
        choice = shell.close_unsaved_dialog(self.root)
        if choice == "cancel":
            return
        if choice == "discard":
            audio_preview.stop_preview()
            self.root.destroy()
            return
        errs = am.validate_rows_errors(
            PROJECT_ROOT,
            self.bgm,
            self.se_categories,
            self.se,
            check_files_exist=True,
        )
        if errs:
            shell.validation_error_dialog(self.root, errs)
            return
        try:
            am.save_manifest_v2(
                PROJECT_ROOT, self.bgm, self.se_categories, self.se
            )
        except OSError as exc:
            messagebox.showerror("保存エラー", str(exc), parent=self.root)
            return
        audio_preview.stop_preview()
        self.root.destroy()

    def run(self) -> None:
        self.root.mainloop()


def main() -> None:
    BgmEditorApp().run()


if __name__ == "__main__":
    main()
