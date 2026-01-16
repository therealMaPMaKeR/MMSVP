# Simple Video Player

A basic video player application built with Qt 6.5.3 and libVLC.

## Setup Instructions

### 1. Copy libVLC Files

You need to manually copy the libVLC files from your MMDiary project to this project:

**Copy the following directories:**
- From: `MMDiary/3rdparty/libvlc/bin/`
- To: `SimpleVideoPlayer/3rdparty/libvlc/bin/`

- From: `MMDiary/3rdparty/libvlc/lib/`
- To: `SimpleVideoPlayer/3rdparty/libvlc/lib/`

- From: `MMDiary/3rdparty/libvlc/include/`
- To: `SimpleVideoPlayer/3rdparty/libvlc/include/`

**Required files structure:**
```
SimpleVideoPlayer/
├── 3rdparty/
│   └── libvlc/
│       ├── bin/
│       │   ├── libvlc.dll
│       │   ├── libvlccore.dll
│       │   └── plugins/ (entire directory)
│       ├── lib/
│       │   ├── libvlc.lib
│       │   └── libvlccore.lib
│       └── include/
│           └── vlc/ (entire directory with headers)
```

### 2. Build and Run

1. Open `SimpleVideoPlayer.pro` in Qt Creator
2. Configure the project with MSVC2019 compiler
3. Build the project (Debug or Release)
4. Run the application

## Features

- Open video files (MP4, AVI, MKV, MOV, WMV, FLV, WebM)
- Play/Pause controls
- Stop button
- Seek slider for video position
- Volume control
- Time display (current time / total duration)

## Usage

1. Click "Open File" to select a video file
2. Use "Play" to start playback
3. Use "Pause" to pause playback
4. Use "Stop" to stop playback
5. Drag the seek slider to jump to different positions
6. Adjust the volume slider to change audio volume

## Technical Details

- Built with Qt 6.5.3
- Uses libVLC for video playback
- MSVC2019 compiler
- Windows platform

## Notes

- The .pro file is configured to automatically copy libVLC DLLs and plugins to the build output directory
- Make sure all libVLC files are present before building
- The video player widget is embedded directly in the main window
