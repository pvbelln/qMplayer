# MPlayer - Qt 音乐播放器课程项目

本项目使用 Qt Widgets / Qt Multimedia 实现一个桌面音乐播放器，支持本地音乐曲库、播放控制、歌词同步、歌单管理、音频波形、批量转码和 MV 播放等功能。

## 一、运行环境

**本源码只在 Windows 11 环境下编译并运行通过，不保证在其他 OS 下能运行成功。**

推荐运行环境：

- Windows 10/11
- CMake 3.19 或更高版本
- Qt 6.x MinGW 版本
- MinGW 64-bit 工具链

本项目开发时使用：

```text
Qt: D:/Qt/6.11.1/mingw_64
MinGW: D:/Qt/Tools/mingw1310_64
```

说明：Qt 6.11 不保证支持 Windows 7。若在 Windows 7 上运行，可能出现缺少 `GetCurrentPackageFullName` 等系统 API 的错误。建议使用 Windows 10/11 评分。

## 二、从源码编译

在项目根目录打开 PowerShell，执行以下命令。

```powershell
$env:Path='D:\Qt\6.11.1\mingw_64\bin;D:\Qt\Tools\mingw1310_64\bin;' + $env:Path
cmake -S . -B build -G "MinGW Makefiles" -DCMAKE_PREFIX_PATH=D:/Qt/6.11.1/mingw_64
cmake --build build
```

生成文件：

```text
build/MPlayer.exe
```

如果您本机 Qt 安装路径不同，请把上述命令中的 `D:/Qt/6.11.1/mingw_64` 和 MinGW 路径替换为实际路径。

## 三、运行方法

编译后可直接运行：

```powershell
.\build\MPlayer.exe
```

程序运行数据保存在 Qt 的 `QStandardPaths::AppDataLocation` 下，通常类似：

```text
C:\Users\<用户名>\AppData\Roaming\MPlayerProj\MPlayer\
```

其中保存曲库 JSON、歌单、歌词偏移和上次播放位置等数据。

## 四、FFmpeg 获取与放置

本项目的**音乐播放不依赖 FFmpeg**，播放由 Qt Multimedia 完成。  
只有“转码”功能需要外部 `ffmpeg.exe`。

评分者如需测试转码功能，请自行下载 Windows 版 FFmpeg：

```text
https://ffmpeg.org/download.html
```

下载 Windows 预编译包后，将其中的 `ffmpeg.exe` 放置为：

```text
项目根目录/ffmpeg/bin/ffmpeg.exe
```

也就是：

```text
Mplayer-proj/
  ffmpeg/
    bin/
      ffmpeg.exe
```

如果测试的是部署后的 exe，也可放在：

```text
MPlayer.exe 同级目录/ffmpeg/bin/ffmpeg.exe
```

程序查找顺序为：先查找 exe 同级目录下的 `ffmpeg/bin/ffmpeg.exe`，再查找源码项目根目录下的 `ffmpeg/bin/ffmpeg.exe`。

## 五、已实现的必做功能

- 扫描本地音乐文件夹，支持递归扫描。
- 支持 `mp3`、`flac`、`wav`、`m4a`、`aac`、`ogg` 等常见音频格式。
- 曲库信息使用 JSON 持久化保存，重启后自动恢复。
- 支持播放、暂停、停止、上一首、下一首。
- 支持进度条点击、拖动 seek，并实时显示当前时间和总时长。
- 支持音量调节和静音。
- 读取并显示歌曲标题、歌手、专辑、时长等元数据。
- 支持读取音乐文件内嵌专辑图片；没有封面时显示默认占位图。
- 支持搜索过滤，可按标题、歌手、专辑、文件名、标签查找。
- 支持三种播放模式：列表循环、单曲循环、随机播放。
- 支持 LRC 歌词解析、自动匹配同名歌词、手动加载歌词。
- 歌词支持同步高亮、平滑滚动、鼠标滚轮浏览、拖动浏览、点击歌词跳转。
- 支持歌词偏移 `-0.5s` / `+0.5s` 调整并保存。
- 支持程序启动时检测曲库文件是否仍可访问，并自动清理无效条目。
- 支持记忆上次播放歌曲和播放位置，下次启动恢复到该位置并暂停。

## 六、已实现的加分功能

- day/night 双主题，并适配对应图标和占位图。
- 自建歌单：新建、重命名、删除、添加歌曲。
- 分类浏览：按歌手、专辑、自定义标签组织曲库。
- 多选批量管理：`Shift` / `Ctrl` 多选歌曲，批量添加到歌单、从曲库移除、从当前歌单移除。
- 音频波形 / 频谱显示：使用 `QAudioDecoder` 解码 PCM 数据，计算分段能量并绘制动态波形。
- 批量转码：支持多文件队列、输出格式、码率、采样率、进度百分比、暂停和停止。
- MV 播放：独立视频播放窗口，支持打开本地视频、播放/暂停、停止、进度拖动和音量控制。
- 音乐与 MV 互斥播放：打开 MV 时自动暂停主界面音乐，关闭 MV 后恢复。
- 自定义 About / Help 窗口。
- 应用图标：通过 `resources/app.rc` 和 `assets/img/icon.ico` 设置 exe、窗口和任务栏图标。

## 七、设计说明

项目按功能拆分类和源文件。

- `Track` 保存单首歌曲的路径、标题、歌手、专辑、标签、时长、歌词路径和歌词偏移；
- `Playlist` 管理曲库列表和当前播放索引；`LibraryStore` 负责 JSON 持久化，包括曲库、用户歌单、播放快照和歌词偏移。
- 播放部分由 `AudioPlayer` 封装，它组合 `QMediaPlayer` 与 `QAudioOutput`，对外提供播放、暂停、seek、音量和元数据变化信号。
- 歌词部分由 `LrcParser`、`Lyrics` 和 `LyricsWidget` 组合完成，分别负责解析、存储和绘制交互。
- 主窗口实现也拆分为 `mainwindow_ui.cpp`、`mainwindow_library.cpp`、`mainwindow_playback.cpp`、`mainwindow_lyrics.cpp`、`mainwindow_waveform.cpp` 等文件，便于维护。
- **多态**主要用于播放模式：`PlaybackStrategy` 是抽象基类，列表循环、单曲循环和随机播放分别由不同子类实现，主窗口只依赖抽象接口取得下一首歌曲。
- **最大难点**是歌词交互和播放同步：用户滚轮或拖动歌词时不能被播放进度立即拉回，而正常播放时又要自动居中当前句。解决方法是区分播放中心行和用户浏览偏移，并在用户停止操作后用定时器自动回到当前歌词。

## 八、源码结构

```text
assets/
  img/
    icon.ico
    album-placeholder-day.jpg
    album-placeholder-night.jpg
    player-logo.svg
    player-icons/
    ui-icons/
resources/
  app.rc
src/
  main.cpp
  mainwindow*.cpp / mainwindow.h
  audioplayer.*
  playlist.*
  track.*
  librarystore.*
  playbackstrategy.*
  lrcparser.*
  lyrics.*
  lyricswidget.*
  waveformwidget.*
  transcoder.*
  transcodedialog.*
  mvplayerdialog.*
```

## 九、第三方项目和资源

- Qt Framework  
  用途：GUI、Widgets、多媒体播放、视频输出、JSON、路径和进程管理。  
  版权者：The Qt Company Ltd. and Qt contributors。  
  许可证：商业许可证 / LGPL / GPL，取决于使用和发布方式。  
  官网：`https://www.qt.io/`

- FFmpeg  
  用途：音频转码。  
  版权者：FFmpeg developers。  
  许可证：主要为 LGPL，部分构建可能包含 GPL 组件。  
  官网：`https://ffmpeg.org/`

- Player Icons  
  用途：播放、暂停、停止、上一首、下一首、循环、随机、音量等控制按钮。  
  版权者：Daniele De Santis。  
  许可证：CC BY 4.0。  
  来源：`https://www.iconarchive.com/artist/danieledesantis.html`

- UI Icons  
  用途：歌单、标签、帮助、关于、主题、MV、歌词、删除等界面图标。  
  版权者：Pictogrammers contributors。  
  许可证：Apache License 2.0 / MIT，详见 `assets/img/ui-icons/LICENSE.txt`。  
  官网：`https://pictogrammers.com/`

- 专辑占位图片  
  用途：当音乐文件没有内嵌专辑封面时显示。  
  来源：AI 生成图片。

## 十、建议测试流程

1. 编译并运行程序。
2. 点击 `Add Folder`，选择 `test-media` 或本地音乐文件夹。
3. 双击歌曲测试播放、暂停、停止、切歌、进度拖动和音量。
4. 测试搜索、歌单、标签、专辑/歌手分类。
5. 测试歌词自动匹配、手动加载、右键 Reload / Load、滚轮浏览和点击跳转。
6. 测试多选歌曲后右键批量添加到歌单、从曲库移除、从歌单移除。
7. 如已放置 FFmpeg，测试批量转码功能。
8. 测试 `Play MV` 打开视频文件，并确认音乐与 MV 不会同时播放。
