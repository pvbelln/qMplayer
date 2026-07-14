#ifndef LIBRARYSTORE_H
#define LIBRARYSTORE_H

#include "track.h"

#include <QJsonObject>
#include <QString>
#include <QStringList>
#include <QVector>

struct UserPlaylist
{
    QString name;
    QStringList filePaths;
};

struct PlaybackSnapshot
{
    QString filePath;
    qint64 positionMs = 0;
};

class LibraryStore
{
public:
    LibraryStore();

    QVector<Track> load() const;
    bool save(const QVector<Track> &tracks) const;
    QVector<UserPlaylist> loadUserPlaylists() const;
    bool saveUserPlaylists(const QVector<UserPlaylist> &playlists) const;
    PlaybackSnapshot loadPlaybackSnapshot() const;
    bool savePlaybackSnapshot(const PlaybackSnapshot &snapshot) const;
    QString storagePath() const;

private:
    QJsonObject loadRootObject() const;
    bool saveRootObject(const QJsonObject &root) const;

    QString m_storagePath;
};

#endif // LIBRARYSTORE_H
