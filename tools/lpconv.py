#!/usr/bin/env python3
"""Convert LisaPaint pictures to and from MacPaint and PNG.

    lpconv.py IN OUT                       convert one picture
    lpconv.py export DISK.dc42 [DIR] [--format png|mac|lpnt] [--aspect]
                                           every LisaPaint document on a
                                           floppy image, named after it

The format follows the extension: .lpnt (LisaPaint's own document file, as
<prefix>PIC on the Lisa), .mac / .pntg / .macpaint (MacPaint; a MacBinary
header is skipped), .png.  A LisaPaint page is 720 x 800 pixels; a MacPaint
page, 576 x 720, is its top left corner: a MacPaint picture comes in there,
pixel for pixel, with white to the right and below, and going out to MacPaint
keeps that corner (with a warning if anything outside it is lost).  A PNG is
turned black and white (dark < 50% gray) and placed at the top left; it may
be at most 720 x 800.

Lisa pixels are 1.5 times taller than wide, so a picture looks taller on the
Lisa than on a Mac.  --aspect writes a PNG at the Lisa screen's proportions
(each pixel 2 wide and 3 tall).

export reads documents of tool 21 (LisaPaint) with tools/lisadisk.py from
lisa-c (LISA_C, default ../lisa-c next to this directory or the submodule):
the saved picture ({D<id>T21}PIC), else the set-aside state (SUS).
"""
import os
import re
import struct
import sys
import zlib

W, H = 720, 800                 # LisaPaint's page
ROW = W // 8
MW, MH = 576, 720               # MacPaint's page: the top left corner
MROW = MW // 8
TOOL = 21
T_PENCIL = 4

# ---- LisaPaint (src/doc.c) -------------------------------------------------
# Header, big-endian: "LPNT", version 2, width, height, rowBytes (720, 800,
# 90), sx, sy,
# zoom, tool, pattern, line, brush, hasSel, sel (top, left, bottom, right),
# 8 spare words; then height x rowBytes bytes, 1 = black, high bit leftmost.
LPNT = struct.Struct('>4s12h4h8h')


def read_lpnt(b):
    f = LPNT.unpack_from(b)
    if f[0] != b'LPNT' or f[1:5] != (2, W, H, ROW):
        raise SystemExit(f'not a LisaPaint picture (version 2, {W} x {H})')
    return bytearray(b[LPNT.size:LPNT.size + ROW * H])


def write_lpnt(bits):
    return LPNT.pack(b'LPNT', 2, W, H, ROW, 0, 0, 1, T_PENCIL, 0, 1, 0, 0,
                     0, 0, 0, 0, *[0] * 8) + bytes(bits)


# ---- MacPaint ------------------------------------------------------------
# 512-byte header (version, 38 patterns, padding), then 720 rows of 72
# bytes (576 pixels), each packed with PackBits.  1 = black.
def unpackbits(b, pos, n):
    out = bytearray()
    while len(out) < n and pos < len(b):
        c = b[pos]
        pos += 1
        if c < 128:
            out += b[pos:pos + c + 1]
            pos += c + 1
        elif c > 128:
            out += bytes([b[pos]]) * (257 - c)
            pos += 1
    return out[:n], pos


def packbits(row):
    out = bytearray()
    i = 0
    while i < len(row):
        j = i
        while j + 1 < len(row) and row[j + 1] == row[i] and j - i < 127:
            j += 1
        if j > i:                                   # a run of 2..128
            out += bytes([257 - (j - i + 1), row[i]])
            i = j + 1
            continue
        j = i                                       # literals up to the next run
        while j + 1 < len(row) and j - i < 127 and not (j + 2 < len(row) and row[j + 1] == row[j + 2]):
            j += 1
        out += bytes([j - i]) + row[i:j + 1]
        i = j + 1
    return out


def read_macpaint(b):
    if len(b) > 128 and b[0] == 0 and b[65:69] == b'PNTG':
        b = b[128:]                                 # MacBinary
    mac, _ = unpackbits(b, 512, MROW * MH)
    mac += bytearray(MROW * MH - len(mac))
    bits = bytearray(ROW * H)
    for y in range(MH):
        bits[y * ROW:y * ROW + MROW] = mac[y * MROW:(y + 1) * MROW]
    return bits


def write_macpaint(bits):
    lost = any(any(bits[y * ROW + MROW:(y + 1) * ROW]) for y in range(MH)) or any(bits[MH * ROW:])
    if lost:
        print(f'warning: the picture goes beyond MacPaint\'s {MW} x {MH}; that part is left out',
              file=sys.stderr)
    out = bytearray(512)
    for y in range(MH):
        out += packbits(bits[y * ROW:y * ROW + MROW])
    return bytes(out)


# ---- PNG -------------------------------------------------------------------
def read_png(b):
    if b[:8] != b'\x89PNG\r\n\x1a\n':
        raise SystemExit('not a PNG file')
    pos, idat, plte = 8, b'', None
    while pos < len(b):
        n, t = struct.unpack_from('>I4s', b, pos)
        c = b[pos + 8:pos + 8 + n]
        if t == b'IHDR':
            w, h, depth, ctype, _, _, interlace = struct.unpack('>IIBBBBB', c)
        elif t == b'PLTE':
            plte = c
        elif t == b'IDAT':
            idat += c
        pos += 12 + n
    if interlace:
        raise SystemExit('interlaced PNGs are not supported')
    if w > W or h > H:
        raise SystemExit(f'PNG is {w} x {h}; at most {W} x {H}')
    chans = {0: 1, 2: 3, 3: 1, 4: 2, 6: 4}[ctype]
    bpp = max(1, chans * depth // 8)
    stride = (w * chans * depth + 7) // 8
    raw = zlib.decompress(idat)
    bits = bytearray(ROW * H)
    prev = bytearray(stride)
    for y in range(h):
        f = raw[y * (stride + 1)]
        line = bytearray(raw[y * (stride + 1) + 1:(y + 1) * (stride + 1)])
        for i in range(stride):
            a = line[i - bpp] if i >= bpp else 0
            up = prev[i]
            c = prev[i - bpp] if i >= bpp else 0
            if f == 1:
                line[i] = (line[i] + a) & 255
            elif f == 2:
                line[i] = (line[i] + up) & 255
            elif f == 3:
                line[i] = (line[i] + (a + up) // 2) & 255
            elif f == 4:
                p = a + up - c
                pa, pb, pc = abs(p - a), abs(p - up), abs(p - c)
                line[i] = (line[i] + (a if pa <= pb and pa <= pc else up if pb <= pc else c)) & 255
        prev = line
        for x in range(w):
            if depth < 8:
                v = line[x * depth // 8] >> (8 - depth - x * depth % 8) & ((1 << depth) - 1)
                gray = v * 255 // ((1 << depth) - 1) if ctype == 0 else None
            else:
                v = line[x * bpp]
            if ctype == 3:
                r, g, bl = plte[v * 3:v * 3 + 3]
                gray = (r * 299 + g * 587 + bl * 114) // 1000
            elif ctype in (2, 6):
                r, g, bl = line[x * bpp], line[x * bpp + depth // 8], line[x * bpp + 2 * depth // 8]
                gray = (r * 299 + g * 587 + bl * 114) // 1000
            elif depth >= 8:
                gray = v
            if gray < 128:
                bits[y * ROW + x // 8] |= 0x80 >> (x % 8)
    return bits


def write_png(bits, aspect=False):
    sx, sy = (2, 3) if aspect else (1, 1)
    raw = bytearray()
    for y in range(H):
        row = bits[y * ROW:(y + 1) * ROW]
        if sx == 2:                                 # double each bit
            wide = bytearray()
            for byte in row:
                v = 0
                for k in range(8):
                    v = v << 2 | (3 if byte & (0x80 >> k) else 0)
                wide += v.to_bytes(2, 'big')
            row = wide
        raw += (b'\0' + bytes(255 - c for c in row)) * sy    # PNG gray: 0 = black

    def chunk(t, d):
        return struct.pack('>I', len(d)) + t + d + struct.pack('>I', zlib.crc32(t + d))
    return (b'\x89PNG\r\n\x1a\n' + chunk(b'IHDR', struct.pack('>IIBBBBB', W * sx, H * sy, 1, 0, 0, 0, 0))
            + chunk(b'IDAT', zlib.compress(bytes(raw), 9)) + chunk(b'IEND', b''))


# ---- formats by extension -------------------------------------------------
def kind(path):
    ext = os.path.splitext(path)[1].lower()
    for k, exts in (('lpnt', ('.lpnt',)), ('mac', ('.mac', '.pntg', '.macpaint')), ('png', ('.png',))):
        if ext in exts:
            return k
    raise SystemExit(f'{path}: unknown format (.lpnt, .mac, .pntg, .macpaint, .png)')


def read_picture(data, k):
    return {'lpnt': read_lpnt, 'mac': read_macpaint, 'png': read_png}[k](data)


def write_picture(bits, k, aspect=False):
    if k == 'png':
        return write_png(bits, aspect)
    return {'lpnt': write_lpnt, 'mac': write_macpaint}[k](bits)


def export(disk, outdir, fmt, aspect):
    here = os.path.dirname(os.path.abspath(__file__))
    lisa_c = os.environ.get('LISA_C') or next(
        (p for p in (os.path.join(here, '..', 'lisa-c'), os.path.join(here, '..', '..', 'lisa-c'))
         if os.path.exists(os.path.join(p, 'tools', 'lisadisk.py'))), None)
    if not lisa_c:
        raise SystemExit('lisa-c not found (set LISA_C)')
    sys.path.insert(0, os.path.join(lisa_c, 'tools'))
    import lisadisk
    d = lisadisk.Disk(disk)
    files = d.files()
    os.makedirs(outdir, exist_ok=True)
    ext = {'png': '.png', 'mac': '.mac', 'lpnt': '.lpnt'}[fmt]
    n = 0
    for o in d.desktop():
        if o.kind != 4 or o.tool != TOOL:
            continue
        data = files.get(f'{{D{o.id}T{TOOL}}}PIC') or files.get(f'{{D{o.id}T{TOOL}}}SUS')
        if not data:
            print(f'{o.name}: never saved, skipped')
            continue
        out = os.path.join(outdir, re.sub(r'[/\\:]', '-', o.name) + ext)
        open(out, 'wb').write(write_picture(read_lpnt(data), fmt, aspect))
        print(out)
        n += 1
    if not n:
        print(f'no LisaPaint pictures on {d.volname}')


def main():
    args = sys.argv[1:]
    aspect = '--aspect' in args
    args = [a for a in args if a != '--aspect']
    if args[:1] == ['export'] and len(args) >= 2:
        fmt = 'png'
        if '--format' in args:
            i = args.index('--format')
            fmt = args[i + 1]
            del args[i:i + 2]
        if fmt not in ('png', 'mac', 'lpnt'):
            raise SystemExit('--format png, mac or lpnt')
        export(args[1], args[2] if len(args) > 2 else '.', fmt, aspect)
    elif len(args) == 2:
        src, dst = args
        bits = read_picture(open(src, 'rb').read(), kind(src))
        open(dst, 'wb').write(write_picture(bits, kind(dst), aspect))
    else:
        sys.exit(__doc__.split('\n\n')[1])


if __name__ == '__main__':
    main()
