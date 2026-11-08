import bpy, math, os

OUT = r"D:\UEC++\ExtractGame\优酸乳\preview"
os.makedirs(OUT, exist_ok=True)
scene = bpy.context.scene

# ---- colour management: neutral, texture-accurate
scene.view_settings.view_transform = 'Standard'
scene.view_settings.look = 'None'
scene.view_settings.exposure = 0.0
scene.view_settings.gamma = 1.0

# ---- lighting calibrated for a 10 cm object with Standard view transform
def light(name, energy):
    ob = bpy.data.objects.get(name)
    if ob and ob.type == 'LIGHT':
        ob.data.energy = energy
light("LGT_Key", 4.0)
light("LGT_Fill", 1.5)
light("LGT_Rim", 1.5)
if scene.world:
    bg = scene.world.node_tree.nodes.get("Background")
    if bg:
        bg.inputs[1].default_value = 0.20

# ---- hero camera a little closer
cam = bpy.data.objects.get("CAM_Box")
if cam:
    cam.location = (0.150, -0.225, 0.125)

# ---- orthographic QC camera
od = bpy.data.cameras.new("CAM_Ortho")
od.type = 'ORTHO'
ortho = bpy.data.objects.new("CAM_Ortho", od)
bpy.data.collections["COL_YouSuanNai"].objects.link(ortho)

H = 0.106
views = {
    "front":  ((0.0, -1.0, H / 2), (math.radians(90), 0, 0), 0.125, 520, 800),
    "back":   ((0.0, 1.0, H / 2), (math.radians(90), 0, math.radians(180)), 0.125, 520, 800),
    "left":   ((-1.0, 0.0, H / 2), (math.radians(90), 0, math.radians(-90)), 0.125, 520, 800),
    "right":  ((1.0, 0.0, H / 2), (math.radians(90), 0, math.radians(90)), 0.125, 520, 800),
    "top":    ((0.0, 0.0, 1.0), (0, 0, 0), 0.076, 800, 520),
    "bottom": ((0.0, 0.0, -1.0), (math.radians(180), 0, 0), 0.076, 800, 520),
}

scene.camera = ortho
for name, (loc, rot, oscale, w, h) in views.items():
    ortho.location = loc
    ortho.rotation_euler = rot
    od.ortho_scale = oscale
    scene.render.resolution_x = w
    scene.render.resolution_y = h
    scene.render.resolution_percentage = 100
    scene.render.image_settings.file_format = 'PNG'
    scene.render.filepath = os.path.join(OUT, f"ortho_{name}_v002.png")
    bpy.ops.render.render(write_still=True)

print("rendered", list(views))
