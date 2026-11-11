/*
 * SPDX-FileCopyrightText: 2022 Kartikey Subramanium <kartikey@tutanota.com>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls

import org.kde.ki18n
import org.kde.kirigami as Kirigami

import win.sectorfive.runaplayer

ToolButton {
    id: root

    required property VlcVideo m_mpv
    required property MenuBarLoader m_menuBarLoader

    property bool isOpen: false

    icon.name: "application-menu"
    focusPolicy: Qt.NoFocus

    onReleased: {
        root.isOpen = !root.isOpen
    }

    Loader {
        id: menuLoader

        asynchronous: true
        active: root.m_menuBarLoader.state === "hidden"
        sourceComponent: Menu {
            id: menu

            parent: root
            y: -height - Kirigami.Units.largeSpacing
            visible:  root.isOpen
            closePolicy: Popup.CloseOnReleaseOutside
            onClosed: root.isOpen = menu.visible
            modal: true

            MenuItem {
                action: RunaplayerApp.actions.openFileAction
            }
            MenuItem {
                action: RunaplayerApp.actions.openFolderAction
            }
            MenuItem {
                action: RunaplayerApp.actions.openUrlAction
            }

            Menu {
                title: KI18n.i18nc("@title:menu", "Recent Files")

                Repeater {
                    model: Models.recentFilesModel
                    delegate: MenuItem {
                        id: delegate

                        required property string url
                        required property string filename
                        required property int openedFrom

                        text: openedFrom === RecentFilesModel.OpenedFrom.OpenAction
                              || openedFrom === RecentFilesModel.OpenedFrom.ExternalApp
                              ? `<strong>${delegate.filename}</strong>`
                              : delegate.filename

                        onClicked: {
                            const mainWindow = Window.window as Main
                            mainWindow.openFile(delegate.url, RecentFilesModel.OpenedFrom.Other)
                        }
                    }
                }
            }

            SubtitleTracksMenu {
                id: primarySubtitlesMenu

                m_mpv: root.m_mpv
                title: KI18n.i18nc("@title:menu", "Primary Subtitle")
            }


            AudioTracksMenu {
                id: audioMenu

                m_mpv: root.m_mpv

                title: KI18n.i18nc("@title:menu", "Audio Track")
                model: m_mpv.audioTracksModel
                enabled: m_mpv.audioTracksModel.rowCount > 0
            }

            MenuSeparator {}

            MenuItem { action: RunaplayerApp.actions.toggleFullscreenAction }
            MenuItem { action: RunaplayerApp.actions.toggleDeinterlacingAction }
            MenuItem { action: RunaplayerApp.actions.screenshotAction }

            MenuSeparator {}

            MenuItem {
                action: RunaplayerApp.actions.configureAction
            }
            MenuItem { action: RunaplayerApp.actions.configureShortcutsAction }
            MenuItem {
                action: RunaplayerApp.actions.aboutRunaplayerAction
                icon.source: "qrc:/icons/22-apps-runa.png"
            }

            MenuSeparator {}

            Menu {
                title: KI18n.i18nc("@title:menu", "&More")

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
                HelpMenu {}
            }
        }
    }

}
