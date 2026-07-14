#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "audioplayer.h"
#include "librarystore.h"
#include "lyrics.h"
#include "playlist.h"
#include "playbackstrategy.h"
#include "transcoder.h"

#include <memory>
#include <vector>
#include <QWidget>

class QLabel;
class QAudioBuffer;
class QAudioDecoder;
class QCloseEvent;
class QComboBox;
class QKeyEvent;
class QListWidget;
class QListWidgetItem;
class QLineEdit;
class QPushButton;
class QSlider;
class LyricsWidget;
class WaveformWidget;

class MainWindow : public QWidget
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);

protected:
    void closeEvent(QCloseEvent *event) override;
    bool eventFilter(QObject *watched, QEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;
    void keyReleaseEvent(QKeyEvent *event) override;

private:
    void buildUi();
    void applyBaseStyle();
    void connectSignals();
    void setupIconButton(QPushButton *button, const QString &iconName, const QString &toolTip);
    void updatePlayPauseButton(bool playing);
    void updateMuteButton();
    void updateModeButton();
    void updateThemedIcons();
    void toggleTheme();
    void showAboutDialog();
    void showHelpDialog();
    void showMvWindow();
    QString themeName() const;
    bool handlePlaybackShortcut(QKeyEvent *event);
    bool handleKeyboardSeekRelease(QKeyEvent *event);
    void seekByKeyboard(qint64 deltaMs);
    void showTransientFeedback(const QString &message);
    void cyclePlaybackMode();
    QString assetPath(const QString &relativePath) const;
    void loadLibrary();
    void saveLibrary();
    void restorePlaybackSnapshot();
    void savePlaybackSnapshot();
    void applyRestoredPositionBeforePlayback();
    int findTrackIndexByPath(const QString &filePath) const;
    void refreshPlaylist();
    QListWidgetItem *currentPlaylistItem() const;
    void ensureCurrentTrackVisibleIfNeeded();
    void refreshCollectionCombo();
    void createUserPlaylist();
    void addSelectedTrackToPlaylist();
    void editSelectedTrackTags();
    void showPlaylistContextMenu(const QPoint &position);
    void showPlaylistManageMenu();
    void renameActivePlaylist();
    void deleteActivePlaylist();
    void removeTrackFromActivePlaylist(int trackIndex);
    void showTrackInFolder(int trackIndex);
    int selectedOrCurrentTrackIndex() const;
    bool trackVisibleInCurrentCollection(const Track &track) const;
    QString collectionKey(const QString &type, const QString &value = QString()) const;
    QString currentCollectionLabel() const;
    QString emptyStateMessage(int visibleCount) const;
    void updateSearchHint(int visibleCount);
    void updateLibraryActionState();
    int nextIndexInCurrentView() const;
    int previousIndexInCurrentView() const;
    void scanFolder();
    void transcodeSelectedTrack();
    void playTrackAt(int index);
    void playCurrent();
    void playNext();
    void playPrevious();
    void updateCurrentTrackView();
    void updateCoverImage(const QImage &coverImage);
    void updateTimeLabel(qint64 positionMs, qint64 durationMs);
    void loadLyricsForCurrentTrack();
    void reloadLyricsForCurrentTrack();
    void chooseLyricsFileForCurrentTrack();
    void showLyricsContextMenu(const QPoint &globalPosition);
    void applyLyricsFileToCurrentTrack(const QString &filePath);
    void adjustLyricsOffset(int deltaMs);
    QString lrcPathForTrack(const Track &track) const;
    QString ffmpegPath() const;
    QVector<Track> scanAudioFiles(const QString &folderPath) const;
    bool readTrackMetadata(Track *track) const;
    void startWaveformDecode(const QString &filePath);
    void processWaveformBuffer(const QAudioBuffer &buffer);
    QVector<double> normalizedWaveform() const;
    QString formatTime(qint64 milliseconds) const;
    bool matchesSearch(const Track &track) const;

    QLineEdit *searchEdit = nullptr;
    QComboBox *collectionCombo = nullptr;
    QListWidget *playlistWidget = nullptr;
    QLabel *coverLabel = nullptr;
    QLabel *titleLabel = nullptr;
    QLabel *artistLabel = nullptr;
    QLabel *libraryHeaderLabel = nullptr;
    QLabel *searchHintLabel = nullptr;
    QLabel *emptyLibraryLabel = nullptr;
    LyricsWidget *lyricsWidget = nullptr;
    WaveformWidget *waveformWidget = nullptr;
    QLabel *timeLabel = nullptr;
    QLabel *feedbackLabel = nullptr;
    QLabel *statusLabel = nullptr;
    QSlider *positionSlider = nullptr;
    QSlider *volumeSlider = nullptr;
    QPushButton *playPauseButton = nullptr;
    QPushButton *stopButton = nullptr;
    QPushButton *previousButton = nullptr;
    QPushButton *nextButton = nullptr;
    QPushButton *scanButton = nullptr;
    QPushButton *transcodeButton = nullptr;
    QPushButton *playMvButton = nullptr;
    QPushButton *themeButton = nullptr;
    QPushButton *aboutButton = nullptr;
    QPushButton *helpButton = nullptr;
    QPushButton *newPlaylistButton = nullptr;
    QPushButton *addToPlaylistButton = nullptr;
    QPushButton *tagButton = nullptr;
    QPushButton *managePlaylistButton = nullptr;
    QPushButton *modeButton = nullptr;
    QPushButton *muteButton = nullptr;
    QPushButton *lyricsDelayMinusButton = nullptr;
    QPushButton *lyricsDelayPlusButton = nullptr;

    Playlist playlist;
    LibraryStore libraryStore;
    QVector<UserPlaylist> userPlaylists;
    QVector<int> visibleTrackIndexes;
    AudioPlayer audioPlayer;
    Transcoder transcoder;
    QAudioDecoder *waveformDecoder = nullptr;
    QVector<double> waveformBlocks;
    double waveformAccumulatedEnergy = 0.0;
    int waveformAccumulatedFrames = 0;
    int waveformBlockFrames = 2048;
    Lyrics currentLyrics;
    std::vector<std::unique_ptr<PlaybackStrategy>> playbackStrategies;
    int playbackStrategyIndex = 0;
    bool sliderPressed = false;
    bool keyboardSeekWasPlaying = false;
    qint64 currentDurationMs = 0;
    qint64 keyboardSeekTargetMs = -1;
    qint64 pendingRestorePositionMs = -1;
    qint64 lastSavedPlaybackPositionMs = -1;
    QString currentPlaybackPath;
    QString activeCollectionType = "all";
    QString activeCollectionValue;
    QImage currentCoverImage;
    bool nightMode = false;
    bool updatingCollectionCombo = false;
    bool handlingNearEndError = false;
};

#endif // MAINWINDOW_H
