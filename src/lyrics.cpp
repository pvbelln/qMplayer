#include "lyrics.h"

#include <algorithm>

void Lyrics::clear()
{
    m_lines.clear();
    m_baseOffsetMs = 0;
}

bool Lyrics::isEmpty() const
{
    return m_lines.isEmpty();
}

int Lyrics::lineCount() const
{
    return m_lines.size();
}

int Lyrics::baseOffsetMs() const
{
    return m_baseOffsetMs;
}

void Lyrics::setBaseOffsetMs(int offsetMs)
{
    m_baseOffsetMs = offsetMs;
}

void Lyrics::addLine(qint64 timeMs, const QString &text)
{
    m_lines.push_back({timeMs, text.trimmed()});
}

void Lyrics::sortLines()
{
    std::sort(m_lines.begin(), m_lines.end(), [](const LyricLine &a, const LyricLine &b) {
        return a.timeMs < b.timeMs;
    });
}

const LyricLine *Lyrics::lineAt(int index) const
{
    if (index < 0 || index >= m_lines.size()) {
        return nullptr;
    }
    return &m_lines[index];
}

int Lyrics::currentLineIndex(qint64 positionMs, int userOffsetMs) const
{
    if (m_lines.isEmpty()) {
        return -1;
    }

    const qint64 adjustedPosition = positionMs + m_baseOffsetMs + userOffsetMs;
    int low = 0;
    int high = m_lines.size() - 1;
    int answer = -1;

    while (low <= high) {
        const int mid = low + (high - low) / 2;
        if (m_lines[mid].timeMs <= adjustedPosition) {
            answer = mid;
            low = mid + 1;
        } else {
            high = mid - 1;
        }
    }

    return answer < 0 ? 0 : answer;
}
