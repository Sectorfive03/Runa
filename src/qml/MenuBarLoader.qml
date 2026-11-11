/*
 * SPDX-FileCopyrightText: 2023 George Florea Bănuș <georgefb899@gmail.com>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls as QQC2
import Qt.labs.platform as Platform

import org.kde.ki18n
import org.kde.kirigami as Kirigami

import win.sectorfive.runaplayer
import win.sectorfive.runaplayer.utilities
import win.sectorfive.runaplayer.settings

Loader {
    id: root

    required property VlcVideo m_mpv

    property bool isShowAnimationRunning: false
    property bool showGlobalMenu: SystemUtils.platformName() !== "windows"
                                    && Kirigami.Settings.hasPlatformMenuBar
                                    && !Kirigami.Settings.isMobile

    clip: true
    asynchronous: !GeneralSettings.showMenuBar
    sourceComponent: showGlobalMenu ? globalMenuBarComponent : menuBarComponent

    Component {
        id: menuBarItemDelegate

        QQC2.MenuBarItem {
            id: menuItem

            hoverEnabled: true

            Kirigami.MnemonicData.enabled: menuItem.enabled && menuItem.visible
            Kirigami.MnemonicData.controlType: Kirigami.MnemonicData.MenuItem
            Kirigami.MnemonicData.label: menuItem.text

            Shortcut {
                enabled: !(RegExp(/\&[^\&]/).test(menuItem.text))
                sequence: menuItem.Kirigami.MnemonicData.sequence
                onActivated: menuItem.clicked()
            }

            contentItem: Text {
                text: menuItem.Kirigami.MnemonicData.richTextLabel
                font: menuItem.font
                color: menuItem.highlighted || menuItem.down || menuItem.hovered ? "#ffffff" : Kirigami.Theme.textColor
                horizontalAlignment: Text.AlignHCenter
                verticalAlignment: Text.AlignVCenter
                elide: Text.ElideRight
            }

            background: Rectangle {
                radius: 4
                color: menuItem.highlighted || menuItem.down
                    ? Qt.rgba(1.0, 0.4, 0.0, 0.85)
                    : menuItem.hovered ? Qt.rgba(1.0, 0.4, 0.0, 0.65) : "transparent"
            }
        }
    }

    Component {
        id: menuBarComponent

        QQC2.MenuBar {
            hoverEnabled: true
            Kirigami.Theme.colorSet: Kirigami.Theme.Header
            delegate: menuBarItemDelegate

            FileMenu {}
            ViewMenu {}
            PlaybackMenu {}
            VideoMenu {}
            SubtitlesMenu {
                m_mpv: root.m_mpv
            }
            AudioMenu {
                m_mpv: root.m_mpv
            }

            // Settings has no drop-down menu anymore: clicking it opens the
            // settings window directly, like the Settings button in the header.
            QQC2.MenuBarItem {
                id: settingsMenuItem

                text: KI18n.i18nc("@title:menu", "&Settings")
                hoverEnabled: true

                Kirigami.MnemonicData.enabled: settingsMenuItem.enabled && settingsMenuItem.visible
                Kirigami.MnemonicData.controlType: Kirigami.MnemonicData.MenuItem
                Kirigami.MnemonicData.label: settingsMenuItem.text

                onClicked: RunaplayerApp.actions.configureAction.trigger()

                contentItem: Text {
                    text: settingsMenuItem.Kirigami.MnemonicData.richTextLabel
                    font: settingsMenuItem.font
                    color: settingsMenuItem.highlighted || settingsMenuItem.down || settingsMenuItem.hovered
                           ? "#ffffff"
                           : Kirigami.Theme.textColor
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                    elide: Text.ElideRight
                }

                background: Rectangle {
                    radius: 4
                    color: settingsMenuItem.highlighted || settingsMenuItem.down
                           ? Qt.rgba(1.0, 0.4, 0.0, 0.85)
                           : settingsMenuItem.hovered ? Qt.rgba(1.0, 0.4, 0.0, 0.65) : "transparent"
                }
            }

            HelpMenu {}
        }
    }

    Component {
        id: globalMenuBarComponent

        Platform.MenuBar {
            GlobalFileMenu {}
            GlobalViewMenu {}
            GlobalPlaybackMenu {}
            GlobalVideoMenu {}
            GlobalSubtitlesMenu {
                m_mpv: root.m_mpv
            }
            GlobalAudioMenu {
                m_mpv: root.m_mpv
            }
            GlobalHelpMenu {}
        }
    }

    states: [
        State {
            name: "hidden"
            PropertyChanges {
                root.height: 0
                root.visible: false
            }
        },
        State {
            name : "visible"
            PropertyChanges {
                root.height: root.implicitHeight
                root.visible: true
            }
        }
    ]

    transitions: [
        Transition {
            from: "visible"
            to: "hidden"

            SequentialAnimation {
                NumberAnimation {
                    target: root
                    property: "height"
                    duration: Kirigami.Units.shortDuration
                    easing.type: Easing.Linear
                }
                PropertyAction {
                    target: root
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
                    target: root
                    property: "isShowAnimationRunning"
                    value: true
                }
                PropertyAction {
                    target: root
                    property: "visible"
                    value: true
                }
                NumberAnimation {
                    target: root
                    property: "height"
                    duration: Kirigami.Units.shortDuration
                    easing.type: Easing.Linear
                }
                PropertyAction {
                    target: root
                    property: "isShowAnimationRunning"
                    value: false
                }
            }
        }
    ]
}
