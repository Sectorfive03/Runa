/*
 * SPDX-FileCopyrightText: 2022 George Florea Bănuș <georgefb899@gmail.com>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

import Qt.labs.platform as Labs

import org.kde.ki18n

import win.sectorfive.runaplayer

Labs.Menu {
    id: root

    title: KI18n.i18nc("@title:menu", "&Playback")

    Labs.MenuItem {
        icon.name: RunaplayerApp.actions.playPauseAction.icon.name
        text: RunaplayerApp.actions.playPauseAction.text
        onTriggered: RunaplayerApp.actions.playPauseAction.trigger()
    }

    Labs.MenuItem {
        icon.name: RunaplayerApp.actions.stopAction.icon.name
        text: RunaplayerApp.actions.stopAction.text
        onTriggered: RunaplayerApp.actions.stopAction.trigger()
    }

    Labs.MenuItem {
        icon.name: RunaplayerApp.actions.playNextAction.icon.name
        text: RunaplayerApp.actions.playNextAction.text
        onTriggered: RunaplayerApp.actions.playNextAction.trigger()
    }

    Labs.MenuItem {
        icon.name: RunaplayerApp.actions.playPreviousAction.icon.name
        text: RunaplayerApp.actions.playPreviousAction.text
        onTriggered: RunaplayerApp.actions.playPreviousAction.trigger()
    }

    Labs.MenuItem {
        icon.name: RunaplayerApp.actions.restartPlaybackAction.icon.name
        text: RunaplayerApp.actions.restartPlaybackAction.text
        onTriggered: RunaplayerApp.actions.restartPlaybackAction.trigger()
    }

    Labs.MenuItem {
        icon.name: RunaplayerApp.actions.setLoopAction.icon.name
        text: RunaplayerApp.actions.setLoopAction.text
        onTriggered: RunaplayerApp.actions.setLoopAction.trigger()
    }

    Labs.MenuSeparator {}

    Labs.MenuItem {
        icon.name: RunaplayerApp.actions.playbackSpeedIncreaseAction.icon.name
        text: RunaplayerApp.actions.playbackSpeedIncreaseAction.text
        onTriggered: RunaplayerApp.actions.playbackSpeedIncreaseAction.trigger()
    }

    Labs.MenuItem {
        icon.name: RunaplayerApp.actions.playbackSpeedDecreaseAction.icon.name
        text: RunaplayerApp.actions.playbackSpeedDecreaseAction.text
        onTriggered: RunaplayerApp.actions.playbackSpeedDecreaseAction.trigger()
    }

    Labs.MenuItem {
        icon.name: RunaplayerApp.actions.playbackSpeedResetAction.icon.name
        text: RunaplayerApp.actions.playbackSpeedResetAction.text
        onTriggered: RunaplayerApp.actions.playbackSpeedResetAction.trigger()
    }

    Labs.MenuSeparator {}

    Labs.Menu {
        title: KI18n.i18nc("@title:menu", "Seek")
        Labs.MenuItem {
            icon.name: RunaplayerApp.actions.seekForwardSmallAction.icon.name
            text: RunaplayerApp.actions.seekForwardSmallAction.text
            onTriggered: RunaplayerApp.actions.seekForwardSmallAction.trigger()
        }
        Labs.MenuItem {
            icon.name: RunaplayerApp.actions.seekBackwardSmallAction.icon.name
            text: RunaplayerApp.actions.seekBackwardSmallAction.text
            onTriggered: RunaplayerApp.actions.seekBackwardSmallAction.trigger()
        }

        Labs.MenuSeparator {}

        Labs.MenuItem {
            icon.name: RunaplayerApp.actions.seekForwardMediumAction.icon.name
            text: RunaplayerApp.actions.seekForwardMediumAction.text
            onTriggered: RunaplayerApp.actions.seekForwardMediumAction.trigger()
        }

        Labs.MenuItem {
            icon.name: RunaplayerApp.actions.seekBackwardMediumAction.icon.name
            text: RunaplayerApp.actions.seekBackwardMediumAction.text
            onTriggered: RunaplayerApp.actions.seekBackwardMediumAction.trigger()
        }

        Labs.MenuSeparator {}

        Labs.MenuItem {
            icon.name: RunaplayerApp.actions.seekForwardBigAction.icon.name
            text: RunaplayerApp.actions.seekForwardBigAction.text
            onTriggered: RunaplayerApp.actions.seekForwardBigAction.trigger()
        }

        Labs.MenuItem {
            icon.name: RunaplayerApp.actions.seekBackwardBigAction.icon.name
            text: RunaplayerApp.actions.seekBackwardBigAction.text
            onTriggered: RunaplayerApp.actions.seekBackwardBigAction.trigger()
        }

        Labs.MenuSeparator {}

        Labs.MenuItem {
            icon.name: RunaplayerApp.actions.seekNextChapterAction.icon.name
            text: RunaplayerApp.actions.seekNextChapterAction.text
            onTriggered: RunaplayerApp.actions.seekNextChapterAction.trigger()
        }

        Labs.MenuItem {
            icon.name: RunaplayerApp.actions.seekPreviousChapterAction.icon.name
            text: RunaplayerApp.actions.seekPreviousChapterAction.text
            onTriggered: RunaplayerApp.actions.seekPreviousChapterAction.trigger()
        }

        Labs.MenuSeparator {}

        Labs.MenuItem {
            icon.name: RunaplayerApp.actions.frameStepForwardAction.icon.name
            text: RunaplayerApp.actions.frameStepForwardAction.text
            onTriggered: RunaplayerApp.actions.frameStepForwardAction.trigger()
        }

        Labs.MenuItem {
            icon.name: RunaplayerApp.actions.frameStepBackwardAction.icon.name
            text: RunaplayerApp.actions.frameStepBackwardAction.text
            onTriggered: RunaplayerApp.actions.frameStepBackwardAction.trigger()
        }

        Labs.MenuItem {
            icon.name: RunaplayerApp.actions.seekToWatchLaterPositionAction.icon.name
            text: RunaplayerApp.actions.seekToWatchLaterPositionAction.text
            onTriggered: RunaplayerApp.actions.seekToWatchLaterPositionAction.trigger()
        }
    }
}
