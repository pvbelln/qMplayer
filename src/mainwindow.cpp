#include "mainwindow.h"

#include "lrcparser.h"
#include "seekslider.h"
#include "lyricswidget.h"
#include "transcodedialog.h"

#include <algorithm>
#include <QAbstractItemView>
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

MainWindow::MainWindow(QWidget *parent)
    : QWidget(parent)
{
    playbackStrategies.push_back(std::make_unique<ListLoopStrategy>());
    playbackStrategies.push_back(std::make_unique<SingleLoopStrategy>());
    playbackStrategies.push_back(std::make_unique<ShuffleStrategy>());
    nightMode = QSettings().value("theme/nightMode", false).toBool();

    setWindowTitle("MPlayer");
    resize(1120, 720);
    buildUi();
    applyBaseStyle();
    connectSignals();
    loadLibrary();
    audioPlayer.setVolume(volumeSlider->value());
    qApp->installEventFilter(this);
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    savePlaybackSnapshot();
    qApp->removeEventFilter(this);
    QWidget::closeEvent(event);
}

bool MainWindow::eventFilter(QObject *watched, QEvent *event)
{
    if (event->type() == QEvent::MouseButtonPress && QApplication::activeWindow() == this) {
        QWidget *focusWidget = QApplication::focusWidget();
        if (focusWidget == searchEdit) {
            QObject *target = watched;
            bool clickedInsideSearch = false;
            while (target != nullptr) {
                if (target == searchEdit) {
                    clickedInsideSearch = true;
                    break;
                }
                target = target->parent();
            }
            if (!clickedInsideSearch) {
                searchEdit->clearFocus();
                setFocus(Qt::MouseFocusReason);
            }
        }
    }

    if (event->type() == QEvent::KeyPress && QApplication::activeWindow() == this) {
        if (QApplication::focusWidget() == searchEdit) {
            auto *keyEvent = static_cast<QKeyEvent *>(event);
            if (keyEvent->key() == Qt::Key_Escape && !searchEdit->text().isEmpty()) {
                searchEdit->clear();
                searchEdit->clearFocus();
                setFocus(Qt::ShortcutFocusReason);
                return true;
            }
            return false;
        }
        auto *keyEvent = static_cast<QKeyEvent *>(event);
        if (handlePlaybackShortcut(keyEvent)) {
            return true;
        }
    }

    if (event->type() == QEvent::KeyRelease && QApplication::activeWindow() == this) {
        if (QApplication::focusWidget() == searchEdit) {
            return false;
        }
        auto *keyEvent = static_cast<QKeyEvent *>(event);
        if (handleKeyboardSeekRelease(keyEvent)) {
            return true;
        }
    }

    return QWidget::eventFilter(watched, event);
}

void MainWindow::keyPressEvent(QKeyEvent *event)
{
    if (handlePlaybackShortcut(event)) {
        return;
    }

    QWidget::keyPressEvent(event);
}

void MainWindow::keyReleaseEvent(QKeyEvent *event)
{
    if (handleKeyboardSeekRelease(event)) {
        return;
    }

    QWidget::keyReleaseEvent(event);
}

bool MainWindow::handlePlaybackShortcut(QKeyEvent *event)
{
    if (event->key() == Qt::Key_Escape && !searchEdit->text().isEmpty()) {
        searchEdit->clear();
        searchEdit->clearFocus();
        setFocus(Qt::ShortcutFocusReason);
        event->accept();
        return true;
    }

    if (event->key() == Qt::Key_Space) {
        if (event->isAutoRepeat()) {
            event->accept();
            return true;
        }
        if (playlist.currentTrack() == nullptr && !playlist.isEmpty()) {
            const int index = !visibleTrackIndexes.isEmpty() ? visibleTrackIndexes.first() : (playlist.currentIndex() < 0 ? 0 : playlist.currentIndex());
            playTrackAt(index);
        } else {
            applyRestoredPositionBeforePlayback();
            audioPlayer.togglePlayPause();
        }
        showTransientFeedback(audioPlayer.isPlaying() ? "Playing" : "Paused");
        event->accept();
        return true;
    }

    if (event->key() == Qt::Key_Left) {
        seekByKeyboard(-5000);
        showTransientFeedback("Seek -5s");
        event->accept();
        return true;
    }

    if (event->key() == Qt::Key_Right) {
        seekByKeyboard(5000);
        showTransientFeedback("Seek +5s");
        event->accept();
        return true;
    }

    if (event->key() == Qt::Key_PageUp) {
        playPrevious();
        showTransientFeedback("Previous track");
        event->accept();
        return true;
    }

    if (event->key() == Qt::Key_PageDown) {
        playNext();
        showTransientFeedback("Next track");
        event->accept();
        return true;
    }

    if (QApplication::focusWidget() == playlistWidget
        && (event->key() == Qt::Key_Up || event->key() == Qt::Key_Down)) {
        return false;
    }

    if (event->key() == Qt::Key_Up) {
        volumeSlider->setValue(qMin(100, volumeSlider->value() + 5));
        showTransientFeedback(QString("Volume %1%").arg(volumeSlider->value()));
        event->accept();
        return true;
    }

    if (event->key() == Qt::Key_Down) {
        volumeSlider->setValue(qMax(0, volumeSlider->value() - 5));
        showTransientFeedback(QString("Volume %1%").arg(volumeSlider->value()));
        event->accept();
        return true;
    }

    if ((event->key() == Qt::Key_Return || event->key() == Qt::Key_Enter)
        && QApplication::focusWidget() == playlistWidget
        && !playlistWidget->selectedItems().isEmpty()) {
        playTrackAt(playlistWidget->selectedItems().first()->data(Qt::UserRole).toInt());
        event->accept();
        return true;
    }

    return false;
}

void MainWindow::seekByKeyboard(qint64 deltaMs)
{
    if (keyboardSeekTargetMs < 0) {
        keyboardSeekWasPlaying = audioPlayer.isPlaying();
        if (keyboardSeekWasPlaying) {
            audioPlayer.pause();
        }
    }

    const qint64 upper = currentDurationMs > 0 ? currentDurationMs : audioPlayer.duration();
    const qint64 base = keyboardSeekTargetMs >= 0 ? keyboardSeekTargetMs : audioPlayer.position();
    const qint64 unclampedTarget = base + deltaMs;
    const qint64 target = upper > 0
        ? qBound<qint64>(0, unclampedTarget, upper)
        : qMax<qint64>(0, unclampedTarget);

    keyboardSeekTargetMs = target;
    audioPlayer.seek(target);
    positionSlider->setValue(static_cast<int>(target));
    lyricsWidget->setPosition(target);
    updateTimeLabel(target, currentDurationMs);
    savePlaybackSnapshot();
}

bool MainWindow::handleKeyboardSeekRelease(QKeyEvent *event)
{
    if (event->isAutoRepeat()
        || (event->key() != Qt::Key_Left && event->key() != Qt::Key_Right)
        || keyboardSeekTargetMs < 0) {
        return false;
    }

    const qint64 target = keyboardSeekTargetMs;
    const bool shouldResume = keyboardSeekWasPlaying;
    keyboardSeekTargetMs = -1;
    keyboardSeekWasPlaying = false;

    audioPlayer.seek(target);
    positionSlider->setValue(static_cast<int>(target));
    lyricsWidget->setPosition(target);
    updateTimeLabel(target, currentDurationMs);
    savePlaybackSnapshot();
    if (shouldResume) {
        audioPlayer.play();
    }
    showTransientFeedback(QString("Seek %1").arg(formatTime(target)));

    event->accept();
    return true;
}

void MainWindow::showTransientFeedback(const QString &message)
{
    if (feedbackLabel == nullptr) {
        return;
    }
    feedbackLabel->setText(message);
    feedbackLabel->show();
    QTimer::singleShot(1400, this, [this, message]() {
        if (feedbackLabel != nullptr && feedbackLabel->text() == message) {
            feedbackLabel->hide();
        }
    });
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
    newPlaylistButton = new QPushButton("+ Playlist", this);
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
    lyricsDelayMinusButton = new QPushButton("Earlier -0.5s", this);
    lyricsDelayPlusButton = new QPushButton("Later +0.5s", this);
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
    playbackLayout->addWidget(positionSlider, 1);
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
        if (pendingRestorePositionMs >= 0) {
            const qint64 restoredPosition = qMin(pendingRestorePositionMs, qMax<qint64>(0, durationMs - 1000));
            audioPlayer.seek(restoredPosition);
            positionSlider->setValue(static_cast<int>(restoredPosition));
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
    statusLabel->setText(nightMode ? "Night mode" : "Day mode");
}

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

QString MainWindow::themeName() const
{
    return nightMode ? "night" : "day";
}

void MainWindow::cyclePlaybackMode()
{
    playbackStrategyIndex = (playbackStrategyIndex + 1) % static_cast<int>(playbackStrategies.size());
    updateModeButton();
}

QString MainWindow::assetPath(const QString &relativePath) const
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

void MainWindow::loadLibrary()
{
    const QVector<Track> loadedTracks = libraryStore.load();
    userPlaylists = libraryStore.loadUserPlaylists();
    QVector<Track> accessibleTracks;
    accessibleTracks.reserve(loadedTracks.size());
    int removedCount = 0;
    bool metadataChanged = false;

    for (Track track : loadedTracks) {
        const QFileInfo info(track.filePath);
        if (info.exists() && info.isReadable()) {
            const bool likelyFilenameTitle = track.title.trimmed().isEmpty()
                || (track.title.trimmed() == info.completeBaseName()
                    && track.artist.trimmed().isEmpty()
                    && track.album.trimmed().isEmpty());
            if (likelyFilenameTitle && readTrackMetadata(&track)) {
                metadataChanged = true;
            }
            accessibleTracks.push_back(track);
        } else {
            ++removedCount;
        }
    }

    playlist.setTracks(accessibleTracks);
    for (UserPlaylist &userPlaylist : userPlaylists) {
        QStringList existingPaths;
        for (const QString &path : userPlaylist.filePaths) {
            if (findTrackIndexByPath(path) >= 0 && !existingPaths.contains(path)) {
                existingPaths.append(path);
            }
        }
        userPlaylist.filePaths = existingPaths;
    }
    if (removedCount > 0 || metadataChanged) {
        libraryStore.save(playlist.tracks());
        libraryStore.saveUserPlaylists(userPlaylists);
    }
    if (removedCount > 0) {
        QMessageBox::warning(
            this,
            "Library warning",
            QString("%1 file(s) in the library cannot be accessed. The library will be refreshed and those entries will be removed.")
                .arg(removedCount));
    }
    refreshCollectionCombo();
    refreshPlaylist();
    restorePlaybackSnapshot();
    statusLabel->setText(QString("Loaded %1 songs").arg(playlist.count()));
}

void MainWindow::saveLibrary()
{
    libraryStore.save(playlist.tracks());
}

void MainWindow::restorePlaybackSnapshot()
{
    const PlaybackSnapshot snapshot = libraryStore.loadPlaybackSnapshot();
    const int index = findTrackIndexByPath(snapshot.filePath);
    if (index < 0) {
        updateCurrentTrackView();
        return;
    }

    playlist.setCurrentIndex(index);
    const Track *track = playlist.currentTrack();
    if (track == nullptr) {
        updateCurrentTrackView();
        return;
    }

    currentPlaybackPath = QFileInfo(track->filePath).absoluteFilePath();
    pendingRestorePositionMs = qMax<qint64>(0, snapshot.positionMs);
    lastSavedPlaybackPositionMs = pendingRestorePositionMs;
    audioPlayer.setSource(track->filePath);
    refreshPlaylist();
    updateCurrentTrackView();
    updateCoverImage(QImage());
    loadLyricsForCurrentTrack();
    updatePlayPauseButton(false);
}

void MainWindow::savePlaybackSnapshot()
{
    const Track *track = playlist.currentTrack();
    if (track == nullptr) {
        return;
    }

    PlaybackSnapshot snapshot;
    snapshot.filePath = QFileInfo(track->filePath).absoluteFilePath();
    snapshot.positionMs = pendingRestorePositionMs >= 0 ? pendingRestorePositionMs : audioPlayer.position();
    libraryStore.savePlaybackSnapshot(snapshot);
    lastSavedPlaybackPositionMs = snapshot.positionMs;
}

void MainWindow::applyRestoredPositionBeforePlayback()
{
    if (pendingRestorePositionMs < 0) {
        return;
    }

    const qint64 upper = currentDurationMs > 0 ? currentDurationMs : audioPlayer.duration();
    const qint64 restoredPosition = upper > 0
        ? qMin(pendingRestorePositionMs, qMax<qint64>(0, upper - 1000))
        : pendingRestorePositionMs;
    audioPlayer.seek(restoredPosition);
    positionSlider->setValue(static_cast<int>(restoredPosition));
    lyricsWidget->setPosition(restoredPosition);
    updateTimeLabel(restoredPosition, currentDurationMs);
    lastSavedPlaybackPositionMs = restoredPosition;
    pendingRestorePositionMs = -1;
}

int MainWindow::findTrackIndexByPath(const QString &filePath) const
{
    const QString absolutePath = QFileInfo(filePath).absoluteFilePath();
    for (int i = 0; i < playlist.count(); ++i) {
        const Track *track = playlist.trackAt(i);
        if (track != nullptr && QFileInfo(track->filePath).absoluteFilePath() == absolutePath) {
            return i;
        }
    }
    return -1;
}

void MainWindow::refreshPlaylist()
{
    const int previousScrollValue = playlistWidget->verticalScrollBar() != nullptr
        ? playlistWidget->verticalScrollBar()->value()
        : 0;
    playlistWidget->setUpdatesEnabled(false);
    playlistWidget->clear();
    visibleTrackIndexes.clear();
    int visibleCount = 0;
    for (int i = 0; i < playlist.count(); ++i) {
        const Track *track = playlist.trackAt(i);
        if (!trackVisibleInCurrentCollection(*track) || !matchesSearch(*track)) {
            continue;
        }
        ++visibleCount;
        visibleTrackIndexes.push_back(i);
        QString itemText = track->displayTitle();
        const QString sub = track->subtitle();
        if (!sub.isEmpty()) {
            itemText += "\n" + sub;
        }
        auto *item = new QListWidgetItem(playlistWidget);
        item->setData(Qt::UserRole, i);
        item->setData(Qt::AccessibleTextRole, itemText);
        item->setToolTip(track->filePath);
        item->setSizeHint(QSize(0, 66));
        if (i == playlist.currentIndex()) {
            item->setSelected(true);
        }

        auto *itemWidget = new QWidget(playlistWidget);
        itemWidget->setObjectName("trackItemWidget");
        itemWidget->setProperty("active", i == playlist.currentIndex());
        itemWidget->setAttribute(Qt::WA_StyledBackground, true);
        itemWidget->setAttribute(Qt::WA_TransparentForMouseEvents, true);

        auto *itemLayout = new QHBoxLayout(itemWidget);
        itemLayout->setContentsMargins(12, 7, 10, 7);
        itemLayout->setSpacing(10);

        auto *textLayout = new QVBoxLayout;
        textLayout->setContentsMargins(0, 0, 0, 0);
        textLayout->setSpacing(3);

        auto *title = new QLabel(track->displayTitle(), itemWidget);
        title->setObjectName("trackTitleLabel");
        title->setWordWrap(false);
        title->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Preferred);
        title->setAttribute(Qt::WA_TransparentForMouseEvents, true);

        auto *subtitle = new QLabel(sub.isEmpty() ? QFileInfo(track->filePath).absolutePath() : sub, itemWidget);
        subtitle->setObjectName("trackSubtitleLabel");
        subtitle->setWordWrap(false);
        subtitle->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Preferred);
        subtitle->setAttribute(Qt::WA_TransparentForMouseEvents, true);

        textLayout->addWidget(title);
        textLayout->addWidget(subtitle);

        auto *duration = new QLabel(track->durationMs > 0 ? formatTime(track->durationMs) : QString("--:--"), itemWidget);
        duration->setObjectName("trackDurationLabel");
        duration->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
        duration->setMinimumWidth(48);
        duration->setAttribute(Qt::WA_TransparentForMouseEvents, true);

        itemLayout->addLayout(textLayout, 1);
        itemLayout->addWidget(duration);
        playlistWidget->setItemWidget(item, itemWidget);
    }
    if (libraryHeaderLabel != nullptr) {
        libraryHeaderLabel->setText(QString("%1  %2/%3")
            .arg(currentCollectionLabel())
            .arg(visibleCount)
            .arg(playlist.count()));
    }
    updateSearchHint(visibleCount);
    if (emptyLibraryLabel != nullptr) {
        emptyLibraryLabel->setText(emptyStateMessage(visibleCount));
        const bool isEmpty = visibleCount == 0;
        emptyLibraryLabel->setVisible(isEmpty);
        playlistWidget->setVisible(!isEmpty);
    }
    if (playlistWidget->isVisible() && playlistWidget->verticalScrollBar() != nullptr) {
        playlistWidget->verticalScrollBar()->setValue(previousScrollValue);
    }
    playlistWidget->setUpdatesEnabled(true);
    if (playlistWidget->isVisible()) {
        ensureCurrentTrackVisibleIfNeeded();
    }
    updateLibraryActionState();
}

QListWidgetItem *MainWindow::currentPlaylistItem() const
{
    const int currentIndex = playlist.currentIndex();
    if (currentIndex < 0) {
        return nullptr;
    }

    for (int row = 0; row < playlistWidget->count(); ++row) {
        QListWidgetItem *item = playlistWidget->item(row);
        if (item != nullptr && item->data(Qt::UserRole).toInt() == currentIndex) {
            return item;
        }
    }

    return nullptr;
}

void MainWindow::ensureCurrentTrackVisibleIfNeeded()
{
    QListWidgetItem *item = currentPlaylistItem();
    if (item == nullptr || playlistWidget->viewport() == nullptr) {
        return;
    }

    const QRect itemRect = playlistWidget->visualItemRect(item);
    const QRect viewportRect = playlistWidget->viewport()->rect();
    if (itemRect.isValid()
        && (!viewportRect.contains(itemRect.topLeft()) || !viewportRect.contains(itemRect.bottomLeft()))) {
        playlistWidget->scrollToItem(item, QAbstractItemView::PositionAtCenter);
    }
}

QString MainWindow::collectionKey(const QString &type, const QString &value) const
{
    return value.isEmpty() ? type : QString("%1:%2").arg(type, value);
}

QString MainWindow::currentCollectionLabel() const
{
    if (activeCollectionType == "playlist") {
        return QString("Playlist: %1").arg(activeCollectionValue);
    }
    if (activeCollectionType == "artist") {
        return QString("Artist: %1").arg(activeCollectionValue);
    }
    if (activeCollectionType == "album") {
        return QString("Album: %1").arg(activeCollectionValue);
    }
    if (activeCollectionType == "tag") {
        return QString("Tag: %1").arg(activeCollectionValue);
    }
    return "Library";
}

QString MainWindow::emptyStateMessage(int visibleCount) const
{
    if (visibleCount > 0) {
        return QString();
    }
    if (playlist.isEmpty()) {
        return "No songs yet.\nUse Add Folder to scan your music library.";
    }
    if (!searchEdit->text().trimmed().isEmpty()) {
        return QString("No songs match \"%1\" in %2.")
            .arg(searchEdit->text().trimmed(), currentCollectionLabel());
    }
    if (activeCollectionType == "playlist") {
        return "This playlist is empty.\nSelect a song from All Songs, then use Add to.";
    }
    if (activeCollectionType == "tag") {
        return "No songs have this tag yet.\nSelect a song and use Tags to organize it.";
    }
    return QString("No songs in %1.").arg(currentCollectionLabel());
}

void MainWindow::updateSearchHint(int visibleCount)
{
    if (searchHintLabel == nullptr) {
        return;
    }

    const QString keyword = searchEdit->text().trimmed();
    if (keyword.isEmpty()) {
        searchHintLabel->setText(QString("Showing %1").arg(currentCollectionLabel()));
        return;
    }

    searchHintLabel->setText(QString("Searching \"%1\" in %2 - %3 result(s)")
        .arg(keyword)
        .arg(currentCollectionLabel())
        .arg(visibleCount));
}

void MainWindow::updateLibraryActionState()
{
    const bool hasTrack = selectedOrCurrentTrackIndex() >= 0;
    if (addToPlaylistButton != nullptr) {
        addToPlaylistButton->setEnabled(hasTrack);
        addToPlaylistButton->setToolTip(hasTrack ? "Add selected song to a playlist" : "Select a song first");
    }
    if (tagButton != nullptr) {
        tagButton->setEnabled(hasTrack);
        tagButton->setToolTip(hasTrack ? "Edit custom tags for the selected song" : "Select a song first");
    }
    if (managePlaylistButton != nullptr) {
        const bool canManagePlaylist = activeCollectionType == "playlist";
        managePlaylistButton->setEnabled(canManagePlaylist);
        managePlaylistButton->setToolTip(canManagePlaylist
            ? "Rename or delete the current playlist"
            : "Choose a playlist to manage it");
    }
}

void MainWindow::refreshCollectionCombo()
{
    if (collectionCombo == nullptr) {
        return;
    }

    const QString activeKey = collectionKey(activeCollectionType, activeCollectionValue);
    updatingCollectionCombo = true;
    collectionCombo->clear();
    collectionCombo->addItem("All Songs", collectionKey("all"));

    for (const UserPlaylist &userPlaylist : userPlaylists) {
        collectionCombo->addItem(QString("Playlists / %1").arg(userPlaylist.name), collectionKey("playlist", userPlaylist.name));
    }

    QSet<QString> artists;
    QSet<QString> albums;
    QSet<QString> tags;
    for (const Track &track : playlist.tracks()) {
        if (!track.artist.trimmed().isEmpty()) {
            artists.insert(track.artist.trimmed());
        }
        if (!track.album.trimmed().isEmpty()) {
            albums.insert(track.album.trimmed());
        }
        for (const QString &tag : track.tags) {
            if (!tag.trimmed().isEmpty()) {
                tags.insert(tag.trimmed());
            }
        }
    }

    QStringList sortedArtists = artists.values();
    QStringList sortedAlbums = albums.values();
    QStringList sortedTags = tags.values();
    sortedArtists.sort(Qt::CaseInsensitive);
    sortedAlbums.sort(Qt::CaseInsensitive);
    sortedTags.sort(Qt::CaseInsensitive);

    for (const QString &artist : sortedArtists) {
        collectionCombo->addItem(QString("Artists / %1").arg(artist), collectionKey("artist", artist));
    }
    for (const QString &album : sortedAlbums) {
        collectionCombo->addItem(QString("Albums / %1").arg(album), collectionKey("album", album));
    }
    for (const QString &tag : sortedTags) {
        collectionCombo->addItem(QString("Tags / %1").arg(tag), collectionKey("tag", tag));
    }

    int activeIndex = collectionCombo->findData(activeKey);
    if (activeIndex < 0) {
        activeCollectionType = "all";
        activeCollectionValue.clear();
        activeIndex = 0;
    }
    collectionCombo->setCurrentIndex(activeIndex);
    updatingCollectionCombo = false;
    searchEdit->setPlaceholderText(QString("Search in %1").arg(currentCollectionLabel()));
}

bool MainWindow::trackVisibleInCurrentCollection(const Track &track) const
{
    if (activeCollectionType == "all") {
        return true;
    }
    if (activeCollectionType == "artist") {
        return track.artist.compare(activeCollectionValue, Qt::CaseInsensitive) == 0;
    }
    if (activeCollectionType == "album") {
        return track.album.compare(activeCollectionValue, Qt::CaseInsensitive) == 0;
    }
    if (activeCollectionType == "tag") {
        return track.tags.contains(activeCollectionValue, Qt::CaseInsensitive);
    }
    if (activeCollectionType == "playlist") {
        const QString absolutePath = QFileInfo(track.filePath).absoluteFilePath();
        for (const UserPlaylist &userPlaylist : userPlaylists) {
            if (userPlaylist.name.compare(activeCollectionValue, Qt::CaseInsensitive) == 0) {
                return userPlaylist.filePaths.contains(absolutePath);
            }
        }
        return false;
    }
    return true;
}

int MainWindow::selectedOrCurrentTrackIndex() const
{
    if (playlistWidget != nullptr && !playlistWidget->selectedItems().isEmpty()) {
        return playlistWidget->selectedItems().first()->data(Qt::UserRole).toInt();
    }
    const int currentIndex = playlist.currentIndex();
    return visibleTrackIndexes.contains(currentIndex) ? currentIndex : -1;
}

void MainWindow::createUserPlaylist()
{
    bool ok = false;
    const QString name = QInputDialog::getText(this, "New playlist", "Playlist name:", QLineEdit::Normal, QString(), &ok).trimmed();
    if (!ok || name.isEmpty()) {
        return;
    }
    for (const UserPlaylist &userPlaylist : userPlaylists) {
        if (userPlaylist.name.compare(name, Qt::CaseInsensitive) == 0) {
            QMessageBox::information(this, "Playlist", "A playlist with this name already exists. Choose a different name.");
            return;
        }
    }

    UserPlaylist userPlaylist;
    userPlaylist.name = name;
    const int trackIndex = selectedOrCurrentTrackIndex();
    const Track *track = playlist.trackAt(trackIndex);
    if (track != nullptr) {
        const QMessageBox::StandardButton choice = QMessageBox::question(
            this,
            "New playlist",
            QString("Add \"%1\" to this playlist now?").arg(track->displayTitle()),
            QMessageBox::Yes | QMessageBox::No,
            QMessageBox::Yes);
        if (choice == QMessageBox::Yes) {
            userPlaylist.filePaths.append(QFileInfo(track->filePath).absoluteFilePath());
        }
    }
    userPlaylists.push_back(userPlaylist);
    libraryStore.saveUserPlaylists(userPlaylists);
    activeCollectionType = "playlist";
    activeCollectionValue = name;
    refreshCollectionCombo();
    refreshPlaylist();
    showTransientFeedback(QString("Created playlist \"%1\"").arg(name));
}

void MainWindow::addSelectedTrackToPlaylist()
{
    const int trackIndex = selectedOrCurrentTrackIndex();
    const Track *track = playlist.trackAt(trackIndex);
    if (track == nullptr) {
        QMessageBox::information(this, "Playlist", "Select a song before adding it to a playlist.");
        return;
    }
    if (userPlaylists.isEmpty()) {
        createUserPlaylist();
        return;
    }

    QStringList names;
    for (const UserPlaylist &userPlaylist : userPlaylists) {
        names << userPlaylist.name;
    }

    bool ok = false;
    const QString name = QInputDialog::getItem(this, "Add to playlist", "Playlist:", names, 0, false, &ok);
    if (!ok || name.isEmpty()) {
        return;
    }

    const QString absolutePath = QFileInfo(track->filePath).absoluteFilePath();
    for (UserPlaylist &userPlaylist : userPlaylists) {
        if (userPlaylist.name == name) {
            if (!userPlaylist.filePaths.contains(absolutePath)) {
                userPlaylist.filePaths.append(absolutePath);
                libraryStore.saveUserPlaylists(userPlaylists);
            }
            activeCollectionType = "playlist";
            activeCollectionValue = name;
            refreshCollectionCombo();
            refreshPlaylist();
            showTransientFeedback(QString("Added to \"%1\"").arg(name));
            return;
        }
    }
}

void MainWindow::showPlaylistContextMenu(const QPoint &position)
{
    QListWidgetItem *item = playlistWidget->itemAt(position);
    if (item == nullptr) {
        return;
    }

    playlistWidget->setCurrentItem(item);
    item->setSelected(true);
    const int trackIndex = item->data(Qt::UserRole).toInt();
    const Track *track = playlist.trackAt(trackIndex);
    if (track == nullptr) {
        return;
    }

    QMenu menu(this);
    QAction *playAction = menu.addAction("Play");
    QAction *addAction = menu.addAction("Add to Playlist");
    QAction *tagAction = menu.addAction("Edit Tags");
    QAction *showAction = menu.addAction("Show in Folder");
    QAction *removeAction = nullptr;
    if (activeCollectionType == "playlist") {
        menu.addSeparator();
        removeAction = menu.addAction("Remove from This Playlist");
    }

    QAction *chosen = menu.exec(playlistWidget->viewport()->mapToGlobal(position));
    if (chosen == nullptr) {
        return;
    }
    if (chosen == playAction) {
        playTrackAt(trackIndex);
    } else if (chosen == addAction) {
        addSelectedTrackToPlaylist();
    } else if (chosen == tagAction) {
        editSelectedTrackTags();
    } else if (chosen == showAction) {
        showTrackInFolder(trackIndex);
    } else if (chosen == removeAction) {
        removeTrackFromActivePlaylist(trackIndex);
    }
}

void MainWindow::showPlaylistManageMenu()
{
    if (activeCollectionType != "playlist") {
        return;
    }

    QMenu menu(this);
    QAction *renameAction = menu.addAction("Rename Playlist");
    QAction *deleteAction = menu.addAction("Delete Playlist");
    QAction *chosen = menu.exec(managePlaylistButton->mapToGlobal(QPoint(0, managePlaylistButton->height())));
    if (chosen == renameAction) {
        renameActivePlaylist();
    } else if (chosen == deleteAction) {
        deleteActivePlaylist();
    }
}

void MainWindow::renameActivePlaylist()
{
    if (activeCollectionType != "playlist") {
        return;
    }

    bool ok = false;
    const QString newName = QInputDialog::getText(
        this,
        "Rename playlist",
        "Playlist name:",
        QLineEdit::Normal,
        activeCollectionValue,
        &ok).trimmed();
    if (!ok || newName.isEmpty() || newName == activeCollectionValue) {
        return;
    }

    for (const UserPlaylist &userPlaylist : userPlaylists) {
        if (userPlaylist.name.compare(newName, Qt::CaseInsensitive) == 0) {
            QMessageBox::information(this, "Playlist", "A playlist with this name already exists. Choose a different name.");
            return;
        }
    }

    for (UserPlaylist &userPlaylist : userPlaylists) {
        if (userPlaylist.name.compare(activeCollectionValue, Qt::CaseInsensitive) == 0) {
            userPlaylist.name = newName;
            activeCollectionValue = newName;
            libraryStore.saveUserPlaylists(userPlaylists);
            refreshCollectionCombo();
            refreshPlaylist();
            showTransientFeedback(QString("Renamed to \"%1\"").arg(newName));
            return;
        }
    }
}

void MainWindow::deleteActivePlaylist()
{
    if (activeCollectionType != "playlist") {
        return;
    }

    const QMessageBox::StandardButton choice = QMessageBox::warning(
        this,
        "Delete playlist",
        QString("Delete playlist \"%1\"?\nMusic files will not be deleted.").arg(activeCollectionValue),
        QMessageBox::Yes | QMessageBox::No,
        QMessageBox::No);
    if (choice != QMessageBox::Yes) {
        return;
    }

    for (int i = 0; i < userPlaylists.size(); ++i) {
        if (userPlaylists[i].name.compare(activeCollectionValue, Qt::CaseInsensitive) == 0) {
            userPlaylists.removeAt(i);
            activeCollectionType = "all";
            activeCollectionValue.clear();
            libraryStore.saveUserPlaylists(userPlaylists);
            refreshCollectionCombo();
            refreshPlaylist();
            showTransientFeedback("Playlist deleted");
            return;
        }
    }
}

void MainWindow::removeTrackFromActivePlaylist(int trackIndex)
{
    if (activeCollectionType != "playlist") {
        return;
    }
    const Track *track = playlist.trackAt(trackIndex);
    if (track == nullptr) {
        return;
    }

    const QString absolutePath = QFileInfo(track->filePath).absoluteFilePath();
    for (UserPlaylist &userPlaylist : userPlaylists) {
        if (userPlaylist.name.compare(activeCollectionValue, Qt::CaseInsensitive) != 0) {
            continue;
        }
        const int removed = userPlaylist.filePaths.removeAll(absolutePath);
        if (removed > 0) {
            libraryStore.saveUserPlaylists(userPlaylists);
            refreshCollectionCombo();
            refreshPlaylist();
            showTransientFeedback("Removed from playlist");
        }
        return;
    }
}

void MainWindow::showTrackInFolder(int trackIndex)
{
    const Track *track = playlist.trackAt(trackIndex);
    if (track == nullptr) {
        return;
    }

    const QFileInfo info(track->filePath);
    if (!info.exists()) {
        QMessageBox::warning(this, "Show in Folder", "This file cannot be accessed.");
        return;
    }

    if (QDesktopServices::openUrl(QUrl::fromLocalFile(info.absolutePath()))) {
        showTransientFeedback("Opened folder");
    }
}

void MainWindow::editSelectedTrackTags()
{
    const int trackIndex = selectedOrCurrentTrackIndex();
    Track *track = playlist.trackAt(trackIndex);
    if (track == nullptr) {
        QMessageBox::information(this, "Tags", "Select a song before editing tags.");
        return;
    }

    bool ok = false;
    const QString text = QInputDialog::getText(
        this,
        "Edit tags",
        "Tags, separated by commas:",
        QLineEdit::Normal,
        track->tags.join(", "),
        &ok);
    if (!ok) {
        return;
    }

    QStringList tags;
    for (const QString &part : text.split(",", Qt::SkipEmptyParts)) {
        const QString tag = part.trimmed();
        if (!tag.isEmpty() && !tags.contains(tag, Qt::CaseInsensitive)) {
            tags.append(tag);
        }
    }

    track->tags = tags;
    saveLibrary();
    refreshCollectionCombo();
    refreshPlaylist();
    showTransientFeedback("Tags updated");
}

int MainWindow::nextIndexInCurrentView() const
{
    if (visibleTrackIndexes.isEmpty()) {
        return playbackStrategies[playbackStrategyIndex]->nextIndex(playlist.currentIndex(), playlist.count());
    }
    const int position = visibleTrackIndexes.indexOf(playlist.currentIndex());
    const int strategyIndex = playbackStrategies[playbackStrategyIndex]->nextIndex(position, visibleTrackIndexes.size());
    return strategyIndex >= 0 && strategyIndex < visibleTrackIndexes.size()
        ? visibleTrackIndexes[strategyIndex]
        : -1;
}

int MainWindow::previousIndexInCurrentView() const
{
    if (visibleTrackIndexes.isEmpty()) {
        return playlist.previousIndex();
    }
    int position = visibleTrackIndexes.indexOf(playlist.currentIndex());
    if (position < 0) {
        position = 0;
    }
    return position <= 0 ? visibleTrackIndexes.last() : visibleTrackIndexes[position - 1];
}

void MainWindow::scanFolder()
{
    const QString folder = QFileDialog::getExistingDirectory(this, "Add music folder");
    if (folder.isEmpty()) {
        return;
    }

    statusLabel->setText("Scanning...");
    const QVector<Track> foundTracks = scanAudioFiles(folder);
    const int added = playlist.addTracks(foundTracks);
    saveLibrary();
    refreshCollectionCombo();
    refreshPlaylist();
    updateCurrentTrackView();
    statusLabel->setText(QString("Added %1 songs").arg(added));
    showTransientFeedback(QString("Added %1 song(s)").arg(added));
}

void MainWindow::transcodeSelectedTrack()
{
    int index = playlist.currentIndex();
    if (!playlistWidget->selectedItems().isEmpty()) {
        index = playlistWidget->selectedItems().first()->data(Qt::UserRole).toInt();
    }

    const QString program = ffmpegPath();
    if (program.isEmpty()) {
        QMessageBox::warning(this, "Transcode", "ffmpeg.exe was not found in the ffmpeg/bin folder.");
        return;
    }

    const Track *track = playlist.trackAt(index);
    const QString initialPath = track == nullptr ? QString() : track->filePath;
    const qint64 initialDuration = track == nullptr ? 0 : track->durationMs;
    TranscodeDialog dialog(program, initialPath, initialDuration, nightMode, this);
    dialog.exec();
}

void MainWindow::playTrackAt(int index)
{
    const Track *track = playlist.trackAt(index);
    if (track == nullptr) {
        return;
    }

    const bool sameTrack = QFileInfo(track->filePath).absoluteFilePath() == currentPlaybackPath;
    currentPlaybackPath = QFileInfo(track->filePath).absoluteFilePath();
    keyboardSeekTargetMs = -1;
    keyboardSeekWasPlaying = false;
    pendingRestorePositionMs = -1;
    lastSavedPlaybackPositionMs = -1;
    playlist.setCurrentIndex(index);
    audioPlayer.setSource(track->filePath);
    if (!sameTrack) {
        updateCoverImage(QImage());
    }
    audioPlayer.play();
    refreshPlaylist();
    updateCurrentTrackView();
    loadLyricsForCurrentTrack();
}

void MainWindow::playCurrent()
{
    int index = playlist.currentIndex();
    if (!visibleTrackIndexes.isEmpty() && !visibleTrackIndexes.contains(index)) {
        index = visibleTrackIndexes.first();
    } else if (index < 0) {
        index = !visibleTrackIndexes.isEmpty() ? visibleTrackIndexes.first() : 0;
    }
    playTrackAt(index);
}

void MainWindow::playNext()
{
    const int index = nextIndexInCurrentView();
    if (index >= 0) {
        playTrackAt(index);
    }
}

void MainWindow::playPrevious()
{
    const int index = previousIndexInCurrentView();
    if (index >= 0) {
        playTrackAt(index);
    }
}

void MainWindow::updateCurrentTrackView()
{
    const Track *track = playlist.currentTrack();
    if (track == nullptr) {
        titleLabel->setText("No song selected");
        artistLabel->setText("Artist / Album");
        updateCoverImage(QImage());
        currentLyrics.clear();
        lyricsWidget->setMessage("Lyrics will appear here");
        return;
    }

    titleLabel->setText(track->displayTitle());
    artistLabel->setText(track->subtitle());
    QList<QListWidgetItem *> selectedItems = playlistWidget->selectedItems();
    for (QListWidgetItem *item : selectedItems) {
        item->setSelected(false);
    }
    for (int row = 0; row < playlistWidget->count(); ++row) {
        QListWidgetItem *item = playlistWidget->item(row);
        if (item->data(Qt::UserRole).toInt() == playlist.currentIndex()) {
            item->setSelected(true);
            break;
        }
    }
}

void MainWindow::updateCoverImage(const QImage &coverImage)
{
    currentCoverImage = coverImage;
    QPixmap pixmap;
    if (!coverImage.isNull()) {
        pixmap = QPixmap::fromImage(coverImage);
    } else {
        pixmap = QPixmap(assetPath(QString("img/album-placeholder-%1.jpg").arg(themeName())));
    }

    if (pixmap.isNull()) {
        coverLabel->clear();
        coverLabel->setText("Cover");
        return;
    }

    coverLabel->setText("");
    coverLabel->setPixmap(pixmap.scaled(coverLabel->size(), Qt::KeepAspectRatioByExpanding, Qt::SmoothTransformation));
}

void MainWindow::loadLyricsForCurrentTrack()
{
    const Track *track = playlist.currentTrack();
    if (track == nullptr) {
        currentLyrics.clear();
        lyricsWidget->setLoadButtonVisible(false);
        lyricsWidget->setMessage("Lyrics will appear here");
        return;
    }

    const QString lrcPath = lrcPathForTrack(*track);
    if (lrcPath.isEmpty()) {
        currentLyrics.clear();
        lyricsWidget->setUserOffsetMs(track->lyricsOffsetMs);
        lyricsWidget->setLoadButtonVisible(true);
        lyricsWidget->setMessage("No LRC file found");
        return;
    }

    currentLyrics = LrcParser::parseFile(lrcPath);
    lyricsWidget->setUserOffsetMs(track->lyricsOffsetMs);
    if (currentLyrics.isEmpty()) {
        lyricsWidget->setLoadButtonVisible(true);
        lyricsWidget->setMessage("No timed lyrics found");
    } else {
        lyricsWidget->setLoadButtonVisible(false);
        lyricsWidget->setLyrics(currentLyrics);
    }
}

void MainWindow::chooseLyricsFileForCurrentTrack()
{
    Track *track = playlist.trackAt(playlist.currentIndex());
    if (track == nullptr) {
        QMessageBox::information(this, "Lyrics", "Select or play a song before loading lyrics.");
        return;
    }

    const QFileInfo trackInfo(track->filePath);
    const QString startDir = !track->lrcPath.trimmed().isEmpty()
        ? QFileInfo(track->lrcPath).absolutePath()
        : trackInfo.absolutePath();
    const QString filePath = QFileDialog::getOpenFileName(
        this,
        "Load LRC File",
        startDir,
        "LRC lyrics (*.lrc);;All files (*.*)");
    if (filePath.isEmpty()) {
        return;
    }

    applyLyricsFileToCurrentTrack(filePath);
}

void MainWindow::applyLyricsFileToCurrentTrack(const QString &filePath)
{
    Track *track = playlist.trackAt(playlist.currentIndex());
    if (track == nullptr) {
        return;
    }

    const QFileInfo info(filePath);
    if (!info.exists() || !info.isReadable()) {
        QMessageBox::warning(this, "Lyrics", "This LRC file cannot be accessed.");
        return;
    }

    currentLyrics = LrcParser::parseFile(info.absoluteFilePath());
    track->lrcPath = info.absoluteFilePath();
    lyricsWidget->setUserOffsetMs(track->lyricsOffsetMs);
    if (currentLyrics.isEmpty()) {
        lyricsWidget->setLoadButtonVisible(true);
        lyricsWidget->setMessage("No timed lyrics found");
        QMessageBox::warning(this, "Lyrics", "This file does not contain timed LRC lyrics.");
    } else {
        lyricsWidget->setLoadButtonVisible(false);
        lyricsWidget->setLyrics(currentLyrics);
        showTransientFeedback("Lyrics loaded");
    }
    saveLibrary();
}

void MainWindow::adjustLyricsOffset(int deltaMs)
{
    Track *track = playlist.trackAt(playlist.currentIndex());
    if (track == nullptr) {
        return;
    }

    track->lyricsOffsetMs += deltaMs;
    lyricsWidget->setUserOffsetMs(track->lyricsOffsetMs);
    saveLibrary();
    const double seconds = track->lyricsOffsetMs / 1000.0;
    statusLabel->setText(QString("Lyrics offset %1s").arg(seconds, 0, 'f', 1));
    showTransientFeedback(QString("Lyrics offset %1s").arg(seconds, 0, 'f', 1));
}

QString MainWindow::lrcPathForTrack(const Track &track) const
{
    if (!track.lrcPath.trimmed().isEmpty()) {
        const QFileInfo customInfo(track.lrcPath);
        if (customInfo.exists() && customInfo.isReadable()) {
            return customInfo.absoluteFilePath();
        }
    }

    const QFileInfo info(track.filePath);
    const QString lrcPath = QDir(info.absolutePath()).filePath(QString("%1.lrc").arg(info.completeBaseName()));
    return QFileInfo::exists(lrcPath) ? lrcPath : QString();
}

QString MainWindow::ffmpegPath() const
{
    const QString appLocalPath = QDir(QCoreApplication::applicationDirPath()).filePath("ffmpeg/bin/ffmpeg.exe");
    if (QFileInfo::exists(appLocalPath)) {
        return appLocalPath;
    }

    const QString sourcePath = QDir(QString::fromUtf8(PROJECT_ROOT_DIR)).filePath("ffmpeg/bin/ffmpeg.exe");
    if (QFileInfo::exists(sourcePath)) {
        return sourcePath;
    }

    return QString();
}

void MainWindow::updateTimeLabel(qint64 positionMs, qint64 durationMs)
{
    timeLabel->setText(QString("%1 / %2").arg(formatTime(positionMs), formatTime(durationMs)));
}

QVector<Track> MainWindow::scanAudioFiles(const QString &folderPath) const
{
    static const QStringList patterns = {"*.mp3", "*.flac", "*.wav", "*.m4a", "*.aac", "*.ogg"};
    QVector<Track> tracks;
    QDirIterator iterator(folderPath, patterns, QDir::Files, QDirIterator::Subdirectories);
    while (iterator.hasNext()) {
        const QString path = QFileInfo(iterator.next()).absoluteFilePath();
        Track track;
        track.filePath = path;
        readTrackMetadata(&track);
        tracks.push_back(track);
    }
    return tracks;
}

bool MainWindow::readTrackMetadata(Track *track) const
{
    if (track == nullptr || track->filePath.trimmed().isEmpty()) {
        return false;
    }

    QMediaPlayer player;
    QEventLoop loop;
    QTimer timeout;
    timeout.setSingleShot(true);

    bool finished = false;
    auto quitOnce = [&]() {
        if (!finished) {
            finished = true;
            loop.quit();
        }
    };

    connect(&player, &QMediaPlayer::metaDataChanged, &loop, quitOnce);
    connect(&player, &QMediaPlayer::durationChanged, &loop, quitOnce);
    connect(&player, &QMediaPlayer::mediaStatusChanged, &loop, [&](QMediaPlayer::MediaStatus status) {
        if (status == QMediaPlayer::LoadedMedia || status == QMediaPlayer::InvalidMedia) {
            quitOnce();
        }
    });
    connect(&timeout, &QTimer::timeout, &loop, quitOnce);

    player.setSource(QUrl::fromLocalFile(track->filePath));
    timeout.start(1200);
    loop.exec();

    const QMediaMetaData metaData = player.metaData();
    const QString title = metaData.stringValue(QMediaMetaData::Title).trimmed();
    QString artist = metaData.stringValue(QMediaMetaData::Author).trimmed();
    if (artist.isEmpty()) {
        artist = metaData.stringValue(QMediaMetaData::ContributingArtist).trimmed();
    }
    const QString album = metaData.stringValue(QMediaMetaData::AlbumTitle).trimmed();
    const qint64 durationMs = player.duration();

    bool changed = false;
    if (!title.isEmpty() && track->title != title) {
        track->title = title;
        changed = true;
    }
    if (!artist.isEmpty() && track->artist != artist) {
        track->artist = artist;
        changed = true;
    }
    if (!album.isEmpty() && track->album != album) {
        track->album = album;
        changed = true;
    }
    if (durationMs > 0 && track->durationMs != durationMs) {
        track->durationMs = durationMs;
        changed = true;
    }

    return changed;
}

QString MainWindow::formatTime(qint64 milliseconds) const
{
    const qint64 totalSeconds = qMax<qint64>(0, milliseconds / 1000);
    const qint64 minutes = totalSeconds / 60;
    const qint64 seconds = totalSeconds % 60;
    return QString("%1:%2")
        .arg(minutes, 2, 10, QLatin1Char('0'))
        .arg(seconds, 2, 10, QLatin1Char('0'));
}

bool MainWindow::matchesSearch(const Track &track) const
{
    const QString keyword = searchEdit->text().trimmed();
    if (keyword.isEmpty()) {
        return true;
    }

    const bool tagMatched = std::any_of(track.tags.cbegin(), track.tags.cend(), [&](const QString &tag) {
        return tag.contains(keyword, Qt::CaseInsensitive);
    });

    return track.displayTitle().contains(keyword, Qt::CaseInsensitive)
        || track.artist.contains(keyword, Qt::CaseInsensitive)
        || track.album.contains(keyword, Qt::CaseInsensitive)
        || tagMatched
        || QFileInfo(track.filePath).fileName().contains(keyword, Qt::CaseInsensitive);
}

void MainWindow::applyBaseStyle()
{
    themeButton->setText("");
    themeButton->setIcon(QIcon(assetPath(QString("img/ui-icons/%1/%2.256.png").arg(themeName(), nightMode ? "night" : "day"))));
    themeButton->setIconSize(QSize(22, 22));
    themeButton->setToolTip(nightMode ? "Switch to day mode" : "Switch to night mode");
    scanButton->setIcon(QIcon(assetPath(QString("img/ui-icons/%1/add-folder.256.png").arg(themeName()))));
    scanButton->setIconSize(QSize(18, 18));
    transcodeButton->setIcon(QIcon(assetPath(QString("img/ui-icons/%1/transcode.256.png").arg(themeName()))));
    transcodeButton->setIconSize(QSize(18, 18));
    helpButton->setIcon(QIcon(assetPath(QString("img/ui-icons/%1/help.256.png").arg(themeName()))));
    helpButton->setIconSize(QSize(18, 18));
    aboutButton->setIcon(QIcon(assetPath(QString("img/ui-icons/%1/about.256.png").arg(themeName()))));
    aboutButton->setIconSize(QSize(18, 18));
    polishComboPopup(collectionCombo, nightMode ? "#1f2530" : "#ffffff");
    lyricsWidget->setNightMode(nightMode);

    if (!nightMode) {
        setStyleSheet(R"(
        QWidget {
            background: #eef3f8;
            color: #172033;
            font-family: "Segoe UI", "Microsoft YaHei UI", sans-serif;
            font-size: 14px;
        }
        QWidget#sidePanel, QWidget#contentPanel, QWidget#playbackPanel {
            background: #ffffff;
            border: 1px solid #d9e2ef;
            border-radius: 14px;
        }
        QLabel#sectionTitle {
            color: #23324d;
            font-size: 18px;
            font-weight: 700;
            background: transparent;
        }
        QLabel#emptyLibraryLabel {
            background: #f7fafc;
            border: 1px dashed #cbd8e8;
            border-radius: 12px;
            color: #6c7c94;
            padding: 18px;
        }
        QLabel#searchHintLabel {
            background: transparent;
            color: #7b8aa0;
            font-size: 12px;
        }
        QLineEdit {
            background: #f6f9fd;
            border: 1px solid #d8e1ee;
            border-radius: 10px;
            padding: 9px 12px;
            selection-background-color: #b9d4ff;
        }
        QLineEdit:focus {
            border-color: #6aa0f8;
            background: #ffffff;
        }
        QComboBox {
            background: #f6f9fd;
            border: 1px solid #d8e1ee;
            border-radius: 12px;
            padding: 8px 30px 8px 12px;
            color: #263850;
            combobox-popup: 1;
        }
        QComboBox:hover {
            border-color: #9fb8d6;
            background: #ffffff;
        }
        QComboBox::drop-down {
            border: none;
            width: 26px;
            subcontrol-origin: padding;
            subcontrol-position: top right;
            background: transparent;
        }
        QComboBox::down-arrow {
            image: none;
            width: 0px;
            height: 0px;
            border-left: 5px solid transparent;
            border-right: 5px solid transparent;
            border-top: 6px solid #61718a;
            margin-right: 9px;
        }
        QComboBox QAbstractItemView {
            background: #ffffff;
            border: none;
            border-radius: 12px;
            padding: 6px;
            color: #263850;
            selection-background-color: #edf5ff;
            selection-color: #12345d;
            outline: none;
        }
        QComboBox QAbstractItemView::viewport {
            background: #ffffff;
            border: none;
            border-radius: 12px;
        }
        QComboBox QAbstractItemView::item {
            color: #263850;
            background: transparent;
            min-height: 28px;
            padding: 6px 10px;
            border-radius: 8px;
        }
        QComboBox QAbstractItemView::item:selected {
            background: #edf5ff;
            color: #12345d;
        }
        QMenu {
            background: #ffffff;
            border: 1px solid #d8e1ee;
            border-radius: 10px;
            padding: 6px;
            color: #263850;
        }
        QMenu::item {
            padding: 8px 28px 8px 12px;
            border-radius: 8px;
        }
        QMenu::item:selected {
            background: #edf5ff;
            color: #12345d;
        }
        QMenu::separator {
            height: 1px;
            background: #e3eaf3;
            margin: 5px 8px;
        }
        QListWidget {
            background: transparent;
            border: none;
            outline: none;
            padding: 2px;
        }
        LyricsWidget#lyricsWidget {
            background: #ffffff;
            border: 1px solid #d9e2ef;
            border-radius: 14px;
        }
        QListWidget::item {
            background: transparent;
            border-radius: 10px;
            border: 1px solid transparent;
            margin: 1px 0;
            padding: 8px 10px;
            color: #4d5f7a;
        }
        QListWidget::item:hover {
            background: #eef5ff;
        }
        QListWidget::item:selected {
            background: #edf5ff;
            color: #12345d;
            border: 1px solid #cfe0f5;
        }
        QWidget#trackItemWidget {
            background: transparent;
            border: 1px solid transparent;
            border-radius: 10px;
        }
        QWidget#trackItemWidget[active="true"] {
            background: #e8f2ff;
            border-color: #bfd7f6;
        }
        QLabel#trackTitleLabel {
            background: transparent;
            color: #20304a;
            font-size: 14px;
            font-weight: 700;
        }
        QLabel#trackSubtitleLabel {
            background: transparent;
            color: #6c7c94;
            font-size: 12px;
        }
        QLabel#trackDurationLabel {
            background: transparent;
            color: #7b8aa0;
            font-size: 12px;
            font-weight: 600;
        }
        QWidget#trackItemWidget[active="true"] QLabel#trackTitleLabel {
            color: #174ea6;
        }
        QWidget#trackItemWidget[active="true"] QLabel#trackSubtitleLabel,
        QWidget#trackItemWidget[active="true"] QLabel#trackDurationLabel {
            color: #42658f;
        }
        QScrollBar:vertical {
            background: transparent;
            border: none;
            border-radius: 6px;
            width: 12px;
            margin: 4px 0 4px 0;
        }
        QScrollBar::handle:vertical {
            background: #c8d5e6;
            border: none;
            border-radius: 6px;
            min-height: 32px;
            margin: 1px 2px;
        }
        QScrollBar::handle:vertical:hover {
            background: #9eb6d4;
        }
        QScrollBar:horizontal {
            background: transparent;
            border: none;
            border-radius: 6px;
            height: 12px;
            margin: 0 4px 0 4px;
        }
        QScrollBar::handle:horizontal {
            background: #c8d5e6;
            border: none;
            border-radius: 6px;
            min-width: 32px;
            margin: 2px 1px;
        }
        QScrollBar::handle:horizontal:hover {
            background: #9eb6d4;
        }
        QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical,
        QScrollBar::add-page:vertical, QScrollBar::sub-page:vertical,
        QScrollBar::add-line:horizontal, QScrollBar::sub-line:horizontal,
        QScrollBar::add-page:horizontal, QScrollBar::sub-page:horizontal {
            background: transparent;
            border: none;
            width: 0;
            height: 0;
        }
        QScrollBar::corner {
            background: transparent;
            border: none;
        }
        QPushButton {
            background: #f8fbff;
            border: 1px solid #d4dfec;
            border-radius: 10px;
            padding: 8px 13px;
            color: #22314a;
            font-weight: 600;
        }
        QPushButton:hover {
            background: #edf5ff;
            border-color: #8fb8f4;
        }
        QPushButton:pressed {
            background: #d8e9ff;
        }
        QPushButton:disabled {
            color: #9aa8bb;
            background: #eef2f6;
            border-color: #e0e7f0;
        }
        QPushButton#primaryActionButton {
            background: #1f6f78;
            border-color: #1f6f78;
            color: #ffffff;
            border-radius: 12px;
            padding: 9px 16px;
        }
        QPushButton#primaryActionButton:hover {
            background: #185f68;
            border-color: #185f68;
        }
        QPushButton#secondaryActionButton {
            background: #ffffff;
            border-color: #d7e0ec;
            color: #31415d;
            border-radius: 12px;
            padding: 9px 15px;
        }
        QPushButton#secondaryActionButton:hover {
            background: #f3f7fb;
            border-color: #9fb2ca;
        }
        QPushButton#compactActionButton {
            background: #f7fafc;
            border-color: #d7e0ec;
            color: #40516c;
            border-radius: 9px;
            padding: 7px 8px;
            font-size: 11px;
        }
        QPushButton#compactActionButton:hover {
            background: #eef5ff;
            border-color: #9fb8d6;
        }
        QPushButton#compactActionButton:disabled {
            color: #a5b1c2;
            background: #f2f5f8;
            border-color: #e0e7f0;
        }
        QPushButton#playbackIconButton {
            min-width: 38px;
            width: 38px;
            height: 38px;
            padding: 0;
            border-radius: 19px;
            background: transparent;
            border-color: transparent;
        }
        QPushButton#playbackIconButton:hover {
            background: #edf4fb;
            border-color: #d5e1ee;
        }
        QPushButton#playbackIconButton:pressed {
            background: #dce9f7;
        }
        QPushButton#primaryPlaybackButton {
            min-width: 50px;
            width: 50px;
            height: 44px;
            padding: 0;
            border-radius: 22px;
            background: #2563eb;
            border-color: #2563eb;
        }
        QPushButton#primaryPlaybackButton:hover {
            background: #1d4ed8;
            border-color: #1d4ed8;
        }
        QPushButton#primaryPlaybackButton:pressed {
            background: #1e40af;
        }
        QLabel#titleLabel {
            background: transparent;
            color: #101828;
            font-size: 30px;
            font-weight: 700;
        }
        QLabel#artistLabel {
            background: transparent;
            color: #61718a;
        }
        QLabel#hintLabel, QLabel#timeLabel {
            background: transparent;
            border: none;
            color: #61718a;
        }
        QLabel#feedbackLabel {
            background: transparent;
            border: none;
            color: #1f6f78;
            font-size: 12px;
            font-weight: 700;
        }
        QPushButton#themeToggleButton {
            min-width: 0;
            width: 42px;
            height: 42px;
            padding: 0;
            border-radius: 21px;
        }
        QLabel#coverLabel {
            background: #ffffff;
            border: 1px solid #d9e2ef;
            border-radius: 16px;
            color: #61718a;
        }
        QSlider::groove:horizontal {
            height: 7px;
            background: #dce5f1;
            border-radius: 4px;
        }
        QSlider {
            background: transparent;
            border: none;
        }
        QSlider::handle:horizontal {
            width: 16px;
            margin: -5px 0;
            border-radius: 8px;
            background: #3b82f6;
            border: 2px solid #ffffff;
        }
        QSlider::handle:horizontal:hover {
            background: #2563eb;
        }
        QSlider#volumeSlider::groove:horizontal {
            height: 5px;
            background: #e2e8f0;
            border-radius: 3px;
        }
        QSlider#volumeSlider::handle:horizontal {
            width: 14px;
            margin: -5px 0;
            border-radius: 7px;
            background: #1f6f78;
            border: 2px solid #ffffff;
        }
        QSplitter::handle {
            background: transparent;
        }
    )");
        return;
    }

    setStyleSheet(R"(
        QWidget {
            background: #10131a;
            color: #edf2f7;
            font-family: "Segoe UI", "Microsoft YaHei UI", sans-serif;
            font-size: 14px;
        }
        QWidget#sidePanel, QWidget#contentPanel, QWidget#playbackPanel {
            background: #171b24;
            border: 1px solid #252c39;
            border-radius: 14px;
        }
        QLabel#sectionTitle {
            color: #f4f7fb;
            font-size: 18px;
            font-weight: 700;
            background: transparent;
        }
        QLabel#emptyLibraryLabel {
            background: #1b212c;
            border: 1px dashed #344052;
            border-radius: 12px;
            color: #9daabd;
            padding: 18px;
        }
        QLabel#searchHintLabel {
            background: transparent;
            color: #8995a8;
            font-size: 12px;
        }
        QLineEdit, QListWidget {
            background: #1f2530;
            border: 1px solid #303847;
            border-radius: 10px;
            padding: 9px 12px;
            selection-background-color: #385f9f;
        }
        QLineEdit:focus {
            border-color: #7da8f8;
            background: #202838;
        }
        QComboBox {
            background: #1f2530;
            border: 1px solid #303847;
            border-radius: 12px;
            padding: 8px 30px 8px 12px;
            color: #e8edf5;
            combobox-popup: 1;
        }
        QComboBox:hover {
            background: #242d3b;
            border-color: #52647d;
        }
        QComboBox::drop-down {
            border: none;
            width: 26px;
            subcontrol-origin: padding;
            subcontrol-position: top right;
            background: transparent;
        }
        QComboBox::down-arrow {
            image: none;
            width: 0px;
            height: 0px;
            border-left: 5px solid transparent;
            border-right: 5px solid transparent;
            border-top: 6px solid #aeb8c8;
            margin-right: 9px;
        }
        QComboBox QAbstractItemView {
            background: #1f2530;
            border: none;
            border-radius: 12px;
            padding: 6px;
            color: #e8edf5;
            selection-background-color: #263957;
            selection-color: #ffffff;
            outline: none;
        }
        QComboBox QAbstractItemView::viewport {
            background: #1f2530;
            border: none;
            border-radius: 12px;
        }
        QComboBox QAbstractItemView::item {
            color: #e8edf5;
            background: transparent;
            min-height: 28px;
            padding: 6px 10px;
            border-radius: 8px;
        }
        QComboBox QAbstractItemView::item:selected {
            background: #263957;
            color: #ffffff;
        }
        QMenu {
            background: #1f2530;
            border: 1px solid #303847;
            border-radius: 10px;
            padding: 6px;
            color: #e8edf5;
        }
        QMenu::item {
            padding: 8px 28px 8px 12px;
            border-radius: 8px;
        }
        QMenu::item:selected {
            background: #263957;
            color: #ffffff;
        }
        QMenu::separator {
            height: 1px;
            background: #303847;
            margin: 5px 8px;
        }
        QListWidget {
            background: transparent;
            border: none;
            outline: none;
            padding: 2px;
        }
        LyricsWidget#lyricsWidget {
            background: #141821;
            border: 1px solid #252c39;
            border-radius: 14px;
        }
        QListWidget::item {
            background: transparent;
            border-radius: 10px;
            border: 1px solid transparent;
            margin: 1px 0;
            padding: 8px 10px;
            color: #d6deea;
        }
        QListWidget::item:hover {
            background: #222a37;
        }
        QListWidget::item:selected {
            background: #202b3a;
            color: #ffffff;
            border: 1px solid #344259;
        }
        QWidget#trackItemWidget {
            background: transparent;
            border: 1px solid transparent;
            border-radius: 10px;
        }
        QWidget#trackItemWidget[active="true"] {
            background: #23344f;
            border-color: #3d5f94;
        }
        QLabel#trackTitleLabel {
            background: transparent;
            color: #eef4ff;
            font-size: 14px;
            font-weight: 700;
        }
        QLabel#trackSubtitleLabel {
            background: transparent;
            color: #9daabd;
            font-size: 12px;
        }
        QLabel#trackDurationLabel {
            background: transparent;
            color: #9aa7ba;
            font-size: 12px;
            font-weight: 600;
        }
        QWidget#trackItemWidget[active="true"] QLabel#trackTitleLabel {
            color: #ffffff;
        }
        QWidget#trackItemWidget[active="true"] QLabel#trackSubtitleLabel,
        QWidget#trackItemWidget[active="true"] QLabel#trackDurationLabel {
            color: #b8caff;
        }
        QScrollBar:vertical {
            background: transparent;
            border: none;
            border-radius: 6px;
            width: 12px;
            margin: 4px 0 4px 0;
        }
        QScrollBar::handle:vertical {
            background: #3a4658;
            border: none;
            border-radius: 6px;
            min-height: 32px;
            margin: 1px 2px;
        }
        QScrollBar::handle:vertical:hover {
            background: #53657c;
        }
        QScrollBar:horizontal {
            background: transparent;
            border: none;
            border-radius: 6px;
            height: 12px;
            margin: 0 4px 0 4px;
        }
        QScrollBar::handle:horizontal {
            background: #3a4658;
            border: none;
            border-radius: 6px;
            min-width: 32px;
            margin: 2px 1px;
        }
        QScrollBar::handle:horizontal:hover {
            background: #53657c;
        }
        QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical,
        QScrollBar::add-page:vertical, QScrollBar::sub-page:vertical,
        QScrollBar::add-line:horizontal, QScrollBar::sub-line:horizontal,
        QScrollBar::add-page:horizontal, QScrollBar::sub-page:horizontal {
            background: transparent;
            border: none;
            width: 0;
            height: 0;
        }
        QScrollBar::corner {
            background: transparent;
            border: none;
        }
        QPushButton {
            background: #202631;
            border: 1px solid #323b4c;
            border-radius: 10px;
            padding: 8px 13px;
            color: #e8edf5;
            font-weight: 600;
        }
        QPushButton:hover {
            background: #2a3342;
            border-color: #61799f;
        }
        QPushButton:pressed {
            background: #314466;
        }
        QPushButton:disabled {
            color: #697386;
            background: #1a1f29;
            border-color: #262d39;
        }
        QPushButton#primaryActionButton {
            background: #4f8f96;
            border-color: #4f8f96;
            color: #ffffff;
            border-radius: 12px;
            padding: 9px 16px;
        }
        QPushButton#primaryActionButton:hover {
            background: #64a4aa;
            border-color: #64a4aa;
        }
        QPushButton#secondaryActionButton {
            background: #1b212c;
            border-color: #303847;
            color: #e8edf5;
            border-radius: 12px;
            padding: 9px 15px;
        }
        QPushButton#secondaryActionButton:hover {
            background: #242d3b;
            border-color: #52647d;
        }
        QPushButton#compactActionButton {
            background: #1b212c;
            border-color: #303847;
            color: #dce5f2;
            border-radius: 9px;
            padding: 7px 8px;
            font-size: 11px;
        }
        QPushButton#compactActionButton:hover {
            background: #242d3b;
            border-color: #52647d;
        }
        QPushButton#compactActionButton:disabled {
            color: #687385;
            background: #1a1f29;
            border-color: #262d39;
        }
        QPushButton#playbackIconButton {
            min-width: 38px;
            width: 38px;
            height: 38px;
            padding: 0;
            border-radius: 19px;
            background: transparent;
            border-color: transparent;
        }
        QPushButton#playbackIconButton:hover {
            background: #242d3b;
            border-color: #303847;
        }
        QPushButton#playbackIconButton:pressed {
            background: #2f3b4e;
        }
        QPushButton#primaryPlaybackButton {
            min-width: 50px;
            width: 50px;
            height: 44px;
            padding: 0;
            border-radius: 22px;
            background: #8fb3ff;
            border-color: #8fb3ff;
        }
        QPushButton#primaryPlaybackButton:hover {
            background: #a9c4ff;
            border-color: #a9c4ff;
        }
        QPushButton#primaryPlaybackButton:pressed {
            background: #6f9df3;
        }
        QLabel#titleLabel {
            background: transparent;
            color: #ffffff;
            font-size: 30px;
            font-weight: 700;
        }
        QLabel#artistLabel {
            background: transparent;
            color: #aeb8c8;
        }
        QLabel#hintLabel, QLabel#timeLabel {
            background: transparent;
            border: none;
            color: #aeb8c8;
        }
        QLabel#feedbackLabel {
            background: transparent;
            border: none;
            color: #8fb3ff;
            font-size: 12px;
            font-weight: 700;
        }
        QPushButton#themeToggleButton {
            min-width: 0;
            width: 42px;
            height: 42px;
            padding: 0;
            border-radius: 21px;
        }
        QLabel#coverLabel {
            background: #1f2530;
            border: 1px solid #303847;
            border-radius: 16px;
            color: #aeb8c8;
        }
        QSlider::groove:horizontal {
            height: 7px;
            background: #303847;
            border-radius: 4px;
        }
        QSlider {
            background: transparent;
            border: none;
        }
        QSlider::handle:horizontal {
            width: 16px;
            margin: -5px 0;
            border-radius: 8px;
            background: #8fb3ff;
            border: 2px solid #171b24;
        }
        QSlider::handle:horizontal:hover {
            background: #a9c4ff;
        }
        QSlider#volumeSlider::groove:horizontal {
            height: 5px;
            background: #293241;
            border-radius: 3px;
        }
        QSlider#volumeSlider::handle:horizontal {
            width: 14px;
            margin: -5px 0;
            border-radius: 7px;
            background: #4f8f96;
            border: 2px solid #171b24;
        }
        QSplitter::handle {
            background: transparent;
        }
    )");
}
