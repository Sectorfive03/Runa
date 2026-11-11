/*
 * SPDX-FileCopyrightText: 2022 George Florea Bănuș <georgefb899@gmail.com>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

import QtQuick
import Qt.labs.platform as Labs

import org.kde.ki18n

import win.sectorfive.runaplayer
import win.sectorfive.runaplayer.settings

Labs.Menu {
    id: root

    title: KI18n.i18nc("@title:menu", "&View")

    Labs.MenuItem {
        icon.name: RunaplayerApp.actions.toggleFullscreenAction.icon.name
        text: RunaplayerApp.actions.toggleFullscreenAction.text
        onTriggered: RunaplayerApp.actions.toggleFullscreenAction.trigger()
    }

    Labs.MenuItem {
        icon.name: RunaplayerApp.actions.toggleMenuBarAction.icon.name
        text: {
            if (GeneralSettings.showMenuBar) {
                return KI18n.i18nc("@action:inmenu", "Hide Menubar")
            } else {
                return KI18n.i18nc("@action:inmenu", "Show Menubar")
            }
        }
        onTriggered: RunaplayerApp.actions.toggleMenuBarAction.trigger()
    }

}
