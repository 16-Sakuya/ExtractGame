import bpy, math, os

OUT = r"D:\UEC++\ExtractGame\优酸乳\_verify"
scene = bpy.context.scene

# strip the default startup objects so only the re-imported FBX remains
for name in ("Cube", "Camera", "Light"):
    ob = bpy.data.objects.get(name)
    if ob:
        bpy.data.objects.remove(ob, do_unlink=True)

obj = bpy.data.objects["GEO_YouSuanNai_Box"]
H = obj.dimensions.z

cam_d = bpy.data.cameras.new("CAM_V")
cam_d.type = 'ORTHO'
cam_d.ortho_scale = 0.125
cam = bpy.data.objects.new("CAM_V", cam_d)
scene.collection.objects.link(cam)
scene.camera = cam

sun_d = bpy.data.lights.new("LGT_Sun", type='SUN')
sun_d.energy = 3.0
sun = bpy.data.objects.new("LGT_Sun", sun_d)
sun.rotation_euler = (math.radians(55), 0, math.radians(35))
scene.collection.objects.link(sun)

world = scene.world or bpy.data.worlds.new("WLD")
scene.world = world
world.use_nodes = True
bg = next((n for n in world.node_tree.nodes if n.type == 'BACKGROUND'), None)
if bg:
    bg.inputs[0].default_value = (1, 1, 1, 1)
    bg.inputs[1].default_value = 0.55
scene.view_settings.view_transform = 'Standard'

views = {
    "front": ((0.0, -1.0, H / 2), (math.radians(90), 0, 0)),
    "back": ((0.0, 1.0, H / 2), (math.radians(90), 0, math.radians(180))),
    "left": ((-1.0, 0.0, H / 2), (math.radians(90), 0, math.radians(-90))),
    "right": ((1.0, 0.0, H / 2), (math.radians(90), 0, math.radians(90))),
}
scene.render.resolution_x = 300
scene.render.resolution_y = 470
scene.render.image_settings.file_format = 'PNG'
for name, (loc, rot) in views.items():
    cam.location = loc
    cam.rotation_euler = rot
    scene.render.filepath = os.path.join(OUT, f"fbx_{name}.png")
    bpy.ops.render.render(write_still=True)

print("done", list(views))
