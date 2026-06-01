#!/usr/bin/env python3
from __future__ import annotations

import copy
import os
import tkinter as tk
import tkinter.font as tkfont
from tkinter import messagebox, ttk

from PIL import Image, ImageTk

import puzzle_levels_io as pio

SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))
PROJECT_ROOT = os.path.dirname(os.path.dirname(SCRIPT_DIR))

CELL = 28
GRID_W = pio.MAP_W * CELL
GRID_H = pio.MAP_H * CELL
PALETTE_BTN_WIDTH = 9
PALETTE_CANVAS_HEIGHT = 72

# BMP が無い／未対応タイル用の色（プレビューのみ）
BG_COLORS = {
    0: "#c8b898",
    1: "#404040",
    2: "#60a060",
    3: "#40c040",
    4: "#a0d8ff",
    5: "#a08050",
    6: "#806030",
    7: "#202020",
    8: "#e0c040",
    9: "#ff8080",
    10: "#8080ff",
    11: "#ffff80",
    12: "#80ff80",
    13: "#ff80ff",
    14: "#ff4040",
    15: "#c03030",
    16: "#4080ff",
    17: "#3060c0",
}

FG_COLORS = {0: None, 1: "#00ffff", 2: "#8b4513", 3: "#ff00ff"}

# renderer.cpp で BMP タイルに割り当てている床をプレビューに使う
RENDER_TILE_BY_BG: dict[int, int] = {
    0: 0,   # FLOOR
    1: 1,   # WALL
    2: 21,  # GOAL (閉門状態を表示)
    3: 4,   # SWITCH
    4: 9,   # ICE
    5: 10,  # CRACKED_1
    6: 11,  # CRACKED_2
    7: 7,   # HOLE
    8: 8,   # EXIT
    9: 12,  # ARROW_UP
    10: 13, # ARROW_DOWN
    11: 14, # ARROW_LEFT
    12: 15, # ARROW_RIGHT
    13: 16, # TOGGLE_SWITCH
    14: 17, # WARP_RED_A
    15: 18, # WARP_RED_B
    16: 19, # WARP_BLUE_A
    17: 20, # WARP_BLUE_B
}

# すべて画像アセット化されたため空にする
BG_COLOR_ONLY_HINT = set()


class PuzzleEditorApp:
    def __init__(self, root: tk.Tk):
        self.root = root
        self.root.title("GBAソコバン パズルエディタ")
        self.levels = pio.load_levels(PROJECT_ROOT)
        self.original = copy.deepcopy(self.levels)
        self.current_idx: int | None = 0 if self.levels else None
        self.layer = tk.StringVar(value="bg")
        self.selected_bg = tk.IntVar(value=1)
        self.selected_fg = tk.IntVar(value=0)
        self.tile_images: dict[int, ImageTk.PhotoImage] = {}
        self._bmp_loaded = False
        self._load_tile_images()
        self._setup_ui()
        self._refresh_level_list()
        self._draw_grid()

    def _load_tile_images(self) -> None:
        bmp_path = pio.puzzle_map_bmp_path(PROJECT_ROOT)
        self._bmp_loaded = os.path.exists(bmp_path)
        if not self._bmp_loaded:
            return
        img = Image.open(bmp_path).convert("RGB")
        for tid in range(23):
            x0 = tid * 8
            y0 = 0
            if x0 + 8 > img.width:
                continue
            tile = img.crop((x0, y0, x0 + 8, y0 + 8)).resize((CELL, CELL), Image.NEAREST)
            self.tile_images[tid] = ImageTk.PhotoImage(tile)

    def _create_scroll_palette_host(self, parent: ttk.Frame) -> tuple[ttk.Frame, tk.Frame]:
        """横スクロール可能なパレット。(外枠, ボタンを並べる内側フレーム)"""
        outer = ttk.Frame(parent)

        canvas = tk.Canvas(outer, height=PALETTE_CANVAS_HEIGHT, highlightthickness=0)
        hscroll = ttk.Scrollbar(outer, orient=tk.HORIZONTAL, command=canvas.xview)
        canvas.configure(xscrollcommand=hscroll.set)
        canvas.pack(side=tk.TOP, fill=tk.X, expand=True)
        hscroll.pack(side=tk.BOTTOM, fill=tk.X)

        inner = tk.Frame(canvas)
        window_id = canvas.create_window((0, 0), window=inner, anchor=tk.NW)

        def _on_inner_configure(_event=None) -> None:
            canvas.configure(scrollregion=canvas.bbox("all"))

        def _on_canvas_configure(event) -> None:
            canvas.itemconfig(window_id, height=event.height)

        inner.bind("<Configure>", _on_inner_configure)
        canvas.bind("<Configure>", _on_canvas_configure)

        def _scroll_x(delta: int) -> None:
            canvas.xview_scroll(delta, "units")

        def _on_shift_wheel(event) -> None:
            _scroll_x(-1 if event.delta > 0 else 1)

        def _on_wheel(event) -> None:
            if event.state & 0x0001:
                _on_shift_wheel(event)
            else:
                _scroll_x(-1 if event.delta > 0 else 1)

        canvas.bind("<Enter>", lambda _e: canvas.focus_set())
        canvas.bind("<Shift-MouseWheel>", _on_shift_wheel)
        canvas.bind("<MouseWheel>", _on_wheel)

        return outer, inner

    def _setup_ui(self) -> None:
        main = ttk.PanedWindow(self.root, orient=tk.HORIZONTAL)
        main.pack(fill=tk.BOTH, expand=True, padx=8, pady=8)

        left = ttk.Frame(main)
        main.add(left, weight=1)
        ttk.Label(left, text="ステージ一覧").pack(anchor="w")
        self.level_tree = ttk.Treeview(left, columns=("title",), show="headings", height=20)
        self.level_tree.heading("title", text="名前")
        self.level_tree.pack(fill=tk.BOTH, expand=True)
        self.level_tree.bind("<<TreeviewSelect>>", self._on_level_select)

        lb = ttk.Frame(left)
        lb.pack(fill=tk.X, pady=4)
        ttk.Button(lb, text="追加", command=self._add_level).pack(side=tk.LEFT)
        ttk.Button(lb, text="複製", command=self._dup_level).pack(side=tk.LEFT)
        ttk.Button(lb, text="削除", command=self._del_level).pack(side=tk.LEFT)

        center = ttk.Frame(main)
        main.add(center, weight=3)
        top = ttk.Frame(center)
        top.pack(fill=tk.X)
        ttk.Label(top, text="編集レイヤー:").pack(side=tk.LEFT)
        ttk.Radiobutton(top, text="床（背景）", variable=self.layer, value="bg", command=self._on_layer).pack(side=tk.LEFT)
        ttk.Radiobutton(top, text="前景（キャラ・樽）", variable=self.layer, value="fg", command=self._on_layer).pack(side=tk.LEFT)
        self.lbl_info = ttk.Label(top, text="")
        self.lbl_info.pack(side=tk.RIGHT)

        hint = (
            "※ 壁・床・スイッチ・矢印などは「床（背景）」｜"
            "プレイヤー・樽・シェイディは「前景」だけ配置できます"
        )
        ttk.Label(center, text=hint, wraplength=GRID_W + 80, foreground="#333").pack(anchor="w", pady=(0, 4))

        tool_row = ttk.Frame(center)
        tool_row.pack(fill=tk.X, pady=2)
        ttk.Label(tool_row, text="配置するもの:").pack(side=tk.LEFT)
        self.tool_combo = ttk.Combobox(tool_row, state="readonly", width=36)
        self.tool_combo.pack(side=tk.LEFT, padx=4)
        self.tool_combo.bind("<<ComboboxSelected>>", self._on_tool_combo)
        self.var_status = tk.StringVar(value="")
        ttk.Label(center, textvariable=self.var_status, foreground="#006").pack(anchor="w")

        self.canvas = tk.Canvas(center, width=GRID_W, height=GRID_H, bg="#222", highlightthickness=1, highlightbackground="#666")
        self.canvas.pack(pady=8)
        self.canvas.bind("<Button-1>", self._on_paint)
        self.canvas.bind("<B1-Motion>", self._on_paint)
        self.canvas.bind("<Button-3>", self._on_erase)

        pal_frame = ttk.LabelFrame(center, text="パレット（横スクロール・下のバーまたは Shift+ホイール）")
        pal_frame.pack(fill=tk.X, pady=4)
        self.bg_palette_outer, self.bg_palette = self._create_scroll_palette_host(pal_frame)
        self.fg_palette_outer, self.fg_palette = self._create_scroll_palette_host(pal_frame)
        self.bg_palette_outer.pack(fill=tk.X, expand=True)
        self.fg_palette_outer.pack_forget()

        if self._bmp_loaded:
            tex_note = (
                "プレビュー: still_puzzle_map.bmp のすべての意味のある新規タイル (0-22) が正常に読み込まれています。"
            )
        else:
            tex_note = "プレビュー: BMP未検出 → " + pio.PUZZLE_MAP_BMP_REL
        ttk.Label(center, text=tex_note, font=("", 8), wraplength=GRID_W + 120).pack(anchor="w")

        self._build_palette()
        self._update_tool_combo()
        self._refresh_tool_status()

        right = ttk.Frame(main)
        main.add(right, weight=1)
        ttk.Label(right, text="ステージ設定").pack(anchor="w")
        form = ttk.Frame(right)
        form.pack(fill=tk.X, pady=4)
        ttk.Label(form, text="タイトル:").grid(row=0, column=0, sticky="e")
        self.var_title = tk.StringVar()
        ent = ttk.Entry(form, textvariable=self.var_title, width=28)
        ent.grid(row=0, column=1, sticky="we")
        ent.bind("<FocusOut>", self._apply_title)
        ent.bind("<Return>", self._apply_title)

        ttk.Label(form, text="プレイヤー X/Y:").grid(row=1, column=0, sticky="e")
        self.var_px = tk.StringVar()
        self.var_py = tk.StringVar()
        ttk.Entry(form, textvariable=self.var_px, width=6).grid(row=1, column=1, sticky="w")
        ttk.Entry(form, textvariable=self.var_py, width=6).grid(row=1, column=1, sticky="e", padx=(70, 0))

        ttk.Label(form, text="シェイディ X/Y:").grid(row=2, column=0, sticky="e")
        ttk.Label(form, text="（いないとき -1）", font=("", 8)).grid(row=2, column=1, sticky="w", pady=(0, 0))
        self.var_sx = tk.StringVar()
        self.var_sy = tk.StringVar()
        ttk.Entry(form, textvariable=self.var_sx, width=6).grid(row=3, column=1, sticky="w")
        ttk.Entry(form, textvariable=self.var_sy, width=6).grid(row=3, column=1, sticky="e", padx=(70, 0))
        ttk.Button(form, text="座標をマップに反映", command=self._apply_actors).grid(row=4, column=0, columnspan=2, pady=6)

        ttk.Label(right, text="チェック結果").pack(anchor="w", pady=(8, 0))
        self.txt_issues = tk.Text(right, height=8, width=32, state="disabled")
        self.txt_issues.pack(fill=tk.BOTH, expand=True)

        bottom = ttk.Frame(self.root)
        bottom.pack(fill=tk.X, padx=8, pady=8)
        ttk.Button(bottom, text="読み直し", command=self._reload).pack(side=tk.LEFT)
        ttk.Button(bottom, text="保存（JSON + levels.h）", command=self._save_all).pack(side=tk.RIGHT)

    def _palette_btn(self, parent: tk.Frame, text: str, variable: tk.IntVar, value: int, bg: str) -> None:
        try:
            fnt = tkfont.Font(size=9)
        except tk.TclError:
            fnt = None
        kw: dict = {
            "text": text,
            "variable": variable,
            "value": value,
            "indicatoron": 0,
            "width": PALETTE_BTN_WIDTH,
            "bg": bg,
            "command": self._refresh_tool_status,
            "wraplength": 72,
            "justify": tk.CENTER,
        }
        if fnt:
            kw["font"] = fnt
        tk.Radiobutton(parent, **kw).pack(side=tk.LEFT, padx=2, pady=2)

    def _build_palette(self) -> None:
        for w in self.bg_palette.winfo_children():
            w.destroy()
        for w in self.fg_palette.winfo_children():
            w.destroy()
        for val, _name in pio.BG_TILES:
            ja = pio.BG_TILES_JA.get(val, str(val))
            label = f"{val}\n{ja}"
            self._palette_btn(self.bg_palette, label, self.selected_bg, val, BG_COLORS.get(val, "#888"))
        for val, _name in pio.FG_OBJS:
            ja = pio.FG_OBJS_JA.get(val, str(val))
            fg_bg = FG_COLORS.get(val) or "#dddddd"
            self._palette_btn(self.fg_palette, ja, self.selected_fg, val, fg_bg)

    def _on_layer(self) -> None:
        if self.layer.get() == "bg":
            self.fg_palette_outer.pack_forget()
            self.bg_palette_outer.pack(fill=tk.X, expand=True)
        else:
            self.bg_palette_outer.pack_forget()
            self.fg_palette_outer.pack(fill=tk.X, expand=True)
        self._update_tool_combo()
        self._refresh_tool_status()

    def _bg_combo_labels(self) -> list[str]:
        return [f"[{v}] {pio.BG_TILES_JA.get(v, n)}" for v, n in pio.BG_TILES]

    def _fg_combo_labels(self) -> list[str]:
        return [f"[{v}] {pio.FG_OBJS_JA.get(v, n)}" for v, n in pio.FG_OBJS]

    def _update_tool_combo(self) -> None:
        if self.layer.get() == "bg":
            labels = self._bg_combo_labels()
            self.tool_combo["values"] = labels
            idx = self.selected_bg.get()
            if 0 <= idx < len(labels):
                self.tool_combo.current(idx)
        else:
            labels = self._fg_combo_labels()
            self.tool_combo["values"] = labels
            idx = self.selected_fg.get()
            if 0 <= idx < len(labels):
                self.tool_combo.current(idx)

    def _on_tool_combo(self, _event=None) -> None:
        idx = self.tool_combo.current()
        if idx < 0:
            return
        if self.layer.get() == "bg":
            if idx < len(pio.BG_TILES):
                self.selected_bg.set(pio.BG_TILES[idx][0])
        else:
            if idx < len(pio.FG_OBJS):
                self.selected_fg.set(pio.FG_OBJS[idx][0])
        self._refresh_tool_status()

    def _refresh_tool_status(self) -> None:
        self._update_tool_combo()
        if self.layer.get() == "bg":
            v = self.selected_bg.get()
            ja = pio.BG_TILES_JA.get(v, "?")
            hint = ""
            if v in BG_COLOR_ONLY_HINT:
                hint = "（プレビューは色のみ。配置はできています）"
            self.var_status.set(
                f"床レイヤー: [{v}] {ja} を配置 → マップをクリック{hint}"
            )
        else:
            v = self.selected_fg.get()
            ja = pio.FG_OBJS_JA.get(v, "?")
            extra = ""
            if v == 1:
                extra = "（ステージにプレイヤーは1人）"
            elif v == 3:
                extra = "（いないステージは「なし」で消すか座標を-1）"
            self.var_status.set(f"前景レイヤー: [{v}] {ja} を配置 → マップをクリック{extra}")

    def _current(self) -> pio.LevelEntry | None:
        if self.current_idx is None or self.current_idx < 0 or self.current_idx >= len(self.levels):
            return None
        return self.levels[self.current_idx]

    def _refresh_level_list(self) -> None:
        for i in self.level_tree.get_children():
            self.level_tree.delete(i)
        for i, lv in enumerate(self.levels):
            self.level_tree.insert("", tk.END, iid=str(i), values=(lv.title,))
        if self.levels and self.current_idx is not None:
            self.level_tree.selection_set(str(self.current_idx))
            self._load_level_fields()

    def _on_level_select(self, _event=None) -> None:
        sel = self.level_tree.selection()
        if not sel:
            return
        self.current_idx = int(sel[0])
        self._load_level_fields()
        self._draw_grid()

    def _load_level_fields(self) -> None:
        lv = self._current()
        if not lv:
            return
        self.var_title.set(lv.title)
        self.var_px.set(str(lv.player_x))
        self.var_py.set(str(lv.player_y))
        self.var_sx.set(str(lv.shady_x))
        self.var_sy.set(str(lv.shady_y))
        self.lbl_info.config(text=f"ステージ {self.current_idx}（{pio.MAP_W}×{pio.MAP_H}）")
        self._update_validation()

    def _apply_title(self, _event=None) -> None:
        lv = self._current()
        if lv:
            lv.title = self.var_title.get().strip() or "名称未設定"
            self._refresh_level_list()

    def _apply_actors(self) -> None:
        lv = self._current()
        if not lv:
            return
        try:
            lv.player_x = int(self.var_px.get())
            lv.player_y = int(self.var_py.get())
            lv.shady_x = int(self.var_sx.get())
            lv.shady_y = int(self.var_sy.get())
        except ValueError:
            messagebox.showerror("入力エラー", "座標は整数で入力してください。")
            return
        pio.sync_actor_positions(lv)
        self._draw_grid()
        self._update_validation()

    def _add_level(self) -> None:
        self.levels.append(pio.LevelEntry(title=f"ステージ {len(self.levels)}"))
        self.current_idx = len(self.levels) - 1
        pio.sync_actor_positions(self.levels[-1])
        self._refresh_level_list()
        self._draw_grid()

    def _dup_level(self) -> None:
        lv = self._current()
        if not lv:
            return
        dup = copy.deepcopy(lv)
        dup.title = lv.title + "（コピー）"
        self.levels.append(dup)
        self.current_idx = len(self.levels) - 1
        self._refresh_level_list()
        self._draw_grid()

    def _del_level(self) -> None:
        if self.current_idx is None or len(self.levels) <= 1:
            messagebox.showinfo("削除できません", "ステージは最低1つ必要です。")
            return
        if not messagebox.askyesno("削除の確認", "このステージを削除しますか？"):
            return
        del self.levels[self.current_idx]
        self.current_idx = min(self.current_idx, len(self.levels) - 1)
        self._refresh_level_list()
        self._draw_grid()

    def _cell_at(self, event) -> tuple[int, int] | None:
        x = event.x // CELL
        y = event.y // CELL
        if 0 <= x < pio.MAP_W and 0 <= y < pio.MAP_H:
            return x, y
        return None

    def _on_paint(self, event) -> None:
        cell = self._cell_at(event)
        if cell is None:
            return
        x, y = cell
        lv = self._current()
        if not lv:
            return
        if self.layer.get() == "bg":
            bid = self.selected_bg.get()
            lv.bg[y][x] = bid
            ja = pio.BG_TILES_JA.get(bid, "?")
            self.var_status.set(f"({x},{y}) に [{bid}] {ja} を配置しました")
        else:
            val = self.selected_fg.get()
            if val == 1:
                lv.player_x, lv.player_y = x, y
                pio.sync_actor_positions(lv)
                self.var_px.set(str(x))
                self.var_py.set(str(y))
            elif val == 3:
                lv.shady_x, lv.shady_y = x, y
                pio.sync_actor_positions(lv)
                self.var_sx.set(str(x))
                self.var_sy.set(str(y))
            elif val == 0:
                if lv.fg[y][x] == 1 and (lv.player_x != x or lv.player_y != y):
                    pass
                elif lv.fg[y][x] == 1:
                    messagebox.showwarning(
                        "プレイヤー",
                        "プレイヤーを消す場合は、別のマスにプレイヤーを置き直してください。",
                    )
                if lv.fg[y][x] == 3:
                    lv.shady_x = lv.shady_y = -1
                    self.var_sx.set("-1")
                    self.var_sy.set("-1")
                    lv.fg[y][x] = 0
                elif lv.fg[y][x] != 1:
                    lv.fg[y][x] = 0
            else:
                if lv.fg[y][x] == 1:
                    lv.fg[y][x] = 0
                elif lv.fg[y][x] == 3:
                    lv.fg[y][x] = 0
                lv.fg[y][x] = val
        self._draw_grid()
        self._update_validation()

    def _on_erase(self, event) -> None:
        if self.layer.get() == "bg":
            self.selected_bg.set(0)
        else:
            self.selected_fg.set(0)
        self._on_paint(event)

    def _draw_bg_cell(self, x0: int, y0: int, bg: int) -> None:
        tid = RENDER_TILE_BY_BG.get(bg)
        if tid is not None and tid in self.tile_images:
            self.canvas.create_image(x0, y0, image=self.tile_images[tid], anchor=tk.NW)
        else:
            color = BG_COLORS.get(bg, "#888888")
            self.canvas.create_rectangle(x0, y0, x0 + CELL, y0 + CELL, fill=color, outline="#333333")
            if bg in BG_COLOR_ONLY_HINT:
                self.canvas.create_text(
                    x0 + CELL // 2,
                    y0 + CELL // 2,
                    text=str(bg),
                    fill="#000",
                    font=("", 7),
                )

    def _draw_fg_overlay(self, x0: int, y0: int, bg: int, fg: int) -> None:
        if fg == 0:
            return
        if fg == 1 and 2 in self.tile_images:
            self.canvas.create_image(x0, y0, image=self.tile_images[2], anchor=tk.NW)
            return
        if fg == 2:
            tid = 5 if bg == 3 else 3
            if tid in self.tile_images:
                self.canvas.create_image(x0, y0, image=self.tile_images[tid], anchor=tk.NW)
                return
        if fg == 3 and 6 in self.tile_images:
            self.canvas.create_image(x0, y0, image=self.tile_images[6], anchor=tk.NW)
            return
        color = FG_COLORS.get(fg)
        if color:
            self.canvas.create_oval(x0 + 5, y0 + 5, x0 + CELL - 5, y0 + CELL - 5, fill=color, outline="#000")

    def _draw_cell(self, x: int, y: int, lv: pio.LevelEntry) -> None:
        x0, y0 = x * CELL, y * CELL
        bg = lv.bg[y][x]
        fg = lv.fg[y][x]
        self._draw_bg_cell(x0, y0, bg)
        self._draw_fg_overlay(x0, y0, bg, fg)

    def _draw_grid(self) -> None:
        self.canvas.delete("all")
        lv = self._current()
        if not lv:
            return
        for y in range(pio.MAP_H):
            for x in range(pio.MAP_W):
                self._draw_cell(x, y, lv)
        for x in range(pio.MAP_W + 1):
            self.canvas.create_line(x * CELL, 0, x * CELL, GRID_H, fill="#444444")
        for y in range(pio.MAP_H + 1):
            self.canvas.create_line(0, y * CELL, GRID_W, y * CELL, fill="#444444")

    def _update_validation(self) -> None:
        lv = self._current()
        self.txt_issues.configure(state="normal")
        self.txt_issues.delete("1.0", tk.END)
        if lv:
            issues = pio.validate_level(lv)
            if issues:
                self.txt_issues.insert(tk.END, "\n".join(issues))
            else:
                self.txt_issues.insert(tk.END, "問題なし")
        self.txt_issues.configure(state="disabled")

    def _save_all(self) -> None:
        for i, lv in enumerate(self.levels):
            pio.sync_actor_positions(lv)
            issues = pio.validate_level(lv)
            if issues:
                messagebox.showerror(
                    "保存できません",
                    f"ステージ {i}（{lv.title}）:\n" + "\n".join(issues),
                )
                return
        try:
            pio.save_levels(PROJECT_ROOT, self.levels)
        except OSError as e:
            messagebox.showerror("保存失敗", str(e))
            return
        self.original = copy.deepcopy(self.levels)
        messagebox.showinfo("保存完了", "levels.json と levels.h を更新しました。\nゲームをビルドし直してください。")

    def _reload(self) -> None:
        if self.levels != self.original:
            if not messagebox.askyesno("読み直し", "未保存の変更を破棄してファイルから読み直しますか？"):
                return
        self.levels = pio.load_levels(PROJECT_ROOT)
        self.original = copy.deepcopy(self.levels)
        self.current_idx = 0 if self.levels else None
        self._refresh_level_list()
        self._draw_grid()


def main() -> None:
    root = tk.Tk()
    root.minsize(920, 560)
    PuzzleEditorApp(root)
    root.mainloop()


if __name__ == "__main__":
    main()
