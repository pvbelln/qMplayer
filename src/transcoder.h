#ifndef TRANSCODER_H
#define TRANSCODER_H

#include <QObject>
#include <QProcess>
#include <QString>

struct TranscodeOptions
{
    QString ffmpegPath;
    QString inputPath;
    QString outputPath;
    QString format;
    QString bitrate;
    QString sampleRate;
    qint64 durationMs = 0;
};

class Transcoder : public QObject
{
    Q_OBJECT

public:
    explicit Transcoder(QObject *parent = nullptr);

    bool isRunning() const;
    bool isPaused() const;
    void start(const QString &ffmpegPath, const QString &inputPath, const QString &outputPath);
    void start(const TranscodeOptions &options);
    void stop();
    void togglePause();

signals:
    void statusChanged(const QString &status);
    void progressChanged(int percent);
    void finished(bool success, const QString &message);

private:
    qint64 parseProgressTimeMs(const QString &text) const;
    bool setProcessSuspended(bool suspended);

    QProcess *m_process = nullptr;
    TranscodeOptions m_options;
    QString m_outputPath;
    bool m_paused = false;
    bool m_stopping = false;
};

#endif // TRANSCODER_H
