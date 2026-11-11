/*
 * SPDX-FileCopyrightText: 2022 George Florea Bănuș <georgefb899@gmail.com>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

pragma ComponentBehavior: Bound

import QtQml
import QtQuick
import Qt.labs.platform as Labs

import org.kde.ki18n

import win.sectorfive.runaplayer

Labs.Menu {
    id: root

    required property VlcVideo m_mpv

    title: KI18n.i18nc("@title:menu", "&Audio")

    Labs.Menu {
        id: audioMenu

        title: KI18n.i18nc("@title:menu", "&Track")
        enabled: root.m_mpv.audioTracksModel.rowCount > 0

        Instantiator {
            model: root.m_mpv.audioTracksModel
            delegate: Labs.MenuItem {
                id: delegate

                required property string displayText
                required property string language
                required property string title
                required property string codec
                required property string trackId

                checkable: true
                checked: delegate.trackId === root.m_mpv.audioId
                text: delegate.displayText
                onTriggered: {
                    root.m_mpv.audioId = delegate.trackId === root.m_mpv.audioId ? -1 : delegate.trackId
                }
            }
            onObjectAdded: function(index, object) {
                audioMenu.insertItem(index, object)
            }
            onObjectRemoved: function(index, object) {
                audioMenu.removeItem(object)
            }
        }
    }

    Labs.MenuSeparator {}

    Labs.MenuItem {
        icon.name: RunaplayerApp.actions.muteAction.icon.name
        text: RunaplayerApp.actions.muteAction.text
        onTriggered: RunaplayerApp.actions.muteAction.trigger()
    }

    Labs.MenuItem {
        icon.name: RunaplayerApp.actions.volumeUpAction.icon.name
        text: RunaplayerApp.actions.volumeUpAction.text
        onTriggered: RunaplayerApp.actions.volumeUpAction.trigger()
    }

    Labs.MenuItem {
        icon.name: RunaplayerApp.actions.volumeDownAction.icon.name
        text: RunaplayerApp.actions.volumeDownAction.text
        onTriggered: RunaplayerApp.actions.volumeDownAction.trigger()
    }

    Labs.MenuItem {
        icon.name: RunaplayerApp.actions.audioCycleUpAction.icon.name
        text: RunaplayerApp.actions.audioCycleUpAction.text
        onTriggered: RunaplayerApp.actions.audioCycleUpAction.trigger()
    }

    Labs.MenuItem {
        icon.name: RunaplayerApp.actions.audioCycleDownAction.icon.name
        text: RunaplayerApp.actions.audioCycleDownAction.text
        onTriggered: RunaplayerApp.actions.audioCycleDownAction.trigger()
    }
}
