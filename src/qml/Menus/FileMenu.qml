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
import win.sectorfive.runaplayer.settings

Menu {
    id: root

    title: KI18n.i18nc("@title:menu", "&File")

    MenuItem { action: RunaplayerApp.actions.openFileAction }
    MenuItem { action: RunaplayerApp.actions.openFolderAction }
    MenuItem { action: RunaplayerApp.actions.openUrlAction }
    MenuItem {
        action: RunaplayerApp.actions.loadLastPlayedFileAction
        enabled: GeneralSettings.lastPlayedFile !== ""
    }

    Menu {
        id: recentFilesMenu

        title: KI18n.i18nc("@title:menu", "Recent Files")
        onAboutToShow: instantiator.model = Models.recentFilesModel

        Instantiator {
            id: instantiator

            model: 0
            delegate: MenuItem {
                id: delegate

                required property string url
                required property string toolTipText
                required property string filename
                required property int openedFrom

                text: openedFrom === RecentFilesModel.OpenedFrom.OpenAction
                      || openedFrom === RecentFilesModel.OpenedFrom.ExternalApp
                      ? `<strong>${delegate.filename}</strong>`
                      : delegate.filename

                onClicked: {
                    recentFilesMenu.dismiss()
                    RunaplayerApp.openRecentFile(delegate.url)
                }

                ToolTip {
                    text: delegate.toolTipText
                }
            }
            onObjectAdded: (index, object) => recentFilesMenu.insertItem(index, object)
            onObjectRemoved: (index, object) => recentFilesMenu.removeItem(object)
        }

        MenuSeparator {}
        MenuItem {
            text: KI18n.i18nc("@action:inmenu", "Clear List")
            onClicked: Models.recentFilesModel.deleteEntries()
        }
    }


    MenuSeparator {}

    MenuItem { action: RunaplayerApp.actions.quitApplicationAction }
}
