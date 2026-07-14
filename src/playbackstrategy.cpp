#include "playbackstrategy.h"

#include <QRandomGenerator>

int ListLoopStrategy::nextIndex(int currentIndex, int trackCount) const
{
    if (trackCount <= 0) {
        return -1;
    }
    return currentIndex < 0 ? 0 : (currentIndex + 1) % trackCount;
}

QString ListLoopStrategy::name() const
{
    return "List loop";
}

QString ListLoopStrategy::iconName() const
{
    return "Folder-loop.512.png";
}

int SingleLoopStrategy::nextIndex(int currentIndex, int trackCount) const
{
    if (trackCount <= 0) {
        return -1;
    }
    return currentIndex < 0 ? 0 : currentIndex;
}

QString SingleLoopStrategy::name() const
{
    return "Single loop";
}

QString SingleLoopStrategy::iconName() const
{
    return "Single-loop.512.png";
}

int ShuffleStrategy::nextIndex(int currentIndex, int trackCount) const
{
    if (trackCount <= 0) {
        return -1;
    }
    if (trackCount == 1) {
        return 0;
    }

    int next = currentIndex;
    while (next == currentIndex) {
        next = QRandomGenerator::global()->bounded(trackCount);
    }
    return next;
}

QString ShuffleStrategy::name() const
{
    return "Shuffle";
}

QString ShuffleStrategy::iconName() const
{
    return "Shuffle.512.png";
}
