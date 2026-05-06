import tkinter as tk
from tkinter import ttk, filedialog, messagebox
import json
import os
import time
import copy
from PIL import Image, ImageTk

SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))
PROJECT_ROOT = os.path.dirname(os.path.dirname(SCRIPT_DIR)) # src/tools -> root
GRAPHICS_DIR = os.path.join(PROJECT_ROOT, "Asset", "graphics")
SCREENS_DIR = os.path.join(PROJECT_ROOT, "Asset", "layouts")
MANIFEST_PATH = os.path.join(PROJECT_ROOT, "Asset", "tools", "ui_editor", "image_manifest.json")

class GBAUIEditor(tk.Tk):
    def __init__(self):
        super().__init__()
        self.title("GBA UI Editor")
        
        self.load_editor_config()
        self.geometry(self.editor_config.get("geometry", "1100x650"))
        
        self.protocol("WM_DELETE_WINDOW", self.on_close)
        
        self.scale = 2
        self.grid_size = 8
        self.snap_to_grid = tk.BooleanVar(value=True)
        
        self.scene = {
            "screen": "new_screen",
            "root": {"type": "group", "id": "root", "x": 0, "y": 0, "children": []}
        }
        self.selected_node = None
        self.current_filepath = None
        
        # Global key bindings
        self.bind('<Control-s>', lambda e: self.save_json())
        self.bind('<Control-S>', lambda e: self.save_json())
        self.bind('<Delete>',    self._on_delete_key)
        self.bind('<BackSpace>', self._on_delete_key)
        self.bind('<Control-c>', lambda e: self.copy_node())
        self.bind('<Control-C>', lambda e: self.copy_node())
        self.bind('<Control-v>', lambda e: self.paste_node())
        self.bind('<Control-V>', lambda e: self.paste_node())
        self.bind('<Control-Up>',   self.move_node_up)
        self.bind('<Control-Down>', self.move_node_down)
        self.bind('<Control-Left>',  self.outdent_node)
        self.bind('<Control-Right>', self.indent_node)
        self.bind('<Control-z>', self.undo)
        self.bind('<Control-Z>', self.undo)
        self.bind('<Control-y>', self.redo)
        self.bind('<Control-Y>', self.redo)
        self.bind('<Up>',   self._on_global_up)
        self.bind('<Down>', self._on_global_down)
        
        self.undo_stack = []
        self.redo_stack = []
        
        self.clipboard_node = None
        self.drag_data = None        # canvas sprite drag
        self.image_cache = {}

        # Listbox hierarchy state
        self._hier_nodes = []        # [(depth, node_dict), ...]
        self._drag_src_idx = None
        self._drag_press_y = None

        # UI Animation playback state
        self.playing_ui_anim_node = None
        self.ui_anim_frame = 0
        self.ui_anim_preset_data = None
        
        self.load_manifest()
        self.bg_list = self.get_bg_list()
        
        self.create_ui()
        self.refresh_tree()
        self.render_canvas()
        self.saved_scene = copy.deepcopy(self.scene)
        
        # Apply saved sash positions after UI is drawn
        self.after(100, self.apply_sashes)

    def on_close(self):
        if self.confirm_discard_changes():
            self.save_editor_config()
            self.destroy()

    def confirm_discard_changes(self):
        if self.scene == self.saved_scene:
            return True # No unsaved changes
            
        ans = messagebox.askyesnocancel(
            "保存されていない変更",
            "変更内容が保存されていません。保存しますか？\n\n"
            "【はい】 保存して進む\n"
            "【いいえ】 保存せずに進む（変更は破棄されます）\n"
            "【キャンセル】 編集に戻る"
        )
        if ans is True:
            self.save_json()
            return True
        elif ans is False:
            return True # Proceed without saving
        else:
            return False # Cancel action and return to editor

    def load_editor_config(self):
        self.editor_config_path = os.path.join(SCRIPT_DIR, "editor_config.json")
        try:
            with open(self.editor_config_path, "r") as f:
                self.editor_config = json.load(f)
        except:
            self.editor_config = {}

    def save_editor_config(self):
        try:
            self.editor_config["geometry"] = self.geometry()
            self.editor_config["sash0"] = self.paned.sash_coord(0)[0]
            self.editor_config["sash1"] = self.paned.sash_coord(1)[0]
            with open(self.editor_config_path, "w") as f:
                json.dump(self.editor_config, f)
        except Exception as e:
            print("Failed to save config:", e)

    def apply_sashes(self):
        if "sash0" in self.editor_config and "sash1" in self.editor_config:
            try:
                self.paned.sash_place(0, self.editor_config["sash0"], 0)
                self.paned.sash_place(1, self.editor_config["sash1"], 0)
            except:
                pass

    # ------------------------------------------------------------------
    def load_manifest(self):
        try:
            with open(MANIFEST_PATH, 'r', encoding='utf-8') as f:
                self.manifest = json.load(f)
        except Exception as e:
            self.manifest = {}
            print(f"Error loading manifest: {e}")

    def get_bg_list(self):
        bgs = []
        stills_dir = os.path.join(GRAPHICS_DIR, "stills")
        if os.path.exists(stills_dir):
            for root, dirs, files in os.walk(stills_dir):
                for f in files:
                    if f.endswith(".bmp"):
                        bgs.append(f[:-4])
        return bgs

    # ------------------------------------------------------------------
    # UI Construction
    # ------------------------------------------------------------------
    def create_ui(self):
        toolbar = tk.Frame(self, bd=1, relief=tk.RAISED)
        toolbar.pack(side=tk.TOP, fill=tk.X)
        
        tk.Label(toolbar, text="Screen Name:").pack(side=tk.LEFT, padx=5)
        self.screen_name_var = tk.StringVar(value=self.scene["screen"])
        tk.Entry(toolbar, textvariable=self.screen_name_var, width=15).pack(side=tk.LEFT)
        
        tk.Button(toolbar, text="New JSON",          command=self.new_json).pack(side=tk.LEFT, padx=5)
        tk.Button(toolbar, text="Load JSON",         command=self.load_json).pack(side=tk.LEFT, padx=5)
        tk.Button(toolbar, text="Save JSON (Ctrl+S)",command=self.save_json).pack(side=tk.LEFT, padx=5)
        tk.Button(toolbar, text="Show in Explorer",  command=self.show_in_explorer).pack(side=tk.LEFT, padx=5)
        
        self.status_var = tk.StringVar(value="Status: Ready")
        tk.Label(toolbar, textvariable=self.status_var, fg="#00b894").pack(side=tk.RIGHT, padx=10)
        
        tk.Checkbutton(toolbar, text="Snap to Grid", variable=self.snap_to_grid).pack(side=tk.LEFT, padx=10)
        tk.Label(toolbar, text="Grid Size:").pack(side=tk.LEFT)
        self.grid_size_var = tk.IntVar(value=self.grid_size)
        tk.Entry(toolbar, textvariable=self.grid_size_var, width=5).pack(side=tk.LEFT)
        self.grid_size_var.trace("w", lambda *args: self.update_grid_size())

        self.paned = tk.PanedWindow(self, orient=tk.HORIZONTAL)
        self.paned.pack(fill=tk.BOTH, expand=True)

        # ---- Left: Hierarchy (Listbox) ----
        left_frame = tk.Frame(self.paned, width=250)
        self.paned.add(left_frame, minsize=200, stretch="never")
        
        tk.Label(left_frame, text="Hierarchy  [drag to reorder/reparent]",
                 bg="#444", fg="white").pack(fill=tk.X)
        
        lb_frame = tk.Frame(left_frame)
        lb_frame.pack(fill=tk.BOTH, expand=True)
        
        self.lb = tk.Listbox(lb_frame, selectmode=tk.SINGLE,
                             font=("Courier", 10), activestyle="none")
        self.lb.pack(side=tk.LEFT, fill=tk.BOTH, expand=True)
        sb = tk.Scrollbar(lb_frame, orient=tk.VERTICAL, command=self.lb.yview)
        sb.pack(side=tk.RIGHT, fill=tk.Y)
        self.lb.configure(yscrollcommand=sb.set)
        
        # Listbox bindings – plain Listbox has no built-in drag so these work cleanly
        self.lb.bind("<<ListboxSelect>>", self.on_lb_select)
        self.lb.bind("<ButtonPress-1>",   self.on_lb_press)
        self.lb.bind("<B1-Motion>",       self.on_lb_motion)
        self.lb.bind("<ButtonRelease-1>", self.on_lb_release)
        self.lb.bind("<Button-3>",        self.show_context_menu)

        # Floating drag tooltip
        self._drag_label = tk.Label(self, bg="#f1c40f", fg="black",
                                    text="", relief=tk.SOLID, bd=1)

        # Context Menu
        self.context_menu = tk.Menu(self, tearoff=0)
        self.context_menu.add_command(label="+ Group",  command=lambda: self.add_node("group"))
        self.context_menu.add_command(label="+ Sprite", command=lambda: self.add_node("sprite"))
        self.context_menu.add_command(label="+ Text",   command=lambda: self.add_node("text"))
        self.context_menu.add_command(label="+ UIAnim", command=lambda: self.add_node("anim"))
        self.context_menu.add_separator()
        self.context_menu.add_command(label="Copy (Ctrl+C)", command=self.copy_node)
        self.context_menu.add_command(label="Paste (Ctrl+V)",command=self.paste_node)
        self.context_menu.add_separator()
        self.context_menu.add_command(label="Delete (Del)",  command=self.delete_node)

        # ---- Center: Canvas ----
        center_frame = tk.Frame(self.paned, bg="#222", width=600)
        self.paned.add(center_frame, minsize=300, stretch="always")
        
        self.canvas = tk.Canvas(center_frame,
                                width=240 * self.scale,
                                height=160 * self.scale,
                                bg="black")
        self.canvas.place(relx=0.5, rely=0.5, anchor=tk.CENTER)
        
        self.canvas.bind("<ButtonPress-1>",   self.on_canvas_press)
        self.canvas.bind("<B1-Motion>",       self.on_canvas_drag)
        self.canvas.bind("<ButtonRelease-1>", self.on_canvas_release)

        # ---- Right: Inspector ----
        self.right_frame = tk.Frame(self.paned, width=250)
        self.paned.add(self.right_frame, minsize=200, stretch="never")
        tk.Label(self.right_frame, text="Inspector", bg="#444", fg="white").pack(fill=tk.X)
        self.inspector_content = tk.Frame(self.right_frame)
        self.inspector_content.pack(fill=tk.BOTH, expand=True, padx=5, pady=5)

    # ------------------------------------------------------------------
    def update_grid_size(self):
        try:
            self.grid_size = self.grid_size_var.get()
        except:
            pass

    def show_context_menu(self, event):
        idx = self.lb.nearest(event.y)
        if idx >= 0:
            self.lb.selection_clear(0, tk.END)
            self.lb.selection_set(idx)
            self.on_lb_select(None)
        self.context_menu.tk_popup(event.x_root, event.y_root)

    # ------------------------------------------------------------------
    # Node operations
    # ------------------------------------------------------------------
    def add_node(self, ntype):
        self.push_undo()
        parent = (self.selected_node
                  if self.selected_node and self.selected_node.get("type") in ("group", "root", "anim")
                  else self.scene["root"])
        parent.setdefault("children", [])
        
        node = {"type": ntype, "id": f"new_{ntype}", "x": 0, "y": 0, "visible": True, "viewer_visible": True}
        if ntype == "group":
            node["children"] = []
        elif ntype == "sprite":
            node["image_set"] = list(self.manifest.keys())[0] if self.manifest else ""
            node["image_no"] = "0"
            node["rotation"] = 0.0
        elif ntype == "text":
            node["text"] = "Text"
            node["align"] = "center"
            node["font_size"] = 1.0
        elif ntype == "anim":
            node["preset_id"] = ""
            node["children"] = []
            
        parent["children"].append(node)
        self.refresh_tree()
        self.render_canvas()

    def _on_delete_key(self, event):
        """Delete/BackSpace: only delete node if no input widget has focus."""
        focused = self.focus_get()
        if isinstance(focused, (tk.Entry, tk.Spinbox, ttk.Combobox)):
            return  # テキスト入力中や値の調整中は何もしない
        self.delete_node()

    def _on_global_up(self, event):
        focused = self.focus_get()
        if isinstance(focused, (tk.Entry, tk.Spinbox, ttk.Combobox)):
            return  # テキスト入力中やスピンボックス調整中などはデフォルト動作を優先
            
        sel = self.lb.curselection()
        if not sel:
            idx = 0
        else:
            idx = max(0, sel[0] - 1)
            
        self.lb.selection_clear(0, tk.END)
        self.lb.selection_set(idx)
        self.lb.see(idx)
        self.on_lb_select(None)
        return "break"

    def _on_global_down(self, event):
        focused = self.focus_get()
        if isinstance(focused, (tk.Entry, tk.Spinbox, ttk.Combobox)):
            return  # テキスト入力中やスピンボックス調整中などはデフォルト動作を優先
            
        sel = self.lb.curselection()
        if not sel:
            idx = 0
        else:
            idx = min(self.lb.size() - 1, sel[0] + 1)
            
        self.lb.selection_clear(0, tk.END)
        self.lb.selection_set(idx)
        self.lb.see(idx)
        self.on_lb_select(None)
        return "break"

    # ------------------------------------------------------------------
    # Undo / Redo System
    # ------------------------------------------------------------------
    def push_undo(self):
        # Create a snapshot state
        state = {
            "scene": copy.deepcopy(self.scene),
            "selected_id": self.selected_node["id"] if self.selected_node else None
        }
        # Prevent pushing identical states sequentially
        if self.undo_stack:
            last_state = self.undo_stack[-1]
            if last_state["scene"] == state["scene"] and last_state["selected_id"] == state["selected_id"]:
                return
                
        self.undo_stack.append(state)
        if len(self.undo_stack) > 30:
            self.undo_stack.pop(0)
        self.redo_stack.clear()

    def undo(self, event=None):
        if not self.undo_stack:
            self.status_var.set("Status: Nothing to undo")
            return
            
        current_state = {
            "scene": copy.deepcopy(self.scene),
            "selected_id": self.selected_node["id"] if self.selected_node else None
        }
        self.redo_stack.append(current_state)
        if len(self.redo_stack) > 30:
            self.redo_stack.pop(0)
            
        prev_state = self.undo_stack.pop()
        self.scene = prev_state["scene"]
        self.restore_selection_by_id(prev_state["selected_id"])
        
        self.refresh_tree()
        self.render_canvas()
        self.render_inspector()
        self.status_var.set("Status: Undo action")

    def redo(self, event=None):
        if not self.redo_stack:
            self.status_var.set("Status: Nothing to redo")
            return
            
        current_state = {
            "scene": copy.deepcopy(self.scene),
            "selected_id": self.selected_node["id"] if self.selected_node else None
        }
        self.undo_stack.append(current_state)
        if len(self.undo_stack) > 30:
            self.undo_stack.pop(0)
            
        next_state = self.redo_stack.pop()
        self.scene = next_state["scene"]
        self.restore_selection_by_id(next_state["selected_id"])
        
        self.refresh_tree()
        self.render_canvas()
        self.render_inspector()
        self.status_var.set("Status: Redo action")

    def restore_selection_by_id(self, node_id):
        if not node_id:
            self.selected_node = None
            return
            
        def find_by_id(node):
            if node.get("id") == node_id:
                return node
            for c in node.get("children", []):
                res = find_by_id(c)
                if res:
                    return res
            return None
            
        self.selected_node = find_by_id(self.scene["root"])

    def bind_undo(self, widget):
        widget.bind("<FocusIn>", lambda e: self.push_undo(), add="+")
        widget.bind("<ButtonPress-1>", lambda e: self.push_undo(), add="+")

    def find_parent(self, parent, target):
        ch = parent.get("children", [])
        if target in ch:
            return parent
        for c in ch:
            res = self.find_parent(c, target)
            if res:
                return res
        return None

    def move_node_up(self, event=None):
        if not self.selected_node:
            return
        if self.selected_node["id"] == "root" or self.selected_node.get("type") == "bg":
            return
        
        parent = self.find_parent(self.scene["root"], self.selected_node)
        if not parent:
            return
        
        children = parent.get("children", [])
        idx = children.index(self.selected_node)
        if idx > 0:
            # 背景(bg)ノードよりも上に持っていくことはできないように制限（描画が背景の後ろに隠れてしまうのを防ぐため）
            if idx == 1 and children[0].get("type") == "bg":
                return
            self.push_undo()
            children[idx], children[idx-1] = children[idx-1], children[idx]
            self.refresh_tree()
            self.render_canvas()

    def move_node_down(self, event=None):
        if not self.selected_node:
            return
        if self.selected_node["id"] == "root" or self.selected_node.get("type") == "bg":
            return
        
        parent = self.find_parent(self.scene["root"], self.selected_node)
        if not parent:
            return
        
        children = parent.get("children", [])
        idx = children.index(self.selected_node)
        if idx < len(children) - 1:
            self.push_undo()
            children[idx], children[idx+1] = children[idx+1], children[idx]
            self.refresh_tree()
            self.render_canvas()

    def outdent_node(self, event=None):
        if not self.selected_node:
            return
        if self.selected_node["id"] == "root" or self.selected_node.get("type") == "bg":
            return
            
        parent = self.find_parent(self.scene["root"], self.selected_node)
        if not parent or parent["id"] == "root":
            return # Already at the root level, cannot outdent further!
            
        grandparent = self.find_parent(self.scene["root"], parent)
        if not grandparent:
            return
            
        self.push_undo()
        
        # Remove selected_node from parent
        parent.setdefault("children", []).remove(self.selected_node)
        
        # Insert into grandparent right after former parent
        g_children = grandparent.setdefault("children", [])
        p_idx = g_children.index(parent)
        g_children.insert(p_idx + 1, self.selected_node)
        
        self.refresh_tree()
        self.render_canvas()

    def indent_node(self, event=None):
        if not self.selected_node:
            return
        if self.selected_node["id"] == "root" or self.selected_node.get("type") == "bg":
            return
            
        parent = self.find_parent(self.scene["root"], self.selected_node)
        if not parent:
            return
            
        siblings = parent.setdefault("children", [])
        idx = siblings.index(self.selected_node)
        if idx <= 0:
            return # No sibling above it to indent into!
            
        target_parent = siblings[idx - 1]
        if target_parent.get("type") == "bg":
            return # Cannot indent into bg node!
            
        self.push_undo()
        
        # Remove from current parent's children
        siblings.remove(self.selected_node)
        
        # Append to target sibling's children
        target_parent.setdefault("children", []).append(self.selected_node)
        
        self.refresh_tree()
        self.render_canvas()

    def delete_node(self):
        if not self.selected_node:
            return
        if self.selected_node["id"] == "root" or self.selected_node.get("type") == "bg":
            return
            
        self.push_undo()
            
        def remove(parent):
            ch = parent.get("children", [])
            if self.selected_node in ch:
                ch.remove(self.selected_node)
                return True
            return any(remove(c) for c in ch)
            
        remove(self.scene["root"])
        self.selected_node = None
        self.refresh_tree()
        self.render_canvas()
        self.render_inspector()

    def copy_node(self):
        if not self.selected_node or self.selected_node["id"] == "root" or self.selected_node.get("type") == "bg":
            return
        self.clipboard_node = copy.deepcopy(self.selected_node)
        self.status_var.set(f"Status: Copied '{self.selected_node['id']}'")

    def paste_node(self):
        if not self.clipboard_node:
            return
            
        self.push_undo()
            
        new_node = copy.deepcopy(self.clipboard_node)
        
        # Rename IDs recursively to avoid duplicates
        def rename(n):
            if "id" in n:
                # Add _copy, but prevent _copy_copy_copy
                if n["id"].endswith("_copy"):
                    n["id"] = n["id"][:-5] + "_copy2"
                elif n["id"].endswith("_copy2"):
                    n["id"] = n["id"][:-6] + "_copy3"
                else:
                    n["id"] = f"{n['id']}_copy"
            for c in n.get("children", []):
                rename(c)
        rename(new_node)
        
        target = self.selected_node if self.selected_node else self.scene["root"]
        
        # Insert logic
        if target.get("type") in ("group", "root") and target.get("id") != "root_bg_hack":
            target.setdefault("children", []).append(new_node)
        else:
            def insert_after(parent):
                ch = parent.get("children", [])
                if target in ch:
                    ch.insert(ch.index(target) + 1, new_node)
                    return True
                return any(insert_after(c) for c in ch)
            insert_after(self.scene["root"])
            
        self.selected_node = new_node
        self.refresh_tree()
        self.render_canvas()
        self.status_var.set(f"Status: Pasted '{new_node['id']}'")

    # ------------------------------------------------------------------
    # Listbox hierarchy
    # ------------------------------------------------------------------
    def refresh_tree(self):
        # Ensure bg node is first child of root
        bg = next((c for c in self.scene["root"]["children"] if c.get("type") == "bg"), None)
        if not bg:
            bg = {"type": "bg", "id": "background", "image_id": ""}
            self.scene["root"]["children"].insert(0, bg)

        self._hier_nodes = []
        def walk(node, depth):
            self._hier_nodes.append((depth, node))
            for c in node.get("children", []):
                walk(c, depth + 1)
        walk(self.scene["root"], 0)

        self.lb.delete(0, tk.END)
        icons = {"group": "▶", "bg": "▣", "sprite": "★", "text": "T", "anim": "⧖", "root": "⊞"}
        for depth, node in self._hier_nodes:
            icon = icons.get(node.get("type"), "•")
            label = f"{'  ' * depth}{icon} {node.get('id', '')}"
            self.lb.insert(tk.END, label)

        # Re-select
        if self.selected_node is not None:
            for i, (_, n) in enumerate(self._hier_nodes):
                if n is self.selected_node:
                    self.lb.selection_set(i)
                    self.lb.see(i)
                    break

    def on_lb_select(self, event):
        sel = self.lb.curselection()
        if not sel:
            return
        _, node = self._hier_nodes[sel[0]]
        self.selected_node = node
        self.render_inspector()
        self.render_canvas()

    # ------------------------------------------------------------------
    # Listbox Drag & Drop  (works cleanly because Listbox has no default drag)
    # ------------------------------------------------------------------
    def on_lb_press(self, event):
        idx = self.lb.nearest(event.y)
        self._drag_src_idx = idx
        self._drag_press_y = event.y
        self.lb.selection_clear(0, tk.END)
        self.lb.selection_set(idx)
        self.on_lb_select(None)

    def on_lb_motion(self, event):
        if self._drag_src_idx is None:
            return
        if abs(event.y - self._drag_press_y) < 5:
            return
        tgt = self.lb.nearest(event.y)
        if 0 <= tgt < len(self._hier_nodes) and tgt != self._drag_src_idx:
            _, tgt_node = self._hier_nodes[tgt]
            self._drag_label.config(text=f"  → {tgt_node.get('id', '?')}  ")
            self._drag_label.place(
                x=event.x_root - self.winfo_rootx() + 16,
                y=event.y_root - self.winfo_rooty() - 12)
            # Highlight drop target
            self.lb.itemconfig(tgt, bg="#f1c40f", fg="black")
            for i in range(len(self._hier_nodes)):
                if i != tgt:
                    self.lb.itemconfig(i, bg="", fg="")
        else:
            self._drag_label.place_forget()

    def on_lb_release(self, event):
        self._drag_label.place_forget()
        # Reset highlight
        for i in range(self.lb.size()):
            self.lb.itemconfig(i, bg="", fg="")

        if self._drag_src_idx is None:
            return

        src_idx  = self._drag_src_idx
        press_y  = self._drag_press_y
        self._drag_src_idx = None
        self._drag_press_y = None

        # Not a drag if barely moved
        if abs(event.y - press_y) < 5:
            return

        tgt = self.lb.nearest(event.y)
        if tgt < 0 or tgt == src_idx:
            return

        _, src_node = self._hier_nodes[src_idx]
        _, tgt_node = self._hier_nodes[tgt]

        # Guards
        if src_node.get("type") == "bg" or src_node.get("id") == "root":
            return

        def is_ancestor(node, target):
            if node is target: return True
            return any(is_ancestor(c, target) for c in node.get("children", []))

        # Reversal Logic (descendant dragged onto ancestor)
        if is_ancestor(tgt_node, src_node):
            if tgt_node.get("id") == "root":
                return
            self.push_undo()
            
            # Find path from tgt_node to src_node
            path = []
            def find_path_to_node(current, target, path_list):
                path_list.append(current)
                if current is target:
                    return True
                for c in current.get("children", []):
                    if find_path_to_node(c, target, path_list):
                        return True
                path_list.pop()
                return False
                
            find_path_to_node(tgt_node, src_node, path)
            
            p0 = self.find_parent(self.scene["root"], tgt_node)
            if p0:
                p0_children = p0.get("children", [])
                idx_in_p0 = p0_children.index(tgt_node)
                
                # Collect original children of all nodes in path
                orig_children = {id(n): list(n.get("children", [])) for n in path}
                
                # Perform the inversion
                # N_k's children become [N_{k-1}]
                path[-1]["children"] = [path[-2]]
                
                # N_i's children (for i from 1 to k-1) become [N_{i-1}] + (orig_children[N_i] without N_{i+1})
                for i in range(1, len(path) - 1):
                    ni = path[i]
                    ni_next = path[i+1]
                    ni_prev = path[i-1]
                    other_children = [c for c in orig_children[id(ni)] if c is not ni_next]
                    ni["children"] = [ni_prev] + other_children
                    
                # N_0's children become (orig_children[N_k]) + (orig_children[N_0] without N_1)
                n0 = path[0]
                n1 = path[1]
                nk = path[-1]
                other_n0_children = [c for c in orig_children[id(n0)] if c is not n1]
                n0["children"] = orig_children[id(nk)] + other_n0_children
                
                # Replace N_0 with N_k in p0's children
                p0_children[idx_in_p0] = nk
                
                self.selected_node = nk
                self.refresh_tree()
                self.render_canvas()
                return

        if is_ancestor(src_node, tgt_node):
            return

        self.push_undo()

        # Remove from current parent
        def remove_node(parent):
            ch = parent.get("children", [])
            if src_node in ch:
                ch.remove(src_node)
                return True
            return any(remove_node(c) for c in ch)

        remove_node(self.scene["root"])

        # Always append as child of tgt_node (Allows nesting under Sprite/Text/Group/Root)
        tgt_node.setdefault("children", []).append(src_node)

        self.selected_node = src_node
        self.refresh_tree()
        self.render_canvas()

    # ------------------------------------------------------------------
    # Inspector
    # ------------------------------------------------------------------
    def render_inspector(self):
        for w in self.inspector_content.winfo_children():
            w.destroy()
            
        node = self.selected_node
        if not node: return
        
        def make_entry(label, key, is_float=False):
            frame = tk.Frame(self.inspector_content)
            frame.pack(fill=tk.X, pady=2)
            tk.Label(frame, text=label, width=10, anchor="w").pack(side=tk.LEFT)
            var = tk.StringVar(value=str(node.get(key, "")))
            entry = tk.Entry(frame, textvariable=var)
            entry.pack(side=tk.LEFT, fill=tk.X, expand=True)
            self.bind_undo(entry)
            
            def on_change(*args):
                val = var.get()
                if is_float:
                    try: val = float(val)
                    except: return
                node[key] = val
                if key == "id": self.refresh_tree()
                self.render_canvas()
            var.trace("w", on_change)
            
        make_entry("ID", "id")
        
        if node.get("id") != "root":
            vis_frame = tk.Frame(self.inspector_content)
            vis_frame.pack(fill=tk.X, pady=4)
            tk.Label(vis_frame, text="Visibility", width=10, anchor="w").pack(side=tk.LEFT)
            
            vis_var = tk.BooleanVar(value=bool(node.get("visible", True)))
            vvis_var = tk.BooleanVar(value=bool(node.get("viewer_visible", True)))
            
            def on_vis_change(*args):
                node["visible"] = vis_var.get()
                if vis_var.get():
                    vvis_var.set(True)
                    node["viewer_visible"] = True
                else:
                    vvis_var.set(False)
                    node["viewer_visible"] = False
                self.render_canvas()
                
            def on_vvis_change(*args):
                node["viewer_visible"] = vvis_var.get()
                self.render_canvas()
                
            vis_cb = tk.Checkbutton(vis_frame, text="Game", variable=vis_var)
            vis_cb.pack(side=tk.LEFT, padx=5)
            self.bind_undo(vis_cb)
            
            vvis_cb = tk.Checkbutton(vis_frame, text="Editor", variable=vvis_var)
            vvis_cb.pack(side=tk.LEFT, padx=5)
            self.bind_undo(vvis_cb)
            
            vis_var.trace("w", on_vis_change)
            vvis_var.trace("w", on_vvis_change)
        
        if node.get("type") not in ("bg", "root") and node.get("id") != "root":
            make_entry("X", "x", True)
            make_entry("Y", "y", True)
            
        if node.get("type") == "bg":
            frame = tk.Frame(self.inspector_content)
            frame.pack(fill=tk.X, pady=2)
            tk.Label(frame, text="Image ID", width=10, anchor="w").pack(side=tk.LEFT)
            var = tk.StringVar(value=node.get("image_id", ""))
            cb = ttk.Combobox(frame, textvariable=var, values=self.bg_list)
            cb.pack(side=tk.LEFT, fill=tk.X, expand=True)
            self.bind_undo(cb)
            def on_bg(*args):
                node["image_id"] = var.get()
                self.image_cache.pop(var.get(), None)
                self.render_canvas()
            var.trace("w", on_bg)
            
        elif node.get("type") == "sprite":
            frame = tk.Frame(self.inspector_content)
            frame.pack(fill=tk.X, pady=2)
            tk.Label(frame, text="Image Set", width=10, anchor="w").pack(side=tk.LEFT)
            set_var = tk.StringVar(value=node.get("image_set", ""))
            set_cb = ttk.Combobox(frame, textvariable=set_var, values=list(self.manifest.keys()))
            set_cb.pack(side=tk.LEFT, fill=tk.X, expand=True)
            self.bind_undo(set_cb)
            
            frame2 = tk.Frame(self.inspector_content)
            frame2.pack(fill=tk.X, pady=2)
            tk.Label(frame2, text="Image No", width=10, anchor="w").pack(side=tk.LEFT)
            no_var = tk.StringVar(value=str(node.get("image_no", "0")))
            no_cb = ttk.Combobox(frame2, textvariable=no_var)
            no_cb.pack(side=tk.LEFT, fill=tk.X, expand=True)
            self.bind_undo(no_cb)
            
            def update_opts(*args):
                node["image_set"] = set_var.get()
                if set_var.get() in self.manifest:
                    no_cb["values"] = [f"{k} ({v})" for k, v in self.manifest[set_var.get()].items()]
                self.render_canvas()
            
            def on_no(*args):
                node["image_no"] = no_var.get().split(" ")[0]
                self.render_canvas()
                
            set_var.trace("w", update_opts)
            no_var.trace("w", on_no)
            update_opts()
            
            # Rotation
            rot_frame = tk.Frame(self.inspector_content)
            rot_frame.pack(fill=tk.X, pady=2)
            tk.Label(rot_frame, text="Rotation (Z)", width=10, anchor="w").pack(side=tk.LEFT)
            rot_var = tk.DoubleVar(value=float(node.get("rotation", 0.0)))
            
            # Slider (for mouse dragging)
            rot_scale = tk.Scale(rot_frame, from_=-180, to=180, resolution=1, orient=tk.HORIZONTAL, variable=rot_var, showvalue=False)
            rot_scale.pack(side=tk.LEFT, fill=tk.X, expand=True)
            self.bind_undo(rot_scale)
            
            # Spinbox (for precise typing/fine tuning)
            rot_spin = tk.Spinbox(rot_frame, from_=-180.0, to=180.0, increment=1.0, textvariable=rot_var, width=5)
            rot_spin.pack(side=tk.RIGHT)
            self.bind_undo(rot_spin)
            
            def on_rot(*args):
                try:
                    node["rotation"] = rot_var.get()
                    self.render_canvas()
                except:
                    pass
            rot_var.trace("w", on_rot)

        elif node.get("type") == "text":
            make_entry("Text", "text")

            # Alignment
            align_frame = tk.Frame(self.inspector_content)
            align_frame.pack(fill=tk.X, pady=2)
            tk.Label(align_frame, text="Align", width=10, anchor="w").pack(side=tk.LEFT)
            align_var = tk.StringVar(value=node.get("align", "center"))
            align_cb = ttk.Combobox(align_frame, textvariable=align_var,
                                    values=["left", "center", "right"], state="readonly", width=10)
            align_cb.pack(side=tk.LEFT)
            self.bind_undo(align_cb)
            def on_align(*args):
                node["align"] = align_var.get()
                self.render_canvas()
            align_var.trace("w", on_align)

            # Font size
            fs_frame = tk.Frame(self.inspector_content)
            fs_frame.pack(fill=tk.X, pady=2)
            tk.Label(fs_frame, text="Font Scale", width=10, anchor="w").pack(side=tk.LEFT)
            fs_var = tk.DoubleVar(value=float(node.get("font_size", 1.0)))
            fs_spin = tk.Spinbox(fs_frame, from_=0.1, to=10.0, increment=0.1, textvariable=fs_var, width=5)
            fs_spin.pack(side=tk.LEFT)
            self.bind_undo(fs_spin)
            def on_fs(*args):
                try:
                    node["font_size"] = fs_var.get()
                    self.render_canvas()
                except:
                    pass
            fs_var.trace("w", on_fs)

        elif node.get("type") == "anim":
            frame = tk.Frame(self.inspector_content)
            frame.pack(fill=tk.X, pady=2)
            tk.Label(frame, text="Preset File", width=10, anchor="w").pack(side=tk.LEFT)
            
            preset_files = []
            try:
                for f in os.listdir(os.path.join(PROJECT_ROOT, "Asset", "animations")):
                    if f.endswith(".anim.json"):
                        preset_files.append(f[:-10])
            except:
                pass
                
            preset_var = tk.StringVar(value=node.get("preset_id", ""))
            preset_cb = ttk.Combobox(frame, textvariable=preset_var, values=preset_files)
            preset_cb.pack(side=tk.LEFT, fill=tk.X, expand=True)
            self.bind_undo(preset_cb)
            
            def on_preset(*args):
                node["preset_id"] = preset_var.get()
                self.render_canvas()
            preset_var.trace("w", on_preset)

            # Playback Controls in Inspector
            ctrl_frame = tk.Frame(self.inspector_content)
            ctrl_frame.pack(fill=tk.X, pady=10)
            
            def start_play():
                preset_id = node.get("preset_id", "")
                preset_path = os.path.join(PROJECT_ROOT, "Asset", "animations", f"{preset_id}.anim.json")
                if not os.path.exists(preset_path):
                    messagebox.showwarning("Warning", f"Preset file {preset_id}.anim.json not found!")
                    return
                    
                with open(preset_path, 'r', encoding='utf-8') as f:
                    preset_data = json.load(f)
                    
                if "keyframes" not in preset_data:
                    global_ease = preset_data.get("ease_type", "LINEAR")
                    preset_data["keyframes"] = [
                        {"frame": 0, "x": float(preset_data.get("start_x", 0.0)), "y": float(preset_data.get("start_y", 0.0)), "rot": float(preset_data.get("start_rot", 0.0)), "scale": float(preset_data.get("start_scale", 1.0)), "ease_type": global_ease},
                        {"frame": int(preset_data.get("duration_frames", 60)), "x": float(preset_data.get("end_x", 0.0)), "y": float(preset_data.get("end_y", 0.0)), "rot": float(preset_data.get("end_rot", 0.0)), "scale": float(preset_data.get("end_scale", 1.0)), "ease_type": "LINEAR"}
                    ]
                else:
                    for k in preset_data["keyframes"]:
                        if "ease_type" not in k:
                            k["ease_type"] = "LINEAR"
                            
                self.playing_ui_anim_node = node
                self.ui_anim_frame = 0
                self.ui_anim_preset_data = preset_data
                self.step_ui_anim()
                
            def stop_play():
                self.playing_ui_anim_node = None
                self.render_canvas()
                
            tk.Button(ctrl_frame, text="▶ Play Preview", command=start_play, bg="#00b894", fg="white", font=("Helvetica", 9, "bold")).pack(side=tk.LEFT, fill=tk.X, expand=True, padx=2)
            tk.Button(ctrl_frame, text="■ Stop", command=stop_play, bg="#d63031", fg="white", font=("Helvetica", 9, "bold")).pack(side=tk.LEFT, fill=tk.X, expand=True, padx=2)

    # ------------------------------------------------------------------
    # Image loading
    # ------------------------------------------------------------------
    def get_image(self, path, rotation=0.0, scale=1.0):
        if not path: return None
        cache_key = (path, rotation, scale)
        if cache_key in self.image_cache: return self.image_cache[cache_key]
        
        found = None
        name = path.split("/")[-1] + ".bmp"
        for root, dirs, files in os.walk(GRAPHICS_DIR):
            if name in files:
                found = os.path.join(root, name)
                break
        if not found: return None
        
        try:
            img = Image.open(found)
            # For backgrounds show only 240x160 area
            if img.width > 240 or img.height > 160:
                img = img.crop((0, 0, 240, 160))
            
            # Ensure the image is in RGBA mode to preserve transparency and make rotated margins transparent
            img = img.convert("RGBA")
            
            # GBA rotates clockwise, PIL rotates counter-clockwise, so we use -rotation.
            # We do this before resize for quality, and expand=True ensures the full rotated texture is preserved.
            if rotation != 0.0:
                img = img.rotate(-rotation, resample=Image.Resampling.NEAREST, expand=True)
                
            target_w = int(img.width * self.scale * scale)
            target_h = int(img.height * self.scale * scale)
            if target_w < 1: target_w = 1
            if target_h < 1: target_h = 1
            img = img.resize((target_w, target_h), Image.Resampling.NEAREST)
            photo = ImageTk.PhotoImage(img)
            self.image_cache[cache_key] = photo
            return photo
        except Exception as e:
            print(f"Error loading {found}: {e}")
            return None

    # ------------------------------------------------------------------
    # Canvas rendering
    # ------------------------------------------------------------------
    def render_canvas(self):
        self.canvas.delete("all")
        
        if self.snap_to_grid.get():
            g = self.grid_size * self.scale
            for x in range(0, 240 * self.scale, g):
                self.canvas.create_line(x, 0, x, 160 * self.scale, fill="#333")
            for y in range(0, 160 * self.scale, g):
                self.canvas.create_line(0, y, 240 * self.scale, y, fill="#333")
                
        def draw_node(node, px, py, parent_visible=True, anim_x=0.0, anim_y=0.0, anim_rot=0.0, anim_scale=1.0, in_anim=False):
            current_visible = parent_visible and bool(node.get("viewer_visible", True))
            
            # If this is the active playing anim node, compute/override animation parameters
            is_active_anim = (self.playing_ui_anim_node is node)
            if is_active_anim:
                ax, ay, a_rot, a_scale = self.get_ui_anim_interpolated(self.ui_anim_frame, self.ui_anim_preset_data)
                anim_x, anim_y, anim_rot, anim_scale = ax, ay, a_rot, a_scale
                in_anim = True
                
            gx = px + float(node.get("x", 0))
            gy = py + float(node.get("y", 0))
            
            if in_anim and (self.playing_ui_anim_node is not node):
                cx = (gx + anim_x + 120) * self.scale
                cy = (gy + anim_y + 80) * self.scale
                rot = anim_rot
                scale_factor = anim_scale
            else:
                cx = (gx + 120) * self.scale
                cy = (gy + 80) * self.scale
                rot = float(node.get("rotation", 0.0)) if "rotation" in node else 0.0
                scale_factor = float(node.get("font_size", 1.0)) if node.get("type") == "text" else 1.0
            
            t = node.get("type")
            if current_visible:
                if t == "bg":
                    img = self.get_image(node.get("image_id", ""))
                    if img:
                        self.canvas.create_image(0, 0, anchor=tk.NW, image=img)
                elif t == "sprite":
                    s, n = node.get("image_set", ""), str(node.get("image_no", "0"))
                    if s in self.manifest and n in self.manifest[s]:
                        img = self.get_image(self.manifest[s][n], rot, scale_factor)
                        if img:
                            self.canvas.create_image(cx, cy, image=img,
                                                     tags=("draggable", str(id(node))))
                            if node is self.selected_node:
                                w, h = img.width(), img.height()
                                self.canvas.create_rectangle(cx-w/2, cy-h/2, cx+w/2, cy+h/2,
                                                             outline="yellow", width=2)
                elif t == "text":
                    align = node.get("align", "center")
                    anchor_map = {"left": tk.W, "center": tk.CENTER, "right": tk.E}
                    anchor = anchor_map.get(align, tk.CENTER)
                    font_size = max(0.1, scale_factor)
                    font = ("Courier", max(1, int(8 * font_size)))
                    txt_id = self.canvas.create_text(
                        cx, cy, text=node.get("text", "Text"),
                        fill="white", font=font, anchor=anchor,
                        tags=("draggable_text", str(id(node)))
                    )
                    if node is self.selected_node:
                        bbox = self.canvas.bbox(txt_id)
                        if bbox:
                            self.canvas.create_rectangle(
                                bbox[0]-2, bbox[1]-2, bbox[2]+2, bbox[3]+2,
                                outline="yellow", width=2
                            )
                elif t == "anim":
                    # Draw a nice dashed representation of the abstract anim node
                    if node is self.selected_node:
                        self.canvas.create_oval(cx-6, cy-6, cx+6, cy+6,
                                                outline="cyan", dash=(2, 2), width=2,
                                                tags=("draggable_text", str(id(node))))
            else:
                # If hidden but currently selected, draw a dotted bounding box so the user can see/drag it!
                if node is self.selected_node:
                    if t == "sprite":
                        s, n = node.get("image_set", ""), str(node.get("image_no", "0"))
                        if s in self.manifest and n in self.manifest[s]:
                            img = self.get_image(self.manifest[s][n], rot, scale_factor)
                            if img:
                                w, h = img.width(), img.height()
                                self.canvas.create_rectangle(cx-w/2, cy-h/2, cx+w/2, cy+h/2,
                                                             outline="yellow", dash=(2, 2), width=2,
                                                             tags=("draggable", str(id(node))))
                    elif t == "text":
                        self.canvas.create_rectangle(cx-15, cy-6, cx+15, cy+6,
                                                     outline="yellow", dash=(2, 2), width=2,
                                                     tags=("draggable_text", str(id(node))))
                    elif t in ("group", "anim"):
                        self.canvas.create_oval(cx-4, cy-4, cx+4, cy+4,
                                                outline="yellow", dash=(2, 2), width=2,
                                                tags=("draggable_text", str(id(node))))
                
            for c in node.get("children", []):
                draw_node(c, gx, gy, current_visible, anim_x, anim_y, anim_rot, anim_scale, in_anim)
                
        draw_node(self.scene["root"], 0, 0, True)

    def step_ui_anim(self):
        if not self.playing_ui_anim_node:
            return
        duration = max(1, self.ui_anim_preset_data.get("duration_frames", 60))
        if self.ui_anim_frame <= duration:
            self.render_canvas()
            self.ui_anim_frame += 1
            self.after(16, self.step_ui_anim)
        else:
            self.playing_ui_anim_node = None
            self.render_canvas()

    def get_ui_anim_interpolated(self, frame, preset_data):
        kfs = sorted(preset_data.get("keyframes", []), key=lambda k: k["frame"])
        if not kfs:
            return 0.0, 0.0, 0.0, 1.0
            
        duration = max(1, preset_data.get("duration_frames", 60))
        frame = max(0, min(duration, frame))
        
        if frame <= kfs[0]["frame"]:
            k = kfs[0]
            return k.get("x", 0.0), k.get("y", 0.0), k.get("rot", 0.0), k.get("scale", 1.0)
        if frame >= kfs[-1]["frame"]:
            k = kfs[-1]
            return k.get("x", 0.0), k.get("y", 0.0), k.get("rot", 0.0), k.get("scale", 1.0)
            
        for i in range(len(kfs) - 1):
            kf_prev = kfs[i]
            kf_next = kfs[i+1]
            if kf_prev["frame"] <= frame <= kf_next["frame"]:
                f_range = kf_next["frame"] - kf_prev["frame"]
                t = 0.0 if f_range == 0 else (frame - kf_prev["frame"]) / f_range
                eased_t = self.ease_ui_curve(t, kf_prev.get("ease_type", "LINEAR"))
                
                x = kf_prev.get("x", 0.0) + (kf_next.get("x", 0.0) - kf_prev.get("x", 0.0)) * eased_t
                y = kf_prev.get("y", 0.0) + (kf_next.get("y", 0.0) - kf_prev.get("y", 0.0)) * eased_t
                rot = kf_prev.get("rot", 0.0) + (kf_next.get("rot", 0.0) - kf_prev.get("rot", 0.0)) * eased_t
                scale = kf_prev.get("scale", 1.0) + (kf_next.get("scale", 1.0) - kf_prev.get("scale", 1.0)) * eased_t
                return x, y, rot, scale
                
        return 0.0, 0.0, 0.0, 1.0

    def ease_ui_curve(self, t, type_str):
        if type_str == "EASE_IN":
            return t * t
        elif type_str == "EASE_OUT":
            return t * (2 - t)
        elif type_str == "EASE_IN_OUT":
            if t < 0.5:
                return 2 * t * t
            return -1 + (4 - 2 * t) * t
        elif type_str == "CUBIC_IN":
            return t * t * t
        elif type_str == "CUBIC_OUT":
            t -= 1
            return 1 + t * t * t
        elif type_str == "CUBIC_IN_OUT":
            if t < 0.5:
                return 4 * t * t * t
            t -= 1
            return 1 + 4 * t * t * t
        elif type_str == "BACK_IN":
            s = 1.70158
            return t * t * ((s + 1) * t - s)
        elif type_str == "BACK_OUT":
            s = 1.70158
            t -= 1
            return 1 + t * t * ((s + 1) * t + s)
        elif type_str == "BOUNCE_OUT":
            if t < 1 / 2.75:
                return 7.5625 * t * t
            elif t < 2 / 2.75:
                t -= 1.5 / 2.75
                return 7.5625 * t * t + 0.75
            elif t < 2.5 / 2.75:
                t -= 2.25 / 2.75
                return 7.5625 * t * t + 0.9375
            else:
                t -= 2.625 / 2.75
                return 7.5625 * t * t + 0.984375
        return t # LINEAR

    # ------------------------------------------------------------------
    # Canvas D&D (sprite positioning)
    # ------------------------------------------------------------------
    def on_canvas_press(self, event):
        items = self.canvas.find_withtag("current")
        if not items: return
        tags = self.canvas.gettags(items[0])
        # "draggable" (sprite) または "draggable_text" (text) どちらも処理する
        if "draggable" in tags:
            node_ptr = tags[1]
        elif "draggable_text" in tags:
            node_ptr = tags[1]
        else:
            return
        node = self._find_by_ptr(self.scene["root"], node_ptr)
        if not node: return
        self.selected_node = node
        self.render_inspector()
        self.render_canvas()
        for i, (_, n) in enumerate(self._hier_nodes):
            if n is node:
                self.lb.selection_clear(0, tk.END)
                self.lb.selection_set(i)
                self.lb.see(i)
                break
        self.drag_data = {"x": event.x, "y": event.y,
                          "start_x": node.get("x", 0),
                          "start_y": node.get("y", 0),
                          "undone": False}

    def _find_by_ptr(self, parent, ptr):
        if str(id(parent)) == ptr: return parent
        for c in parent.get("children", []):
            r = self._find_by_ptr(c, ptr)
            if r: return r
        return None

    def on_canvas_drag(self, event):
        if not self.drag_data or not self.selected_node: return
        
        # Save state once when the drag actually begins moving
        if not self.drag_data.get("undone"):
            self.push_undo()
            self.drag_data["undone"] = True
            
        dx = (event.x - self.drag_data["x"]) / self.scale
        dy = (event.y - self.drag_data["y"]) / self.scale
        nx = self.drag_data["start_x"] + dx
        ny = self.drag_data["start_y"] + dy
        if self.snap_to_grid.get():
            g = self.grid_size
            nx = round(nx / g) * g
            ny = round(ny / g) * g
        self.selected_node["x"] = nx
        self.selected_node["y"] = ny
        self.render_canvas()
        self.render_inspector()

    def on_canvas_release(self, event):
        self.drag_data = None

    # ------------------------------------------------------------------
    # File I/O
    # ------------------------------------------------------------------
    def new_json(self):
        if not self.confirm_discard_changes():
            return
            
        self.scene = {
            "screen": "new_screen",
            "root": {"type": "group", "id": "root", "x": 0, "y": 0, "children": []}
        }
        self.current_filepath = None
        self.screen_name_var.set("new_screen")
        self.title("GBA UI Editor")
        self.status_var.set("Status: New layout created")
        self.selected_node = None
        self.image_cache.clear()
        self.undo_stack.clear()
        self.redo_stack.clear()
        self.saved_scene = copy.deepcopy(self.scene)
        self.refresh_tree()
        self.render_canvas()

    def load_json(self):
        if not self.confirm_discard_changes():
            return
            
        path = filedialog.askopenfilename(initialdir=SCREENS_DIR,
                                          filetypes=[("JSON files", "*.json")])
        if not path: return
        with open(path, 'r', encoding='utf-8') as f:
            self.scene = json.load(f)
        self.current_filepath = path
        self.screen_name_var.set(self.scene.get("screen", "new"))
        self.title(f"GBA UI Editor - {os.path.basename(path)}")
        self.status_var.set(f"Status: Loaded {os.path.basename(path)}")
        self.selected_node = None
        self.image_cache.clear()
        self.undo_stack.clear()
        self.redo_stack.clear()
        self.saved_scene = copy.deepcopy(self.scene)
        self.refresh_tree()
        self.render_canvas()
        
    def save_json(self):
        self.scene["screen"] = self.screen_name_var.get()
        if self.current_filepath:
            path = self.current_filepath
        else:
            path = os.path.join(SCREENS_DIR, f"{self.scene['screen']}.json")
            self.current_filepath = path
            
        with open(path, 'w', encoding='utf-8') as f:
            json.dump(self.scene, f, indent=2)
            
        self.saved_scene = copy.deepcopy(self.scene)
        self.title(f"GBA UI Editor - {os.path.basename(path)}")
        self.status_var.set(f"Status: Saved at {time.strftime('%H:%M:%S')}")

    def show_in_explorer(self):
        import subprocess
        path = self.current_filepath
        if path and os.path.exists(path):
            norm_path = os.path.normpath(path)
            subprocess.Popen(f'explorer /select,"{norm_path}"')
            self.status_var.set(f"Status: Revealed in Explorer")
        else:
            norm_path = os.path.normpath(SCREENS_DIR)
            if os.path.exists(norm_path):
                subprocess.Popen(f'explorer "{norm_path}"')
                self.status_var.set("Status: Opened layouts folder")

if __name__ == "__main__":
    app = GBAUIEditor()
    app.mainloop()
