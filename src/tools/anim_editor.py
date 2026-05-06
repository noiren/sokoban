import tkinter as tk
from tkinter import ttk, messagebox, filedialog
import json
import os
import math

SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))
PROJECT_ROOT = os.path.dirname(os.path.dirname(SCRIPT_DIR))
ANIMS_DIR = os.path.join(PROJECT_ROOT, "Asset", "animations")

EASE_TYPES = [
    "LINEAR", 
    "EASE_IN", "EASE_OUT", "EASE_IN_OUT", 
    "CUBIC_IN", "CUBIC_OUT", "CUBIC_IN_OUT",
    "BACK_IN", "BACK_OUT", "BOUNCE_OUT"
]

class AnimEditor(tk.Tk):
    def __init__(self):
        super().__init__()
        self.title("GBA Anim Editor")
        self.geometry("800x600")
        
        os.makedirs(ANIMS_DIR, exist_ok=True)
        
        self.anim_data = {
            "id": "new_anim",
            "duration_frames": 60,
            "ease_type": "LINEAR",
            "keyframes": [
                {"frame": 0, "x": 0.0, "y": 60.0, "rot": 0.0, "scale": 1.0, "ease_type": "LINEAR"},
                {"frame": 60, "x": 0.0, "y": 0.0, "rot": 0.0, "scale": 1.0, "ease_type": "LINEAR"}
            ]
        }
        self.current_filepath = None
        self.is_playing = False
        self.current_frame = 0
        
        self.timeline_window = None
        self.selected_kf_idx = -1
        self._dragging_kf_idx = -1
        
        self.create_ui()
        self.open_timeline()
        self.render_canvas()
        
        # Bind space to toggle playback
        self.bind("<space>", lambda e: self.toggle_play())

    def toggle_play(self):
        if self.is_playing:
            self.is_playing = False
        else:
            self.play_preview()

    def create_ui(self):
        toolbar = tk.Frame(self, bd=1, relief=tk.RAISED)
        toolbar.pack(side=tk.TOP, fill=tk.X)
        
        tk.Button(toolbar, text="New", command=self.new_json).pack(side=tk.LEFT, padx=5)
        tk.Button(toolbar, text="Load", command=self.load_json).pack(side=tk.LEFT, padx=5)
        tk.Button(toolbar, text="Save", command=self.save_json).pack(side=tk.LEFT, padx=5)
        tk.Button(toolbar, text="Play Preview (Space)", command=self.play_preview).pack(side=tk.LEFT, padx=5)
        tk.Button(toolbar, text="Open Timeline Window", command=self.open_timeline).pack(side=tk.LEFT, padx=5)
        
        self.paned = tk.PanedWindow(self, orient=tk.HORIZONTAL)
        self.paned.pack(fill=tk.BOTH, expand=True)
        
        # Left: Inspector
        self.inspector = tk.Frame(self.paned, width=320)
        self.paned.add(self.inspector, minsize=280)
        
        tk.Label(self.inspector, text="Animation Settings", bg="#444", fg="white").pack(fill=tk.X)
        self.prop_frame = tk.Frame(self.inspector)
        self.prop_frame.pack(fill=tk.BOTH, expand=True, padx=5, pady=5)
        
        self.entries = {}
        self.build_inspector()
        
        # Right: Preview Canvas
        self.canvas_frame = tk.Frame(self.paned)
        self.paned.add(self.canvas_frame, stretch="always")
        
        tk.Label(self.canvas_frame, text="Preview (60fps)", bg="#444", fg="white").pack(fill=tk.X)
        self.canvas = tk.Canvas(self.canvas_frame, bg="#2d3436")
        self.canvas.pack(fill=tk.BOTH, expand=True)

    def build_inspector(self):
        for widget in self.prop_frame.winfo_children():
            widget.destroy()
            
        def make_entry(label, key, is_float=False, is_int=False):
            frame = tk.Frame(self.prop_frame)
            frame.pack(fill=tk.X, pady=2)
            tk.Label(frame, text=label, width=15, anchor="w").pack(side=tk.LEFT)
            var = tk.StringVar(value=str(self.anim_data.get(key, "")))
            entry = tk.Entry(frame, textvariable=var)
            entry.pack(side=tk.LEFT, fill=tk.X, expand=True)
            
            def on_change(*args):
                val = var.get()
                if is_float:
                    try: val = float(val)
                    except: return
                elif is_int:
                    try: val = int(val)
                    except: return
                self.anim_data[key] = val
                self.render_canvas()
                self.render_timeline_graph()
            var.trace("w", on_change)
            self.entries[key] = var
            
        make_entry("ID", "id")
        make_entry("Duration (frames)", "duration_frames", is_int=True)
        
        # Keyframe property section
        self.kf_section = tk.LabelFrame(self.prop_frame, text="Selected Keyframe")
        self.kf_section.pack(fill=tk.BOTH, expand=True, pady=10)
        self.build_keyframe_section()

    def build_keyframe_section(self):
        for w in self.kf_section.winfo_children():
            w.destroy()
            
        kfs = self.anim_data.get("keyframes", [])
        if self.selected_kf_idx < 0 or self.selected_kf_idx >= len(kfs):
            tk.Label(self.kf_section, text="Select a keyframe on the graph\nto edit its values here.", fg="#888", pady=15).pack()
            return
            
        kf = kfs[self.selected_kf_idx]
        
        def make_kf_entry(label, field_key, is_float=False):
            frame = tk.Frame(self.kf_section)
            frame.pack(fill=tk.X, padx=5, pady=2)
            tk.Label(frame, text=label, width=10, anchor="w").pack(side=tk.LEFT)
            var = tk.StringVar(value=str(kf.get(field_key, 0.0)))
            entry = tk.Entry(frame, textvariable=var)
            entry.pack(side=tk.LEFT, fill=tk.X, expand=True)
            
            def on_kf_change(*args):
                try:
                    val = float(var.get()) if is_float else int(var.get())
                    kf[field_key] = val
                    self.render_canvas()
                    self.render_timeline_graph()
                except:
                    pass
            var.trace("w", on_kf_change)
            
        # Frame Number
        frame = tk.Frame(self.kf_section)
        frame.pack(fill=tk.X, padx=5, pady=2)
        tk.Label(frame, text="Frame", width=10, anchor="w").pack(side=tk.LEFT)
        f_var = tk.StringVar(value=str(kf["frame"]))
        f_entry = tk.Entry(frame, textvariable=f_var)
        f_entry.pack(side=tk.LEFT, fill=tk.X, expand=True)
        # Lock frame 0 and last frame from moving frame index via textbox
        if self.selected_kf_idx == 0 or self.selected_kf_idx == len(kfs) - 1:
            f_entry.configure(state="disabled")
        else:
            def on_frame_change(*args):
                try:
                    new_f = int(f_var.get())
                    duration = max(1, self.anim_data.get("duration_frames", 60))
                    new_f = max(1, min(duration - 1, new_f))
                    kf["frame"] = new_f
                    # Re-sort keyframes and preserve selection
                    current_kf = self.anim_data["keyframes"][self.selected_kf_idx]
                    self.anim_data["keyframes"] = sorted(self.anim_data["keyframes"], key=lambda k: k["frame"])
                    self.selected_kf_idx = self.anim_data["keyframes"].index(current_kf)
                    self.render_canvas()
                    self.render_timeline_graph()
                except:
                    pass
            f_var.trace("w", on_frame_change)
            
        make_kf_entry("X Pos", "x", is_float=True)
        make_kf_entry("Y Pos", "y", is_float=True)
        make_kf_entry("Rotation", "rot", is_float=True)
        make_kf_entry("Scale", "scale", is_float=True)
        
        # Segment Ease Type (Interpolation to next keyframe)
        if self.selected_kf_idx < len(kfs) - 1:
            frame = tk.Frame(self.kf_section)
            frame.pack(fill=tk.X, padx=5, pady=2)
            tk.Label(frame, text="Ease to Next", width=10, anchor="w").pack(side=tk.LEFT)
            ease_var = tk.StringVar(value=kf.get("ease_type", "LINEAR"))
            ease_cb = ttk.Combobox(frame, textvariable=ease_var, values=EASE_TYPES, state="readonly")
            ease_cb.pack(side=tk.LEFT, fill=tk.X, expand=True)
            def on_kf_ease(*args):
                kf["ease_type"] = ease_var.get()
                self.render_canvas()
                self.render_timeline_graph()
            ease_var.trace("w", on_kf_ease)

    def refresh_inspector(self):
        for k, v in self.anim_data.items():
            if k in self.entries:
                self.entries[k].set(str(v))
        self.build_keyframe_section()

    def ease(self, t, type_str):
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

    def get_interpolated_val(self, frame):
        kfs = sorted(self.anim_data.get("keyframes", []), key=lambda k: k["frame"])
        if not kfs:
            return 0.0, 0.0, 0.0, 1.0
            
        duration = max(1, self.anim_data.get("duration_frames", 60))
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
                eased_t = self.ease(t, kf_prev.get("ease_type", "LINEAR"))
                
                x = kf_prev.get("x", 0.0) + (kf_next.get("x", 0.0) - kf_prev.get("x", 0.0)) * eased_t
                y = kf_prev.get("y", 0.0) + (kf_next.get("y", 0.0) - kf_prev.get("y", 0.0)) * eased_t
                rot = kf_prev.get("rot", 0.0) + (kf_next.get("rot", 0.0) - kf_prev.get("rot", 0.0)) * eased_t
                scale = kf_prev.get("scale", 1.0) + (kf_next.get("scale", 1.0) - kf_prev.get("scale", 1.0)) * eased_t
                return x, y, rot, scale
                
        return 0.0, 0.0, 0.0, 1.0

    def render_canvas(self):
        self.canvas.delete("all")
        cx, cy = 400, 300
        
        self.canvas.create_line(cx, 0, cx, 600, fill="#555", dash=(2, 2))
        self.canvas.create_line(0, cy, 800, cy, fill="#555", dash=(2, 2))
        
        x, y, rot, scale = self.get_interpolated_val(self.current_frame)
        
        box_cx = cx + x
        box_cy = cy + y
        
        # Simple rotation and scale math for box
        w, h = 20 * scale, 20 * scale
        points = [
            (-w, -h), (w, -h), (w, h), (-w, h)
        ]
        
        rad = math.radians(rot)
        cos_a = math.cos(rad)
        sin_a = math.sin(rad)
        
        rotated_points = []
        for px, py in points:
            rx = px * cos_a - py * sin_a
            ry = px * sin_a + py * cos_a
            rotated_points.extend([box_cx + rx, box_cy + ry])
            
        self.canvas.create_polygon(rotated_points, outline="yellow", fill="#0984e3", width=2)
        self.render_timeline_graph()

    def play_preview(self):
        if self.is_playing: return
        self.is_playing = True
        self.current_frame = 0
        self.animate_step()
        
    def animate_step(self):
        if not self.is_playing: return
        self.render_canvas()
        self.current_frame += 1
        duration = max(1, self.anim_data.get("duration_frames", 60))
        if self.current_frame <= duration:
            self.after(16, self.animate_step)
        else:
            self.is_playing = False
            self.render_canvas()

    # Timeline Window & Graph UI
    def open_timeline(self):
        if self.timeline_window and tk.Toplevel.winfo_exists(self.timeline_window):
            self.timeline_window.lift()
            return
            
        self.timeline_window = tk.Toplevel(self)
        self.timeline_window.title("Timeline & Graph Editor")
        self.timeline_window.geometry("700x400")
        
        top_frame = tk.Frame(self.timeline_window)
        top_frame.pack(fill=tk.X, pady=5)
        
        tk.Label(top_frame, text="Active Channel:").pack(side=tk.LEFT, padx=5)
        self.channel_var = tk.StringVar(value="X Position")
        channel_cb = ttk.Combobox(top_frame, textvariable=self.channel_var, values=["X Position", "Y Position", "Rotation", "Scale"], state="readonly")
        channel_cb.pack(side=tk.LEFT, padx=5)
        channel_cb.bind("<<ComboboxSelected>>", lambda e: self.render_timeline_graph())
        
        tk.Button(top_frame, text="Add Keyframe", command=self.add_keyframe_at_current).pack(side=tk.LEFT, padx=5)
        tk.Button(top_frame, text="Delete Selected Keyframe", command=self.delete_selected_keyframe).pack(side=tk.LEFT, padx=5)
        
        self.graph = tk.Canvas(self.timeline_window, bg="#1e272e")
        self.graph.pack(fill=tk.BOTH, expand=True, padx=5, pady=5)
        
        self.graph.bind("<B1-Motion>", self.on_graph_drag)
        self.graph.bind("<ButtonPress-1>", self.on_graph_press)
        self.graph.bind("<ButtonRelease-1>", self.on_graph_release)
        self.graph.bind("<Double-Button-1>", self.on_graph_double_click)
        self.graph.bind("<Configure>", lambda e: self.render_timeline_graph())
        
        # Bind space to toggle playback inside Timeline Window too
        self.timeline_window.bind("<space>", lambda e: self.toggle_play())
        
        self.render_timeline_graph()

    def get_val_range(self):
        ch = self.channel_var.get() if hasattr(self, "channel_var") else "X Position"
        if ch == "X Position" or ch == "Y Position":
            return -120.0, 120.0, "px"
        elif ch == "Rotation":
            return -180.0, 180.0, "deg"
        else:
            return 0.1, 4.0, "scale"

    def get_kf_field(self):
        ch = self.channel_var.get()
        if ch == "X Position": return "x"
        if ch == "Y Position": return "y"
        if ch == "Rotation": return "rot"
        return "scale"

    def render_timeline_graph(self):
        if not self.timeline_window or not tk.Toplevel.winfo_exists(self.timeline_window):
            return
            
        self.graph.delete("all")
        width = self.graph.winfo_width()
        height = self.graph.winfo_height()
        if width <= 1 or height <= 1:
            width, height = 690, 300
            
        duration = max(1, self.anim_data.get("duration_frames", 60))
        vmin, vmax, unit = self.get_val_range()
        field = self.get_kf_field()
        
        gw = width - 80
        gh = height - 60
        
        def to_x(frame):
            return 40 + (frame / duration) * gw
            
        def to_y(val):
            norm = (val - vmin) / (vmax - vmin)
            return height - 30 - norm * gh
            
        # Draw grid
        self.graph.create_line(40, height-30, 40+gw, height-30, fill="#555")
        self.graph.create_line(40, 30, 40, 30+gh, fill="#555")
        
        # Zero line if cross zero
        if vmin <= 0 <= vmax:
            y0 = to_y(0.0)
            self.graph.create_line(40, y0, 40+gw, y0, fill="#3d4e5e", dash=(2, 2))
            
        # Draw background labels
        self.graph.create_text(10, 30, text=f"{vmax:.1f}", fill="#888", anchor="w")
        self.graph.create_text(10, height-30, text=f"{vmin:.1f}", fill="#888", anchor="w")
        
        # Plot Eased Lines
        pts = []
        for frame in range(duration + 1):
            x, y, rot, scale = self.get_interpolated_val(frame)
            cur_v = x if field == "x" else (y if field == "y" else (rot if field == "rot" else scale))
            pts.extend([to_x(frame), to_y(cur_v)])
            
        if len(pts) >= 4:
            self.graph.create_line(pts, fill="#00bec4", width=2)
            
        # Draw Keyframe Dots
        kfs = self.anim_data.get("keyframes", [])
        for i, k in enumerate(kfs):
            kx = to_x(k["frame"])
            ky = to_y(k.get(field, 0.0))
            
            color = "#ff7675" if i == self.selected_kf_idx else "#eccc68"
            self.graph.create_oval(kx-6, ky-6, kx+6, ky+6, fill=color, outline="white", width=1.5, tags=f"kf_{i}")
            
        # Draw current scrubber line
        sc_x = to_x(self.current_frame)
        self.graph.create_line(sc_x, 10, sc_x, height-10, fill="white", width=1)
        self.graph.create_text(sc_x, 15, text=f"{self.current_frame}f", fill="white", font=("Helvetica", 8))

    def on_graph_press(self, event):
        kfs = self.anim_data.get("keyframes", [])
        duration = max(1, self.anim_data.get("duration_frames", 60))
        vmin, vmax, _ = self.get_val_range()
        field = self.get_kf_field()
        
        width = self.graph.winfo_width()
        height = self.graph.winfo_height()
        gw = width - 80
        gh = height - 60
        
        # Check if clicked a keyframe
        for i, k in enumerate(kfs):
            kx = 40 + (k["frame"] / duration) * gw
            ky = height - 30 - ((k.get(field, 0.0) - vmin) / (vmax - vmin)) * gh
            dist = math.hypot(event.x - kx, event.y - ky)
            if dist < 8:
                self.selected_kf_idx = i
                self._dragging_kf_idx = i
                self.build_keyframe_section()
                self.render_timeline_graph()
                return
                
        # Clicked empty space: scrub timeline
        t = (event.x - 40) / gw
        t = max(0.0, min(1.0, t))
        self.current_frame = int(t * duration)
        self.render_canvas()

    def on_graph_drag(self, event):
        duration = max(1, self.anim_data.get("duration_frames", 60))
        vmin, vmax, _ = self.get_val_range()
        field = self.get_kf_field()
        
        width = self.graph.winfo_width()
        height = self.graph.winfo_height()
        gw = width - 80
        gh = height - 60
        
        if self._dragging_kf_idx >= 0:
            kfs = self.anim_data["keyframes"]
            kf = kfs[self._dragging_kf_idx]
            
            # 1. Update value based on vertical coordinate
            norm_y = (height - 30 - event.y) / gh
            norm_y = max(0.0, min(1.0, norm_y))
            val = vmin + norm_y * (vmax - vmin)
            kf[field] = round(val, 2)
            
            # 2. Update frame based on horizontal coordinate (do not move index 0 or final index frame)
            if self._dragging_kf_idx != 0 and self._dragging_kf_idx != len(kfs) - 1:
                t = (event.x - 40) / gw
                t = max(0.0, min(1.0, t))
                new_f = int(t * duration)
                # Keep bounding order
                new_f = max(1, min(duration - 1, new_f))
                kf["frame"] = new_f
                
            self.render_canvas()
            self.build_keyframe_section()
        else:
            # Scrub
            t = (event.x - 40) / gw
            t = max(0.0, min(1.0, t))
            self.current_frame = int(t * duration)
            self.render_canvas()

    def on_graph_release(self, event):
        if self._dragging_kf_idx >= 0:
            current_kf = self.anim_data["keyframes"][self._dragging_kf_idx]
            # Sort array chronologically and restore selected index
            self.anim_data["keyframes"] = sorted(self.anim_data["keyframes"], key=lambda k: k["frame"])
            self.selected_kf_idx = self.anim_data["keyframes"].index(current_kf)
            self._dragging_kf_idx = -1
            self.render_canvas()
            self.build_keyframe_section()

    def on_graph_double_click(self, event):
        duration = max(1, self.anim_data.get("duration_frames", 60))
        width = self.graph.winfo_width()
        gw = width - 80
        
        t = (event.x - 40) / gw
        t = max(0.0, min(1.0, t))
        new_frame = int(t * duration)
        
        # Check if keyframe already exists at this exact frame
        kfs = self.anim_data["keyframes"]
        if any(k["frame"] == new_frame for k in kfs):
            return
            
        # Get interpolated values at this frame to start off with
        x, y, rot, scale = self.get_interpolated_val(new_frame)
        
        new_kf = {
            "frame": new_frame,
            "x": round(x, 1),
            "y": round(y, 1),
            "rot": round(rot, 1),
            "scale": round(scale, 2),
            "ease_type": "LINEAR" # default ease to next
        }
        
        kfs.append(new_kf)
        self.anim_data["keyframes"] = sorted(kfs, key=lambda k: k["frame"])
        self.selected_kf_idx = self.anim_data["keyframes"].index(new_kf)
        
        self.render_canvas()
        self.build_keyframe_section()

    def add_keyframe_at_current(self):
        kfs = self.anim_data["keyframes"]
        if any(k["frame"] == self.current_frame for k in kfs):
            messagebox.showwarning("Warning", f"Keyframe at frame {self.current_frame} already exists!")
            return
            
        x, y, rot, scale = self.get_interpolated_val(self.current_frame)
        new_kf = {
            "frame": self.current_frame,
            "x": round(x, 1),
            "y": round(y, 1),
            "rot": round(rot, 1),
            "scale": round(scale, 2),
            "ease_type": "LINEAR"
        }
        kfs.append(new_kf)
        self.anim_data["keyframes"] = sorted(kfs, key=lambda k: k["frame"])
        self.selected_kf_idx = self.anim_data["keyframes"].index(new_kf)
        
        self.render_canvas()
        self.build_keyframe_section()

    def delete_selected_keyframe(self):
        kfs = self.anim_data["keyframes"]
        if self.selected_kf_idx <= 0 or self.selected_kf_idx >= len(kfs) - 1:
            messagebox.showwarning("Warning", "Cannot delete start or end keyframes!")
            return
            
        kfs.pop(self.selected_kf_idx)
        self.selected_kf_idx = -1
        self.render_canvas()
        self.build_keyframe_section()

    # File I/O
    def new_json(self):
        self.anim_data = {
            "id": "new_anim",
            "duration_frames": 60,
            "ease_type": "LINEAR",
            "keyframes": [
                {"frame": 0, "x": 0.0, "y": 60.0, "rot": 0.0, "scale": 1.0, "ease_type": "LINEAR"},
                {"frame": 60, "x": 0.0, "y": 0.0, "rot": 0.0, "scale": 1.0, "ease_type": "LINEAR"}
            ]
        }
        self.current_filepath = None
        self.selected_kf_idx = -1
        self.refresh_inspector()
        self.render_canvas()

    def load_json(self):
        path = filedialog.askopenfilename(initialdir=ANIMS_DIR, filetypes=[("JSON files", "*.anim.json")])
        if not path: return
        with open(path, 'r', encoding='utf-8') as f:
            self.anim_data = json.load(f)
            
        # Convert backward compatibility
        if "keyframes" not in self.anim_data:
            global_ease = self.anim_data.get("ease_type", "LINEAR")
            self.anim_data["keyframes"] = [
                {
                    "frame": 0,
                    "x": float(self.anim_data.get("start_x", 0.0)),
                    "y": float(self.anim_data.get("start_y", 0.0)),
                    "rot": float(self.anim_data.get("start_rot", 0.0)),
                    "scale": float(self.anim_data.get("start_scale", 1.0)),
                    "ease_type": global_ease
                },
                {
                    "frame": int(self.anim_data.get("duration_frames", 60)),
                    "x": float(self.anim_data.get("end_x", 0.0)),
                    "y": float(self.anim_data.get("end_y", 0.0)),
                    "rot": float(self.anim_data.get("end_rot", 0.0)),
                    "scale": float(self.anim_data.get("end_scale", 1.0)),
                    "ease_type": "LINEAR"
                }
            ]
        else:
            for k in self.anim_data["keyframes"]:
                if "ease_type" not in k:
                    k["ease_type"] = "LINEAR"
            
        self.current_filepath = path
        self.selected_kf_idx = -1
        self.refresh_inspector()
        self.render_canvas()

    def save_json(self):
        if self.current_filepath:
            path = self.current_filepath
        else:
            path = os.path.join(ANIMS_DIR, f"{self.anim_data['id']}.anim.json")
            self.current_filepath = path
            
        with open(path, 'w', encoding='utf-8') as f:
            json.dump(self.anim_data, f, indent=2)
        messagebox.showinfo("Saved", f"Saved to {os.path.basename(path)}")

if __name__ == "__main__":
    app = AnimEditor()
    app.mainloop()
