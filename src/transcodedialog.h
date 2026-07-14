#ifndef TRANSCODEDIALOG_H
#define TRANSCODEDIALOG_H

#include "transcoder.h"

#include <QDialog>
#include <QVector>

class QComboBox;
class QLabel;
class QLineEdit;
class QListWidget;
class QProgressBar;
class QPushButton;

struct TranscodeJob
{
    QString inputPath;
    qint64 durationMs = 0;
};

class TranscodeDialog : public QDialog
{
    Q_OBJECT

public:
    explicit TranscodeDialog(const QString &ffmpegPath,
                             const QString &initialFilePath,
                             qint64 initialDurationMs,
                             bool nightMode,
                             QWidget *parent = nullptr);

private:
    void buildUi();
    void applyStyle();
    void connectSignals();
    void addFiles();
    void removeSelectedFiles();
    void chooseOutputFolder();
    void startBatch();
    void startNextJob();
    void stopBatch();
    void updateButtons(bool running);
    QString outputPathFor(const QString &inputPath) const;
    QString ffprobePath() const;
    qint64 probeDurationMs(const QString &filePath) const;

    QString m_ffmpegPath;
    QVector<TranscodeJob> m_jobs;
    int m_currentJobIndex = -1;
    bool m_batchStopping = false;
    bool m_nightMode = false;

    QListWidget *fileListWidget = nullptr;
    QComboBox *formatCombo = nullptr;
    QComboBox *bitrateCombo = nullptr;
    QComboBox *sampleRateCombo = nullptr;
    QLineEdit *outputFolderEdit = nullptr;
    QProgressBar *currentProgressBar = nullptr;
    QProgressBar *totalProgressBar = nullptr;
    QLabel *statusLabel = nullptr;
    QPushButton *addFilesButton = nullptr;
    QPushButton *removeFilesButton = nullptr;
    QPushButton *browseOutputButton = nullptr;
    QPushButton *startButton = nullptr;
    QPushButton *pauseButton = nullptr;
    QPushButton *stopButton = nullptr;
    QPushButton *closeButton = nullptr;

    Transcoder transcoder;
};

#endif // TRANSCODEDIALOG_H
