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

    title: KI18n.i18nc("@title:menu", "&Subtitles")

    Labs.Menu {
        id: primarySubtitleMenu

        title: KI18n.i18nc("@title:menu", "Primary Subtitle")
        enabled: root.m_mpv.subtitleTracksModel.rowCount > 1

        Instantiator {
            model: root.m_mpv.subtitleTracksModel
            delegate: Labs.MenuItem {
                id: primarySubDelegate

                required property string displayText
                required property string language
                required property string title
                required property string codec
                required property string trackId

                checkable: true
                checked: primarySubDelegate.trackId === root.m_mpv.subtitleId
                text: primarySubDelegate.displayText
                onTriggered: {
                    root.m_mpv.subtitleId = primarySubDelegate.trackId === root.m_mpv.subtitleId ? -1 : primarySubDelegate.trackId
                }
            }
            onObjectAdded: function(index, object) {
                primarySubtitleMenu.insertItem(index, object)
            }
            onObjectRemoved: function(index, object) {
                primarySubtitleMenu.removeItem(object)
            }
        }
    }

    Labs.MenuItem {
        text: RunaplayerApp.actions.toggleTranscriptAction.text
        enabled: root.m_mpv.playbackState !== VlcItem.PlaybackState.Stopped
        onTriggered: RunaplayerApp.actions.toggleTranscriptAction.trigger()
    }

    Labs.MenuSeparator {}

    Labs.MenuItem {
        icon.name: RunaplayerApp.actions.addExternalSubtitleAction.icon.name
        text: RunaplayerApp.actions.addExternalSubtitleAction.text
        onTriggered: RunaplayerApp.actions.addExternalSubtitleAction.trigger()
    }

    Labs.MenuItem {
        icon.name: RunaplayerApp.actions.subtitleQuickenAction.icon.name
        text: RunaplayerApp.actions.subtitleQuickenAction.text
        onTriggered: RunaplayerApp.actions.subtitleQuickenAction.trigger()
    }

    Labs.MenuItem {
        icon.name: RunaplayerApp.actions.subtitleDelayAction.icon.name
        text: RunaplayerApp.actions.subtitleDelayAction.text
        onTriggered: RunaplayerApp.actions.subtitleDelayAction.trigger()
    }

    Labs.MenuItem {
        icon.name: RunaplayerApp.actions.subtitleToggleAction.icon.name
        text: RunaplayerApp.actions.subtitleToggleAction.text
        onTriggered: RunaplayerApp.actions.subtitleToggleAction.trigger()
    }

    Labs.MenuItem {
        icon.name: RunaplayerApp.actions.subtitleCycleUpAction.icon.name
        text: RunaplayerApp.actions.subtitleCycleUpAction.text
        onTriggered: RunaplayerApp.actions.subtitleCycleUpAction.trigger()
    }

    Labs.MenuItem {
        icon.name: RunaplayerApp.actions.subtitleCycleDownAction.icon.name
        text: RunaplayerApp.actions.subtitleCycleDownAction.text
        onTriggered: RunaplayerApp.actions.subtitleCycleDownAction.trigger()
    }

}
