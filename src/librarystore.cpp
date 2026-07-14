#include "librarystore.h"

#include <QDir>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QStandardPaths>

LibraryStore::LibraryStore()
{
    const QString appDir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QDir().mkpath(appDir);
    m_storagePath = QDir(appDir).filePath("library.json");
}

QVector<Track> LibraryStore::load() const
{
    const QJsonObject root = loadRootObject();
    QVector<Track> tracks;
    const QJsonArray array = root["tracks"].toArray();
    tracks.reserve(array.size());
    for (const QJsonValue &value : array) {
        const Track track = Track::fromJson(value.toObject());
        if (!track.filePath.isEmpty()) {
            tracks.push_back(track);
        }
    }
    return tracks;
}

bool LibraryStore::save(const QVector<Track> &tracks) const
{
    QJsonArray array;
    for (const Track &track : tracks) {
        array.append(track.toJson());
    }

    QJsonObject root = loadRootObject();
    root["tracks"] = array;
    return saveRootObject(root);
}

QVector<UserPlaylist> LibraryStore::loadUserPlaylists() const
{
    const QJsonObject root = loadRootObject();
    QVector<UserPlaylist> playlists;
    const QJsonArray array = root["playlists"].toArray();
    playlists.reserve(array.size());
    for (const QJsonValue &value : array) {
        const QJsonObject object = value.toObject();
        UserPlaylist playlist;
        playlist.name = object["name"].toString().trimmed();
        const QJsonArray paths = object["filePaths"].toArray();
        for (const QJsonValue &pathValue : paths) {
            const QString path = pathValue.toString().trimmed();
            if (!path.isEmpty() && !playlist.filePaths.contains(path)) {
                playlist.filePaths.append(path);
            }
        }
        if (!playlist.name.isEmpty()) {
            playlists.push_back(playlist);
        }
    }
    return playlists;
}

bool LibraryStore::saveUserPlaylists(const QVector<UserPlaylist> &playlists) const
{
    QJsonArray array;
    for (const UserPlaylist &playlist : playlists) {
        if (playlist.name.trimmed().isEmpty()) {
            continue;
        }
        QJsonObject object;
        object["name"] = playlist.name.trimmed();
        QJsonArray paths;
        for (const QString &path : playlist.filePaths) {
            paths.append(path);
        }
        object["filePaths"] = paths;
        array.append(object);
    }

    QJsonObject root = loadRootObject();
    root["playlists"] = array;
    return saveRootObject(root);
}

PlaybackSnapshot LibraryStore::loadPlaybackSnapshot() const
{
    const QJsonObject object = loadRootObject()["lastPlayback"].toObject();
    PlaybackSnapshot snapshot;
    snapshot.filePath = object["filePath"].toString();
    snapshot.positionMs = object["positionMs"].toVariant().toLongLong();
    return snapshot;
}

bool LibraryStore::savePlaybackSnapshot(const PlaybackSnapshot &snapshot) const
{
    QJsonObject root = loadRootObject();
    QJsonObject object;
    object["filePath"] = snapshot.filePath;
    object["positionMs"] = QString::number(qMax<qint64>(0, snapshot.positionMs));
    root["lastPlayback"] = object;
    return saveRootObject(root);
}

QString LibraryStore::storagePath() const
{
    return m_storagePath;
}

QJsonObject LibraryStore::loadRootObject() const
{
    QFile file(m_storagePath);
    if (!file.open(QIODevice::ReadOnly)) {
        return {};
    }

    return QJsonDocument::fromJson(file.readAll()).object();
}

bool LibraryStore::saveRootObject(const QJsonObject &root) const
{
    QFile file(m_storagePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        return false;
    }
    file.write(QJsonDocument(root).toJson(QJsonDocument::Indented));
    return true;
}
