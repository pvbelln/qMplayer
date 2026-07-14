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

namespace
{
void polishComboPopup(QComboBox *comboBox, const QString &backgroundColor)
{
    if (comboBox == nullptr || comboBox->view() == nullptr) {
        return;
    }

    comboBox->view()->setFrameShape(QFrame::NoFrame);
    comboBox->view()->setStyleSheet(QString("background: %1; border: none;").arg(backgroundColor));
    comboBox->view()->setAutoFillBackground(false);
    if (comboBox->view()->viewport() != nullptr) {
        comboBox->view()->viewport()->setStyleSheet(QString("background: %1; border: none;").arg(backgroundColor));
        comboBox->view()->viewport()->setAutoFillBackground(true);
    }

    QWidget *popupContainer = comboBox->view()->parentWidget();
    if (popupContainer != nullptr) {
        popupContainer->setStyleSheet(QString("background: %1; border: none;").arg(backgroundColor));
        popupContainer->setAutoFillBackground(true);
        popupContainer->setWindowFlags(Qt::Popup | Qt::FramelessWindowHint);
    }
}
}

void MainWindow::buildUi()
{
    auto *rootLayout = new QVBoxLayout(this);
    rootLayout->setContentsMargins(20, 18, 20, 18);
    rootLayout->setSpacing(14);

    auto *topLayout = new QHBoxLayout;
    topLayout->setSpacing(10);
    scanButton = new QPushButton("Add Folder", this);
    scanButton->setObjectName("primaryActionButton");
    scanButton->setCursor(Qt::PointingHandCursor);
    transcodeButton = new QPushButton("Transcode", this);
    transcodeButton->setObjectName("secondaryActionButton");
    transcodeButton->setCursor(Qt::PointingHandCursor);
    playMvButton = new QPushButton("Play MV", this);
    playMvButton->setObjectName("secondaryActionButton");
    playMvButton->setCursor(Qt::PointingHandCursor);
    themeButton = new QPushButton(this);
    themeButton->setObjectName("themeToggleButton");
    themeButton->setFixedSize(42, 42);
    themeButton->setCursor(Qt::PointingHandCursor);
    aboutButton = new QPushButton("About", this);
    aboutButton->setObjectName("secondaryActionButton");
    aboutButton->setCursor(Qt::PointingHandCursor);
    helpButton = new QPushButton("Help", this);
    helpButton->setObjectName("secondaryActionButton");
    helpButton->setCursor(Qt::PointingHandCursor);
    statusLabel = new QLabel(this);
    statusLabel->setObjectName("statusLabel");
    statusLabel->hide();
    topLayout->addWidget(scanButton);
    topLayout->addWidget(transcodeButton);
    topLayout->addWidget(playMvButton);
    topLayout->addWidget(helpButton);
    topLayout->addWidget(aboutButton);
    topLayout->addStretch();
    topLayout->addWidget(themeButton);

    auto *leftPanel = new QWidget(this);
    leftPanel->setObjectName("sidePanel");
    auto *leftLayout = new QVBoxLayout(leftPanel);
    leftLayout->setContentsMargins(16, 16, 16, 16);
    leftLayout->setSpacing(12);
    libraryHeaderLabel = new QLabel("Library", this);
    libraryHeaderLabel->setObjectName("sectionTitle");
    searchEdit = new QLineEdit(this);
    searchEdit->setObjectName("searchEdit");
    searchEdit->setPlaceholderText("Search in current view");
    searchEdit->setClearButtonEnabled(true);
    searchEdit->setToolTip("Search title, artist, album, tag, or filename in the current view");
    collectionCombo = new QComboBox(this);
    collectionCombo->setObjectName("collectionCombo");
    collectionCombo->setCursor(Qt::PointingHandCursor);
    collectionCombo->setToolTip("Choose library, playlist, artist, album, or tag");
    polishComboPopup(collectionCombo, nightMode ? "#1f2530" : "#ffffff");
    newPlaylistButton = new QPushButton("New Playlist", this);
    newPlaylistButton->setObjectName("compactActionButton");
    newPlaylistButton->setCursor(Qt::PointingHandCursor);
    newPlaylistButton->setToolTip("Create playlist");
    addToPlaylistButton = new QPushButton("Add to", this);
    addToPlaylistButton->setObjectName("compactActionButton");
    addToPlaylistButton->setCursor(Qt::PointingHandCursor);
    addToPlaylistButton->setToolTip("Add selected song to a playlist");
    tagButton = new QPushButton("Tags", this);
    tagButton->setObjectName("compactActionButton");
    tagButton->setCursor(Qt::PointingHandCursor);
    tagButton->setToolTip("Edit custom tags for the selected song");
    managePlaylistButton = new QPushButton("Manage", this);
    managePlaylistButton->setObjectName("compactActionButton");
    managePlaylistButton->setCursor(Qt::PointingHandCursor);
    managePlaylistButton->setToolTip("Rename or delete the current playlist");
    auto *collectionToolsLayout = new QHBoxLayout;
    collectionToolsLayout->setSpacing(6);
    collectionToolsLayout->addWidget(newPlaylistButton);
    collectionToolsLayout->addWidget(addToPlaylistButton);
    collectionToolsLayout->addWidget(tagButton);
    collectionToolsLayout->addWidget(managePlaylistButton);
    playlistWidget = new QListWidget(this);
    playlistWidget->setObjectName("playlistWidget");
    playlistWidget->setCursor(Qt::PointingHandCursor);
    playlistWidget->setAlternatingRowColors(false);
    playlistWidget->setSpacing(4);
    playlistWidget->setUniformItemSizes(false);
    playlistWidget->setContextMenuPolicy(Qt::CustomContextMenu);
    emptyLibraryLabel = new QLabel(this);
    emptyLibraryLabel->setObjectName("emptyLibraryLabel");
    emptyLibraryLabel->setAlignment(Qt::AlignCenter);
    emptyLibraryLabel->setWordWrap(true);
    emptyLibraryLabel->hide();
    searchHintLabel = new QLabel(this);
    searchHintLabel->setObjectName("searchHintLabel");
    searchHintLabel->setWordWrap(true);
    leftLayout->addWidget(libraryHeaderLabel);
    leftLayout->addWidget(collectionCombo);
    leftLayout->addLayout(collectionToolsLayout);
    leftLayout->addWidget(searchEdit);
    leftLayout->addWidget(searchHintLabel);
    leftLayout->addWidget(playlistWidget, 1);
    leftLayout->addWidget(emptyLibraryLabel, 1);

    auto *rightPanel = new QWidget(this);
    rightPanel->setObjectName("contentPanel");
    auto *rightLayout = new QVBoxLayout(rightPanel);
    rightLayout->setContentsMargins(18, 18, 18, 14);
    rightLayout->setSpacing(16);

    auto *infoLayout = new QHBoxLayout;
    infoLayout->setSpacing(18);
    coverLabel = new QLabel(this);
    coverLabel->setObjectName("coverLabel");
    coverLabel->setFixedSize(168, 168);
    coverLabel->setAlignment(Qt::AlignCenter);
    updateCoverImage(QImage());

    auto *metaLayout = new QVBoxLayout;
    metaLayout->setSpacing(8);
    titleLabel = new QLabel("No song selected", this);
    titleLabel->setObjectName("titleLabel");
    titleLabel->setWordWrap(true);
    artistLabel = new QLabel("Artist / Album", this);
    artistLabel->setObjectName("artistLabel");
    artistLabel->setWordWrap(true);
    metaLayout->addStretch();
    metaLayout->addWidget(titleLabel);
    metaLayout->addWidget(artistLabel);
    metaLayout->addStretch();
    infoLayout->addWidget(coverLabel);
    infoLayout->addLayout(metaLayout, 1);

    lyricsWidget = new LyricsWidget(this);

    auto *lyricsAdjustLayout = new QHBoxLayout;
    lyricsAdjustLayout->setSpacing(8);
    auto *lyricsOffsetLabel = new QLabel("Lyrics sync offset", this);
    lyricsOffsetLabel->setObjectName("hintLabel");
    lyricsDelayMinusButton = new QPushButton("-0.5s", this);
    lyricsDelayPlusButton = new QPushButton("+0.5s", this);
    lyricsDelayMinusButton->setObjectName("compactActionButton");
    lyricsDelayPlusButton->setObjectName("compactActionButton");
    lyricsDelayMinusButton->setFixedSize(92, 38);
    lyricsDelayPlusButton->setFixedSize(92, 38);
    lyricsDelayMinusButton->setCursor(Qt::PointingHandCursor);
    lyricsDelayPlusButton->setCursor(Qt::PointingHandCursor);
    lyricsDelayMinusButton->setFocusPolicy(Qt::NoFocus);
    lyricsDelayPlusButton->setFocusPolicy(Qt::NoFocus);
    lyricsAdjustLayout->addStretch();
    lyricsAdjustLayout->addWidget(lyricsOffsetLabel);
    lyricsAdjustLayout->addWidget(lyricsDelayMinusButton);
    lyricsAdjustLayout->addWidget(lyricsDelayPlusButton);

    rightLayout->addLayout(infoLayout);
    rightLayout->addWidget(lyricsWidget, 1);
    rightLayout->addLayout(lyricsAdjustLayout);

    auto *splitter = new QSplitter(this);
    splitter->addWidget(leftPanel);
    splitter->addWidget(rightPanel);
    splitter->setStretchFactor(0, 1);
    splitter->setStretchFactor(1, 2);
    splitter->setSizes({340, 760});

    auto *playbackLayout = new QHBoxLayout;
    playbackLayout->setObjectName("playbackLayout");
    playbackLayout->setSpacing(10);
    previousButton = new QPushButton(this);
    playPauseButton = new QPushButton(this);
    stopButton = new QPushButton(this);
    nextButton = new QPushButton(this);
    modeButton = new QPushButton(this);
    positionSlider = new SeekSlider(Qt::Horizontal, this);
    positionSlider->setObjectName("positionSlider");
    positionSlider->setCursor(Qt::PointingHandCursor);
    positionSlider->setRange(0, 0);
    positionSlider->setToolTip("Click or drag to seek. Left/Right keys step 5 seconds.");
    waveformWidget = new WaveformWidget(this);
    waveformWidget->setToolTip("Decoded audio energy waveform");
    timeLabel = new QLabel("00:00 / 00:00", this);
    timeLabel->setObjectName("timeLabel");
    feedbackLabel = new QLabel(this);
    feedbackLabel->setObjectName("feedbackLabel");
    feedbackLabel->setMinimumWidth(96);
    feedbackLabel->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    feedbackLabel->hide();
    muteButton = new QPushButton(this);
    volumeSlider = new QSlider(Qt::Horizontal, this);
    volumeSlider->setObjectName("volumeSlider");
    volumeSlider->setCursor(Qt::PointingHandCursor);
    volumeSlider->setRange(0, 100);
    volumeSlider->setValue(80);
    volumeSlider->setFixedWidth(132);
    volumeSlider->setToolTip("Volume. Up/Down keys adjust volume.");
    setupIconButton(previousButton, "First-track.512.png", "Previous (PgUp)");
    setupIconButton(playPauseButton, "Play.512.png", "Play / Pause (Space)");
    setupIconButton(stopButton, "Stop.512.png", "Stop");
    setupIconButton(nextButton, "Last-track.512.png", "Next (PgDn)");
    setupIconButton(modeButton, "Folder-loop.512.png", "Playback mode");
    setupIconButton(muteButton, "Audio.512.png", "Mute");
    previousButton->setObjectName("playbackIconButton");
    playPauseButton->setObjectName("primaryPlaybackButton");
    stopButton->setObjectName("playbackIconButton");
    nextButton->setObjectName("playbackIconButton");
    modeButton->setObjectName("playbackIconButton");
    muteButton->setObjectName("playbackIconButton");
    playPauseButton->setFixedSize(50, 44);
    playPauseButton->setIconSize(QSize(26, 26));
    scanButton->setToolTip("Add folder");
    transcodeButton->setToolTip("Transcode selected song");
    playMvButton->setToolTip("Open MV player");
    themeButton->setToolTip("Switch to night mode");
    helpButton->setToolTip("Help and shortcuts");
    aboutButton->setToolTip("About MPlayer");
    lyricsOffsetLabel->setToolTip("Adjust the whole LRC timing when lyrics are not aligned with music");
    lyricsDelayMinusButton->setToolTip("Show each lyric line 0.5 seconds earlier");
    lyricsDelayPlusButton->setToolTip("Show each lyric line 0.5 seconds later");

    playbackLayout->addWidget(previousButton);
    playbackLayout->addWidget(playPauseButton);
    playbackLayout->addWidget(stopButton);
    playbackLayout->addWidget(nextButton);
    playbackLayout->addWidget(modeButton);
    auto *progressStack = new QVBoxLayout;
    progressStack->setContentsMargins(0, 0, 0, 0);
    progressStack->setSpacing(2);
    progressStack->addWidget(waveformWidget);
    progressStack->addWidget(positionSlider);
    playbackLayout->addLayout(progressStack, 1);
    playbackLayout->addWidget(timeLabel);
    playbackLayout->addWidget(feedbackLabel);
    playbackLayout->addWidget(muteButton);
    playbackLayout->addWidget(volumeSlider);

    rootLayout->addLayout(topLayout);
    rootLayout->addWidget(splitter, 1);

    auto *playbackPanel = new QWidget(this);
    playbackPanel->setObjectName("playbackPanel");
    auto *playbackPanelLayout = new QHBoxLayout(playbackPanel);
    playbackPanelLayout->setContentsMargins(14, 10, 14, 10);
    playbackPanelLayout->addLayout(playbackLayout);
    rootLayout->addWidget(playbackPanel);
}
void MainWindow::connectSignals()
{
    connect(scanButton, &QPushButton::clicked, this, &MainWindow::scanFolder);
    connect(transcodeButton, &QPushButton::clicked, this, &MainWindow::transcodeSelectedTrack);
    connect(playMvButton, &QPushButton::clicked, this, &MainWindow::showMvWindow);
    connect(themeButton, &QPushButton::clicked, this, &MainWindow::toggleTheme);
    connect(aboutButton, &QPushButton::clicked, this, &MainWindow::showAboutDialog);
    connect(helpButton, &QPushButton::clicked, this, &MainWindow::showHelpDialog);
    connect(searchEdit, &QLineEdit::textChanged, this, &MainWindow::refreshPlaylist);
    connect(collectionCombo, &QComboBox::currentIndexChanged, this, [this](int index) {
        if (updatingCollectionCombo || index < 0) {
            return;
        }
        const QString key = collectionCombo->itemData(index).toString();
        const QStringList parts = key.split(":", Qt::KeepEmptyParts);
        activeCollectionType = parts.value(0, "all");
        activeCollectionValue = parts.mid(1).join(":");
        searchEdit->setPlaceholderText(QString("Search in %1").arg(currentCollectionLabel()));
        refreshPlaylist();
        updateLibraryActionState();
    });
    connect(newPlaylistButton, &QPushButton::clicked, this, &MainWindow::createUserPlaylist);
    connect(addToPlaylistButton, &QPushButton::clicked, this, &MainWindow::addSelectedTrackToPlaylist);
    connect(tagButton, &QPushButton::clicked, this, &MainWindow::editSelectedTrackTags);
    connect(managePlaylistButton, &QPushButton::clicked, this, &MainWindow::showPlaylistManageMenu);
    connect(modeButton, &QPushButton::clicked, this, &MainWindow::cyclePlaybackMode);
    connect(playlistWidget, &QListWidget::itemDoubleClicked, this, [this](QListWidgetItem *item) {
        playTrackAt(item->data(Qt::UserRole).toInt());
    });
    connect(playlistWidget, &QListWidget::itemSelectionChanged, this, &MainWindow::updateLibraryActionState);
    connect(playlistWidget, &QListWidget::customContextMenuRequested, this, &MainWindow::showPlaylistContextMenu);

    connect(playPauseButton, &QPushButton::clicked, this, [this]() {
        if (playlist.currentTrack() == nullptr && !playlist.isEmpty()) {
            const int index = !visibleTrackIndexes.isEmpty() ? visibleTrackIndexes.first() : (playlist.currentIndex() < 0 ? 0 : playlist.currentIndex());
            playTrackAt(index);
            return;
        }
        applyRestoredPositionBeforePlayback();
        audioPlayer.togglePlayPause();
        showTransientFeedback(audioPlayer.isPlaying() ? "Playing" : "Paused");
    });
    connect(stopButton, &QPushButton::clicked, this, [this]() {
        audioPlayer.stop();
        showTransientFeedback("Stopped");
    });
    connect(nextButton, &QPushButton::clicked, this, [this]() {
        playNext();
        showTransientFeedback("Next track");
    });
    connect(previousButton, &QPushButton::clicked, this, [this]() {
        playPrevious();
        showTransientFeedback("Previous track");
    });
    connect(volumeSlider, &QSlider::valueChanged, &audioPlayer, &AudioPlayer::setVolume);
    connect(lyricsDelayMinusButton, &QPushButton::clicked, this, [this]() {
        adjustLyricsOffset(-500);
    });
    connect(lyricsDelayPlusButton, &QPushButton::clicked, this, [this]() {
        adjustLyricsOffset(500);
    });
    connect(lyricsWidget, &LyricsWidget::jumpRequested, this, [this](qint64 positionMs) {
        audioPlayer.seek(positionMs);
        audioPlayer.play();
        showTransientFeedback("Jumped to lyric");
    });
    connect(lyricsWidget, &LyricsWidget::loadLrcRequested, this, &MainWindow::chooseLyricsFileForCurrentTrack);
    connect(lyricsWidget, &LyricsWidget::lyricsContextMenuRequested, this, &MainWindow::showLyricsContextMenu);
    connect(muteButton, &QPushButton::clicked, this, [this]() {
        audioPlayer.toggleMuted();
        updateMuteButton();
        showTransientFeedback(audioPlayer.isMuted() ? "Muted" : "Unmuted");
    });

    connect(positionSlider, &QSlider::sliderPressed, this, [this]() {
        keyboardSeekTargetMs = -1;
        keyboardSeekWasPlaying = false;
        sliderPressed = true;
    });
    connect(positionSlider, &QSlider::sliderReleased, this, [this]() {
        sliderPressed = false;
        audioPlayer.seek(positionSlider->value());
        waveformWidget->setPosition(positionSlider->value());
        savePlaybackSnapshot();
        showTransientFeedback(QString("Seek %1").arg(formatTime(positionSlider->value())));
    });

    connect(&audioPlayer, &AudioPlayer::positionChanged, this, [this](qint64 positionMs) {
        if (keyboardSeekTargetMs >= 0) {
            return;
        }
        if (!sliderPressed) {
            positionSlider->setValue(static_cast<int>(positionMs));
        }
        waveformWidget->setPosition(positionMs);
        lyricsWidget->setPosition(positionMs);
        updateTimeLabel(positionMs, currentDurationMs);
        if (playlist.currentTrack() != nullptr
            && pendingRestorePositionMs < 0
            && qAbs(positionMs - lastSavedPlaybackPositionMs) >= 3000) {
            savePlaybackSnapshot();
        }
    });
    connect(&audioPlayer, &AudioPlayer::durationChanged, this, [this](qint64 durationMs) {
        currentDurationMs = durationMs;
        positionSlider->setRange(0, static_cast<int>(durationMs));
        waveformWidget->setDuration(durationMs);
        if (pendingRestorePositionMs >= 0) {
            const qint64 restoredPosition = qMin(pendingRestorePositionMs, qMax<qint64>(0, durationMs - 1000));
            audioPlayer.seek(restoredPosition);
            positionSlider->setValue(static_cast<int>(restoredPosition));
            waveformWidget->setPosition(restoredPosition);
            lyricsWidget->setPosition(restoredPosition);
            updateTimeLabel(restoredPosition, durationMs);
            lastSavedPlaybackPositionMs = restoredPosition;
            statusLabel->setText("Restored last position");
        } else {
            updateTimeLabel(audioPlayer.position(), durationMs);
        }
        if (Track *track = playlist.trackAt(playlist.currentIndex())) {
            track->durationMs = durationMs;
            saveLibrary();
        }
    });
    connect(&audioPlayer, &AudioPlayer::metadataChanged, this, [this](const QString &title, const QString &artist, const QString &album, const QImage &coverImage) {
        Track *track = playlist.trackAt(playlist.currentIndex());
        if (track == nullptr) {
            return;
        }

        bool changed = false;
        if (!title.trimmed().isEmpty() && track->title != title.trimmed()) {
            track->title = title.trimmed();
            changed = true;
        }
        if (!artist.trimmed().isEmpty() && track->artist != artist.trimmed()) {
            track->artist = artist.trimmed();
            changed = true;
        }
        if (!album.trimmed().isEmpty() && track->album != album.trimmed()) {
            track->album = album.trimmed();
            changed = true;
        }

        if (changed) {
            saveLibrary();
            refreshCollectionCombo();
            refreshPlaylist();
            updateCurrentTrackView();
        }
        if (!coverImage.isNull()) {
            updateCoverImage(coverImage);
        }
    });
    connect(&audioPlayer, &AudioPlayer::playbackStateChanged, this, [this](bool playing) {
        updatePlayPauseButton(playing);
        lyricsWidget->setPlaybackActive(playing);
        waveformWidget->setPlaybackActive(playing);
        statusLabel->setText(playing ? "Playing" : "Paused");
    });
    connect(&audioPlayer, &AudioPlayer::finished, this, &MainWindow::playNext);
    connect(&audioPlayer, &AudioPlayer::errorOccurred, this, [this](const QString &message) {
        const qint64 remainingMs = currentDurationMs - audioPlayer.position();
        if (currentDurationMs > 0
            && remainingMs >= -1000
            && remainingMs <= 4000
            && message.contains("Invalid data", Qt::CaseInsensitive)
            && !handlingNearEndError) {
            handlingNearEndError = true;
            statusLabel->setText("Reached end of media");
            playNext();
            handlingNearEndError = false;
            return;
        }

        statusLabel->setText("Playback error");
        QMessageBox::warning(this, "Playback error", message);
    });
    connect(&transcoder, &Transcoder::statusChanged, this, [this](const QString &status) {
        statusLabel->setText(status);
    });
    connect(&transcoder, &Transcoder::finished, this, [this](bool success, const QString &message) {
        transcodeButton->setEnabled(true);
        statusLabel->setText(message);
        if (success) {
            showTransientFeedback("Transcode finished");
        } else {
            QMessageBox::warning(this, "Transcode failed", message);
        }
    });
    connect(waveformDecoder, &QAudioDecoder::bufferReady, this, [this]() {
        processWaveformBuffer(waveformDecoder->read());
    });
    connect(waveformDecoder, &QAudioDecoder::finished, this, [this]() {
        waveformWidget->setEnergies(normalizedWaveform());
    });
}
void MainWindow::setupIconButton(QPushButton *button, const QString &iconName, const QString &toolTip)
{
    button->setText("");
    button->setIcon(QIcon(assetPath(QString("img/player-icons/%1/%2").arg(themeName(), iconName))));
    button->setIconSize(QSize(22, 22));
    button->setToolTip(toolTip);
    button->setFixedSize(42, 38);
    button->setCursor(Qt::PointingHandCursor);
    button->setFocusPolicy(Qt::NoFocus);
}
void MainWindow::updatePlayPauseButton(bool playing)
{
    playPauseButton->setIcon(QIcon(assetPath(QString("img/player-icons/%1/%2").arg(themeName(), playing ? "Pause.512.png" : "Play.512.png"))));
    playPauseButton->setToolTip(playing ? "Pause" : "Play");
}
void MainWindow::updateMuteButton()
{
    const bool muted = audioPlayer.isMuted();
    muteButton->setIcon(QIcon(assetPath(QString("img/player-icons/%1/%2").arg(themeName(), muted ? "Audio-off.512.png" : "Audio.512.png"))));
    muteButton->setToolTip(muted ? "Unmute" : "Mute");
}
void MainWindow::updateModeButton()
{
    const PlaybackStrategy *strategy = playbackStrategies[playbackStrategyIndex].get();
    modeButton->setIcon(QIcon(assetPath(QString("img/player-icons/%1/%2").arg(themeName(), strategy->iconName()))));
    modeButton->setToolTip(strategy->name());
    statusLabel->setText(strategy->name());
}
void MainWindow::updateThemedIcons()
{
    setupIconButton(previousButton, "First-track.512.png", "Previous");
    updatePlayPauseButton(audioPlayer.isPlaying());
    playPauseButton->setFixedSize(50, 44);
    playPauseButton->setIconSize(QSize(26, 26));
    setupIconButton(stopButton, "Stop.512.png", "Stop");
    setupIconButton(nextButton, "Last-track.512.png", "Next");
    updateModeButton();
    updateMuteButton();
}
void MainWindow::toggleTheme()
{
    nightMode = !nightMode;
    QSettings().setValue("theme/nightMode", nightMode);
    applyBaseStyle();
    updateThemedIcons();
    updateCoverImage(currentCoverImage);
    lyricsWidget->setNightMode(nightMode);
    waveformWidget->setNightMode(nightMode);
    statusLabel->setText(nightMode ? "Night mode" : "Day mode");
}
void MainWindow::cyclePlaybackMode()
{
    playbackStrategyIndex = (playbackStrategyIndex + 1) % static_cast<int>(playbackStrategies.size());
    updateModeButton();
}
