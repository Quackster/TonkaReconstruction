#!/usr/bin/env python3
"""
Tonka Construction BIN/POS file parser.

Parses PATH*.BIN (vehicle waypoints), CONT*.BIN (animation frame sequences),
M##OBJ##.BIN (object definitions), and *.POS (position data) files.

PATH*.BIN format:
  Array of (int32 x, int32 y) pairs. First pair is often the count or starting position.
  Sentinel value 0xFFFFFFFA (-6) in x or y separates path segments.

CONT*.BIN format:
  Array of uint16 frame indices for sprite animation sequences.

*.POS format:
  Array of (uint32 x, uint32 y) coordinate pairs for object placement.

M##OBJ##.BIN format:
  Similar to PATH - array of (int32 x, int32 y) with sentinel separators.
"""

import struct
import os
import sys
import json
from pathlib import Path


SENTINEL = -6  # 0xFFFFFFFA


def parse_path_bin(filepath: str) -> dict:
    """Parse PATH*.BIN or M##OBJ##.BIN waypoint files."""
    with open(filepath, "rb") as f:
        data = f.read()

    segments = []
    current_segment = []
    i = 0

    while i + 7 < len(data):
        x = struct.unpack_from("<i", data, i)[0]
        y = struct.unpack_from("<i", data, i + 4)[0]
        i += 8

        if x == SENTINEL or y == SENTINEL:
            if current_segment:
                segments.append(current_segment)
                current_segment = []
        else:
            current_segment.append({"x": x, "y": y})

    if current_segment:
        segments.append(current_segment)

    return {
        "file": os.path.basename(filepath),
        "type": "path",
        "segments": segments,
        "total_points": sum(len(s) for s in segments),
    }


def parse_cont_bin(filepath: str) -> dict:
    """Parse CONT*.BIN animation frame sequence files."""
    with open(filepath, "rb") as f:
        data = f.read()

    frame_count = len(data) // 2
    frames = [struct.unpack_from("<H", data, i * 2)[0] for i in range(frame_count)]

    return {
        "file": os.path.basename(filepath),
        "type": "animation_frames",
        "frame_count": frame_count,
        "frames": frames,
    }


def parse_pos_file(filepath: str) -> dict:
    """Parse *.POS position coordinate files."""
    with open(filepath, "rb") as f:
        data = f.read()

    point_count = len(data) // 8
    points = []
    for i in range(point_count):
        x = struct.unpack_from("<I", data, i * 8)[0]
        y = struct.unpack_from("<I", data, i * 8 + 4)[0]
        points.append({"x": x, "y": y})

    return {
        "file": os.path.basename(filepath),
        "type": "positions",
        "point_count": point_count,
        "points": points,
    }


def main():
    import argparse

    parser = argparse.ArgumentParser(description="Parse Tonka Construction BIN/POS files")
    parser.add_argument("input", help="Path to BIN/POS file or DATA directory")
    parser.add_argument(
        "-o", "--output", default="assets/data", help="Output directory (default: assets/data)"
    )
    args = parser.parse_args()

    input_path = Path(args.input)
    output_dir = args.output
    os.makedirs(output_dir, exist_ok=True)

    if input_path.is_file():
        files = [input_path]
    elif input_path.is_dir():
        files = sorted(
            list(input_path.glob("PATH*.BIN"))
            + list(input_path.glob("CONT*.BIN"))
            + list(input_path.glob("M*.BIN"))
            + list(input_path.glob("*.POS"))
        )
    else:
        print(f"ERROR: {input_path} not found")
        sys.exit(1)

    for filepath in files:
        name = filepath.stem.lower()
        fname = filepath.name.upper()

        if fname.startswith("PATH") or (fname.startswith("M") and "OBJ" in fname):
            result = parse_path_bin(str(filepath))
            subdir = "paths" if fname.startswith("PATH") else "objects"
        elif fname.startswith("CONT"):
            result = parse_cont_bin(str(filepath))
            subdir = "animations"
        elif filepath.suffix.upper() == ".POS":
            result = parse_pos_file(str(filepath))
            subdir = "positions"
        else:
            result = parse_path_bin(str(filepath))
            subdir = "objects"

        out_subdir = os.path.join(output_dir, subdir)
        os.makedirs(out_subdir, exist_ok=True)
        out_path = os.path.join(out_subdir, f"{name}.json")

        with open(out_path, "w") as f:
            json.dump(result, f, indent=2)

        if result["type"] == "path":
            print(f"  {fname}: {len(result['segments'])} segments, {result['total_points']} points")
        elif result["type"] == "animation_frames":
            print(f"  {fname}: {result['frame_count']} frames")
        elif result["type"] == "positions":
            print(f"  {fname}: {result['point_count']} positions")


if __name__ == "__main__":
    main()
