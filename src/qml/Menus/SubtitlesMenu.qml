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

    title: KI18n.i18nc("@title:menu", "&Subtitles")

    SubtitleTracksMenu {
        m_mpv: root.m_mpv
        title: KI18n.i18nc("@title:menu", "Primary Subtitle")
    }

    MenuItem {
        action: RunaplayerApp.actions.toggleTranscriptAction
        enabled: root.m_mpv.playbackState !== VlcItem.PlaybackState.Stopped
    }

    MenuSeparator {}

    MenuItem {
        action: RunaplayerApp.actions.addExternalSubtitleAction
        enabled: root.m_mpv.currentUrl.toString() !== ""
    }
    MenuItem { action: RunaplayerApp.actions.subtitleQuickenAction }
    MenuItem { action: RunaplayerApp.actions.subtitleDelayAction }
    MenuItem { action: RunaplayerApp.actions.subtitleToggleAction }
    MenuItem { action: RunaplayerApp.actions.subtitleCycleUpAction }
    MenuItem { action: RunaplayerApp.actions.subtitleCycleDownAction }
}
