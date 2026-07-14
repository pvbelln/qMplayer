#include "lyricswidget.h"

#include <QEvent>
#include <QMouseEvent>
#include <QPainter>
#include <QEasingCurve>
#include <QWheelEvent>
#include <QtMath>

LyricsWidget::LyricsWidget(QWidget *parent)
    : QWidget(parent)
{
    setMinimumHeight(320);
    setObjectName("lyricsWidget");
    setMouseTracking(true);
    m_browseAnimation = new QPropertyAnimation(this, "browseOffset", this);
    m_browseAnimation->setDuration(180);
    m_browseAnimation->setEasingCurve(QEasingCurve::OutCubic);
    m_centerAnimation = new QPropertyAnimation(this, "centerLine", this);
    m_centerAnimation->setDuration(320);
    m_centerAnimation->setEasingCurve(QEasingCurve::OutCubic);
    m_returnTimer.setSingleShot(true);
    m_returnTimer.setInterval(3000);
    connect(&m_returnTimer, &QTimer::timeout, this, [this]() {
        m_dragging = false;
        m_browseAnimation->stop();
        m_browseAnimation->setStartValue(m_browseOffsetLines);
        m_browseAnimation->setEndValue(0.0);
        m_browseAnimation->start();
        update();
    });
    connect(m_browseAnimation, &QPropertyAnimation::finished, this, [this]() {
        if (qAbs(m_browseOffsetLines) < 0.01 && !m_dragging) {
            m_browsing = false;
            m_browseOffsetLines = 0.0;
            unsetCursor();
            update();
        }
    });

    m_animationTimer.setInterval(33);
    connect(&m_animationTimer, &QTimer::timeout, this, [this]() {
        if (!m_browsing && !m_lyrics.isEmpty()) {
            updatePlaybackCenter();
            update();
        }
    });
}

void LyricsWidget::setLyrics(const Lyrics &lyrics)
{
    m_lyrics = lyrics;
    m_browsing = false;
    m_dragging = false;
    m_browseOffsetLines = 0.0;
    m_lastPlaybackLineIndex = playbackLineIndex();
    m_centerLine = qMax(0, m_lastPlaybackLineIndex);
    m_centerAnimation->stop();
    m_returnTimer.stop();
    if (!m_animationTimer.isActive()) {
        m_animationTimer.start();
    }
    m_positionClock.restart();
    update();
}

void LyricsWidget::setMessage(const QString &message)
{
    m_message = message;
    m_lyrics.clear();
    m_browsing = false;
    m_dragging = false;
    m_browseOffsetLines = 0.0;
    m_centerLine = 0.0;
    m_lastPlaybackLineIndex = -1;
    m_centerAnimation->stop();
    m_returnTimer.stop();
    m_animationTimer.stop();
    update();
}

void LyricsWidget::setPosition(qint64 positionMs)
{
    m_positionMs = positionMs;
    m_positionClock.restart();
    if (!m_browsing) {
        updatePlaybackCenter();
        update();
    }
}

void LyricsWidget::setPlaybackActive(bool active)
{
    m_positionMs = effectivePositionMs();
    m_playbackActive = active;
    m_positionClock.restart();
    if (active && !m_lyrics.isEmpty() && !m_animationTimer.isActive()) {
        m_animationTimer.start();
    }
    update();
}

void LyricsWidget::setUserOffsetMs(int offsetMs)
{
    m_userOffsetMs = offsetMs;
    update();
}

int LyricsWidget::userOffsetMs() const
{
    return m_userOffsetMs;
}

void LyricsWidget::setNightMode(bool nightMode)
{
    m_nightMode = nightMode;
    update();
}

void LyricsWidget::setLoadButtonVisible(bool visible)
{
    m_showLoadButton = visible;
    update();
}

double LyricsWidget::browseOffset() const
{
    return m_browseOffsetLines;
}

void LyricsWidget::setBrowseOffset(double offset)
{
    if (m_lyrics.isEmpty()) {
        m_browseOffsetLines = 0.0;
    } else {
        m_browseOffsetLines = qBound(-static_cast<double>(m_lyrics.lineCount()), offset, static_cast<double>(m_lyrics.lineCount()));
    }
    update();
}

double LyricsWidget::centerLine() const
{
    return m_centerLine;
}

void LyricsWidget::setCenterLine(double centerLine)
{
    m_centerLine = centerLine;
    update();
}

bool LyricsWidget::event(QEvent *event)
{
    if (event->type() == QEvent::MouseMove && m_showLoadButton && m_lyrics.isEmpty()) {
        auto *mouseEvent = static_cast<QMouseEvent *>(event);
        setCursor(isLoadButtonHotspot(mouseEvent->pos()) ? Qt::PointingHandCursor : Qt::ArrowCursor);
    } else if (event->type() == QEvent::MouseMove && m_browsing && !m_lyrics.isEmpty()) {
        auto *mouseEvent = static_cast<QMouseEvent *>(event);
        setCursor(isJumpHotspot(mouseEvent->pos()) ? Qt::PointingHandCursor : Qt::ArrowCursor);
    } else if (event->type() == QEvent::Leave) {
        unsetCursor();
    }

    return QWidget::event(event);
}

void LyricsWidget::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::RightButton) {
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
        emit lyricsContextMenuRequested(event->globalPosition().toPoint());
#else
        emit lyricsContextMenuRequested(event->globalPos());
#endif
        event->accept();
        return;
    }

    if (m_lyrics.isEmpty()
        && event->button() == Qt::LeftButton
        && isLoadButtonHotspot(event->pos())) {
        emit loadLrcRequested();
        event->accept();
        return;
    }

    if (m_lyrics.isEmpty() || event->button() != Qt::LeftButton) {
        QWidget::mousePressEvent(event);
        return;
    }

    if (isJumpHotspot(event->pos())) {
        jumpToCenterLine();
        return;
    }

    m_browsing = true;
    m_dragging = true;
    m_dragStartPos = event->pos();
    m_lastMousePos = event->pos();
    m_dragStartBrowseOffset = m_browseOffsetLines;
    m_browseAnimation->stop();
    m_returnTimer.stop();
    update();
}

void LyricsWidget::mouseMoveEvent(QMouseEvent *event)
{
    if (!m_dragging || m_lyrics.isEmpty()) {
        QWidget::mouseMoveEvent(event);
        return;
    }

    const double lineHeight = 42.0;
    const int deltaY = event->pos().y() - m_dragStartPos.y();
    setBrowseOffset(m_dragStartBrowseOffset - deltaY / lineHeight);
    m_lastMousePos = event->pos();

    setCursor(isJumpHotspot(event->pos()) ? Qt::PointingHandCursor : Qt::ArrowCursor);
}

void LyricsWidget::mouseReleaseEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton && m_dragging) {
        m_dragging = false;
        resetBrowseTimer();
        update();
        return;
    }

    QWidget::mouseReleaseEvent(event);
}

void LyricsWidget::wheelEvent(QWheelEvent *event)
{
    if (m_lyrics.isEmpty()) {
        QWidget::wheelEvent(event);
        return;
    }

    const int delta = event->angleDelta().y();
    if (delta == 0) {
        QWidget::wheelEvent(event);
        return;
    }

    m_browsing = true;
    m_dragging = false;
    m_browseAnimation->stop();
    const double targetOffset = qBound(
        -static_cast<double>(m_lyrics.lineCount()),
        m_browseOffsetLines + (delta > 0 ? -1.0 : 1.0),
        static_cast<double>(m_lyrics.lineCount()));
    m_browseAnimation->setStartValue(m_browseOffsetLines);
    m_browseAnimation->setEndValue(targetOffset);
    m_browseAnimation->start();
    resetBrowseTimer();
    update();
    event->accept();
}

void LyricsWidget::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event)

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    const QColor background = m_nightMode ? QColor("#141821") : QColor("#ffffff");
    const QColor mutedText = m_nightMode ? QColor("#aeb6c4") : QColor("#64748b");
    const QColor primaryText = m_nightMode ? QColor("#edf0f5") : QColor("#1f2937");
    const QColor accent = m_nightMode ? QColor("#8fb3ff") : QColor("#2563eb");
    const QColor guide = m_nightMode ? QColor("#343946") : QColor("#d8dee9");

    painter.fillRect(rect(), background);

    if (m_lyrics.isEmpty()) {
        painter.setPen(mutedText);
        const QRect messageRect(24, height() / 2 - 58, width() - 48, 44);
        painter.drawText(messageRect, Qt::AlignCenter | Qt::TextWordWrap, m_message);
        if (m_showLoadButton) {
            const QSize buttonSize(148, 38);
            m_loadButtonRect = QRect(
                (width() - buttonSize.width()) / 2,
                height() / 2 + 4,
                buttonSize.width(),
                buttonSize.height());
            painter.setPen(Qt::NoPen);
            painter.setBrush(accent);
            painter.drawRoundedRect(m_loadButtonRect, 19, 19);
            QFont font = painter.font();
            font.setBold(true);
            painter.setFont(font);
            painter.setPen(QColor("#ffffff"));
            painter.drawText(m_loadButtonRect, Qt::AlignCenter, "Load LRC File");
        } else {
            m_loadButtonRect = QRect();
        }
        return;
    }

    const int currentIndex = playbackLineIndex();
    const int centerIndex = displayCenterIndex();
    const double smoothCenter = m_browsing ? m_centerLine + m_browseOffsetLines : m_centerLine;
    const int lineHeight = 42;
    const int centerY = height() / 2;
    const int left = m_browsing ? 76 : 24;
    const int right = 24;
    const int textWidth = qMax(10, width() - left - right);

    if (m_browsing) {
        painter.setPen(QPen(guide, 1));
        painter.drawLine(20, centerY, width() - 20, centerY);

        m_jumpButtonRect = QRect(24, centerY - 17, 34, 34);
        painter.setBrush(accent);
        painter.setPen(Qt::NoPen);
        painter.drawRoundedRect(m_jumpButtonRect, 17, 17);

        QPolygon triangle;
        triangle << QPoint(m_jumpButtonRect.left() + 13, m_jumpButtonRect.top() + 9)
                 << QPoint(m_jumpButtonRect.left() + 13, m_jumpButtonRect.bottom() - 9)
                 << QPoint(m_jumpButtonRect.right() - 10, m_jumpButtonRect.center().y());
        painter.setBrush(QColor("#ffffff"));
        painter.drawPolygon(triangle);
    } else {
        m_jumpButtonRect = QRect();
    }
    m_centerLineRect = QRect(left, centerY - lineHeight / 2, textWidth, lineHeight);

    for (int i = 0; i < m_lyrics.lineCount(); ++i) {
        const LyricLine *line = m_lyrics.lineAt(i);
        if (line == nullptr) {
            continue;
        }

        const double yFloat = centerY + (i - smoothCenter) * lineHeight;
        const int y = qRound(yFloat);
        if (y < -lineHeight || y > height() + lineHeight) {
            continue;
        }

        QFont font = painter.font();
        font.setPointSize(i == currentIndex ? 17 : 13);
        font.setBold(i == currentIndex);
        painter.setFont(font);

        QRect textRect(left, y - lineHeight / 2, textWidth, lineHeight);
        const int distanceToEdge = qMin(textRect.center().y(), height() - textRect.center().y());
        const double fadeOpacity = qBound(0.0, distanceToEdge / 90.0, 1.0);
        painter.setOpacity(fadeOpacity);

        if (i == currentIndex && !m_browsing) {
            painter.setPen(mutedText);
            painter.drawText(textRect, Qt::AlignCenter, line->text);

            painter.save();
            painter.setClipRect(QRect(textRect.left(), textRect.top(), qRound(textRect.width() * lineProgress(i)), textRect.height()));
            painter.setPen(accent);
            painter.drawText(textRect, Qt::AlignCenter, line->text);
            painter.restore();
        } else {
            if (i == currentIndex) {
                painter.setPen(accent);
            } else if (m_browsing && i == centerIndex) {
                painter.setPen(primaryText);
            } else {
                painter.setPen(mutedText);
            }
            painter.drawText(textRect, Qt::AlignCenter, line->text);
        }
        painter.setOpacity(1.0);
    }

    QLinearGradient topFade(0, 0, 0, qMin(90, height() / 3));
    topFade.setColorAt(0.0, background);
    topFade.setColorAt(1.0, QColor(background.red(), background.green(), background.blue(), 0));
    painter.fillRect(QRect(0, 0, width(), qMin(90, height() / 3)), topFade);

    QLinearGradient bottomFade(0, height() - qMin(90, height() / 3), 0, height());
    bottomFade.setColorAt(0.0, QColor(background.red(), background.green(), background.blue(), 0));
    bottomFade.setColorAt(1.0, background);
    painter.fillRect(QRect(0, height() - qMin(90, height() / 3), width(), qMin(90, height() / 3)), bottomFade);
}

int LyricsWidget::playbackLineIndex() const
{
    return m_lyrics.currentLineIndex(effectivePositionMs(), m_userOffsetMs);
}

qint64 LyricsWidget::effectivePositionMs() const
{
    if (!m_playbackActive || !m_positionClock.isValid()) {
        return m_positionMs;
    }
    return m_positionMs + m_positionClock.elapsed();
}

qint64 LyricsWidget::adjustedPositionMs() const
{
    return effectivePositionMs() + m_lyrics.baseOffsetMs() + m_userOffsetMs;
}

double LyricsWidget::lineProgress(int lineIndex) const
{
    const LyricLine *current = m_lyrics.lineAt(lineIndex);
    if (current == nullptr) {
        return 0.0;
    }

    qint64 endTime = current->timeMs + 4000;
    if (const LyricLine *next = m_lyrics.lineAt(lineIndex + 1)) {
        endTime = next->timeMs;
    }

    const qint64 duration = qMax<qint64>(1, endTime - current->timeMs);
    return qBound(0.0, (adjustedPositionMs() - current->timeMs) / static_cast<double>(duration), 1.0);
}

double LyricsWidget::targetCenterPosition() const
{
    const int index = playbackLineIndex();
    if (index < 0) {
        return 0.0;
    }
    return index;
}

void LyricsWidget::updatePlaybackCenter()
{
    const int index = playbackLineIndex();
    if (index < 0 || m_browsing) {
        return;
    }

    if (m_lastPlaybackLineIndex < 0) {
        m_lastPlaybackLineIndex = index;
        m_centerLine = index;
        return;
    }

    if (index != m_lastPlaybackLineIndex) {
        m_lastPlaybackLineIndex = index;
        m_centerAnimation->stop();
        m_centerAnimation->setStartValue(m_centerLine);
        m_centerAnimation->setEndValue(static_cast<double>(index));
        m_centerAnimation->start();
    }
}

int LyricsWidget::displayCenterIndex() const
{
    return qBound(0, qRound(m_centerLine + m_browseOffsetLines), qMax(0, m_lyrics.lineCount() - 1));
}

int LyricsWidget::lineIndexAtCenter() const
{
    return displayCenterIndex();
}

bool LyricsWidget::isJumpHotspot(const QPoint &pos) const
{
    if (!m_browsing || m_lyrics.isEmpty()) {
        return false;
    }
    return m_jumpButtonRect.contains(pos) || m_centerLineRect.contains(pos);
}

bool LyricsWidget::isLoadButtonHotspot(const QPoint &pos) const
{
    return m_showLoadButton && m_lyrics.isEmpty() && m_loadButtonRect.contains(pos);
}

void LyricsWidget::jumpToCenterLine()
{
    const LyricLine *line = m_lyrics.lineAt(lineIndexAtCenter());
    if (line == nullptr) {
        return;
    }

    m_returnTimer.stop();
    m_browsing = false;
    m_dragging = false;
    m_browseOffsetLines = 0;
    unsetCursor();
    emit jumpRequested(qMax<qint64>(0, line->timeMs - m_lyrics.baseOffsetMs() - m_userOffsetMs));
    update();
}

void LyricsWidget::resetBrowseTimer()
{
    if (m_browsing) {
        m_returnTimer.start();
    }
}
