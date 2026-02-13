"""临时脚本：给 icon.png 添加边距（缩小内容，四周留白）"""
from PIL import Image

SRC = "Qt/resource/img/icon.png"
OUT = "Qt/resource/img/icon.png"  # 直接覆盖原文件

# 边距占比（每边 12%），可调整
PADDING_RATIO = 0.12

img = Image.open(SRC).convert("RGBA")
w, h = img.size

pad = int(min(w, h) * PADDING_RATIO)
inner_w = w - 2 * pad
inner_h = h - 2 * pad

# 缩小原图内容
shrunk = img.resize((inner_w, inner_h), Image.LANCZOS)

# 创建同尺寸透明画布，将缩小后的内容居中放置
canvas = Image.new("RGBA", (w, h), (0, 0, 0, 0))
canvas.paste(shrunk, (pad, pad))

canvas.save(OUT)
print(f"Done: {w}x{h}, padding={pad}px ({PADDING_RATIO:.0%} each side)")
print(f"Saved to {OUT}")
