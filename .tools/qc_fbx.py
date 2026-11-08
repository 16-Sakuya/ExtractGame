import os
from PIL import Image, ImageDraw
base = r"D:\UEC++\ExtractGame\优酸乳"
prev = os.path.join(base, "preview")
ver = os.path.join(base, "_verify")
names = ["front","back","left","right"]
labels = ["前 / front","后 / back","左 / left","右 / right"]
H = 300
tiles=[]
for n in names:
    a = Image.open(os.path.join(prev, f"ortho_{n}_v002.png")).convert("RGB")
    b = Image.open(os.path.join(ver, f"fbx_{n}.png")).convert("RGB")
    tiles.append((a.resize((int(a.width*H/a.height),H), Image.LANCZOS),
                  b.resize((int(b.width*H/b.height),H), Image.LANCZOS)))
pad=10
W = sum(t[0].width+t[1].width+pad*3 for t in tiles)+pad
sheet = Image.new("RGB",(W, H+40),(250,250,250))
d = ImageDraw.Draw(sheet)
d.text((pad,4),"left of pair = .blend render   |   right of pair = FBX re-imported render",fill=(0,0,0))
x=pad
for i,(a,b) in enumerate(tiles):
    sheet.paste(a,(x,24)); x+=a.width+pad
    sheet.paste(b,(x,24)); x+=b.width+pad*2
    d.text((x-b.width-pad+2, H+26), labels[i], fill=(0,0,0))
out = os.path.join(ver,"fbx_roundtrip_qc.png")
sheet.save(out); print(out, sheet.size)
