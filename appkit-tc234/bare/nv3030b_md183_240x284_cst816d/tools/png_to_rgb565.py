#!/usr/bin/env python3
"""Convert a PNG into a C RGB565 array for the NV3030B LCD projects.

Pure standard library (zlib only) - no Pillow/numpy needed, so it runs in a
plain Python install.

Usage:
    python png_to_rgb565.py <input.png> <output.c> [symbol] [WxH]

Behaviour:
  * PNG is decoded (8-bit truecolour, colour type 6 = RGBA or 2 = RGB; also
    handles the low-bit-depth greyscale/palette cases the toolchain assets use).
  * Alpha is composited over BLACK, matching the panel background, so
    transparent or rounded-corner pixels come out black rather than garbage.
  * Each pixel becomes a standard RGB565 word (R in bits 15..11, G 10..5,
    B 4..0) - the same packing LCD_CopyBuffer expects (it sends the high byte
    first).
  * Output is a .c file defining `const uint16_t <symbol>[]` plus the width
    and height macros in the matching comment header.

The generated file is checked in, but this script is kept so the asset can be
regenerated and its provenance is auditable.
"""

import struct
import sys
import zlib


def load_png(path):
    """Decode a PNG to (w, h, pixels, bpp, colortype). pixels is raw bytes."""
    data = open(path, 'rb').read()
    if data[:8] != b'\x89PNG\r\n\x1a\n':
        raise ValueError('not a PNG: %s' % path)

    pos = 8
    idat = b''
    w = h = bitdepth = colortype = 0
    palette = b''

    while pos < len(data):
        (length,) = struct.unpack('>I', data[pos:pos + 4])
        ctype = data[pos + 4:pos + 8]
        chunk = data[pos + 8:pos + 8 + length]

        if ctype == b'IHDR':
            w, h, bitdepth, colortype = struct.unpack('>IIBB', chunk[:10])
            if bitdepth != 8:
                raise ValueError('only 8-bit PNGs supported (got %d)' % bitdepth)
        elif ctype == b'PLTE':
            palette = chunk
        elif ctype == b'IDAT':
            idat += chunk
        elif ctype == b'IEND':
            break

        pos += 12 + length

    raw = zlib.decompress(idat)
    nch = {0: 1, 2: 3, 3: 1, 4: 2, 6: 4}[colortype]
    stride = w * nch

    out = bytearray(h * stride)
    prev = bytearray(stride)
    p = 0

    for y in range(h):
        filt = raw[p]
        p += 1
        line = bytearray(raw[p:p + stride])
        p += stride

        if filt == 1:                       # Sub
            for i in range(nch, stride):
                line[i] = (line[i] + line[i - nch]) & 0xFF
        elif filt == 2:                     # Up
            for i in range(stride):
                line[i] = (line[i] + prev[i]) & 0xFF
        elif filt == 3:                     # Average
            for i in range(stride):
                a = line[i - nch] if i >= nch else 0
                line[i] = (line[i] + ((a + prev[i]) >> 1)) & 0xFF
        elif filt == 4:                     # Paeth
            for i in range(stride):
                a = line[i - nch] if i >= nch else 0
                b = prev[i]
                c = prev[i - nch] if i >= nch else 0
                pa, pb, pc = abs(b - c), abs(a - c), abs(a + b - 2 * c)
                pr = a if (pa <= pb and pa <= pc) else (b if pb <= pc else c)
                line[i] = (line[i] + pr) & 0xFF

        out[y * stride:(y + 1) * stride] = line
        prev = line

    return w, h, out, nch, colortype, palette


def to_rgb565(w, h, px, nch, colortype, palette):
    """Return a list of RGB565 words, alpha composited over black."""
    words = []

    for i in range(w * h):
        o = i * nch

        if colortype == 6:
            r, g, b, a = px[o], px[o + 1], px[o + 2], px[o + 3]
        elif colortype == 2:
            r, g, b = px[o], px[o + 1], px[o + 2]
            a = 255
        elif colortype == 3:
            idx = px[o] * 3
            r, g, b = palette[idx], palette[idx + 1], palette[idx + 2]
            a = 255
        elif colortype == 4:
            r = g = b = px[o]
            a = px[o + 1]
        else:                               # colortype 0
            r = g = b = px[o]
            a = 255

        if a != 255:                        # composite over black
            r = (r * a) // 255
            g = (g * a) // 255
            b = (b * a) // 255

        words.append(((r >> 3) << 11) | ((g >> 2) << 5) | (b >> 3))

    return words


def emit_c(base_path, symbol, source_name, w, h, words):
    """Write <base>.c (the array) and <base>.h (declaration + dimensions)."""
    per_line = 8
    lines = []

    for i in range(0, len(words), per_line):
        lines.append('    ' + ' '.join('0x%04X,' % v for v in words[i:i + per_line]))

    guard = symbol.upper()
    base_file = base_path.split('/')[-1].split('\\')[-1]

    with open(base_path + '.c', 'w', newline='\n') as f:
        f.write('/*\n')
        f.write('  %s.c - generated RGB565 asset, DO NOT EDIT BY HAND.\n' % base_file)
        f.write('\n')
        f.write('  Source : %s (%dx%d)\n' % (source_name, w, h))
        f.write('  Format : RGB565, row-major, alpha composited over black\n')
        f.write('  Tool   : tools/png_to_rgb565.py (regenerate rather than editing)\n')
        f.write('*/\n\n')
        f.write('#include "%s.h"\n\n' % base_file)
        f.write('const uint16_t %s[%d] = {\n' % (symbol, len(words)))
        f.write('\n'.join(lines))
        f.write('\n};\n')

    # The include guard must NOT collide with the dimension macros. Using
    # <SYMBOL>_H for the height while also guarding on <SYMBOL>_H would make
    # the guard define the height macro (to nothing) and then redefine it -
    # the height would silently be broken. Guard on <SYMBOL>_H_INCLUDED.
    with open(base_path + '.h', 'w', newline='\n') as f:
        f.write('/*\n')
        f.write('  %s.h - generated RGB565 asset, DO NOT EDIT BY HAND.\n' % base_file)
        f.write('\n')
        f.write('  Source : %s (%dx%d)\n' % (source_name, w, h))
        f.write('  Format : RGB565, row-major, alpha composited over black\n')
        f.write('  Tool   : tools/png_to_rgb565.py (regenerate rather than editing)\n')
        f.write('*/\n\n')
        f.write('#ifndef %s_H_INCLUDED\n#define %s_H_INCLUDED\n\n' % (guard, guard))
        f.write('#include <stdint.h>\n\n')
        f.write('#define %s_W %d\n' % (guard, w))
        f.write('#define %s_H %d\n\n' % (guard, h))
        f.write('extern const uint16_t %s[%d];\n\n' % (symbol, len(words)))
        f.write('#endif /* %s_H_INCLUDED */\n' % guard)


def main():
    if len(sys.argv) < 3:
        print(__doc__)
        return 1

    src = sys.argv[1]
    dst = sys.argv[2]
    symbol = sys.argv[3] if len(sys.argv) > 3 else 'asset_image'

    w, h, px, nch, colortype, palette = load_png(src)
    words = to_rgb565(w, h, px, nch, colortype, palette)
    emit_c(dst, symbol, src.split('/')[-1].split('\\')[-1], w, h, words)

    print('wrote %s: %dx%d, %d px, %d bytes' % (dst, w, h, len(words), 2 * len(words)))
    return 0


if __name__ == '__main__':
    sys.exit(main())
