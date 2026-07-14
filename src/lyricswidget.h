#ifndef LYRICSWIDGET_H
#define LYRICSWIDGET_H

#include "lyrics.h"

#include <QElapsedTimer>
#include <QPoint>
#include <QRect>
#include <QPropertyAnimation>
#include <QTimer>
#include <QWidget>

class LyricsWidget : public QWidget
{
    Q_OBJECT
    Q_PROPERTY(double browseOffset READ browseOffset WRITE setBrowseOffset)
    Q_PROPERTY(double centerLine READ centerLine WRITE setCenterLine)

public:
    explicit LyricsWidget(QWidget *parent = nullptr);

    void setLyrics(const Lyrics &lyrics);
    void setMessage(const QString &message);
    void setPosition(qint64 positionMs);
    void setPlaybackActive(bool active);
    void setUserOffsetMs(int offsetMs);
    int userOffsetMs() const;
    void setNightMode(bool nightMode);
    void setLoadButtonVisible(bool visible);
    double browseOffset() const;
    void setBrowseOffset(double offset);
    double centerLine() const;
    void setCenterLine(double centerLine);

signals:
    void jumpRequested(qint64 positionMs);
    void loadLrcRequested();
    void lyricsContextMenuRequested(const QPoint &globalPosition);

protected:
    bool event(QEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void wheelEvent(QWheelEvent *event) override;
    void paintEvent(QPaintEvent *event) override;

private:
    int playbackLineIndex() const;
    qint64 effectivePositionMs() const;
    qint64 adjustedPositionMs() const;
    double lineProgress(int lineIndex) const;
    double targetCenterPosition() const;
    void updatePlaybackCenter();
    int displayCenterIndex() const;
    int lineIndexAtCenter() const;
    bool isJumpHotspot(const QPoint &pos) const;
    bool isLoadButtonHotspot(const QPoint &pos) const;
    void jumpToCenterLine();
    void resetBrowseTimer();

    Lyrics m_lyrics;
    QString m_message = "Lyrics will appear here";
    qint64 m_positionMs = 0;
    bool m_playbackActive = false;
    QElapsedTimer m_positionClock;
    QTimer m_animationTimer;
    int m_userOffsetMs = 0;
    bool m_nightMode = false;
    double m_browseOffsetLines = 0.0;
    double m_centerLine = 0.0;
    int m_lastPlaybackLineIndex = -1;
    double m_dragStartBrowseOffset = 0.0;
    bool m_browsing = false;
    bool m_dragging = false;
    QPoint m_lastMousePos;
    QPoint m_dragStartPos;
    QRect m_jumpButtonRect;
    QRect m_centerLineRect;
    QRect m_loadButtonRect;
    QTimer m_returnTimer;
    QPropertyAnimation *m_browseAnimation = nullptr;
    QPropertyAnimation *m_centerAnimation = nullptr;
    bool m_showLoadButton = false;
};

#endif // LYRICSWIDGET_H
