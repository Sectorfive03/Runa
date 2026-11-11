/*
 * SPDX-FileCopyrightText: 2020 George Florea Bănuș <georgefb899@gmail.com>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

import QtQuick
import QtQuick.Layouts
import QtQuick.Controls

import org.kde.ki18n
import org.kde.kirigami as Kirigami

import win.sectorfive.runaplayer
import win.sectorfive.runaplayer.settings

SettingsBasePage {
    id: root

    GridLayout {
        id: content

        columns: 2

        Label {
            text: KI18n.i18nc("@label:textbox", "Preferred language")
            Layout.alignment: Qt.AlignRight
        }

        RowLayout {
            TextField {
                id: preferredLanguage

                text: AudioSettings.preferredLanguage
                placeholderText: "eng, ger etc."
                onEditingFinished: save()

                Connections {
                    target: root
                    function onSave() {
                        preferredLanguage.save()
                    }
                }

                function save() {
                    AudioSettings.preferredLanguage = text
                    AudioSettings.save()
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
            text: KI18n.i18nc("@label:spinbox", "Preferred track id")
            Layout.alignment: Qt.AlignRight
        }

        RowLayout {
            SpinBox {
                from: 0
                to: 100
                value: AudioSettings.preferredTrack
                editable: true
                onValueModified: {
                    AudioSettings.preferredTrack = value
                    AudioSettings.save()
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

        // Volume Step
        Label {
            text: KI18n.i18nc("@label:spinbox", "Volume step")
            Layout.alignment: Qt.AlignRight
        }

        SpinBox {
            id: volumeStep
            editable: true
            from: 0
            to: 100
            value: AudioSettings.volumeStep
            onValueModified: {
                AudioSettings.volumeStep = volumeStep.value
                AudioSettings.save()
            }
        }

        Item {
            Layout.preferredWidth: Kirigami.Units.gridUnit
            Layout.preferredHeight: Kirigami.Units.gridUnit
        }
    }
}
