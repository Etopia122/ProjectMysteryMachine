#!/usr/bin/env python3
# Makes the CIA's HOME Menu icon (48x48) and banner (256x128) from the game's
# own GameCube memory card icon and banner (sd2gcicon.tpl, sd2gcbanner.tpl on
# the player's disc), plus a short silent banner sound. Nothing from the game is
# stored in the repository: this runs at build time.
# usage: python n3ds/tools/make_cia_art.py <game files dir> <output dir>
import os, struct, sys, zlib


def read_tpl(path, index=0):
    """Decodes one RGB5A3 image of a TPL file into rows of (r, g, b, a)."""
    d = open(path, 'rb').read()
    _, count, table = struct.unpack_from('>III', d, 0)
    image_off, _ = struct.unpack_from('>II', d, table + index * 8)
    h, w, fmt, data = struct.unpack_from('>HHII', d, image_off)
    if fmt != 5:
        raise ValueError('%s: format %d, expected RGB5A3 (5)' % (path, fmt))
    px = [[(0, 0, 0, 0)] * w for _ in range(h)]
    pos = data
    for ty in range(0, h, 4):
        for tx in range(0, w, 4):
            for y in range(4):
                for x in range(4):
                    v = struct.unpack_from('>H', d, pos)[0]
                    pos += 2
                    if v & 0x8000:
                        c = (((v >> 10) & 31) * 255 // 31, ((v >> 5) & 31) * 255 // 31, (v & 31) * 255 // 31, 255)
                    else:
                        c = (((v >> 8) & 15) * 17, ((v >> 4) & 15) * 17, (v & 15) * 17, ((v >> 12) & 7) * 255 // 7)
                    if ty + y < h and tx + x < w:
                        px[ty + y][tx + x] = c
    return px


def scale(img, w, h):
    """Bilinear resize."""
    sh, sw = len(img), len(img[0])
    out = []
    for y in range(h):
        fy = (y + 0.5) * sh / h - 0.5
        y0 = max(0, min(sh - 1, int(fy)))
        y1 = min(sh - 1, y0 + 1)
        ty = min(max(fy - y0, 0.0), 1.0)
        row = []
        for x in range(w):
            fx = (x + 0.5) * sw / w - 0.5
            x0 = max(0, min(sw - 1, int(fx)))
            x1 = min(sw - 1, x0 + 1)
            tx = min(max(fx - x0, 0.0), 1.0)
            c = []
            for k in range(4):
                a = img[y0][x0][k] * (1 - tx) + img[y0][x1][k] * tx
                b = img[y1][x0][k] * (1 - tx) + img[y1][x1][k] * tx
                c.append(int(round(a * (1 - ty) + b * ty)))
            row.append(tuple(c))
        out.append(row)
    return out


def blit(dst, src, ox, oy):
    for y, row in enumerate(src):
        for x, (r, g, b, a) in enumerate(row):
            if 0 <= oy + y < len(dst) and 0 <= ox + x < len(dst[0]):
                br, bg, bb, ba = dst[oy + y][ox + x]
                t = a / 255.0
                dst[oy + y][ox + x] = (int(r * t + br * (1 - t)), int(g * t + bg * (1 - t)), int(b * t + bb * (1 - t)),
                                       max(a, ba))


def write_png(path, img):
    h, w = len(img), len(img[0])
    raw = b''.join(b'\0' + bytes(v for px in row for v in px) for row in img)

    def chunk(tag, data):
        return struct.pack('>I', len(data)) + tag + data + struct.pack('>I', zlib.crc32(tag + data) & 0xffffffff)

    png = b'\x89PNG\r\n\x1a\n' + chunk(b'IHDR', struct.pack('>IIBBBBB', w, h, 8, 6, 0, 0, 0))
    png += chunk(b'IDAT', zlib.compress(raw, 9)) + chunk(b'IEND', b'')
    open(path, 'wb').write(png)


def write_silent_wav(path, seconds=0.5, rate=22050):
    n = int(seconds * rate)
    data = b'\0\0' * n
    hdr = b'RIFF' + struct.pack('<I', 36 + len(data)) + b'WAVEfmt ' + struct.pack('<IHHIIHH', 16, 1, 1, rate, rate * 2, 2, 16)
    open(path, 'wb').write(hdr + b'data' + struct.pack('<I', len(data)) + data)


def main():
    files, out = sys.argv[1], sys.argv[2]
    os.makedirs(out, exist_ok=True)
    icon = read_tpl(os.path.join(files, 'sd2gcicon.tpl'), 0)
    banner = read_tpl(os.path.join(files, 'sd2gcbanner.tpl'), 0)
    # Icon: the memory card icon, 32 -> 48 pixels.
    write_png(os.path.join(out, 'icon.png'), scale(icon, 48, 48))
    # Banner: the memory card banner (96x32 -> 240x80) under the icon on a dark
    # purple background.
    canvas = [[(24, 12, 40, 255)] * 256 for _ in range(128)]
    blit(canvas, scale(icon, 40, 40), 108, 4)
    blit(canvas, scale(banner, 240, 80), 8, 46)
    write_png(os.path.join(out, 'banner.png'), canvas)
    write_silent_wav(os.path.join(out, 'banner.wav'))
    print('icon.png, banner.png, banner.wav ->', out)


if __name__ == '__main__':
    main()
