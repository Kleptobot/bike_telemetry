# convert_to_rgb565.py
from PIL import Image
import struct
import sys
import os
from pathlib import Path

def rgb888_to_rgb565(r, g, b):
    return ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3)

def convert_file(infile, outfile, size=(256, 256)):
    """Convert a single image file to RGB565 raw format."""
    try:
        im = Image.open(infile).convert('RGB').resize(size, Image.LANCZOS)
        w, h = im.size
        with open(outfile, 'wb') as f:
            for y in range(h):
                for x in range(w):
                    r, g, b = im.getpixel((x, y))
                    v = rgb888_to_rgb565(r, g, b)
                    f.write(struct.pack('<H', v))
        print(f"✓ {infile} → {outfile}")
        return True
    except Exception as e:
        print(f"✗ {infile}: {e}", file=sys.stderr)
        return False

def convert_directory(input_dir, output_dir, size=(256, 256), extensions=('.png', '.jpg', '.jpeg', '.bmp')):
    input_path = Path(input_dir)
    output_path = Path(output_dir)

    if not input_path.is_dir():
        print(f'Error: {input_dir} is not a directory', file=sys.stderr)
        return False

    count = 0
    success = 0

    for infile in input_path.rglob('*'):
        if infile.is_file() and infile.suffix.lower() in extensions:
            rel_path = infile.relative_to(input_path)
            outfile = output_path / rel_path.with_suffix('.raw')
            outfile.parent.mkdir(parents=True, exist_ok=True)
            count += 1
            if convert_file(str(infile), str(outfile), size):
                success += 1

    print(f"\nConversion complete: {success}/{count} files processed successfully")
    return success == count

def convert(infile, outfile, size=(256, 256)):
    return convert_file(infile, outfile, size)

if __name__ == '__main__':
    if len(sys.argv) < 3:
        print('Usage:')
        print('  Single file:  convert_to_rgb565.py input.png output.raw')
        print('  Directory:    convert_to_rgb565.py input_dir output_dir')
        print('  With size:    convert_to_rgb565.py input_dir output_dir 256 256')
        sys.exit(1)

    input_path = sys.argv[1]
    output_path = sys.argv[2]
    size = (256, 256)

    if len(sys.argv) == 4:
        size = (int(sys.argv[3]), int(sys.argv[3]))
    elif len(sys.argv) == 5:
        size = (int(sys.argv[3]), int(sys.argv[4]))

    if os.path.isdir(input_path):
        success = convert_directory(input_path, output_path, size)
        sys.exit(0 if success else 1)
    elif os.path.isfile(input_path):
        success = convert(input_path, output_path, size)
        sys.exit(0 if success else 1)
    else:
        print(f'Error: {input_path} not found', file=sys.stderr)
        sys.exit(1)