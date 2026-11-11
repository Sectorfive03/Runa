/*
 * SPDX-FileCopyrightText: 2021 George Florea Bănuș <georgefb899@gmail.com>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

import QtQuick.Controls

import org.kde.ki18n

import win.sectorfive.runaplayer

Menu {
    id: root

    title: KI18n.i18nc("@title:menu", "&Video")

    MenuItem { action: RunaplayerApp.actions.toggleDeinterlacingAction }
    MenuItem { action: RunaplayerApp.actions.screenshotAction }

    Menu {
        title: KI18n.i18nc("@title:menu", "Adjustments")

        MenuItem { action: RunaplayerApp.actions.contrastUpAction }
        MenuItem { action: RunaplayerApp.actions.contrastDownAction }
        MenuItem { action: RunaplayerApp.actions.contrastResetAction }
        MenuItem { action: RunaplayerApp.actions.brightnessUpAction }
        MenuItem { action: RunaplayerApp.actions.brightnessDownAction }
        MenuItem { action: RunaplayerApp.actions.brightnessResetAction }
        MenuItem { action: RunaplayerApp.actions.gammaUpAction }
        MenuItem { action: RunaplayerApp.actions.gammaDownAction }
        MenuItem { action: RunaplayerApp.actions.gammaResetAction }
        MenuItem { action: RunaplayerApp.actions.saturationUpAction }
        MenuItem { action: RunaplayerApp.actions.saturationDownAction }
        MenuItem { action: RunaplayerApp.actions.saturationResetAction }
    }
}
