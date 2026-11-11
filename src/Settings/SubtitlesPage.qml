/*
 * SPDX-FileCopyrightText: 2020 George Florea Bănuș <georgefb899@gmail.com>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Layouts
import QtQuick.Controls

import org.kde.ki18n
import org.kde.kirigami as Kirigami

import win.sectorfive.runaplayer
import win.sectorfive.runaplayer.settings
import win.sectorfive.runaplayer.utilities

SettingsBasePage {
    id: root

    GridLayout {
        id: content

        columns: 2

        SubtitlesFolders {
            id: subtitleFolders
            implicitWidth: root.width
            Layout.fillWidth: true
            Layout.columnSpan: 2
        }

        Item { Layout.preferredWidth: 1; Layout.preferredHeight: 1 }

        CheckBox {
            id: autoSelectSubtitles

            text: KI18n.i18nc("@label:check", "Auto select track")
            checked: SubtitlesSettings.autoSelectSubtitles

            onClicked: {
                SubtitlesSettings.autoSelectSubtitles = checked
                SubtitlesSettings.save()
            }

            ToolTip {
                text: KI18n.i18nc("@info:tooltip", "When checked a subtitle track will be automatically selected and displayed")
                delay: RunaplayerApp.isAltKeyPressed ? 0 : Kirigami.Units.toolTipDelay
                visible: autoSelectSubtitles.hovered
                         && (GeneralSettings.showExplanatoryToolTips || RunaplayerApp.isAltKeyPressed)
            }
        }

        Label {
            text: KI18n.i18nc("@label:textbox", "Preferred language")
            Layout.alignment: Qt.AlignRight
        }

        RowLayout {
            TextField {
                id: preferredLanguage

                text: SubtitlesSettings.preferredLanguage
                placeholderText: "eng, ger etc."
                Layout.fillWidth: true
                onEditingFinished: save()
                enabled: autoSelectSubtitles.checked

                Connections {
                    target: root
                    function onSave() {
                        preferredLanguage.save()
                    }
                }

                function save() : void {
                    SubtitlesSettings.preferredLanguage = text
                    SubtitlesSettings.save()
                }
            }
            ToolTipButton {
                toolTipWidth: 400
                toolTipText: KI18n.i18nc("@info:tooltip",
                                         "Track selection priority:<br>" +
                                         "- preferred track id<br>" +
                                         "- track matching the preferred language<br>" +
                                         "- default track<br>" +
                                         "- no track")
            }
        }

        Label {
            text: KI18n.i18nc("@label:spinbox", "Preferred track")
            Layout.alignment: Qt.AlignRight
        }

        RowLayout {
            SpinBox {
                from: 0
                to: 100
                value: SubtitlesSettings.preferredTrack
                editable: true
                enabled: autoSelectSubtitles.checked
                onValueModified: {
                    SubtitlesSettings.preferredTrack = value
                    SubtitlesSettings.save()
                }
            }
            ToolTipButton {
                toolTipWidth: 400
                toolTipText: KI18n.i18nc("@info:tooltip",
                                         "Track selection priority:<br>" +
                                         "- preferred track id<br>" +
                                         "- track matching the preferred language<br>" +
                                         "- default track<br>" +
                                         "- no track")
            }
        }

        Item {
            Layout.preferredWidth: Kirigami.Units.gridUnit
            Layout.preferredHeight: Kirigami.Units.gridUnit
        }
    }
}
