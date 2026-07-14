#ifndef WAVEFORMWIDGET_H
#define WAVEFORMWIDGET_H

#include <QTimer>
#include <QVector>
#include <QWidget>

class WaveformWidget : public QWidget
{
    Q_OBJECT

public:
    explicit WaveformWidget(QWidget *parent = nullptr);

    void setEnergies(const QVector<double> &energies);
    void setPosition(qint64 positionMs);
    void setDuration(qint64 durationMs);
    void setPlaybackActive(bool active);
    void setNightMode(bool nightMode);
    void clear();

protected:
    void paintEvent(QPaintEvent *event) override;

private:
    QVector<double> m_energies;
    qint64 m_positionMs = 0;
    qint64 m_durationMs = 0;
    bool m_playbackActive = false;
    bool m_nightMode = false;
    double m_pulsePhase = 0.0;
    QTimer m_pulseTimer;
};

#endif // WAVEFORMWIDGET_H
