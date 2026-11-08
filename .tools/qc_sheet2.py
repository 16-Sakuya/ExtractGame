import os
from PIL import Image, ImageDraw

base = r"D:\UEC++\ExtractGame\优酸乳"
prev = os.path.join(base, "preview")
tex = os.path.join(base, "textures")

pairs = [("前 / front", "T_front.jpg", "ortho_front_v002.png"),
         ("后 / back", "T_back.jpg", "ortho_back_v002.png"),
         ("左 / left", "T_left.jpg", "ortho_left_v002.png"),
         ("右 / right", "T_right.jpg", "ortho_right_v002.png"),
         ("上 / top", "T_top.jpg", "ortho_top_v002.png"),
         ("下 / bottom", "T_bottom.jpg", "ortho_bottom_v002.png")]

H = 300
pad = 10
cellw = []
tiles = []
for label, t, r in pairs:
    a = Image.open(os.path.join(tex, t)).convert("RGB")
    b = Image.open(os.path.join(prev, r)).convert("RGB")
    aw = int(a.width * H / a.height)
    bw = int(b.width * H / b.height)
    tiles.append((label, a.resize((aw, H), Image.LANCZOS), b.resize((bw, H), Image.LANCZOS)))
    cellw.append(max(aw, bw))

head = 22
W = sum(cellw) + pad * (len(tiles) + 1)
sheet = Image.new("RGB", (W, H * 2 + head * 2 + pad * 3), (248, 248, 248))
d = ImageDraw.Draw(sheet)
d.text((pad, 4), "TEXTURE (source, upright)", fill=(0, 0, 0))
d.text((pad, H + head + pad + 4), "RENDER (orthographic)", fill=(0, 0, 0))
x = pad
for i, (label, a, b) in enumerate(tiles):
    sheet.paste(a, (x, head))
    sheet.paste(b, (x, H + head * 2 + pad))
    d.text((x + 4, head - 16), label, fill=(20, 20, 20))
    x += cellw[i] + pad
out = os.path.join(prev, "qc_v002.png")
sheet.save(out)
print(out, sheet.size)
