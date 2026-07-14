#include "mvplayerdialog.h"

#include "seekslider.h"

#include <QAudioOutput>
#include <QCloseEvent>
#include <QCoreApplication>
#include <QDir>
#include <QFileDialog>
#include <QFileInfo>
#include <QHBoxLayout>
#include <QIcon>
#include <QLabel>
#include <QMediaPlayer>
#include <QPushButton>
#include <QSizePolicy>
#include <QSlider>
#include <QUrl>
#include <QVBoxLayout>
#include <QVideoWidget>

MvPlayerDialog::MvPlayerDialog(bool nightMode, QWidget *parent)
    : QDialog(parent),
      m_nightMode(nightMode),
      m_player(new QMediaPlayer(this)),
      m_audioOutput(new QAudioOutput(this))
{
    setWindowTitle("Play MV");
    setWindowFlag(Qt::WindowMaximizeButtonHint, true);
    setSizeGripEnabled(true);
    resize(920, 600);
    m_player->setAudioOutput(m_audioOutput);
    m_audioOutput->setVolume(0.8);

    buildUi();
    applyStyle();
    connectSignals();
}

void MvPlayerDialog::closeEvent(QCloseEvent *event)
{
    m_player->stop();
    QDialog::closeEvent(event);
}

void MvPlayerDialog::buildUi()
{
    auto *root = new QVBoxLayout(this);
    root->setContentsMargins(24, 22, 24, 20);
    root->setSpacing(16);

    auto *header = new QHBoxLayout;
    auto *titleBlock = new QVBoxLayout;
    titleBlock->setSpacing(2);
    m_titleLabel = new QLabel("Play MV", this);
    m_titleLabel->setObjectName("mvTitle");
    m_hintLabel = new QLabel("Open a video file to start playback", this);
    m_hintLabel->setObjectName("mvHint");
    titleBlock->addWidget(m_titleLabel);
    titleBlock->addWidget(m_hintLabel);

    m_openButton = new QPushButton("Open MV", this);
    m_openButton->setCursor(Qt::PointingHandCursor);
    m_openButton->setIcon(QIcon(assetPath(QString("img/ui-icons/%1/video-play.256.png").arg(themeName()))));
    m_openButton->setIconSize(QSize(18, 18));
    m_openButton->setToolTip("Open a video file");
    m_closeButton = new QPushButton("Close", this);
    m_closeButton->setCursor(Qt::PointingHandCursor);

    header->addLayout(titleBlock);
    header->addStretch();
    header->addWidget(m_openButton);
    header->addWidget(m_closeButton);

    m_videoWidget = new QVideoWidget(this);
    m_videoWidget->setObjectName("mvStage");
    m_videoWidget->setMinimumHeight(360);
    m_videoWidget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    m_player->setVideoOutput(m_videoWidget);

    auto *controlLayout = new QHBoxLayout;
    controlLayout->setSpacing(10);
    m_playPauseButton = new QPushButton(this);
    m_playPauseButton->setObjectName("mvPrimaryPlaybackButton");
    m_playPauseButton->setFixedSize(50, 44);
    m_playPauseButton->setCursor(Qt::PointingHandCursor);
    m_playPauseButton->setToolTip("Play / Pause");
    m_stopButton = new QPushButton(this);
    m_stopButton->setObjectName("mvIconButton");
    m_stopButton->setFixedSize(44, 38);
    m_stopButton->setCursor(Qt::PointingHandCursor);
    m_stopButton->setIcon(QIcon(assetPath(QString("img/player-icons/%1/Stop.512.png").arg(themeName()))));
    m_stopButton->setIconSize(QSize(22, 22));
    m_stopButton->setToolTip("Stop");
    updatePlayPauseButton();

    m_positionSlider = new SeekSlider(Qt::Horizontal, this);
    m_positionSlider->setRange(0, 0);
    m_positionSlider->setCursor(Qt::PointingHandCursor);
    m_positionSlider->setToolTip("Click or drag to seek");
    m_timeLabel = new QLabel("00:00 / 00:00", this);
    m_timeLabel->setObjectName("mvTimeLabel");
    m_timeLabel->setMinimumWidth(110);

    m_volumeSlider = new QSlider(Qt::Horizontal, this);
    m_volumeSlider->setRange(0, 100);
    m_volumeSlider->setValue(80);
    m_volumeSlider->setFixedWidth(120);
    m_volumeSlider->setCursor(Qt::PointingHandCursor);
    m_volumeSlider->setToolTip("Volume");

    controlLayout->addWidget(m_playPauseButton);
    controlLayout->addWidget(m_stopButton);
    controlLayout->addWidget(m_positionSlider, 1);
    controlLayout->addWidget(m_timeLabel);
    controlLayout->addWidget(m_volumeSlider);

    m_statusLabel = new QLabel("No MV loaded", this);
    m_statusLabel->setObjectName("mvStatusLabel");

    root->addLayout(header);
    root->addWidget(m_videoWidget, 1);
    root->addLayout(controlLayout);
    root->addWidget(m_statusLabel);
}

void MvPlayerDialog::applyStyle()
{
    const QString panelColor = m_nightMode ? "#171b24" : "#ffffff";
    const QString textColor = m_nightMode ? "#edf2f7" : "#172033";
    const QString mutedColor = m_nightMode ? "#aeb8c8" : "#61718a";
    const QString cardColor = m_nightMode ? "#10141d" : "#f7fafc";
    const QString buttonColor = m_nightMode ? "#242d3b" : "#f3f7fb";
    const QString buttonHover = m_nightMode ? "#303b4f" : "#e8f1fb";
    const QString borderColor = m_nightMode ? "#303847" : "#d9e2ef";
    const QString accent = m_nightMode ? "#8fb3ff" : "#1f6f78";
    const QString playbackBg = m_nightMode ? "#8fb3ff" : "#2563eb";
    const QString playbackHover = m_nightMode ? "#a9c4ff" : "#1d4ed8";
    const QString playbackPressed = m_nightMode ? "#6f9df3" : "#1e40af";
    const QString sliderBg = m_nightMode ? "#2b3444" : "#d8e1ee";

    setStyleSheet(QString(R"(
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
        QLabel#mvHint, QLabel#mvStatusLabel {
            color: %3;
        }
        QLabel#mvTimeLabel {
            color: %3;
            font-weight: 600;
        }
        QVideoWidget#mvStage {
            background: %4;
            border: 1px solid %7;
            border-radius: 14px;
        }
        QPushButton {
            background: %5;
            border: 1px solid %7;
            border-radius: 10px;
            color: %2;
            padding: 8px 16px;
            font-weight: 700;
        }
        QPushButton:hover {
            background: %6;
            border-color: %8;
        }
        QPushButton#mvIconButton {
            padding: 0;
            border-radius: 10px;
        }
        QPushButton#mvPrimaryPlaybackButton {
            min-width: 50px;
            width: 50px;
            height: 44px;
            padding: 0;
            border-radius: 22px;
            background: %10;
            border-color: %10;
        }
        QPushButton#mvPrimaryPlaybackButton:hover {
            background: %11;
            border-color: %11;
        }
        QPushButton#mvPrimaryPlaybackButton:pressed {
            background: %12;
            border-color: %12;
        }
        QSlider::groove:horizontal {
            height: 6px;
            background: %9;
            border-radius: 3px;
        }
        QSlider::sub-page:horizontal {
            background: %8;
            border-radius: 3px;
        }
        QSlider::handle:horizontal {
            width: 16px;
            height: 16px;
            margin: -5px 0;
            background: %8;
            border: 2px solid %1;
            border-radius: 8px;
        }
    )").arg(panelColor, textColor, mutedColor, cardColor, buttonColor, buttonHover, borderColor, accent, sliderBg, playbackBg, playbackHover, playbackPressed));
}

void MvPlayerDialog::connectSignals()
{
    connect(m_openButton, &QPushButton::clicked, this, &MvPlayerDialog::openVideoFile);
    connect(m_closeButton, &QPushButton::clicked, this, &QDialog::accept);
    connect(m_playPauseButton, &QPushButton::clicked, this, &MvPlayerDialog::togglePlayPause);
    connect(m_stopButton, &QPushButton::clicked, this, [this]() {
        m_player->stop();
        m_positionSlider->setValue(0);
        updateTimeLabel(0, m_durationMs);
        updatePlayPauseButton();
        m_statusLabel->setText("Stopped");
    });
    connect(m_volumeSlider, &QSlider::valueChanged, this, [this](int value) {
        m_audioOutput->setVolume(qBound(0, value, 100) / 100.0);
    });
    connect(m_positionSlider, &QSlider::sliderPressed, this, [this]() {
        m_sliderPressed = true;
    });
    connect(m_positionSlider, &QSlider::sliderReleased, this, [this]() {
        m_sliderPressed = false;
        m_player->setPosition(m_positionSlider->value());
    });
    connect(m_positionSlider, &QSlider::sliderMoved, this, [this](int value) {
        updateTimeLabel(value, m_durationMs);
    });
    connect(m_player, &QMediaPlayer::positionChanged, this, [this](qint64 positionMs) {
        if (!m_sliderPressed) {
            m_positionSlider->setValue(static_cast<int>(positionMs));
        }
        updateTimeLabel(positionMs, m_durationMs);
    });
    connect(m_player, &QMediaPlayer::durationChanged, this, [this](qint64 durationMs) {
        m_durationMs = durationMs;
        m_positionSlider->setRange(0, static_cast<int>(qMax<qint64>(0, durationMs)));
        updateTimeLabel(m_player->position(), durationMs);
    });
    connect(m_player, &QMediaPlayer::playbackStateChanged, this, [this]() {
        updatePlayPauseButton();
    });
    connect(m_player, &QMediaPlayer::errorOccurred, this, [this](QMediaPlayer::Error, const QString &errorString) {
        if (!errorString.isEmpty()) {
            m_statusLabel->setText(QString("Playback error: %1").arg(errorString));
        }
    });
}

void MvPlayerDialog::openVideoFile()
{
    const QString filePath = QFileDialog::getOpenFileName(
        this,
        "Open MV File",
        QDir::homePath(),
        "Video files (*.mp4 *.mkv *.avi *.mov *.wmv *.m4v *.webm);;All files (*.*)");
    if (filePath.isEmpty()) {
        return;
    }

    m_player->setSource(QUrl::fromLocalFile(filePath));
    m_hintLabel->setText(QFileInfo(filePath).fileName());
    m_statusLabel->setText("Playing");
    m_player->play();
    updatePlayPauseButton();
}

void MvPlayerDialog::togglePlayPause()
{
    if (m_player->source().isEmpty()) {
        openVideoFile();
        return;
    }

    if (m_player->playbackState() == QMediaPlayer::PlayingState) {
        m_player->pause();
        m_statusLabel->setText("Paused");
    } else {
        m_player->play();
        m_statusLabel->setText("Playing");
    }
    updatePlayPauseButton();
}

void MvPlayerDialog::updatePlayPauseButton()
{
    const bool playing = m_player->playbackState() == QMediaPlayer::PlayingState;
    m_playPauseButton->setIcon(QIcon(assetPath(QString("img/player-icons/%1/%2").arg(themeName(), playing ? "Pause.512.png" : "Play.512.png"))));
    m_playPauseButton->setIconSize(QSize(22, 22));
    m_playPauseButton->setToolTip(playing ? "Pause" : "Play");
}

void MvPlayerDialog::updateTimeLabel(qint64 positionMs, qint64 durationMs)
{
    m_timeLabel->setText(QString("%1 / %2").arg(formatTime(positionMs), formatTime(durationMs)));
}

QString MvPlayerDialog::assetPath(const QString &relativePath) const
{
    const QString appLocalPath = QDir(QCoreApplication::applicationDirPath()).filePath(QString("assets/%1").arg(relativePath));
    if (QFileInfo::exists(appLocalPath)) {
        return appLocalPath;
    }

    const QString sourcePath = QDir(QString::fromUtf8(PROJECT_ASSETS_DIR)).filePath(relativePath);
    if (QFileInfo::exists(sourcePath)) {
        return sourcePath;
    }

    return appLocalPath;
}

QString MvPlayerDialog::themeName() const
{
    return m_nightMode ? "night" : "day";
}

QString MvPlayerDialog::formatTime(qint64 milliseconds) const
{
    const qint64 totalSeconds = qMax<qint64>(0, milliseconds / 1000);
    const qint64 hours = totalSeconds / 3600;
    const qint64 minutes = (totalSeconds % 3600) / 60;
    const qint64 seconds = totalSeconds % 60;
    if (hours > 0) {
        return QString("%1:%2:%3")
            .arg(hours)
            .arg(minutes, 2, 10, QLatin1Char('0'))
            .arg(seconds, 2, 10, QLatin1Char('0'));
    }
    return QString("%1:%2")
        .arg(minutes, 2, 10, QLatin1Char('0'))
        .arg(seconds, 2, 10, QLatin1Char('0'));
}
