/*
 * SPDX-FileCopyrightText: 2022 George Florea Bănuș <georgefb899@gmail.com>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls

import org.kde.ki18n

import win.sectorfive.runaplayer

Loader {
    id: root

    required property VlcVideo m_mpv

    function open() : void {
        if (!root.active) {
            root.active = true
            root.loaded.connect(function() {
                root.open()
            })
            return
        }

        const contextMenu = root.item as Menu
        contextMenu.popup()
    }

    function close() : void {
        root.active = false
    }

    active: false
    asynchronous: true
    sourceComponent: Menu {

        onClosed: root.active = false

        MenuItem { action: RunaplayerApp.actions.playPauseAction }
        MenuItem { action: RunaplayerApp.actions.playNextAction }
        MenuItem { action: RunaplayerApp.actions.playPreviousAction }
        Menu {
            title: KI18n.i18nc("@title:menu", "Playback Speed")
            MenuItem { action: RunaplayerApp.actions.playbackSpeedIncreaseAction }
            MenuItem { action: RunaplayerApp.actions.playbackSpeedDecreaseAction }
            MenuItem { action: RunaplayerApp.actions.playbackSpeedResetAction }
        }

        MenuSeparator {}

        MenuItem { action: RunaplayerApp.actions.openFileAction }
        MenuItem { action: RunaplayerApp.actions.openFolderAction }
        MenuItem { action: RunaplayerApp.actions.openUrlAction }

        MenuSeparator {}

        VideoMenu {}
        AudioMenu {
            m_mpv: root.m_mpv
        }
        SubtitlesMenu {
            m_mpv: root.m_mpv
        }

        MenuSeparator {}

        MenuItem { action: RunaplayerApp.actions.configureAction }
    }
}