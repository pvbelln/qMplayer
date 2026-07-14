#include "transcodedialog.h"

#include <QAbstractItemView>
#include <QComboBox>
#include <QCoreApplication>
#include <QDir>
#include <QFileDialog>
#include <QFileInfo>
#include <QFrame>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QMessageBox>
#include <QProcess>
#include <QProgressBar>
#include <QPushButton>
#include <QVBoxLayout>

namespace
{
void polishComboPopup(QComboBox *comboBox, const QString &backgroundColor)
{
    if (comboBox == nullptr || comboBox->view() == nullptr) {
        return;
    }

    comboBox->view()->setFrameShape(QFrame::NoFrame);
    comboBox->view()->setStyleSheet(QString("background: %1; border: none;").arg(backgroundColor));
    comboBox->view()->setAutoFillBackground(false);
    if (comboBox->view()->viewport() != nullptr) {
        comboBox->view()->viewport()->setStyleSheet(QString("background: %1; border: none;").arg(backgroundColor));
        comboBox->view()->viewport()->setAutoFillBackground(true);
    }

    QWidget *popupContainer = comboBox->view()->parentWidget();
    if (popupContainer != nullptr) {
        popupContainer->setStyleSheet(QString("background: %1; border: none;").arg(backgroundColor));
        popupContainer->setAutoFillBackground(true);
        popupContainer->setWindowFlags(Qt::Popup | Qt::FramelessWindowHint);
    }
}
}

TranscodeDialog::TranscodeDialog(const QString &ffmpegPath,
                                 const QString &initialFilePath,
                                 qint64 initialDurationMs,
                                 bool nightMode,
                                 QWidget *parent)
    : QDialog(parent),
      m_ffmpegPath(ffmpegPath),
      m_nightMode(nightMode),
      transcoder(this)
{
    setWindowTitle("Transcode");
    resize(680, 480);
    buildUi();
    applyStyle();
    connectSignals();

    if (!initialFilePath.isEmpty()) {
        m_jobs.push_back({initialFilePath, initialDurationMs});
        fileListWidget->addItem(initialFilePath);
        outputFolderEdit->setText(QFileInfo(initialFilePath).absolutePath());
    }
}

void TranscodeDialog::applyStyle()
{
    if (!m_nightMode) {
        setStyleSheet(R"(
        QDialog {
            background: #ffffff;
            color: #172033;
            font-family: "Segoe UI", "Microsoft YaHei UI", sans-serif;
            font-size: 13px;
        }
        QLabel {
            background: transparent;
            color: #40516c;
            font-weight: 600;
        }
        QListWidget, QLineEdit {
            background: #f7fafc;
            border: 1px solid #d8e1ee;
            border-radius: 12px;
            padding: 8px 10px;
            outline: none;
            selection-background-color: #edf5ff;
            selection-color: #12345d;
        }
        QListWidget::item {
            border-radius: 8px;
            padding: 7px 8px;
            margin: 1px 0;
        }
        QListWidget::item:selected {
            background: #e8f2ff;
            color: #174ea6;
        }
        QComboBox {
            background: #f7fafc;
            border: 1px solid #d8e1ee;
            border-radius: 12px;
            padding: 8px 30px 8px 12px;
            color: #263850;
            combobox-popup: 1;
        }
        QComboBox:hover, QLineEdit:focus {
            border-color: #9fb8d6;
            background: #ffffff;
        }
        QComboBox::drop-down {
            border: none;
            width: 26px;
            subcontrol-origin: padding;
            subcontrol-position: top right;
            background: transparent;
        }
        QComboBox::down-arrow {
            image: none;
            width: 0px;
            height: 0px;
            border-left: 5px solid transparent;
            border-right: 5px solid transparent;
            border-top: 6px solid #61718a;
            margin-right: 9px;
        }
        QComboBox QAbstractItemView {
            background: #ffffff;
            border: none;
            border-radius: 12px;
            padding: 6px;
            outline: none;
            color: #263850;
            selection-background-color: #edf5ff;
            selection-color: #12345d;
        }
        QComboBox QAbstractItemView::viewport {
            background: #ffffff;
            border: none;
            border-radius: 12px;
        }
        QComboBox QAbstractItemView::item {
            color: #263850;
            background: transparent;
            min-height: 28px;
            padding: 6px 10px;
            border-radius: 8px;
        }
        QComboBox QAbstractItemView::item:selected {
            background: #edf5ff;
            color: #12345d;
        }
        QPushButton {
            background: #f8fbff;
            border: 1px solid #d4dfec;
            border-radius: 10px;
            padding: 8px 13px;
            color: #22314a;
            font-weight: 700;
        }
        QPushButton:hover {
            background: #edf5ff;
            border-color: #8fb8f4;
        }
        QPushButton:disabled {
            color: #a5b1c2;
            background: #f2f5f8;
            border-color: #e0e7f0;
        }
        QProgressBar {
            background: #e2e8f0;
            border: none;
            border-radius: 6px;
            height: 12px;
            text-align: center;
            color: #40516c;
            font-size: 11px;
        }
        QProgressBar::chunk {
            background: #1f6f78;
            border-radius: 6px;
        }
        QScrollBar:vertical {
            background: transparent;
            border: none;
            border-radius: 6px;
            width: 12px;
            margin: 4px 0;
        }
        QScrollBar::handle:vertical {
            background: #c8d5e6;
            border: none;
            border-radius: 6px;
            min-height: 32px;
            margin: 1px 2px;
        }
        QScrollBar::handle:vertical:hover {
            background: #9eb6d4;
        }
        QScrollBar:horizontal {
            background: transparent;
            border: none;
            border-radius: 6px;
            height: 12px;
            margin: 0 4px;
        }
        QScrollBar::handle:horizontal {
            background: #c8d5e6;
            border: none;
            border-radius: 6px;
            min-width: 32px;
            margin: 2px 1px;
        }
        QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical,
        QScrollBar::add-page:vertical, QScrollBar::sub-page:vertical,
        QScrollBar::add-line:horizontal, QScrollBar::sub-line:horizontal,
        QScrollBar::add-page:horizontal, QScrollBar::sub-page:horizontal {
            background: transparent;
            border: none;
            width: 0;
            height: 0;
        }
        QScrollBar::corner {
            background: transparent;
            border: none;
        }
    )");
        return;
    }

    setStyleSheet(R"(
        QDialog {
            background: #171b24;
            color: #edf2f7;
            font-family: "Segoe UI", "Microsoft YaHei UI", sans-serif;
            font-size: 13px;
        }
        QLabel {
            background: transparent;
            color: #c2ccda;
            font-weight: 600;
        }
        QListWidget, QLineEdit {
            background: #1f2530;
            border: 1px solid #303847;
            border-radius: 12px;
            padding: 8px 10px;
            outline: none;
            color: #e8edf5;
            selection-background-color: #263957;
            selection-color: #ffffff;
        }
        QListWidget::item {
            border-radius: 8px;
            padding: 7px 8px;
            margin: 1px 0;
        }
        QListWidget::item:selected {
            background: #23344f;
            color: #ffffff;
        }
        QComboBox {
            background: #1f2530;
            border: 1px solid #303847;
            border-radius: 12px;
            padding: 8px 30px 8px 12px;
            color: #e8edf5;
            combobox-popup: 1;
        }
        QComboBox:hover, QLineEdit:focus {
            border-color: #52647d;
            background: #242d3b;
        }
        QComboBox::drop-down {
            border: none;
            width: 26px;
            subcontrol-origin: padding;
            subcontrol-position: top right;
            background: transparent;
        }
        QComboBox::down-arrow {
            image: none;
            width: 0px;
            height: 0px;
            border-left: 5px solid transparent;
            border-right: 5px solid transparent;
            border-top: 6px solid #aeb8c8;
            margin-right: 9px;
        }
        QComboBox QAbstractItemView {
            background: #1f2530;
            border: none;
            border-radius: 12px;
            padding: 6px;
            outline: none;
            color: #e8edf5;
            selection-background-color: #263957;
            selection-color: #ffffff;
        }
        QComboBox QAbstractItemView::viewport {
            background: #1f2530;
            border: none;
            border-radius: 12px;
        }
        QComboBox QAbstractItemView::item {
            color: #e8edf5;
            background: transparent;
            min-height: 28px;
            padding: 6px 10px;
            border-radius: 8px;
        }
        QComboBox QAbstractItemView::item:selected {
            background: #263957;
            color: #ffffff;
        }
        QPushButton {
            background: #202631;
            border: 1px solid #323b4c;
            border-radius: 10px;
            padding: 8px 13px;
            color: #e8edf5;
            font-weight: 700;
        }
        QPushButton:hover {
            background: #2a3342;
            border-color: #61799f;
        }
        QPushButton:disabled {
            color: #687385;
            background: #1a1f29;
            border-color: #262d39;
        }
        QProgressBar {
            background: #293241;
            border: none;
            border-radius: 6px;
            height: 12px;
            text-align: center;
            color: #c2ccda;
            font-size: 11px;
        }
        QProgressBar::chunk {
            background: #8fb3ff;
            border-radius: 6px;
        }
        QScrollBar:vertical {
            background: transparent;
            border: none;
            border-radius: 6px;
            width: 12px;
            margin: 4px 0;
        }
        QScrollBar::handle:vertical {
            background: #3a4658;
            border: none;
            border-radius: 6px;
            min-height: 32px;
            margin: 1px 2px;
        }
        QScrollBar::handle:vertical:hover {
            background: #53657c;
        }
        QScrollBar:horizontal {
            background: transparent;
            border: none;
            border-radius: 6px;
            height: 12px;
            margin: 0 4px;
        }
        QScrollBar::handle:horizontal {
            background: #3a4658;
            border: none;
            border-radius: 6px;
            min-width: 32px;
            margin: 2px 1px;
        }
        QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical,
        QScrollBar::add-page:vertical, QScrollBar::sub-page:vertical,
        QScrollBar::add-line:horizontal, QScrollBar::sub-line:horizontal,
        QScrollBar::add-page:horizontal, QScrollBar::sub-page:horizontal {
            background: transparent;
            border: none;
            width: 0;
            height: 0;
        }
        QScrollBar::corner {
            background: transparent;
            border: none;
        }
    )");
}

void TranscodeDialog::buildUi()
{
    auto *rootLayout = new QVBoxLayout(this);

    fileListWidget = new QListWidget(this);
    auto *fileButtonsLayout = new QHBoxLayout;
    addFilesButton = new QPushButton("Add Files", this);
    removeFilesButton = new QPushButton("Remove", this);
    fileButtonsLayout->addWidget(addFilesButton);
    fileButtonsLayout->addWidget(removeFilesButton);
    fileButtonsLayout->addStretch();

    formatCombo = new QComboBox(this);
    formatCombo->addItems({"mp3", "wav", "flac", "aac", "ogg"});
    polishComboPopup(formatCombo, m_nightMode ? "#1f2530" : "#ffffff");
    bitrateCombo = new QComboBox(this);
    bitrateCombo->addItems({"Original", "128k", "192k", "256k", "320k"});
    bitrateCombo->setCurrentText("192k");
    polishComboPopup(bitrateCombo, m_nightMode ? "#1f2530" : "#ffffff");
    sampleRateCombo = new QComboBox(this);
    sampleRateCombo->addItems({"Original", "44100", "48000"});
    polishComboPopup(sampleRateCombo, m_nightMode ? "#1f2530" : "#ffffff");

    auto *optionsLayout = new QHBoxLayout;
    optionsLayout->addWidget(new QLabel("Format", this));
    optionsLayout->addWidget(formatCombo);
    optionsLayout->addWidget(new QLabel("Bitrate", this));
    optionsLayout->addWidget(bitrateCombo);
    optionsLayout->addWidget(new QLabel("Sample rate", this));
    optionsLayout->addWidget(sampleRateCombo);

    outputFolderEdit = new QLineEdit(this);
    browseOutputButton = new QPushButton("Browse", this);
    auto *outputLayout = new QHBoxLayout;
    outputLayout->addWidget(new QLabel("Output folder", this));
    outputLayout->addWidget(outputFolderEdit, 1);
    outputLayout->addWidget(browseOutputButton);

    currentProgressBar = new QProgressBar(this);
    currentProgressBar->setRange(0, 100);
    totalProgressBar = new QProgressBar(this);
    totalProgressBar->setRange(0, 100);
    statusLabel = new QLabel("Ready", this);

    auto *controlLayout = new QHBoxLayout;
    startButton = new QPushButton("Start", this);
    pauseButton = new QPushButton("Pause", this);
    stopButton = new QPushButton("Stop", this);
    closeButton = new QPushButton("Close", this);
    pauseButton->setEnabled(false);
    stopButton->setEnabled(false);
    controlLayout->addWidget(startButton);
    controlLayout->addWidget(pauseButton);
    controlLayout->addWidget(stopButton);
    controlLayout->addStretch();
    controlLayout->addWidget(closeButton);

    rootLayout->addWidget(new QLabel("Files", this));
    rootLayout->addWidget(fileListWidget, 1);
    rootLayout->addLayout(fileButtonsLayout);
    rootLayout->addLayout(optionsLayout);
    rootLayout->addLayout(outputLayout);
    rootLayout->addWidget(new QLabel("Current file", this));
    rootLayout->addWidget(currentProgressBar);
    rootLayout->addWidget(new QLabel("Batch total", this));
    rootLayout->addWidget(totalProgressBar);
    rootLayout->addWidget(statusLabel);
    rootLayout->addLayout(controlLayout);
}

void TranscodeDialog::connectSignals()
{
    connect(addFilesButton, &QPushButton::clicked, this, &TranscodeDialog::addFiles);
    connect(removeFilesButton, &QPushButton::clicked, this, &TranscodeDialog::removeSelectedFiles);
    connect(browseOutputButton, &QPushButton::clicked, this, &TranscodeDialog::chooseOutputFolder);
    connect(startButton, &QPushButton::clicked, this, &TranscodeDialog::startBatch);
    connect(pauseButton, &QPushButton::clicked, this, [this]() {
        transcoder.togglePause();
        pauseButton->setText(transcoder.isPaused() ? "Resume" : "Pause");
    });
    connect(stopButton, &QPushButton::clicked, this, &TranscodeDialog::stopBatch);
    connect(closeButton, &QPushButton::clicked, this, &QDialog::accept);
    connect(&transcoder, &Transcoder::statusChanged, statusLabel, &QLabel::setText);
    connect(&transcoder, &Transcoder::progressChanged, this, [this](int percent) {
        currentProgressBar->setValue(percent);
        const int completedPercent = m_jobs.isEmpty()
            ? 0
            : static_cast<int>(((m_currentJobIndex * 100) + percent) / m_jobs.size());
        totalProgressBar->setValue(qBound(0, completedPercent, 100));
    });
    connect(&transcoder, &Transcoder::finished, this, [this](bool success, const QString &message) {
        statusLabel->setText(message);
        if (!success || m_batchStopping) {
            updateButtons(false);
            m_batchStopping = false;
            return;
        }
        startNextJob();
    });
}

void TranscodeDialog::addFiles()
{
    const QStringList files = QFileDialog::getOpenFileNames(
        this,
        "Add audio files",
        QString(),
        "Audio Files (*.mp3 *.flac *.wav *.m4a *.aac *.ogg);;All Files (*.*)");
    for (const QString &file : files) {
        bool exists = false;
        for (const TranscodeJob &job : m_jobs) {
            if (QFileInfo(job.inputPath).absoluteFilePath() == QFileInfo(file).absoluteFilePath()) {
                exists = true;
                break;
            }
        }
        if (!exists) {
            m_jobs.push_back({file, probeDurationMs(file)});
            fileListWidget->addItem(file);
        }
    }
    if (outputFolderEdit->text().isEmpty() && !m_jobs.isEmpty()) {
        outputFolderEdit->setText(QFileInfo(m_jobs.first().inputPath).absolutePath());
    }
}

void TranscodeDialog::removeSelectedFiles()
{
    const auto selectedItems = fileListWidget->selectedItems();
    for (QListWidgetItem *item : selectedItems) {
        const int row = fileListWidget->row(item);
        if (row >= 0 && row < m_jobs.size()) {
            m_jobs.remove(row);
        }
        delete fileListWidget->takeItem(row);
    }
}

void TranscodeDialog::chooseOutputFolder()
{
    const QString folder = QFileDialog::getExistingDirectory(this, "Choose output folder", outputFolderEdit->text());
    if (!folder.isEmpty()) {
        outputFolderEdit->setText(folder);
    }
}

void TranscodeDialog::startBatch()
{
    if (m_ffmpegPath.isEmpty() || !QFileInfo::exists(m_ffmpegPath)) {
        QMessageBox::warning(this, "Transcode", "ffmpeg.exe was not found.");
        return;
    }
    if (m_jobs.isEmpty()) {
        QMessageBox::information(this, "Transcode", "Add at least one file.");
        return;
    }
    if (outputFolderEdit->text().trimmed().isEmpty()) {
        QMessageBox::information(this, "Transcode", "Choose an output folder.");
        return;
    }

    QDir().mkpath(outputFolderEdit->text().trimmed());
    m_currentJobIndex = -1;
    m_batchStopping = false;
    currentProgressBar->setValue(0);
    totalProgressBar->setValue(0);
    updateButtons(true);
    startNextJob();
}

void TranscodeDialog::startNextJob()
{
    ++m_currentJobIndex;
    if (m_currentJobIndex >= m_jobs.size()) {
        statusLabel->setText("Batch transcode finished");
        currentProgressBar->setValue(100);
        totalProgressBar->setValue(100);
        updateButtons(false);
        return;
    }

    fileListWidget->setCurrentRow(m_currentJobIndex);
    const TranscodeJob &job = m_jobs[m_currentJobIndex];
    TranscodeOptions options;
    options.ffmpegPath = m_ffmpegPath;
    options.inputPath = job.inputPath;
    options.outputPath = outputPathFor(job.inputPath);
    options.format = formatCombo->currentText();
    options.bitrate = bitrateCombo->currentText();
    options.sampleRate = sampleRateCombo->currentText();
    options.durationMs = job.durationMs > 0 ? job.durationMs : probeDurationMs(job.inputPath);
    currentProgressBar->setValue(0);
    statusLabel->setText(QString("Transcoding %1").arg(QFileInfo(job.inputPath).fileName()));
    transcoder.start(options);
}

void TranscodeDialog::stopBatch()
{
    m_batchStopping = true;
    transcoder.stop();
    updateButtons(false);
}

void TranscodeDialog::updateButtons(bool running)
{
    startButton->setEnabled(!running);
    addFilesButton->setEnabled(!running);
    removeFilesButton->setEnabled(!running);
    formatCombo->setEnabled(!running);
    bitrateCombo->setEnabled(!running);
    sampleRateCombo->setEnabled(!running);
    outputFolderEdit->setEnabled(!running);
    browseOutputButton->setEnabled(!running);
    pauseButton->setEnabled(running);
    stopButton->setEnabled(running);
    pauseButton->setText("Pause");
}

QString TranscodeDialog::outputPathFor(const QString &inputPath) const
{
    const QFileInfo info(inputPath);
    const QString format = formatCombo->currentText();
    QString outputPath = QDir(outputFolderEdit->text().trimmed()).filePath(info.completeBaseName() + "." + format);
    if (QFileInfo(outputPath).absoluteFilePath() == info.absoluteFilePath()) {
        outputPath = QDir(outputFolderEdit->text().trimmed()).filePath(info.completeBaseName() + "_converted." + format);
    }
    return outputPath;
}

QString TranscodeDialog::ffprobePath() const
{
    const QFileInfo ffmpegInfo(m_ffmpegPath);
    const QString path = QDir(ffmpegInfo.absolutePath()).filePath("ffprobe.exe");
    return QFileInfo::exists(path) ? path : QString();
}

qint64 TranscodeDialog::probeDurationMs(const QString &filePath) const
{
    const QString program = ffprobePath();
    if (program.isEmpty()) {
        return 0;
    }

    QProcess process;
    process.start(program, {"-v", "error", "-show_entries", "format=duration", "-of", "default=noprint_wrappers=1:nokey=1", filePath});
    if (!process.waitForFinished(3000)) {
        process.kill();
        return 0;
    }

    const double seconds = QString::fromLocal8Bit(process.readAllStandardOutput()).trimmed().toDouble();
    return seconds > 0 ? static_cast<qint64>(seconds * 1000) : 0;
}
