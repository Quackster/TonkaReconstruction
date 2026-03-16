# Tonka Construction - Modern Reconstruction

A modern C++17/SDL2 reconstruction of **Tonka Construction** (1996), reverse-engineered from the original game files. The original 16-bit Windows 3.1 game cannot run on modern 64-bit systems; this project rebuilds it as a native application.

> **Legal notice:** This project contains no copyrighted assets. You must supply your own copy of the original game disc to extract assets. The extraction tools and game engine source code are original work.

## Prerequisites

### Windows (MSYS2 / MinGW64)

1. Install [MSYS2](https://www.msys2.org/)
2. Open the **MSYS2 MinGW64** terminal and install dependencies:

```bash
pacman -S mingw-w64-x86_64-gcc mingw-w64-x86_64-cmake \
          mingw-w64-x86_64-SDL2 mingw-w64-x86_64-SDL2_image \
          mingw-w64-x86_64-SDL2_mixer mingw-w64-x86_64-SDL2_ttf
```

3. Install Python 3.10+ with [Pillow](https://pypi.org/project/Pillow/):

```bash
pip install Pillow
```

## Obtaining the Original Game Files

You need the original **Tonka Construction** CD-ROM (1996). The files can also be found on archive.org if you own a license.

1. Mount or extract the CD contents
2. Copy the entire disc into a `.tonka/` directory at the project root:

```
TonkaReconstruction/
  .tonka/
    AUTORUN.INF
    PLAY.EXE
    SETUP.EXE
    TONKA.TXT
    DATA/
      MODULE01.DAT
      MODULE02.DAT
      ...
      TONKA.EXE
      SNDS/
        *.WAV
    ...
```

The `.tonka/` directory is gitignored and will not be committed.

## Extracting Assets

From the project root, run the Python extraction tools to convert the proprietary MODULE DAT archives into standard PNG/WAV/JSON files:

```bash
# 1. Extract all images (backgrounds + overlay sprites) from MODULE*.DAT
python tools/dat_extractor.py .tonka/DATA/ -o assets/images -v

# 2. Parse BIN/POS data files (vehicle paths, animation frames, object positions)
python tools/bin_parser.py .tonka/DATA/ -o assets/data

# 3. Copy audio assets (already standard WAV format, no conversion needed)
mkdir -p assets/audio/ambient assets/audio/sfx assets/audio/snds
cp .tonka/DATA/AMBNT*.WAV assets/audio/ambient/
cp .tonka/DATA/SFX*.WAV   assets/audio/sfx/
cp .tonka/DATA/SNDS/*.WAV  assets/audio/snds/
cp .tonka/DATA/LOAD.WAV    assets/audio/

# 4. Export overlay position data for each module (used by the engine at runtime)
python tools/export_module_data.py
```

Step 4 uses a helper script. If it doesn't exist yet, the `dat_extractor.py` output includes `metadata.json` per module which the engine can also use. Alternatively, this data is generated automatically during the build's asset copy step.

After extraction you should have:
- `assets/images/` - ~2,800 PNG sprites + 36 backgrounds
- `assets/audio/` - ~66 WAV audio files
- `assets/data/` - ~76 JSON data files

The `assets/` directory is gitignored since it contains copyrighted material.

## Building

All commands should be run from the **MSYS2 MinGW64** terminal:

```bash
# Configure
mkdir build && cd build
cmake -G "MinGW Makefiles" ..

# Build
mingw32-make -j4

# The build automatically copies assets/ into the build directory
```

### Bundling for Distribution

To create a standalone package (still requires users to supply their own assets):

```bash
# From the build directory after a successful build:
mkdir -p TonkaConstruction-dist

# Copy the executable
cp TonkaConstruction.exe TonkaConstruction-dist/

# Copy required DLLs (from MSYS2 MinGW64)
ldd TonkaConstruction.exe | grep mingw64 | awk '{print $3}' | \
  xargs -I{} cp {} TonkaConstruction-dist/

# Copy assets (user must generate these from their own disc)
cp -r assets TonkaConstruction-dist/
```

## Controls

| Input | Action |
|-------|--------|
| **Mouse click** | Navigate, select vehicles, interact with objects |
| **Escape** | Go back to previous screen / exit |
| **H** or **F1** | Open help screen (in activity areas) |
| **Arrow keys** | Scroll viewport (in wide areas like Quarry, Desert) |

## Project Structure

```
src/
  main.cpp                  Entry point
  engine/
    Application.h/cpp       SDL2 init, main loop, subsystem management
    Renderer.h/cpp          640x480 logical resolution, scaled rendering
    AssetManager.h/cpp      Texture and sound caching
    AudioManager.h/cpp      SDL2_mixer: ambient loops + SFX channels
    InputManager.h/cpp      Mouse input with logical coordinate mapping
    SceneManager.h/cpp      Stack-based scene transitions
    TextRenderer.h/cpp      SDL2_ttf text rendering
  game/
    SignInScene.h/cpp        Player name entry (MODULE15)
    MainMapScene.h/cpp       Hub world with area navigation (MODULE10)
    QuarryScene.h/cpp        Quarry with vehicle mechanics (MODULE01)
    GarageScene.h/cpp        Vehicle spray painting (MODULE12)
    CityMapScene.h/cpp       City sub-hub with 3 sites (MODULE11)
    TimeCardScene.h/cpp      Progress tracking / quit (MODULE13)
    ActivityScene.h/cpp      Data-driven scene for all other areas
  objects/
    GameObject.h/cpp         Animated sprite with hit testing
    Vehicle.h/cpp            Path-following vehicle with state machine
  data/
    ModuleData.h             MODULE DAT position data loader
    GameState.h              Save/load progression tracking
tools/
  dat_extractor.py          MODULE*.DAT -> PNG + metadata JSON
  bin_parser.py             PATH/CONT/OBJ/POS BIN -> JSON
  export_module_data.py     Overlay position data -> per-module JSON
  ghidra_scripts/           Ghidra decompilation helpers
```

## TODO

The following items are needed to fully match the original game's behavior and appearance:

### Gameplay Mechanics
- [ ] Per-area scripted vehicle sequences (cement pouring in City, snow rescue in Avalanche, tunnel digging in Desert)
- [ ] Overlay visibility state machine: type 1 overlays with `flag=8` should be hidden initially and revealed by game events (e.g., clicking flashing arrows summons vehicles)
- [ ] Crane claw physics: position claw above target, click to drop/grab (currently simplified to drag-and-drop)
- [ ] Front loader dig/scoop/dump cycle at specific targets (black pile, red pile, gold wall)
- [ ] Bulldozer push mechanics: push rocks/brush in movement direction
- [ ] Cement truck pour animation for floors and foundation walls
- [ ] Dump truck material transport between locations
- [ ] Vehicle approach/departure driving animations along PATH*.BIN waypoints when entering/leaving areas
- [ ] Decoration placement phase after area completion (arrows mark valid spots, some decorations animate)
- [ ] Area-specific completion criteria (from TONKA.TXT hints)

### UI & Navigation
- [ ] Options menu overlay (MODULE10 overlay 037: Time Clock, Go Back, Clipboard, Phone buttons)
- [ ] Proper cursor state machine (invisible during narration, hourglass during animations, vehicle-specific cursors)
- [ ] Sign-in screen: multiple save slots with numbered time card grid
- [ ] Time card cell stamps should show small certificate thumbnails for completed areas

### Audio & Narration
- [ ] Character voice dialogue playback (Tonka Joe, Rusty, Miko) - voice clips embedded in MODULE DAT animation sequences
- [ ] SFX1-SFX7 mapped to specific game events
- [ ] LOAD.WAV playback during scene transitions

### Video & Animation
- [ ] Intro movie (MOVIE1.MVE - proprietary "MVE4" format, 640x480, 22050 Hz audio)
- [ ] Full-screen driving animation sequences (type 1 overlays with flag=8, 640x480, 50-150+ frames)
- [ ] PCX panorama rendering (PAN10.PCX)
- [ ] CONTDN/CONTUP frame index tables for scrolling background animation

### Data
- [ ] Vehicle interior scenes (MODULE71, MODULE81, MODULE91) - monster truck, dozer, grader cab views
- [ ] M##OBJ##.BIN object bounding box / hotspot definitions per area
- [ ] M##O##A.BIN additional object path data

## Technical Details

The original game uses a custom engine (not Macromedia Director) with:
- **WinG** for fast DIB blitting (replaced by SDL2 hardware rendering)
- **Proprietary MODULE DAT archives** containing 8-bit indexed images with BGRA palettes
- **PackBits RLE compression** for overlay sprites (decompression routine at VA 0x0042EA66)
- **22050 Hz WAV audio** for ambient and SFX (directly usable, no conversion needed)
- Transparent color: palette index `0x0F`

The MODULE DAT format was reverse-engineered using Ghidra headless decompilation of `TONKA.EXE` (32-bit PE, Borland C++, Oct 1999 re-release).

## License

The source code in this repository is provided for educational and preservation purposes. The original Tonka Construction game is copyrighted by Hasbro/Media Station. No copyrighted assets are included in this repository.
