/*
 * SPDX-FileCopyrightText: 2023 George Florea Bănuș <georgefb899@gmail.com>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls

import win.sectorfive.runaplayer

Menu {
    id: root

    required property VlcVideo m_mpv

    property TracksModel model: m_mpv.subtitleTracksModel

    enabled: model.rowCount > 1

    contentItem: ListView {
        property bool hasCheckables: false
        property bool hasIcons: false

        model: root.model
        implicitWidth: contentItem.children.reduce((maxWidth, child) => Math.max(maxWidth, child.implicitWidth), 0)
        implicitHeight: Math.max(1, contentItem.childrenRect.height)
        spacing: 0
        clip: true
        delegate: MenuItem {
            id: delegate

            required property int trackId
            required property string displayText
            required property string language
            required property string title
            required property string codec

            checkable: true
            checked: delegate.trackId === root.m_mpv.subtitleId
            text: delegate.displayText
            onTriggered: {
                root.m_mpv.subtitleId = delegate.trackId === root.m_mpv.subtitleId ? -1 : delegate.trackId
            }
        }
    }
}
