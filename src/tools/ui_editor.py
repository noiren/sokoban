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
        self.title("GBA Sokoban UI Editor (Python)")
        
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
        self.bind('<Delete>',    lambda e: self.delete_node())
        self.bind('<BackSpace>', lambda e: self.delete_node())
        self.bind('<Control-c>', lambda e: self.copy_node())
        self.bind('<Control-C>', lambda e: self.copy_node())
        self.bind('<Control-v>', lambda e: self.paste_node())
        self.bind('<Control-V>', lambda e: self.paste_node())
        
        self.clipboard_node = None
        self.drag_data = None        # canvas sprite drag
        self.image_cache = {}

        # Listbox hierarchy state
        self._hier_nodes = []        # [(depth, node_dict), ...]
        self._drag_src_idx = None
        self._drag_press_y = None
        
        self.load_manifest()
        self.bg_list = self.get_bg_list()
        
        self.create_ui()
        self.refresh_tree()
        self.render_canvas()
        
        # Apply saved sash positions after UI is drawn
        self.after(100, self.apply_sashes)

    def on_close(self):
        self.save_editor_config()
        self.destroy()

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
        parent = (self.selected_node
                  if self.selected_node and self.selected_node.get("type") in ("group", "root")
                  else self.scene["root"])
        parent.setdefault("children", [])
        
        node = {"type": ntype, "id": f"new_{ntype}", "x": 0, "y": 0}
        if ntype == "group":
            node["children"] = []
        elif ntype == "sprite":
            node["image_set"] = list(self.manifest.keys())[0] if self.manifest else ""
            node["image_no"] = "0"
            node["visible"] = True
        elif ntype == "text":
            node["text"] = "Text"
            node["visible"] = True
            
        parent["children"].append(node)
        self.refresh_tree()
        self.render_canvas()

    def delete_node(self):
        if not self.selected_node:
            return
        if self.selected_node["id"] == "root" or self.selected_node.get("type") == "bg":
            return
            
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
        icons = {"group": "▶", "bg": "▣", "sprite": "★", "text": "T", "root": "⊞"}
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

        if is_ancestor(src_node, tgt_node):
            return

        # Remove from current parent
        def remove_node(parent):
            ch = parent.get("children", [])
            if src_node in ch:
                ch.remove(src_node)
                return True
            return any(remove_node(c) for c in ch)

        remove_node(self.scene["root"])

        # Insert: group/root → append as child; else → insert after
        if tgt_node.get("type") in ("group", "root"):
            tgt_node.setdefault("children", []).append(src_node)
        else:
            def insert_after(parent):
                ch = parent.get("children", [])
                if tgt_node in ch:
                    ch.insert(ch.index(tgt_node) + 1, src_node)
                    return True
                return any(insert_after(c) for c in ch)
            insert_after(self.scene["root"])

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
            tk.Entry(frame, textvariable=var).pack(side=tk.LEFT, fill=tk.X, expand=True)
            
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
            
            frame2 = tk.Frame(self.inspector_content)
            frame2.pack(fill=tk.X, pady=2)
            tk.Label(frame2, text="Image No", width=10, anchor="w").pack(side=tk.LEFT)
            no_var = tk.StringVar(value=str(node.get("image_no", "0")))
            no_cb = ttk.Combobox(frame2, textvariable=no_var)
            no_cb.pack(side=tk.LEFT, fill=tk.X, expand=True)
            
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

        elif node.get("type") == "text":
            make_entry("Text", "text")

    # ------------------------------------------------------------------
    # Image loading
    # ------------------------------------------------------------------
    def get_image(self, path):
        if not path: return None
        if path in self.image_cache: return self.image_cache[path]
        
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
            img = img.resize((img.width * self.scale, img.height * self.scale),
                              Image.Resampling.NEAREST)
            photo = ImageTk.PhotoImage(img)
            self.image_cache[path] = photo
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
                
        def draw_node(node, px, py):
            gx = px + float(node.get("x", 0))
            gy = py + float(node.get("y", 0))
            cx = (gx + 120) * self.scale
            cy = (gy + 80) * self.scale
            
            t = node.get("type")
            if t == "bg":
                img = self.get_image(node.get("image_id", ""))
                if img:
                    self.canvas.create_image(0, 0, anchor=tk.NW, image=img)
            elif t == "sprite":
                s, n = node.get("image_set", ""), str(node.get("image_no", "0"))
                if s in self.manifest and n in self.manifest[s]:
                    img = self.get_image(self.manifest[s][n])
                    if img:
                        self.canvas.create_image(cx, cy, image=img,
                                                 tags=("draggable", str(id(node))))
                        if node is self.selected_node:
                            w, h = img.width(), img.height()
                            self.canvas.create_rectangle(cx-w/2, cy-h/2, cx+w/2, cy+h/2,
                                                         outline="yellow", width=2)
            elif t == "text":
                self.canvas.create_text(cx, cy, text=node.get("text", "Text"),
                                        fill="white", font=("Courier", 10))
                
            for c in node.get("children", []):
                draw_node(c, gx, gy)
                
        draw_node(self.scene["root"], 0, 0)

    # ------------------------------------------------------------------
    # Canvas D&D (sprite positioning)
    # ------------------------------------------------------------------
    def on_canvas_press(self, event):
        items = self.canvas.find_withtag("current")
        if not items: return
        tags = self.canvas.gettags(items[0])
        if "draggable" not in tags: return
        node = self._find_by_ptr(self.scene["root"], tags[1])
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
                          "start_y": node.get("y", 0)}

    def _find_by_ptr(self, parent, ptr):
        if str(id(parent)) == ptr: return parent
        for c in parent.get("children", []):
            r = self._find_by_ptr(c, ptr)
            if r: return r
        return None

    def on_canvas_drag(self, event):
        if not self.drag_data or not self.selected_node: return
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
        if messagebox.askyesno("Confirm", "Are you sure you want to create a new layout? Unsaved changes will be lost."):
            self.scene = {
                "screen": "new_screen",
                "root": {"type": "group", "id": "root", "x": 0, "y": 0, "children": []}
            }
            self.current_filepath = None
            self.screen_name_var.set("new_screen")
            self.title("GBA Sokoban UI Editor (Python)")
            self.status_var.set("Status: New layout created")
            self.selected_node = None
            self.image_cache.clear()
            self.refresh_tree()
            self.render_canvas()

    def load_json(self):
        path = filedialog.askopenfilename(initialdir=SCREENS_DIR,
                                          filetypes=[("JSON files", "*.json")])
        if not path: return
        with open(path, 'r', encoding='utf-8') as f:
            self.scene = json.load(f)
        self.current_filepath = path
        self.screen_name_var.set(self.scene.get("screen", "new"))
        self.title(f"GBA Sokoban UI Editor - {os.path.basename(path)}")
        self.status_var.set(f"Status: Loaded {os.path.basename(path)}")
        self.selected_node = None
        self.image_cache.clear()
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
            
        self.title(f"GBA Sokoban UI Editor - {os.path.basename(path)}")
        self.status_var.set(f"Status: Saved at {time.strftime('%H:%M:%S')}")

if __name__ == "__main__":
    app = GBAUIEditor()
    app.mainloop()
