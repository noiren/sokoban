#!/usr/bin/env python3
import os
import tkinter as tk
from tkinter import ttk, messagebox

import fix_data_io

SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))
PROJECT_ROOT = os.path.dirname(os.path.dirname(SCRIPT_DIR))

class StoryEditorApp:
    def __init__(self, root: tk.Tk):
        self.root = root
        self.root.title("GBAソコバン ストーリー進行エディタ")
        self.chapters = fix_data_io.load_story_progression(PROJECT_ROOT)
        self.events = fix_data_io.load_all_events(PROJECT_ROOT)
        self.event_ids = [e.id for e in self.events]
        self._setup_ui()
        self._refresh_tree()

    def _setup_ui(self):
        main = ttk.PanedWindow(self.root, orient=tk.HORIZONTAL)
        main.pack(fill=tk.BOTH, expand=True, padx=8, pady=8)

        # 左ペイン (Tree)
        left = ttk.Frame(main)
        main.add(left, weight=1)

        self.tree = ttk.Treeview(left, columns=("type", "ref"), show="tree headings")
        self.tree.heading("#0", text="項目")
        self.tree.heading("type", text="種別")
        self.tree.heading("ref", text="参照")
        self.tree.column("#0", width=120)
        self.tree.column("type", width=80)
        self.tree.column("ref", width=120)
        self.tree.pack(fill=tk.BOTH, expand=True)
        self.tree.bind("<<TreeviewSelect>>", self._on_select)

        btn_f = ttk.Frame(left)
        btn_f.pack(fill=tk.X, pady=4)
        ttk.Button(btn_f, text="チャプター追加", command=self._add_chapter).pack(side=tk.LEFT)
        ttk.Button(btn_f, text="ステップ追加", command=self._add_step).pack(side=tk.LEFT)
        ttk.Button(btn_f, text="削除", command=self._delete_item).pack(side=tk.LEFT)
        ttk.Button(btn_f, text="↑", command=self._move_up).pack(side=tk.LEFT)
        ttk.Button(btn_f, text="↓", command=self._move_down).pack(side=tk.LEFT)

        # 右ペイン (詳細編集)
        self.right = ttk.Frame(main)
        main.add(self.right, weight=2)

        # チャプター編集用フレーム
        self.frm_chapter = ttk.LabelFrame(self.right, text="チャプター編集")
        ttk.Label(self.frm_chapter, text="ID:").grid(row=0, column=0, sticky="e")
        self.var_ch_id = tk.StringVar()
        ttk.Entry(self.frm_chapter, textvariable=self.var_ch_id).grid(row=0, column=1, sticky="w")
        
        ttk.Label(self.frm_chapter, text="タイトル(日):").grid(row=1, column=0, sticky="e")
        self.var_ch_title = tk.StringVar()
        ttk.Entry(self.frm_chapter, textvariable=self.var_ch_title).grid(row=1, column=1, sticky="w")
        ttk.Button(self.frm_chapter, text="適用", command=self._apply_chapter).grid(row=2, column=0, columnspan=2, pady=4)

        # ステップ編集用フレーム
        self.frm_step = ttk.LabelFrame(self.right, text="ステップ編集")
        ttk.Label(self.frm_step, text="タイプ:").grid(row=0, column=0, sticky="e")
        self.var_st_type = tk.StringVar()
        cb_type = ttk.Combobox(self.frm_step, textvariable=self.var_st_type, values=["STILL_EVENT", "EVENT", "PUZZLE"], state="readonly")
        cb_type.grid(row=0, column=1, sticky="w")
        cb_type.bind("<<ComboboxSelected>>", self._on_step_type_change)
        
        ttk.Label(self.frm_step, text="参照先(ref):").grid(row=1, column=0, sticky="e")
        self.var_st_ref = tk.StringVar()
        self.cb_ref = ttk.Combobox(self.frm_step, textvariable=self.var_st_ref)
        self.cb_ref.grid(row=1, column=1, sticky="we")

        ttk.Label(self.frm_step, text="パズル前イベント:").grid(row=2, column=0, sticky="e")
        self.var_st_intro = tk.StringVar()
        self.cb_intro = ttk.Combobox(self.frm_step, textvariable=self.var_st_intro, values=[""] + self.event_ids)
        self.cb_intro.grid(row=2, column=1, sticky="we")

        ttk.Button(self.frm_step, text="適用", command=self._apply_step).grid(row=3, column=0, columnspan=2, pady=4)

        # 下部ボタン
        bottom = ttk.Frame(self.root)
        bottom.pack(fill=tk.X, padx=8, pady=8)
        ttk.Button(bottom, text="保存", command=self._save).pack(side=tk.RIGHT)

        self.selected_item = None

    def _refresh_tree(self):
        for i in self.tree.get_children():
            self.tree.delete(i)
        
        for c_idx, ch in enumerate(self.chapters):
            c_iid = f"c_{c_idx}"
            self.tree.insert("", "end", iid=c_iid, text=ch.get("title_ja", ch.get("id", "")), values=("CHAPTER", ch.get("id", "")), open=True)
            for s_idx, st in enumerate(ch.get("steps", [])):
                s_iid = f"s_{c_idx}_{s_idx}"
                self.tree.insert(c_iid, "end", iid=s_iid, text=f"Step {s_idx}", values=(st.get("type", ""), st.get("ref", "")))

    def _on_select(self, event):
        sel = self.tree.selection()
        if not sel:
            self.frm_chapter.pack_forget()
            self.frm_step.pack_forget()
            self.selected_item = None
            return
        
        iid = sel[0]
        self.selected_item = iid
        if iid.startswith("c_"):
            c_idx = int(iid.split("_")[1])
            ch = self.chapters[c_idx]
            self.var_ch_id.set(ch.get("id", ""))
            self.var_ch_title.set(ch.get("title_ja", ""))
            self.frm_step.pack_forget()
            self.frm_chapter.pack(fill=tk.X, anchor="n")
        elif iid.startswith("s_"):
            c_idx = int(iid.split("_")[1])
            s_idx = int(iid.split("_")[2])
            st = self.chapters[c_idx]["steps"][s_idx]
            self.var_st_type.set(st.get("type", "EVENT"))
            self.var_st_ref.set(str(st.get("ref", "")))
            self.var_st_intro.set(st.get("intro_event", ""))
            self._update_ref_choices()
            self.frm_chapter.pack_forget()
            self.frm_step.pack(fill=tk.X, anchor="n")

    def _update_ref_choices(self):
        stype = self.var_st_type.get()
        if stype in ["EVENT", "STILL_EVENT"]:
            self.cb_ref["values"] = self.event_ids
            self.cb_intro.state(["disabled"])
        else: # PUZZLE
            self.cb_ref["values"] = [str(i) for i in range(100)] # 簡易的
            self.cb_intro.state(["!disabled"])

    def _on_step_type_change(self, event):
        self._update_ref_choices()

    def _apply_chapter(self):
        if not self.selected_item or not self.selected_item.startswith("c_"): return
        c_idx = int(self.selected_item.split("_")[1])
        ch = self.chapters[c_idx]
        ch["id"] = self.var_ch_id.get()
        ch["title_ja"] = self.var_ch_title.get()
        self._refresh_tree()
        self.tree.selection_set(self.selected_item)

    def _apply_step(self):
        if not self.selected_item or not self.selected_item.startswith("s_"): return
        c_idx = int(self.selected_item.split("_")[1])
        s_idx = int(self.selected_item.split("_")[2])
        st = self.chapters[c_idx]["steps"][s_idx]
        
        stype = self.var_st_type.get()
        st["type"] = stype
        
        ref_val = self.var_st_ref.get()
        if stype == "PUZZLE":
            try:
                st["ref"] = int(ref_val)
            except ValueError:
                messagebox.showerror("エラー", "パズルのrefは数値を入力してください")
                return
            if self.var_st_intro.get():
                st["intro_event"] = self.var_st_intro.get()
            elif "intro_event" in st:
                del st["intro_event"]
        else:
            st["ref"] = ref_val
            if "intro_event" in st:
                del st["intro_event"]
                
        self._refresh_tree()
        self.tree.selection_set(self.selected_item)

    def _add_chapter(self):
        self.chapters.append({"id": f"CH{len(self.chapters)+1}", "title_ja": "新規チャプター", "steps": []})
        self._refresh_tree()

    def _add_step(self):
        sel = self.tree.selection()
        if not sel: return
        iid = sel[0]
        c_idx = int(iid.split("_")[1])
        ch = self.chapters[c_idx]
        if "steps" not in ch: ch["steps"] = []
        ch["steps"].append({"type": "EVENT", "ref": ""})
        self._refresh_tree()
        s_iid = f"s_{c_idx}_{len(ch['steps'])-1}"
        self.tree.selection_set(s_iid)

    def _delete_item(self):
        if not self.selected_item: return
        iid = self.selected_item
        c_idx = int(iid.split("_")[1])
        if iid.startswith("c_"):
            del self.chapters[c_idx]
        elif iid.startswith("s_"):
            s_idx = int(iid.split("_")[2])
            del self.chapters[c_idx]["steps"][s_idx]
        self.selected_item = None
        self._refresh_tree()

    def _move_up(self):
        if not self.selected_item: return
        iid = self.selected_item
        c_idx = int(iid.split("_")[1])
        if iid.startswith("c_"):
            if c_idx > 0:
                self.chapters[c_idx], self.chapters[c_idx-1] = self.chapters[c_idx-1], self.chapters[c_idx]
                self._refresh_tree()
                self.tree.selection_set(f"c_{c_idx-1}")
        elif iid.startswith("s_"):
            s_idx = int(iid.split("_")[2])
            if s_idx > 0:
                steps = self.chapters[c_idx]["steps"]
                steps[s_idx], steps[s_idx-1] = steps[s_idx-1], steps[s_idx]
                self._refresh_tree()
                self.tree.selection_set(f"s_{c_idx}_{s_idx-1}")

    def _move_down(self):
        if not self.selected_item: return
        iid = self.selected_item
        c_idx = int(iid.split("_")[1])
        if iid.startswith("c_"):
            if c_idx < len(self.chapters) - 1:
                self.chapters[c_idx], self.chapters[c_idx+1] = self.chapters[c_idx+1], self.chapters[c_idx]
                self._refresh_tree()
                self.tree.selection_set(f"c_{c_idx+1}")
        elif iid.startswith("s_"):
            s_idx = int(iid.split("_")[2])
            steps = self.chapters[c_idx]["steps"]
            if s_idx < len(steps) - 1:
                steps[s_idx], steps[s_idx+1] = steps[s_idx+1], steps[s_idx]
                self._refresh_tree()
                self.tree.selection_set(f"s_{c_idx}_{s_idx+1}")

    def _save(self):
        fix_data_io.save_story_progression(PROJECT_ROOT, self.chapters)
        messagebox.showinfo("保存完了", "story_progression.json を保存しました。\nmake または generate_fix_data.py の実行でゲームに反映されます。")

def main():
    root = tk.Tk()
    root.geometry("800x600")
    app = StoryEditorApp(root)
    root.mainloop()

if __name__ == "__main__":
    main()
