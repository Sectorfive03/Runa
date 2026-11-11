/*
 * SPDX-FileCopyrightText: 2022 George Florea Bănuș <georgefb899@gmail.com>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

pragma ComponentBehavior: Bound

import QtQuick
import Qt.labs.platform as Labs

import org.kde.ki18n

import win.sectorfive.runaplayer

Labs.Menu {
    id: root

    title: KI18n.i18nc("@title:menu", "&File")

    Labs.MenuItem {
        icon.name: RunaplayerApp.actions.openFileAction.icon.name
        text: RunaplayerApp.actions.openFileAction.text
        onTriggered: RunaplayerApp.actions.openFileAction.trigger()
    }

    Labs.MenuItem {
        icon.name: RunaplayerApp.actions.openFolderAction.icon.name
        text: RunaplayerApp.actions.openFolderAction.text
        onTriggered: RunaplayerApp.actions.openFolderAction.trigger()
    }

    Labs.MenuItem {
        icon.name: RunaplayerApp.actions.openUrlAction.icon.name
        text: RunaplayerApp.actions.openUrlAction.text
        onTriggered: RunaplayerApp.actions.openUrlAction.trigger()
    }

    Labs.MenuItem {
        icon.name: RunaplayerApp.actions.loadLastPlayedFileAction.icon.name
        text: RunaplayerApp.actions.loadLastPlayedFileAction.text
        onTriggered: RunaplayerApp.actions.loadLastPlayedFileAction.trigger()
    }

    Labs.Menu {
        id: recentFilesMenu

        title: KI18n.i18nc("@title:menu", "Recent Files")
        onAboutToShow: recentFilesInstantiator.model = Models.recentFilesModel

        Instantiator {
            id: recentFilesInstantiator

            model: 0
            asynchronous: true
            onObjectAdded: (index, object) => recentFilesMenu.insertItem( index, object )
            onObjectRemoved: (index, object) => recentFilesMenu.removeItem( object )
            delegate: Labs.MenuItem {
                id: delegate

                required property string url
                required property string filename
                required property int openedFrom

                text: openedFrom === RecentFilesModel.OpenedFrom.OpenAction
                      || openedFrom === RecentFilesModel.OpenedFrom.ExternalApp
                      ? `<strong>${delegate.filename}</strong>`
                      : delegate.filename
                onTriggered: {
                    recentFilesMenu.close()
                    RunaplayerApp.openRecentFile(delegate.url)
                }
            }
        }
        Labs.MenuSeparator {}
        Labs.MenuItem {
            text: KI18n.i18nc("@action:inmenu", "Clear List")
            onTriggered: Models.recentFilesModel.deleteEntries()
        }
    }


    Labs.MenuSeparator {}

    Labs.MenuItem {
        icon.name: RunaplayerApp.actions.quitApplicationAction.icon.name
        text: RunaplayerApp.actions.quitApplicationAction.text
        onTriggered: RunaplayerApp.actions.quitApplicationAction.trigger()
    }
}
