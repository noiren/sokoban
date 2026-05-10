#!/usr/bin/env python3
import os
import tkinter as tk
from tkinter import ttk, messagebox
import fix_data_io
import audio_editor_shell as shell
import copy
import dataclasses

SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))
PROJECT_ROOT = os.path.dirname(os.path.dirname(SCRIPT_DIR))

class GalleryEditorApp:
    def __init__(self, root: tk.Tk):
        self.root = root
        self.root.title("Gallery Editor")
        
        # Load unified gallery data
        self.entries = fix_data_io.load_gallery(PROJECT_ROOT)
        self.original_entries = copy.deepcopy(self.entries)
        
        # Master IDs to sync with
        # Master IDs to sync with
        tachi_e_ids = []
        for c in fix_data_io.load_characters(PROJECT_ROOT):
            for fid, img in c.faces.items():
                if img:
                    tachi_e_ids.append(img)
        
        self.master_ids = {
            "tachi-e": sorted(list(set(tachi_e_ids))),
            "still": fix_data_io.scan_still_resources(PROJECT_ROOT)
        }
        bgms, ses = fix_data_io.get_audio_ids(PROJECT_ROOT)
        self.master_ids["bgm"] = bgms
        self.master_ids["se"] = ses

        self._setup_ui()
        self._refresh_all_tabs()

    def _setup_ui(self):
        main_frame = ttk.Frame(self.root, padding=8)
        main_frame.pack(fill=tk.BOTH, expand=True)

        self.notebook = ttk.Notebook(main_frame)
        self.notebook.pack(fill=tk.BOTH, expand=True, pady=(0, 8))

        self.tabs = {}
        for cat in ["tachi-e", "still", "bgm", "se"]:
            frame = ttk.Frame(self.notebook)
            self.notebook.add(frame, text=cat.upper())
            
            tree = ttk.Treeview(frame, columns=("res", "ja"), show="headings", selectmode="browse")
            tree.heading("res", text="Resource ID")
            tree.heading("ja", text="Gallery Title (JA)")
            tree.pack(fill=tk.BOTH, expand=True)
            
            # Double-click to edit
            tree.bind("<Double-1>", lambda e, c=cat: self._edit_entry(c))
            
            self.tabs[cat] = tree

        bf = ttk.Frame(main_frame)
        bf.pack(fill=tk.X)
        ttk.Button(bf, text="Sync Master IDs", command=self._sync_masters).pack(side=tk.LEFT)
        ttk.Button(bf, text="Edit Selected Title", command=self._edit_current_tab).pack(side=tk.LEFT)
        ttk.Button(bf, text="Save All", command=self._save_all).pack(side=tk.RIGHT)

    def _get_entries_by_cat(self, cat: str) -> list[fix_data_io.GalleryEntry]:
        return [e for e in self.entries if e.category == cat]

    def _refresh_all_tabs(self):
        for cat, tree in self.tabs.items():
            for item in tree.get_children():
                tree.delete(item)
            for i, e in enumerate(self.entries):
                if e.category == cat:
                    tree.insert("", tk.END, iid=str(i), values=(e.resource_id, e.ja))

    def _sync_masters(self):
        """Add missing IDs from master files. Never deletes existing names."""
        added = 0
        for cat, m_ids in self.master_ids.items():
            existing_ids = {e.resource_id for e in self._get_entries_by_cat(cat)}
            for mid in m_ids:
                if mid not in existing_ids:
                    self.entries.append(fix_data_io.GalleryEntry(category=cat, resource_id=mid, ja="未設定"))
                    added += 1
        
        self._refresh_all_tabs()
        if added > 0:
            messagebox.showinfo("Sync", f"Added {added} new resource IDs to gallery.")
        else:
            messagebox.showinfo("Sync", "Gallery is already up to date with master files.")

    def _edit_current_tab(self):
        current_tab_idx = self.notebook.index(self.notebook.select())
        cat = ["tachi-e", "still", "bgm", "se"][current_tab_idx]
        self._edit_entry(cat)

    def _edit_entry(self, cat: str):
        tree = self.tabs[cat]
        sel = tree.selection()
        if not sel: return
        idx = int(sel[0])
        e = self.entries[idx]
        
        ja = shell.ask_string(self.root, "Edit Title", f"Enter Gallery Title for '{e.resource_id}':", e.ja)
        if ja is not None:
            e.ja = ja
            self._refresh_all_tabs()
            tree.selection_set(str(idx))

    def _is_dirty(self):
        return shell.snapshot_json([dataclasses.asdict(e) for e in self.entries]) != \
               shell.snapshot_json([dataclasses.asdict(e) for e in self.original_entries])

    def _save_all(self):
        fix_data_io.save_gallery(PROJECT_ROOT, self.entries)
        self.original_entries = copy.deepcopy(self.entries)
        messagebox.showinfo("Saved", "Gallery saved successfully.")

    def on_closing(self):
        if self._is_dirty():
            res = shell.close_unsaved_dialog(self.root)
            if res == "cancel": return
            if res == "save": self._save_all()
        self.root.destroy()

if __name__ == "__main__":
    root = tk.Tk()
    app = GalleryEditorApp(root)
    root.protocol("WM_DELETE_WINDOW", app.on_closing)
    root.mainloop()