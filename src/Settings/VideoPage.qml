/*
 * SPDX-FileCopyrightText: 2020 George Florea Bănuș <georgefb899@gmail.com>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

import QtCore
import QtQuick
import QtQuick.Layouts
import QtQuick.Controls
import QtQuick.Dialogs

import org.kde.ki18n
import org.kde.kirigami as Kirigami

import win.sectorfive.runaplayer
import win.sectorfive.runaplayer.settings

SettingsBasePage {
    id: root

    required property VlcVideo m_mpv

    GridLayout {
        id: content

        columns: 2

        Label {
            text: KI18n.i18nc("@label:listbox", "Default cover")
            Layout.alignment: Qt.AlignRight
        }

        RowLayout {
            TextField {
                id: defaultCover

                text: VideoSettings.defaultCover
                placeholderText: KI18n.i18nc("placeholder text", "path to image")
                Layout.fillWidth: true
                onEditingFinished: save()

                Connections {
                    target: root
                    function onSave() {
                        defaultCover.save()
                    }
                }

                ToolTip {
                    text: KI18n.i18nc("@info:tooltip", "Used for music files that don’t have a video track, "
                                + "an embedded cover image or a cover/folder image "
                                + "in the same folder as the played file.")
                    delay: RunaplayerApp.isAltKeyPressed ? 0 : Kirigami.Units.toolTipDelay
                    visible: defaultCover.hovered
                             && (GeneralSettings.showExplanatoryToolTips || RunaplayerApp.isAltKeyPressed)
                }

                function save() : void {
                    VideoSettings.defaultCover = defaultCover.text
                    VideoSettings.save()
                }
            }

            Button {
                icon.name: "folder"
                onClicked: fileDialog.open()
            }

            FileDialog {
                id: fileDialog

                currentFolder: StandardPaths.writableLocation(StandardPaths.PicturesLocation)
                title: KI18n.i18nc("@title:window", "Select file")
                fileMode: FileDialog.OpenFile

                onAccepted: {
                    VideoSettings.defaultCover = fileDialog.selectedFile
                }
                onRejected: root.m_mpv.focus = true
                onVisibleChanged: {
                    RunaplayerApp.actionsEnabled = !visible
                }
            }
        }

        Kirigami.ListSectionHeader {
            text: KI18n.i18nc("@title", "Screenshots")
            Layout.columnSpan: 2
            Layout.fillWidth: true
        }

        // ------------------------------------
        // Screenshot Format
        // ------------------------------------
        Label {
            text: KI18n.i18nc("@label:listbox", "Format")
            Layout.alignment: Qt.AlignRight
        }

        ComboBox {
            id: screenshotFormat
            textRole: "key"
            model: ListModel {
                ListElement { key: "PNG"; value: "png" }
                ListElement { key: "JPG"; value: "jpg" }
                ListElement { key: "WebP"; value: "webp" }
                ListElement { key: "JPEG XL"; value: "jxl" }
            }

            onActivated: function(index) {
                VideoSettings.screenshotFormat = model.get(index).value
                VideoSettings.save()
            }

            Component.onCompleted: {
                if (VideoSettings.screenshotFormat === "png") {
                    currentIndex = 0
                }
                if (VideoSettings.screenshotFormat === "jpg") {
                    currentIndex = 1
                }
                if (VideoSettings.screenshotFormat === "webp") {
                    currentIndex = 2
                }
                if (VideoSettings.screenshotFormat === "jxl") {
                    currentIndex = 3
                }
            }
        }

        // ------------------------------------
        // Screenshot template
        // ------------------------------------
        Label {
            text: KI18n.i18nc("@label:textbox", "Template")
            Layout.alignment: Qt.AlignRight
        }

        RowLayout {
            TextField {
                id: screenshotTemplate

                text: VideoSettings.screenshotTemplate
                Layout.fillWidth: true
                onEditingFinished: save()

                Connections {
                    target: root
                    function onSave() {
                        screenshotTemplate.save()
                    }
                }

                function save() : void {
                    VideoSettings.screenshotTemplate = text
                    VideoSettings.save()
                }
            }

            ToolTipButton {
                toolTipWidth: 480
                toolTipHeight: 180
                toolTipText: KI18n.i18nc("@info:tooltip",
                                   "Specify the filename template used to save screenshots. The template specifies the " +
                                   "filename without file extension. By default, the template is “%F-%n”.<br>" +
                                   "The template can start with a relative or absolute path, in order to specify " +
                                   "a directory location where screenshots should be saved.<br><br>" +
                                   "<strong>Allowed format specifiers:</strong><br>" +
                                   "<ul><li><strong>%F</strong> Filename of the currently played video, without extension.<br>" +
                                   "<li><strong>%n</strong> A sequence number. If the target file already exists, the next free number is used.<br>" +
                                   "<li><strong>%%</strong> Replaced with the % character itself.</li></ul>")
            }
        }

        Kirigami.ListSectionHeader {
            text: KI18n.i18nc("@title", "Image adjustments")
            Layout.columnSpan: 2
            Layout.fillWidth: true
        }


        // ------------------------------------
        // CONTRAST
        // ------------------------------------
        Label {
            text: KI18n.i18nc("@label:slider", "Contrast")
            Layout.alignment: Qt.AlignRight
        }

        ImageAdjustmentSlider {
            id: contrastSlider

            value: root.m_mpv.imageAdjustment("contrast")
            onSliderValueChanged: function(value) {
                root.m_mpv.setImageAdjustment("contrast", value)
            }

            Layout.topMargin: Kirigami.Units.largeSpacing
        }

        // ------------------------------------
        // BRIGHTNESS
        // ------------------------------------
        Label {
            text: KI18n.i18nc("@label:slider", "Brightness")
            Layout.alignment: Qt.AlignRight
        }

        ImageAdjustmentSlider {
            id: brightnessSlider

            value: root.m_mpv.imageAdjustment("brightness")
            onSliderValueChanged: function(value) {
                root.m_mpv.setImageAdjustment("brightness", value)
            }

            Layout.topMargin: Kirigami.Units.largeSpacing
        }

        // ------------------------------------
        // GAMMA
        // ------------------------------------
        Label {
            text: KI18n.i18nc("@label:slider", "Gamma")
            Layout.alignment: Qt.AlignRight
        }

        ImageAdjustmentSlider {
            id: gammaSlider

            value: root.m_mpv.imageAdjustment("gamma")
            onSliderValueChanged: function(value) {
                root.m_mpv.setImageAdjustment("gamma", value)
            }

            Layout.topMargin: Kirigami.Units.largeSpacing
        }

        // ------------------------------------
        // SATURATION
        // ------------------------------------
        Label {
            text: KI18n.i18nc("@label:slider", "Saturation")
            Layout.alignment: Qt.AlignRight
        }

        ImageAdjustmentSlider {
            id: saturationSlider

            value: root.m_mpv.imageAdjustment("saturation")
            onSliderValueChanged: function(value) {
                root.m_mpv.setImageAdjustment("saturation", value)
            }

            Layout.topMargin: Kirigami.Units.largeSpacing
        }

        Label {
            text: KI18n.i18nc("@info", "Middle click on the sliders to reset them")
            Layout.columnSpan: 2
            Layout.topMargin: Kirigami.Units.largeSpacing
        }

    }
}
