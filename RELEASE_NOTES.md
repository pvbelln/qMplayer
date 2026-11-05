# qMplayer v1.0.0

qMplayer is a desktop music player for your local music collection, built with C++17 and Qt 6. This release provides a portable Windows x64 package with an English interface and documentation.

## Highlights

- Local music library with recursive folder scanning, metadata, album artwork, search, and JSON persistence.
- Playback controls with seeking, volume, mute, repeat-all, repeat-one, and shuffle modes.
- Playlists, artist and album browsing, custom tags, and multi-select library management.
- Synchronized LRC lyrics with automatic matching, manual loading, click-to-seek, and saved timing offsets.
- Day and night themes and audio waveform visualization.
- Batch audio transcoding with configurable output formats, bitrates, sample rates, and queue controls; requires a separate FFmpeg executable.
- Local music video playback that pauses the main audio player and restores its previous playback state when the video window closes.
- Saved library, playlists, lyric offsets, and playback position, with playback restored in a paused state.

## Download

| Asset | Description |
| --- | --- |
| `qMplayer-v1.0.0-windows-x64.zip` | Portable Windows x64 application, Qt runtime libraries, plugins, assets, and documentation |
| `qMplayer-v1.0.0-windows-x64.zip.sha256` | SHA-256 checksum for the ZIP archive |

Extract the entire ZIP archive and run **qMplayer.exe**. Keep the bundled DLLs, plugin folders, and assets beside the executable. No separate Qt installation is required.

Built on **Windows 11 x64** using **Qt 6.11.1** and **MinGW 13.1.0**. Other Windows versions have not been verified.

## Optional FFmpeg setup

For batch transcoding, place a standalone Windows FFmpeg executable at:

```text
<directory containing qMplayer.exe>/ffmpeg/bin/ffmpeg.exe
```

The standalone transcoder is not included. Qt's multimedia playback libraries are included in the package.

## Data and compatibility

- Library data is stored at `%APPDATA%\qMplayer\qMplayer\library.json`.
- Theme preferences use `HKEY_CURRENT_USER\Software\qMplayer\qMplayer`.
- Data from earlier application identities is not migrated automatically. Back up an existing `library.json`, close the application, and copy it to the path above if needed; media paths must remain valid.
- The scanner recognizes MP3, FLAC, WAV, M4A, AAC, and OGG files. Decoding support depends on the media backend and file encoding.

## Verification

The Release build, archive integrity, packaged dependencies, English-language project text, and startup from an extracted package with Qt development paths removed were checked. The application opened with the qMplayer window title and exited normally.

Playback across all codecs and devices, interactive lyric and playlist workflows, video playback, and optional transcoding were not exhaustively tested in this release check.

Developed by **Xuqi Zhu**. See the included `THIRD_PARTY_NOTICES.md` for component and asset credits.
