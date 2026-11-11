/*
 * SPDX-FileCopyrightText: 2020 George Florea Bănuș <georgefb899@gmail.com>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls

import org.kde.ki18n

import win.sectorfive.runaplayer
import win.sectorfive.runaplayer.playlist
import win.sectorfive.runaplayer.utilities
import win.sectorfive.runaplayer.settings

Item {
    id: root

    required property VlcItem m_mpv
    required property PlaylistsManager playlistsManager
    required property PlayerContextMenu m_mpvContextMenuLoader
    required property Footer m_footer
    required property InputPopup m_openUrlPopup

    signal openSettingsWindow(int page)
    signal openFileDialog()
    signal openFolderDialog()
    signal openSubtitleDialog()
    signal togglePlaylist()
    signal toggleTranscript()

    Instantiator {
        model: Models.actionsModel
        delegate: Action {
            id: delegate

            required property string actionName
            required property string actionText
            required property string actionDescription
            required property string actionShortcut
            required property string actionIcon
            required property string actionIconSource
            required property string actionType

            enabled: RunaplayerApp.actionsEnabled
            objectName: delegate.actionName
            text: delegate.actionText
            shortcut: delegate.actionShortcut
            icon.name: delegate.actionIcon
            icon.source: delegate.actionIconSource
            onTriggered: {
                if (delegate.actionType === "NormalAction") {
                    Models.actionsModel.signalEmitter(objectName)
                }
            }

            Component.onCompleted: RunaplayerApp.actions[objectName] = this
        }
    }

    Connections {
        target: Models.actionsModel

        function onOpenActionsDialogAction() {
            const item = selectActionPopupComponent.createObject(root.m_mpv)
            const actionsPopup = item as SelectActionPopup
            actionsPopup.open()
        }

        function onAboutRunaplayerAction() {
            root.openSettingsWindow(SettingsWindow.Page.About)
        }

        function onReportBugAction() {
            Qt.openUrlExternally("https://github.com/Sectorfive03/Runa/issues/new")
        }

        function onSponsorAction() {
            Qt.openUrlExternally("https://runa.sectorfive.win")
        }

        function onAudioCycleUpAction() {
            root.m_mpv.setNextTrack(VlcItem.TrackType.Audio)
        }

        function onAudioCycleDownAction() {
            root.m_mpv.setPreviousTrack(VlcItem.TrackType.Audio)
        }

        function onConfigureAction() {
            root.openSettingsWindow(SettingsWindow.Page.None)
        }

        function onConfigureGeneralAction() {
            root.openSettingsWindow(SettingsWindow.Page.General)
        }

        function onConfigureOsdAction() {
            root.openSettingsWindow(SettingsWindow.Page.Osd)
        }

        function onConfigurePlaybackAction() {
            root.openSettingsWindow(SettingsWindow.Page.Playback)
        }

        function onConfigureVideoAction() {
            root.openSettingsWindow(SettingsWindow.Page.Video)
        }

        function onConfigureAudioAction() {
            root.openSettingsWindow(SettingsWindow.Page.Audio)
        }

        function onConfigureSubtitlesAction() {
            root.openSettingsWindow(SettingsWindow.Page.Subtitles)
        }

        function onConfigurePlaylistAction() {
            root.openSettingsWindow(SettingsWindow.Page.Playlist)
        }

        function onConfigureMouseAction() {
            root.openSettingsWindow(SettingsWindow.Page.Mouse)
        }

        function onConfigureShortcutsAction() {
            root.openSettingsWindow(SettingsWindow.Page.Shortcuts)
        }

        function onConfigureDatabaseAction() {
            root.openSettingsWindow(SettingsWindow.Page.Database)
        }

        function onConfigureDebugAction() {
            root.openSettingsWindow(SettingsWindow.Page.Debug)
        }

        function onExitFullscreenAction() {
            const mainWindow = root.Window.window as Main
            mainWindow.exitFullscreen()
        }

        function onFrameStepForwardAction() {
            root.m_mpv.frameStepForward()
        }

        function onFrameStepBackwardAction() {
            // libvlc only exposes stepping to the next frame. Seeking slightly
            // back is the closest behaviour we can offer.
            root.m_mpv.seekRelative(-0.04)
        }

        function onLoadLastPlayedFileAction() {
            if (GeneralSettings.lastPlayedFile === "") {
                return
            }

            const mainWindow = root.Window.window as Main
            mainWindow.openFile(GeneralSettings.lastPlayedFile, RecentFilesModel.OpenedFrom.Other)
        }

        function onMuteAction() {
            root.m_mpv.mute = !root.m_mpv.mute
        }

        function onOpenContextMenuAction() {
            // if menu is open it will be closed because of the Popup.CloseOnPressOutside close policy
            // but that happens after the call to openContextMenuLoader() below
            // resulting in the menu being opened for a very short time and then closed
            root.m_mpvContextMenuLoader.close()
            root.m_mpvContextMenuLoader.open()
        }

        function onOpenFileAction() {
            root.openFileDialog()
        }

        function onOpenFolderAction() {
            root.openFolderDialog()
        }

        function onAddExternalSubtitleAction() {
            if (root.m_mpv.currentUrl.toString() === "") {
                const msg = KI18n.i18nc("osd message when trying to open subtitle selection dialog while no file is open",
                                        "Can’t load subtitle when no file is open")
                MiscUtils.error(msg)
                return
            }

            root.openSubtitleDialog()
        }

        function onOpenUrlAction() {
            if (root.m_openUrlPopup.visible) {
                root.m_openUrlPopup.close()
            } else {
                root.m_openUrlPopup.open()
            }
        }

        function onPlaybackSpeedIncreaseAction() {
            const speed = parseFloat(root.m_mpv.playbackSpeed + 0.1)
            root.m_mpv.playbackSpeed = speed
            OsdUtils.message(KI18n.i18nc("@info:tooltip", "Playback speed: %1", speed.toFixed(2)), OsdUtils.Context.ShowPlaybackSpeed)
        }

        function onPlaybackSpeedDecreaseAction() {
            const speed = parseFloat(root.m_mpv.playbackSpeed - 0.1)
            root.m_mpv.playbackSpeed = speed
            OsdUtils.message(KI18n.i18nc("@info:tooltip", "Playback speed: %1", speed.toFixed(2)), OsdUtils.Context.ShowPlaybackSpeed)
        }

        function onPlaybackSpeedResetAction() {
            root.m_mpv.playbackSpeed = 1.0
            OsdUtils.message(KI18n.i18nc("@info:tooltip", "Playback speed: %1", 1.0), OsdUtils.Context.ShowPlaybackSpeed)
        }

        function onPlayPauseAction() {
            root.m_mpv.pause = !root.m_mpv.pause
        }

        function onPlayNextAction() {
            root.playlistsManager.playNext()
            Models.recentFilesModel.addRecentFile(root.playlistsManager.activeItemPath(), RecentFilesModel.OpenedFrom.Playlist)
        }

        function onStopAction() {
            root.m_mpv.stop()
        }

        function onPlayPreviousAction() {
            root.playlistsManager.playPrevious()
            Models.recentFilesModel.addRecentFile(root.playlistsManager.activeItemPath(), RecentFilesModel.OpenedFrom.Playlist)
        }

        function onQuitApplicationAction() {
            Qt.quit()
        }

        function onRestartPlaybackAction() {
            root.m_mpv.seekAbsolute(0)
        }

        function onSeekForwardSmallAction() {
            root.m_mpv.seekRelative(PlaybackSettings.seekSmallStep)
        }

        function onSeekBackwardSmallAction() {
            root.m_mpv.seekRelative(-PlaybackSettings.seekSmallStep)
        }

        function onSeekForwardMediumAction() {
            root.m_mpv.seekRelative(PlaybackSettings.seekMediumStep)
        }

        function onSeekBackwardMediumAction() {
            root.m_mpv.seekRelative(-PlaybackSettings.seekMediumStep)
        }

        function onSeekForwardBigAction() {
            root.m_mpv.seekRelative(PlaybackSettings.seekBigStep)
        }

        function onSeekBackwardBigAction() {
            root.m_mpv.seekRelative(-PlaybackSettings.seekBigStep)
        }

        function onSeekNextChapterAction() {
            if (root.m_mpv.chapter + 1 >= root.m_mpv.chaptersModel.rowCount) {
                Models.actionsModel.signalEmitter("playNextAction")
                return
            }
            root.m_mpv.nextChapter()
        }

        function onSeekPreviousChapterAction() {
            root.m_mpv.previousChapter()
        }

        function onSeekToWatchLaterPositionAction() {
            if (root.m_mpv.watchLaterPosition === 0) {
                return
            }
            root.m_mpv.seekAbsolute(root.m_mpv.watchLaterPosition)
        }

        function onSetLoopAction() {
            const a = root.m_mpv.abLoopA
            const b = root.m_mpv.abLoopB

            const aIsSet = a >= 0
            const bIsSet = b >= 0

            if (!aIsSet && !bIsSet) {
                root.m_mpv.abLoopA = root.m_mpv.position
                root.m_footer.progressBar.loopIndicator.startPosition = root.m_mpv.position
                const msg = KI18n.i18nc("@info:tooltip; %1 is the timestamp where the loop starts",
                                        "Loop start: %1",
                                        MiscUtils.formatTime(root.m_mpv.position))
                OsdUtils.message(msg, OsdUtils.Context.ShowABLoop)
            } else if (aIsSet && !bIsSet) {
                // set b position slightly ahead to ensure the loop section is not skipped
                const bPosition = root.m_mpv.position + 0.1
                root.m_mpv.abLoopB = bPosition
                root.m_footer.progressBar.loopIndicator.endPosition = bPosition
                const msg = KI18n.i18nc("@info:tooltip, use en dash for the range",
                                        "Loop: %1–%2",
                                        MiscUtils.formatTime(a), MiscUtils.formatTime(bPosition))
                OsdUtils.message(msg, OsdUtils.Context.ShowABLoop)
            } else {
                root.m_mpv.clearABLoop()
                root.m_footer.progressBar.loopIndicator.startPosition = -1
                root.m_footer.progressBar.loopIndicator.endPosition = -1
                const msg = KI18n.i18nc("@info:tooltip", "Loop cleared")
                OsdUtils.message(msg, OsdUtils.Context.ShowABLoop)
            }
        }

        function onScreenshotAction() {
            root.m_mpv.takeScreenshot()
        }

        function onSubtitleQuickenAction() {
            root.m_mpv.subtitleDelay = root.m_mpv.subtitleDelay - 0.1
        }

        function onSubtitleDelayAction() {
            root.m_mpv.subtitleDelay = root.m_mpv.subtitleDelay + 0.1
        }

        function onSubtitleToggleAction() {
            const visible = root.m_mpv.subtitlesVisible
            const message = visible ? KI18n.i18nc("@info:tooltip", "Subtitles off") : KI18n.i18nc("@info:tooltip", "Subtitles on")
            root.m_mpv.subtitlesVisible = !visible
            OsdUtils.message(message, OsdUtils.Context.ShowSubtitleToggle)
        }

        function onSubtitleCycleUpAction() {
            root.m_mpv.setNextTrack(VlcItem.TrackType.Subtitle)
        }

        function onSubtitleCycleDownAction() {
            root.m_mpv.setPreviousTrack(VlcItem.TrackType.Subtitle)
        }



        function onToggleDeinterlacingAction() {
            const deinterlaced = !root.m_mpv.deinterlaced
            root.m_mpv.deinterlaced = deinterlaced
            const msg = KI18n.i18nc("@info:tooltip", "Deinterlace: %1", deinterlaced)
            OsdUtils.message(msg, OsdUtils.Context.ShowDeinterlace)
        }

        function onToggleFullscreenAction() {
            const mainWindow = root.Window.window as Main
            mainWindow.toggleFullScreen()
        }

        function onToggleMenuBarAction() {
            GeneralSettings.showMenuBar = !GeneralSettings.showMenuBar
            GeneralSettings.save()
        }

        function onTogglePlaylistAction() {
            root.togglePlaylist()
        }

        function onToggleTranscriptAction() {
            root.toggleTranscript()
        }

        function onVolumeUpAction() {
            root.m_mpv.volume = Math.min(root.m_mpv.volumeMax, root.m_mpv.volume + AudioSettings.volumeStep)
        }

        function onVolumeDownAction() {
            root.m_mpv.volume = Math.max(0, root.m_mpv.volume - AudioSettings.volumeStep)
        }

        function onContrastUpAction() {
            const contrast = root.m_mpv.imageAdjustment("contrast") + 1
            root.m_mpv.setImageAdjustment("contrast", contrast)
            const msg = KI18n.i18nc("@info:tooltip", "Contrast: %1", contrast)
            OsdUtils.message(msg, OsdUtils.Context.ShowImageAdjustments)
        }

        function onContrastDownAction() {
            const contrast = root.m_mpv.imageAdjustment("contrast") - 1
            root.m_mpv.setImageAdjustment("contrast", contrast)
            const msg = KI18n.i18nc("@info:tooltip", "Contrast: %1", contrast)
            OsdUtils.message(msg, OsdUtils.Context.ShowImageAdjustments)
        }

        function onContrastResetAction() {
            root.m_mpv.setImageAdjustment("contrast", 0)
            const msg = KI18n.i18nc("@info:tooltip", "Contrast: 0")
            OsdUtils.message(msg, OsdUtils.Context.ShowImageAdjustments)
        }

        function onBrightnessUpAction() {
            const brightness = root.m_mpv.imageAdjustment("brightness") + 1
            root.m_mpv.setImageAdjustment("brightness", brightness)
            const msg = KI18n.i18nc("@info:tooltip", "Brightness: %1", brightness)
            OsdUtils.message(msg, OsdUtils.Context.ShowImageAdjustments)
        }

        function onBrightnessDownAction() {
            const brightness = root.m_mpv.imageAdjustment("brightness") - 1
            root.m_mpv.setImageAdjustment("brightness", brightness)
            const msg = KI18n.i18nc("@info:tooltip", "Brightness: %1", brightness)
            OsdUtils.message(msg, OsdUtils.Context.ShowImageAdjustments)
        }

        function onBrightnessResetAction() {
            root.m_mpv.setImageAdjustment("brightness", 0)
            const msg = KI18n.i18nc("@info:tooltip", "Brightness: 0")
            OsdUtils.message(msg, OsdUtils.Context.ShowImageAdjustments)
        }

        function onGammaUpAction() {
            const gamma = root.m_mpv.imageAdjustment("gamma") + 1
            root.m_mpv.setImageAdjustment("gamma", gamma)
            const msg = KI18n.i18nc("@info:tooltip", "Gamma: %1", gamma)
            OsdUtils.message(msg, OsdUtils.Context.ShowImageAdjustments)
        }

        function onGammaDownAction() {
            const gamma = root.m_mpv.imageAdjustment("gamma") - 1
            root.m_mpv.setImageAdjustment("gamma", gamma)
            const msg = KI18n.i18nc("@info:tooltip", "Gamma: %1", gamma)
            OsdUtils.message(msg, OsdUtils.Context.ShowImageAdjustments)
        }

        function onGammaResetAction() {
            root.m_mpv.setImageAdjustment("gamma", 0)
            const msg = KI18n.i18nc("@info:tooltip", "Gamma: 0")
            OsdUtils.message(msg, OsdUtils.Context.ShowImageAdjustments)
        }

        function onSaturationUpAction() {
            const saturation = root.m_mpv.imageAdjustment("saturation") + 1
            root.m_mpv.setImageAdjustment("saturation", saturation)
            const msg = KI18n.i18nc("@info:tooltip", "Saturation: %1", saturation)
            OsdUtils.message(msg, OsdUtils.Context.ShowImageAdjustments)
        }

        function onSaturationDownAction() {
            const saturation = root.m_mpv.imageAdjustment("saturation") - 1
            root.m_mpv.setImageAdjustment("saturation", saturation)
            const msg = KI18n.i18nc("@info:tooltip", "Saturation: %1", saturation)
            OsdUtils.message(msg, OsdUtils.Context.ShowImageAdjustments)
        }

        function onSaturationResetAction() {
            root.m_mpv.setImageAdjustment("saturation", 0)
            const msg = KI18n.i18nc("@info:tooltip", "Saturation: 0")
            OsdUtils.message(msg, OsdUtils.Context.ShowImageAdjustments)
        }
    }

    Component {
        id: selectActionPopupComponent

        SelectActionPopup {
            id: triggerActionPopup

            onActionSelected: function(actionName) {
                RunaplayerApp.actions[actionName].trigger()
            }
            onClosed: destroy()
        }
    }
}
