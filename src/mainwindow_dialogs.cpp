#include "mainwindow.h"

#include "lrcparser.h"
#include "seekslider.h"
#include "lyricswidget.h"
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
    dialog.setWindowTitle("About MPlayer");
    dialog.setModal(true);
    dialog.resize(560, 360);

    const QString panelColor = nightMode ? "#171b24" : "#ffffff";
    const QString textColor = nightMode ? "#edf2f7" : "#172033";
    const QString mutedColor = nightMode ? "#aeb8c8" : "#61718a";
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
        QLabel#aboutSubtitle, QLabel#aboutBody {
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
    )").arg(panelColor, textColor, mutedColor, borderColor, buttonBg, buttonHover, accent));

    auto *root = new QVBoxLayout(&dialog);
    root->setContentsMargins(24, 24, 24, 20);
    root->setSpacing(18);

    auto *content = new QHBoxLayout;
    content->setSpacing(22);

    auto *logoLabel = new QLabel(&dialog);
    logoLabel->setObjectName("logoPanel");
    logoLabel->setFixedSize(154, 154);
    logoLabel->setAlignment(Qt::AlignCenter);
    const QPixmap logo(assetPath("img/player-logo.svg"));
    if (!logo.isNull()) {
        logoLabel->setPixmap(logo.scaled(118, 118, Qt::KeepAspectRatio, Qt::SmoothTransformation));
    } else {
        logoLabel->setText("MPlayer");
    }

    auto *right = new QVBoxLayout;
    right->setSpacing(8);
    auto *title = new QLabel("MPlayer", &dialog);
    title->setObjectName("aboutTitle");
    auto *subtitle = new QLabel("Qt Desktop Music Player", &dialog);
    subtitle->setObjectName("aboutSubtitle");
    auto *author = new QLabel("Author: Xuqi Zhu", &dialog);
    author->setObjectName("aboutSection");
    auto *body = new QLabel(
        "A course project music player with local library scanning, playback controls, playlists, lyrics, day/night themes, and FFmpeg transcoding.\n\n"
        "Open source and assets:\n"
        "- Qt Framework: GUI, multimedia, JSON, and process management.\n"
        "- FFmpeg: transcoding, bundled license in ffmpeg/LICENSE.\n"
        "- Player icons: Daniele De Santis / IconArchive, CC BY 4.0.\n"
        "- UI icons: Pictogrammers, Apache 2.0 / MIT.\n"
        "- Album placeholders: AI-generated images.",
        &dialog);
    body->setObjectName("aboutBody");
    body->setWordWrap(true);

    right->addWidget(title);
    right->addWidget(subtitle);
    right->addSpacing(8);
    right->addWidget(author);
    right->addSpacing(4);
    right->addWidget(body);
    right->addStretch();

    content->addWidget(logoLabel);
    content->addLayout(right, 1);
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
        "Right-click lyrics, or click Load LRC File, to choose lyrics manually.",
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
    QDialog dialog(this);
    dialog.setWindowTitle("Play MV");
    dialog.setModal(false);
    dialog.resize(720, 420);

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
        QLabel#mvTitle {
            font-size: 22px;
            font-weight: 800;
        }
        QLabel#mvHint {
            color: %3;
        }
        QFrame#mvStage {
            background: %4;
            border: 1px solid %5;
            border-radius: 14px;
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
    root->setSpacing(16);

    auto *header = new QHBoxLayout;
    auto *titleBlock = new QVBoxLayout;
    titleBlock->setSpacing(2);

    auto *title = new QLabel("Play MV", &dialog);
    title->setObjectName("mvTitle");
    auto *hint = new QLabel("Video playback window", &dialog);
    hint->setObjectName("mvHint");
    titleBlock->addWidget(title);
    titleBlock->addWidget(hint);

    auto *closeButton = new QPushButton("Close", &dialog);
    closeButton->setCursor(Qt::PointingHandCursor);
    connect(closeButton, &QPushButton::clicked, &dialog, &QDialog::accept);

    header->addLayout(titleBlock);
    header->addStretch();
    header->addWidget(closeButton);

    auto *stage = new QFrame(&dialog);
    stage->setObjectName("mvStage");
    stage->setMinimumHeight(290);

    root->addLayout(header);
    root->addWidget(stage, 1);

    dialog.exec();
}
