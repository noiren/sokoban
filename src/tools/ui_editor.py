import tkinter as tk
from tkinter import ttk, filedialog, messagebox
import json
import os
import time
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
        self.geometry("1000x600")
        
        self.scale = 2
        self.grid_size = 8
        self.snap_to_grid = tk.BooleanVar(value=True)
        
        self.scene = {
            "screen": "new_screen",
            "root": {"type": "group", "id": "root", "x": 0, "y": 0, "children": []}
        }
        self.selected_node = None
        self.current_filepath = None
        
        # Bind Ctrl+S
        self.bind('<Control-s>', lambda e: self.save_json())
        self.bind('<Control-S>', lambda e: self.save_json())
        self.drag_data = None
        self.image_cache = {}
        
        self.load_manifest()
        self.bg_list = self.get_bg_list()
        
        self.create_ui()
        self.refresh_tree()
        self.render_canvas()

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
            for f in os.listdir(stills_dir):
                if f.startswith("bg_") and f.endswith(".bmp"):
                    bgs.append(f[:-4])
        return bgs

    def create_ui(self):
        # Toolbar
        toolbar = tk.Frame(self, bd=1, relief=tk.RAISED)
        toolbar.pack(side=tk.TOP, fill=tk.X)
        
        tk.Label(toolbar, text="Screen Name:").pack(side=tk.LEFT, padx=5)
        self.screen_name_var = tk.StringVar(value=self.scene["screen"])
        tk.Entry(toolbar, textvariable=self.screen_name_var, width=15).pack(side=tk.LEFT)
        
        tk.Button(toolbar, text="Load JSON", command=self.load_json).pack(side=tk.LEFT, padx=5)
        tk.Button(toolbar, text="Save JSON (Ctrl+S)", command=self.save_json).pack(side=tk.LEFT, padx=5)
        
        self.status_var = tk.StringVar(value="Status: Ready")
        tk.Label(toolbar, textvariable=self.status_var, fg="#00b894").pack(side=tk.RIGHT, padx=10)
        
        tk.Checkbutton(toolbar, text="Snap to Grid", variable=self.snap_to_grid).pack(side=tk.LEFT, padx=10)
        tk.Label(toolbar, text="Grid Size:").pack(side=tk.LEFT)
        self.grid_size_var = tk.IntVar(value=self.grid_size)
        tk.Entry(toolbar, textvariable=self.grid_size_var, width=5).pack(side=tk.LEFT)
        self.grid_size_var.trace("w", lambda *args: self.update_grid_size())

        # Main Layout
        paned = tk.PanedWindow(self, orient=tk.HORIZONTAL)
        paned.pack(fill=tk.BOTH, expand=True)

        # Left: Hierarchy
        left_frame = tk.Frame(paned, width=200)
        paned.add(left_frame)
        
        tk.Label(left_frame, text="Hierarchy", bg="#444", fg="white").pack(fill=tk.X)
        self.tree = ttk.Treeview(left_frame, selectmode="browse")
        self.tree.pack(fill=tk.BOTH, expand=True)
        self.tree.bind("<<TreeviewSelect>>", self.on_tree_select)
        
        btn_frame = tk.Frame(left_frame)
        btn_frame.pack(fill=tk.X)
        tk.Button(btn_frame, text="+ Sprite", command=lambda: self.add_node("sprite")).pack(side=tk.LEFT, fill=tk.X, expand=True)
        tk.Button(btn_frame, text="+ Text", command=lambda: self.add_node("text")).pack(side=tk.LEFT, fill=tk.X, expand=True)
        tk.Button(btn_frame, text="Delete", command=self.delete_node).pack(side=tk.LEFT, fill=tk.X, expand=True)

        # Center: Canvas
        center_frame = tk.Frame(paned, bg="#222")
        paned.add(center_frame)
        
        self.canvas = tk.Canvas(center_frame, width=240*self.scale, height=160*self.scale, bg="black")
        self.canvas.place(relx=0.5, rely=0.5, anchor=tk.CENTER)
        
        self.canvas.bind("<ButtonPress-1>", self.on_canvas_press)
        self.canvas.bind("<B1-Motion>", self.on_canvas_drag)
        self.canvas.bind("<ButtonRelease-1>", self.on_canvas_release)

        # Right: Inspector
        self.right_frame = tk.Frame(paned, width=250)
        paned.add(self.right_frame)
        tk.Label(self.right_frame, text="Inspector", bg="#444", fg="white").pack(fill=tk.X)
        self.inspector_content = tk.Frame(self.right_frame)
        self.inspector_content.pack(fill=tk.BOTH, expand=True, padx=5, pady=5)

    def update_grid_size(self):
        try:
            self.grid_size = self.grid_size_var.get()
        except:
            pass

    def add_node(self, ntype):
        parent = self.selected_node if self.selected_node and self.selected_node.get("type") == "group" else self.scene["root"]
        if "children" not in parent: parent["children"] = []
        
        node = {"type": ntype, "id": f"new_{ntype}", "x": 0, "y": 0}
        if ntype == "sprite":
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
        if not self.selected_node or self.selected_node["id"] == "root" or self.selected_node.get("type") == "bg":
            return
            
        def remove(parent):
            if "children" not in parent: return False
            if self.selected_node in parent["children"]:
                parent["children"].remove(self.selected_node)
                return True
            for c in parent["children"]:
                if remove(c): return True
            return False
            
        remove(self.scene["root"])
        self.selected_node = None
        self.refresh_tree()
        self.render_canvas()
        self.render_inspector()

    def refresh_tree(self):
        self.tree.delete(*self.tree.get_children())
        
        def insert_node(parent_id, node):
            text = f"[{node.get('type')}] {node.get('id', '')}"
            item_id = str(id(node))
            self.tree.insert(parent_id, "end", item_id, text=text, open=True)
            if "children" in node:
                for c in node["children"]:
                    insert_node(item_id, c)
                    
        insert_node("", self.scene["root"])
        
        # bg node hack
        bg_node = next((c for c in self.scene["root"]["children"] if c.get("type") == "bg"), None)
        if not bg_node:
            bg_node = {"type": "bg", "id": "background", "image_id": ""}
            self.scene["root"]["children"].insert(0, bg_node)
            self.refresh_tree()

    def find_node_by_id(self, parent, target_id):
        if str(id(parent)) == target_id: return parent
        if "children" in parent:
            for c in parent["children"]:
                res = self.find_node_by_id(c, target_id)
                if res: return res
        return None

    def on_tree_select(self, event):
        sel = self.tree.selection()
        if not sel: return
        self.selected_node = self.find_node_by_id(self.scene["root"], sel[0])
        self.render_inspector()
        self.render_canvas()

    def render_inspector(self):
        for widget in self.inspector_content.winfo_children():
            widget.destroy()
            
        node = self.selected_node
        if not node: return
        
        def make_entry(label, key, is_float=False):
            frame = tk.Frame(self.inspector_content)
            frame.pack(fill=tk.X, pady=2)
            tk.Label(frame, text=label, width=10, anchor="w").pack(side=tk.LEFT)
            var = tk.StringVar(value=str(node.get(key, "")))
            entry = tk.Entry(frame, textvariable=var)
            entry.pack(side=tk.LEFT, fill=tk.X, expand=True)
            
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
        
        if node.get("type") != "bg" and node.get("id") != "root":
            make_entry("X", "x", True)
            make_entry("Y", "y", True)
            
        if node.get("type") == "bg":
            frame = tk.Frame(self.inspector_content)
            frame.pack(fill=tk.X, pady=2)
            tk.Label(frame, text="Image ID", width=10, anchor="w").pack(side=tk.LEFT)
            var = tk.StringVar(value=node.get("image_id", ""))
            cb = ttk.Combobox(frame, textvariable=var, values=self.bg_list)
            cb.pack(side=tk.LEFT, fill=tk.X, expand=True)
            def on_bg_change(*args):
                node["image_id"] = var.get()
                self.render_canvas()
            var.trace("w", on_bg_change)
            
        elif node.get("type") == "sprite":
            # Image Set
            frame = tk.Frame(self.inspector_content)
            frame.pack(fill=tk.X, pady=2)
            tk.Label(frame, text="Image Set", width=10, anchor="w").pack(side=tk.LEFT)
            set_var = tk.StringVar(value=node.get("image_set", ""))
            set_cb = ttk.Combobox(frame, textvariable=set_var, values=list(self.manifest.keys()))
            set_cb.pack(side=tk.LEFT, fill=tk.X, expand=True)
            
            # Image No
            frame2 = tk.Frame(self.inspector_content)
            frame2.pack(fill=tk.X, pady=2)
            tk.Label(frame2, text="Image No", width=10, anchor="w").pack(side=tk.LEFT)
            no_var = tk.StringVar(value=str(node.get("image_no", "0")))
            no_cb = ttk.Combobox(frame2, textvariable=no_var)
            no_cb.pack(side=tk.LEFT, fill=tk.X, expand=True)
            
            def update_no_options(*args):
                node["image_set"] = set_var.get()
                if set_var.get() in self.manifest:
                    opts = [f"{k} ({v})" for k,v in self.manifest[set_var.get()].items()]
                    no_cb["values"] = opts
                self.render_canvas()
            
            def on_no_change(*args):
                val = no_var.get().split(" ")[0] # extract just the number
                node["image_no"] = val
                self.render_canvas()
                
            set_var.trace("w", update_no_options)
            no_var.trace("w", on_no_change)
            update_no_options() # init

    def get_image(self, path):
        if path in self.image_cache: return self.image_cache[path]
        
        # Determine if it's a BG or a Sprite
        if path.startswith("bg_"):
            full_path = os.path.join(GRAPHICS_DIR, "stills", path + ".bmp")
        else:
            full_path = os.path.join(GRAPHICS_DIR, "tiles", path.replace("ui/", "ui/").replace("chara/", "chara/").replace("gallery/", "ui/") + ".bmp")
            # The above is a bit messy since path already contains relative paths from old manifest like "ui/common/spr_...".
            # Let's just try to find the file recursively in Asset/graphics
            found_path = None
            for root, dirs, files in os.walk(GRAPHICS_DIR):
                if path.split("/")[-1] + ".bmp" in files:
                    found_path = os.path.join(root, path.split("/")[-1] + ".bmp")
                    break
            if found_path:
                full_path = found_path
            else:
                return None
                
        if not os.path.exists(full_path): return None
        try:
            img = Image.open(full_path)
            img = img.resize((img.width * self.scale, img.height * self.scale), Image.Resampling.NEAREST)
            photo = ImageTk.PhotoImage(img)
            self.image_cache[path] = photo
            return photo
        except Exception as e:
            print(f"Error loading {full_path}: {e}")
            return None

    def render_canvas(self):
        self.canvas.delete("all")
        
        # Grid
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
            
            if node.get("type") == "bg":
                img = self.get_image(node.get("image_id", ""))
                if img: self.canvas.create_image(img.width()/2, img.height()/2, image=img)
            elif node.get("type") == "sprite":
                img_set = node.get("image_set", "")
                img_no = str(node.get("image_no", "0"))
                if img_set in self.manifest and img_no in self.manifest[img_set]:
                    path = self.manifest[img_set][img_no]
                    img = self.get_image(path)
                    if img:
                        item_id = self.canvas.create_image(cx, cy, image=img, tags=("draggable", str(id(node))))
                        if node == self.selected_node:
                            self.canvas.create_rectangle(
                                cx - img.width()/2, cy - img.height()/2, 
                                cx + img.width()/2, cy + img.height()/2, 
                                outline="yellow", width=2)
            elif node.get("type") == "text":
                self.canvas.create_text(cx, cy, text=node.get("text", "Text"), fill="white", font=("Courier", 12*self.scale))
                
            if "children" in node:
                for c in node["children"]: draw_node(c, gx, gy)
                
        draw_node(self.scene["root"], 0, 0)

    def on_canvas_press(self, event):
        items = self.canvas.find_withtag("current")
        if not items: return
        tags = self.canvas.gettags(items[0])
        if "draggable" in tags:
            node_id = tags[1]
            self.selected_node = self.find_node_by_id(self.scene["root"], node_id)
            self.render_inspector()
            self.render_canvas()
            
            # Select in tree
            for item in self.tree.get_children(""):
                if item == node_id: self.tree.selection_set(item)
            
            self.drag_data = {"x": event.x, "y": event.y, "start_x": self.selected_node.get("x", 0), "start_y": self.selected_node.get("y", 0)}

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

    def load_json(self):
        path = filedialog.askopenfilename(initialdir=SCREENS_DIR, filetypes=[("JSON files", "*.json")])
        if not path: return
        with open(path, 'r', encoding='utf-8') as f:
            self.scene = json.load(f)
        self.current_filepath = path
        self.screen_name_var.set(self.scene.get("screen", "new"))
        self.title(f"GBA Sokoban UI Editor - {os.path.basename(path)}")
        self.status_var.set(f"Status: Loaded {os.path.basename(path)}")
        self.selected_node = None
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
