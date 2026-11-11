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

    title: KI18n.i18nc("@title:menu", "&Video")

    Labs.MenuItem {
        icon.name: RunaplayerApp.actions.toggleDeinterlacingAction.icon.name
        text: RunaplayerApp.actions.toggleDeinterlacingAction.text
        onTriggered: RunaplayerApp.actions.toggleDeinterlacingAction.trigger()
    }

    Labs.MenuItem {
        icon.name: RunaplayerApp.actions.screenshotAction.icon.name
        text: RunaplayerApp.actions.screenshotAction.text
        onTriggered: RunaplayerApp.actions.screenshotAction.trigger()
    }

    Labs.Menu {
        title: KI18n.i18nc("@title:menu", "Adjustments")

        Labs.MenuItem {
            icon.name: RunaplayerApp.actions.contrastUpAction.icon.name
            text: RunaplayerApp.actions.contrastUpAction.text
            onTriggered: RunaplayerApp.actions.contrastUpAction.trigger()
        }

        Labs.MenuItem {
            icon.name: RunaplayerApp.actions.contrastDownAction.icon.name
            text: RunaplayerApp.actions.contrastDownAction.text
            onTriggered: RunaplayerApp.actions.contrastDownAction.trigger()
        }

        Labs.MenuItem {
            icon.name: RunaplayerApp.actions.contrastResetAction.icon.name
            text: RunaplayerApp.actions.contrastResetAction.text
            onTriggered: RunaplayerApp.actions.contrastResetAction.trigger()
        }

        Labs.MenuItem {
            icon.name: RunaplayerApp.actions.brightnessUpAction.icon.name
            text: RunaplayerApp.actions.brightnessUpAction.text
            onTriggered: RunaplayerApp.actions.brightnessUpAction.trigger()
        }

        Labs.MenuItem {
            icon.name: RunaplayerApp.actions.brightnessDownAction.icon.name
            text: RunaplayerApp.actions.brightnessDownAction.text
            onTriggered: RunaplayerApp.actions.brightnessDownAction.trigger()
        }

        Labs.MenuItem {
            icon.name: RunaplayerApp.actions.brightnessResetAction.icon.name
            text: RunaplayerApp.actions.brightnessResetAction.text
            onTriggered: RunaplayerApp.actions.brightnessResetAction.trigger()
        }

        Labs.MenuItem {
            icon.name: RunaplayerApp.actions.gammaUpAction.icon.name
            text: RunaplayerApp.actions.gammaUpAction.text
            onTriggered: RunaplayerApp.actions.gammaUpAction.trigger()
        }

        Labs.MenuItem {
            icon.name: RunaplayerApp.actions.gammaDownAction.icon.name
            text: RunaplayerApp.actions.gammaDownAction.text
            onTriggered: RunaplayerApp.actions.gammaDownAction.trigger()
        }

        Labs.MenuItem {
            icon.name: RunaplayerApp.actions.gammaResetAction.icon.name
            text: RunaplayerApp.actions.gammaResetAction.text
            onTriggered: RunaplayerApp.actions.gammaResetAction.trigger()
        }

        Labs.MenuItem {
            icon.name: RunaplayerApp.actions.saturationUpAction.icon.name
            text: RunaplayerApp.actions.saturationUpAction.text
            onTriggered: RunaplayerApp.actions.saturationUpAction.trigger()
        }
        Labs.MenuItem {
            icon.name: RunaplayerApp.actions.saturationDownAction.icon.name
            text: RunaplayerApp.actions.saturationDownAction.text
            onTriggered: RunaplayerApp.actions.saturationDownAction.trigger()
        }
        Labs.MenuItem {
            icon.name: RunaplayerApp.actions.saturationResetAction.icon.name
            text: RunaplayerApp.actions.saturationResetAction.text
            onTriggered: RunaplayerApp.actions.saturationResetAction.trigger()
        }
    }
}
