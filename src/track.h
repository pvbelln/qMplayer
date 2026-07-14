#ifndef TRACK_H
#define TRACK_H

#include <QJsonObject>
#include <QString>
#include <QStringList>

struct Track
{
    QString filePath;
    QString title;
    QString artist;
    QString album;
    QString lrcPath;
    QStringList tags;
    qint64 durationMs = 0;
    int lyricsOffsetMs = 0;

    QString displayTitle() const;
    QString subtitle() const;

    QJsonObject toJson() const;
    static Track fromJson(const QJsonObject &object);
};

#endif // TRACK_H
