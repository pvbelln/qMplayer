#ifndef AUDIOPLAYER_H
#define AUDIOPLAYER_H

#include <QObject>
#include <QUrl>

class QImage;
class QAudioOutput;
class QMediaPlayer;

class AudioPlayer : public QObject
{
    Q_OBJECT

public:
    explicit AudioPlayer(QObject *parent = nullptr);

    void setSource(const QString &filePath);
    void play();
    void pause();
    void stop();
    void togglePlayPause();
    void seek(qint64 positionMs);
    void setVolume(int value);
    void setMuted(bool muted);
    void toggleMuted();
    bool isPlaying() const;
    bool isMuted() const;
    qint64 position() const;
    qint64 duration() const;

signals:
    void positionChanged(qint64 positionMs);
    void durationChanged(qint64 durationMs);
    void metadataChanged(const QString &title, const QString &artist, const QString &album, const QImage &coverImage);
    void playbackStateChanged(bool playing);
    void finished();
    void errorOccurred(const QString &message);

private:
    QMediaPlayer *m_player = nullptr;
    QAudioOutput *m_audioOutput = nullptr;
};

#endif // AUDIOPLAYER_H
