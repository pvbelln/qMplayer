#ifndef PLAYLIST_H
#define PLAYLIST_H

#include "track.h"

#include <QVector>

class Playlist
{
public:
    int count() const;
    bool isEmpty() const;
    int currentIndex() const;
    void setCurrentIndex(int index);

    const Track *currentTrack() const;
    const Track *trackAt(int index) const;
    Track *trackAt(int index);

    const QVector<Track> &tracks() const;
    void setTracks(const QVector<Track> &tracks);
    int addTracks(const QVector<Track> &newTracks);
    void clearMissingFiles();

    int nextIndex() const;
    int previousIndex() const;

private:
    QVector<Track> m_tracks;
    int m_currentIndex = -1;
};

#endif // PLAYLIST_H
