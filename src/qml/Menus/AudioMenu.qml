/*
 * SPDX-FileCopyrightText: 2020 George Florea Bănuș <georgefb899@gmail.com>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

import QtQml
import QtQuick
import QtQuick.Controls

import org.kde.ki18n

import win.sectorfive.runaplayer

Menu {
    id: root

    required property VlcVideo m_mpv

    title: KI18n.i18nc("@title:menu", "&Audio")

    AudioTracksMenu {
        id: audioMenu

        m_mpv: root.m_mpv

        y: parent.height
        title: KI18n.i18nc("@title:menu", "&Track")
    }

    MenuSeparator {}

    MenuItem { action: RunaplayerApp.actions.muteAction }
    MenuItem { action: RunaplayerApp.actions.volumeUpAction }
    MenuItem { action: RunaplayerApp.actions.volumeDownAction }
    MenuItem { action: RunaplayerApp.actions.audioCycleUpAction }
    MenuItem { action: RunaplayerApp.actions.audioCycleDownAction }
}
