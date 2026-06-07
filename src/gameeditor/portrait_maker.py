#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
立ち絵メーカー (Portrait Maker)

プログラムが分からない人でも、
  1) 感情の一覧を編集し(追加・削除・数の変更)、
  2) 各感情に PNG 画像を割り当て、切り出し位置を微調整し、
  3) 「GBA素材に変換」を押すだけで
立ち絵スプライト(BMP)を作れるツール。

- 感情カタログ : Asset/fixdata/face_catalog.json
- 画像の割り当て: Asset/graphics/png/chara_bustup/<prefix>/_assign.json
- 出力BMP       : Asset/graphics/sprites/chara/chara/spr_ch_<prefix>_<感情>_<番号>.bmp
- 変換後はビルドすればゲームに反映される(対応表はビルド時に自動生成)。
"""
import os
import sys
import json
import subprocess
import tkinter as tk
from tkinter import ttk, messagebox, filedialog
from PIL import Image, ImageTk

SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))
PROJECT_ROOT = os.path.dirname(os.path.dirname(SCRIPT_DIR))
TOOLS_DIR = os.path.join(PROJECT_ROOT, "src", "tools")

if SCRIPT_DIR not in sys.path:
    sys.path.insert(0, SCRIPT_DIR)
if TOOLS_DIR not in sys.path:
    sys.path.insert(0, TOOLS_DIR)

import fix_data_io
import audio_editor_shell as shell
import portrait_convert

BUSTUP_ROOT = os.path.join(PROJECT_ROOT, "Asset", "graphics", "png", "chara_bustup")
SPRITE_OUT_DIR = os.path.join(PROJECT_ROOT, "Asset", "graphics", "sprites", "chara", "chara")

CANVAS = 300          # 切り出し編集キャンバスのサイズ(px)
PREVIEW_ZOOM = 2      # 64x64プレビューの拡大率


def default_prefix(char_id: str) -> str:
    if char_id.startswith("chara_"):
        return char_id[len("chara_"):]
    return char_id


class PortraitMaker:
    """親は `ttk.Frame` または `Tk`。ダイアログは `winfo_toplevel()` を親にする。"""

    def __init__(self, parent: tk.Misc):
        self.parent = parent
        self.master_win = parent.winfo_toplevel()

        # データ
        self.categories = fix_data_io.load_face_categories(PROJECT_ROOT)  # [{id,ja,variants}]
        self.characters = fix_data_io.load_characters(PROJECT_ROOT)        # CharacterEntry[]
        self.char_idx = None
        self.prefix = ""
        self.assign = {"version": 1, "prefix": "", "slots": {}}  # slots: {face_id:{src,crop}}

        # 選択中スロット編集状態
        self.cur_face_id = None
        self.src_img = None         # PIL RGBA
        self.disp_scale = 1.0
        self.disp_off = (0, 0)
        self.crop = None            # {left,top,side}
        self._drag = None
        self._tk_refs = {}          # PhotoImage参照保持

        self._build_ui()
        self._refresh_char_combo()

    # ============================================================ UI
    def _build_ui(self):
        # 上部バー
        top = ttk.Frame(self.parent, padding=6)
        top.pack(fill="x")
        ttk.Label(top, text="キャラクター:").pack(side="left")
        self.var_char = tk.StringVar()
        self.cb_char = ttk.Combobox(top, textvariable=self.var_char, state="readonly", width=28)
        self.cb_char.pack(side="left", padx=4)
        self.cb_char.bind("<<ComboboxSelected>>", lambda e: self._on_char_change())
        ttk.Button(top, text="キャラ追加", command=self._add_char).pack(side="left", padx=2)
        ttk.Button(top, text="名前/プレフィックス編集", command=self._edit_char).pack(side="left", padx=2)
        ttk.Label(top, text="   PNG格納先:").pack(side="left")
        self.lbl_folder = ttk.Label(top, text="-", foreground="#555")
        self.lbl_folder.pack(side="left")
        ttk.Button(top, text="フォルダを開く", command=self._open_folder).pack(side="left", padx=6)

        body = ttk.Frame(self.parent)
        body.pack(fill="both", expand=True, padx=6, pady=4)

        # 左: 感情カタログ
        left = ttk.LabelFrame(body, text="感情の一覧 (増やす/減らす)", padding=4)
        left.pack(side="left", fill="y")
        self.cat_tree = ttk.Treeview(left, columns=("ja", "id", "n"), show="headings",
                                     selectmode="browse", height=18)
        self.cat_tree.heading("ja", text="表示名")
        self.cat_tree.heading("id", text="ID(英字)")
        self.cat_tree.heading("n", text="数")
        self.cat_tree.column("ja", width=90)
        self.cat_tree.column("id", width=90)
        self.cat_tree.column("n", width=36, anchor="center")
        self.cat_tree.pack(fill="y", expand=False)
        cbtn = ttk.Frame(left)
        cbtn.pack(fill="x", pady=3)
        ttk.Button(cbtn, text="追加", width=5, command=self._cat_add).grid(row=0, column=0, padx=1)
        ttk.Button(cbtn, text="リネーム", width=7, command=self._cat_rename).grid(row=0, column=1, padx=1)
        ttk.Button(cbtn, text="削除", width=5, command=self._cat_delete).grid(row=0, column=2, padx=1)
        ttk.Button(cbtn, text="数 +", width=4, command=lambda: self._cat_variants(+1)).grid(row=1, column=0, padx=1, pady=2)
        ttk.Button(cbtn, text="数 -", width=4, command=lambda: self._cat_variants(-1)).grid(row=1, column=1, padx=1, pady=2)
        ttk.Button(cbtn, text="↑", width=3, command=lambda: self._cat_move(-1)).grid(row=2, column=0, padx=1)
        ttk.Button(cbtn, text="↓", width=3, command=lambda: self._cat_move(+1)).grid(row=2, column=1, padx=1)

        # 中央: スロット一覧
        mid = ttk.LabelFrame(body, text="感情スロット(PNG割り当て)", padding=4)
        mid.pack(side="left", fill="both", expand=True, padx=6)
        self.slot_tree = ttk.Treeview(mid, columns=("face", "img", "state"), show="headings",
                                      selectmode="browse")
        self.slot_tree.heading("face", text="感情")
        self.slot_tree.heading("img", text="割り当てPNG")
        self.slot_tree.heading("state", text="状態")
        self.slot_tree.column("face", width=120)
        self.slot_tree.column("img", width=220)
        self.slot_tree.column("state", width=70, anchor="center")
        self.slot_tree.pack(fill="both", expand=True)
        self.slot_tree.bind("<<TreeviewSelect>>", lambda e: self._on_slot_select())
        sbtn = ttk.Frame(mid)
        sbtn.pack(fill="x", pady=3)
        ttk.Button(sbtn, text="PNGを割り当て", command=self._assign_png).pack(side="left", padx=2)
        ttk.Button(sbtn, text="割り当てクリア", command=self._clear_png).pack(side="left", padx=2)

        # 右: 切り出し編集 + プレビュー
        right = ttk.LabelFrame(body, text="切り出し調整", padding=4)
        right.pack(side="left", fill="y")
        self.canvas = tk.Canvas(right, width=CANVAS, height=CANVAS, bg="#444",
                                highlightthickness=1, highlightbackground="#888")
        self.canvas.pack()
        self.canvas.bind("<ButtonPress-1>", self._cv_press)
        self.canvas.bind("<B1-Motion>", self._cv_drag)
        self.canvas.bind("<MouseWheel>", self._cv_wheel)

        zf = ttk.Frame(right)
        zf.pack(fill="x", pady=3)
        ttk.Label(zf, text="ズーム").pack(side="left")
        self.var_zoom = tk.DoubleVar(value=1.0)
        self.scl_zoom = ttk.Scale(zf, from_=0.3, to=3.0, variable=self.var_zoom,
                                  command=lambda e: self._on_zoom())
        self.scl_zoom.pack(side="left", fill="x", expand=True, padx=4)
        ttk.Button(zf, text="自動(顔)", command=self._auto_crop).pack(side="left")

        ttk.Label(right, text="変換プレビュー(実機の見た目)").pack(pady=(6, 0))
        self.lbl_prev = tk.Label(right, width=64 * PREVIEW_ZOOM, height=64 * PREVIEW_ZOOM, bg="#222")
        self.lbl_prev.pack(pady=4)
        ttk.Label(right, text="※ ドラッグで位置、ホイール/スライダーで拡大縮小",
                  foreground="#777").pack()

        # 下部
        bottom = ttk.Frame(self.parent, padding=6)
        bottom.pack(fill="x")
        ttk.Button(bottom, text="保存", command=self._save_all).pack(side="left")
        ttk.Button(bottom, text="この感情だけ変換", command=self._convert_current).pack(side="left", padx=4)
        ttk.Button(bottom, text="GBA素材に変換(すべて)", command=self._convert_all).pack(side="left", padx=4)
        self.lbl_status = ttk.Label(bottom, text="", foreground="#070")
        self.lbl_status.pack(side="right")

        self._refresh_cat_tree()

    # ============================================================ カタログ
    def _refresh_cat_tree(self):
        sel = self.cat_tree.selection()
        keep = sel[0] if sel else None
        for i in self.cat_tree.get_children():
            self.cat_tree.delete(i)
        for idx, c in enumerate(self.categories):
            self.cat_tree.insert("", "end", iid=str(idx),
                                 values=(c["ja"], c["id"], c.get("variants", 3)))
        if keep and self.cat_tree.exists(keep):
            self.cat_tree.selection_set(keep)
        self._refresh_slot_tree()

    def _selected_cat(self):
        sel = self.cat_tree.selection()
        return int(sel[0]) if sel else None

    def _cat_add(self):
        cid = shell.ask_string(self.master_win, "感情を追加", "感情のID(英字・記号なし。例: wink):")
        if not cid:
            return
        cid = cid.strip().lower()
        if not cid.isalnum():
            messagebox.showerror("入力エラー", "IDは英数字のみにしてください(アンダースコア不可)。", parent=self.master_win)
            return
        if any(c["id"] == cid for c in self.categories):
            messagebox.showerror("入力エラー", "そのIDは既にあります。", parent=self.master_win)
            return
        ja = shell.ask_string(self.master_win, "感情を追加", "表示名(日本語。例: ウインク):", cid)
        if ja is None:
            return
        n = shell.ask_int(self.master_win, "感情を追加", "バリエーション数(1〜):", 3)
        if n is None:
            return
        self.categories.append({"id": cid, "ja": ja, "variants": max(1, n)})
        self._refresh_cat_tree()

    def _cat_rename(self):
        i = self._selected_cat()
        if i is None:
            return
        c = self.categories[i]
        ja = shell.ask_string(self.master_win, "リネーム", "表示名(日本語):", c["ja"])
        if ja is not None:
            c["ja"] = ja
            self._refresh_cat_tree()

    def _cat_delete(self):
        i = self._selected_cat()
        if i is None:
            return
        c = self.categories[i]
        if not messagebox.askyesno("確認",
                                   f"感情「{c['ja']}」を一覧から削除しますか？\n"
                                   "(変換済みのBMPは残りますが、対応は外れます)", parent=self.master_win):
            return
        del self.categories[i]
        self._refresh_cat_tree()

    def _cat_variants(self, delta):
        i = self._selected_cat()
        if i is None:
            return
        c = self.categories[i]
        c["variants"] = max(1, int(c.get("variants", 3)) + delta)
        self._refresh_cat_tree()

    def _cat_move(self, delta):
        i = self._selected_cat()
        if i is None:
            return
        j = i + delta
        if 0 <= j < len(self.categories):
            self.categories[i], self.categories[j] = self.categories[j], self.categories[i]
            self._refresh_cat_tree()
            self.cat_tree.selection_set(str(j))

    def _face_ids(self):
        return fix_data_io.build_face_ids(self.categories)

    def _face_ja(self, face_id):
        base, _, var = face_id.rpartition("_")
        for c in self.categories:
            if c["id"] == base:
                return f"{c['ja']} {var}"
        return face_id

    # ============================================================ キャラ
    def _refresh_char_combo(self):
        names = [f"{c.name_ja} ({c.id})" for c in self.characters]
        self.cb_char["values"] = names
        if names and self.char_idx is None:
            self.char_idx = 0
            self.cb_char.current(0)
            self._load_assign()
        self._refresh_slot_tree()

    def _on_char_change(self):
        i = self.cb_char.current()
        if i < 0:
            return
        self._save_assign_silent()
        self.char_idx = i
        self._load_assign()

    def _char_folder(self):
        return os.path.join(BUSTUP_ROOT, self.prefix)

    def _load_assign(self):
        c = self.characters[self.char_idx]
        self.prefix = default_prefix(c.id)
        folder = os.path.join(BUSTUP_ROOT, self.prefix)
        path = os.path.join(folder, "_assign.json")
        if os.path.exists(path):
            try:
                with open(path, "r", encoding="utf-8") as f:
                    self.assign = json.load(f)
                self.prefix = self.assign.get("prefix", self.prefix)
            except Exception:
                self.assign = {"version": 1, "prefix": self.prefix, "slots": {}}
        else:
            self.assign = {"version": 1, "prefix": self.prefix, "slots": {}}
        self.assign.setdefault("slots", {})
        self.lbl_folder.config(text=os.path.relpath(self._char_folder(), PROJECT_ROOT))
        self.cur_face_id = None
        self._refresh_slot_tree()
        self._clear_editor()

    def _add_char(self):
        cid = shell.ask_string(self.master_win, "キャラ追加", "キャラID(例: chara_riri):")
        if not cid:
            return
        cid = cid.strip()
        if any(c.id == cid for c in self.characters):
            messagebox.showerror("入力エラー", "そのIDは既にあります。", parent=self.master_win)
            return
        name = shell.ask_string(self.master_win, "キャラ追加", "表示名(日本語):", "")
        if name is None:
            return
        self.characters.append(fix_data_io.CharacterEntry(id=cid, name_ja=name, faces={}))
        self._refresh_char_combo()
        self.cb_char.current(len(self.characters) - 1)
        self._on_char_change()

    def _edit_char(self):
        if self.char_idx is None:
            return
        c = self.characters[self.char_idx]
        name = shell.ask_string(self.master_win, "キャラ編集", "表示名(日本語):", c.name_ja)
        if name is not None:
            c.name_ja = name
        pfx = shell.ask_string(self.master_win, "キャラ編集",
                               "画像プレフィックス(英字。image_id/BMP名の先頭になる):", self.prefix)
        if pfx:
            self.prefix = pfx.strip().lower()
            self.assign["prefix"] = self.prefix
            self.lbl_folder.config(text=os.path.relpath(self._char_folder(), PROJECT_ROOT))
        self._refresh_char_combo()

    def _open_folder(self):
        folder = self._char_folder()
        os.makedirs(folder, exist_ok=True)
        try:
            os.startfile(folder)  # Windows
        except Exception:
            messagebox.showinfo("フォルダ", folder, parent=self.master_win)

    # ============================================================ スロット
    def _refresh_slot_tree(self):
        for i in self.slot_tree.get_children():
            self.slot_tree.delete(i)
        for fid in self._face_ids():
            slot = self.assign.get("slots", {}).get(fid)
            src = slot.get("src", "") if slot else ""
            if not src:
                state = "未割当"
            else:
                bmp = os.path.join(SPRITE_OUT_DIR, f"spr_ch_{self.prefix}_{fid}.bmp")
                state = "変換済" if os.path.exists(bmp) else "未変換"
            self.slot_tree.insert("", "end", iid=fid,
                                  values=(self._face_ja(fid), src, state))

    def _on_slot_select(self):
        sel = self.slot_tree.selection()
        if not sel:
            return
        self.cur_face_id = sel[0]
        self._load_slot_editor()

    def _resolve_src_path(self, src):
        if not src:
            return None
        p = os.path.join(self._char_folder(), src)
        if os.path.exists(p):
            return p
        if os.path.isabs(src) and os.path.exists(src):
            return src
        p2 = os.path.join(PROJECT_ROOT, src)
        return p2 if os.path.exists(p2) else None

    def _load_slot_editor(self):
        self._clear_editor()
        slot = self.assign.get("slots", {}).get(self.cur_face_id)
        if not slot or not slot.get("src"):
            return
        path = self._resolve_src_path(slot["src"])
        if not path:
            self.lbl_prev.config(text="画像が見つかりません")
            return
        try:
            self.src_img = Image.open(path).convert("RGBA")
        except Exception as e:
            messagebox.showerror("読込エラー", str(e), parent=self.master_win)
            return
        self.crop = slot.get("crop") or portrait_convert.default_crop(self.src_img)
        self._sync_zoom_from_crop()
        self._redraw()

    def _clear_editor(self):
        self.src_img = None
        self.crop = None
        self.canvas.delete("all")
        self.lbl_prev.config(image="", text="")
        self._tk_refs.clear()

    def _assign_png(self):
        if self.cur_face_id is None:
            messagebox.showinfo("案内", "先に左の感情スロットを選んでください。", parent=self.master_win)
            return
        folder = self._char_folder()
        os.makedirs(folder, exist_ok=True)
        path = filedialog.askopenfilename(
            parent=self.master_win,
            title="PNGを選択", initialdir=folder,
            filetypes=[("画像", "*.png *.webp *.jpg *.jpeg"), ("すべて", "*.*")])
        if not path:
            return
        # 同フォルダ内ならファイル名のみ、外なら絶対パスで保存
        try:
            rel = os.path.relpath(path, folder)
        except ValueError:
            rel = path
        src = rel if not rel.startswith("..") else path
        self.assign.setdefault("slots", {})[self.cur_face_id] = {"src": src, "crop": None}
        self._refresh_slot_tree()
        self.slot_tree.selection_set(self.cur_face_id)
        self._load_slot_editor()

    def _clear_png(self):
        if self.cur_face_id is None:
            return
        self.assign.get("slots", {}).pop(self.cur_face_id, None)
        self._refresh_slot_tree()
        self.slot_tree.selection_set(self.cur_face_id)
        self._clear_editor()

    # ============================================================ crop編集
    def _sync_zoom_from_crop(self):
        if not self.src_img or not self.crop:
            return
        base = self.src_img.split()[3].getbbox()
        base_w = (base[2] - base[0]) if base else self.src_img.width
        z = base_w / max(1, self.crop["side"])
        self.var_zoom.set(max(0.3, min(3.0, z)))

    def _redraw(self):
        self.canvas.delete("all")
        if not self.src_img:
            return
        w, h = self.src_img.size
        self.disp_scale = min(CANVAS / w, CANVAS / h)
        dw, dh = int(w * self.disp_scale), int(h * self.disp_scale)
        ox, oy = (CANVAS - dw) // 2, (CANVAS - dh) // 2
        self.disp_off = (ox, oy)
        disp = self.src_img.convert("RGBA")
        bg = Image.new("RGBA", disp.size, (80, 80, 90, 255))
        bg.alpha_composite(disp)
        disp = bg.convert("RGB").resize((dw, dh), Image.LANCZOS)
        ph = ImageTk.PhotoImage(disp)
        self._tk_refs["canvas"] = ph
        self.canvas.create_image(ox, oy, anchor="nw", image=ph)
        # crop枠
        c = self.crop
        x0 = ox + c["left"] * self.disp_scale
        y0 = oy + c["top"] * self.disp_scale
        s = c["side"] * self.disp_scale
        self.canvas.create_rectangle(x0, y0, x0 + s, y0 + s, outline="#ff4", width=2)
        self._update_preview()

    def _update_preview(self):
        if not self.src_img or not self.crop:
            return
        prev = portrait_convert.render_preview_rgba(self.src_img, self.crop)
        prev = prev.resize((64 * PREVIEW_ZOOM, 64 * PREVIEW_ZOOM), Image.NEAREST)
        bg = Image.new("RGBA", prev.size, (34, 34, 34, 255))
        bg.alpha_composite(prev)
        ph = ImageTk.PhotoImage(bg.convert("RGB"))
        self._tk_refs["prev"] = ph
        self.lbl_prev.config(image=ph, text="")

    def _cv_press(self, e):
        self._drag = (e.x, e.y)

    def _cv_drag(self, e):
        if not self.crop or not self._drag or self.disp_scale <= 0:
            return
        dx = (e.x - self._drag[0]) / self.disp_scale
        dy = (e.y - self._drag[1]) / self.disp_scale
        self.crop["left"] = int(self.crop["left"] + dx)
        self.crop["top"] = int(self.crop["top"] + dy)
        self._drag = (e.x, e.y)
        self._redraw()
        self._mark_dirty_slot()

    def _cv_wheel(self, e):
        if not self.crop:
            return
        factor = 0.9 if e.delta > 0 else 1.1  # 上回し=ズームイン(sideを小さく)
        self._zoom_to(self.crop["side"] * factor)

    def _on_zoom(self):
        if not self.src_img or not self.crop:
            return
        base = self.src_img.split()[3].getbbox()
        base_w = (base[2] - base[0]) if base else self.src_img.width
        side = base_w / max(0.01, self.var_zoom.get())
        self._zoom_to(side)

    def _zoom_to(self, new_side):
        c = self.crop
        cx = c["left"] + c["side"] / 2
        cy = c["top"] + c["side"] / 2
        new_side = max(8, int(new_side))
        c["side"] = new_side
        c["left"] = int(cx - new_side / 2)
        c["top"] = int(cy - new_side / 2)
        self._redraw()
        self._mark_dirty_slot()

    def _auto_crop(self):
        if not self.src_img:
            return
        self.crop = portrait_convert.default_crop(self.src_img)
        self._sync_zoom_from_crop()
        self._redraw()
        self._mark_dirty_slot()

    def _mark_dirty_slot(self):
        if self.cur_face_id and self.crop:
            slot = self.assign.setdefault("slots", {}).setdefault(self.cur_face_id, {"src": "", "crop": None})
            slot["crop"] = dict(self.crop)

    # ============================================================ 保存/変換
    def _save_assign_silent(self):
        if not self.prefix:
            return
        folder = self._char_folder()
        os.makedirs(folder, exist_ok=True)
        self.assign["prefix"] = self.prefix
        with open(os.path.join(folder, "_assign.json"), "w", encoding="utf-8") as f:
            json.dump(self.assign, f, ensure_ascii=False, indent=2)

    def _save_all(self):
        fix_data_io.save_face_categories(PROJECT_ROOT, self.categories)
        self._save_assign_silent()
        fix_data_io.save_characters(PROJECT_ROOT, self.characters)
        self._status("保存しました")

    def _convert_slot(self, fid) -> bool:
        slot = self.assign.get("slots", {}).get(fid)
        if not slot or not slot.get("src"):
            return False
        path = self._resolve_src_path(slot["src"])
        if not path:
            return False
        out = os.path.join(SPRITE_OUT_DIR, f"spr_ch_{self.prefix}_{fid}.bmp")
        os.makedirs(SPRITE_OUT_DIR, exist_ok=True)
        crop = slot.get("crop")
        portrait_convert.convert_file(path, out, crop)
        # characters.json の faces に反映
        c = self.characters[self.char_idx]
        c.faces[fid] = f"{self.prefix}_{fid}"
        return True

    def _convert_current(self):
        if self.cur_face_id is None:
            return
        if self._convert_slot(self.cur_face_id):
            self._post_convert(1)
        else:
            messagebox.showinfo("案内", "この感情にはPNGが割り当てられていません。", parent=self.master_win)

    def _convert_all(self):
        n = 0
        for fid in self._face_ids():
            if self._convert_slot(fid):
                n += 1
        if n == 0:
            messagebox.showinfo("案内", "割り当て済みのPNGがありません。", parent=self.master_win)
            return
        self._post_convert(n)

    def _post_convert(self, n):
        # 保存 + 対応表(コード生成)を更新
        self._save_all()
        try:
            subprocess.run([sys.executable, os.path.join(TOOLS_DIR, "generate_chara_portraits.py")],
                           check=True, cwd=PROJECT_ROOT)
            subprocess.run([sys.executable, os.path.join(TOOLS_DIR, "generate_fix_data.py")],
                           check=True, cwd=PROJECT_ROOT)
        except Exception as e:
            messagebox.showwarning("コード生成", f"BMPは作成しましたが、コード生成で警告:\n{e}", parent=self.master_win)
        self._refresh_slot_tree()
        if self.cur_face_id:
            self.slot_tree.selection_set(self.cur_face_id)
        messagebox.showinfo("変換完了",
                            f"{n}枚のGBA素材(BMP)を作成しました。\n"
                            "ゲームをビルドすると反映されます。", parent=self.master_win)

    def _status(self, msg):
        self.lbl_status.config(text=msg)
        self.master_win.after(2500, lambda: self.lbl_status.config(text=""))

    def on_closing(self):
        """タブ切替時は呼ばない想定。ウィンドウ終了時は外から _save_assign_silent のみ。"""
        self._save_assign_silent()


if __name__ == "__main__":
    from asset_bake_tab import AssetBakeTab

    root = tk.Tk()
    root.title("GBA アセットメーカー（立ち絵 / 汎用焼き）")
    root.geometry("1200x780")

    nb = ttk.Notebook(root)
    nb.pack(fill="both", expand=True)

    tab_port = ttk.Frame(nb)
    tab_bake = ttk.Frame(nb)
    nb.add(tab_port, text="立ち絵")
    nb.add(tab_bake, text="汎用焼き")

    app = PortraitMaker(tab_port)
    AssetBakeTab(tab_bake, root).pack(fill="both", expand=True)

    def _on_root_close():
        app.on_closing()
        root.destroy()

    root.protocol("WM_DELETE_WINDOW", _on_root_close)
    root.mainloop()
