# MIDIVFX
A C++ openFrameworks application that transforms MIDI music into animated piano roll visualizations, with support for live input, MIDI file playback, and offline video rendering.

## Project Overview

**Goal**
- MIDI realtime / MIDI file reactive visualizer
- open-source
- CC reactive
- Fully customizable visual effects
- Offline video rendering support

**Tech Stack:**
- **Language:** C++ with openFrameworks
- **Graphics:** OpenGL (3D rendering)
- **MIDI:** ofxMidi / ofxMidifile addons
- **Config:** yaml-cpp (YAML configuration)
- **Video export:** FFmpeg (H.264/MP4)
- **Development Platform:** Windows (Visual Studio)

## Build

**Prerequisites**
- [openFrameworks](https://openframeworks.cc/) for Windows (Visual Studio)
- Visual Studio 2022 (toolset v143, C++20)
- openFrameworks addons placed in `OF_ROOT/addons/`:
  - `ofxMidi`
  - `ofxMidifile`
  - `ofxGui`
- `ffmpeg.exe` placed in `bin/` (required for video export only)

**Steps**

1. Open `midivfx.sln` in Visual Studio.
2. Select configuration (`Debug` or `Release`) and platform (`x64`).
3. Build → outputs `bin/midivfx_debug.exe` or `bin/midivfx.exe`.

`yaml-cpp` is embedded under `libs/yaml-cpp/` and compiled as part of the project — no separate installation needed.

## Usage

Run the executable from `bin/`. It reads `config.yaml` from the same directory by default.

```
midivfx.exe [--config <path/to/config.yaml>]
```

Use `--config` to point to a different configuration file.

**Modes** (set `playback.mode` in config.yaml):

| Mode | Description |
|------|-------------|
| `preview` | Load a MIDI file and display it in a window |
| `live` | Visualize real-time input from a MIDI port |
| `export` | Render a MIDI file to an MP4 video (requires `ffmpeg.exe`) |

**Preview / Live** — a window opens with the visualization. Mouse controls the 3D camera (drag to orbit, scroll to zoom).

**Export** — no window interaction needed. Frames are captured to `export_frames/` then encoded to the path specified in `export.path`. Progress is printed to stdout.

## Architecture

```
midivfx/
├── src/
│   ├── main.cpp                  # Entry point
│   ├── ofApp.h/cpp               # Main application (setup/update/draw, camera, playback)
│   ├── VisualizerConfig.h        # Configuration data structures and enums
│   ├── ConfigLoader.h/cpp        # YAML config file parsing
│   ├── MidiProcessor.h/cpp       # MIDI event processing and state tracking
│   ├── MidiFileLoader.h/cpp      # MIDI file parsing (SMF, time signatures, beat events)
│   ├── ChannelState.h            # Per-channel MIDI state (key statuses, note histories)
│   ├── PianoKeys.h/cpp           # Piano keyboard container and beat line rendering
│   ├── PianoKey.h/cpp            # Individual key geometry and rendering
│   ├── NoteHistoryRenderer.h/cpp # Waterfall/history visualization (fill, decay, CC modes)
│   ├── VideoExporter.h/cpp       # Offline video export via FFmpeg
│   ├── StyleResolution.h/cpp     # Resolves visual styles based on config and track/channel overrides
│   └── midiUtil.h                # Core MIDI data structures
└── examples/
    ├── config.yaml               # Example configuration files
    ├── *.mp3                     # Example audio files
    └── *.mid                     # Example MIDI files
```

## Configuration

All behavior is controlled via yaml config file.

**playback** — input source
- `mode`: `preview` (MIDI file), `live` (real-time MIDI input), `export` (offline render)
- `midiFilePath`: path to `.mid` file
- `audioFilePath`: path to audio file
- `midiInPort`: live MIDI input port index
- `startPadding`: extra lead-in time before the first note (microseconds)
- `endPadding`: extra tail time after the last note (microseconds)

**export** — video output
- `path`: output `.mp4` file path
- `fps`: frame rate
- `limit`: max duration in microseconds (`0` = no limit)

**timing** — event scheduling offsets
- `dispatchOffset`: how early before a note's on-time to dispatch events (microseconds)
- `removeOffset`: how long after pedal-off to keep note history (microseconds)
- `audioOffset`: audio delay relative to visuals (microseconds); positive = audio starts later

**display** — visual layout
- `width`, `height`: window resolution
- `showPiano`: show piano keyboard or just a line
- `reverse`: true waterfall (notes fall toward camera) vs. normal
- `horizontal`: rotate 90° (pitch on Y-axis, time on X-axis)
- `speed`: scroll speed in pixels per microsecond
- `averageWidth`: use average key width instead of exact key widths
- `beat`: overlay beat/bar lines

**camera** — 3D view
- `pitchAxis`, `timeAxis`, `zAxis`: camera position
- `targetPitchAxis`, `targetTimeAxis`: camera target

**style** — note and pedal appearance
- `note.mode`: `fill`, `decay`, or `CC<n>` (e.g. `CC64`)
- `note.velocity`: modulate alpha/brightness by MIDI velocity
- `note.color`: hex color
- `note.gap`: gap between adjacent notes as a ratio of note height `[0, 1]`
- `note.radius`: corner radius as a ratio of `min(scrollExtent, noteWidth)` `[0, 0.5]`
- `note.z`: z-depth offset of note mesh vertices
- `note.decay.rate` / `note.decay.timeSegment`: decay parameters
- `pedal.show`, `pedal.color`: sustain pedal visualization
- `pedal.track`, `pedal.channel`: redirect pedal data to a different track/channel (1-based)
- `remaps`: percussive pitch remapping entries

**tracks** — per-track/channel overrides
- `track`: track numbers (1-based, supports ranges like `1,3-5`)
- `regex`: match track names by regex pattern (case-insensitive)
- `channels`: nested per-channel style overrides

**debug** — top-level boolean; shows debug overlays such as history IDs when `true`

## Roadmap
- [x] Piano keyboard
- [x] History waterfall
    - [x] fill mode
    - [x] decay mode
    - [x] CC reactive mode
- [x] Pedal visualizing (binary on/off)
- [x] Note visual mapping (percussive remaps)
- [x] MIDI file input
    - [x] Reversed mode (true waterfall)
- [x] Barlines and time signatures
- [x] Video export
- [x] Camera positioning
- [x] Horizontal mode
- [x] Average key width mode
- [x] Everything parameterized via config.yaml
- [x] Per-track / per-channel style overrides (with regex matching)
- [ ] Note history styles
- [ ] Current playing effects
- [ ] Lighting
    - [ ] Piano key lighting
    - [ ] Note history lighting
    - [ ] Line lighting
    - [ ] Bloom/glow
    - [ ] Particle effects
