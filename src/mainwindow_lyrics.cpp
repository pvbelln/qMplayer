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
void MainWindow::reloadLyricsForCurrentTrack()
{
    const Track *track = playlist.currentTrack();
    if (track == nullptr) {
        QMessageBox::information(this, "Lyrics", "Select or play a song before reloading lyrics.");
        return;
    }

    loadLyricsForCurrentTrack();
    showTransientFeedback(currentLyrics.isEmpty() ? "No lyrics loaded" : "Lyrics reloaded");
}
void MainWindow::showLyricsContextMenu(const QPoint &globalPosition)
{
    QMenu menu(this);
    QAction *reloadAction = menu.addAction("Reload Lyric");
    reloadAction->setIcon(QIcon(assetPath(QString("img/ui-icons/%1/reload-lyric.256.png").arg(themeName()))));
    QAction *loadAction = menu.addAction("Load Lyric");
    loadAction->setIcon(QIcon(assetPath(QString("img/ui-icons/%1/load-lyric.256.png").arg(themeName()))));

    const bool hasTrack = playlist.currentTrack() != nullptr;
    reloadAction->setEnabled(hasTrack);
    loadAction->setEnabled(hasTrack);

    QAction *chosen = menu.exec(globalPosition);
    if (chosen == reloadAction) {
        reloadLyricsForCurrentTrack();
    } else if (chosen == loadAction) {
        chooseLyricsFileForCurrentTrack();
    }
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
