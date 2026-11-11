/*
 * SPDX-FileCopyrightText: 2024 George Florea Bănuș <georgefb899@gmail.com>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import org.kde.ki18n

import org.kde.kirigami as Kirigami
import win.sectorfive.runaplayer
import win.sectorfive.runaplayer.playlist
import win.sectorfive.runaplayer.settings

Item {
    id: root

    required property VlcVideo m_mpv
    required property PlaylistsManager playlistsManager
    required property MenuBarLoader m_menuBarLoader

    property alias progressBar: progressBar
    property string footerState: isFloating ? "hidden" : "visible"
    readonly property bool isFullScreen: Window.window ? Window.window.visibility === Window.FullScreen : false
    // Fullscreen always uses the compact floating bar instead of a full-width
    // footer, matching the intended player look.
    property bool isFloating: GeneralSettings.footerStyle === "floating" || isFullScreen
    property bool isSmallSize: width < 600
    property int buttonSize: isSmallSize ? Kirigami.Units.iconSizes.small : Kirigami.Units.iconSizes.smallMedium

    implicitWidth: Window.window.contentItem.width
    implicitHeight: root.isFloating
                    ? footer.height + Kirigami.Units.largeSpacing * (root.isSmallSize ? 1 : 2)
                    : footer.height

    ToolBar {
        id: footer

        x: (root.width - width) / 2
        width: root.isFloating
               ? Math.min(Window.window.contentItem.width - Kirigami.Units.largeSpacing * (root.isSmallSize ? 2 : 4),
                          root.isFullScreen ? 700 : 900)
               : root.width

        state: root.footerState
        padding: Kirigami.Units.smallSpacing
        position: ToolBar.Footer
        hoverEnabled: true
        background: Rectangle {
            color: Qt.alpha(Kirigami.Theme.backgroundColor, root.isFloating ? 0.82 : 0.96)
            radius: root.isFloating ? Kirigami.Units.cornerRadius * 1.5 : 0
            border.width: root.isFloating ? 1 : 0
            border.color: Qt.rgba(1, 1, 1, 0.08)

            Rectangle {
                anchors.top: parent.top
                width: parent.width
                height: 1
                visible: !root.isFloating
                color: Qt.rgba(1, 1, 1, 0.06)
            }
        }

        Connections {
            target: RunaplayerApp
            function onQmlApplicationMouseLeave() {
                if (root.isFloating) {
                    hideFooterTimer.start()
                }
            }
        }

        Connections {
            target: root.m_mpv
            function onMousePositionChanged(x, y) {
                Qt.callLater(footer.setState)
            }
        }

        Timer {
            interval: 50
            running: !root.isFloating
            repeat: true
            onTriggered: {
                Qt.callLater(footer.staticState)
            }
        }

        Timer {
            id: hideFooterTimer

            running: false
            repeat: false
            interval: 1000
            onTriggered: {
                if (footer.hovered || hamburgerMenu.isOpen || progressBar.chaptersPopupIsOpen
                        || subtitleMenu.visible || audioMenu.visible) {
                    return
                }

                root.footerState = "hidden"
            }
        }

        Component {
            id: togglePlaylistButton

            ToolButton {
                action: RunaplayerApp.actions.togglePlaylistAction
                icon.width: root.buttonSize
                icon.height: root.buttonSize
                display: AbstractButton.IconOnly
            }
        }

        ColumnLayout {
            spacing: 0
            anchors.fill: parent

            RowLayout {
                id: footerRow

                spacing: root.isSmallSize ? 1 : Kirigami.Units.smallSpacing

                HamburgerMenu {
                    id: hamburgerMenu

                    m_mpv: root.m_mpv
                    m_menuBarLoader: root.m_menuBarLoader

                    visible: root.m_menuBarLoader.state === "hidden"
                    icon.width: root.buttonSize
                    icon.height: root.buttonSize
                }

                Loader {
                    sourceComponent: togglePlaylistButton
                    active: !PlaylistSettings.canToggleWithMouse && PlaylistSettings.position === "left"
                    visible: active
                    asynchronous: false
                }

                ToolButton {
                    id: playPauseButton

                    display: AbstractButton.IconOnly
                    icon.name: root.m_mpv.pause ? "media-playback-start" : "media-playback-pause"
                    icon.width: root.buttonSize
                    icon.height: root.buttonSize
                    focusPolicy: Qt.NoFocus
                    enabled: root.m_mpv.duration !== 0

                    onClicked: RunaplayerApp.actions.playPauseAction.trigger()

                    ToolTip {
                        id: playPauseButtonToolTip

                        text: root.m_mpv.pause ? KI18n.i18nc("@info:tooltip", "Start playback") : KI18n.i18nc("@info:tooltip", "Pause playback")
                        delay: RunaplayerApp.isAltKeyPressed ? 0 : Kirigami.Units.toolTipDelay
                        visible: playPauseButton.hovered
                                 && (GeneralSettings.showExplanatoryToolTips || RunaplayerApp.isAltKeyPressed)
                    }
                }

                ToolButton {
                    id: playPreviousFile

                    icon.name: footer.LayoutMirroring.enabled ? "media-skip-forward" : "media-skip-backward"
                    icon.width: root.buttonSize
                    icon.height: root.buttonSize
                    display: AbstractButton.IconOnly
                    focusPolicy: Qt.NoFocus
                    enabled: root.playlistsManager.activePlaylist.itemCount > 1

                    onClicked: RunaplayerApp.actions.playPreviousAction.trigger()

                    ToolTip {
                        text: KI18n.i18nc("@info:tooltip", "Play previous file")
                        delay: RunaplayerApp.isAltKeyPressed ? 0 : Kirigami.Units.toolTipDelay
                        visible: playPreviousFile.hovered
                                 && (GeneralSettings.showExplanatoryToolTips || RunaplayerApp.isAltKeyPressed)
                    }
                }

                ToolButton {
                    id: playNextFile

                    icon.name: footer.LayoutMirroring.enabled ? "media-skip-backward" : "media-skip-forward"
                    icon.width: root.buttonSize
                    icon.height: root.buttonSize
                    display: AbstractButton.IconOnly
                    focusPolicy: Qt.NoFocus
                    enabled: root.playlistsManager.activePlaylist.itemCount > 1

                    onClicked: RunaplayerApp.actions.playNextAction.trigger()

                    ToolTip {
                        text: KI18n.i18nc("@info:tooltip", "Play next file")
                        delay: RunaplayerApp.isAltKeyPressed ? 0 : Kirigami.Units.toolTipDelay
                        visible: playNextFile.hovered
                                 && (GeneralSettings.showExplanatoryToolTips || RunaplayerApp.isAltKeyPressed)
                    }
                }

                ToolButton {
                    id: chaptersMenuButton

                    icon.name: "overflow-menu"
                    focusPolicy: Qt.NoFocus
                    visible: !progressBar.showChapterMarkers && root.m_mpv.chaptersModel.rowCount > 0

                    onClicked: {
                        progressBar.openChapterPopup(this)
                    }

                    ToolTip {
                        text: KI18n.i18nc("@info:tooltip", "Open chapters menu")
                        delay: RunaplayerApp.isAltKeyPressed ? 0 : Kirigami.Units.toolTipDelay
                        visible: chaptersMenuButton.hovered
                                 && (GeneralSettings.showExplanatoryToolTips || RunaplayerApp.isAltKeyPressed)
                    }
                }

                HProgressBar {
                    id: progressBar

                    m_mpv: root.m_mpv

                    Layout.fillWidth: true
                    Layout.preferredHeight: 20
                }

                ToolButton {
                    id: mute

                    action: RunaplayerApp.actions.muteAction
                    icon.name: root.m_mpv.mute || root.m_mpv.volume === 0 ? "player-volume-muted" : "player-volume"
                    icon.width: root.buttonSize
                    icon.height: root.buttonSize
                    display: AbstractButton.IconOnly
                    focusPolicy: Qt.NoFocus

                    ToolTip {
                        text: RunaplayerApp.actions.muteAction.text
                        delay: RunaplayerApp.isAltKeyPressed ? 0 : Kirigami.Units.toolTipDelay
                        visible: mute.hovered
                                 && (GeneralSettings.showExplanatoryToolTips || RunaplayerApp.isAltKeyPressed)
                    }
                }


                VolumeSlider {
                    id: volumeSlider

                    m_mpv: root.m_mpv

                    Layout.preferredWidth: root.isSmallSize ? 60 : 100
                    Layout.preferredHeight: 20
                }

                ToolButton {
                    id: subtitleMenuButton

                    display: AbstractButton.IconOnly
                    icon.name: "media-view-subtitles-symbolic"
                    icon.width: root.buttonSize
                    icon.height: root.buttonSize
                    focusPolicy: Qt.NoFocus
                    enabled: root.m_mpv.subtitleTracksModel.rowCount > 1
                    opacity: enabled ? 1.0 : 0.6

                    onReleased: {
                        subtitleMenu.visible = !subtitleMenu.visible
                    }

                    SubtitleTracksMenu {
                        id: subtitleMenu

                        m_mpv: root.m_mpv

                        y: -height
                    }

                    ToolTip {
                        text: KI18n.i18nc("@info:tooltip", "Subtitles")
                        delay: RunaplayerApp.isAltKeyPressed ? 0 : Kirigami.Units.toolTipDelay
                        visible: subtitleMenuButton.hovered
                                 && (GeneralSettings.showExplanatoryToolTips || RunaplayerApp.isAltKeyPressed)
                    }
                }

                ToolButton {
                    id: audioMenuButton

                    display: AbstractButton.IconOnly
                    icon.name: "media-album-track"
                    icon.width: root.buttonSize
                    icon.height: root.buttonSize
                    focusPolicy: Qt.NoFocus
                    enabled: root.m_mpv.audioTracksModel.rowCount > 0
                    opacity: enabled ? 1.0 : 0.6

                    onReleased: {
                        audioMenu.visible = !audioMenu.visible
                    }

                    AudioTracksMenu {
                        id: audioMenu

                        m_mpv: root.m_mpv

                        y: -height
                        model: root.m_mpv.audioTracksModel
                    }

                    ToolTip {
                        text: KI18n.i18nc("@info:tooltip", "Audio")
                        delay: RunaplayerApp.isAltKeyPressed ? 0 : Kirigami.Units.toolTipDelay
                        visible: audioMenuButton.hovered
                                 && (GeneralSettings.showExplanatoryToolTips || RunaplayerApp.isAltKeyPressed)
                    }
                }

                Loader {
                    sourceComponent: togglePlaylistButton
                    active: !PlaylistSettings.canToggleWithMouse && PlaylistSettings.position === "right"
                    visible: active
                    asynchronous: false
                }
            }

            RowLayout {
                LabelWithTooltip {
                    id: mediaTitleLabel

                    text: root.m_mpv.mediaTitle
                    font.pointSize: 9
                    toolTipText: root.m_mpv.mediaTitle
                    elide: Text.ElideRight
                    visible: !root.isFullScreen

                    Layout.fillWidth: true
                }

                LabelWithTooltip {
                    id: timeInfo

                    text: "%1 / %2".arg(root.m_mpv.formattedPosition).arg(root.m_mpv.formattedDuration)
                    font.pointSize: 9
                    toolTipText: KI18n.i18nc("@info:tooltip", "Remaining: %1", root.m_mpv.formattedRemaining)
                    alwaysShowToolTip: true
                    horizontalAlignment: root.isFullScreen ? Qt.AlignRight : Qt.AlignHCenter

                    Layout.fillWidth: root.isFullScreen
                    Layout.minimumWidth: textMetrics.width

                    TextMetrics {
                        id: textMetrics

                        font: timeInfo.font
                        text: "000:00:00 / 000:00:00"
                    }
                }
            }
        }

        function setState() : void {
            if (root.isFloating) {
                floatingState()
            } else {
                staticState()
            }
        }

        function floatingState() {
            if (GeneralSettings.floatingFooterTrigger === "EveryMouseMovement") {
                hideFooterTimer.stop()
                root.footerState = "visible"
                if (root.m_mpv.mouseY < root.m_mpv.height - footer.height * 2) {
                    hideFooterTimer.restart()
                }
            } else {
                if (root.m_mpv.mouseY > root.m_mpv.height - footer.height * 2) {
                    const mainWindow = root.Window.window as Main
                    if (mainWindow.containsMouse) {
                        hideFooterTimer.stop()
                        root.footerState = "visible"
                    }
                } else {
                    hideFooterTimer.start()
                }
            }
        }

        function staticState() {
            const mainWindow = Window.window as Main
            if (!mainWindow.isFullScreen() || (root.m_mpv.mouseY > root.m_mpv.height - footer.height * 2 && mainWindow.containsMouse)) {
                root.footerState = "visible"
            } else {
                root.footerState = "hidden"
            }
        }

        states: [
            State {
                name: "hidden"
                PropertyChanges {
                    footer.y: root.height
                    footer.visible: false
                }
            },
            State {
                name : "visible"
                PropertyChanges {
                    footer.y: 0
                    footer.visible: true
                }
            }
        ]

        transitions: [
            Transition {
                from: "visible"
                to: "hidden"

                SequentialAnimation {
                    ParallelAnimation {
                        NumberAnimation {
                            target: footer
                            property: "y"
                            duration: Kirigami.Units.shortDuration
                            easing.type: Easing.Linear
                        }
                    }
                    PropertyAction {
                        target: footer
                        property: "visible"
                        value: false
                    }
                }
            },
            Transition {
                from: "hidden"
                to: "visible"

                SequentialAnimation {
                    PropertyAction {
                        target: footer
                        property: "visible"
                        value: true
                    }
                    ParallelAnimation {
                        NumberAnimation {
                            target: footer
                            property: "y"
                            duration: Kirigami.Units.shortDuration
                            easing.type: Easing.Linear
                        }
                    }
                }
            }
        ]
    }
}
