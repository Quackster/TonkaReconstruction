#!/usr/bin/env python3
"""
Export overlay position/metadata for each MODULE*.DAT file to JSON.

The C++ engine loads these JSON files at runtime to know where to place
each sprite overlay on screen.

Usage:
    python tools/export_module_data.py [--data-dir .tonka/DATA] [--output assets/data/modules]
"""

import struct
import json
import os
import sys
from pathlib import Path


def export_module(dat_path: str, output_dir: str) -> dict:
    """Read a MODULE DAT file and export overlay entry metadata to JSON."""
    with open(dat_path, "rb") as f:
        data = f.read()

    count = struct.unpack_from("<H", data, 0)[0]
    offsets = [struct.unpack_from("<I", data, 2 + i * 4)[0] for i in range(count)]

    # Background dimensions from image header
    bg_start = 2 + count * 4
    img_hdr = bg_start + 176 + 1024  # after header + path + rects + palette
    bg_w = struct.unpack_from("<I", data, img_hdr)[0]
    bg_h = struct.unpack_from("<I", data, img_hdr + 4)[0]

    entries = []
    for oi in range(count):
        off = offsets[oi]
        if off + 128 > len(data):
            continue
        hdr = data[off : off + 128]

        entries.append(
            {
                "index": oi,
                "type": struct.unpack_from("<H", hdr, 0x00)[0],
                "width": struct.unpack_from("<H", hdr, 0x0A)[0],
                "height": struct.unpack_from("<H", hdr, 0x0C)[0],
                "cx": struct.unpack_from("<H", hdr, 0x62)[0],
                "cy": struct.unpack_from("<H", hdr, 0x64)[0],
                "flag": struct.unpack_from("<H", hdr, 0x08)[0],
                "frames": (
                    struct.unpack_from("<H", hdr, 0x02)[0]
                    * struct.unpack_from("<H", hdr, 0x04)[0]
                    * struct.unpack_from("<H", hdr, 0x06)[0]
                ),
                "nav_module": struct.unpack_from("<H", hdr, 0x74)[0],
            }
        )

    mod_name = Path(dat_path).stem.lower()
    mod_id = int(mod_name.replace("module", ""))

    result = {
        "module_id": mod_id,
        "entry_count": count,
        "bg_width": bg_w,
        "bg_height": bg_h,
        "entries": entries,
    }

    os.makedirs(output_dir, exist_ok=True)
    out_path = os.path.join(output_dir, f"{mod_name}.json")
    with open(out_path, "w") as f:
        json.dump(result, f, indent=2)

    return result


def main():
    import argparse

    parser = argparse.ArgumentParser(description="Export MODULE overlay position data to JSON")
    parser.add_argument(
        "--data-dir", default=".tonka/DATA", help="Path to DATA directory with MODULE*.DAT files"
    )
    parser.add_argument(
        "--output", default="assets/data/modules", help="Output directory for JSON files"
    )
    args = parser.parse_args()

    data_dir = Path(args.data_dir)
    dat_files = sorted(data_dir.glob("MODULE*.DAT"))

    if not dat_files:
        print(f"No MODULE*.DAT files found in {data_dir}")
        sys.exit(1)

    for dat_file in dat_files:
        result = export_module(str(dat_file), args.output)
        print(
            f"  {dat_file.stem}: {result['entry_count']} entries, "
            f"bg {result['bg_width']}x{result['bg_height']}"
        )

    print(f"\nExported {len(dat_files)} module data files to {args.output}/")


if __name__ == "__main__":
    main()
