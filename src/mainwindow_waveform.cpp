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
double sampleValue(const char *data, qsizetype sampleIndex, const QAudioFormat &format)
{
    switch (format.sampleFormat()) {
    case QAudioFormat::UInt8:
        return (static_cast<const quint8 *>(static_cast<const void *>(data))[sampleIndex] - 128) / 128.0;
    case QAudioFormat::Int16:
        return static_cast<const qint16 *>(static_cast<const void *>(data))[sampleIndex] / 32768.0;
    case QAudioFormat::Int32:
        return static_cast<const qint32 *>(static_cast<const void *>(data))[sampleIndex] / 2147483648.0;
    case QAudioFormat::Float:
        return static_cast<const float *>(static_cast<const void *>(data))[sampleIndex];
    default:
        return 0.0;
    }
}
}

void MainWindow::startWaveformDecode(const QString &filePath)
{
    if (waveformDecoder == nullptr || filePath.trimmed().isEmpty()) {
        return;
    }

    waveformDecoder->stop();
    waveformBlocks.clear();
    waveformAccumulatedEnergy = 0.0;
    waveformAccumulatedFrames = 0;
    waveformBlockFrames = 2048;
    waveformWidget->clear();
    waveformWidget->setDuration(currentDurationMs);

    waveformDecoder->setSource(QUrl::fromLocalFile(filePath));
    waveformDecoder->start();
}
void MainWindow::processWaveformBuffer(const QAudioBuffer &buffer)
{
    if (!buffer.isValid()) {
        return;
    }

    const QAudioFormat format = buffer.format();
    const int channels = qMax(1, format.channelCount());
    const int sampleRate = qMax(1, format.sampleRate());
    waveformBlockFrames = qMax(512, sampleRate / 24);

    const char *data = buffer.constData<char>();
    const qsizetype sampleCount = buffer.sampleCount();
    const qsizetype frameCount = sampleCount / channels;

    for (qsizetype frame = 0; frame < frameCount; ++frame) {
        double frameEnergy = 0.0;
        for (int channel = 0; channel < channels; ++channel) {
            const double value = sampleValue(data, frame * channels + channel, format);
            frameEnergy += value * value;
        }
        frameEnergy /= channels;
        waveformAccumulatedEnergy += frameEnergy;
        ++waveformAccumulatedFrames;

        if (waveformAccumulatedFrames >= waveformBlockFrames) {
            waveformBlocks.push_back(qSqrt(waveformAccumulatedEnergy / waveformAccumulatedFrames));
            waveformAccumulatedEnergy = 0.0;
            waveformAccumulatedFrames = 0;
        }
    }
}
QVector<double> MainWindow::normalizedWaveform() const
{
    QVector<double> blocks = waveformBlocks;
    if (waveformAccumulatedFrames > 0) {
        blocks.push_back(qSqrt(waveformAccumulatedEnergy / waveformAccumulatedFrames));
    }
    if (blocks.isEmpty()) {
        return {};
    }

    const int targetCount = 150;
    QVector<double> result;
    result.reserve(targetCount);
    for (int i = 0; i < targetCount; ++i) {
        const int begin = i * blocks.size() / targetCount;
        const int end = qMax(begin + 1, (i + 1) * blocks.size() / targetCount);
        double peak = 0.0;
        for (int j = begin; j < end && j < blocks.size(); ++j) {
            peak = qMax(peak, blocks[j]);
        }
        result.push_back(peak);
    }

    const double maxValue = *std::max_element(result.cbegin(), result.cend());
    if (maxValue <= 0.000001) {
        return result;
    }
    for (double &value : result) {
        value = qBound(0.03, qPow(value / maxValue, 0.65), 1.0);
    }
    return result;
}
