/*
 * SPDX-FileCopyrightText: 2020 George Florea Bănuș <georgefb899@gmail.com>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

pragma ComponentBehavior: Bound

import QtQml
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import org.kde.ki18n
import org.kde.kirigami as Kirigami

import win.sectorfive.runaplayer
import win.sectorfive.runaplayer.utilities
import win.sectorfive.runaplayer.settings

RowLayout {
    id: root

    required property VlcVideo m_mpv

    property alias loopIndicator: loopIndicator
    property alias chaptersPopupIsOpen: chaptersPopup.visible
    property bool showChapterMarkers: GeneralSettings.showChapterMarkers
                                      && m_mpv.chaptersModel.rowCount * 20 < width

    function openChapterPopup(triggerItem: Item) {
        const x = (chaptersPopup.width + triggerItem.width) * -0.5
        const globalPoint = root.mapToGlobal(x, 0)
        const localPoint = root.mapFromGlobal(Kirigami.Units.largeSpacing, 0)
        chaptersPopup.x = globalPoint.x < 0 ? localPoint.x : x
        chaptersPopup.open()
    }

    spacing: Kirigami.Units.smallSpacing

    Slider {
        id: slider

        property bool seekStarted: false
        property bool prePressPause: root.m_mpv.pause
        property double mouseX: 0

        function updateTooltip() : string {
            if (progressBarBG.width === 0) {
                return
            }

            slider.mouseX = progressBarMouseArea.mouseX
            const time = progressBarMouseArea.mouseX / progressBarBG.width * slider.to
            const chapterTitle = root.m_mpv.chaptersModel.titleAtPosition(time)
            const formattedTime = MiscUtils.formatTime(time)
            timePositionLabel.text = chapterTitle === ""
                    ? formattedTime
                    :`${formattedTime} - ${chapterTitle}`
        }

        from: 0
        to: root.m_mpv.duration
        implicitWidth: root.width
        implicitHeight: root.height
        leftPadding: 0
        rightPadding: 0

        Layout.fillWidth: true

        handle: Item { visible: false }

        background: Rectangle {
            id: progressBarBG
            color: "transparent"
            scale: slider.mirrored ? -1 : 1

            Rectangle {
                id: progressTrack

                anchors.verticalCenter: parent.verticalCenter
                width: parent.width
                height: 6
                radius: height / 2
                color: Qt.alpha(Kirigami.Theme.textColor, 0.16)
            }

            Rectangle {
                id: loopIndicator
                property double startPosition: -1
                property double endPosition: -1
                width: endPosition === -1 ? 1 : (endPosition / root.m_mpv.duration * progressBarBG.width) - x
                height: progressTrack.height
                radius: progressTrack.radius
                anchors.verticalCenter: parent.verticalCenter
                color: Qt.hsla(0, 0, 0, 0.45)
                visible: startPosition !== -1
                x: startPosition / root.m_mpv.duration * progressBarBG.width
                z: 110
            }

            Rectangle {
                width: slider.position * parent.width
                height: progressTrack.height
                radius: progressTrack.radius
                anchors.verticalCenter: parent.verticalCenter
                color: RunaplayerApp.accentColor
            }

            Rectangle {
                id: progressBarToolTip

                property int margin: Kirigami.Units.smallSpacing

                x: {
                    if (slider.pressed) {
                        const xPos = (slider.value / slider.to) * slider.width
                        slider.mouseX = xPos
                        return xPos - (progressBarToolTip.width * 0.5)
                    }

                    return slider.mouseX - (progressBarToolTip.width * 0.5)
                }
                y: -height - Kirigami.Units.largeSpacing
                z: 10
                width: layout.width + (margin * 2)
                height: layout.height + (margin * 2)
                visible: progressBarMouseArea.containsMouse && root.m_mpv.duration > 0

                color: Kirigami.Theme.alternateBackgroundColor
                radius: Kirigami.Units.cornerRadius
                border.width: 1
                border.color: Qt.rgba(1, 1, 1, 0.12)

                ColumnLayout {
                    id: layout

                    anchors.centerIn: parent

                    Label {
                        id: timePositionLabel

                        elide: Text.ElideRight
                        Kirigami.Theme.colorSet: Kirigami.Theme.Tooltip
                        Layout.alignment: Qt.AlignCenter
                    }
                }
            }

            MouseArea {
                id: progressBarMouseArea

                anchors.fill: parent
                hoverEnabled: true
                acceptedButtons: Qt.MiddleButton | Qt.RightButton

                onClicked: function(mouse) {
                    if (mouse.button === Qt.MiddleButton) {
                        if (!GeneralSettings.showChapterMarkers) {
                            return
                        }

                        const time = mouseX / progressBarBG.width * slider.to
                        const nextChapter = root.m_mpv.chaptersModel.indexAtPosition(time)
                        if (nextChapter >= 0) {
                            root.m_mpv.chapter = nextChapter
                        }
                    }
                    if (mouse.button === Qt.RightButton && root.m_mpv.chaptersModel.rowCount > 0) {
                        chaptersPopup.x = mouseX - chaptersPopup.width * 0.5
                        chaptersPopup.open()
                    }
                }

                onMouseXChanged: {
                    slider.updateTooltip()
                }

                onWheel: function(wheel) {
                    if (wheel.angleDelta.y > 0) {
                        RunaplayerApp.actions.seekForwardMediumAction.trigger()
                    } else if (wheel.angleDelta.y) {
                        RunaplayerApp.actions.seekBackwardMediumAction.trigger()
                    }
                }
            }
        }

        onToChanged: {
            value = root.m_mpv.position
            slider.updateTooltip()
        }

        onMoved: {
            if (!pressed) {
                return
            }

            root.m_mpv.seekAbsolute(slider.value)
            const chapterTitle = root.m_mpv.chaptersModel.titleAtPosition(slider.value)
            const formattedTime = MiscUtils.formatTime(slider.value)
            timePositionLabel.text = chapterTitle === ""
                    ? formattedTime
                    :`${formattedTime} - ${chapterTitle}`
        }

        onPressedChanged: {
            if (pressed) {
                slider.prePressPause = root.m_mpv.pause
                root.m_mpv.pause = true
            } else {
                root.m_mpv.pause = slider.prePressPause
            }
        }

        // create markers for the chapters
        Repeater {
            id: chaptersInstantiator

            model: root.showChapterMarkers
                   ? root.m_mpv.chaptersModel
                   : 0
            delegate: Rectangle {
                required property double startTime

                x: slider.mirrored
                   ? progressBarBG.width - (startTime / root.m_mpv.duration * progressBarBG.width)
                   : startTime / root.m_mpv.duration * progressBarBG.width
                y: (progressBarBG.height - height) / 2
                height: 6
                width: 2
                radius: 1
                color: Qt.alpha(Kirigami.Theme.textColor, 0.55)
            }
        }

        Popup {
            id: chaptersPopup

            property int itemHeight
            property int itemBiggestWidth: 1
            property int maxWidth: Math.max(Kirigami.Units.gridUnit * 40, root.width * 0.8)

            y: -height - slider.height
            z: 20
            width: Math.min(itemBiggestWidth, maxWidth)
            height: Math.min(root.m_mpv.height, listViewContainer.contentHeight)
            modal: true
            padding: 0
            onOpened: {
                listView.positionViewAtIndex(root.m_mpv.chapter, ListView.Beginning)
            }

            ScrollView {
                id: listViewContainer

                anchors.fill: parent

                ListView {
                    id: listView

                    clip: true
                    footerPositioning: ListView.OverlayFooter
                    model: root.m_mpv.chaptersModel
                    reuseItems: true
                    delegate: RadioDelegate {
                        id: menuitem

                        required property int index
                        required property string title
                        required property double startTime

                        text: `${MiscUtils.formatTime(menuitem.startTime)} - ${menuitem.title}`
                        checked: menuitem.index === root.m_mpv.chapter
                        width: ListView.view.width
                        onClicked: {
                            chaptersPopup.close()
                            root.m_mpv.chapter = menuitem.index
                        }
                        onImplicitWidthChanged: {
                            let popup = chaptersPopup
                            popup.itemBiggestWidth = Math.max(implicitWidth + 50, popup.width)
                            popup.itemHeight = height
                        }
                    }
                    footer: ToolBar {
                        z: 100
                        width: parent.width
                        height: 26
                        CheckBox {
                            id: skipChaptersCheckBox

                            anchors.verticalCenter: parent.verticalCenter
                            text: KI18n.i18nc("@action:inmenu", "Skip Chapters")
                            checked: PlaybackSettings.skipChapters
                            onCheckedChanged: {
                                PlaybackSettings.skipChapters = checked
                                PlaybackSettings.save()
                            }

                            ToolTip {
                                text: KI18n.i18nc("@info:tooltip", "Automatically skips chapters containing certain words/characters.\nCheck “Playback” settings for more details.")
                                delay: RunaplayerApp.isAltKeyPressed ? 0 : Kirigami.Units.toolTipDelay
                                visible: skipChaptersCheckBox.hovered
                                         && (GeneralSettings.showExplanatoryToolTips || RunaplayerApp.isAltKeyPressed)
                            }
                        }
                    }
                }
            }
        }

        Connections {
            target: root.m_mpv
            function onFileStarted() {
                loopIndicator.startPosition = -1
                loopIndicator.endPosition = -1
            }
            function onPositionChanged() {
                if (!slider.seekStarted) {
                    slider.value = root.m_mpv.position
                }
            }
        }
    }
}
