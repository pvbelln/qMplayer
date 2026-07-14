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
    waveformWidget->setPosition(target);
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
    waveformWidget->setPosition(target);
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
    startWaveformDecode(track->filePath);
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
        waveformWidget->clear();
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
void MainWindow::updateTimeLabel(qint64 positionMs, qint64 durationMs)
{
    timeLabel->setText(QString("%1 / %2").arg(formatTime(positionMs), formatTime(durationMs)));
}
