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

    title: KI18n.i18nc("@title:menu", "&Help")

    MenuItem {
        action: RunaplayerApp.actions.aboutRunaplayerAction
        icon.source: "qrc:/icons/22-apps-runa.png"
    }
    MenuItem { action: RunaplayerApp.actions.reportBugAction }
    MenuItem { action: RunaplayerApp.actions.sponsorAction }
}
