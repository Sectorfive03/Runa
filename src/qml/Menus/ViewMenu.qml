/*
 * SPDX-FileCopyrightText: 2020 George Florea Bănuș <georgefb899@gmail.com>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

import QtQuick
import QtQuick.Controls

import org.kde.ki18n

import win.sectorfive.runaplayer

Menu {
    id: root

    title: KI18n.i18nc("@title:menu", "&View")

    MenuItem { action: RunaplayerApp.actions.toggleFullscreenAction }

    MenuItem {
        action: RunaplayerApp.actions.toggleMenuBarAction
        text: {
            const mainWindow = Window.window as Main
            if (mainWindow?.menuBar.visible) {
                return KI18n.i18nc("@action:inmenu", "Hide Menubar")
            } else {
                return KI18n.i18nc("@action:inmenu", "Show Menubar")
            }
        }
    }

}
