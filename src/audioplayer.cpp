#include "audioplayer.h"

#include <QAudioOutput>
#include <QImage>
#include <QMediaPlayer>
#include <QMediaMetaData>

AudioPlayer::AudioPlayer(QObject *parent)
    : QObject(parent),
      m_player(new QMediaPlayer(this)),
      m_audioOutput(new QAudioOutput(this))
{
    m_player->setAudioOutput(m_audioOutput);
    m_audioOutput->setVolume(0.8);

    connect(m_player, &QMediaPlayer::positionChanged, this, &AudioPlayer::positionChanged);
    connect(m_player, &QMediaPlayer::durationChanged, this, &AudioPlayer::durationChanged);
    connect(m_player, &QMediaPlayer::metaDataChanged, this, [this]() {
        const QMediaMetaData metaData = m_player->metaData();
        const QString title = metaData.stringValue(QMediaMetaData::Title);
        QString artist = metaData.stringValue(QMediaMetaData::Author);
        if (artist.isEmpty()) {
            artist = metaData.stringValue(QMediaMetaData::ContributingArtist);
        }
        const QString album = metaData.stringValue(QMediaMetaData::AlbumTitle);
        QImage coverImage = metaData.value(QMediaMetaData::CoverArtImage).value<QImage>();
        if (coverImage.isNull()) {
            coverImage = metaData.value(QMediaMetaData::ThumbnailImage).value<QImage>();
        }
        emit metadataChanged(title, artist, album, coverImage);
    });
    connect(m_player, &QMediaPlayer::playbackStateChanged, this, [this](QMediaPlayer::PlaybackState state) {
        emit playbackStateChanged(state == QMediaPlayer::PlayingState);
    });
    connect(m_player, &QMediaPlayer::mediaStatusChanged, this, [this](QMediaPlayer::MediaStatus status) {
        if (status == QMediaPlayer::EndOfMedia) {
            emit finished();
        }
    });
    connect(m_player, &QMediaPlayer::errorOccurred, this, [this](QMediaPlayer::Error, const QString &errorString) {
        if (!errorString.isEmpty()) {
            emit errorOccurred(errorString);
        }
    });
}

void AudioPlayer::setSource(const QString &filePath)
{
    m_player->setSource(QUrl::fromLocalFile(filePath));
}

void AudioPlayer::play()
{
    m_player->play();
}

void AudioPlayer::pause()
{
    m_player->pause();
}

void AudioPlayer::stop()
{
    m_player->stop();
}

void AudioPlayer::togglePlayPause()
{
    isPlaying() ? pause() : play();
}

void AudioPlayer::seek(qint64 positionMs)
{
    m_player->setPosition(positionMs);
}

void AudioPlayer::setVolume(int value)
{
    m_audioOutput->setVolume(qBound(0, value, 100) / 100.0);
}

void AudioPlayer::setMuted(bool muted)
{
    m_audioOutput->setMuted(muted);
}

void AudioPlayer::toggleMuted()
{
    setMuted(!isMuted());
}

bool AudioPlayer::isPlaying() const
{
    return m_player->playbackState() == QMediaPlayer::PlayingState;
}

bool AudioPlayer::isMuted() const
{
    return m_audioOutput->isMuted();
}

qint64 AudioPlayer::position() const
{
    return m_player->position();
}

qint64 AudioPlayer::duration() const
{
    return m_player->duration();
}
