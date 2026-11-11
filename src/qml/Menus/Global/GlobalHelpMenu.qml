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

    title: KI18n.i18nc("@title:menu", "&Help")

    Labs.MenuItem {
        icon.name: RunaplayerApp.actions.aboutRunaplayerAction.icon.name
        icon.source: "qrc:/icons/22-apps-runa.png"
        text: RunaplayerApp.actions.aboutRunaplayerAction.text
        onTriggered: RunaplayerApp.actions.aboutRunaplayerAction.trigger()
    }
    Labs.MenuItem {
        icon.name: RunaplayerApp.actions.reportBugAction.icon.name
        text: RunaplayerApp.actions.reportBugAction.text
        onTriggered: RunaplayerApp.actions.reportBugAction.trigger()
    }
    Labs.MenuItem {
        icon.name: RunaplayerApp.actions.sponsorAction.icon.name
        text: RunaplayerApp.actions.sponsorAction.text
        onTriggered: RunaplayerApp.actions.sponsorAction.trigger()
    }
}
