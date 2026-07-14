#include "mainwindow.h"

#include "lrcparser.h"
#include "seekslider.h"
#include "lyricswidget.h"
#include "transcodedialog.h"
#include "waveformwidget.h"

#include <algorithm>
#include <QAbstractItemView>
#include <QAudioBuffer>
#include <QAudioDecoder>
#include <QAudioFormat>
#include <QDirIterator>
#include <QApplication>
#include <QCloseEvent>
#include <QComboBox>
#include <QCoreApplication>
#include <QDesktopServices>
#include <QDialog>
#include <QFileDialog>
#include <QFileInfo>
#include <QFrame>
#include <QIcon>
#include <QImage>
#include <QEventLoop>
#include <QInputDialog>
#include <QKeyEvent>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QMenu>
#include <QMediaMetaData>
#include <QMediaPlayer>
#include <QMessageBox>
#include <QPixmap>
#include <QPushButton>
#include <QSettings>
#include <QSizePolicy>
#include <QSlider>
#include <QSplitter>
#include <QSet>
#include <QStyle>
#include <QToolButton>
#include <QTimer>
#include <QUrl>
#include <QVBoxLayout>
#include <QScrollBar>
#include <QHBoxLayout>
#include <QtMath>

void MainWindow::loadLibrary()
{
    const QVector<Track> loadedTracks = libraryStore.load();
    userPlaylists = libraryStore.loadUserPlaylists();
    QVector<Track> accessibleTracks;
    accessibleTracks.reserve(loadedTracks.size());
    int removedCount = 0;
    bool metadataChanged = false;

    for (Track track : loadedTracks) {
        const QFileInfo info(track.filePath);
        if (info.exists() && info.isReadable()) {
            const bool likelyFilenameTitle = track.title.trimmed().isEmpty()
                || (track.title.trimmed() == info.completeBaseName()
                    && track.artist.trimmed().isEmpty()
                    && track.album.trimmed().isEmpty());
            if (likelyFilenameTitle && readTrackMetadata(&track)) {
                metadataChanged = true;
            }
            accessibleTracks.push_back(track);
        } else {
            ++removedCount;
        }
    }

    playlist.setTracks(accessibleTracks);
    for (UserPlaylist &userPlaylist : userPlaylists) {
        QStringList existingPaths;
        for (const QString &path : userPlaylist.filePaths) {
            if (findTrackIndexByPath(path) >= 0 && !existingPaths.contains(path)) {
                existingPaths.append(path);
            }
        }
        userPlaylist.filePaths = existingPaths;
    }
    if (removedCount > 0 || metadataChanged) {
        libraryStore.save(playlist.tracks());
        libraryStore.saveUserPlaylists(userPlaylists);
    }
    if (removedCount > 0) {
        QMessageBox::warning(
            this,
            "Library warning",
            QString("%1 file(s) in the library cannot be accessed. The library will be refreshed and those entries will be removed.")
                .arg(removedCount));
    }
    refreshCollectionCombo();
    refreshPlaylist();
    restorePlaybackSnapshot();
    statusLabel->setText(QString("Loaded %1 songs").arg(playlist.count()));
}
void MainWindow::saveLibrary()
{
    libraryStore.save(playlist.tracks());
}
void MainWindow::restorePlaybackSnapshot()
{
    const PlaybackSnapshot snapshot = libraryStore.loadPlaybackSnapshot();
    const int index = findTrackIndexByPath(snapshot.filePath);
    if (index < 0) {
        updateCurrentTrackView();
        return;
    }

    playlist.setCurrentIndex(index);
    const Track *track = playlist.currentTrack();
    if (track == nullptr) {
        updateCurrentTrackView();
        return;
    }

    currentPlaybackPath = QFileInfo(track->filePath).absoluteFilePath();
    pendingRestorePositionMs = qMax<qint64>(0, snapshot.positionMs);
    lastSavedPlaybackPositionMs = pendingRestorePositionMs;
    audioPlayer.setSource(track->filePath);
    startWaveformDecode(track->filePath);
    refreshPlaylist();
    updateCurrentTrackView();
    updateCoverImage(QImage());
    loadLyricsForCurrentTrack();
    updatePlayPauseButton(false);
}
void MainWindow::savePlaybackSnapshot()
{
    const Track *track = playlist.currentTrack();
    if (track == nullptr) {
        return;
    }

    PlaybackSnapshot snapshot;
    snapshot.filePath = QFileInfo(track->filePath).absoluteFilePath();
    snapshot.positionMs = pendingRestorePositionMs >= 0 ? pendingRestorePositionMs : audioPlayer.position();
    libraryStore.savePlaybackSnapshot(snapshot);
    lastSavedPlaybackPositionMs = snapshot.positionMs;
}
void MainWindow::applyRestoredPositionBeforePlayback()
{
    if (pendingRestorePositionMs < 0) {
        return;
    }

    const qint64 upper = currentDurationMs > 0 ? currentDurationMs : audioPlayer.duration();
    const qint64 restoredPosition = upper > 0
        ? qMin(pendingRestorePositionMs, qMax<qint64>(0, upper - 1000))
        : pendingRestorePositionMs;
    audioPlayer.seek(restoredPosition);
    positionSlider->setValue(static_cast<int>(restoredPosition));
    waveformWidget->setPosition(restoredPosition);
    lyricsWidget->setPosition(restoredPosition);
    updateTimeLabel(restoredPosition, currentDurationMs);
    lastSavedPlaybackPositionMs = restoredPosition;
    pendingRestorePositionMs = -1;
}
int MainWindow::findTrackIndexByPath(const QString &filePath) const
{
    const QString absolutePath = QFileInfo(filePath).absoluteFilePath();
    for (int i = 0; i < playlist.count(); ++i) {
        const Track *track = playlist.trackAt(i);
        if (track != nullptr && QFileInfo(track->filePath).absoluteFilePath() == absolutePath) {
            return i;
        }
    }
    return -1;
}
void MainWindow::refreshPlaylist()
{
    const int previousScrollValue = playlistWidget->verticalScrollBar() != nullptr
        ? playlistWidget->verticalScrollBar()->value()
        : 0;
    playlistWidget->setUpdatesEnabled(false);
    playlistWidget->clear();
    visibleTrackIndexes.clear();
    int visibleCount = 0;
    for (int i = 0; i < playlist.count(); ++i) {
        const Track *track = playlist.trackAt(i);
        if (!trackVisibleInCurrentCollection(*track) || !matchesSearch(*track)) {
            continue;
        }
        ++visibleCount;
        visibleTrackIndexes.push_back(i);
        QString itemText = track->displayTitle();
        const QString sub = track->subtitle();
        if (!sub.isEmpty()) {
            itemText += "\n" + sub;
        }
        auto *item = new QListWidgetItem(playlistWidget);
        item->setData(Qt::UserRole, i);
        item->setData(Qt::AccessibleTextRole, itemText);
        item->setToolTip(track->filePath);
        item->setSizeHint(QSize(0, 66));
        if (i == playlist.currentIndex()) {
            item->setSelected(true);
        }

        auto *itemWidget = new QWidget(playlistWidget);
        itemWidget->setObjectName("trackItemWidget");
        itemWidget->setProperty("active", i == playlist.currentIndex());
        itemWidget->setAttribute(Qt::WA_StyledBackground, true);
        itemWidget->setAttribute(Qt::WA_TransparentForMouseEvents, true);

        auto *itemLayout = new QHBoxLayout(itemWidget);
        itemLayout->setContentsMargins(12, 7, 10, 7);
        itemLayout->setSpacing(10);

        auto *textLayout = new QVBoxLayout;
        textLayout->setContentsMargins(0, 0, 0, 0);
        textLayout->setSpacing(3);

        auto *title = new QLabel(track->displayTitle(), itemWidget);
        title->setObjectName("trackTitleLabel");
        title->setWordWrap(false);
        title->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Preferred);
        title->setAttribute(Qt::WA_TransparentForMouseEvents, true);

        auto *subtitle = new QLabel(sub.isEmpty() ? QFileInfo(track->filePath).absolutePath() : sub, itemWidget);
        subtitle->setObjectName("trackSubtitleLabel");
        subtitle->setWordWrap(false);
        subtitle->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Preferred);
        subtitle->setAttribute(Qt::WA_TransparentForMouseEvents, true);

        textLayout->addWidget(title);
        textLayout->addWidget(subtitle);

        auto *duration = new QLabel(track->durationMs > 0 ? formatTime(track->durationMs) : QString("--:--"), itemWidget);
        duration->setObjectName("trackDurationLabel");
        duration->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
        duration->setMinimumWidth(48);
        duration->setAttribute(Qt::WA_TransparentForMouseEvents, true);

        itemLayout->addLayout(textLayout, 1);
        itemLayout->addWidget(duration);
        playlistWidget->setItemWidget(item, itemWidget);
    }
    if (libraryHeaderLabel != nullptr) {
        libraryHeaderLabel->setText(QString("%1  %2/%3")
            .arg(currentCollectionLabel())
            .arg(visibleCount)
            .arg(playlist.count()));
    }
    updateSearchHint(visibleCount);
    if (emptyLibraryLabel != nullptr) {
        emptyLibraryLabel->setText(emptyStateMessage(visibleCount));
        const bool isEmpty = visibleCount == 0;
        emptyLibraryLabel->setVisible(isEmpty);
        playlistWidget->setVisible(!isEmpty);
    }
    if (playlistWidget->isVisible() && playlistWidget->verticalScrollBar() != nullptr) {
        playlistWidget->verticalScrollBar()->setValue(previousScrollValue);
    }
    playlistWidget->setUpdatesEnabled(true);
    if (playlistWidget->isVisible()) {
        ensureCurrentTrackVisibleIfNeeded();
    }
    updateLibraryActionState();
}
QListWidgetItem *MainWindow::currentPlaylistItem() const
{
    const int currentIndex = playlist.currentIndex();
    if (currentIndex < 0) {
        return nullptr;
    }

    for (int row = 0; row < playlistWidget->count(); ++row) {
        QListWidgetItem *item = playlistWidget->item(row);
        if (item != nullptr && item->data(Qt::UserRole).toInt() == currentIndex) {
            return item;
        }
    }

    return nullptr;
}
void MainWindow::ensureCurrentTrackVisibleIfNeeded()
{
    QListWidgetItem *item = currentPlaylistItem();
    if (item == nullptr || playlistWidget->viewport() == nullptr) {
        return;
    }

    const QRect itemRect = playlistWidget->visualItemRect(item);
    const QRect viewportRect = playlistWidget->viewport()->rect();
    if (itemRect.isValid()
        && (!viewportRect.contains(itemRect.topLeft()) || !viewportRect.contains(itemRect.bottomLeft()))) {
        playlistWidget->scrollToItem(item, QAbstractItemView::PositionAtCenter);
    }
}
QString MainWindow::collectionKey(const QString &type, const QString &value) const
{
    return value.isEmpty() ? type : QString("%1:%2").arg(type, value);
}
QString MainWindow::currentCollectionLabel() const
{
    if (activeCollectionType == "playlist") {
        return QString("Playlist: %1").arg(activeCollectionValue);
    }
    if (activeCollectionType == "artist") {
        return QString("Artist: %1").arg(activeCollectionValue);
    }
    if (activeCollectionType == "album") {
        return QString("Album: %1").arg(activeCollectionValue);
    }
    if (activeCollectionType == "tag") {
        return QString("Tag: %1").arg(activeCollectionValue);
    }
    return "Library";
}
QString MainWindow::emptyStateMessage(int visibleCount) const
{
    if (visibleCount > 0) {
        return QString();
    }
    if (playlist.isEmpty()) {
        return "No songs yet.\nUse Add Folder to scan your music library.";
    }
    if (!searchEdit->text().trimmed().isEmpty()) {
        return QString("No songs match \"%1\" in %2.")
            .arg(searchEdit->text().trimmed(), currentCollectionLabel());
    }
    if (activeCollectionType == "playlist") {
        return "This playlist is empty.\nSelect a song from All Songs, then use Add to.";
    }
    if (activeCollectionType == "tag") {
        return "No songs have this tag yet.\nSelect a song and use Tags to organize it.";
    }
    return QString("No songs in %1.").arg(currentCollectionLabel());
}
void MainWindow::updateSearchHint(int visibleCount)
{
    if (searchHintLabel == nullptr) {
        return;
    }

    const QString keyword = searchEdit->text().trimmed();
    if (keyword.isEmpty()) {
        searchHintLabel->setText(QString("Showing %1").arg(currentCollectionLabel()));
        return;
    }

    searchHintLabel->setText(QString("Searching \"%1\" in %2 - %3 result(s)")
        .arg(keyword)
        .arg(currentCollectionLabel())
        .arg(visibleCount));
}
void MainWindow::updateLibraryActionState()
{
    const bool hasTrack = selectedOrCurrentTrackIndex() >= 0;
    if (addToPlaylistButton != nullptr) {
        addToPlaylistButton->setEnabled(hasTrack);
        addToPlaylistButton->setToolTip(hasTrack ? "Add selected song to a playlist" : "Select a song first");
    }
    if (tagButton != nullptr) {
        tagButton->setEnabled(hasTrack);
        tagButton->setToolTip(hasTrack ? "Edit custom tags for the selected song" : "Select a song first");
    }
    if (managePlaylistButton != nullptr) {
        const bool canManagePlaylist = activeCollectionType == "playlist";
        managePlaylistButton->setEnabled(canManagePlaylist);
        managePlaylistButton->setToolTip(canManagePlaylist
            ? "Rename or delete the current playlist"
            : "Choose a playlist to manage it");
    }
}
void MainWindow::refreshCollectionCombo()
{
    if (collectionCombo == nullptr) {
        return;
    }

    const QString activeKey = collectionKey(activeCollectionType, activeCollectionValue);
    updatingCollectionCombo = true;
    collectionCombo->clear();
    collectionCombo->addItem("All Songs", collectionKey("all"));

    for (const UserPlaylist &userPlaylist : userPlaylists) {
        collectionCombo->addItem(QString("Playlists / %1").arg(userPlaylist.name), collectionKey("playlist", userPlaylist.name));
    }

    QSet<QString> artists;
    QSet<QString> albums;
    QSet<QString> tags;
    for (const Track &track : playlist.tracks()) {
        if (!track.artist.trimmed().isEmpty()) {
            artists.insert(track.artist.trimmed());
        }
        if (!track.album.trimmed().isEmpty()) {
            albums.insert(track.album.trimmed());
        }
        for (const QString &tag : track.tags) {
            if (!tag.trimmed().isEmpty()) {
                tags.insert(tag.trimmed());
            }
        }
    }

    QStringList sortedArtists = artists.values();
    QStringList sortedAlbums = albums.values();
    QStringList sortedTags = tags.values();
    sortedArtists.sort(Qt::CaseInsensitive);
    sortedAlbums.sort(Qt::CaseInsensitive);
    sortedTags.sort(Qt::CaseInsensitive);

    for (const QString &artist : sortedArtists) {
        collectionCombo->addItem(QString("Artists / %1").arg(artist), collectionKey("artist", artist));
    }
    for (const QString &album : sortedAlbums) {
        collectionCombo->addItem(QString("Albums / %1").arg(album), collectionKey("album", album));
    }
    for (const QString &tag : sortedTags) {
        collectionCombo->addItem(QString("Tags / %1").arg(tag), collectionKey("tag", tag));
    }

    int activeIndex = collectionCombo->findData(activeKey);
    if (activeIndex < 0) {
        activeCollectionType = "all";
        activeCollectionValue.clear();
        activeIndex = 0;
    }
    collectionCombo->setCurrentIndex(activeIndex);
    updatingCollectionCombo = false;
    searchEdit->setPlaceholderText(QString("Search in %1").arg(currentCollectionLabel()));
}
bool MainWindow::trackVisibleInCurrentCollection(const Track &track) const
{
    if (activeCollectionType == "all") {
        return true;
    }
    if (activeCollectionType == "artist") {
        return track.artist.compare(activeCollectionValue, Qt::CaseInsensitive) == 0;
    }
    if (activeCollectionType == "album") {
        return track.album.compare(activeCollectionValue, Qt::CaseInsensitive) == 0;
    }
    if (activeCollectionType == "tag") {
        return track.tags.contains(activeCollectionValue, Qt::CaseInsensitive);
    }
    if (activeCollectionType == "playlist") {
        const QString absolutePath = QFileInfo(track.filePath).absoluteFilePath();
        for (const UserPlaylist &userPlaylist : userPlaylists) {
            if (userPlaylist.name.compare(activeCollectionValue, Qt::CaseInsensitive) == 0) {
                return userPlaylist.filePaths.contains(absolutePath);
            }
        }
        return false;
    }
    return true;
}
int MainWindow::selectedOrCurrentTrackIndex() const
{
    if (playlistWidget != nullptr && !playlistWidget->selectedItems().isEmpty()) {
        return playlistWidget->selectedItems().first()->data(Qt::UserRole).toInt();
    }
    const int currentIndex = playlist.currentIndex();
    return visibleTrackIndexes.contains(currentIndex) ? currentIndex : -1;
}
void MainWindow::createUserPlaylist()
{
    bool ok = false;
    const QString name = QInputDialog::getText(this, "New playlist", "Playlist name:", QLineEdit::Normal, QString(), &ok).trimmed();
    if (!ok || name.isEmpty()) {
        return;
    }
    for (const UserPlaylist &userPlaylist : userPlaylists) {
        if (userPlaylist.name.compare(name, Qt::CaseInsensitive) == 0) {
            QMessageBox::information(this, "Playlist", "A playlist with this name already exists. Choose a different name.");
            return;
        }
    }

    UserPlaylist userPlaylist;
    userPlaylist.name = name;
    const int trackIndex = selectedOrCurrentTrackIndex();
    const Track *track = playlist.trackAt(trackIndex);
    if (track != nullptr) {
        const QMessageBox::StandardButton choice = QMessageBox::question(
            this,
            "New playlist",
            QString("Add \"%1\" to this playlist now?").arg(track->displayTitle()),
            QMessageBox::Yes | QMessageBox::No,
            QMessageBox::Yes);
        if (choice == QMessageBox::Yes) {
            userPlaylist.filePaths.append(QFileInfo(track->filePath).absoluteFilePath());
        }
    }
    userPlaylists.push_back(userPlaylist);
    libraryStore.saveUserPlaylists(userPlaylists);
    activeCollectionType = "playlist";
    activeCollectionValue = name;
    refreshCollectionCombo();
    refreshPlaylist();
    showTransientFeedback(QString("Created playlist \"%1\"").arg(name));
}
void MainWindow::addSelectedTrackToPlaylist()
{
    const int trackIndex = selectedOrCurrentTrackIndex();
    const Track *track = playlist.trackAt(trackIndex);
    if (track == nullptr) {
        QMessageBox::information(this, "Playlist", "Select a song before adding it to a playlist.");
        return;
    }
    if (userPlaylists.isEmpty()) {
        createUserPlaylist();
        return;
    }

    QStringList names;
    for (const UserPlaylist &userPlaylist : userPlaylists) {
        names << userPlaylist.name;
    }

    bool ok = false;
    const QString name = QInputDialog::getItem(this, "Add to playlist", "Playlist:", names, 0, false, &ok);
    if (!ok || name.isEmpty()) {
        return;
    }

    const QString absolutePath = QFileInfo(track->filePath).absoluteFilePath();
    for (UserPlaylist &userPlaylist : userPlaylists) {
        if (userPlaylist.name == name) {
            if (!userPlaylist.filePaths.contains(absolutePath)) {
                userPlaylist.filePaths.append(absolutePath);
                libraryStore.saveUserPlaylists(userPlaylists);
            }
            activeCollectionType = "playlist";
            activeCollectionValue = name;
            refreshCollectionCombo();
            refreshPlaylist();
            showTransientFeedback(QString("Added to \"%1\"").arg(name));
            return;
        }
    }
}
void MainWindow::showPlaylistContextMenu(const QPoint &position)
{
    QListWidgetItem *item = playlistWidget->itemAt(position);
    if (item == nullptr) {
        return;
    }

    playlistWidget->setCurrentItem(item);
    item->setSelected(true);
    const int trackIndex = item->data(Qt::UserRole).toInt();
    const Track *track = playlist.trackAt(trackIndex);
    if (track == nullptr) {
        return;
    }

    QMenu menu(this);
    QAction *playAction = menu.addAction("Play");
    playAction->setIcon(QIcon(assetPath(QString("img/ui-icons/%1/play.256.png").arg(themeName()))));
    QAction *addAction = menu.addAction("Add to Playlist");
    addAction->setIcon(QIcon(assetPath(QString("img/ui-icons/%1/add-to-playlist.256.png").arg(themeName()))));
    QAction *tagAction = menu.addAction("Edit Tags");
    tagAction->setIcon(QIcon(assetPath(QString("img/ui-icons/%1/add-tag.256.png").arg(themeName()))));
    QAction *showAction = menu.addAction("Show in Folder");
    showAction->setIcon(QIcon(assetPath(QString("img/ui-icons/%1/folder-locate.256.png").arg(themeName()))));
    QAction *removeAction = nullptr;
    if (activeCollectionType == "playlist") {
        menu.addSeparator();
        removeAction = menu.addAction("Remove from This Playlist");
        removeAction->setIcon(QIcon(assetPath(QString("img/ui-icons/%1/playlist-delete.256.png").arg(themeName()))));
    }

    QAction *chosen = menu.exec(playlistWidget->viewport()->mapToGlobal(position));
    if (chosen == nullptr) {
        return;
    }
    if (chosen == playAction) {
        playTrackAt(trackIndex);
    } else if (chosen == addAction) {
        addSelectedTrackToPlaylist();
    } else if (chosen == tagAction) {
        editSelectedTrackTags();
    } else if (chosen == showAction) {
        showTrackInFolder(trackIndex);
    } else if (chosen == removeAction) {
        removeTrackFromActivePlaylist(trackIndex);
    }
}
void MainWindow::showPlaylistManageMenu()
{
    if (activeCollectionType != "playlist") {
        return;
    }

    QMenu menu(this);
    QAction *renameAction = menu.addAction("Rename Playlist");
    renameAction->setIcon(QIcon(assetPath(QString("img/ui-icons/%1/playlist-rename.256.png").arg(themeName()))));
    QAction *deleteAction = menu.addAction("Delete Playlist");
    deleteAction->setIcon(QIcon(assetPath(QString("img/ui-icons/%1/playlist-delete.256.png").arg(themeName()))));
    QAction *chosen = menu.exec(managePlaylistButton->mapToGlobal(QPoint(0, managePlaylistButton->height())));
    if (chosen == renameAction) {
        renameActivePlaylist();
    } else if (chosen == deleteAction) {
        deleteActivePlaylist();
    }
}
void MainWindow::renameActivePlaylist()
{
    if (activeCollectionType != "playlist") {
        return;
    }

    bool ok = false;
    const QString newName = QInputDialog::getText(
        this,
        "Rename playlist",
        "Playlist name:",
        QLineEdit::Normal,
        activeCollectionValue,
        &ok).trimmed();
    if (!ok || newName.isEmpty() || newName == activeCollectionValue) {
        return;
    }

    for (const UserPlaylist &userPlaylist : userPlaylists) {
        if (userPlaylist.name.compare(newName, Qt::CaseInsensitive) == 0) {
            QMessageBox::information(this, "Playlist", "A playlist with this name already exists. Choose a different name.");
            return;
        }
    }

    for (UserPlaylist &userPlaylist : userPlaylists) {
        if (userPlaylist.name.compare(activeCollectionValue, Qt::CaseInsensitive) == 0) {
            userPlaylist.name = newName;
            activeCollectionValue = newName;
            libraryStore.saveUserPlaylists(userPlaylists);
            refreshCollectionCombo();
            refreshPlaylist();
            showTransientFeedback(QString("Renamed to \"%1\"").arg(newName));
            return;
        }
    }
}
void MainWindow::deleteActivePlaylist()
{
    if (activeCollectionType != "playlist") {
        return;
    }

    const QMessageBox::StandardButton choice = QMessageBox::warning(
        this,
        "Delete playlist",
        QString("Delete playlist \"%1\"?\nMusic files will not be deleted.").arg(activeCollectionValue),
        QMessageBox::Yes | QMessageBox::No,
        QMessageBox::No);
    if (choice != QMessageBox::Yes) {
        return;
    }

    for (int i = 0; i < userPlaylists.size(); ++i) {
        if (userPlaylists[i].name.compare(activeCollectionValue, Qt::CaseInsensitive) == 0) {
            userPlaylists.removeAt(i);
            activeCollectionType = "all";
            activeCollectionValue.clear();
            libraryStore.saveUserPlaylists(userPlaylists);
            refreshCollectionCombo();
            refreshPlaylist();
            showTransientFeedback("Playlist deleted");
            return;
        }
    }
}
void MainWindow::removeTrackFromActivePlaylist(int trackIndex)
{
    if (activeCollectionType != "playlist") {
        return;
    }
    const Track *track = playlist.trackAt(trackIndex);
    if (track == nullptr) {
        return;
    }

    const QString absolutePath = QFileInfo(track->filePath).absoluteFilePath();
    for (UserPlaylist &userPlaylist : userPlaylists) {
        if (userPlaylist.name.compare(activeCollectionValue, Qt::CaseInsensitive) != 0) {
            continue;
        }
        const int removed = userPlaylist.filePaths.removeAll(absolutePath);
        if (removed > 0) {
            libraryStore.saveUserPlaylists(userPlaylists);
            refreshCollectionCombo();
            refreshPlaylist();
            showTransientFeedback("Removed from playlist");
        }
        return;
    }
}
void MainWindow::showTrackInFolder(int trackIndex)
{
    const Track *track = playlist.trackAt(trackIndex);
    if (track == nullptr) {
        return;
    }

    const QFileInfo info(track->filePath);
    if (!info.exists()) {
        QMessageBox::warning(this, "Show in Folder", "This file cannot be accessed.");
        return;
    }

    if (QDesktopServices::openUrl(QUrl::fromLocalFile(info.absolutePath()))) {
        showTransientFeedback("Opened folder");
    }
}
void MainWindow::editSelectedTrackTags()
{
    const int trackIndex = selectedOrCurrentTrackIndex();
    Track *track = playlist.trackAt(trackIndex);
    if (track == nullptr) {
        QMessageBox::information(this, "Tags", "Select a song before editing tags.");
        return;
    }

    bool ok = false;
    const QString text = QInputDialog::getText(
        this,
        "Edit tags",
        "Tags, separated by commas:",
        QLineEdit::Normal,
        track->tags.join(", "),
        &ok);
    if (!ok) {
        return;
    }

    QStringList tags;
    for (const QString &part : text.split(",", Qt::SkipEmptyParts)) {
        const QString tag = part.trimmed();
        if (!tag.isEmpty() && !tags.contains(tag, Qt::CaseInsensitive)) {
            tags.append(tag);
        }
    }

    track->tags = tags;
    saveLibrary();
    refreshCollectionCombo();
    refreshPlaylist();
    showTransientFeedback("Tags updated");
}
void MainWindow::scanFolder()
{
    const QString folder = QFileDialog::getExistingDirectory(this, "Add music folder");
    if (folder.isEmpty()) {
        return;
    }

    statusLabel->setText("Scanning...");
    const QVector<Track> foundTracks = scanAudioFiles(folder);
    const int added = playlist.addTracks(foundTracks);
    saveLibrary();
    refreshCollectionCombo();
    refreshPlaylist();
    updateCurrentTrackView();
    statusLabel->setText(QString("Added %1 songs").arg(added));
    showTransientFeedback(QString("Added %1 song(s)").arg(added));
}
void MainWindow::transcodeSelectedTrack()
{
    int index = playlist.currentIndex();
    if (!playlistWidget->selectedItems().isEmpty()) {
        index = playlistWidget->selectedItems().first()->data(Qt::UserRole).toInt();
    }

    const QString program = ffmpegPath();
    if (program.isEmpty()) {
        QMessageBox::warning(this, "Transcode", "ffmpeg.exe was not found in the ffmpeg/bin folder.");
        return;
    }

    const Track *track = playlist.trackAt(index);
    const QString initialPath = track == nullptr ? QString() : track->filePath;
    const qint64 initialDuration = track == nullptr ? 0 : track->durationMs;
    TranscodeDialog dialog(program, initialPath, initialDuration, nightMode, this);
    dialog.exec();
}
QVector<Track> MainWindow::scanAudioFiles(const QString &folderPath) const
{
    static const QStringList patterns = {"*.mp3", "*.flac", "*.wav", "*.m4a", "*.aac", "*.ogg"};
    QVector<Track> tracks;
    QDirIterator iterator(folderPath, patterns, QDir::Files, QDirIterator::Subdirectories);
    while (iterator.hasNext()) {
        const QString path = QFileInfo(iterator.next()).absoluteFilePath();
        Track track;
        track.filePath = path;
        readTrackMetadata(&track);
        tracks.push_back(track);
    }
    return tracks;
}
bool MainWindow::readTrackMetadata(Track *track) const
{
    if (track == nullptr || track->filePath.trimmed().isEmpty()) {
        return false;
    }

    QMediaPlayer player;
    QEventLoop loop;
    QTimer timeout;
    timeout.setSingleShot(true);

    bool finished = false;
    auto quitOnce = [&]() {
        if (!finished) {
            finished = true;
            loop.quit();
        }
    };

    connect(&player, &QMediaPlayer::metaDataChanged, &loop, quitOnce);
    connect(&player, &QMediaPlayer::durationChanged, &loop, quitOnce);
    connect(&player, &QMediaPlayer::mediaStatusChanged, &loop, [&](QMediaPlayer::MediaStatus status) {
        if (status == QMediaPlayer::LoadedMedia || status == QMediaPlayer::InvalidMedia) {
            quitOnce();
        }
    });
    connect(&timeout, &QTimer::timeout, &loop, quitOnce);

    player.setSource(QUrl::fromLocalFile(track->filePath));
    timeout.start(1200);
    loop.exec();

    const QMediaMetaData metaData = player.metaData();
    const QString title = metaData.stringValue(QMediaMetaData::Title).trimmed();
    QString artist = metaData.stringValue(QMediaMetaData::Author).trimmed();
    if (artist.isEmpty()) {
        artist = metaData.stringValue(QMediaMetaData::ContributingArtist).trimmed();
    }
    const QString album = metaData.stringValue(QMediaMetaData::AlbumTitle).trimmed();
    const qint64 durationMs = player.duration();

    bool changed = false;
    if (!title.isEmpty() && track->title != title) {
        track->title = title;
        changed = true;
    }
    if (!artist.isEmpty() && track->artist != artist) {
        track->artist = artist;
        changed = true;
    }
    if (!album.isEmpty() && track->album != album) {
        track->album = album;
        changed = true;
    }
    if (durationMs > 0 && track->durationMs != durationMs) {
        track->durationMs = durationMs;
        changed = true;
    }

    return changed;
}
bool MainWindow::matchesSearch(const Track &track) const
{
    const QString keyword = searchEdit->text().trimmed();
    if (keyword.isEmpty()) {
        return true;
    }

    const bool tagMatched = std::any_of(track.tags.cbegin(), track.tags.cend(), [&](const QString &tag) {
        return tag.contains(keyword, Qt::CaseInsensitive);
    });

    return track.displayTitle().contains(keyword, Qt::CaseInsensitive)
        || track.artist.contains(keyword, Qt::CaseInsensitive)
        || track.album.contains(keyword, Qt::CaseInsensitive)
        || tagMatched
        || QFileInfo(track.filePath).fileName().contains(keyword, Qt::CaseInsensitive);
}
