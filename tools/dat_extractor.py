#!/usr/bin/env python3
"""
Tonka Construction MODULE*.DAT asset extractor.

Extracts all images (backgrounds + overlay sprites) from MODULE DAT archives.

MODULE DAT file format:
  - uint16: entry count (N)
  - N * uint32: absolute file offsets to overlay entries
  - Background entry (after index):
      16-byte header | 64-byte path | 96-byte rect area | 1024-byte BGRA palette
      24-byte image header (uint32 w, h, raw_size, 0, 0, 0) | raw pixels (w*h bytes)
  - Overlay entries at each offset:
      128-byte entry header (type, flags, w at +0x0A, h at +0x0C, ...)
      Animation offset table (f2*f4*f6 uint32 offsets)
      At each animation offset: 24-byte sprite header (w, h, pixel_count, comp_size, pad, pad)
      Followed by comp_size bytes of PackBits-compressed pixel data

Palette format: 256 entries of (B, G, R, reserved) - Windows RGBQUAD
Transparent color: palette index 0x0F
Compression: PackBits variant (literal count = ctrl+1 for ctrl<0x80, run count = 257-ctrl for ctrl>=0x80)
"""

import struct
import os
import sys
import json
from pathlib import Path

try:
    from PIL import Image
except ImportError:
    print("ERROR: Pillow is required. Install with: pip install Pillow")
    sys.exit(1)


TRANSPARENT_INDEX = 0x0F


def read_palette(data: bytes, offset: int) -> list[tuple[int, int, int]]:
    """Read 256-entry BGRA palette, return as RGB tuples."""
    palette = []
    for i in range(256):
        b, g, r, _ = data[offset + i * 4 : offset + i * 4 + 4]
        palette.append((r, g, b))
    return palette


def packbits_decompress(src: bytes, comp_size: int) -> bytes:
    """Decompress PackBits-encoded data.

    Control byte < 0x80: copy next (ctrl+1) literal bytes
    Control byte >= 0x80: repeat next byte (257-ctrl) times
    """
    out = bytearray()
    si = 0
    rem = comp_size
    while rem > 0 and si < len(src):
        ctrl = src[si]
        si += 1
        rem -= 1
        if ctrl < 0x80:
            cnt = ctrl + 1
            out.extend(src[si : si + cnt])
            si += cnt
            rem -= cnt
        else:
            cnt = 257 - ctrl
            val = src[si]
            si += 1
            rem -= 1
            out.extend(bytes([val]) * cnt)
    return bytes(out)


def extract_module(dat_path: str, output_dir: str, verbose: bool = False) -> dict:
    """Extract all images from a MODULE DAT file.

    Returns a metadata dict describing all extracted assets.
    """
    module_name = Path(dat_path).stem.lower()
    mod_out = os.path.join(output_dir, module_name)
    os.makedirs(mod_out, exist_ok=True)

    with open(dat_path, "rb") as f:
        data = f.read()

    # Parse file header
    entry_count = struct.unpack_from("<H", data, 0)[0]
    offsets = [struct.unpack_from("<I", data, 2 + i * 4)[0] for i in range(entry_count)]

    # Background entry starts right after the index
    bg_start = 2 + entry_count * 4

    # Parse background header
    bg_header = data[bg_start : bg_start + 16]
    bg_path_raw = data[bg_start + 16 : bg_start + 80]
    bg_path = bg_path_raw.split(b"\x00")[0].decode("ascii", errors="replace")

    # Palette at bg_start + 176 (after 16 header + 64 path + 96 rect)
    pal_offset = bg_start + 176
    palette = read_palette(data, pal_offset)

    # Background image header at pal_offset + 1024
    img_hdr_offset = pal_offset + 1024
    bg_w = struct.unpack_from("<I", data, img_hdr_offset)[0]
    bg_h = struct.unpack_from("<I", data, img_hdr_offset + 4)[0]
    bg_raw_size = struct.unpack_from("<I", data, img_hdr_offset + 8)[0]

    # Background pixel data at img_hdr_offset + 24
    bg_pix_offset = img_hdr_offset + 24
    bg_pixels = data[bg_pix_offset : bg_pix_offset + bg_w * bg_h]

    # Save background image
    bg_img = Image.new("RGB", (bg_w, bg_h))
    px = bg_img.load()
    for y in range(bg_h):
        row_off = y * bg_w
        for x in range(bg_w):
            px[x, y] = palette[bg_pixels[row_off + x]]
    bg_path_out = os.path.join(mod_out, "background.png")
    bg_img.save(bg_path_out)

    metadata = {
        "module": module_name,
        "entry_count": entry_count,
        "background": {
            "width": bg_w,
            "height": bg_h,
            "original_path": bg_path,
            "file": "background.png",
        },
        "overlays": [],
    }

    if verbose:
        print(f"  Background: {bg_w}x{bg_h}, path={bg_path}")

    # Extract overlay entries
    extracted = 0
    for oi in range(entry_count):
        off = offsets[oi]
        if off + 128 > len(data):
            continue

        entry_hdr = data[off : off + 128]

        # Entry header fields
        entry_type = struct.unpack_from("<H", entry_hdr, 0x00)[0]
        f2 = struct.unpack_from("<H", entry_hdr, 0x02)[0]
        f4 = struct.unpack_from("<H", entry_hdr, 0x04)[0]
        f6 = struct.unpack_from("<H", entry_hdr, 0x06)[0]
        entry_flag = struct.unpack_from("<H", entry_hdr, 0x08)[0]
        entry_w = struct.unpack_from("<H", entry_hdr, 0x0A)[0]
        entry_h = struct.unpack_from("<H", entry_hdr, 0x0C)[0]

        num_anim_frames = f2 * f4 * f6
        if num_anim_frames == 0:
            continue

        # Read animation offset table from entry+128
        anim_table_off = off + 128
        if anim_table_off + num_anim_frames * 4 > len(data):
            continue

        for frame_idx in range(num_anim_frames):
            anim_off = struct.unpack_from("<I", data, anim_table_off + frame_idx * 4)[0]
            if anim_off == 0 or anim_off + 24 > len(data):
                continue

            # Sprite header (24 bytes)
            spr_w = struct.unpack_from("<I", data, anim_off)[0]
            spr_h = struct.unpack_from("<I", data, anim_off + 4)[0]
            pixel_count = struct.unpack_from("<I", data, anim_off + 8)[0]
            comp_size = struct.unpack_from("<I", data, anim_off + 12)[0]

            if spr_w == 0 or spr_h == 0 or spr_w > 2000 or spr_h > 500:
                continue
            if pixel_count != spr_w * spr_h:
                continue
            if comp_size == 0 or comp_size > 1000000:
                continue

            rle_off = anim_off + 24
            if rle_off + comp_size > len(data):
                continue

            # Decompress
            rle_data = data[rle_off : rle_off + comp_size]
            pixels = packbits_decompress(rle_data, comp_size)
            pixels = pixels[:pixel_count]  # Trim sentinel pixel

            if len(pixels) < pixel_count:
                if verbose:
                    print(
                        f"  [{oi}] frame {frame_idx}: short decode "
                        f"({len(pixels)}/{pixel_count})"
                    )
                continue

            # Create RGBA image with transparency
            img = Image.new("RGBA", (spr_w, spr_h))
            px_out = img.load()
            for y in range(spr_h):
                row_off = y * spr_w
                for x in range(spr_w):
                    idx = pixels[row_off + x]
                    r, g, b = palette[idx]
                    a = 0 if idx == TRANSPARENT_INDEX else 255
                    px_out[x, y] = (r, g, b, a)

            if num_anim_frames > 1:
                fname = f"overlay_{oi:03d}_frame{frame_idx:03d}.png"
            else:
                fname = f"overlay_{oi:03d}.png"
            img.save(os.path.join(mod_out, fname))
            extracted += 1

            # Position info from entry header
            pos_x = struct.unpack_from("<h", entry_hdr, 0x64)[0] if 0x66 <= len(entry_hdr) else 0
            pos_y = struct.unpack_from("<h", entry_hdr, 0x66)[0] if 0x68 <= len(entry_hdr) else 0

            overlay_info = {
                "index": oi,
                "frame": frame_idx,
                "width": spr_w,
                "height": spr_h,
                "type": entry_type,
                "flag": entry_flag,
                "file": fname,
            }
            metadata["overlays"].append(overlay_info)

    if verbose:
        print(f"  Extracted {extracted} overlay sprites")

    # Save metadata
    meta_path = os.path.join(mod_out, "metadata.json")
    with open(meta_path, "w") as f:
        json.dump(metadata, f, indent=2)

    return metadata


def main():
    import argparse

    parser = argparse.ArgumentParser(description="Extract Tonka Construction MODULE DAT assets")
    parser.add_argument("input", help="Path to MODULE*.DAT file or directory containing them")
    parser.add_argument(
        "-o", "--output", default="assets/images", help="Output directory (default: assets/images)"
    )
    parser.add_argument("-v", "--verbose", action="store_true", help="Verbose output")
    args = parser.parse_args()

    input_path = Path(args.input)
    output_dir = args.output

    if input_path.is_file():
        dat_files = [str(input_path)]
    elif input_path.is_dir():
        dat_files = sorted(str(p) for p in input_path.glob("MODULE*.DAT"))
    else:
        print(f"ERROR: {input_path} not found")
        sys.exit(1)

    if not dat_files:
        print("No MODULE*.DAT files found")
        sys.exit(1)

    os.makedirs(output_dir, exist_ok=True)

    total_overlays = 0
    for dat_file in dat_files:
        name = Path(dat_file).stem
        print(f"Extracting {name}...")
        try:
            meta = extract_module(dat_file, output_dir, verbose=args.verbose)
            n = len(meta["overlays"])
            total_overlays += n
            bg = meta["background"]
            print(f"  Background: {bg['width']}x{bg['height']}")
            print(f"  Overlays: {n}/{meta['entry_count']}")
        except Exception as e:
            print(f"  ERROR: {e}")
            if args.verbose:
                import traceback
                traceback.print_exc()

    print(f"\nDone. Extracted {len(dat_files)} backgrounds + {total_overlays} overlay sprites.")


if __name__ == "__main__":
    main()
