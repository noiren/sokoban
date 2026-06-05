#!/usr/bin/env python3
"""
Gallery Editor - ギャラリーデータ編集ツール
各アイテムの表示名（ja）と解禁条件（unlock_event_id）を設定する。
"""
import os
import tkinter as tk
from tkinter import ttk, messagebox
import fix_data_io
import audio_editor_shell as shell
import copy
import dataclasses

SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))
PROJECT_ROOT = os.path.dirname(os.path.dirname(SCRIPT_DIR))

CATEGORIES = ["tachi-e", "still", "event", "bgm", "se"]
CAT_LABELS = {
    "tachi-e": "立ち絵",
    "still":   "スチル",
    "event":   "イベントセレクト",
    "bgm":     "BGM",
    "se":      "SE",
}

class GalleryEditorApp:
    def __init__(self, root: tk.Tk):
        self.root = root
        self.root.title("Gallery Editor")
        self.root.minsize(720, 480)

        # Load gallery data
        self.entries = fix_data_io.load_gallery(PROJECT_ROOT)
        self.original_entries = copy.deepcopy(self.entries)

        # Collect all available event IDs for unlock dropdowns
        self.all_event_ids: list[str] = ["(常時解禁)"]
        for e in fix_data_io.load_all_events(PROJECT_ROOT):
            self.all_event_ids.append(e.id)
        for e in fix_data_io.load_all_still_events(PROJECT_ROOT):
            self.all_event_ids.append(e.id)

        # Master IDs
        tachi_e_ids = []
        for c in fix_data_io.load_characters(PROJECT_ROOT):
            for fid, img in c.faces.items():
                if img:
                    tachi_e_ids.append(img)
        self.master_ids = {
            "tachi-e": sorted(list(set(tachi_e_ids))),
            "still":   fix_data_io.scan_still_resources(PROJECT_ROOT),
        }
        bgms, ses = fix_data_io.get_audio_ids(PROJECT_ROOT)
        self.master_ids["bgm"] = bgms
        self.master_ids["se"]  = ses
        # event: all event IDs (excluding "(常時解禁)")
        self.master_ids["event"] = [e for e in self.all_event_ids if e != "(常時解禁)"]

        self._setup_ui()
        self._refresh_all_tabs()

    # ------------------------------------------------------------------
    # UI 構築
    # ------------------------------------------------------------------
    def _setup_ui(self):
        main_frame = ttk.Frame(self.root, padding=8)
        main_frame.pack(fill=tk.BOTH, expand=True)

        self.notebook = ttk.Notebook(main_frame)
        self.notebook.pack(fill=tk.BOTH, expand=True, pady=(0, 8))

        self.tabs: dict[str, ttk.Treeview] = {}
        for cat in CATEGORIES:
            frame = ttk.Frame(self.notebook)
            self.notebook.add(frame, text=CAT_LABELS[cat])

            cols = ("res", "ja", "unlock")
            tree = ttk.Treeview(frame, columns=cols, show="headings", selectmode="browse")
            tree.heading("res",    text="Resource ID")
            tree.heading("ja",     text="ギャラリー名")
            tree.heading("unlock", text="解禁条件イベントID")
            tree.column("res",    width=200)
            tree.column("ja",     width=180)
            tree.column("unlock", width=260)
            tree.pack(fill=tk.BOTH, expand=True)
            tree.bind("<Double-1>", lambda e, c=cat: self._open_edit_dialog(c))
            self.tabs[cat] = tree

        bf = ttk.Frame(main_frame)
        bf.pack(fill=tk.X)
        ttk.Button(bf, text="Sync Master IDs", command=self._sync_masters).pack(side=tk.LEFT, padx=2)
        ttk.Button(bf, text="Edit Selected",   command=self._edit_current_tab).pack(side=tk.LEFT, padx=2)
        ttk.Button(bf, text="Save All",        command=self._save_all).pack(side=tk.RIGHT, padx=2)

    # ------------------------------------------------------------------
    # データ操作
    # ------------------------------------------------------------------
    def _get_entries_by_cat(self, cat: str) -> list[fix_data_io.GalleryEntry]:
        return [e for e in self.entries if e.category == cat]

    def _refresh_all_tabs(self):
        for cat, tree in self.tabs.items():
            for item in tree.get_children():
                tree.delete(item)
            for i, e in enumerate(self.entries):
                if e.category == cat:
                    unlock_label = e.unlock_event_id if e.unlock_event_id else "（常時解禁）"
                    tree.insert("", tk.END, iid=str(i), values=(e.resource_id, e.ja, unlock_label))

    def _sync_masters(self):
        """Add missing IDs from master files. Never deletes existing entries."""
        added = 0
        for cat in CATEGORIES:
            m_ids = self.master_ids.get(cat, [])
            existing_ids = {e.resource_id for e in self._get_entries_by_cat(cat)}
            for mid in m_ids:
                if mid not in existing_ids:
                    self.entries.append(fix_data_io.GalleryEntry(
                        category=cat, resource_id=mid, ja="未設定", unlock_event_id=""
                    ))
                    added += 1
        self._refresh_all_tabs()
        if added > 0:
            messagebox.showinfo("Sync", f"{added} 件の新しいIDをギャラリーに追加しました。")
        else:
            messagebox.showinfo("Sync", "ギャラリーはすでに最新です。")

    # ------------------------------------------------------------------
    # 編集ダイアログ
    # ------------------------------------------------------------------
    def _edit_current_tab(self):
        cat = CATEGORIES[self.notebook.index(self.notebook.select())]
        self._open_edit_dialog(cat)

    def _open_edit_dialog(self, cat: str):
        tree = self.tabs[cat]
        sel = tree.selection()
        if not sel:
            return
        idx = int(sel[0])
        e = self.entries[idx]

        dlg = tk.Toplevel(self.root)
        dlg.title(f"編集: {e.resource_id}")
        dlg.resizable(False, False)
        dlg.grab_set()
        dlg.transient(self.root)

        frame = ttk.Frame(dlg, padding=12)
        frame.pack(fill=tk.BOTH, expand=True)

        # ギャラリー名
        ttk.Label(frame, text="ギャラリー名：").grid(row=0, column=0, sticky="w", pady=4)
        ja_var = tk.StringVar(value=e.ja)
        ttk.Entry(frame, textvariable=ja_var, width=30).grid(row=0, column=1, sticky="ew", pady=4, padx=(8, 0))

        # 解禁条件
        ttk.Label(frame, text="解禁条件：").grid(row=1, column=0, sticky="w", pady=4)
        # ドロップダウンに "（常時解禁）" + 全イベントID を表示
        combo_values = self.all_event_ids
        current_val = e.unlock_event_id if e.unlock_event_id else "(常時解禁)"
        unlock_var = tk.StringVar(value=current_val)
        combo = ttk.Combobox(frame, textvariable=unlock_var, values=combo_values, width=30, state="readonly")
        combo.grid(row=1, column=1, sticky="ew", pady=4, padx=(8, 0))

        frame.columnconfigure(1, weight=1)

        # ボタン
        btn_frame = ttk.Frame(dlg)
        btn_frame.pack(fill=tk.X, padx=12, pady=(0, 12))

        def on_ok():
            e.ja = ja_var.get()
            chosen = unlock_var.get()
            e.unlock_event_id = "" if chosen == "(常時解禁)" else chosen
            self._refresh_all_tabs()
            tree.selection_set(str(idx))
            dlg.destroy()

        ttk.Button(btn_frame, text="OK",     command=on_ok).pack(side=tk.RIGHT, padx=4)
        ttk.Button(btn_frame, text="Cancel", command=dlg.destroy).pack(side=tk.RIGHT)
        dlg.bind("<Return>", lambda _: on_ok())

    # ------------------------------------------------------------------
    # 保存・終了
    # ------------------------------------------------------------------
    def _is_dirty(self):
        return shell.snapshot_json([dataclasses.asdict(e) for e in self.entries]) != \
               shell.snapshot_json([dataclasses.asdict(e) for e in self.original_entries])

    def _save_all(self):
        fix_data_io.save_gallery(PROJECT_ROOT, self.entries)
        self.original_entries = copy.deepcopy(self.entries)
        messagebox.showinfo("Saved", "ギャラリーデータを保存しました。\nmake を実行してゲームに反映してください。")

    def on_closing(self):
        if self._is_dirty():
            res = shell.close_unsaved_dialog(self.root)
            if res == "cancel":
                return
            if res == "save":
                self._save_all()
        self.root.destroy()


if __name__ == "__main__":
    root = tk.Tk()
    app = GalleryEditorApp(root)
    root.protocol("WM_DELETE_WINDOW", app.on_closing)
    root.mainloop()