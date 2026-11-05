# qMplayer Third-Party Notices

The applicable terms for each component depend on its actual version, build configuration, and original license files.

| Component / asset | Purpose | Source and license information |
| --- | --- | --- |
| Qt Framework | Widgets UI, media playback, JSON, file handling, and process management | The Qt Company Ltd. and Qt contributors; [Qt](https://www.qt.io/), [licensing](https://www.qt.io/licensing/). The application links dynamically. The packaging script copies available license material and SBOM files from the Qt installation. |
| FFmpeg | Qt media backend runtime libraries and optional standalone audio transcoding | FFmpeg developers; [website](https://ffmpeg.org/), [license information](https://ffmpeg.org/legal.html). Terms depend on the build configuration. The standalone `ffmpeg.exe` is not bundled. |
| Player Control Icons | Playback, pause, track navigation, repeat, and volume controls | Daniele De Santis; [source](https://www.iconarchive.com/artist/danieledesantis.html), CC BY 4.0. Xuqi Zhu added or modified repeat-one and night-theme variants. See `assets/img/player-icons/LICENSE`. |
| UI Icons | Folder, playlist, tag, help, theme, and video controls | Pictogrammers contributors; [source](https://pictogrammers.com/). See `assets/img/ui-icons/LICENSE.txt`. |
| Placeholder album artwork | Default artwork for tracks without embedded covers | Existing AI-generated project assets. |
| MinGW-w64 / GCC runtime | Windows C++ runtime libraries | Deployed with the Qt MinGW toolchain. The packaging script copies available toolchain license files. |

The release retains the original attribution files under `assets` and includes license material supplied by the build environment under `licenses`. The project's own source code does not currently specify an open-source license.
