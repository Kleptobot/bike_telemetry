#!/usr/bin/env python3
"""Convert simulator PPM frame dumps to PNG.

Uses only the standard library (zlib + struct) -- no Pillow needed, so this
works on a bare Python install.

    python tools/ppm2png.py out/frame_0039.ppm            -> out/frame_0039.png
    python tools/ppm2png.py out/                          -> converts every .ppm
    python tools/ppm2png.py out/ --scale 3                -> 3x nearest-neighbour
    python tools/ppm2png.py out/ --gif out/ride.gif       -> also write an animation
"""
import argparse
import os
import struct
import sys
import zlib


def read_ppm(path):
    data = open(path, 'rb').read()
    if not data.startswith(b'P6'):
        raise ValueError(f'{path}: not a binary PPM')
    # Header is whitespace-separated: magic, width, height, maxval.
    fields, i = [], 2
    while len(fields) < 3:
        while data[i:i + 1].isspace():
            i += 1
        if data[i:i + 1] == b'#':                       # comment line
            while data[i] != 0x0A:
                i += 1
            continue
        j = i
        while not data[j:j + 1].isspace():
            j += 1
        fields.append(int(data[i:j]))
        i = j
    i += 1                                              # single whitespace after maxval
    w, h, _maxval = fields
    return w, h, data[i:i + w * h * 3]


def write_png(path, w, h, rgb, scale=1):
    rows = []
    for y in range(h):
        row = bytearray()
        base = y * w * 3
        for x in range(w):
            px = rgb[base + x * 3: base + x * 3 + 3]
            row += px * scale
        line = b'\x00' + bytes(row)
        rows.extend([line] * scale)
    raw = b''.join(rows)

    def chunk(tag, payload):
        return (struct.pack('>I', len(payload)) + tag + payload
                + struct.pack('>I', zlib.crc32(tag + payload) & 0xFFFFFFFF))

    png = (b'\x89PNG\r\n\x1a\n'
           + chunk(b'IHDR', struct.pack('>IIBBBBB', w * scale, h * scale, 8, 2, 0, 0, 0))
           + chunk(b'IDAT', zlib.compress(raw, 9))
           + chunk(b'IEND', b''))
    open(path, 'wb').write(png)


def main():
    ap = argparse.ArgumentParser(description=__doc__,
                                 formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument('path', help='a .ppm file or a directory of them')
    ap.add_argument('--scale', type=int, default=2, help='nearest-neighbour scale (default 2)')
    ap.add_argument('--gif', help='also write an animated GIF of all frames')
    args = ap.parse_args()

    if os.path.isdir(args.path):
        files = sorted(f for f in os.listdir(args.path) if f.lower().endswith('.ppm'))
        files = [os.path.join(args.path, f) for f in files]
    else:
        files = [args.path]

    if not files:
        print(f'no .ppm files found in {args.path}', file=sys.stderr)
        return 1

    for src in files:
        w, h, rgb = read_ppm(src)
        dst = os.path.splitext(src)[0] + '.png'
        write_png(dst, w, h, rgb, args.scale)
        print(f'{src} -> {dst}  ({w * args.scale}x{h * args.scale})')

    if args.gif:
        # GIF needs Pillow; it is optional and only for the animation.
        try:
            from PIL import Image
        except ImportError:
            print('--gif needs Pillow (pip install pillow); PNGs were still written',
                  file=sys.stderr)
            return 0
        frames = [Image.open(os.path.splitext(f)[0] + '.png') for f in files]
        frames[0].save(args.gif, save_all=True, append_images=frames[1:],
                       duration=50, loop=0)
        print(f'wrote {args.gif} ({len(frames)} frames)')
    return 0


if __name__ == '__main__':
    sys.exit(main())
