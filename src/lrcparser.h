#ifndef LRCPARSER_H
#define LRCPARSER_H

#include "lyrics.h"

#include <QString>

class LrcParser
{
public:
    static Lyrics parseFile(const QString &filePath);

private:
    static QString decodeFile(const QByteArray &data);
    static bool parseTimeTag(const QString &tag, qint64 *timeMs);
};

#endif // LRCPARSER_H
