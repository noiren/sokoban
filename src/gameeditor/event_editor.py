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

class EventEditorApp:
    def __init__(self, root: tk.Tk):
        self.root = root
        self.root.title("Event Editor")
        self.characters = fix_data_io.load_characters(PROJECT_ROOT)
        self.events = fix_data_io.load_all_events(PROJECT_ROOT)
        self.original_events = copy.deepcopy(self.events)
        self.current_evt_idx = None
        
        self.preview_photo = None
        
        self.bgms, self.ses = fix_data_io.get_audio_ids(PROJECT_ROOT)
        
        self._setup_ui()
        self._refresh_event_list()

    def _setup_ui(self):
        main_paned = ttk.PanedWindow(self.root, orient=tk.HORIZONTAL)
        main_paned.pack(fill=tk.BOTH, expand=True, padx=8, pady=8)

        # ---------------- Left: Event File List ----------------
        left_frame = ttk.Frame(main_paned)
        main_paned.add(left_frame, weight=1)
        
        ttk.Label(left_frame, text="Event Files").pack(anchor="w")
        self.evt_tree = ttk.Treeview(left_frame, columns=("id", "title"), show="headings", selectmode="browse")
        self.evt_tree.heading("id", text="Event ID")
        self.evt_tree.heading("title", text="Title (JA)")
        self.evt_tree.pack(fill=tk.BOTH, expand=True)
        self.evt_tree.bind("<<TreeviewSelect>>", self._on_event_select)

        btn_f1 = ttk.Frame(left_frame)
        btn_f1.pack(fill=tk.X, pady=4)
        ttk.Button(btn_f1, text="New Event", command=self._add_event).pack(side=tk.LEFT)
        ttk.Button(btn_f1, text="Edit Title", command=self._edit_event_title).pack(side=tk.LEFT)
        ttk.Button(btn_f1, text="Save All", command=self._save_all).pack(side=tk.RIGHT)

        # ---------------- Right: Lines Editor ----------------
        right_frame = ttk.Frame(main_paned)
        main_paned.add(right_frame, weight=3)
        
        self.lbl_lines = ttk.Label(right_frame, text="Lines: None Selected")
        self.lbl_lines.pack(anchor="w")
        
        # Line Tree
        self.line_tree = ttk.Treeview(right_frame, columns=("speaker", "face", "pos", "image", "center", "text"), show="headings", selectmode="browse")
        self.line_tree.heading("speaker", text="Speaker")
        self.line_tree.heading("face", text="Face")
        self.line_tree.heading("pos", text="Pos")
        self.line_tree.column("pos", width=50)
        self.line_tree.heading("image", text="Image")
        self.line_tree.heading("center", text="Center")
        self.line_tree.column("center", width=90)
        self.line_tree.heading("text", text="Text Preview")
        self.line_tree.pack(fill=tk.BOTH, expand=True)
        self.line_tree.bind("<<TreeviewSelect>>", self._on_line_select)

        btn_f2 = ttk.Frame(right_frame)
        btn_f2.pack(fill=tk.X, pady=4)
        ttk.Button(btn_f2, text="Add Line", command=self._add_line).pack(side=tk.LEFT)
        ttk.Button(btn_f2, text="Delete Line", command=self._del_line).pack(side=tk.LEFT)
        ttk.Button(btn_f2, text="Move Up", command=lambda: self._move_line(-1)).pack(side=tk.LEFT, padx=(10,0))
        ttk.Button(btn_f2, text="Move Down", command=lambda: self._move_line(1)).pack(side=tk.LEFT)

        # ---------------- Edit Panel & Preview ----------------
        bottom_edit_frame = ttk.Frame(right_frame)
        bottom_edit_frame.pack(fill=tk.X, pady=8)
        
        # Left side: Form
        edit_form = ttk.LabelFrame(bottom_edit_frame, text="Edit Line")
        edit_form.pack(side=tk.LEFT, fill=tk.BOTH, expand=True, padx=(0, 8))

        # Row 0: Speaker & Pos
        ttk.Label(edit_form, text="Speaker:").grid(row=0, column=0, padx=4, pady=4, sticky="e")
        self.var_speaker = tk.StringVar()
        self.cb_speaker = ttk.Combobox(edit_form, textvariable=self.var_speaker, state="readonly")
        self.cb_speaker['values'] = [c.id for c in self.characters]
        self.cb_speaker.grid(row=0, column=1, padx=4, pady=4, sticky="w")
        self.cb_speaker.bind("<<ComboboxSelected>>", self._on_speaker_changed)

        ttk.Label(edit_form, text="Position:").grid(row=0, column=2, padx=4, pady=4, sticky="e")
        self.var_pos = tk.StringVar(value="LEFT")
        self.cb_pos = ttk.Combobox(edit_form, textvariable=self.var_pos, state="readonly", width=10)
        self.cb_pos['values'] = ["LEFT", "RIGHT"]
        self.cb_pos.grid(row=0, column=3, padx=4, pady=4, sticky="w")
        self.cb_pos.bind("<<ComboboxSelected>>", self._on_field_modified)

        # Row 1: Face & Auto Image
        ttk.Label(edit_form, text="Face:").grid(row=1, column=0, padx=4, pady=4, sticky="e")
        self.var_face = tk.StringVar()
        self.cb_face = ttk.Combobox(edit_form, textvariable=self.var_face, state="readonly")
        self.cb_face['values'] = fix_data_io.FACE_IDS
        self.cb_face.grid(row=1, column=1, padx=4, pady=4, sticky="w")
        self.cb_face.bind("<<ComboboxSelected>>", self._on_face_changed)

        ttk.Label(edit_form, text="Image ID:").grid(row=1, column=2, padx=4, pady=4, sticky="e")
        self.var_image = tk.StringVar()
        ttk.Entry(edit_form, textvariable=self.var_image, state="readonly", width=20).grid(row=1, column=3, padx=4, pady=4, sticky="w")

        # Row 2: Audio
        ttk.Label(edit_form, text="BGM:").grid(row=2, column=0, padx=4, pady=4, sticky="e")
        self.var_bgm = tk.StringVar()
        self.cb_bgm = ttk.Combobox(edit_form, textvariable=self.var_bgm, state="readonly", width=12)
        self.cb_bgm['values'] = [""] + self.bgms
        self.cb_bgm.grid(row=2, column=1, padx=4, pady=4, sticky="w")
        self.cb_bgm.bind("<<ComboboxSelected>>", self._on_field_modified)

        self.var_stop_bgm = tk.BooleanVar()
        self.chk_stop_bgm = ttk.Checkbutton(edit_form, text="Stop", variable=self.var_stop_bgm, command=self._on_field_modified)
        self.chk_stop_bgm.grid(row=2, column=2, padx=4, pady=4, sticky="w")

        ttk.Label(edit_form, text="SE:").grid(row=2, column=3, padx=4, pady=4, sticky="e") # fixed grid
        self.var_se = tk.StringVar()
        self.cb_se = ttk.Combobox(edit_form, textvariable=self.var_se, state="readonly", width=12)
        self.cb_se['values'] = [""] + self.ses
        self.cb_se.grid(row=2, column=4, padx=4, pady=4, sticky="w")
        self.cb_se.bind("<<ComboboxSelected>>", self._on_field_modified)

        # Row 3: Emotion
        ttk.Label(edit_form, text="Emotion:").grid(row=3, column=0, padx=4, pady=4, sticky="e")
        self.var_emotion = tk.StringVar()
        self.cb_emotion = ttk.Combobox(edit_form, textvariable=self.var_emotion, state="readonly", width=12)
        self.cb_emotion['values'] = ["", "exclamation", "question", "anger", "sweat", "heart"]
        self.cb_emotion.grid(row=3, column=1, padx=4, pady=4, sticky="w")
        self.cb_emotion.bind("<<ComboboxSelected>>", self._on_field_modified)

        ttk.Label(edit_form, text="Center CG:").grid(row=4, column=0, padx=4, pady=4, sticky="e")
        self.var_center = tk.StringVar()
        ttk.Entry(edit_form, textvariable=self.var_center, width=28).grid(row=4, column=1, columnspan=4, padx=4, pady=4, sticky="w")
        self.var_center.trace_add("write", lambda *_: self._on_field_modified())

        # Row 5: Text
        ttk.Label(edit_form, text="Text:").grid(row=5, column=0, padx=4, pady=4, sticky="ne")
        self.txt_text = tk.Text(edit_form, height=4, width=40)
        self.txt_text.grid(row=5, column=1, columnspan=4, padx=4, pady=4, sticky="we")
        self.txt_text.bind("<KeyRelease>", self._on_field_modified)

        # Right side: Preview
        prev_frame = ttk.LabelFrame(bottom_edit_frame, text="Preview")
        prev_frame.pack(side=tk.RIGHT, fill=tk.BOTH)
        self.lbl_preview = ttk.Label(prev_frame, text="No Image", width=20, anchor=tk.CENTER)
        self.lbl_preview.pack(fill=tk.BOTH, expand=True, padx=4, pady=4)

    def _refresh_event_list(self):
        for item in self.evt_tree.get_children():
            self.evt_tree.delete(item)
        for i, e in enumerate(self.events):
            self.evt_tree.insert("", tk.END, iid=str(i), values=(e.id, e.title_ja))

    def _on_event_select(self, event):
        sel = self.evt_tree.selection()
        if sel:
            self.current_evt_idx = int(sel[0])
            self.lbl_lines.config(text=f"Lines: {self.events[self.current_evt_idx].title_ja}")
        else:
            self.current_evt_idx = None
            self.lbl_lines.config(text="Lines: None Selected")
        self._refresh_line_list()

    def _refresh_line_list(self, select_idx=None):
        for item in self.line_tree.get_children():
            self.line_tree.delete(item)
        
        self.var_speaker.set("")
        self.var_face.set("")
        self.var_pos.set("LEFT")
        self.var_image.set("")
        self.var_bgm.set("")
        self.var_se.set("")
        self.var_stop_bgm.set(False)
        self.var_emotion.set("")
        self.var_center.set("")
        self.txt_text.delete(1.0, tk.END)
        self._update_preview("")

        if self.current_evt_idx is not None:
            lines = self.events[self.current_evt_idx].lines
            for i, ln in enumerate(lines):
                preview = ln.text.replace('\n', ' ')[:20]
                self.line_tree.insert("", tk.END, iid=str(i), values=(ln.speaker_id, ln.face_id, ln.position, ln.image_id, ln.center_image_id, preview))
            if select_idx is not None and 0 <= select_idx < len(lines):
                self.line_tree.selection_set(str(select_idx))

    def _on_line_select(self, event):
        sel = self.line_tree.selection()
        if not sel or self.current_evt_idx is None: return
        idx = int(sel[0])
        ln = self.events[self.current_evt_idx].lines[idx]

        self.var_speaker.set(ln.speaker_id)
        self.var_face.set(ln.face_id)
        self.var_pos.set(ln.position)
        self.var_image.set(ln.image_id)
        self.var_bgm.set(ln.bgm_id)
        self.var_se.set(ln.se_id)
        self.var_stop_bgm.set(ln.stop_bgm)
        self.var_emotion.set(ln.emotion_id)
        self.var_center.set(ln.center_image_id)
        
        self.txt_text.delete(1.0, tk.END)
        self.txt_text.insert(1.0, ln.text)
        
        self._update_preview(ln.image_id)

    # --- Core Logic ---
    def _on_speaker_changed(self, event=None):
        self.var_face.set("normal_1") # デフォルト顔
        self._on_face_changed()

    def _on_face_changed(self, event=None):
        spk = self.var_speaker.get()
        fid = self.var_face.get()
        char = next((c for c in self.characters if c.id == spk), None)
        img_id = ""
        if char:
            img_id = char.faces.get(fid, "")
        
        self.var_image.set(img_id)
        self._update_preview(img_id)
        self._sync_current_line_to_data()

    def _on_field_modified(self, event=None):
        self._sync_current_line_to_data()

    def _sync_current_line_to_data(self):
        sel = self.line_tree.selection()
        if not sel or self.current_evt_idx is None: return
        idx = int(sel[0])
        ln = self.events[self.current_evt_idx].lines[idx]
        
        ln.speaker_id = self.var_speaker.get()
        ln.face_id = self.var_face.get()
        ln.position = self.var_pos.get()
        ln.image_id = self.var_image.get()
        ln.bgm_id = self.var_bgm.get()
        ln.se_id = self.var_se.get()
        ln.stop_bgm = self.var_stop_bgm.get()
        ln.emotion_id = self.var_emotion.get()
        ln.center_image_id = self.var_center.get().strip()
        ln.text = self.txt_text.get(1.0, "end-1c")

        self.line_tree.item(str(idx), values=(ln.speaker_id, ln.face_id, ln.position, ln.image_id, ln.center_image_id, ln.text.replace('\n', ' ')[:20]))

    def _update_preview(self, image_id: str):
        if not image_id:
            self.lbl_preview.config(image='', text="No Image")
            self.preview_photo = None
            return
        path = fix_data_io.find_chara_sprite_path(PROJECT_ROOT, image_id)
        if not path:
            self.lbl_preview.config(image='', text="Not Found")
            self.preview_photo = None
            return
        try:
            img = Image.open(path)
            img.thumbnail((128, 128), Image.Resampling.LANCZOS)
            self.preview_photo = ImageTk.PhotoImage(img)
            self.lbl_preview.config(image=self.preview_photo, text="")
        except:
            self.lbl_preview.config(image='', text="Error")

    # --- Actions ---
    def _add_event(self):
        eid = shell.ask_string(self.root, "New Event", "Enter Event ID:")
        if not eid: return
        tja = shell.ask_string(self.root, "New Event", "Enter Event Title (JA):")
        if tja is None: return
        self.events.append(fix_data_io.EventEntry(id=eid, title_ja=tja, lines=[]))
        self._refresh_event_list()

    def _edit_event_title(self):
        if self.current_evt_idx is None: return
        e = self.events[self.current_evt_idx]
        tja = shell.ask_string(self.root, "Edit Title", "Enter Event Title (JA):", e.title_ja)
        if tja is not None:
            e.title_ja = tja
            self._refresh_event_list()

    def _add_line(self):
        if self.current_evt_idx is None: return
        new_ln = fix_data_io.EventLine(speaker_id="", face_id="normal_1", position="LEFT", image_id="", text="", bgm_id="", se_id="", stop_bgm=False, emotion_id="", center_image_id="")
        self.events[self.current_evt_idx].lines.append(new_ln)
        self._refresh_line_list(select_idx=len(self.events[self.current_evt_idx].lines)-1)

    def _del_line(self):
        sel = self.line_tree.selection()
        if not sel or self.current_evt_idx is None: return
        idx = int(sel[0])
        del self.events[self.current_evt_idx].lines[idx]
        self._refresh_line_list()

    def _move_line(self, direction):
        sel = self.line_tree.selection()
        if not sel or self.current_evt_idx is None: return
        idx = int(sel[0])
        new_idx = idx + direction
        lines = self.events[self.current_evt_idx].lines
        if 0 <= new_idx < len(lines):
            lines[idx], lines[new_idx] = lines[new_idx], lines[idx]
            self._refresh_line_list(select_idx=new_idx)

    def _is_dirty(self):
        return shell.snapshot_json([dataclasses.asdict(e) for e in self.events]) != \
               shell.snapshot_json([dataclasses.asdict(e) for e in self.original_events])

    def _save_all(self):
        for e in self.events:
            filename = fix_data_io.event_filename_from_id(e.id)
            fix_data_io.save_event_file(PROJECT_ROOT, filename, e)
        self.original_events = copy.deepcopy(self.events)
        messagebox.showinfo("Saved", "All events saved successfully.")

    def on_closing(self):
        if self._is_dirty():
            res = shell.close_unsaved_dialog(self.root)
            if res == "cancel": return
            if res == "save": self._save_all()
        self.root.destroy()

if __name__ == "__main__":
    root = tk.Tk()
    app = EventEditorApp(root)
    root.protocol("WM_DELETE_WINDOW", app.on_closing)
    root.mainloop()