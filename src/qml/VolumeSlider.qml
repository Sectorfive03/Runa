/*
 * SPDX-FileCopyrightText: 2023 George Florea Bănuș <georgefb899@gmail.com>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

import QtQuick
import QtQuick.Controls

import org.kde.kirigami as Kirigami
import win.sectorfive.runaplayer
import win.sectorfive.runaplayer.settings

Slider {
    id: root

    required property VlcItem m_mpv

    from: 0
    to: root.m_mpv.volumeMax
    value: root.m_mpv.volume
    implicitWidth: 100
    implicitHeight: 25
    wheelEnabled: true
    stepSize: AudioSettings.volumeStep
    leftPadding: 0
    rightPadding: 0

    handle: Item { visible: false }
    background: Rectangle {
        color: "transparent"
        scale: root.mirrored ? -1 : 1

        Rectangle {
            anchors.verticalCenter: parent.verticalCenter
            width: parent.width
            height: 5
            radius: height / 2
            color: Qt.alpha(Kirigami.Theme.textColor, 0.16)
        }

        Rectangle {
            width: root.position * parent.width
            height: 5
            radius: height / 2
            anchors.verticalCenter: parent.verticalCenter
            color: RunaplayerApp.accentColor
        }
    }

    onPressedChanged: {
        root.m_mpv.volume = value.toFixed(0)
    }

    onValueChanged: {
        if (!root.m_mpv.isReady) {
            return
        }

        root.m_mpv.volume = value.toFixed(0)
    }
}
