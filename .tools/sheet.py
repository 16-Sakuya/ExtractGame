import os
from PIL import Image, ImageDraw
src = r"D:\UEC++\BldModels\优酸乳\贴图_去文字"
names = ["前","后","左","右","上","下"]
tiles = []
H = 420
for n in names:
    im = Image.open(os.path.join(src, n + ".jpg")).convert("RGB")
    w = int(im.width * H / im.height)
    tiles.append((n, im.resize((w, H), Image.LANCZOS)))
pad = 10
label = 24
total_w = sum(t.width for _, t in tiles) + pad * (len(tiles) + 1)
sheet = Image.new("RGB", (total_w, H + label + pad * 2), (255, 255, 255))
d = ImageDraw.Draw(sheet)
x = pad
for n, t in tiles:
    sheet.paste(t, (x, pad))
    d.text((x + 4, pad + H + 4), f"{n}.jpg  {t.width}x{t.height}", fill=(0, 0, 0))
    x += t.width + pad
out = r"D:\UEC++\ExtractGame\.tools\panels_sheet.png"
sheet.save(out)
print(out, sheet.size)
