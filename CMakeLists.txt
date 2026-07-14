cmake_minimum_required(VERSION 3.19)

project(MPlayer VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

find_package(QT NAMES Qt6 Qt5 COMPONENTS Core Gui Widgets Multimedia REQUIRED)
find_package(Qt${QT_VERSION_MAJOR} COMPONENTS Core Gui Widgets Multimedia REQUIRED)

add_executable(${PROJECT_NAME}
    src/main.cpp
    src/audioplayer.cpp
    src/audioplayer.h
    src/librarystore.cpp
    src/librarystore.h
    src/lrcparser.cpp
    src/lrcparser.h
    src/lyrics.cpp
    src/lyrics.h
    src/lyricswidget.cpp
    src/lyricswidget.h
    src/mainwindow.cpp
    src/mainwindow.h
    src/playbackstrategy.cpp
    src/playbackstrategy.h
    src/playlist.cpp
    src/playlist.h
    src/seekslider.cpp
    src/seekslider.h
    src/track.cpp
    src/track.h
    src/transcodedialog.cpp
    src/transcodedialog.h
    src/transcoder.cpp
    src/transcoder.h
    src/waveformwidget.cpp
    src/waveformwidget.h
)

set_target_properties(${PROJECT_NAME} PROPERTIES
    WIN32_EXECUTABLE ON
)

target_link_libraries(${PROJECT_NAME} PRIVATE
    Qt${QT_VERSION_MAJOR}::Core
    Qt${QT_VERSION_MAJOR}::Gui
    Qt${QT_VERSION_MAJOR}::Widgets
    Qt${QT_VERSION_MAJOR}::Multimedia
)

target_compile_definitions(${PROJECT_NAME} PRIVATE
    PROJECT_ASSETS_DIR="${CMAKE_SOURCE_DIR}/assets"
    PROJECT_ROOT_DIR="${CMAKE_SOURCE_DIR}"
)
