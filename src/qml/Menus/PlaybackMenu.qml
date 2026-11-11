/*
 * SPDX-FileCopyrightText: 2020 George Florea Bănuș <georgefb899@gmail.com>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

import QtQuick.Controls

import org.kde.ki18n

import win.sectorfive.runaplayer

Menu {
    id: root

    title: KI18n.i18nc("@title:menu", "&Playback")

    MenuItem { action: RunaplayerApp.actions.playPauseAction }
    MenuItem { action: RunaplayerApp.actions.stopAction }
    MenuItem { action: RunaplayerApp.actions.playNextAction }
    MenuItem { action: RunaplayerApp.actions.playPreviousAction }

    MenuItem { action: RunaplayerApp.actions.restartPlaybackAction }
    MenuItem { action: RunaplayerApp.actions.setLoopAction }

    MenuSeparator {}

    MenuItem { action: RunaplayerApp.actions.playbackSpeedIncreaseAction }
    MenuItem { action: RunaplayerApp.actions.playbackSpeedDecreaseAction }
    MenuItem { action: RunaplayerApp.actions.playbackSpeedResetAction }

    MenuSeparator {}

    Menu {
        title: KI18n.i18nc("@title:menu", "Seek")
        MenuItem { action: RunaplayerApp.actions.seekForwardSmallAction }
        MenuItem { action: RunaplayerApp.actions.seekBackwardSmallAction }

        MenuSeparator {}

        MenuItem { action: RunaplayerApp.actions.seekForwardMediumAction }
        MenuItem { action: RunaplayerApp.actions.seekBackwardMediumAction }

        MenuSeparator {}

        MenuItem { action: RunaplayerApp.actions.seekForwardBigAction }
        MenuItem { action: RunaplayerApp.actions.seekBackwardBigAction }

        MenuSeparator {}

        MenuItem { action: RunaplayerApp.actions.seekNextChapterAction }
        MenuItem { action: RunaplayerApp.actions.seekPreviousChapterAction }

        MenuSeparator {}

        MenuItem { action: RunaplayerApp.actions.frameStepForwardAction }
        MenuItem { action: RunaplayerApp.actions.frameStepBackwardAction }
        MenuItem { action: RunaplayerApp.actions.seekToWatchLaterPositionAction }
    }
}
