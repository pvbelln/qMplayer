#include "track.h"

#include <QFileInfo>
#include <QJsonArray>

QString Track::displayTitle() const
{
    if (!title.trimmed().isEmpty()) {
        return title.trimmed();
    }
    return QFileInfo(filePath).completeBaseName();
}

QString Track::subtitle() const
{
    QStringList parts;
    if (!artist.trimmed().isEmpty()) {
        parts << artist.trimmed();
    }
    if (!album.trimmed().isEmpty()) {
        parts << album.trimmed();
    }
    return parts.isEmpty() ? QFileInfo(filePath).absolutePath() : parts.join(" - ");
}

QJsonObject Track::toJson() const
{
    QJsonObject object;
    object["filePath"] = filePath;
    object["title"] = title;
    object["artist"] = artist;
    object["album"] = album;
    object["lrcPath"] = lrcPath;
    QJsonArray tagsArray;
    for (const QString &tag : tags) {
        tagsArray.append(tag);
    }
    object["tags"] = tagsArray;
    object["durationMs"] = QString::number(durationMs);
    object["lyricsOffsetMs"] = lyricsOffsetMs;
    return object;
}

Track Track::fromJson(const QJsonObject &object)
{
    Track track;
    track.filePath = object["filePath"].toString();
    track.title = object["title"].toString();
    track.artist = object["artist"].toString();
    track.album = object["album"].toString();
    track.lrcPath = object["lrcPath"].toString();
    const QJsonArray tagsArray = object["tags"].toArray();
    for (const QJsonValue &value : tagsArray) {
        const QString tag = value.toString().trimmed();
        if (!tag.isEmpty() && !track.tags.contains(tag, Qt::CaseInsensitive)) {
            track.tags.append(tag);
        }
    }
    track.durationMs = object["durationMs"].toVariant().toLongLong();
    track.lyricsOffsetMs = object["lyricsOffsetMs"].toInt();
    return track;
}
