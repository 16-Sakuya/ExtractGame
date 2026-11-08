"""
优酸乳 (Yili YouSuanNai) 250ml 纸盒 —— 由 6 张面板实拍贴图构成的长方体。
尺寸: 宽 64 mm (X) × 深 40 mm (Y) × 高 106 mm (Z), 原点位于底面中心。
每个面一个材质槽 / 一张贴图, UV 严格满铺 (0..1)。
"""
import bpy
import os

TEX_DIR = r"D:\UEC++\ExtractGame\优酸乳\textures"

# ---------------------------------------------------------------- clean scene
bpy.ops.wm.read_factory_settings(use_empty=True)
scene = bpy.context.scene
scene.unit_settings.system = 'METRIC'
scene.unit_settings.scale_length = 1.0
scene.unit_settings.length_unit = 'MILLIMETERS'

col = bpy.data.collections.new("COL_YouSuanNai")
scene.collection.children.link(col)

# ---------------------------------------------------------------- geometry
W, D, H = 0.064, 0.040, 0.106          # metres

verts = [
    (-W / 2, -D / 2, 0.0),   # 0 底 前 左
    (+W / 2, -D / 2, 0.0),   # 1 底 前 右
    (+W / 2, +D / 2, 0.0),   # 2 底 后 右
    (-W / 2, +D / 2, 0.0),   # 3 底 后 左
    (-W / 2, -D / 2, H),     # 4 顶 前 左
    (+W / 2, -D / 2, H),     # 5 顶 前 右
    (+W / 2, +D / 2, H),     # 6 顶 后 右
    (-W / 2, +D / 2, H),     # 7 顶 后 左
]

# (face label, vertex loop CCW from outside, texture file)
faces = [
    ("Front",  (0, 1, 5, 4), "T_front.jpg"),    # -Y
    ("Back",   (2, 3, 7, 6), "T_back.jpg"),     # +Y
    ("Right",  (1, 2, 6, 5), "T_right.jpg"),    # +X
    ("Left",   (3, 0, 4, 7), "T_left.jpg"),     # -X
    ("Top",    (4, 5, 6, 7), "T_top.jpg"),      # +Z
    ("Bottom", (1, 0, 3, 2), "T_bottom.jpg"),   # -Z
]

me = bpy.data.meshes.new("GEO_YouSuanNai_Box")
me.from_pydata(verts, [], [f[1] for f in faces])
me.update()
obj = bpy.data.objects.new("GEO_YouSuanNai_Box", me)
col.objects.link(obj)
bpy.context.view_layer.objects.active = obj
obj.select_set(True)

# ---------------------------------------------------------------- UVs (full 0..1 per face, upright)
uv_layer = me.uv_layers.new(name="UVMap")
CORNERS = [(0.0, 0.0), (1.0, 0.0), (1.0, 1.0), (0.0, 1.0)]
for poly, (_label, _loop, _tex) in zip(me.polygons, faces):
    for i, li in enumerate(poly.loop_indices):
        uv_layer.data[li].uv = CORNERS[i]

# ---------------------------------------------------------------- materials
def make_material(label, tex_file):
    img = bpy.data.images.load(os.path.join(TEX_DIR, tex_file), check_existing=True)
    img.pack()
    mat = bpy.data.materials.new(name=f"MAT_{label}")
    mat.use_nodes = True
    nt = mat.node_tree
    nt.nodes.clear()
    out = nt.nodes.new("ShaderNodeOutputMaterial")
    out.location = (420, 0)
    bsdf = nt.nodes.new("ShaderNodeBsdfPrincipled")
    bsdf.location = (120, 0)
    bsdf.inputs["Roughness"].default_value = 0.38
    if "Metallic" in bsdf.inputs:
        bsdf.inputs["Metallic"].default_value = 0.0
    tex = nt.nodes.new("ShaderNodeTexImage")
    tex.location = (-200, 0)
    tex.image = img
    tex.interpolation = 'Linear'
    tex.extension = 'EXTEND'
    nt.links.new(tex.outputs["Color"], bsdf.inputs["Base Color"])
    nt.links.new(bsdf.outputs["BSDF"], out.inputs["Surface"])
    mat["panel_texture"] = tex_file
    return mat

for idx, (label, _loop, tex_file) in enumerate(faces):
    mat = make_material(label, tex_file)
    me.materials.append(mat)
    me.polygons[idx].material_index = idx

# flat shading, no auto-smooth
for p in me.polygons:
    p.use_smooth = False
me.update()

# ---------------------------------------------------------------- camera + light (preview only)
cam_data = bpy.data.cameras.new("CAM_Box")
cam_data.lens = 50
cam = bpy.data.objects.new("CAM_Box", cam_data)
cam.location = (0.20, -0.30, 0.20)
col.objects.link(cam)
scene.camera = cam

target = bpy.data.objects.new("GEO_AimTarget", None)
target.location = (0.0, 0.0, H * 0.5)
col.objects.link(target)
tc = cam.constraints.new(type='TRACK_TO')
tc.target = target
tc.track_axis = 'TRACK_NEGATIVE_Z'
tc.up_axis = 'UP_Y'

def add_area(name, loc, energy, size, rot=None):
    ld = bpy.data.lights.new(name, type='AREA')
    ld.energy = energy
    ld.size = size
    lo = bpy.data.objects.new(name, ld)
    lo.location = loc
    col.objects.link(lo)
    c = lo.constraints.new(type='TRACK_TO')
    c.target = target
    c.track_axis = 'TRACK_NEGATIVE_Z'
    c.up_axis = 'UP_Y'
    return lo

add_area("LGT_Key",  (0.28, -0.34, 0.34), 60, 0.35)
add_area("LGT_Fill", (-0.34, -0.24, 0.14), 18, 0.45)
add_area("LGT_Rim",  (0.00, 0.42, 0.30), 25, 0.40)

world = bpy.data.worlds.new("WLD_Studio")
world.use_nodes = True
world.node_tree.nodes["Background"].inputs[0].default_value = (0.92, 0.92, 0.92, 1.0)
world.node_tree.nodes["Background"].inputs[1].default_value = 0.6
scene.world = world

for engine in ('BLENDER_EEVEE_NEXT', 'BLENDER_EEVEE', 'CYCLES'):
    try:
        scene.render.engine = engine
        break
    except Exception:
        continue

# ---------------------------------------------------------------- metadata
obj["source_panels"] = "前/后/左/右/上/下.jpg"
obj["carton_size_mm"] = "64 x 40 x 106"
obj["up_axis"] = "+Z (Blender), exported Y-up for Unreal"

dsh_result = {
    "object": obj.name,
    "dimensions_m": [round(v, 4) for v in obj.dimensions],
    "dimensions_mm": [round(v * 1000, 2) for v in obj.dimensions],
    "vertices": len(me.vertices),
    "polygons": len(me.polygons),
    "uv_layers": [l.name for l in me.uv_layers],
    "materials": [m.name for m in me.materials],
    "textures": [m.get("panel_texture") for m in me.materials],
    "packed_images": [i.name for i in bpy.data.images if i.packed_file],
    "origin": "bottom-centre, +Z up",
}
print(dsh_result)
