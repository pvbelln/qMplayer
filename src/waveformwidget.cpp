#include "waveformwidget.h"

#include <QPainter>
#include <QtMath>

WaveformWidget::WaveformWidget(QWidget *parent)
    : QWidget(parent)
{
    setObjectName("waveformWidget");
    setFixedHeight(34);
    m_pulseTimer.setInterval(33);
    connect(&m_pulseTimer, &QTimer::timeout, this, [this]() {
        m_pulsePhase += 0.16;
        if (m_pulsePhase > 1000.0) {
            m_pulsePhase = 0.0;
        }
        if (m_playbackActive) {
            update();
        }
    });
}

void WaveformWidget::setEnergies(const QVector<double> &energies)
{
    m_energies = energies;
    update();
}

void WaveformWidget::setPosition(qint64 positionMs)
{
    m_positionMs = qMax<qint64>(0, positionMs);
    update();
}

void WaveformWidget::setDuration(qint64 durationMs)
{
    m_durationMs = qMax<qint64>(0, durationMs);
    update();
}

void WaveformWidget::setPlaybackActive(bool active)
{
    m_playbackActive = active;
    if (active && !m_pulseTimer.isActive()) {
        m_pulseTimer.start();
    } else if (!active) {
        m_pulseTimer.stop();
    }
    update();
}

void WaveformWidget::setNightMode(bool nightMode)
{
    m_nightMode = nightMode;
    update();
}

void WaveformWidget::clear()
{
    m_energies.clear();
    m_positionMs = 0;
    m_durationMs = 0;
    update();
}

void WaveformWidget::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event)

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    const QColor base = m_nightMode ? QColor("#303847") : QColor("#d8e1ee");
    const QColor played = m_nightMode ? QColor("#8fb3ff") : QColor("#1f6f78");
    const QColor hot = m_nightMode ? QColor("#d7e4ff") : QColor("#2563eb");

    const QRectF area = rect().adjusted(2, 3, -2, -3);
    const double centerY = area.center().y();

    if (m_energies.isEmpty()) {
        QPen pen(base, 2.0, Qt::SolidLine, Qt::RoundCap);
        painter.setPen(pen);
        painter.drawLine(QPointF(area.left(), centerY), QPointF(area.right(), centerY));
        return;
    }

    const int count = m_energies.size();
    const double gap = 1.5;
    const double barWidth = qMax(2.0, (area.width() - gap * (count - 1)) / count);
    const double progress = m_durationMs > 0
        ? qBound(0.0, m_positionMs / static_cast<double>(m_durationMs), 1.0)
        : 0.0;
    const double activeIndex = progress * qMax(1, count - 1);

    painter.setPen(Qt::NoPen);
    for (int i = 0; i < count; ++i) {
        const double distance = qAbs(i - activeIndex);
        const double pulse = m_playbackActive
            ? qMax(0.0, 1.0 - distance / 4.0) * (0.16 + 0.10 * qSin(m_pulsePhase + i * 0.45))
            : 0.0;
        const double energy = qBound(0.04, m_energies[i] + pulse, 1.0);
        const double height = qMax(3.0, energy * area.height());
        const double x = area.left() + i * (barWidth + gap);
        const QRectF bar(x, centerY - height / 2.0, barWidth, height);

        QColor color = i <= activeIndex ? played : base;
        if (distance < 1.5) {
            color = hot;
        }
        painter.setBrush(color);
        painter.drawRoundedRect(bar, barWidth / 2.0, barWidth / 2.0);
    }
}
