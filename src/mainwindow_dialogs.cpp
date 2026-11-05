#include "mainwindow.h"

#include "lrcparser.h"
#include "seekslider.h"
#include "lyricswidget.h"
#include "mvplayerdialog.h"
#include "transcodedialog.h"
#include "waveformwidget.h"

#include <algorithm>
#include <QAbstractItemView>
#include <QAudioBuffer>
#include <QAudioDecoder>
#include <QAudioFormat>
#include <QDirIterator>
#include <QApplication>
#include <QCloseEvent>
#include <QComboBox>
#include <QCoreApplication>
#include <QDesktopServices>
#include <QDialog>
#include <QFileDialog>
#include <QFileInfo>
#include <QFrame>
#include <QIcon>
#include <QImage>
#include <QEventLoop>
#include <QInputDialog>
#include <QKeyEvent>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QMenu>
#include <QMediaMetaData>
#include <QMediaPlayer>
#include <QMessageBox>
#include <QPixmap>
#include <QPushButton>
#include <QSettings>
#include <QSizePolicy>
#include <QSlider>
#include <QSplitter>
#include <QSet>
#include <QStyle>
#include <QTextEdit>
#include <QToolButton>
#include <QTimer>
#include <QUrl>
#include <QVBoxLayout>
#include <QScrollBar>
#include <QHBoxLayout>
#include <QtMath>

void MainWindow::showAboutDialog()
{
    QDialog dialog(this);
    dialog.setWindowTitle("About qMplayer");
    dialog.setModal(true);
    dialog.resize(760, 520);

    const QString panelColor = nightMode ? "#171b24" : "#ffffff";
    const QString textColor = nightMode ? "#edf2f7" : "#172033";
    const QString mutedColor = nightMode ? "#aeb8c8" : "#61718a";
    const QString fieldColor = nightMode ? "#10141d" : "#f7fafc";
    const QString borderColor = nightMode ? "#303847" : "#d9e2ef";
    const QString buttonBg = nightMode ? "#242d3b" : "#f3f7fb";
    const QString buttonHover = nightMode ? "#303b4f" : "#e8f1fb";
    const QString accent = nightMode ? "#8fb3ff" : "#1f6f78";

    dialog.setStyleSheet(QString(R"(
        QDialog {
            background: %1;
            color: %2;
            font-family: "Segoe UI", "Microsoft YaHei UI", sans-serif;
            font-size: 13px;
        }
        QLabel {
            background: transparent;
            color: %2;
        }
        QLabel#aboutTitle {
            font-size: 28px;
            font-weight: 800;
        }
        QLabel#aboutSubtitle, QLabel#aboutMeta {
            color: %3;
            line-height: 140%;
        }
        QLabel#aboutSection {
            color: %2;
            font-size: 14px;
            font-weight: 800;
        }
        QLabel#logoPanel {
            background: %5;
            border: 1px solid %4;
            border-radius: 18px;
            color: %3;
            font-size: 22px;
            font-weight: 800;
        }
        QTextEdit#aboutText {
            background: %8;
            border: 1px solid %4;
            border-radius: 12px;
            color: %2;
            padding: 12px;
            line-height: 145%;
            selection-background-color: %7;
        }
        QTextEdit#aboutText QScrollBar:vertical {
            background: transparent;
            border: none;
            width: 10px;
            margin: 6px 2px 6px 0;
        }
        QTextEdit#aboutText QScrollBar::handle:vertical {
            background: %4;
            border-radius: 5px;
            min-height: 28px;
        }
        QTextEdit#aboutText QScrollBar::add-line:vertical,
        QTextEdit#aboutText QScrollBar::sub-line:vertical,
        QTextEdit#aboutText QScrollBar::add-page:vertical,
        QTextEdit#aboutText QScrollBar::sub-page:vertical {
            background: transparent;
            border: none;
            height: 0;
        }
        QPushButton {
            background: %5;
            border: 1px solid %4;
            border-radius: 10px;
            color: %2;
            padding: 8px 18px;
            font-weight: 700;
        }
        QPushButton:hover {
            background: %6;
            border-color: %7;
        }
    )").arg(panelColor, textColor, mutedColor, borderColor, buttonBg, buttonHover, accent, fieldColor));

    auto *root = new QVBoxLayout(&dialog);
    root->setContentsMargins(24, 24, 24, 20);
    root->setSpacing(18);

    auto *content = new QHBoxLayout;
    content->setSpacing(22);

    auto *logoLabel = new QLabel(&dialog);
    logoLabel->setObjectName("logoPanel");
    logoLabel->setFixedSize(164, 164);
    logoLabel->setAlignment(Qt::AlignCenter);
    const QPixmap logo(assetPath("img/player-logo.svg"));
    if (!logo.isNull()) {
        logoLabel->setPixmap(logo.scaled(124, 124, Qt::KeepAspectRatio, Qt::SmoothTransformation));
    } else {
        logoLabel->setText("qMplayer");
    }

    auto *left = new QVBoxLayout;
    left->setSpacing(10);
    left->addWidget(logoLabel, 0, Qt::AlignLeft);
    auto *title = new QLabel("qMplayer", &dialog);
    title->setObjectName("aboutTitle");
    auto *subtitle = new QLabel("Qt Desktop Music Player", &dialog);
    subtitle->setObjectName("aboutSubtitle");
    auto *author = new QLabel("Author\nDavid Zhu", &dialog);
    author->setObjectName("aboutSection");
    auto *version = new QLabel(QString("Version %1\nCourse Project").arg(QCoreApplication::applicationVersion()), &dialog);
    version->setObjectName("aboutMeta");
    left->addWidget(title);
    left->addWidget(subtitle);
    left->addSpacing(8);
    left->addWidget(author);
    left->addWidget(version);
    left->addStretch();

    auto *aboutText = new QTextEdit(&dialog);
    aboutText->setObjectName("aboutText");
    aboutText->setReadOnly(true);
    aboutText->setAcceptRichText(false);
    aboutText->setLineWrapMode(QTextEdit::WidgetWidth);
    aboutText->setText(
        "qMplayer\n"
        "========\n\n"
        "Program Overview\n"
        "----------------\n"
        "qMplayer is a Qt-based desktop music player developed by David Zhu. It focuses on local media playback and library management, including folder scanning, metadata display, playlists, artist/album/tag organization, synchronized LRC lyrics, day/night themes, audio waveform visualization, FFmpeg-based transcoding, and local MV video playback.\n\n"
        "This application is a course project and is intended for learning, demonstration, and personal desktop use.\n\n"
        "Project Assets\n"
        "----------------\n\n"
        "1. Qt Framework\n"
        "Purpose: GUI framework, widgets, multimedia playback, video output, JSON/storage helpers, process integration, and core application infrastructure.\n"
        "Copyright holder: The Qt Company Ltd. and Qt contributors.\n"
        "License: Commercial License, GNU LGPL v3, and GNU GPL v3 options depending on distribution and usage.\n"
        "Website: https://www.qt.io/\n"
        "Licensing information: https://www.qt.io/licensing/\n\n"
        "2. FFmpeg\n"
        "Purpose: External command-line transcoding backend used for audio format conversion.\n"
        "Copyright holder: FFmpeg developers.\n"
        "License: FFmpeg is primarily distributed under GNU LGPL v2.1 or later, with optional components under GNU GPL depending on build configuration.\n"
        "Website: https://ffmpeg.org/\n"
        "Legal and license information: https://ffmpeg.org/legal.html\n\n"
        "3. Player Control Icons\n"
        "Purpose: Play, pause, stop, previous/next, loop, shuffle, audio, and related playback controls.\n"
        "Copyright holder: Daniele De Santis.\n"
        "Source: IconArchive artist page.\n"
        "License: Creative Commons Attribution 4.0 International (CC BY 4.0).\n"
        "Website: https://www.iconarchive.com/artist/danieledesantis.html\n"
        "License text: https://creativecommons.org/licenses/by/4.0/\n"
        "Project note: David Zhu modified or added selected variants, including the single-loop icon and night theme icon variants.\n\n"
        "4. UI Icons\n"
        "Purpose: Folder, playlist, tag, help, about, theme, MV, lyric, timer, and related interface icons.\n"
        "Copyright holder: Pictogrammers contributors.\n"
        "License: Apache License 2.0 for icons and fonts, with MIT License for non-font and non-icon code files, as described in assets/img/ui-icons/LICENSE.txt.\n"
        "Website: https://pictogrammers.com/\n"
        "Apache License 2.0: https://www.apache.org/licenses/LICENSE-2.0\n"
        "MIT License: https://opensource.org/licenses/MIT\n\n"
        "License Notice\n"
        "--------------\n"
        "This About page summarizes third-party components used by the application. Detailed license terms are governed by each upstream project's official license documents. If the application is redistributed, the corresponding license texts and required attribution notices should be included with the distribution package.\n");

    content->addLayout(left);
    content->addWidget(aboutText, 1);
    root->addLayout(content);

    auto *buttons = new QHBoxLayout;
    auto *aboutQtButton = new QPushButton("About Qt", &dialog);
    connect(aboutQtButton, &QPushButton::clicked, &dialog, [&dialog]() {
        QMessageBox::aboutQt(&dialog, "About Qt");
    });
    buttons->addWidget(aboutQtButton);
    buttons->addStretch();
    auto *closeButton = new QPushButton("Close", &dialog);
    connect(closeButton, &QPushButton::clicked, &dialog, &QDialog::accept);
    buttons->addWidget(closeButton);
    root->addLayout(buttons);

    dialog.exec();
}
void MainWindow::showHelpDialog()
{
    QDialog dialog(this);
    dialog.setWindowTitle("Help");
    dialog.setModal(true);
    dialog.resize(500, 330);

    const QString panelColor = nightMode ? "#171b24" : "#ffffff";
    const QString textColor = nightMode ? "#edf2f7" : "#172033";
    const QString mutedColor = nightMode ? "#aeb8c8" : "#61718a";
    const QString cardColor = nightMode ? "#1f2530" : "#f7fafc";
    const QString borderColor = nightMode ? "#303847" : "#d9e2ef";
    const QString accent = nightMode ? "#8fb3ff" : "#1f6f78";
    const QString buttonHover = nightMode ? "#303b4f" : "#e8f1fb";

    dialog.setStyleSheet(QString(R"(
        QDialog {
            background: %1;
            color: %2;
            font-family: "Segoe UI", "Microsoft YaHei UI", sans-serif;
            font-size: 13px;
        }
        QLabel {
            background: transparent;
            color: %2;
        }
        QLabel#helpTitle {
            font-size: 24px;
            font-weight: 800;
        }
        QLabel#helpHint {
            color: %3;
        }
        QLabel#helpCard {
            background: %4;
            border: 1px solid %5;
            border-radius: 12px;
            padding: 12px;
            line-height: 145%;
        }
        QLabel#helpCardTitle {
            color: %6;
            font-weight: 800;
            font-size: 14px;
        }
        QPushButton {
            background: %4;
            border: 1px solid %5;
            border-radius: 10px;
            color: %2;
            padding: 8px 18px;
            font-weight: 700;
        }
        QPushButton:hover {
            background: %7;
            border-color: %6;
        }
    )").arg(panelColor, textColor, mutedColor, cardColor, borderColor, accent, buttonHover));

    auto *root = new QVBoxLayout(&dialog);
    root->setContentsMargins(24, 22, 24, 20);
    root->setSpacing(12);

    auto *title = new QLabel("Help", &dialog);
    title->setObjectName("helpTitle");
    auto *hint = new QLabel("Core operations and shortcuts.", &dialog);
    hint->setObjectName("helpHint");
    root->addWidget(title);
    root->addWidget(hint);

    auto *opsTitle = new QLabel("Operations", &dialog);
    opsTitle->setObjectName("helpCardTitle");
    auto *ops = new QLabel(
        "Add Folder scans music. Use the library selector for playlists, artists, albums, and tags.\n"
        "Double-click or press Enter to play. Right-click songs for playlist, tag, and folder actions.\n"
        "Right-click lyrics, or click Load LRC File, to choose lyrics manually.\n"
        "Play MV opens the video player for local MV files.",
        &dialog);
    ops->setObjectName("helpCard");
    ops->setWordWrap(true);

    auto *keysTitle = new QLabel("Shortcuts", &dialog);
    keysTitle->setObjectName("helpCardTitle");
    auto *keys = new QLabel(
        "Space  Play/Pause\n"
        "Left / Right  Seek 5s, hold to scrub\n"
        "Up / Down  Volume\n"
        "PgUp / PgDn  Previous/Next\n"
        "Enter  Play selected song\n"
        "Esc  Clear search",
        &dialog);
    keys->setObjectName("helpCard");
    keys->setWordWrap(true);

    root->addSpacing(4);
    root->addWidget(opsTitle);
    root->addWidget(ops);
    root->addWidget(keysTitle);
    root->addWidget(keys);

    auto *buttons = new QHBoxLayout;
    buttons->addStretch();
    auto *closeButton = new QPushButton("Close", &dialog);
    connect(closeButton, &QPushButton::clicked, &dialog, &QDialog::accept);
    buttons->addWidget(closeButton);
    root->addLayout(buttons);

    dialog.exec();
}
void MainWindow::showMvWindow()
{
    const bool resumeMusicAfterMv = audioPlayer.isPlaying();
    if (resumeMusicAfterMv) {
        audioPlayer.pause();
        showTransientFeedback("Music paused for MV");
    }

    MvPlayerDialog dialog(nightMode, this);
    dialog.exec();

    if (resumeMusicAfterMv) {
        applyRestoredPositionBeforePlayback();
        audioPlayer.play();
        showTransientFeedback("Music resumed");
    }
}
