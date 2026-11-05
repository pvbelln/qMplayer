# qMplayer

A desktop music player built with **C++17, Qt 6 Widgets, and Qt Multimedia**. Manage your local music library, follow synchronized lyrics, create playlists, switch themes, visualize audio, transcode files, and play local music videos.

Developed by **Xuqi Zhu**, qMplayer began as a course project. The application and project documentation are in English. A portable Windows x64 package is provided for release.

## Features

| Area | Features |
| --- | --- |
| Music library | Recursively scan folders; read titles, artists, albums, duration, and embedded artwork; persist the library as JSON |
| Playback | Play, pause, stop, skip tracks, seek, adjust volume, and mute |
| Playback modes | Repeat all, repeat one, and shuffle |
| Organization | Search by title, artist, album, filename, or tag; create and manage playlists; edit multiple selections |
| LRC lyrics | Match same-name lyric files, load lyrics manually, highlight the current line, scroll, seek by clicking, and save timing offsets |
| Appearance | Day and night themes, album artwork, and audio waveform visualization |
| Batch transcoding | Queue files, select output formats, bitrates, and sample rates, and monitor, pause, or stop conversion; requires a separate FFmpeg executable |
| Local music videos | Play videos in a separate window with seek and volume controls; pause music during video playback and resume it if it was previously playing |
| Session restore | Save the library, playlists, lyric offsets, and playback position; restore playback in a paused state |

The library scanner recognizes `mp3`, `flac`, `wav`, `m4a`, `aac`, and `ogg` files. Actual decoding support depends on the Qt Multimedia backend and the media encoding.

## Download and run

Download `qMplayer-v1.0.0-windows-x64.zip` from this repository's **Releases** page, extract the complete archive, and launch `qMplayer.exe`.

Keep the DLLs, plugin folders, and `assets` next to the executable. A separate Qt installation is not required. The package is built on Windows 11 x64 with Qt 6.11.1 and MinGW 13.1.0; other Windows versions have not been verified.

```text
qMplayer-v1.0.0-windows-x64/
|-- qMplayer.exe
|-- Qt6*.dll               # Qt, compiler, and multimedia runtime libraries
|-- platforms/             # Windows platform plugin
|-- multimedia/            # Media backend plugins
|-- imageformats/          # Image format plugins
|-- assets/                # Icons, theme images, and resource notices
|-- licenses/              # Third-party license material from the build environment
|-- README.md
|-- RELEASE_NOTES.md
`-- THIRD_PARTY_NOTICES.md
```

### Quick start

1. Click **Add Folder** to import a local music folder.
2. Double-click a song or press `Enter` to play it. Use the playback bar to seek, adjust volume, or change playback mode.
3. Use the library selector to browse playlists, artists, albums, or tags. Right-click songs for management actions.
4. Place a matching `.lrc` file beside the music file, or click **Load LRC File** to select lyrics manually.
5. Click **Play MV** to open a local video. Use the transcoding dialog to convert audio files after configuring FFmpeg.

### Keyboard shortcuts

| Shortcut | Action |
| --- | --- |
| `Space` | Play / pause |
| `Left` / `Right` | Seek backward / forward by 5 seconds; hold to continue seeking |
| `Up` / `Down` | Adjust volume |
| `PgUp` / `PgDn` | Previous / next track |
| `Enter` | Play the selected song |
| `Esc` | Clear search |
| `Ctrl` / `Shift` + click | Select multiple songs |

## Optional: enable batch transcoding

Get a Windows build through the [FFmpeg download page](https://ffmpeg.org/download.html) and place the standalone command-line executable at:

```text
<directory containing qMplayer.exe>/ffmpeg/bin/ffmpeg.exe
```

During development, `<project root>/ffmpeg/bin/ffmpeg.exe` is also supported. The application checks the executable's directory first, then the source directory.

The release package includes the media libraries needed by Qt's playback backend. It does not include the separate `ffmpeg.exe` used for batch transcoding. Music and video playback use Qt Multimedia.

## Build from source

Install CMake 3.19 or newer, Ninja, a Qt 6 MinGW 64-bit kit, and its matching MinGW toolchain. The required Qt modules are Core, Gui, Widgets, Multimedia, and MultimediaWidgets.

Run PowerShell from the project root, adjusting the installation paths for your machine:

```powershell
$qtDir = 'D:\Qt\6.11.1\mingw_64'
$mingwDir = 'D:\Qt\Tools\mingw1310_64'
$env:Path = "$qtDir\bin;$mingwDir\bin;" + $env:Path

cmake -S . -B build/release -G Ninja `
  -DCMAKE_BUILD_TYPE=Release `
  "-DCMAKE_PREFIX_PATH=$qtDir" `
  "-DCMAKE_CXX_COMPILER=$mingwDir/bin/g++.exe"
cmake --build build/release --parallel 4
.\build\release\qMplayer.exe
```

Keep Qt's `bin` directory in `PATH` when running the development build. The project uses Qt 6 APIs.

### Build the Windows release package

```powershell
powershell -ExecutionPolicy Bypass -File scripts/package-windows.ps1 `
  -QtDir 'D:\Qt\6.11.1\mingw_64' `
  -MingwDir 'D:\Qt\Tools\mingw1310_64'
```

The script builds the Release target, installs the application and assets, deploys dependencies with `windeployqt`, and creates a ZIP archive and a SHA-256 checksum file:

```text
release/qMplayer-v1.0.0-windows-x64/
release/qMplayer-v1.0.0-windows-x64.zip
release/qMplayer-v1.0.0-windows-x64.zip.sha256
```

Release assets follow `<product>-v<major>.<minor>.<patch>-<os>-<architecture>.zip`, with lowercase `os` and architecture identifiers. For this release, use Git tag `v1.0.0` and title **qMplayer v1.0.0**. The ready-to-paste release description is in [RELEASE_NOTES.md](RELEASE_NOTES.md).

The output folder and archive must not already exist. To package another copy, choose a fresh output location with `-OutputDir release-next`. Upload the ZIP and its `.sha256` companion to GitHub Releases. Local `build/` and `release/` output and test media are excluded from Git.

Verify a downloaded archive in PowerShell:

```powershell
(Get-FileHash .\qMplayer-v1.0.0-windows-x64.zip -Algorithm SHA256).Hash
Get-Content .\qMplayer-v1.0.0-windows-x64.zip.sha256
```

Compare the computed digest with the first field in the checksum file; letter case does not affect the comparison.

## Application data

The library, playlists, lyric offsets, and playback snapshot are stored at:

```text
%APPDATA%\qMplayer\qMplayer\library.json
```

Theme preferences use `QSettings`. On Windows they are stored under the current user's registry key:

```text
HKEY_CURRENT_USER\Software\qMplayer\qMplayer
```

This version uses the qMplayer application identity. Data from earlier application identities is not migrated automatically. To retain an existing library, back up its `library.json`, close the application, and copy the file to the directory above. The referenced media files must remain accessible at their original paths.

The portable package runs without an installer; application data is still stored in the system user profile.

## Project structure

```text
assets/                       Icons, placeholder artwork, and theme resources
resources/app.rc              Windows icon and version metadata
scripts/package-windows.ps1   Windows release packaging script
src/
  main.cpp                    Application entry point
  mainwindow*.cpp/.h          Main window, library, playback, lyrics, and theme logic
  audioplayer.*               Qt Multimedia audio wrapper
  librarystore.*              JSON persistence
  track.* / playlist.*        Track and playback-list models
  playbackstrategy.*          Repeat-all, repeat-one, and shuffle strategies
  lrcparser.* / lyrics.* / lyricswidget.*  Lyric parsing and interaction
  waveformwidget.*            Audio waveform display
  transcoder.* / transcodedialog.*        Transcoding backend and queue UI
  mvplayerdialog.*            Local video player
```

## Testing and feedback

Before publishing, check library import and search, playback and seeking, lyric synchronization and offsets, playlists and tags, theme switching, session restore, music/video playback coordination, and optional batch transcoding with your own media. A successful build and startup check do not establish compatibility with every codec or audio device.

When opening an issue, include your operating system, qMplayer version, reproduction steps, expected and actual behavior, media format, and relevant screenshots or error messages.

## Credits and licensing

Author: **Xuqi Zhu**. Third-party components and assets belong to their respective authors. See [THIRD_PARTY_NOTICES.md](THIRD_PARTY_NOTICES.md) and the license notices in the asset directories.

The project's own source code does not currently specify an open-source license. Third-party components remain subject to their respective licenses.
