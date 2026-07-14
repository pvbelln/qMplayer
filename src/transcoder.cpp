#include "transcoder.h"

#include <QFileInfo>
#include <QRegularExpression>

#ifdef Q_OS_WIN
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <tlhelp32.h>
#endif

Transcoder::Transcoder(QObject *parent)
    : QObject(parent),
      m_process(new QProcess(this))
{
    m_process->setProcessChannelMode(QProcess::SeparateChannels);

    connect(m_process, &QProcess::readyReadStandardError, this, [this]() {
        const QString text = QString::fromLocal8Bit(m_process->readAllStandardError());
        const qint64 currentMs = parseProgressTimeMs(text);
        if (currentMs >= 0) {
            if (m_options.durationMs > 0) {
                const int percent = qBound(0, static_cast<int>((currentMs * 100) / m_options.durationMs), 100);
                emit progressChanged(percent);
                emit statusChanged(QString("Transcoding... %1%").arg(percent));
            } else {
                emit statusChanged("Transcoding...");
            }
        } else if (text.contains("Error", Qt::CaseInsensitive) || text.contains("Invalid", Qt::CaseInsensitive)) {
            emit statusChanged(text.split('\n', Qt::SkipEmptyParts).value(0).trimmed());
        }
    });

    connect(m_process, &QProcess::errorOccurred, this, [this](QProcess::ProcessError) {
        if (m_stopping) {
            return;
        }
        emit finished(false, m_process->errorString());
    });

    connect(m_process, qOverload<int, QProcess::ExitStatus>(&QProcess::finished),
            this, [this](int exitCode, QProcess::ExitStatus exitStatus) {
        m_paused = false;
        if (m_stopping) {
            m_stopping = false;
            emit finished(false, "Transcode stopped");
            return;
        }
        const bool success = exitStatus == QProcess::NormalExit && exitCode == 0 && QFileInfo::exists(m_outputPath);
        if (success) {
            emit progressChanged(100);
        }
        emit finished(success, success ? QString("Transcode finished") : QString("Transcode failed"));
    });
}

bool Transcoder::isRunning() const
{
    return m_process->state() != QProcess::NotRunning;
}

bool Transcoder::isPaused() const
{
    return m_paused;
}

void Transcoder::start(const QString &ffmpegPath, const QString &inputPath, const QString &outputPath)
{
    TranscodeOptions options;
    options.ffmpegPath = ffmpegPath;
    options.inputPath = inputPath;
    options.outputPath = outputPath;
    start(options);
}

void Transcoder::start(const TranscodeOptions &options)
{
    if (isRunning()) {
        emit statusChanged("Transcoder is already running");
        return;
    }

    m_options = options;
    m_outputPath = options.outputPath;
    m_paused = false;
    m_stopping = false;
    QStringList arguments;
    arguments << "-y" << "-i" << options.inputPath;
    if (!options.bitrate.isEmpty() && options.bitrate != "Original") {
        arguments << "-b:a" << options.bitrate;
    }
    if (!options.sampleRate.isEmpty() && options.sampleRate != "Original") {
        arguments << "-ar" << options.sampleRate;
    }
    arguments << options.outputPath;
    emit progressChanged(0);
    emit statusChanged("Transcode started");
    m_process->start(options.ffmpegPath, arguments);
}

void Transcoder::stop()
{
    if (!isRunning()) {
        return;
    }
    if (m_paused) {
        setProcessSuspended(false);
        m_paused = false;
    }
    m_stopping = true;
    m_process->kill();
}

void Transcoder::togglePause()
{
    if (!isRunning()) {
        return;
    }

    const bool targetPaused = !m_paused;
    if (setProcessSuspended(targetPaused)) {
        m_paused = targetPaused;
        emit statusChanged(m_paused ? "Transcode paused" : "Transcode resumed");
    } else {
        emit statusChanged("Pause is not supported on this platform");
    }
}

qint64 Transcoder::parseProgressTimeMs(const QString &text) const
{
    static const QRegularExpression timeExpression(R"(time=(\d+:\d+:\d+(?:\.\d+)?))");
    const QRegularExpressionMatch match = timeExpression.match(text);
    if (!match.hasMatch()) {
        return -1;
    }

    const QStringList parts = match.captured(1).split(':');
    if (parts.size() != 3) {
        return -1;
    }

    const qint64 hours = parts[0].toLongLong();
    const qint64 minutes = parts[1].toLongLong();
    const double seconds = parts[2].toDouble();
    return (hours * 3600 + minutes * 60) * 1000 + static_cast<qint64>(seconds * 1000);
}

bool Transcoder::setProcessSuspended(bool suspended)
{
#ifdef Q_OS_WIN
    const DWORD processId = static_cast<DWORD>(m_process->processId());
    if (processId == 0) {
        return false;
    }

    HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD, 0);
    if (snapshot == INVALID_HANDLE_VALUE) {
        return false;
    }

    THREADENTRY32 entry;
    entry.dwSize = sizeof(THREADENTRY32);
    bool changed = false;

    if (Thread32First(snapshot, &entry)) {
        do {
            if (entry.th32OwnerProcessID == processId) {
                HANDLE thread = OpenThread(THREAD_SUSPEND_RESUME, FALSE, entry.th32ThreadID);
                if (thread != nullptr) {
                    if (suspended) {
                        SuspendThread(thread);
                    } else {
                        ResumeThread(thread);
                    }
                    CloseHandle(thread);
                    changed = true;
                }
            }
        } while (Thread32Next(snapshot, &entry));
    }

    CloseHandle(snapshot);
    return changed;
#else
    Q_UNUSED(suspended)
    return false;
#endif
}
