#include "playlist.h"

#include <QFileInfo>
#include <QSet>

int Playlist::count() const
{
    return m_tracks.size();
}

bool Playlist::isEmpty() const
{
    return m_tracks.isEmpty();
}

int Playlist::currentIndex() const
{
    return m_currentIndex;
}

void Playlist::setCurrentIndex(int index)
{
    if (index >= 0 && index < m_tracks.size()) {
        m_currentIndex = index;
    }
}

const Track *Playlist::currentTrack() const
{
    return trackAt(m_currentIndex);
}

const Track *Playlist::trackAt(int index) const
{
    if (index < 0 || index >= m_tracks.size()) {
        return nullptr;
    }
    return &m_tracks[index];
}

Track *Playlist::trackAt(int index)
{
    if (index < 0 || index >= m_tracks.size()) {
        return nullptr;
    }
    return &m_tracks[index];
}

const QVector<Track> &Playlist::tracks() const
{
    return m_tracks;
}

void Playlist::setTracks(const QVector<Track> &tracks)
{
    m_tracks = tracks;
    m_currentIndex = m_tracks.isEmpty() ? -1 : 0;
}

int Playlist::addTracks(const QVector<Track> &newTracks)
{
    QSet<QString> knownPaths;
    for (const Track &track : m_tracks) {
        knownPaths.insert(QFileInfo(track.filePath).absoluteFilePath());
    }

    int added = 0;
    for (const Track &track : newTracks) {
        const QString absolutePath = QFileInfo(track.filePath).absoluteFilePath();
        if (knownPaths.contains(absolutePath)) {
            continue;
        }
        Track copy = track;
        copy.filePath = absolutePath;
        m_tracks.push_back(copy);
        knownPaths.insert(absolutePath);
        ++added;
    }

    if (m_currentIndex < 0 && !m_tracks.isEmpty()) {
        m_currentIndex = 0;
    }
    return added;
}

void Playlist::clearMissingFiles()
{
    QVector<Track> existing;
    existing.reserve(m_tracks.size());
    for (const Track &track : m_tracks) {
        if (QFileInfo::exists(track.filePath)) {
            existing.push_back(track);
        }
    }
    m_tracks = existing;
    if (m_tracks.isEmpty()) {
        m_currentIndex = -1;
    } else if (m_currentIndex >= m_tracks.size()) {
        m_currentIndex = 0;
    }
}

int Playlist::nextIndex() const
{
    if (m_tracks.isEmpty()) {
        return -1;
    }
    return m_currentIndex < 0 ? 0 : (m_currentIndex + 1) % m_tracks.size();
}

int Playlist::previousIndex() const
{
    if (m_tracks.isEmpty()) {
        return -1;
    }
    if (m_currentIndex <= 0) {
        return m_tracks.size() - 1;
    }
    return m_currentIndex - 1;
}
