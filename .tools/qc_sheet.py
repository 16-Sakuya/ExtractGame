import os
from PIL import Image, ImageDraw

base = r"D:\UEC++\ExtractGame\优酸乳"
tex = os.path.join(base, "textures")
prev = os.path.join(base, "preview")
src = r"D:\UEC++\BldModels\优酸乳\贴图_去文字"

rows = [
    ("source", [os.path.join(src, n + ".jpg") for n in ("前", "后", "左", "右")]),
    ("render", [os.path.join(prev, f"view_{v}_v001.png") for v in ("front", "back", "left", "right")]),
]
H = 380
pad = 12
labels = ["前 / front", "后 / back", "左 / left", "右 / right"]
tiles = []
for tag, paths in rows:
    row = []
    for p in paths:
        im = Image.open(p).convert("RGB")
        w = max(1, int(im.width * H / im.height))
        row.append(im.resize((w, H), Image.LANCZOS))
    tiles.append(row)

width = max(sum(t.width for t in row) + pad * (len(row) + 1) for row in tiles)
head = 26
sheet = Image.new("RGB", (width, (H + pad) * 2 + head * 2 + pad), (250, 250, 250))
d = ImageDraw.Draw(sheet)
for ri, row in enumerate(tiles):
    y = pad + ri * (H + head + pad)
    d.text((pad, y - 16), ("SOURCE PANELS" if ri == 0 else "BLENDER RENDER (orthographic)"), fill=(0, 0, 0))
    x = pad
    for ci, t in enumerate(row):
        sheet.paste(t, (x, y))
        d.text((x + 4, y + H + 2), labels[ci], fill=(30, 30, 30))
        x += t.width + pad
out = os.path.join(prev, "qc_panels_v001.png")
sheet.save(out)
print(out, sheet.size)
