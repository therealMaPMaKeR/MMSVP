# Lightweight Video Player (MMSVP)

A simple, lightweight video player built with Qt 6.5.3 and VLC, based on the video player from MMDiary.

## Features

- **Core Playback Controls**: Play, pause, stop
- **Seeking**: Click anywhere on the progress bar to jump to that position
- **Volume Control**: Volume slider with support up to 200%
- **Playback Speed**: Adjustable from 0.1x to 5.0x
- **Keyboard Shortcuts**:
  - `Space`: Play/Pause
  - `Left Arrow`: Seek backward 10 seconds
  - `Right Arrow`: Seek forward 10 seconds
  - `Up Arrow`: Increase volume by 5%
  - `Down Arrow`: Decrease volume by 5%
  - `Mouse Wheel`: Adjust volume
- **Double-click video**: Toggle play/pause

## Project Structure

```
SimpleVideoPlayer/
├── vp_vlcplayer.h/cpp           # VLC wrapper (handles VLC integration)
├── lightweightvideoplayer.h/cpp # Main video player widget
├── main.cpp                     # Application entry point
└── 3rdparty/libvlc/            # VLC libraries (you need to add these)
```

## Setup Instructions

### 1. Install VLC Libraries

You need to copy VLC libraries to the `3rdparty/libvlc` directory:

```
3rdparty/libvlc/
├── bin/
│   ├── libvlc.dll
│   ├── libvlccore.dll
│   └── plugins/          # All VLC plugin folders
├── lib/
│   ├── libvlc.lib
│   └── libvlccore.lib
└── include/
    └── vlc/              # VLC header files
```

**Where to get VLC libraries:**
1. Download VLC from: https://www.videolan.org/vlc/
2. Install VLC or extract the ZIP
3. Copy the files from the VLC installation directory to `3rdparty/libvlc`

### 2. Build the Project

1. Open `SimpleVideoPlayer.pro` in Qt Creator
2. Configure the project with MSVC2019 compiler
3. Build and run

## What Was Removed from MMDiary's BaseVideoPlayer

This lightweight version removes:
- Encryption/decryption features
- Fullscreen mode
- Advanced screen/monitor management
- Database integration
- User settings persistence
- Authentication/password features
- Mute button (you can add it back easily if needed)
- Complex state management
- Windows shutdown handling

## What Was Kept

✅ VLC integration (vp_vlcplayer wrapper)
✅ Play/pause/stop controls
✅ Clickable progress slider
✅ Volume control (0-200%)
✅ Playback speed control (0.1x-5.0x)
✅ Time display (current/duration)
✅ Keyboard shortcuts
✅ Mouse wheel volume control
✅ Error handling
✅ Event system (signals/slots)

## Customization

### Adding More Features

The code is well-structured for adding features:

1. **Add Fullscreen**: Copy the fullscreen methods from MMDiary's BaseVideoPlayer
2. **Add Mute Button**: Uncomment the mute button code in the controls
3. **Add Playlist**: Create a playlist widget and connect it
4. **Add File Menu**: Add QMenuBar with file operations

### Keyboard Shortcuts

Edit `LightweightVideoPlayer::keyPressEvent()` to add more shortcuts.

### UI Customization

Edit `createControls()` and `createLayouts()` to modify the interface.

## Architecture

```
LightweightVideoPlayer (Main Widget)
    ├── VP_VLCPlayer (VLC Wrapper)
    │   └── libvlc (VLC Library)
    ├── Video Widget (Rendering)
    └── Controls Widget
        ├── Play/Pause Button
        ├── Stop Button
        ├── Position Slider
        ├── Volume Slider
        └── Speed Spinbox
```

## Differences from Original videoplayer.cpp

Your original `videoplayer.cpp` was a basic VLC wrapper. The new implementation:

- ✅ Better separation of concerns (VP_VLCPlayer handles VLC, LightweightVideoPlayer handles UI)
- ✅ More robust error handling
- ✅ Cleaner signal/slot architecture
- ✅ Clickable sliders (not just draggable)
- ✅ Better state management
- ✅ More features (playback speed, extended volume range)
- ✅ Keyboard shortcuts built-in

## Building Tips

1. **Clean Build**: If you get link errors, clean and rebuild
2. **VLC Plugins**: Make sure the plugins folder is copied correctly
3. **Debug Output**: Check Qt Creator's output for VLC initialization messages

## License

Based on MMDiary's video player component. Check your MMDiary license for usage terms.

## Support

Check the debug output in Qt Creator for detailed VLC initialization and playback information.
