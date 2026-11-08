import cv2, numpy as np, itertools, os

layout_path = r"C:\Users\hp\.dsh\attachments\v1\objects\ab\abf7fc268b056bd70332c322cbd5956aeb0172cd3169cbe2008ea4dcaad50824"
src = r"D:\UEC++\BldModels\优酸乳\贴图_去文字"

lay = cv2.imread(layout_path)
print("layout", lay.shape)
hsv = cv2.cvtColor(lay, cv2.COLOR_BGR2HSV)
# non-white content
mask = ((hsv[:, :, 1] > 40) | (hsv[:, :, 2] < 235)).astype(np.uint8) * 255
mask = cv2.morphologyEx(mask, cv2.MORPH_CLOSE, np.ones((15, 15), np.uint8))
n, labels, stats, cent = cv2.connectedComponentsWithStats(mask, 8)
blobs = []
for i in range(1, n):
    x, y, w, h, a = stats[i]
    if a > 20000 and w > 60 and h > 60:
        blobs.append((x, y, w, h, a))
print("blobs:", len(blobs), blobs)

def load(p):
    return cv2.imread(p)

def prep(img, size):
    return cv2.resize(img, size, interpolation=cv2.INTER_AREA)

def ncc(a, b):
    a = cv2.cvtColor(a, cv2.COLOR_BGR2GRAY).astype(np.float32)
    b = cv2.cvtColor(b, cv2.COLOR_BGR2GRAY).astype(np.float32)
    a = (a - a.mean()) / (a.std() + 1e-6)
    b = (b - b.mean()) / (b.std() + 1e-6)
    return float((a * b).mean())

names = ["前", "后", "左", "右", "上", "下"]
rots = [None, cv2.ROTATE_90_CLOCKWISE, cv2.ROTATE_180, cv2.ROTATE_90_COUNTERCLOCKWISE]
rotlabel = ["none", "cw90", "180", "ccw90"]

scores = {}
for nm in names:
    im = load(os.path.join(src, nm + ".jpg"))
    for bi, (x, y, w, h, a) in enumerate(blobs):
        crop = lay[y:y+h, x:x+w]
        # aspect filter to prune
        for ri, r in enumerate(rots):
            v = im if r is None else cv2.rotate(im, r)
            sc = ncc(prep(v, (64, 96)), prep(crop, (64, 96)))
            scores[(nm, bi, ri)] = sc

# greedy best assignment jpg->blob with rotation
best = []
used_b = set()
for nm in names:
    cand = sorted(((scores[(nm, b, r)], b, r) for b in range(len(blobs)) for r in range(4)), reverse=True)
    for s, b, r in cand:
        if b not in used_b:
            used_b.add(b)
            best.append((nm, b, r, s, blobs[b]))
            break

for nm, b, r, s, bb in best:
    im = load(os.path.join(src, nm + ".jpg"))
    print(f"{nm}.jpg -> blob{b} {bb[:2]} {bb[2]}x{bb[3]}  rot={rotlabel[r]:5s} ncc={s:.3f}")
