#ifndef LYRICS_H
#define LYRICS_H

#include <QString>
#include <QVector>

struct LyricLine
{
    qint64 timeMs = 0;
    QString text;
};

class Lyrics
{
public:
    void clear();
    bool isEmpty() const;
    int lineCount() const;
    int baseOffsetMs() const;
    void setBaseOffsetMs(int offsetMs);
    void addLine(qint64 timeMs, const QString &text);
    void sortLines();
    const LyricLine *lineAt(int index) const;
    int currentLineIndex(qint64 positionMs, int userOffsetMs) const;

private:
    QVector<LyricLine> m_lines;
    int m_baseOffsetMs = 0;
};

#endif // LYRICS_H
