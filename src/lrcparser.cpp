#include "lrcparser.h"

#include <QFile>
#include <QRegularExpression>
#include <QStringDecoder>

Lyrics LrcParser::parseFile(const QString &filePath)
{
    Lyrics lyrics;
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        return lyrics;
    }

    const QString content = decodeFile(file.readAll());
    const QStringList lines = content.split(QRegularExpression("[\r\n]+"), Qt::SkipEmptyParts);
    const QRegularExpression tagExpression(R"(\[([^\]]+)\])");

    for (const QString &rawLine : lines) {
        QString line = rawLine.trimmed();
        if (line.isEmpty()) {
            continue;
        }

        QRegularExpressionMatchIterator iterator = tagExpression.globalMatch(line);
        QVector<qint64> times;
        int lastTagEnd = 0;

        while (iterator.hasNext()) {
            const QRegularExpressionMatch match = iterator.next();
            const QString tag = match.captured(1).trimmed();
            lastTagEnd = match.capturedEnd();

            qint64 timeMs = 0;
            if (parseTimeTag(tag, &timeMs)) {
                times.push_back(timeMs);
            } else if (tag.startsWith("offset:", Qt::CaseInsensitive)) {
                lyrics.setBaseOffsetMs(tag.mid(QString("offset:").size()).trimmed().toInt());
            }
        }

        QString text = line.mid(lastTagEnd).trimmed();
        if (text.isEmpty()) {
            text = " ";
        }
        for (qint64 timeMs : times) {
            lyrics.addLine(timeMs, text);
        }
    }

    lyrics.sortLines();
    return lyrics;
}

QString LrcParser::decodeFile(const QByteArray &data)
{
    QStringDecoder utf8Decoder(QStringDecoder::Utf8);
    QString text = utf8Decoder.decode(data);
    if (!utf8Decoder.hasError()) {
        return text;
    }

    QStringDecoder localDecoder(QStringDecoder::System);
    return localDecoder.decode(data);
}

bool LrcParser::parseTimeTag(const QString &tag, qint64 *timeMs)
{
    static const QRegularExpression expression(R"(^(\d{1,2}):(\d{2})(?:[.:](\d{1,3}))?$)");
    const QRegularExpressionMatch match = expression.match(tag);
    if (!match.hasMatch()) {
        return false;
    }

    const int minutes = match.captured(1).toInt();
    const int seconds = match.captured(2).toInt();
    QString fraction = match.captured(3);
    while (fraction.size() > 0 && fraction.size() < 3) {
        fraction.append('0');
    }
    if (fraction.size() > 3) {
        fraction = fraction.left(3);
    }

    *timeMs = (minutes * 60 + seconds) * 1000LL + fraction.toInt();
    return true;
}
