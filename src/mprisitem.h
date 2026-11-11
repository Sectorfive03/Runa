/*
 * SPDX-FileCopyrightText: 2026 George Florea Bănuș <georgefb899@gmail.com>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef MPRISITEM_H
#define MPRISITEM_H

#include <QObject>
#include <qqmlintegration.h>

#include "config-runaplayer.h"

class MprisItemPrivate;
class VlcItem;
class PlaylistFilterProxyModel;
class QWindow;

class MprisItem : public QObject
{
    Q_OBJECT
    QML_ELEMENT

public:
    explicit MprisItem(QObject *parent = nullptr);
    ~MprisItem();

    Q_PROPERTY(VlcItem *playerItem READ player WRITE setPlayer NOTIFY playerChanged FINAL REQUIRED)
    Q_SIGNAL void playerChanged();
    VlcItem *player() const;
    void setPlayer(VlcItem *newPlayer);

    Q_PROPERTY(PlaylistFilterProxyModel *activePlaylist READ activePlaylist WRITE setActivePlaylist NOTIFY activePlaylistChanged FINAL)
    Q_SIGNAL void activePlaylistChanged();
    PlaylistFilterProxyModel *activePlaylist() const;
    void setActivePlaylist(PlaylistFilterProxyModel *newActivePlaylist);

    Q_PROPERTY(QWindow *window READ window WRITE setWindow NOTIFY windowChanged FINAL REQUIRED)
    Q_SIGNAL void windowChanged();
    QWindow *window() const;
    void setWindow(QWindow *newWindow);

    Q_SIGNAL void raise();
    Q_SIGNAL void openUri(const QString &uri);

#if HAVE_DBUS
private:
    void setupPlayerConnections();
    void getAudioThumbnail(const QUrl &url);
    QString findAudioThumbnail(const QString &filePath);
    void setAudioThumbnail(const QString &image);
    QByteArray embeddedArtwork(const QUrl &url);
    std::unique_ptr<MprisItemPrivate> d_ptr;
#endif
};

#endif // MPRISITEM_H
