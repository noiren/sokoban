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

class TextEditorApp:
    def __init__(self, root: tk.Tk):
        self.root = root
        self.root.title("Text Editor")
        self.files = fix_data_io.load_text_files(PROJECT_ROOT)
        self.original_files = copy.deepcopy(self.files)
        self.current_file_idx = None
        self._setup_ui()
        self._refresh_file_list()

    def _setup_ui(self):
        paned = ttk.PanedWindow(self.root, orient=tk.HORIZONTAL)
        paned.pack(fill=tk.BOTH, expand=True, padx=8, pady=8)

        # Left: Files
        l_frame = ttk.Frame(paned)
        paned.add(l_frame, weight=1)
        ttk.Label(l_frame, text="Files").pack(anchor="w")
        
        self.file_tree = ttk.Treeview(l_frame, columns=("fname", "cat"), show="headings", selectmode="browse")
        self.file_tree.heading("fname", text="File")
        self.file_tree.heading("cat", text="Category")
        self.file_tree.pack(fill=tk.BOTH, expand=True)
        self.file_tree.bind("<<TreeviewSelect>>", self._on_file_select)
        
        bf1 = ttk.Frame(l_frame)
        bf1.pack(fill=tk.X, pady=4)
        ttk.Button(bf1, text="New File", command=self._add_file).pack(side=tk.LEFT)
        ttk.Button(bf1, text="Save All", command=self._save_all).pack(side=tk.RIGHT)

        # Right: Entries
        r_frame = ttk.Frame(paned)
        paned.add(r_frame, weight=3)
        self.lbl_entries = ttk.Label(r_frame, text="Entries: None Selected")
        self.lbl_entries.pack(anchor="w")
        
        self.entry_tree = ttk.Treeview(r_frame, columns=("id", "ja"), show="headings", selectmode="browse")
        self.entry_tree.heading("id", text="ID")
        self.entry_tree.heading("ja", text="Text (JA)")
        self.entry_tree.pack(fill=tk.BOTH, expand=True)
        
        bf2 = ttk.Frame(r_frame)
        bf2.pack(fill=tk.X, pady=4)
        ttk.Button(bf2, text="Add", command=self._add_entry).pack(side=tk.LEFT)
        ttk.Button(bf2, text="Edit", command=self._edit_entry).pack(side=tk.LEFT)
        ttk.Button(bf2, text="Delete", command=self._del_entry).pack(side=tk.LEFT)

    def _refresh_file_list(self):
        for item in self.file_tree.get_children():
            self.file_tree.delete(item)
        for i, (fname, cat, entries) in enumerate(self.files):
            self.file_tree.insert("", tk.END, iid=str(i), values=(fname, cat))

    def _on_file_select(self, event):
        sel = self.file_tree.selection()
        if sel:
            self.current_file_idx = int(sel[0])
            self.lbl_entries.config(text=f"Entries in {self.files[self.current_file_idx][0]}")
        else:
            self.current_file_idx = None
            self.lbl_entries.config(text="Entries: None Selected")
        self._refresh_entry_list()

    def _refresh_entry_list(self):
        for item in self.entry_tree.get_children():
            self.entry_tree.delete(item)
        if self.current_file_idx is not None:
            _, _, entries = self.files[self.current_file_idx]
            for i, e in enumerate(entries):
                self.entry_tree.insert("", tk.END, iid=str(i), values=(e.id, e.ja))

    def _add_file(self):
        fname = shell.ask_string(self.root, "New File", "Enter File Name (e.g. text_system.json):")
        if not fname: return
        if not fname.endswith(".json"): fname += ".json"
        cat = shell.ask_string(self.root, "New File", "Enter Category:")
        if not cat: return
        self.files.append((fname, cat, []))
        self._refresh_file_list()

    def _add_entry(self):
        if self.current_file_idx is None: return
        tid = shell.ask_string(self.root, "Add", "Enter Text ID:")
        if not tid: return
        tja = shell.ask_string(self.root, "Add", "Enter Text (JA):")
        if not tja: return
        cat = self.files[self.current_file_idx][1]
        self.files[self.current_file_idx][2].append(fix_data_io.TextEntry(id=tid, category=cat, ja=tja))
        self._refresh_entry_list()

    def _edit_entry(self):
        sel = self.entry_tree.selection()
        if not sel or self.current_file_idx is None: return
        idx = int(sel[0])
        e = self.files[self.current_file_idx][2][idx]
        tja = shell.ask_string(self.root, "Edit", "Enter Text (JA):", e.ja)
        if tja is not None:
            e.ja = tja
            self._refresh_entry_list()

    def _del_entry(self):
        sel = self.entry_tree.selection()
        if not sel or self.current_file_idx is None: return
        del self.files[self.current_file_idx][2][int(sel[0])]
        self._refresh_entry_list()

    def _is_dirty(self):
        def dump(f_list):
            return [(fname, cat, [dataclasses.asdict(e) for e in entries]) for fname, cat, entries in f_list]
        return shell.snapshot_json(dump(self.files)) != shell.snapshot_json(dump(self.original_files))

    def _save_all(self):
        for fname, cat, entries in self.files:
            fix_data_io.save_text_file(PROJECT_ROOT, fname, cat, entries)
        self.original_files = copy.deepcopy(self.files)
        messagebox.showinfo("Saved", "All texts saved successfully.")

    def on_closing(self):
        if self._is_dirty():
            res = shell.close_unsaved_dialog(self.root)
            if res == "cancel": return
            if res == "save": self._save_all()
        self.root.destroy()

if __name__ == "__main__":
    root = tk.Tk()
    app = TextEditorApp(root)
    root.protocol("WM_DELETE_WINDOW", app.on_closing)
    root.mainloop()