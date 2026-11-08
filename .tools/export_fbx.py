import bpy, os, glob

OUT_DIR = r"D:\UEC++\ExtractGame\优酸乳"
FBX = os.path.join(OUT_DIR, "YouSuanNai_Carton_64x40x106.fbx")

obj = bpy.data.objects["GEO_YouSuanNai_Box"]
for o in bpy.context.view_layer.objects:
    o.select_set(False)
obj.select_set(True)
bpy.context.view_layer.objects.active = obj
obj["dimensions_mm"] = "64 (X) x 40 (Y) x 106 (Z)"
obj["panels"] = "front/back=6.4x10.6cm, left/right=4.0x10.6cm, top/bottom=6.4x4.0cm"

# pick the FBX exporter that exists in this Blender build
if "fbx_export" in dir(bpy.ops.wm):
    op = bpy.ops.wm.fbx_export
    sel_key = "export_selected_objects"
else:
    op = bpy.ops.export_scene.fbx
    sel_key = "use_selection"

kwargs = {
    "filepath": FBX,
    sel_key: True,
    "object_types": {'MESH'},
    "apply_scale_options": 'FBX_SCALE_NONE',
    "apply_unit_scale": True,
    "global_scale": 1.0,
    "axis_forward": '-Z',
    "axis_up": 'Y',
    "path_mode": 'COPY',
    "embed_textures": True,
    "use_mesh_modifiers": True,
    "mesh_smooth_type": 'OFF',
    "add_leaf_bones": False,
    "use_custom_props": True,
    "bake_anim": False,
}
try:
    valid = set(op.get_rna_type().properties.keys())
    dropped = [k for k in kwargs if k not in valid]
    kwargs = {k: v for k, v in kwargs.items() if k in valid}
except Exception as exc:      # pragma: no cover
    dropped = [f"<could not introspect: {exc}>"]

op(**kwargs)

fbm = sorted(glob.glob(os.path.join(OUT_DIR, "*.fbm", "*")))
dsh_result = {
    "exporter": getattr(op, "idname", str(op)),
    "filepath": FBX,
    "bytes": os.path.getsize(FBX) if os.path.exists(FBX) else 0,
    "dropped_kwargs": dropped,
    "fbm_folder_files": [os.path.basename(p) for p in fbm],
    "selected_only": [o.name for o in bpy.context.selected_objects],
}
print(dsh_result)
