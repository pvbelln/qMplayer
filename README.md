# MPlayer - Qt Desktop Music Player

MPlayer is a Qt desktop music player built for the course project requirements. It supports local library scanning, playback controls, metadata display, synchronized LRC lyrics, lyric seeking interaction, playback modes, search/filtering, user playlists, tag/category organization, FFmpeg transcoding, and styled day/night themes.

## Build And Run

Requirements:

- Qt 6 with `Widgets` and `Multimedia`
- MinGW toolchain
- CMake 3.19+

This project is configured for the local Qt installation in `.vscode/settings.json`:

```powershell
$env:Path='D:\Qt\6.11.1\mingw_64\bin;D:\Qt\Tools\mingw1310_64\bin;' + $env:Path
cmake -S . -B build -G "MinGW Makefiles" -DCMAKE_PREFIX_PATH=D:/Qt/6.11.1/mingw_64
cmake --build build
.\build\MPlayer.exe
```

Runtime data is stored with `QStandardPaths::AppDataLocation`, usually:

```text
C:\Users\ZXQ\AppData\Roaming\MPlayerProj\MPlayer\library.json
```

## Implemented Features

- Recursive music scanning for `mp3`, `flac`, `wav`, `m4a`, `aac`, and `ogg`.
- Persistent library saved as JSON and loaded on startup.
- Playback controls: play, pause, stop, previous, next, seek slider, time display, volume, mute.
- Audio visualization: `QAudioDecoder` decodes PCM samples for the current song, computes per-segment energy, and draws a live highlighted waveform above the seek slider.
- Metadata display from `QMediaMetaData`: title, artist, album, embedded cover image.
- Fallback cover image from `assets/img/album-placeholder-day.jpg` or `assets/img/album-placeholder-night.jpg`.
- LRC lyrics loading by matching the audio filename with a same-name `.lrc` file.
- Manual LRC selection from the lyrics area by right-clicking or using the `Load LRC File` button when no lyrics are found; selected LRC paths are persisted per song.
- LRC parsing with timestamps, multiple time tags per line, `[offset:]`, UTF-8 and system encoding fallback.
- Synchronized lyrics with current line highlight and `+/- 0.5s` persistent lyric offset adjustment.
- Lyric browsing by mouse drag and wheel scrolling.
- Lyric jump by clicking the left jump button or the centered lyric line; cursor changes to a pointing hand over jump areas.
- Automatic lyric return to current playback line after 3 seconds without action.
- Playback modes: list loop, single loop, shuffle.
- Realtime search/filter by title, artist, album, or filename.
- User playlists and category organization: create playlists, add songs to playlists, filter by artist, album, or custom tags, with playlists and tags persisted in JSON.
- Enhanced asynchronous transcoding through `ffmpeg/bin/ffmpeg.exe`: batch queue, selectable output format, bitrate, sample rate, current and total progress percentage, stop, and Windows pause/resume.
- Day/night QSS themes with matching icon buttons, placeholders, and tooltips.

## FFmpeg

Playback uses Qt Multimedia. Transcoding uses the bundled executable:

```text
ffmpeg/bin/ffmpeg.exe
```

The program first looks for `ffmpeg/bin/ffmpeg.exe` beside the executable, then falls back to the project root folder. The current repository already includes `ffmpeg/bin/ffmpeg.exe`.

## Credits And Licenses

- Author: Xuqi Zhu.
- Qt Framework: used for the GUI, multimedia playback, JSON persistence, and process management. Qt is available under open-source licenses including LGPL/GPL, and commercial licenses.
- FFmpeg: used for transcoding through `ffmpeg/bin/ffmpeg.exe`. The bundled FFmpeg license file is `ffmpeg/LICENSE`, currently GNU GPL v3.
- Player icons: original icon set by Daniele De Santis from IconArchive, licensed under CC BY 4.0. Modified by Xuqi Zhu for the single-loop icon and day/night theme variants. See `assets/img/player-icons/LICENSE`.
- UI icons: top action icons and day/night switch icons from Pictogrammers. See `assets/img/ui-icons/LICENSE.txt`.
- Album placeholder images: AI-generated images used when no embedded album cover is found.

## Design Notes

The code is split by responsibility instead of putting all behavior in `MainWindow`. `Track` stores one song's data such as path, title, artist, album, tags, duration, and lyric offset. `Playlist` manages the track list and current index. `LibraryStore` owns JSON persistence through `QStandardPaths::AppDataLocation`, including tracks, custom playlists, tags, playback snapshot, and lyric offsets. `AudioPlayer` wraps `QMediaPlayer` and `QAudioOutput`, exposing clean playback, volume, seek, metadata, and state signals to the UI. `Lyrics` stores ordered lyric lines, `LrcParser` parses `.lrc` files, and `LyricsWidget` paints and handles lyric synchronization, dragging, wheel browsing, center-line jumping, and auto return. `Transcoder` wraps asynchronous `QProcess` FFmpeg execution so the UI stays responsive.

Polymorphism is used in playback mode selection. `PlaybackStrategy` is an abstract interface, with `ListLoopStrategy`, `SingleLoopStrategy`, and `ShuffleStrategy` implementing different next-track behavior. `MainWindow` only keeps a pointer to the active strategy and asks it for the next index, so adding a new mode does not require rewriting playback control flow. The largest challenge was lyric interaction: the widget must separate the currently playing line from the browsed center line, pause auto-follow while browsing, and return automatically without blocking playback.

## Test Media

The `test-media` folder contains sample audio and matching `.lrc` files. A normal test flow is:

1. Run the app and click `Add Folder`.
2. Select `test-media`.
3. Double-click a song.
4. Test playback controls, search, playback modes, lyrics, lyric offset, drag/wheel lyric browsing, lyric jump, and batch transcoding options.

## Project Structure

```text
assets/img/album-placeholder-day.jpg
assets/img/album-placeholder-night.jpg
assets/img/player-icons/day/*.png
assets/img/player-icons/night/*.png
assets/img/ui-icons/day/*.png
assets/img/ui-icons/night/*.png
ffmpeg/bin/ffmpeg.exe
src/
  audioplayer.*
  librarystore.*
  lrcparser.*
  lyrics.*
  lyricswidget.*
  main.cpp
  mainwindow.*
  playbackstrategy.*
  playlist.*
  track.*
  transcoder.*
```
