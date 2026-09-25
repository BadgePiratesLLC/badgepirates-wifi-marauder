#!/usr/bin/env python3
# Regenerates src/UI/img_bp_skull.c from the Badge Pirates brand mark
# (Nexus 176cc276 splash screen). Run whenever badgepiratesllc.github.io's
# images/BP_New.png changes - the .c file is committed output, not built
# at compile time (lv_conf.h has no PNG decoder: LV_USE_LODEPNG=0,
# LV_USE_LIBPNG=0, so this conversion has to happen offline).
#
# Usage: python3 scripts/gen_splash_skull.py src/UI/img_bp_skull.c
#   (assumes badgepiratesllc.github.io is checked out as a sibling repo,
#   same layout as this agent-repo workspace; override SRC below if not)
import sys
from PIL import Image

SRC = "../badgepiratesllc.github.io/images/BP_New.png"
SIZE = 132  # ~120-140px square per Nexus 176cc276

img = Image.open(SRC).convert("RGBA")
img = img.resize((SIZE, SIZE), Image.LANCZOS)

w, h = img.size
px = img.load()

rgb565_bytes = bytearray()
alpha_bytes = bytearray()

for y in range(h):
    for x in range(w):
        r, g, b, a = px[x, y]
        # The source mark is a solid-black silhouette on transparent (built
        # for print/light backgrounds) - alpha already carries the
        # distressed texture as holes (rendered this splash in the sim
        # first with the raw black fill: on THEME_BG's near-black panel it
        # was correctly drawn and completely invisible, Nexus 176cc276).
        # Inverting to white keeps every edge/hole exactly where the alpha
        # map puts it, just recolours the fill to read against a dark
        # screen instead of a light one.
        r, g, b = 255 - r, 255 - g, 255 - b
        v = ((r >> 3) << 11) | ((g >> 2) << 5) | (b >> 3)
        # little-endian 16-bit, matches LVGL's native RGB565 byte order on this target
        rgb565_bytes.append(v & 0xFF)
        rgb565_bytes.append((v >> 8) & 0xFF)
        alpha_bytes.append(a)

data = bytes(rgb565_bytes) + bytes(alpha_bytes)

def emit_c(path, sym):
    with open(path, "w") as f:
        # sim/build.sh compiles this through clang++ (C++ mode) even though
        # it's a .c file - in C++, a namespace-scope `const` has INTERNAL
        # linkage unless a prior declaration already marked it extern. This
        # header's LV_IMAGE_DECLARE() is that prior declaration - drop it
        # and the sim link fails with "symbol not found" despite the file
        # compiling cleanly (caught building this ticket, Nexus 176cc276).
        f.write('#include "lvgl.h"\n')
        f.write('#include "UI/img_bp_skull.h"\n\n')
        f.write("// Badge Pirates skull mark, Nexus 176cc276 splash screen.\n")
        f.write("// Generated from badgepiratesllc.github.io/images/BP_New.png (270x270 RGBA)\n")
        f.write(f"// via scripts/gen_splash_skull.py: LANCZOS downscale to {w}x{h}, RGB565A8 (colour + separate\n")
        f.write("// 8-bit alpha plane) so LVGL composites the distressed edge against\n")
        f.write("// whatever's under it instead of keying onto a hard background.\n")
        f.write("// Do not hand-edit - regenerate from the source PNG if the mark changes.\n\n")
        f.write(f"const uint8_t {sym}_map[] = {{\n")
        for i in range(0, len(data), 16):
            chunk = data[i:i+16]
            f.write("    " + ", ".join(f"0x{b:02x}" for b in chunk) + ",\n")
        f.write("};\n\n")
        f.write(f"const lv_image_dsc_t {sym} = {{\n")
        f.write("    .header = {\n")
        # LV_IMAGE_HEADER_MAGIC (0x19) - without it lv_image_src_get_type()
        # can't identify this as a valid C-array image descriptor and
        # lv_image_set_src() silently draws nothing (caught building this
        # ticket, Nexus 176cc276: the skull rendered as blank black).
        f.write("        .magic = LV_IMAGE_HEADER_MAGIC,\n")
        f.write("        .cf = LV_COLOR_FORMAT_RGB565A8,\n")
        f.write(f"        .w = {w},\n")
        f.write(f"        .h = {h},\n")
        f.write(f"        .stride = {w*2},\n")
        f.write("    },\n")
        f.write(f"    .data_size = sizeof({sym}_map),\n")
        f.write(f"    .data = {sym}_map,\n")
        f.write("};\n")

emit_c(sys.argv[1], "img_bp_skull")
print(f"wrote {sys.argv[1]}: {w}x{h}, {len(data)} bytes data")
