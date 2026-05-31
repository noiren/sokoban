#!/usr/bin/env python3
import os
import tkinter as tk
from tkinter import ttk, messagebox
from PIL import Image, ImageTk
import fix_data_io
import audio_editor_shell as shell
import copy
import dataclasses

SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))
PROJECT_ROOT = os.path.dirname(os.path.dirname(SCRIPT_DIR))

class CharacterEditorApp:
    def __init__(self, root: tk.Tk):
        self.root = root
        self.root.title("Character Editor")
        self.characters = fix_data_io.load_characters(PROJECT_ROOT)
        self.original_data = copy.deepcopy(self.characters)
        self.current_char_idx = None
        
        self.preview_image = None
        self.preview_photo = None
        
        self._setup_ui()
        self._refresh_char_list()

    def _setup_ui(self):
        main_paned = ttk.PanedWindow(self.root, orient=tk.HORIZONTAL)
        main_paned.pack(fill=tk.BOTH, expand=True, padx=8, pady=8)

        # ---------------- Left: Character List ----------------
        left_frame = ttk.Frame(main_paned)
        main_paned.add(left_frame, weight=1)
        
        ttk.Label(left_frame, text="Characters").pack(anchor="w")
        self.char_tree = ttk.Treeview(left_frame, columns=("id", "name"), show="headings", selectmode="browse")
        self.char_tree.heading("id", text="ID")
        self.char_tree.heading("name", text="Name (JA)")
        self.char_tree.pack(fill=tk.BOTH, expand=True)
        self.char_tree.bind("<<TreeviewSelect>>", self._on_char_select)

        btn_frame1 = ttk.Frame(left_frame)
        btn_frame1.pack(fill=tk.X, pady=4)
        ttk.Button(btn_frame1, text="Add", command=self._add_char).pack(side=tk.LEFT)
        ttk.Button(btn_frame1, text="Edit", command=self._edit_char).pack(side=tk.LEFT)
        ttk.Button(btn_frame1, text="Delete", command=self._del_char).pack(side=tk.LEFT)

        # ---------------- Right: Faces & Preview ----------------
        right_frame = ttk.Frame(main_paned)
        main_paned.add(right_frame, weight=3)
        
        right_paned = ttk.PanedWindow(right_frame, orient=tk.HORIZONTAL)
        right_paned.pack(fill=tk.BOTH, expand=True)
        
        # Face List
        face_frame = ttk.Frame(right_paned)
        right_paned.add(face_frame, weight=1)
        self.lbl_face_title = ttk.Label(face_frame, text="Faces (Fixed 21 slots): None Selected")
        self.lbl_face_title.pack(anchor="w")
        
        self.face_tree = ttk.Treeview(face_frame, columns=("face_id", "image_id"), show="headings", selectmode="browse")
        self.face_tree.heading("face_id", text="Face ID")
        self.face_tree.heading("image_id", text="Image ID")
        self.face_tree.pack(fill=tk.BOTH, expand=True)
        self.face_tree.bind("<<TreeviewSelect>>", self._on_face_select)

        btn_frame2 = ttk.Frame(face_frame)
        btn_frame2.pack(fill=tk.X, pady=4)
        ttk.Button(btn_frame2, text="Set Image ID", command=self._edit_face).pack(side=tk.LEFT)
        ttk.Button(btn_frame2, text="Clear Image ID", command=self._clear_face).pack(side=tk.LEFT)

        # Preview Area
        prev_frame = ttk.LabelFrame(right_paned, text="Preview")
        right_paned.add(prev_frame, weight=1)
        self.lbl_preview = ttk.Label(prev_frame, text="No Image", anchor=tk.CENTER)
        self.lbl_preview.pack(fill=tk.BOTH, expand=True, padx=8, pady=8)

        # ---------------- Bottom: Save ----------------
        bottom_frame = ttk.Frame(self.root)
        bottom_frame.pack(fill=tk.X, padx=8, pady=8)
        ttk.Button(bottom_frame, text="Save All", command=self._save_all).pack(side=tk.RIGHT)

    def _refresh_char_list(self):
        for item in self.char_tree.get_children():
            self.char_tree.delete(item)
        for i, c in enumerate(self.characters):
            self.char_tree.insert("", tk.END, iid=str(i), values=(c.id, c.name_ja))
        self._refresh_face_list()

    def _on_char_select(self, event):
        sel = self.char_tree.selection()
        if sel:
            self.current_char_idx = int(sel[0])
            c = self.characters[self.current_char_idx]
            self.lbl_face_title.config(text=f"Faces: {c.id} ({c.name_ja})")
        else:
            self.current_char_idx = None
            self.lbl_face_title.config(text="Faces: None Selected")
        self._refresh_face_list()

    def _refresh_face_list(self):
        for item in self.face_tree.get_children():
            self.face_tree.delete(item)
        self._update_preview(None)
        
        if self.current_char_idx is not None:
            c = self.characters[self.current_char_idx]
            for fid in fix_data_io.FACE_IDS:
                img_id = c.faces.get(fid, "")
                self.face_tree.insert("", tk.END, iid=fid, values=(fid, img_id))

    def _on_face_select(self, event):
        sel = self.face_tree.selection()
        if not sel or self.current_char_idx is None: return
        fid = sel[0]
        img_id = self.characters[self.current_char_idx].faces.get(fid, "")
        self._update_preview(img_id)

    def _update_preview(self, image_id: str):
        if not image_id:
            self.lbl_preview.config(image='', text="No Image")
            self.preview_photo = None
            return

        path = fix_data_io.find_chara_sprite_path(PROJECT_ROOT, image_id)
        if not path:
            self.lbl_preview.config(image='', text=f"File not found for:\n{image_id}")
            self.preview_photo = None
            return

        try:
            img = Image.open(path)
            # Resize for preview (max 256x256) keeping aspect ratio
            img.thumbnail((256, 256), Image.Resampling.LANCZOS)
            self.preview_photo = ImageTk.PhotoImage(img)
            self.lbl_preview.config(image=self.preview_photo, text="")
        except Exception as e:
            self.lbl_preview.config(image='', text=f"Error loading image:\n{e}")
            self.preview_photo = None

    def _add_char(self):
        cid = shell.ask_string(self.root, "Add Character", "Enter Character ID:")
        if not cid: return
        name = shell.ask_string(self.root, "Add Character", "Enter Character Name (JA):")
        if name is None: return
        self.characters.append(fix_data_io.CharacterEntry(id=cid, name_ja=name, faces={}))
        self._refresh_char_list()

    def _edit_char(self):
        if self.current_char_idx is None: return
        c = self.characters[self.current_char_idx]
        new_name = shell.ask_string(self.root, "Edit Character", "Enter Character Name (JA):", c.name_ja)
        if new_name is not None:
            c.name_ja = new_name
            self._refresh_char_list()

    def _del_char(self):
        if self.current_char_idx is not None:
            del self.characters[self.current_char_idx]
            self.current_char_idx = None
            self._refresh_char_list()

    # ---------------- Image ID Selection Logic ----------------
    def _get_available_image_ids(self) -> list[str]:
        base_dir = fix_data_io.sprite_chara_dir(PROJECT_ROOT)
        if not os.path.exists(base_dir): return []
        res = set()
        for root, _, files in os.walk(base_dir):
            for f in files:
                if f.lower().endswith(".bmp"):
                    res.add(os.path.splitext(f)[0])
        return sorted(list(res))

    def _ask_image_id_combobox(self, title: str, options: list[str], initial: str) -> str:
        """Custom dialog with a Combobox for selecting/typing image IDs."""
        top = tk.Toplevel(self.root)
        top.title(title)
        top.transient(self.root)
        top.grab_set()
        out = {"v": None}
        
        ttk.Label(top, text="Select or type the Image ID:", padding=6).pack(fill="x")
        var = tk.StringVar(value=initial)
        cb = ttk.Combobox(top, textvariable=var, values=options)
        cb.pack(fill="x", padx=8, pady=4)
        cb.focus_set()
        cb.select_range(0, tk.END)
        
        bf = ttk.Frame(top, padding=8)
        bf.pack(fill="x")
        
        def ok(*args):
            out["v"] = var.get()
            top.destroy()
        def cancel(*args):
            out["v"] = None
            top.destroy()
            
        ttk.Button(bf, text="OK", command=ok).pack(side="left", padx=4)
        ttk.Button(bf, text="Cancel", command=cancel).pack(side="left", padx=4)
        
        cb.bind("<Return>", ok)
        cb.bind("<Escape>", cancel)
        top.protocol("WM_DELETE_WINDOW", cancel)
        self.root.wait_window(top)
        return out["v"]

    def _edit_face(self):
        sel = self.face_tree.selection()
        if not sel or self.current_char_idx is None: return
        fid = sel[0]
        c = self.characters[self.current_char_idx]
        current_img = c.faces.get(fid, "")
        
        options = self._get_available_image_ids()
        iid = self._ask_image_id_combobox("Select Image ID", options, current_img)
        
        if iid is not None:
            c.faces[fid] = iid
            self._refresh_face_list()
            self.face_tree.selection_set(fid)

    def _clear_face(self):
        sel = self.face_tree.selection()
        if not sel or self.current_char_idx is None: return
        fid = sel[0]
        c = self.characters[self.current_char_idx]
        c.faces[fid] = ""
        self._refresh_face_list()
        self.face_tree.selection_set(fid)

    # ---------------- Save & Exit ----------------
    def _is_dirty(self):
        return shell.snapshot_json([dataclasses.asdict(c) for c in self.characters]) != \
               shell.snapshot_json([dataclasses.asdict(c) for c in self.original_data])

    def _save_all(self):
        fix_data_io.save_characters(PROJECT_ROOT, self.characters)
        self.original_data = copy.deepcopy(self.characters)
        messagebox.showinfo("Saved", "Characters saved successfully.")

    def on_closing(self):
        if self._is_dirty():
            res = shell.close_unsaved_dialog(self.root)
            if res == "cancel": return
            if res == "save": self._save_all()
        self.root.destroy()

if __name__ == "__main__":
    root = tk.Tk()
    app = CharacterEditorApp(root)
    root.protocol("WM_DELETE_WINDOW", app.on_closing)
    root.mainloop()