#ifndef MVPLAYERDIALOG_H
#define MVPLAYERDIALOG_H

#include <QDialog>

class QAudioOutput;
class QCloseEvent;
class QLabel;
class QMediaPlayer;
class QPushButton;
class QSlider;
class QVideoWidget;
class SeekSlider;

class MvPlayerDialog : public QDialog
{
    Q_OBJECT

public:
    explicit MvPlayerDialog(bool nightMode, QWidget *parent = nullptr);

protected:
    void closeEvent(QCloseEvent *event) override;

private:
    void buildUi();
    void applyStyle();
    void connectSignals();
    void openVideoFile();
    void togglePlayPause();
    void updatePlayPauseButton();
    void updateTimeLabel(qint64 positionMs, qint64 durationMs);
    QString assetPath(const QString &relativePath) const;
    QString themeName() const;
    QString formatTime(qint64 milliseconds) const;

    bool m_nightMode = false;
    QMediaPlayer *m_player = nullptr;
    QAudioOutput *m_audioOutput = nullptr;
    QVideoWidget *m_videoWidget = nullptr;
    QPushButton *m_openButton = nullptr;
    QPushButton *m_playPauseButton = nullptr;
    QPushButton *m_stopButton = nullptr;
    QPushButton *m_closeButton = nullptr;
    SeekSlider *m_positionSlider = nullptr;
    QSlider *m_volumeSlider = nullptr;
    QLabel *m_titleLabel = nullptr;
    QLabel *m_hintLabel = nullptr;
    QLabel *m_timeLabel = nullptr;
    QLabel *m_statusLabel = nullptr;
    bool m_sliderPressed = false;
    qint64 m_durationMs = 0;
};

#endif // MVPLAYERDIALOG_H
