#ifndef PLAYBACKSTRATEGY_H
#define PLAYBACKSTRATEGY_H

#include <QString>

class PlaybackStrategy
{
public:
    virtual ~PlaybackStrategy() = default;
    virtual int nextIndex(int currentIndex, int trackCount) const = 0;
    virtual QString name() const = 0;
    virtual QString iconName() const = 0;
};

class ListLoopStrategy : public PlaybackStrategy
{
public:
    int nextIndex(int currentIndex, int trackCount) const override;
    QString name() const override;
    QString iconName() const override;
};

class SingleLoopStrategy : public PlaybackStrategy
{
public:
    int nextIndex(int currentIndex, int trackCount) const override;
    QString name() const override;
    QString iconName() const override;
};

class ShuffleStrategy : public PlaybackStrategy
{
public:
    int nextIndex(int currentIndex, int trackCount) const override;
    QString name() const override;
    QString iconName() const override;
};

#endif // PLAYBACKSTRATEGY_H
