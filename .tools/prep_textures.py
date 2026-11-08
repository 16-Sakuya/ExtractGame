import os
from PIL import Image

src = r"D:\UEC++\BldModels\优酸乳\贴图_去文字"
out = r"D:\UEC++\ExtractGame\优酸乳\textures"
os.makedirs(out, exist_ok=True)

# name -> rotation to apply for physically upright orientation
#  (determined by matching each panel to the 视图.png layout: 前/后/左/右 upright,
#   上 rotated 90 CCW, 下 rotated 90 CW)
plan = [
    ("前", "T_front.jpg", 0),
    ("后", "T_back.jpg", 0),
    ("左", "T_left.jpg", 0),
    ("右", "T_right.jpg", 0),
    ("上", "T_top.jpg", 90),
    ("下", "T_bottom.jpg", -90),
]

report = []
for name, outname, deg in plan:
    p = os.path.join(src, name + ".jpg")
    im = Image.open(p).convert("RGB")
    w0, h0 = im.size
    if deg:
        im = im.rotate(deg, expand=True, resample=Image.BICUBIC)
    im.save(os.path.join(out, outname), quality=95, subsampling=0)
    report.append((name, outname, (w0, h0), im.size, deg))

for r in report:
    print(f"{r[0]}.jpg -> {r[1]}  {r[2]} --rot{r[4]}--> {r[3]}  aspect={r[3][0]/r[3][1]:.3f}")
