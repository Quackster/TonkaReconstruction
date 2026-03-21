# Tonka Construction — Compatibility Patch

Run the original **Tonka Construction** (1996) on modern 64-bit Windows by replacing the obsolete WinG graphics library with a lightweight shim DLL.

> **Legal notice:** This project contains no copyrighted assets. You must supply your own copy of the original game disc. Only the patch code and tooling are original work.

## How It Works

The original `TONKA.EXE` is a 32-bit Windows PE executable (Borland C++, 1999 re-release). It runs fine under WOW64 on 64-bit Windows, except for two problems:

1. **WING32.dll** — The game uses the WinG graphics library (1994), which isn't included with modern Windows. Our shim DLL implements the same 4 functions using standard GDI calls (`CreateDIBSection`, `BitBlt`, `SetDIBColorTable`).

2. **Tonka.ini path** — The game calls `GetPrivateProfileStringA("Tonka.ini")` with a bare filename, which searches the Windows directory instead of the game directory. Our DLL patches this at load time via IAT hooking, redirecting INI lookups to the EXE's own directory.

## Quick Start

### 1. Place the original game files

Copy the contents of your Tonka Construction CD into `.tonka/` at the project root:

```
TonkaReconstruction/
  .tonka/
    DATA/
      TONKA.EXE
      CW3215.DLL
      MODULE01.DAT ... MODULE99.DAT
      AMBNT*.WAV, SFX*.WAV, LOAD.WAV
      SNDS/*.WAV
      ...
    WING/
      WING32.DL_
    ...
```

The `.tonka/` directory is gitignored.

### 2. Build the patch

Requires MSYS2 with the 32-bit MinGW toolchain:

```bash
# Install 32-bit GCC (only needed once)
pacman -S mingw-w64-i686-gcc

# Build the shim DLL
./patch/build.sh
```

This produces `patch/WING32.dll` — a 32-bit DLL that replaces the original WinG library.

### 3. Launch the game

```bash
# From an MSYS2 terminal
./patch/launch.bat

# Or double-click patch/launch.bat from Explorer
```

The launcher copies `WING32.dll` into the game directory and starts `TONKA.EXE`. On first run, the DLL auto-creates `Tonka.ini` and a `saves/` directory next to the executable.

## What the Patch Does

| Problem | Cause | Fix |
|---------|-------|-----|
| Missing WING32.dll | WinG library not on modern Windows | Shim DLL maps 4 WinG functions to GDI equivalents |
| "No Local Directory, Please Run Setup" | `GetPrivateProfileStringA` looks for `Tonka.ini` in Windows directory | IAT hook redirects to EXE directory |
| No Tonka.ini or saves folder | Original installer created these | DLL auto-creates both on first load |

### WinG → GDI Function Mapping

| WinG Function | GDI Equivalent |
|---------------|----------------|
| `WinGCreateDC()` | `CreateCompatibleDC(NULL)` |
| `WinGCreateBitmap(hdc, info, bits)` | `CreateDIBSection(hdc, info, DIB_RGB_COLORS, bits, 0, 0)` |
| `WinGBitBlt(dst, x, y, w, h, src, sx, sy)` | `BitBlt(dst, x, y, w, h, src, sx, sy, SRCCOPY)` |
| `WinGSetDIBColorTable(hdc, start, n, colors)` | `SetDIBColorTable(hdc, start, n, colors)` |

## Project Structure

```
patch/
  wing32.c          WING32.dll source — WinG shim + IAT hook
  wing32.def        DLL export definitions
  build.sh          Build script (produces 32-bit WING32.dll)
  launch.bat        Deploys shim and launches game
tools/
  dat_extractor.py  MODULE*.DAT → PNG + metadata JSON
  bin_parser.py     PATH/CONT/OBJ/POS BIN → JSON
  export_module_data.py  Overlay position data → per-module JSON
  ghidra_scripts/   Ghidra decompilation automation
  decompiled_raw.txt  Full Ghidra decompilation of TONKA.EXE (223 functions)
```

## Controls

| Input | Action |
|-------|--------|
| **Mouse click** | Navigate, select vehicles, interact with objects |
| **Escape** | Go back to previous screen / exit |

## Asset Extraction Tools

The `tools/` directory contains Python scripts for extracting and converting the game's proprietary data formats into standard files (PNG, WAV, JSON). These are useful for analysis and for any future reconstruction work.

```bash
pip install Pillow

# Extract images from MODULE*.DAT archives
python tools/dat_extractor.py .tonka/DATA/ -o assets/images -v

# Parse binary data files (vehicle paths, animation frames, positions)
python tools/bin_parser.py .tonka/DATA/ -o assets/data

# Copy audio (already standard WAV)
mkdir -p assets/audio/{ambient,sfx,snds}
cp .tonka/DATA/AMBNT*.WAV assets/audio/ambient/
cp .tonka/DATA/SFX*.WAV   assets/audio/sfx/
cp .tonka/DATA/SNDS/*.WAV  assets/audio/snds/

# Export overlay position metadata
python tools/export_module_data.py
```

## Technical Details

The original game engine (not Macromedia Director) uses:
- **WinG** for fast 8-bit DIB blitting (256-color palette mode)
- **Proprietary MODULE DAT archives** containing indexed images with BGRA palettes and PackBits RLE compression
- **Win32 waveOut API** for 22050 Hz mono PCM audio playback
- **Borland C++ runtime** (`CW3215.DLL`)
- Transparent color: palette index `0x0F`
- Logical resolution: 640×480

Reverse-engineered using Ghidra from `TONKA.EXE` (32-bit PE, Borland C++, October 1999 re-release).

## License

The source code in this repository is provided for educational and preservation purposes. The original Tonka Construction game is copyrighted by Hasbro/Media Station. No copyrighted assets are included.
