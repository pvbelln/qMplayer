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
    waveformDecoder = new QAudioDecoder(this);
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
QString MainWindow::themeName() const
{
    return nightMode ? "night" : "day";
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
QString MainWindow::formatTime(qint64 milliseconds) const
{
    const qint64 totalSeconds = qMax<qint64>(0, milliseconds / 1000);
    const qint64 minutes = totalSeconds / 60;
    const qint64 seconds = totalSeconds % 60;
    return QString("%1:%2")
        .arg(minutes, 2, 10, QLatin1Char('0'))
        .arg(seconds, 2, 10, QLatin1Char('0'));
}
