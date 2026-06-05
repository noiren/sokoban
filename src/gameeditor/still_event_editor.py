#!/usr/bin/env python3
import os
import tkinter as tk
from tkinter import ttk, messagebox
from PIL import Image, ImageTk, ImageDraw, ImageFont
import fix_data_io
import audio_editor_shell as shell
import copy
import dataclasses

SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))
PROJECT_ROOT = os.path.dirname(os.path.dirname(SCRIPT_DIR))

class StillEventEditorApp:
    def __init__(self, root: tk.Tk):
        self.root = root
        self.root.title("スチルイベントエディタ")
        self.events = fix_data_io.load_all_still_events(PROJECT_ROOT)
        self.original_events = copy.deepcopy(self.events)
        self.current_evt_idx = None
        self.current_page_idx = None
        self.current_msg_idx = None
        
        self.preview_photo = None
        
        self.bgms, self.ses = fix_data_io.get_audio_ids(PROJECT_ROOT)
        self.stills = fix_data_io.scan_still_resources(PROJECT_ROOT)
        
        self.font = None
        try:
            # Try a default Windows Japanese font
            self.font = ImageFont.truetype("msgothic.ttc", 14)
        except:
            try:
                self.font = ImageFont.truetype("meiryo.ttc", 14)
            except:
                self.font = ImageFont.load_default()
        
        self._setup_ui()
        self._refresh_event_list()

    def _setup_ui(self):
        main_paned = ttk.PanedWindow(self.root, orient=tk.HORIZONTAL)
        main_paned.pack(fill=tk.BOTH, expand=True, padx=8, pady=8)

        # ---------------- Left: Event List ----------------
        left_frame = ttk.Frame(main_paned)
        main_paned.add(left_frame, weight=1)
        
        ttk.Label(left_frame, text="イベント一覧").pack(anchor="w")
        self.evt_tree = ttk.Treeview(left_frame, columns=("id", "title"), show="headings", selectmode="browse")
        self.evt_tree.heading("id", text="イベントID")
        self.evt_tree.heading("title", text="タイトル")
        self.evt_tree.pack(fill=tk.BOTH, expand=True)
        self.evt_tree.bind("<<TreeviewSelect>>", self._on_event_select)

        btn_f1 = ttk.Frame(left_frame)
        btn_f1.pack(fill=tk.X, pady=4)
        ttk.Button(btn_f1, text="新規作成", command=self._add_event).pack(side=tk.LEFT)
        ttk.Button(btn_f1, text="タイトル変更", command=self._edit_event_title).pack(side=tk.LEFT)
        ttk.Button(btn_f1, text="すべて保存", command=self._save_all).pack(side=tk.RIGHT)

        # ---------------- Middle: Page List ----------------
        mid_frame = ttk.Frame(main_paned)
        main_paned.add(mid_frame, weight=1)

        ttk.Label(mid_frame, text="ページ(スチル)一覧").pack(anchor="w")
        self.page_tree = ttk.Treeview(mid_frame, columns=("image",), show="headings", selectmode="browse")
        self.page_tree.heading("image", text="スチル画像")
        self.page_tree.pack(fill=tk.BOTH, expand=True)
        self.page_tree.bind("<<TreeviewSelect>>", self._on_page_select)

        btn_f2 = ttk.Frame(mid_frame)
        btn_f2.pack(fill=tk.X, pady=4)
        ttk.Button(btn_f2, text="ページ追加", command=self._add_page).pack(side=tk.LEFT)
        ttk.Button(btn_f2, text="削除", command=self._del_page).pack(side=tk.LEFT)
        ttk.Button(btn_f2, text="↑", command=lambda: self._move_page(-1), width=3).pack(side=tk.LEFT, padx=(5,0))
        ttk.Button(btn_f2, text="↓", command=lambda: self._move_page(1), width=3).pack(side=tk.LEFT)

        # ---------------- Right: Settings & Messages ----------------
        right_frame = ttk.Frame(main_paned)
        main_paned.add(right_frame, weight=3)

        # Right Top: Page Settings & Preview
        top_right = ttk.Frame(right_frame)
        top_right.pack(fill=tk.X, pady=(0, 8))
        
        self.page_settings = ttk.LabelFrame(top_right, text="ページ設定")
        self.page_settings.pack(side=tk.LEFT, fill=tk.BOTH, expand=True, padx=(0, 8))

        ttk.Label(self.page_settings, text="スチル画像:").grid(row=0, column=0, padx=4, pady=4, sticky="e")
        self.var_still = tk.StringVar()
        self.cb_still = ttk.Combobox(self.page_settings, textvariable=self.var_still, state="readonly", width=20)
        self.cb_still['values'] = [""] + self.stills
        self.cb_still.grid(row=0, column=1, padx=4, pady=4, sticky="w")
        self.cb_still.bind("<<ComboboxSelected>>", self._on_page_field_modified)

        ttk.Label(self.page_settings, text="フェードイン(秒):").grid(row=1, column=0, padx=4, pady=4, sticky="e")
        self.var_fade_in = tk.StringVar()
        ttk.Entry(self.page_settings, textvariable=self.var_fade_in, width=8).grid(row=1, column=1, padx=4, pady=4, sticky="w")
        self.var_fade_in.trace_add("write", lambda *args: self._on_page_field_modified())

        ttk.Label(self.page_settings, text="フェードアウト(秒):").grid(row=2, column=0, padx=4, pady=4, sticky="e")
        self.var_fade_out = tk.StringVar()
        ttk.Entry(self.page_settings, textvariable=self.var_fade_out, width=8).grid(row=2, column=1, padx=4, pady=4, sticky="w")
        self.var_fade_out.trace_add("write", lambda *args: self._on_page_field_modified())
        
        # Preview
        prev_frame = ttk.LabelFrame(top_right, text="プレビュー (実機イメージ)")
        prev_frame.pack(side=tk.RIGHT, fill=tk.BOTH)
        self.lbl_preview = ttk.Label(prev_frame, text="画像なし", width=35, anchor=tk.CENTER)
        self.lbl_preview.pack(fill=tk.BOTH, expand=True, padx=4, pady=4)

        # Right Bottom: Messages
        msg_frame = ttk.Frame(right_frame)
        msg_frame.pack(fill=tk.BOTH, expand=True)
        
        ttk.Label(msg_frame, text="メッセージ(テキスト)一覧").pack(anchor="w")
        self.msg_tree = ttk.Treeview(msg_frame, columns=("text", "audio"), show="headings", selectmode="browse", height=5)
        self.msg_tree.heading("text", text="テキスト")
        self.msg_tree.heading("audio", text="BGM/SE")
        self.msg_tree.pack(fill=tk.BOTH, expand=True)
        self.msg_tree.bind("<<TreeviewSelect>>", self._on_msg_select)

        btn_f3 = ttk.Frame(msg_frame)
        btn_f3.pack(fill=tk.X, pady=4)
        ttk.Button(btn_f3, text="メッセージ追加", command=self._add_msg).pack(side=tk.LEFT)
        ttk.Button(btn_f3, text="削除", command=self._del_msg).pack(side=tk.LEFT)
        ttk.Button(btn_f3, text="↑", command=lambda: self._move_msg(-1), width=3).pack(side=tk.LEFT, padx=(5,0))
        ttk.Button(btn_f3, text="↓", command=lambda: self._move_msg(1), width=3).pack(side=tk.LEFT)

        self.msg_edit = ttk.LabelFrame(msg_frame, text="メッセージ編集")
        self.msg_edit.pack(fill=tk.X, pady=(4, 0))

        ttk.Label(self.msg_edit, text="BGM:").grid(row=0, column=0, padx=4, pady=4, sticky="e")
        self.var_bgm = tk.StringVar()
        self.cb_bgm = ttk.Combobox(self.msg_edit, textvariable=self.var_bgm, state="readonly", width=15)
        self.cb_bgm['values'] = [""] + self.bgms
        self.cb_bgm.grid(row=0, column=1, padx=4, pady=4, sticky="w")
        self.cb_bgm.bind("<<ComboboxSelected>>", self._on_msg_field_modified)

        self.var_stop_bgm = tk.BooleanVar()
        self.chk_stop_bgm = ttk.Checkbutton(self.msg_edit, text="BGM停止", variable=self.var_stop_bgm, command=self._on_msg_field_modified)
        self.chk_stop_bgm.grid(row=0, column=2, padx=4, pady=4, sticky="w")

        ttk.Label(self.msg_edit, text="SE:").grid(row=0, column=3, padx=4, pady=4, sticky="e")
        self.var_se = tk.StringVar()
        self.cb_se = ttk.Combobox(self.msg_edit, textvariable=self.var_se, state="readonly", width=15)
        self.cb_se['values'] = [""] + self.ses
        self.cb_se.grid(row=0, column=4, padx=4, pady=4, sticky="w")
        self.cb_se.bind("<<ComboboxSelected>>", self._on_msg_field_modified)

        ttk.Label(self.msg_edit, text="テキスト:").grid(row=1, column=0, padx=4, pady=4, sticky="ne")
        self.txt_text = tk.Text(self.msg_edit, height=4, width=50)
        self.txt_text.grid(row=1, column=1, columnspan=4, padx=4, pady=4, sticky="we")
        self.txt_text.bind("<KeyRelease>", self._on_msg_field_modified)

        self._set_state(self.page_settings, tk.DISABLED)
        self._set_state(self.msg_edit, tk.DISABLED)

    def _set_state(self, widget, state):
        for child in widget.winfo_children():
            try:
                child.configure(state=state)
            except:
                pass
            if isinstance(child, tk.Text):
                child.config(state=state)

    def _refresh_event_list(self):
        for item in self.evt_tree.get_children():
            self.evt_tree.delete(item)
        for i, e in enumerate(self.events):
            self.evt_tree.insert("", tk.END, iid=str(i), values=(e.id, e.title_ja))

    def _on_event_select(self, event):
        sel = self.evt_tree.selection()
        if sel:
            self.current_evt_idx = int(sel[0])
        else:
            self.current_evt_idx = None
        self._refresh_page_list()

    def _refresh_page_list(self, select_idx=None):
        for item in self.page_tree.get_children():
            self.page_tree.delete(item)
        
        self.current_page_idx = None
        self._refresh_msg_list()
        
        if self.current_evt_idx is not None:
            e = self.events[self.current_evt_idx]
            for i, p in enumerate(e.pages):
                self.page_tree.insert("", tk.END, iid=str(i), values=(p.still_image_id or "(空)",))
            if select_idx is not None and 0 <= select_idx < len(e.pages):
                self.page_tree.selection_set(str(select_idx))

    def _on_page_select(self, event):
        sel = self.page_tree.selection()
        if not sel or self.current_evt_idx is None:
            self.current_page_idx = None
            self._set_state(self.page_settings, tk.DISABLED)
            self._refresh_msg_list()
            return
        
        self.current_page_idx = int(sel[0])
        p = self.events[self.current_evt_idx].pages[self.current_page_idx]
        
        self.var_still.set(p.still_image_id)
        self.var_fade_in.set(f"{p.fade_in_frames / 60.0:.2f}")
        self.var_fade_out.set(f"{p.fade_out_frames / 60.0:.2f}")
        
        self._set_state(self.page_settings, tk.NORMAL)
        self.cb_still.config(state="readonly")
        self._refresh_msg_list()
        self._update_preview()

    def _on_page_field_modified(self, event=None):
        if self.current_evt_idx is None or self.current_page_idx is None: return
        p = self.events[self.current_evt_idx].pages[self.current_page_idx]
        p.still_image_id = self.var_still.get()
        
        try:
            p.fade_in_frames = int(float(self.var_fade_in.get()) * 60)
        except: pass
        try:
            p.fade_out_frames = int(float(self.var_fade_out.get()) * 60)
        except: pass
        
        self.page_tree.item(str(self.current_page_idx), values=(p.still_image_id or "(空)",))
        self._update_preview()

    def _refresh_msg_list(self, select_idx=None):
        for item in self.msg_tree.get_children():
            self.msg_tree.delete(item)
            
        self.current_msg_idx = None
        self.var_bgm.set("")
        self.var_se.set("")
        self.var_stop_bgm.set(False)
        self.txt_text.config(state=tk.NORMAL)
        self.txt_text.delete(1.0, tk.END)
        self._set_state(self.msg_edit, tk.DISABLED)
        
        if self.current_evt_idx is not None and self.current_page_idx is not None:
            p = self.events[self.current_evt_idx].pages[self.current_page_idx]
            for i, m in enumerate(p.messages):
                audio = []
                if m.stop_bgm: audio.append("StopBGM")
                if m.bgm_id: audio.append(f"B:{m.bgm_id}")
                if m.se_id: audio.append(f"S:{m.se_id}")
                audio_str = " ".join(audio)
                txt = m.text.replace('\n', ' ')[:20]
                self.msg_tree.insert("", tk.END, iid=str(i), values=(txt, audio_str))
            
            if select_idx is not None and 0 <= select_idx < len(p.messages):
                self.msg_tree.selection_set(str(select_idx))

    def _on_msg_select(self, event):
        sel = self.msg_tree.selection()
        if not sel or self.current_page_idx is None:
            self.current_msg_idx = None
            self._set_state(self.msg_edit, tk.DISABLED)
            self._update_preview()
            return
            
        self.current_msg_idx = int(sel[0])
        m = self.events[self.current_evt_idx].pages[self.current_page_idx].messages[self.current_msg_idx]
        
        self._set_state(self.msg_edit, tk.NORMAL)
        self.cb_bgm.config(state="readonly")
        self.cb_se.config(state="readonly")
        
        self.var_bgm.set(m.bgm_id)
        self.var_se.set(m.se_id)
        self.var_stop_bgm.set(m.stop_bgm)
        self.txt_text.delete(1.0, tk.END)
        self.txt_text.insert(1.0, m.text)
        
        self._update_preview()

    def _on_msg_field_modified(self, event=None):
        if self.current_msg_idx is None: return
        m = self.events[self.current_evt_idx].pages[self.current_page_idx].messages[self.current_msg_idx]
        
        m.bgm_id = self.var_bgm.get()
        m.se_id = self.var_se.get()
        m.stop_bgm = self.var_stop_bgm.get()
        m.text = self.txt_text.get(1.0, "end-1c")
        
        audio = []
        if m.stop_bgm: audio.append("StopBGM")
        if m.bgm_id: audio.append(f"B:{m.bgm_id}")
        if m.se_id: audio.append(f"S:{m.se_id}")
        audio_str = " ".join(audio)
        txt = m.text.replace('\n', ' ')[:20]
        
        self.msg_tree.item(str(self.current_msg_idx), values=(txt, audio_str))
        self._update_preview()

    def _update_preview(self):
        still_id = self.var_still.get() if self.current_page_idx is not None else ""
        txt = self.txt_text.get(1.0, "end-1c") if self.current_msg_idx is not None else ""
        
        img = None
        if still_id:
            s_dir = fix_data_io.still_dir(PROJECT_ROOT)
            path = os.path.join(s_dir, f"{still_id}.bmp")
            if os.path.exists(path):
                try:
                    img = Image.open(path).convert("RGBA")
                except:
                    pass
        
        if img is None:
            img = Image.new("RGBA", (240, 160), (0, 0, 0, 255))
        
        if txt:
            draw = ImageDraw.Draw(img)
            # テキストにシャドウをつけて見やすくする
            y_base = 110
            for line in txt.split('\n'):
                draw.text((17, y_base+1), line, font=self.font, fill=(0, 0, 0, 255))
                draw.text((16, y_base), line, font=self.font, fill=(255, 255, 255, 255))
                y_base += 16
        
        img = img.resize((480, 320), Image.Resampling.NEAREST)
        self.preview_photo = ImageTk.PhotoImage(img)
        self.lbl_preview.config(image=self.preview_photo, text="")

    # --- Actions ---
    def _add_event(self):
        eid = shell.ask_string(self.root, "新規作成", "イベントIDを入力 (例: sevt_01):")
        if not eid: return
        tja = shell.ask_string(self.root, "新規作成", "タイトルを入力:")
        if tja is None: return
        self.events.append(fix_data_io.StillEventEntry(id=eid, title_ja=tja, pages=[]))
        self._refresh_event_list()

    def _edit_event_title(self):
        if self.current_evt_idx is None: return
        e = self.events[self.current_evt_idx]
        tja = shell.ask_string(self.root, "タイトル変更", "タイトルを入力:", e.title_ja)
        if tja is not None:
            e.title_ja = tja
            self._refresh_event_list()

    def _add_page(self):
        if self.current_evt_idx is None: return
        e = self.events[self.current_evt_idx]
        e.pages.append(fix_data_io.StillEventPage(still_image_id="", fade_in_frames=16, fade_out_frames=16, messages=[]))
        self._refresh_page_list(len(e.pages)-1)

    def _del_page(self):
        if self.current_page_idx is None: return
        e = self.events[self.current_evt_idx]
        del e.pages[self.current_page_idx]
        self._refresh_page_list()

    def _move_page(self, direction):
        if self.current_page_idx is None: return
        e = self.events[self.current_evt_idx]
        new_idx = self.current_page_idx + direction
        if 0 <= new_idx < len(e.pages):
            e.pages[self.current_page_idx], e.pages[new_idx] = e.pages[new_idx], e.pages[self.current_page_idx]
            self._refresh_page_list(new_idx)

    def _add_msg(self):
        if self.current_page_idx is None: return
        p = self.events[self.current_evt_idx].pages[self.current_page_idx]
        p.messages.append(fix_data_io.StillEventMessage(text=""))
        self._refresh_msg_list(len(p.messages)-1)

    def _del_msg(self):
        if self.current_msg_idx is None: return
        p = self.events[self.current_evt_idx].pages[self.current_page_idx]
        del p.messages[self.current_msg_idx]
        self._refresh_msg_list()

    def _move_msg(self, direction):
        if self.current_msg_idx is None: return
        p = self.events[self.current_evt_idx].pages[self.current_page_idx]
        new_idx = self.current_msg_idx + direction
        if 0 <= new_idx < len(p.messages):
            p.messages[self.current_msg_idx], p.messages[new_idx] = p.messages[new_idx], p.messages[self.current_msg_idx]
            self._refresh_msg_list(new_idx)

    def _is_dirty(self):
        return shell.snapshot_json([dataclasses.asdict(e) for e in self.events]) != \
               shell.snapshot_json([dataclasses.asdict(e) for e in self.original_events])

    def _save_all(self):
        for e in self.events:
            filename = fix_data_io.still_event_filename_from_id(e.id)
            fix_data_io.save_still_event_file(PROJECT_ROOT, filename, e)
        self.original_events = copy.deepcopy(self.events)
        messagebox.showinfo("保存完了", "スチルイベントをすべて保存しました。")

    def on_closing(self):
        if self._is_dirty():
            res = shell.close_unsaved_dialog(self.root)
            if res == "cancel": return
            if res == "save": self._save_all()
        self.root.destroy()

if __name__ == "__main__":
    root = tk.Tk()
    app = StillEventEditorApp(root)
    root.protocol("WM_DELETE_WINDOW", app.on_closing)
    root.mainloop()
