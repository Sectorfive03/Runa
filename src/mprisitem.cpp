/*
 * SPDX-FileCopyrightText: 2026 George Florea Bănuș <georgefb899@gmail.com>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "mprisitem.h"

#include <QWindow>

#include <KAboutData>

#include "vlcitem.h"
#include "playlistfilterproxymodel.h"

#if HAVE_DBUS

#include <QApplication>
#include <QBuffer>
#include <QFileInfo>
#include <QImageReader>
#include <QPointer>
#include <QThreadPool>

#include "logging/_general.h"
#include "miscutils.h"
#include "mpriscontroller.h"
#include "playlistsettings.h"
#include "videosettings.h"

using namespace Qt::StringLiterals;
using EmbeddedImageDataMap = QMap<KFileMetaData::EmbeddedImageData::ImageType, QByteArray>;

class MprisItemPrivate
{
    Q_DISABLE_COPY(MprisItemPrivate);

public:
    MprisItemPrivate(MprisItem *q)
        : q_ptr(q)
        , mprisController(q)
    {
    }
    MprisItem *const q_ptr;
    VlcItem *player = nullptr;
    PlaylistFilterProxyModel *activePlaylist = nullptr;
    QWindow *window = nullptr;
    MprisController mprisController;

    QMetaObject::Connection playlistNextConnection;
    QMetaObject::Connection playlistPreviousConnection;
    QMetaObject::Connection playlistItemCountConnection;

    QByteArray metadataImage(EmbeddedImageDataMap imageData);
};

QByteArray MprisItemPrivate::metadataImage(EmbeddedImageDataMap imageData)
{
    using namespace KFileMetaData;

    QByteArray data;
    if (imageData.contains(EmbeddedImageData::MovieScreenCapture)) {
        data = imageData.value(EmbeddedImageData::MovieScreenCapture);

    } else if (imageData.contains(EmbeddedImageData::FrontCover)) {
        data = imageData.value(EmbeddedImageData::FrontCover);

    } else if (imageData.contains(EmbeddedImageData::BackCover)) {
        data = imageData.value(EmbeddedImageData::BackCover);

    } else if (imageData.contains(EmbeddedImageData::Other)) {
        data = imageData.value(EmbeddedImageData::Other);

    } else if (imageData.contains(EmbeddedImageData::Unknown)) {
        data = imageData.value(EmbeddedImageData::Unknown);

    } else {
        data = {};
    }

    return data;
}

MprisItem::MprisItem(QObject *parent)
    : QObject(parent)
    , d_ptr{std::make_unique<MprisItemPrivate>(this)}
{
    d_ptr->mprisController.setCanQuit(true);
    d_ptr->mprisController.setCanRaise(true);
    d_ptr->mprisController.setHasTrackList(false);
    d_ptr->mprisController.setIdentity(KAboutData::applicationData().displayName());
    d_ptr->mprisController.setDesktopEntry(QGuiApplication::desktopFileName());
    d_ptr->mprisController.setSupportedUriSchemes({u"file"_s, u"http"_s, u"https"_s});
    d_ptr->mprisController.setSupportedMimeTypes({u"video/*"_s, u"audio/*"_s});

    d_ptr->mprisController.setShuffle(PlaylistSettings::randomPlayback());
    d_ptr->mprisController.setMinimumRate(0.0);
    d_ptr->mprisController.setMaximumRate(10.0);
    d_ptr->mprisController.setCanControl(true);

    auto setLoopStatus = [this]() {
        QString loopStatus = u"None"_s;
        if (PlaylistSettings::playbackBehavior() == u"RepeatItem"_s) {
            loopStatus = u"Track"_s;
        }
        if (PlaylistSettings::playbackBehavior() == u"RepeatPlaylist"_s) {
            loopStatus = u"Playlist"_s;
        }
        d_ptr->mprisController.setLoopStatus(loopStatus);
    };

    setLoopStatus();

    connect(&d_ptr->mprisController, &MprisController::raiseRequested, this, &MprisItem::raise);
    connect(&d_ptr->mprisController, &MprisController::openUriRequested, this, &MprisItem::openUri);
    connect(PlaylistSettings::self(), &PlaylistSettings::PlaybackBehaviorChanged, this, setLoopStatus);
    connect(PlaylistSettings::self(), &PlaylistSettings::RandomPlaybackChanged, this, [this]() {
        d_ptr->mprisController.setShuffle(PlaylistSettings::randomPlayback());
    });
    connect(&d_ptr->mprisController, &MprisController::shuffleChangeRequested, this, [](bool shuffle) {
        PlaylistSettings::setRandomPlayback(shuffle);
        PlaylistSettings::self()->save();
    });

    connect(&d_ptr->mprisController, &MprisController::loopStatusChangeRequested, this, [](const QString &loopStatus) {
        if (loopStatus == u"None"_s) {
            PlaylistSettings::setPlaybackBehavior(u"StopAfterLast"_s);
        } else if (loopStatus == u"Track"_s) {
            PlaylistSettings::setPlaybackBehavior(u"RepeatItem"_s);
        } else if (loopStatus == u"Playlist"_s) {
            PlaylistSettings::setPlaybackBehavior(u"RepeatPlaylist"_s);
        }

        PlaylistSettings::self()->save();
    });

    connect(&d_ptr->mprisController, &MprisController::quitRequested, QApplication::instance(), &QApplication::quit);
}

MprisItem::~MprisItem() = default;

VlcItem *MprisItem::player() const
{
    return d_ptr->player;
}

void MprisItem::setPlayer(VlcItem *newPlayer)
{
    if (d_ptr->player != nullptr) {
        return;
    }
    d_ptr->player = newPlayer;
    Q_EMIT playerChanged();

    d_ptr->mprisController.setVolume(d_ptr->player->volume());

    setupPlayerConnections();
}

PlaylistFilterProxyModel *MprisItem::activePlaylist() const
{
    return d_ptr->activePlaylist;
}

void MprisItem::setActivePlaylist(PlaylistFilterProxyModel *newActivePlaylist)
{
    if (d_ptr->activePlaylist == newActivePlaylist) {
        return;
    }
    d_ptr->activePlaylist = newActivePlaylist;
    Q_EMIT activePlaylistChanged();

    const auto playlist = d_ptr->activePlaylist;
    d_ptr->mprisController.setCanGoNext(playlist->rowCount() > 1);
    d_ptr->mprisController.setCanGoPrevious(playlist->rowCount() > 1);

    disconnect(d_ptr->playlistNextConnection);
    disconnect(d_ptr->playlistPreviousConnection);
    disconnect(d_ptr->playlistItemCountConnection);

    d_ptr->playlistNextConnection = connect(&d_ptr->mprisController, &MprisController::nextRequested, playlist, &PlaylistFilterProxyModel::playNext);

    d_ptr->playlistPreviousConnection =
        connect(&d_ptr->mprisController, &MprisController::previousRequested, playlist, &PlaylistFilterProxyModel::playPrevious);

    d_ptr->playlistItemCountConnection = connect(playlist, &PlaylistFilterProxyModel::itemCountChanged, this, [this]() {
        const auto playlist = d_ptr->activePlaylist;
        d_ptr->mprisController.setCanGoNext(playlist->rowCount() > 1);
        d_ptr->mprisController.setCanGoPrevious(playlist->rowCount() > 1);
    });
}

QWindow *MprisItem::window() const
{
    return d_ptr->window;
}

void MprisItem::setWindow(QWindow *newWindow)
{
    if (d_ptr->window != nullptr) {
        return;
    }
    d_ptr->window = newWindow;
    Q_EMIT windowChanged();

    const auto isFullScreen = d_ptr->window->windowStates() & Qt::WindowFullScreen;
    d_ptr->mprisController.setCanSetFullscreen(!isFullScreen);

    connect(d_ptr->window, &QWindow::windowStateChanged, this, [this]() {
        const auto isFullScreen = d_ptr->window->windowStates() & Qt::WindowFullScreen;
        d_ptr->mprisController.setCanSetFullscreen(!isFullScreen);
    });
    connect(&d_ptr->mprisController, &MprisController::fullscreenChangeRequested, this, [this](bool fullscreen) {
        if (fullscreen) {
            d_ptr->window->showFullScreen();
        } else {
            d_ptr->window->showNormal();
        }
        const auto isFullScreen = d_ptr->window->windowStates().testFlag(Qt::WindowFullScreen);
        d_ptr->mprisController.setFullscreen(isFullScreen);
    });
}

void MprisItem::setupPlayerConnections()
{
    connect(&d_ptr->mprisController, &MprisController::volumeChangeRequested, d_ptr->player, [this](double volume) {
        const auto maxVolume = d_ptr->player->volumeMax();
        d_ptr->player->setVolume(volume * maxVolume);
    });
    connect(&d_ptr->mprisController, &MprisController::seekToRequested, this, [this](qlonglong offset) {
        d_ptr->player->setPosition(offset / 1'000'000.0);
    });
    connect(&d_ptr->mprisController, &MprisController::seekRequested, this, [this](qlonglong position) {
        d_ptr->player->seekRelative(double(position) / 1'000'000.0);
    });
    connect(&d_ptr->mprisController, &MprisController::stopRequested, d_ptr->player, &VlcItem::stop);
    connect(&d_ptr->mprisController, &MprisController::playpauseRequested, this, [this]() {
        d_ptr->player->setPause(!d_ptr->player->pause());
    });
    connect(&d_ptr->mprisController, &MprisController::playRequested, this, [this]() {
        d_ptr->player->setPause(false);
    });
    connect(&d_ptr->mprisController, &MprisController::pauseRequested, this, [this]() {
        d_ptr->player->setPause(true);
    });
    connect(&d_ptr->mprisController, &MprisController::rateChangeRequested, d_ptr->player, &VlcItem::setPlaybackSpeed);

    connect(d_ptr->player, &VlcItem::volumeChanged, &d_ptr->mprisController, [this]() {
        d_ptr->mprisController.setVolume(d_ptr->player->volume());
    });

    connect(d_ptr->player, &VlcItem::positionChanged, &d_ptr->mprisController, [this]() {
        d_ptr->mprisController.setPosition(d_ptr->player->position() * 1'000'000.0);
    });
    connect(d_ptr->player, &VlcItem::playbackStateChanged, &d_ptr->mprisController, [this]() {
        QString status = u"Stopped"_s;
        switch (d_ptr->player->playbackState()) {
        case VlcItem::PlaybackState::Stopped:
        case VlcItem::PlaybackState::Loading:
            status = u"Stopped"_s;
            break;
        case VlcItem::PlaybackState::Paused:
            status = u"Paused"_s;
            break;
        case VlcItem::PlaybackState::Playing:
            status = u"Playing"_s;
            break;
        default:
            status = u"Stopped"_s;
            break;
        }
        d_ptr->mprisController.setPlaybackStatus(status);
    });

    connect(d_ptr->player, &VlcItem::mediaTitleChanged, &d_ptr->mprisController, [this]() {
        if (!d_ptr->player->mediaTitle().isEmpty()) {
            d_ptr->mprisController.setMetadata(u"xesam:title"_s, d_ptr->player->mediaTitle());
        }
    });

    connect(d_ptr->player, &VlcItem::durationChanged, &d_ptr->mprisController, [this]() {
        const auto duration = d_ptr->player->duration() * 1'000'000.0;
        if (duration > 0) {
            d_ptr->mprisController.setMetadata(u"mpris:length"_s, duration);
        }
    });

    connect(d_ptr->player, &VlcItem::playbackSpeedChanged, &d_ptr->mprisController, [this]() {
        const auto speed = d_ptr->player->playbackSpeed();
        if (speed > 0) {
            d_ptr->mprisController.setRate(speed);
        }
    });

    connect(d_ptr->player, &VlcItem::currentUrlChanged, &d_ptr->mprisController, [this]() {
        const auto playlist = d_ptr->activePlaylist;
        const auto playlistItemIndex = playlist->playlistModel()->playingItem();
        const auto url = d_ptr->player->currentUrl();

        d_ptr->mprisController.setCanPlay(true);
        d_ptr->mprisController.setCanPause(true);
        d_ptr->mprisController.setCanSeek(true);

        const auto trackId = u"/win/sectorfive/runaplayer/track/%1"_s.arg(playlistItemIndex);
        d_ptr->mprisController.setMetadata(u"mpris:trackid"_s, trackId);
        d_ptr->mprisController.setMetadata(u"xesam:url"_s, url.toString());

        const auto mimeType = MiscUtils::mimeType(url);
        if (mimeType.startsWith(u"audio"_s)) {
            getAudioThumbnail(url);
        } else {
            // videos don't get a thumbnail so that the window preview
            // in the task manager displays the live window preview rather than a static thumbnail
            d_ptr->mprisController.setMetadata(u"mpris:artUrl"_s, QString{});
        }
    });
}

void MprisItem::getAudioThumbnail(const QUrl &url)
{
    if (!url.isLocalFile()) {
        return;
    }

    QPointer self(this);
    QThreadPool::globalInstance()->start([this, self, url]() {
        if (!self) {
            return;
        }

        QByteArray imageData = embeddedArtwork(url);
        if (!imageData.isEmpty()) {
            QBuffer buffer(&imageData);
            if (!buffer.open(QBuffer::ReadOnly)) {
                qCDebug(RunaplayerGeneral) << "Failed to open buffer:" << buffer.errorString();
            }

            const auto format = QImageReader::imageFormat(&buffer);
            auto base64 = u"data:image/%1;base64,%2"_s.arg(format).arg(QString::fromLatin1(imageData.toBase64()));

            if (!self) {
                return;
            }
            QMetaObject::invokeMethod(self, &MprisItem::setAudioThumbnail, base64);
            return;
        }

        QFileInfo fileInfo(url.toLocalFile());
        if (fileInfo.isDir()) {
            return;
        }

        QString image = findAudioThumbnail(fileInfo.absolutePath());
        if (!self) {
            return;
        }
        QMetaObject::invokeMethod(self, &MprisItem::setAudioThumbnail, image);
    });
}

QString MprisItem::findAudioThumbnail(const QString &filePath)
{
    static const std::vector<std::string> coverNames = {"cover", "front", "folder"};
    static const std::vector<std::string> extensions = {".png", ".jpeg", ".jpg", ".webp"};
    const auto directory = std::filesystem::path(filePath.toStdString());

    for (const auto &name : coverNames) {
        for (const auto &ext : extensions) {
            const auto filePath = directory / (name + ext);

            const auto path = QString::fromStdString(filePath);
            if (QFileInfo::exists(path)) {
                return path;
            }
        }
    }

    return {};
}

void MprisItem::setAudioThumbnail(const QString &image)
{
    // embedded image encoded as base64
    if (image.startsWith(u"data:image/"_s)) {
        d_ptr->mprisController.setMetadata(u"mpris:artUrl"_s, image);
        return;
    }

    // file path
    auto mimeType = MiscUtils::mimeType(QUrl(image));
    if (QFile::exists(image) && mimeType.startsWith(u"image"_s)) {
        d_ptr->mprisController.setMetadata(u"mpris:artUrl"_s, {u"file://"_s % image});
        return;
    }

    if (!VideoSettings::defaultCover().isEmpty()) {
        d_ptr->mprisController.setMetadata(u"mpris:artUrl"_s, VideoSettings::defaultCover());
        return;
    }
    d_ptr->mprisController.setMetadata(u"mpris:artUrl"_s, QString{});
}

QByteArray MprisItem::embeddedArtwork(const QUrl &url)
{
    const auto metadata = MiscUtils::metadata(url, KFileMetaData::ExtractionResult::ExtractImageData);
    if (!metadata.has_value()) {
        return {};
    }

    const auto imageData = metadata.value().imageData;
    if (imageData.isEmpty()) {
        return {};
    }

    const auto data = d_ptr->metadataImage(imageData);
    if (data.isEmpty()) {
        return {};
    }

    return data;
}

#else

MprisItem::MprisItem(QObject *parent)
    : QObject(parent)
{
}

MprisItem::~MprisItem() = default;

VlcItem *MprisItem::player() const
{
    return {};
}

void MprisItem::setPlayer(VlcItem *newPlayer)
{
}

PlaylistFilterProxyModel *MprisItem::activePlaylist() const
{
    return {};
}

void MprisItem::setActivePlaylist(PlaylistFilterProxyModel *newActivePlaylist)
{
}

QWindow *MprisItem::window() const
{
    return {};
}

void MprisItem::setWindow(QWindow *newWindow)
{
}

#endif

#include "moc_mprisitem.cpp"