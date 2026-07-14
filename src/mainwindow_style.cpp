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

void MainWindow::applyBaseStyle()
{
    themeButton->setText("");
    themeButton->setIcon(QIcon(assetPath(QString("img/ui-icons/%1/%2.256.png").arg(themeName(), nightMode ? "night" : "day"))));
    themeButton->setIconSize(QSize(22, 22));
    themeButton->setToolTip(nightMode ? "Switch to day mode" : "Switch to night mode");
    scanButton->setIcon(QIcon(assetPath(QString("img/ui-icons/%1/add-folder.256.png").arg(themeName()))));
    scanButton->setIconSize(QSize(18, 18));
    transcodeButton->setIcon(QIcon(assetPath(QString("img/ui-icons/%1/transcode.256.png").arg(themeName()))));
    transcodeButton->setIconSize(QSize(18, 18));
    playMvButton->setIcon(QIcon(assetPath(QString("img/ui-icons/%1/video-play.256.png").arg(themeName()))));
    playMvButton->setIconSize(QSize(18, 18));
    helpButton->setIcon(QIcon(assetPath(QString("img/ui-icons/%1/help.256.png").arg(themeName()))));
    helpButton->setIconSize(QSize(18, 18));
    aboutButton->setIcon(QIcon(assetPath(QString("img/ui-icons/%1/about.256.png").arg(themeName()))));
    aboutButton->setIconSize(QSize(18, 18));
    newPlaylistButton->setIcon(QIcon(assetPath(QString("img/ui-icons/%1/playlist.256.png").arg(themeName()))));
    newPlaylistButton->setIconSize(QSize(16, 16));
    addToPlaylistButton->setIcon(QIcon(assetPath(QString("img/ui-icons/%1/add-to-playlist.256.png").arg(themeName()))));
    addToPlaylistButton->setIconSize(QSize(16, 16));
    tagButton->setIcon(QIcon(assetPath(QString("img/ui-icons/%1/add-tag.256.png").arg(themeName()))));
    tagButton->setIconSize(QSize(16, 16));
    managePlaylistButton->setIcon(QIcon(assetPath(QString("img/ui-icons/%1/playlist-manage.256.png").arg(themeName()))));
    managePlaylistButton->setIconSize(QSize(16, 16));
    lyricsDelayMinusButton->setIcon(QIcon(assetPath(QString("img/ui-icons/%1/timer-minus.256.png").arg(themeName()))));
    lyricsDelayMinusButton->setIconSize(QSize(20, 20));
    lyricsDelayPlusButton->setIcon(QIcon(assetPath(QString("img/ui-icons/%1/timer-plus.256.png").arg(themeName()))));
    lyricsDelayPlusButton->setIconSize(QSize(20, 20));
    polishComboPopup(collectionCombo, nightMode ? "#1f2530" : "#ffffff");
    lyricsWidget->setNightMode(nightMode);

    if (!nightMode) {
        setStyleSheet(R"(
        QWidget {
            background: #eef3f8;
            color: #172033;
            font-family: "Segoe UI", "Microsoft YaHei UI", sans-serif;
            font-size: 14px;
        }
        QWidget#sidePanel, QWidget#contentPanel, QWidget#playbackPanel {
            background: #ffffff;
            border: 1px solid #d9e2ef;
            border-radius: 14px;
        }
        QLabel#sectionTitle {
            color: #23324d;
            font-size: 18px;
            font-weight: 700;
            background: transparent;
        }
        QLabel#emptyLibraryLabel {
            background: #f7fafc;
            border: 1px dashed #cbd8e8;
            border-radius: 12px;
            color: #6c7c94;
            padding: 18px;
        }
        QLabel#searchHintLabel {
            background: transparent;
            color: #7b8aa0;
            font-size: 12px;
        }
        QLineEdit {
            background: #f6f9fd;
            border: 1px solid #d8e1ee;
            border-radius: 10px;
            padding: 9px 12px;
            selection-background-color: #b9d4ff;
        }
        QLineEdit:focus {
            border-color: #6aa0f8;
            background: #ffffff;
        }
        QComboBox {
            background: #f6f9fd;
            border: 1px solid #d8e1ee;
            border-radius: 12px;
            padding: 8px 30px 8px 12px;
            color: #263850;
            combobox-popup: 1;
        }
        QComboBox:hover {
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
            color: #263850;
            selection-background-color: #edf5ff;
            selection-color: #12345d;
            outline: none;
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
        QMenu {
            background: #ffffff;
            border: 1px solid #d8e1ee;
            border-radius: 10px;
            padding: 6px;
            color: #263850;
        }
        QMenu::item {
            padding: 8px 28px 8px 12px;
            border-radius: 8px;
        }
        QMenu::item:selected {
            background: #edf5ff;
            color: #12345d;
        }
        QMenu::separator {
            height: 1px;
            background: #e3eaf3;
            margin: 5px 8px;
        }
        QListWidget {
            background: transparent;
            border: none;
            outline: none;
            padding: 2px;
        }
        LyricsWidget#lyricsWidget {
            background: #ffffff;
            border: 1px solid #d9e2ef;
            border-radius: 14px;
        }
        WaveformWidget#waveformWidget {
            background: transparent;
            border: none;
        }
        QListWidget::item {
            background: transparent;
            border-radius: 10px;
            border: 1px solid transparent;
            margin: 1px 0;
            padding: 8px 10px;
            color: #4d5f7a;
        }
        QListWidget::item:hover {
            background: #eef5ff;
        }
        QListWidget::item:selected {
            background: #edf5ff;
            color: #12345d;
            border: 1px solid #cfe0f5;
        }
        QWidget#trackItemWidget {
            background: transparent;
            border: 1px solid transparent;
            border-radius: 10px;
        }
        QWidget#trackItemWidget[active="true"] {
            background: #e8f2ff;
            border-color: #bfd7f6;
        }
        QLabel#trackTitleLabel {
            background: transparent;
            color: #20304a;
            font-size: 14px;
            font-weight: 700;
        }
        QLabel#trackSubtitleLabel {
            background: transparent;
            color: #6c7c94;
            font-size: 12px;
        }
        QLabel#trackDurationLabel {
            background: transparent;
            color: #7b8aa0;
            font-size: 12px;
            font-weight: 600;
        }
        QWidget#trackItemWidget[active="true"] QLabel#trackTitleLabel {
            color: #174ea6;
        }
        QWidget#trackItemWidget[active="true"] QLabel#trackSubtitleLabel,
        QWidget#trackItemWidget[active="true"] QLabel#trackDurationLabel {
            color: #42658f;
        }
        QScrollBar:vertical {
            background: transparent;
            border: none;
            border-radius: 6px;
            width: 12px;
            margin: 4px 0 4px 0;
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
            margin: 0 4px 0 4px;
        }
        QScrollBar::handle:horizontal {
            background: #c8d5e6;
            border: none;
            border-radius: 6px;
            min-width: 32px;
            margin: 2px 1px;
        }
        QScrollBar::handle:horizontal:hover {
            background: #9eb6d4;
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
        QPushButton {
            background: #f8fbff;
            border: 1px solid #d4dfec;
            border-radius: 10px;
            padding: 8px 13px;
            color: #22314a;
            font-weight: 600;
        }
        QPushButton:hover {
            background: #edf5ff;
            border-color: #8fb8f4;
        }
        QPushButton:pressed {
            background: #d8e9ff;
        }
        QPushButton:disabled {
            color: #9aa8bb;
            background: #eef2f6;
            border-color: #e0e7f0;
        }
        QPushButton#primaryActionButton {
            background: #1f6f78;
            border-color: #1f6f78;
            color: #ffffff;
            border-radius: 12px;
            padding: 9px 16px;
        }
        QPushButton#primaryActionButton:hover {
            background: #185f68;
            border-color: #185f68;
        }
        QPushButton#secondaryActionButton {
            background: #ffffff;
            border-color: #d7e0ec;
            color: #31415d;
            border-radius: 12px;
            padding: 9px 15px;
        }
        QPushButton#secondaryActionButton:hover {
            background: #f3f7fb;
            border-color: #9fb2ca;
        }
        QPushButton#compactActionButton {
            background: #f7fafc;
            border-color: #d7e0ec;
            color: #40516c;
            border-radius: 9px;
            padding: 7px 8px;
            font-size: 11px;
        }
        QPushButton#compactActionButton:hover {
            background: #eef5ff;
            border-color: #9fb8d6;
        }
        QPushButton#compactActionButton:disabled {
            color: #a5b1c2;
            background: #f2f5f8;
            border-color: #e0e7f0;
        }
        QPushButton#playbackIconButton {
            min-width: 38px;
            width: 38px;
            height: 38px;
            padding: 0;
            border-radius: 19px;
            background: transparent;
            border-color: transparent;
        }
        QPushButton#playbackIconButton:hover {
            background: #edf4fb;
            border-color: #d5e1ee;
        }
        QPushButton#playbackIconButton:pressed {
            background: #dce9f7;
        }
        QPushButton#primaryPlaybackButton {
            min-width: 50px;
            width: 50px;
            height: 44px;
            padding: 0;
            border-radius: 22px;
            background: #2563eb;
            border-color: #2563eb;
        }
        QPushButton#primaryPlaybackButton:hover {
            background: #1d4ed8;
            border-color: #1d4ed8;
        }
        QPushButton#primaryPlaybackButton:pressed {
            background: #1e40af;
        }
        QLabel#titleLabel {
            background: transparent;
            color: #101828;
            font-size: 30px;
            font-weight: 700;
        }
        QLabel#artistLabel {
            background: transparent;
            color: #61718a;
        }
        QLabel#hintLabel, QLabel#timeLabel {
            background: transparent;
            border: none;
            color: #61718a;
        }
        QLabel#feedbackLabel {
            background: transparent;
            border: none;
            color: #1f6f78;
            font-size: 12px;
            font-weight: 700;
        }
        QPushButton#themeToggleButton {
            min-width: 0;
            width: 42px;
            height: 42px;
            padding: 0;
            border-radius: 21px;
        }
        QLabel#coverLabel {
            background: #ffffff;
            border: 1px solid #d9e2ef;
            border-radius: 16px;
            color: #61718a;
        }
        QSlider::groove:horizontal {
            height: 7px;
            background: #dce5f1;
            border-radius: 4px;
        }
        QSlider {
            background: transparent;
            border: none;
        }
        QSlider::handle:horizontal {
            width: 16px;
            margin: -5px 0;
            border-radius: 8px;
            background: #3b82f6;
            border: 2px solid #ffffff;
        }
        QSlider::handle:horizontal:hover {
            background: #2563eb;
        }
        QSlider#volumeSlider::groove:horizontal {
            height: 5px;
            background: #e2e8f0;
            border-radius: 3px;
        }
        QSlider#volumeSlider::handle:horizontal {
            width: 14px;
            margin: -5px 0;
            border-radius: 7px;
            background: #1f6f78;
            border: 2px solid #ffffff;
        }
        QSplitter::handle {
            background: transparent;
        }
    )");
        return;
    }

    setStyleSheet(R"(
        QWidget {
            background: #10131a;
            color: #edf2f7;
            font-family: "Segoe UI", "Microsoft YaHei UI", sans-serif;
            font-size: 14px;
        }
        QWidget#sidePanel, QWidget#contentPanel, QWidget#playbackPanel {
            background: #171b24;
            border: 1px solid #252c39;
            border-radius: 14px;
        }
        QLabel#sectionTitle {
            color: #f4f7fb;
            font-size: 18px;
            font-weight: 700;
            background: transparent;
        }
        QLabel#emptyLibraryLabel {
            background: #1b212c;
            border: 1px dashed #344052;
            border-radius: 12px;
            color: #9daabd;
            padding: 18px;
        }
        QLabel#searchHintLabel {
            background: transparent;
            color: #8995a8;
            font-size: 12px;
        }
        QLineEdit, QListWidget {
            background: #1f2530;
            border: 1px solid #303847;
            border-radius: 10px;
            padding: 9px 12px;
            selection-background-color: #385f9f;
        }
        QLineEdit:focus {
            border-color: #7da8f8;
            background: #202838;
        }
        QComboBox {
            background: #1f2530;
            border: 1px solid #303847;
            border-radius: 12px;
            padding: 8px 30px 8px 12px;
            color: #e8edf5;
            combobox-popup: 1;
        }
        QComboBox:hover {
            background: #242d3b;
            border-color: #52647d;
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
            color: #e8edf5;
            selection-background-color: #263957;
            selection-color: #ffffff;
            outline: none;
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
        QMenu {
            background: #1f2530;
            border: 1px solid #303847;
            border-radius: 10px;
            padding: 6px;
            color: #e8edf5;
        }
        QMenu::item {
            padding: 8px 28px 8px 12px;
            border-radius: 8px;
        }
        QMenu::item:selected {
            background: #263957;
            color: #ffffff;
        }
        QMenu::separator {
            height: 1px;
            background: #303847;
            margin: 5px 8px;
        }
        QListWidget {
            background: transparent;
            border: none;
            outline: none;
            padding: 2px;
        }
        LyricsWidget#lyricsWidget {
            background: #141821;
            border: 1px solid #252c39;
            border-radius: 14px;
        }
        WaveformWidget#waveformWidget {
            background: transparent;
            border: none;
        }
        QListWidget::item {
            background: transparent;
            border-radius: 10px;
            border: 1px solid transparent;
            margin: 1px 0;
            padding: 8px 10px;
            color: #d6deea;
        }
        QListWidget::item:hover {
            background: #222a37;
        }
        QListWidget::item:selected {
            background: #202b3a;
            color: #ffffff;
            border: 1px solid #344259;
        }
        QWidget#trackItemWidget {
            background: transparent;
            border: 1px solid transparent;
            border-radius: 10px;
        }
        QWidget#trackItemWidget[active="true"] {
            background: #23344f;
            border-color: #3d5f94;
        }
        QLabel#trackTitleLabel {
            background: transparent;
            color: #eef4ff;
            font-size: 14px;
            font-weight: 700;
        }
        QLabel#trackSubtitleLabel {
            background: transparent;
            color: #9daabd;
            font-size: 12px;
        }
        QLabel#trackDurationLabel {
            background: transparent;
            color: #9aa7ba;
            font-size: 12px;
            font-weight: 600;
        }
        QWidget#trackItemWidget[active="true"] QLabel#trackTitleLabel {
            color: #ffffff;
        }
        QWidget#trackItemWidget[active="true"] QLabel#trackSubtitleLabel,
        QWidget#trackItemWidget[active="true"] QLabel#trackDurationLabel {
            color: #b8caff;
        }
        QScrollBar:vertical {
            background: transparent;
            border: none;
            border-radius: 6px;
            width: 12px;
            margin: 4px 0 4px 0;
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
            margin: 0 4px 0 4px;
        }
        QScrollBar::handle:horizontal {
            background: #3a4658;
            border: none;
            border-radius: 6px;
            min-width: 32px;
            margin: 2px 1px;
        }
        QScrollBar::handle:horizontal:hover {
            background: #53657c;
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
        QPushButton {
            background: #202631;
            border: 1px solid #323b4c;
            border-radius: 10px;
            padding: 8px 13px;
            color: #e8edf5;
            font-weight: 600;
        }
        QPushButton:hover {
            background: #2a3342;
            border-color: #61799f;
        }
        QPushButton:pressed {
            background: #314466;
        }
        QPushButton:disabled {
            color: #697386;
            background: #1a1f29;
            border-color: #262d39;
        }
        QPushButton#primaryActionButton {
            background: #4f8f96;
            border-color: #4f8f96;
            color: #ffffff;
            border-radius: 12px;
            padding: 9px 16px;
        }
        QPushButton#primaryActionButton:hover {
            background: #64a4aa;
            border-color: #64a4aa;
        }
        QPushButton#secondaryActionButton {
            background: #1b212c;
            border-color: #303847;
            color: #e8edf5;
            border-radius: 12px;
            padding: 9px 15px;
        }
        QPushButton#secondaryActionButton:hover {
            background: #242d3b;
            border-color: #52647d;
        }
        QPushButton#compactActionButton {
            background: #1b212c;
            border-color: #303847;
            color: #dce5f2;
            border-radius: 9px;
            padding: 7px 8px;
            font-size: 11px;
        }
        QPushButton#compactActionButton:hover {
            background: #242d3b;
            border-color: #52647d;
        }
        QPushButton#compactActionButton:disabled {
            color: #687385;
            background: #1a1f29;
            border-color: #262d39;
        }
        QPushButton#playbackIconButton {
            min-width: 38px;
            width: 38px;
            height: 38px;
            padding: 0;
            border-radius: 19px;
            background: transparent;
            border-color: transparent;
        }
        QPushButton#playbackIconButton:hover {
            background: #242d3b;
            border-color: #303847;
        }
        QPushButton#playbackIconButton:pressed {
            background: #2f3b4e;
        }
        QPushButton#primaryPlaybackButton {
            min-width: 50px;
            width: 50px;
            height: 44px;
            padding: 0;
            border-radius: 22px;
            background: #8fb3ff;
            border-color: #8fb3ff;
        }
        QPushButton#primaryPlaybackButton:hover {
            background: #a9c4ff;
            border-color: #a9c4ff;
        }
        QPushButton#primaryPlaybackButton:pressed {
            background: #6f9df3;
        }
        QLabel#titleLabel {
            background: transparent;
            color: #ffffff;
            font-size: 30px;
            font-weight: 700;
        }
        QLabel#artistLabel {
            background: transparent;
            color: #aeb8c8;
        }
        QLabel#hintLabel, QLabel#timeLabel {
            background: transparent;
            border: none;
            color: #aeb8c8;
        }
        QLabel#feedbackLabel {
            background: transparent;
            border: none;
            color: #8fb3ff;
            font-size: 12px;
            font-weight: 700;
        }
        QPushButton#themeToggleButton {
            min-width: 0;
            width: 42px;
            height: 42px;
            padding: 0;
            border-radius: 21px;
        }
        QLabel#coverLabel {
            background: #1f2530;
            border: 1px solid #303847;
            border-radius: 16px;
            color: #aeb8c8;
        }
        QSlider::groove:horizontal {
            height: 7px;
            background: #303847;
            border-radius: 4px;
        }
        QSlider {
            background: transparent;
            border: none;
        }
        QSlider::handle:horizontal {
            width: 16px;
            margin: -5px 0;
            border-radius: 8px;
            background: #8fb3ff;
            border: 2px solid #171b24;
        }
        QSlider::handle:horizontal:hover {
            background: #a9c4ff;
        }
        QSlider#volumeSlider::groove:horizontal {
            height: 5px;
            background: #293241;
            border-radius: 3px;
        }
        QSlider#volumeSlider::handle:horizontal {
            width: 14px;
            margin: -5px 0;
            border-radius: 7px;
            background: #4f8f96;
            border: 2px solid #171b24;
        }
        QSplitter::handle {
            background: transparent;
        }
    )");
}
