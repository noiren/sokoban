#!/usr/bin/env python3
"""
sprite_anim_editor.py
スプライトアニメーション（コマ送り）のマスターデータを編集するGUI。
タイムラインベースのグラフィカルなUIを提供。
"""
import json
import os
import copy
import tkinter as tk
from tkinter import ttk, messagebox, filedialog
from PIL import Image, ImageTk

SCRIPT_DIR   = os.path.dirname(os.path.abspath(__file__))
PROJECT_ROOT = os.path.dirname(os.path.dirname(SCRIPT_DIR))
MANIFEST_PATH = os.path.join(PROJECT_ROOT, "Asset", "fixdata", "sprite_animations.json")
SPRITES_DIR   = os.path.join(PROJECT_ROOT, "Asset", "graphics", "sprites")

def get_se_ids():
    audio_manifest = os.path.join(PROJECT_ROOT, "Asset", "audio", "audio_manifest.json")
    try:
        with open(audio_manifest, encoding="utf-8") as f:
            data = json.load(f)
        ids = [""]
        for se in data.get("se", []):
            cat = se.get("category", "Default")
            id_  = se.get("id", "")
            ids.append(f"{cat}_{id_}")
        return ids
    except Exception:
        return [""]

def find_sprite_bmp(sprite_name: str) -> str | None:
    for root, _, files in os.walk(SPRITES_DIR):
        if f"{sprite_name}.bmp" in files:
            return os.path.join(root, f"{sprite_name}.bmp")
    return None

def load_manifest() -> dict:
    if not os.path.exists(MANIFEST_PATH):
        return {"version": 1, "animations": []}
    with open(MANIFEST_PATH, encoding="utf-8") as f:
        return json.load(f)

def save_manifest(data: dict):
    os.makedirs(os.path.dirname(MANIFEST_PATH), exist_ok=True)
    with open(MANIFEST_PATH, "w", encoding="utf-8") as f:
        json.dump(data, f, indent=2, ensure_ascii=False)


class SpriteAnimEditor:
    def __init__(self, root: tk.Tk):
        self.root = root
        self.root.title("スプライトアニメーション エディタ (タイムライン版)")
        self.root.geometry("1100x750")

        self.manifest = load_manifest()
        self.anims    = self.manifest.get("animations", [])
        self.original = copy.deepcopy(self.anims)
        self.se_ids   = get_se_ids()

        self.sel_anim_idx = None
        self.sel_clip_idx = None

        # 画像キャッシュ
        self.thumbnail_cache = {}  # {sprite_name: PhotoImage}

        # プレビュー制御
        self._preview_frames = []
        self._preview_frame_idx = 0
        self._preview_after_id = None
        self._preview_running = False

        self._build_ui()
        self._refresh_anim_list()

    def _build_ui(self):
        style = ttk.Style()
        style.configure("TButton", padding=4)
        style.configure("TLabelframe", padding=6)
        
        paned = ttk.PanedWindow(self.root, orient=tk.HORIZONTAL)
        paned.pack(fill=tk.BOTH, expand=True, padx=6, pady=6)

        # ---------------- 左ペイン：アニメーション一覧 ----------------
        left = ttk.Frame(paned)
        paned.add(left, weight=1)

        ttk.Label(left, text="📂 アニメーション一覧", font=("", 10, "bold")).pack(anchor="w", pady=(0,4))
        
        list_frame = ttk.Frame(left)
        list_frame.pack(fill=tk.BOTH, expand=True)
        
        scroll = ttk.Scrollbar(list_frame)
        scroll.pack(side=tk.RIGHT, fill=tk.Y)
        
        self.anim_list = tk.Listbox(list_frame, selectmode=tk.SINGLE, yscrollcommand=scroll.set, font=("Consolas", 10))
        self.anim_list.pack(side=tk.LEFT, fill=tk.BOTH, expand=True)
        scroll.config(command=self.anim_list.yview)
        self.anim_list.bind("<<ListboxSelect>>", self._on_anim_select)

        btn_f = ttk.Frame(left)
        btn_f.pack(fill=tk.X, pady=4)
        ttk.Button(btn_f, text="➕ 新規作成", command=self._add_anim).pack(side=tk.LEFT, padx=2)
        ttk.Button(btn_f, text="🗑️ 削除", command=self._del_anim).pack(side=tk.LEFT, padx=2)
        ttk.Button(btn_f, text="💾 保存", command=self._save_all).pack(side=tk.RIGHT, padx=2)

        # ---------------- 右ペイン：詳細とタイムライン ----------------
        right = ttk.Frame(paned)
        paned.add(right, weight=4)

        # --- アニメーション基本設定 ---
        anim_frame = ttk.LabelFrame(right, text="⚙️ アニメーション設定")
        anim_frame.pack(fill=tk.X, padx=4, pady=4)

        ttk.Label(anim_frame, text="アニメーションID:").grid(row=0, column=0, sticky="e", padx=4, pady=4)
        self.var_id = tk.StringVar()
        self.ent_id = ttk.Entry(anim_frame, textvariable=self.var_id, width=30)
        self.ent_id.grid(row=0, column=1, sticky="w", padx=4)
        self.var_id.trace("w", lambda *_: self._sync_anim())

        ttk.Label(anim_frame, text="開始時SE:").grid(row=0, column=2, sticky="e", padx=4)
        self.var_se = tk.StringVar()
        self.cb_se  = ttk.Combobox(anim_frame, textvariable=self.var_se, values=self.se_ids, state="readonly", width=25)
        self.cb_se.grid(row=0, column=3, sticky="w", padx=4)
        self.cb_se.bind("<<ComboboxSelected>>", lambda *_: self._sync_anim())

        ttk.Label(anim_frame, text="デフォルトループ回数:").grid(row=0, column=4, sticky="e", padx=4)
        self.var_loop = tk.StringVar(value="2")
        ttk.Entry(anim_frame, textvariable=self.var_loop, width=6).grid(row=0, column=5, padx=4)
        self.var_loop.trace("w", lambda *_: self._sync_anim())
        ttk.Label(anim_frame, text="(-1で無限ループ)", foreground="gray").grid(row=0, column=6, sticky="w")

        # --- タイムライン ---
        tl_frame = ttk.LabelFrame(right, text="🎬 タイムライン (クリップ構成)")
        tl_frame.pack(fill=tk.BOTH, expand=True, padx=4, pady=4)
        
        tl_btn_f = ttk.Frame(tl_frame)
        tl_btn_f.pack(fill=tk.X, pady=(0, 4))
        ttk.Button(tl_btn_f, text="➕ クリップを追加", command=self._add_clip).pack(side=tk.LEFT, padx=2)
        ttk.Button(tl_btn_f, text="🗑️ クリップを削除", command=self._del_clip).pack(side=tk.LEFT, padx=2)
        ttk.Button(tl_btn_f, text="◀ 左へ移動", command=lambda: self._move_clip(-1)).pack(side=tk.LEFT, padx=2)
        ttk.Button(tl_btn_f, text="▶ 右へ移動", command=lambda: self._move_clip(1)).pack(side=tk.LEFT, padx=2)
        
        self.tl_canvas = tk.Canvas(tl_frame, bg="#2d2d2d", height=120)
        self.tl_canvas.pack(fill=tk.X, padx=2, pady=2)
        self.tl_canvas.bind("<Button-1>", self._on_tl_click)

        # --- クリップ編集 & プレビュー ---
        bottom_f = ttk.Frame(right)
        bottom_f.pack(fill=tk.X, padx=4, pady=4)
        
        clip_edit = ttk.LabelFrame(bottom_f, text="✏️ 選択中のクリップを編集")
        clip_edit.pack(side=tk.LEFT, fill=tk.BOTH, expand=True, padx=(0, 4))

        def labeled(parent, text, row, col, var, w=10):
            ttk.Label(parent, text=text).grid(row=row, column=col*2, sticky="e", padx=4, pady=6)
            ttk.Entry(parent, textvariable=var, width=w).grid(row=row, column=col*2+1, sticky="w", padx=4)

        self.var_clip_sprite = tk.StringVar()
        self.var_clip_frames = tk.StringVar(value="4")
        self.var_clip_dur    = tk.StringVar(value="6")
        self.var_clip_w      = tk.StringVar(value="16")
        self.var_clip_h      = tk.StringVar(value="16")

        ttk.Label(clip_edit, text="画像 (Sprite):").grid(row=0, column=0, sticky="e", padx=4, pady=6)
        ttk.Entry(clip_edit, textvariable=self.var_clip_sprite, width=24).grid(row=0, column=1, columnspan=2, sticky="w", padx=4)
        ttk.Button(clip_edit, text="📂 参照...", command=self._browse_sprite).grid(row=0, column=3, padx=4)
        
        labeled(clip_edit, "コマ数 (枚):", 1, 0, self.var_clip_frames)
        labeled(clip_edit, "表示速度 (f/コマ):", 1, 1, self.var_clip_dur)
        labeled(clip_edit, "横幅 (px):", 2, 0, self.var_clip_w)
        labeled(clip_edit, "縦幅 (px):", 2, 1, self.var_clip_h)

        ttk.Button(clip_edit, text="✔️ 適用して更新", command=self._apply_clip).grid(row=3, column=0, columnspan=4, pady=8)

        # --- プレビュー ---
        prev_f = ttk.LabelFrame(bottom_f, text="📺 アニメーション・プレビュー")
        prev_f.pack(side=tk.RIGHT, fill=tk.BOTH, expand=True, padx=(4, 0))

        prev_ctrl = ttk.Frame(prev_f)
        prev_ctrl.pack(fill=tk.X, pady=4)
        ttk.Button(prev_ctrl, text="▶ 再生", command=self._start_preview).pack(side=tk.LEFT, padx=4)
        ttk.Button(prev_ctrl, text="■ 停止", command=self._stop_preview).pack(side=tk.LEFT)
        self.lbl_prev_info = ttk.Label(prev_ctrl, text="停止中", foreground="gray")
        self.lbl_prev_info.pack(side=tk.LEFT, padx=10)

        self.prev_canvas = tk.Canvas(prev_f, bg="#1a1a2e", width=200, height=200)
        self.prev_canvas.pack(fill=tk.BOTH, expand=True, padx=4, pady=4)

    # ------------------------------------------------------------------ アニメ一覧
    def _refresh_anim_list(self, keep_sel: bool = False):
        old_sel = self.sel_anim_idx
        self.anim_list.delete(0, tk.END)
        for anim in self.anims:
            lp = anim.get("default_loop_count", 1)
            lp_str = "∞" if lp < 0 else str(lp)
            self.anim_list.insert(tk.END, f" {anim.get('id','')} (ループ: {lp_str})")
        
        if keep_sel and old_sel is not None and old_sel < len(self.anims):
            self.anim_list.selection_set(old_sel)
            self.sel_anim_idx = old_sel
        else:
            self._draw_timeline()

    def _on_anim_select(self, _=None):
        sel = self.anim_list.curselection()
        if not sel:
            return
        self.sel_anim_idx = int(sel[0])
        self.sel_clip_idx = None
        self._load_anim_to_form()
        self._draw_timeline()
        self._clear_clip_form()
        self._stop_preview()
        self._prepare_preview_sequence()

    def _load_anim_to_form(self):
        anim = self.anims[self.sel_anim_idx]
        self.var_id.set(anim.get("id", ""))
        self.var_se.set(anim.get("se_id", ""))
        self.var_loop.set(str(anim.get("default_loop_count", 1)))

    def _sync_anim(self):
        if self.sel_anim_idx is None:
            return
        anim = self.anims[self.sel_anim_idx]
        anim["id"] = self.var_id.get()
        anim["se_id"] = self.var_se.get()
        try:
            anim["default_loop_count"] = int(self.var_loop.get())
        except ValueError:
            pass
        self._refresh_anim_list(keep_sel=True)

    def _add_anim(self):
        new = {"id": f"new_anim_{len(self.anims)}", "clips": [], "default_loop_count": 2, "se_id": ""}
        self.anims.append(new)
        self._refresh_anim_list()
        self.anim_list.selection_set(len(self.anims) - 1)
        self._on_anim_select()

    def _del_anim(self):
        if self.sel_anim_idx is None:
            return
        if not messagebox.askyesno("確認", "このアニメーションを削除しますか？"):
            return
        self.anims.pop(self.sel_anim_idx)
        self.sel_anim_idx = None
        self._refresh_anim_list()
        self._draw_timeline()

    # ------------------------------------------------------------------ タイムライン
    def _draw_timeline(self):
        self.tl_canvas.delete("all")
        if self.sel_anim_idx is None:
            self.tl_canvas.create_text(500, 60, text="アニメーションを選択してください", fill="gray", font=("", 12))
            return
            
        anim = self.anims[self.sel_anim_idx]
        clips = anim.get("clips", [])
        
        if not clips:
            self.tl_canvas.create_text(500, 60, text="クリップがありません。「追加」ボタンから追加してください", fill="gray", font=("", 10))
            return

        x = 10
        y = 20
        height = 80
        
        # 描画スケール（1フレームあたりのピクセル幅）
        time_scale = 2.0 
        
        self.tl_rects = [] # (x1, x2, clip_idx)
        
        # タイムラインのメモリ描画
        self.tl_canvas.create_line(0, y+height+5, 2000, y+height+5, fill="#555")
        
        for i, clip in enumerate(clips):
            frames = clip.get("frame_count", 1)
            duration = clip.get("frame_duration", 6)
            total_frames = frames * duration
            
            w = max(40, int(total_frames * time_scale))
            
            is_sel = (i == self.sel_clip_idx)
            bg_color = "#3a6ea5" if is_sel else "#4a4a4a"
            outline = "#88c0d0" if is_sel else "#222"
            
            # クリップの枠
            rect = self.tl_canvas.create_rectangle(x, y, x + w, y + height, fill=bg_color, outline=outline, width=2 if is_sel else 1)
            
            # サムネイル表示（可能なら）
            sprite_name = clip.get("sprite", "")
            img = self._get_cached_thumbnail(sprite_name)
            if img:
                # 中央に配置
                self.tl_canvas.create_image(x + w//2, y + height//2, image=img)
            else:
                self.tl_canvas.create_text(x + w//2, y + height//2, text=sprite_name, fill="white", width=w-10, justify=tk.CENTER)
            
            # 上部に情報
            self.tl_canvas.create_text(x + 4, y + 4, text=f"[{i+1}] {total_frames}f", fill="#aaa", anchor="nw", font=("", 8))
            
            self.tl_rects.append((x, x + w, i))
            x += w + 2
            
    def _on_tl_click(self, event):
        if self.sel_anim_idx is None: return
        clicked_idx = None
        for (x1, x2, idx) in getattr(self, "tl_rects", []):
            if x1 <= event.x <= x2:
                clicked_idx = idx
                break
        
        if clicked_idx is not None:
            self.sel_clip_idx = clicked_idx
            self._load_clip_to_form()
            self._draw_timeline()
        else:
            self.sel_clip_idx = None
            self._clear_clip_form()
            self._draw_timeline()

    def _get_cached_thumbnail(self, sprite_name):
        if not sprite_name: return None
        if sprite_name in self.thumbnail_cache:
            return self.thumbnail_cache[sprite_name]
            
        bmp = find_sprite_bmp(sprite_name)
        if bmp:
            try:
                img = Image.open(bmp).convert("RGBA")
                
                # マゼンタ透過処理
                data = img.load()
                w, h = img.size
                for _y in range(h):
                    for _x in range(w):
                        if data[_x, _y][:3] == (255, 0, 255):
                            data[_x, _y] = (0, 0, 0, 0)
                            
                # サイズ調整（高さ40px程度に）
                ratio = 40.0 / float(h)
                new_w = max(1, int(w * ratio))
                # クリップが横に長すぎる場合は最初の1コマ分くらいにするための制限
                if new_w > 80:
                    img = img.crop((0, 0, int(80/ratio), h))
                    new_w = 80
                    
                img = img.resize((new_w, 40), Image.NEAREST)
                photo = ImageTk.PhotoImage(img)
                self.thumbnail_cache[sprite_name] = photo
                return photo
            except Exception:
                pass
        return None

    # ------------------------------------------------------------------ クリップ編集
    def _clear_clip_form(self):
        self.var_clip_sprite.set("")
        self.var_clip_frames.set("1")
        self.var_clip_dur.set("6")
        self.var_clip_w.set("16")
        self.var_clip_h.set("16")

    def _load_clip_to_form(self):
        if self.sel_anim_idx is None or self.sel_clip_idx is None:
            return
        clip = self.anims[self.sel_anim_idx]["clips"][self.sel_clip_idx]
        self.var_clip_sprite.set(clip.get("sprite",""))
        self.var_clip_frames.set(str(clip.get("frame_count",1)))
        self.var_clip_dur.set(str(clip.get("frame_duration",6)))
        self.var_clip_w.set(str(clip.get("width",16)))
        self.var_clip_h.set(str(clip.get("height",16)))

    def _apply_clip(self):
        if self.sel_anim_idx is None or self.sel_clip_idx is None:
            messagebox.showwarning("警告", "タイムライン上でクリップを選択してください。")
            return
        clip = self.anims[self.sel_anim_idx]["clips"][self.sel_clip_idx]
        clip["sprite"]         = self.var_clip_sprite.get()
        clip["frame_count"]    = int(self.var_clip_frames.get() or 1)
        clip["frame_duration"] = int(self.var_clip_dur.get() or 6)
        clip["width"]          = int(self.var_clip_w.get() or 16)
        clip["height"]         = int(self.var_clip_h.get() or 16)
        
        # キャッシュクリア
        if clip["sprite"] in self.thumbnail_cache:
            del self.thumbnail_cache[clip["sprite"]]
            
        self._draw_timeline()
        self._prepare_preview_sequence()

    def _browse_sprite(self):
        path = filedialog.askopenfilename(
            initialdir=SPRITES_DIR,
            title="スプライト画像を選択",
            filetypes=[("BMP files", "*.bmp")]
        )
        if not path: return
        
        base = os.path.splitext(os.path.basename(path))[0]
        self.var_clip_sprite.set(base)
        
        try:
            img = Image.open(path)
            w, h = img.size
            frames = max(1, int(self.var_clip_frames.get() or 1))
            self.var_clip_w.set(str(w // frames))
            self.var_clip_h.set(str(h))
        except Exception:
            pass

    def _add_clip(self):
        if self.sel_anim_idx is None:
            messagebox.showwarning("警告", "アニメーションが選択されていません。")
            return
        self.anims[self.sel_anim_idx].setdefault("clips", []).append(
            {"sprite":"", "frame_count":4, "frame_duration":6, "width":16, "height":16})
        self.sel_clip_idx = len(self.anims[self.sel_anim_idx]["clips"]) - 1
        self._load_clip_to_form()
        self._draw_timeline()
        self._prepare_preview_sequence()

    def _del_clip(self):
        if self.sel_anim_idx is None or self.sel_clip_idx is None:
            return
        self.anims[self.sel_anim_idx]["clips"].pop(self.sel_clip_idx)
        self.sel_clip_idx = None
        self._clear_clip_form()
        self._draw_timeline()
        self._prepare_preview_sequence()

    def _move_clip(self, delta: int):
        if self.sel_anim_idx is None or self.sel_clip_idx is None:
            return
        clips = self.anims[self.sel_anim_idx]["clips"]
        i = self.sel_clip_idx
        j = i + delta
        if 0 <= j < len(clips):
            clips[i], clips[j] = clips[j], clips[i]
            self.sel_clip_idx = j
            self._draw_timeline()
            self._prepare_preview_sequence()

    # ------------------------------------------------------------------ プレビュー
    def _prepare_preview_sequence(self):
        self._stop_preview()
        self.prev_canvas.delete("all")
        self._preview_sequence = [] # List of (PhotoImage, duration_ms)
        
        if self.sel_anim_idx is None:
            return
            
        anim = self.anims[self.sel_anim_idx]
        clips = anim.get("clips", [])
        
        for clip in clips:
            sprite_name = clip.get("sprite", "")
            frame_count = int(clip.get("frame_count", 1))
            duration_f = int(clip.get("frame_duration", 6))
            sprite_h = int(clip.get("height", 16))
            
            bmp = find_sprite_bmp(sprite_name)
            if not bmp: continue
            
            try:
                img = Image.open(bmp).convert("RGBA")
                data = img.load()
                w, h = img.size
                for y in range(h):
                    for x in range(w):
                        if data[x, y][:3] == (255, 0, 255):
                            data[x, y] = (0, 0, 0, 0)
                
                frame_w = max(1, w // frame_count)
                # 200x200のキャンバスに合わせて拡大
                scale = min(200 // frame_w, 200 // sprite_h)
                if scale < 1: scale = 1
                if scale > 6: scale = 6
                
                ms = max(16, duration_f * 16) # GBA is ~60fps
                
                for i in range(frame_count):
                    left = i * frame_w
                    right = left + frame_w
                    crop = img.crop((left, 0, right, sprite_h))
                    if scale > 1:
                        crop = crop.resize((frame_w * scale, sprite_h * scale), Image.NEAREST)
                    self._preview_sequence.append((ImageTk.PhotoImage(crop), ms))
                    
            except Exception:
                continue
                
        if self._preview_sequence:
            self._draw_preview_frame(0)
            self.lbl_prev_info.config(text=f"準備完了 ({len(self._preview_sequence)} フレーム)", foreground="white")
        else:
            self.lbl_prev_info.config(text="プレビュー不可", foreground="red")

    def _draw_preview_frame(self, idx: int):
        self.prev_canvas.delete("all")
        if not self._preview_sequence: return
        
        photo, _ = self._preview_sequence[idx]
        
        # 中央に描画
        cx = 100
        cy = 100
        self.prev_canvas.create_image(cx, cy, anchor=tk.CENTER, image=photo)

    def _start_preview(self):
        if not hasattr(self, "_preview_sequence") or not self._preview_sequence:
            self._prepare_preview_sequence()
        if not self._preview_sequence:
            return
            
        self._preview_running = True
        self._preview_frame_idx = 0
        self.lbl_prev_info.config(text="▶ 再生中...", foreground="#88c0d0")
        self._animate_preview()

    def _stop_preview(self):
        self._preview_running = False
        if self._preview_after_id:
            self.root.after_cancel(self._preview_after_id)
            self._preview_after_id = None
        self.lbl_prev_info.config(text="■ 停止中", foreground="gray")

    def _animate_preview(self):
        if not self._preview_running or not self._preview_sequence:
            return
            
        self._draw_preview_frame(self._preview_frame_idx)
        
        _, ms = self._preview_sequence[self._preview_frame_idx]
        
        self._preview_frame_idx = (self._preview_frame_idx + 1) % len(self._preview_sequence)
        self._preview_after_id = self.root.after(ms, self._animate_preview)

    # ------------------------------------------------------------------ 保存と終了
    def _is_dirty(self):
        return self.anims != self.original

    def _save_all(self):
        self.manifest["animations"] = self.anims
        save_manifest(self.manifest)
        self.original = copy.deepcopy(self.anims)
        messagebox.showinfo("完了", "アニメーションデータを保存しました。")

    def on_closing(self):
        if self._is_dirty():
            res = messagebox.askyesnocancel("確認", "保存されていない変更があります。保存してから終了しますか？")
            if res is None:
                return
            if res:
                self._save_all()
        self._stop_preview()
        self.root.destroy()


if __name__ == "__main__":
    root = tk.Tk()
    # Windows高DPI設定
    try:
        from ctypes import windll
        windll.shcore.SetProcessDpiAwareness(1)
    except:
        pass
    app = SpriteAnimEditor(root)
    root.protocol("WM_DELETE_WINDOW", app.on_closing)
    root.mainloop()
