#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""汎用焼きタブ（PNG→GBA用 BMP）。portrait_maker から Notebook で埋め込む。"""
import os
import sys
import tkinter as tk
from tkinter import ttk, filedialog, messagebox, scrolledtext
from PIL import Image, ImageTk

SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))
PROJECT_ROOT = os.path.dirname(os.path.dirname(SCRIPT_DIR))
TOOLS_DIR = os.path.join(PROJECT_ROOT, "src", "tools")
GRAPHICS_DIR = os.path.join(PROJECT_ROOT, "Asset", "graphics")

if TOOLS_DIR not in sys.path:
    sys.path.insert(0, TOOLS_DIR)

import gba_asset_bake


class AssetBakeTab(ttk.Frame):
    def __init__(self, parent, master_win: tk.Misc):
        super().__init__(parent)
        self.master_win = master_win
        self.src_path = ""
        self.src_rgba = None  # PIL RGBA
        self._tk_prev = None

        pad = {"padx": 6, "pady": 4}
        row = ttk.Frame(self)
        row.pack(fill="x", **pad)
        ttk.Label(row, text="入力PNG/WebP等:").pack(side="left")
        self.var_src = tk.StringVar()
        ttk.Entry(row, textvariable=self.var_src, width=56).pack(side="left", fill="x", expand=True, padx=4)
        ttk.Button(row, text="参照…", command=self._pick_src).pack(side="left")

        row2 = ttk.Frame(self)
        row2.pack(fill="x", **pad)
        ttk.Label(row2, text="出力種別:").pack(side="left")
        self.var_mode = tk.StringVar(value="sprite")
        ttk.Radiobutton(
            row2, text="スプライト(16色・透過idx0)", variable=self.var_mode, value="sprite"
        ).pack(side="left", padx=4)
        ttk.Radiobutton(
            row2, text="スチル背景(256色・idx0予約)", variable=self.var_mode, value="still"
        ).pack(side="left", padx=4)

        row3 = ttk.Frame(self)
        row3.pack(fill="x", **pad)
        ttk.Label(row3, text="幅×高さ:").pack(side="left")
        self.var_w = tk.StringVar(value="64")
        self.var_h = tk.StringVar(value="64")
        ttk.Entry(row3, textvariable=self.var_w, width=6).pack(side="left")
        ttk.Label(row3, text="×").pack(side="left")
        ttk.Entry(row3, textvariable=self.var_h, width=6).pack(side="left", padx=2)
        ttk.Button(row3, text="64² スプライト", command=lambda: self._preset(64, 64)).pack(side="left", padx=6)
        ttk.Button(row3, text="256² スチル", command=lambda: self._preset(256, 256)).pack(side="left", padx=2)
        ttk.Button(row3, text="32² UI", command=lambda: self._preset(32, 32)).pack(side="left", padx=2)

        row4 = ttk.Frame(self)
        row4.pack(fill="x", **pad)
        ttk.Label(row4, text="フィット:").pack(side="left")
        self.var_fit = tk.StringVar(value="letterbox")
        ttk.Combobox(
            row4,
            textvariable=self.var_fit,
            values=("letterbox", "stretch"),
            state="readonly",
            width=12,
        ).pack(side="left", padx=4)
        ttk.Label(row4, text="透過しきい値(スプライト):").pack(side="left", padx=(12, 0))
        self.var_alpha = tk.StringVar(value="128")
        ttk.Entry(row4, textvariable=self.var_alpha, width=5).pack(side="left", padx=2)
        self.var_event_bg = tk.BooleanVar(value=False)
        ttk.Checkbutton(
            row4,
            text="イベント奥レイヤー想定(240色警告)",
            variable=self.var_event_bg,
        ).pack(side="left", padx=12)

        row5 = ttk.Frame(self)
        row5.pack(fill="x", **pad)
        ttk.Button(row5, text="プレビュー＆検証", command=self._preview).pack(side="left", padx=2)
        ttk.Button(row5, text="BMPに書き出し…", command=self._save_bmp).pack(side="left", padx=2)
        ttk.Label(
            row5,
            text="→ Asset/graphics/ 以下を選ぶと prebuild で自動登録されます",
            foreground="#555",
        ).pack(side="left", padx=8)

        mid = ttk.PanedWindow(self, orient=tk.HORIZONTAL)
        mid.pack(fill="both", expand=True, padx=6, pady=4)

        prev_fr = ttk.LabelFrame(mid, text="プレビュー", padding=4)
        mid.add(prev_fr, weight=1)
        self.lbl_prev = ttk.Label(prev_fr, text="（未）")
        self.lbl_prev.pack(fill="both", expand=True)

        log_fr = ttk.LabelFrame(mid, text="検証メッセージ", padding=4)
        mid.add(log_fr, weight=1)
        self.txt_log = scrolledtext.ScrolledText(log_fr, height=14, width=40, wrap="word")
        self.txt_log.pack(fill="both", expand=True)

    def _preset(self, w: int, h: int):
        self.var_w.set(str(w))
        self.var_h.set(str(h))

    def _pick_src(self):
        p = filedialog.askopenfilename(
            parent=self.master_win,
            title="画像を選択",
            initialdir=os.path.join(GRAPHICS_DIR, "png"),
            filetypes=[("画像", "*.png *.webp *.jpg *.jpeg"), ("すべて", "*.*")],
        )
        if not p:
            return
        self.src_path = p
        self.var_src.set(p)
        try:
            self.src_rgba = Image.open(p).convert("RGBA")
        except Exception as e:
            self.src_rgba = None
            messagebox.showerror("読込エラー", str(e), parent=self.master_win)

    def _parse_int(self, s: str, default: int) -> int:
        try:
            return int(str(s).strip())
        except ValueError:
            return default

    def _preview(self):
        self.txt_log.delete("1.0", tk.END)
        if not self.src_rgba:
            self.txt_log.insert(tk.END, "先に画像を選択してください。\n")
            return
        tw = self._parse_int(self.var_w.get(), 64)
        th = self._parse_int(self.var_h.get(), 64)
        alpha = max(0, min(255, self._parse_int(self.var_alpha.get(), 128)))
        fit = self.var_fit.get()
        if fit not in ("letterbox", "stretch"):
            fit = "letterbox"

        try:
            if self.var_mode.get() == "still":
                im, warns = gba_asset_bake.bake_still(
                    self.src_rgba,
                    tw,
                    th,
                    max_colors=256,
                    fit=fit,
                    warn_event_layer=self.var_event_bg.get(),
                )
            else:
                im, warns = gba_asset_bake.bake_sprite(
                    self.src_rgba, tw, th, alpha_thresh=alpha, fit=fit
                )
        except Exception as e:
            self.txt_log.insert(tk.END, f"変換エラー: {e}\n")
            return

        for w in warns:
            self.txt_log.insert(tk.END, f"※ {w}\n")
        if not warns:
            self.txt_log.insert(tk.END, "（警告なし）\n")

        prv = im.convert("RGBA")
        px = prv.load()
        for y in range(prv.height):
            for x in range(prv.width):
                if px[x, y][:3] == (255, 0, 255):
                    px[x, y] = (40, 40, 48, 255)
        zm = max(1, min(4, 480 // max(tw, th)))
        prv = prv.resize((tw * zm, th * zm), Image.NEAREST)
        self._tk_prev = ImageTk.PhotoImage(prv)
        self.lbl_prev.configure(image=self._tk_prev, text="")

    def _save_bmp(self):
        if not self.src_rgba:
            messagebox.showinfo("案内", "先に画像を選択してください。", parent=self.master_win)
            return
        tw = self._parse_int(self.var_w.get(), 64)
        th = self._parse_int(self.var_h.get(), 64)
        alpha = max(0, min(255, self._parse_int(self.var_alpha.get(), 128)))
        fit = self.var_fit.get() if self.var_fit.get() in ("letterbox", "stretch") else "letterbox"

        try:
            if self.var_mode.get() == "still":
                im, warns = gba_asset_bake.bake_still(
                    self.src_rgba,
                    tw,
                    th,
                    max_colors=256,
                    fit=fit,
                    warn_event_layer=self.var_event_bg.get(),
                )
            else:
                im, warns = gba_asset_bake.bake_sprite(
                    self.src_rgba, tw, th, alpha_thresh=alpha, fit=fit
                )
        except Exception as e:
            messagebox.showerror("変換エラー", str(e), parent=self.master_win)
            return

        initial = GRAPHICS_DIR
        p = filedialog.asksaveasfilename(
            parent=self.master_win,
            title="BMPの保存先",
            initialdir=initial,
            defaultextension=".bmp",
            filetypes=[("BMP", "*.bmp")],
        )
        if not p:
            return
        try:
            gba_asset_bake.save_bmp(im, p)
        except Exception as e:
            messagebox.showerror("保存エラー", str(e), parent=self.master_win)
            return

        msg = f"保存しました:\n{p}\n\nmake を実行すると反映されます。"
        if warns:
            msg += "\n\n警告:\n" + "\n".join(warns)
        messagebox.showinfo("完了", msg, parent=self.master_win)
