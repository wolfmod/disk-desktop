/*
 * Диск Ра Град: окно «Файлы, которые не синхронизируются» в стиле мобильного «Диска» (вместо серого IgnoreListEditor):
 * поле «Добавить маску», свои маски (удалить, «Можно удалять»), встроенные правила программы — свёрнутым списком.
 * Данные и сохранение — RagradIgnoreList (C++), файл исключений тот же, что у прежнего окна.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 * or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License
 * for more details.
 */

import QtQuick
// Basic: у поля ввода свой фон (нативный стиль Windows его не даёт настроить)
import QtQuick.Controls.Basic
import QtQuick.Layouts
import eu.OpenCloud.gui 1.0
import eu.OpenCloud.resources 1.0

Pane {
    id: page
    readonly property RagradIgnoreList list: ocContext
    property string addError: ""
    property bool showSystem: false

    padding: 0
    Accessible.role: Accessible.Pane
    Accessible.name: qsTr("Файлы, которые не синхронизируются")

    background: Rectangle {
        color: "#FFFFFF"
    }

    function addPattern() {
        page.addError = page.list.add(patternField.text);
        if (page.addError === "") {
            patternField.text = "";
        }
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        ScrollView {
            id: scroll
            Layout.fillWidth: true
            Layout.fillHeight: true
            contentWidth: availableWidth
            clip: true

            ColumnLayout {
                width: scroll.availableWidth
                spacing: 20

                ColumnLayout {
                    Layout.fillWidth: true
                    Layout.leftMargin: 22
                    Layout.rightMargin: 22
                    Layout.topMargin: 18
                    spacing: 6

                    Label {
                        text: qsTr("Файлы, которые не синхронизируются")
                        font.pixelSize: 20
                        font.weight: Font.Bold
                        color: "#1A1A1A"
                    }
                    Label {
                        Layout.fillWidth: true
                        text: qsTr("Файлы и папки с подходящим именем остаются только на этом компьютере: Диск их не отправляет и не скачивает. Звёздочка * означает любые символы: например, *.tmp — все файлы с расширением .tmp.")
                        font.pixelSize: 13
                        color: "#8F9399"
                        wrapMode: Text.Wrap
                    }
                }

                RagradSection {
                    Layout.leftMargin: 20
                    Layout.rightMargin: 20
                    title: qsTr("Добавить маску")

                    ColumnLayout {
                        Layout.fillWidth: true
                        Layout.margins: 15
                        spacing: 6

                        RowLayout {
                            Layout.fillWidth: true
                            spacing: 10

                            TextField {
                                id: patternField
                                Layout.fillWidth: true
                                implicitHeight: 36
                                leftPadding: 12
                                rightPadding: 12
                                font.pixelSize: 14
                                color: "#1A1A1A"
                                placeholderText: qsTr("Например, *.bak")
                                placeholderTextColor: "#8F9399"
                                selectByMouse: true
                                Accessible.name: qsTr("Новая маска")
                                onAccepted: page.addPattern()
                                onTextChanged: page.addError = ""
                                background: Rectangle {
                                    radius: 12
                                    color: "#FFFFFF"
                                    border.width: patternField.activeFocus ? 2 : 1
                                    border.color: patternField.activeFocus ? "#2F6FEB" : "#DADDE1"
                                }
                            }
                            RagradButton {
                                text: qsTr("Добавить")
                                onClicked: page.addPattern()
                            }
                        }
                        Label {
                            Layout.fillWidth: true
                            visible: page.addError !== ""
                            text: page.addError
                            font.pixelSize: 12
                            color: "#E5484D"
                            wrapMode: Text.Wrap
                        }
                    }
                }

                ColumnLayout {
                    Layout.fillWidth: true
                    Layout.leftMargin: 20
                    Layout.rightMargin: 20
                    spacing: 6

                    RagradSection {
                        title: qsTr("Ваши маски")

                        Label {
                            Layout.fillWidth: true
                            Layout.margins: 16
                            visible: page.list.userPatterns.length === 0
                            text: qsTr("Пока нет своих масок.")
                            font.pixelSize: 14
                            color: "#8F9399"
                        }

                        Repeater {
                            model: page.list.userPatterns

                            delegate: ColumnLayout {
                                id: row
                                required property var modelData
                                required property int index

                                Layout.fillWidth: true
                                spacing: 0

                                RagradDivider {
                                    visible: row.index > 0
                                }
                                RowLayout {
                                    Layout.fillWidth: true
                                    Layout.leftMargin: 16
                                    Layout.rightMargin: 10
                                    Layout.topMargin: 8
                                    Layout.bottomMargin: 8
                                    spacing: 10

                                    ColumnLayout {
                                        Layout.fillWidth: true
                                        spacing: 1
                                        Label {
                                            Layout.fillWidth: true
                                            text: row.modelData.pattern
                                            font.pixelSize: 14
                                            color: "#1A1A1A"
                                            elide: Text.ElideRight
                                        }
                                        Label {
                                            Layout.fillWidth: true
                                            visible: row.modelData.readOnly
                                            text: qsTr("Задана администратором, изменить нельзя.")
                                            font.pixelSize: 12
                                            color: "#8F9399"
                                            wrapMode: Text.Wrap
                                        }
                                    }

                                    // «Можно удалять»: такие файлы удаляются, если мешают удалить папку, удалённую на Диске
                                    AbstractButton {
                                        id: deletableChip
                                        readonly property bool selected: row.modelData.deletable
                                        visible: !row.modelData.readOnly
                                        text: qsTr("Можно удалять")
                                        hoverEnabled: true
                                        focusPolicy: Qt.StrongFocus
                                        implicitHeight: 30
                                        implicitWidth: chipLabel.implicitWidth + 24
                                        Accessible.role: Accessible.CheckBox
                                        Accessible.name: qsTr("Можно удалять: %1").arg(row.modelData.pattern)
                                        Accessible.checkable: true
                                        Accessible.checked: selected
                                        onClicked: page.list.setDeletable(row.index, !selected)

                                        background: Rectangle {
                                            radius: 15
                                            color: deletableChip.selected ? "#EAF2FF" : (deletableChip.hovered ? "#F7F8FA" : "#FFFFFF")
                                            border.width: deletableChip.visualFocus ? 2 : 1
                                            border.color: deletableChip.selected || deletableChip.visualFocus ? "#2F6FEB" : "#DADDE1"
                                        }
                                        contentItem: Label {
                                            id: chipLabel
                                            text: deletableChip.text
                                            font.pixelSize: 12
                                            color: deletableChip.selected ? "#2F6FEB" : "#5B6167"
                                            horizontalAlignment: Text.AlignHCenter
                                            verticalAlignment: Text.AlignVCenter
                                        }
                                    }

                                    AbstractButton {
                                        id: removeButton
                                        visible: !row.modelData.readOnly
                                        hoverEnabled: true
                                        focusPolicy: Qt.StrongFocus
                                        implicitWidth: 32
                                        implicitHeight: 32
                                        Accessible.role: Accessible.Button
                                        Accessible.name: qsTr("Удалить маску %1").arg(row.modelData.pattern)
                                        onClicked: page.list.remove(row.index)

                                        background: Rectangle {
                                            radius: 10
                                            color: removeButton.hovered ? "#FDECEC" : "transparent"
                                            border.width: removeButton.visualFocus ? 2 : 0
                                            border.color: "#2F6FEB"
                                        }
                                        contentItem: Item {
                                            Image {
                                                anchors.centerIn: parent
                                                sourceSize: Qt.size(14, 14)
                                                source: OCUtils.resourcePath("fontawesome", "", true, 0, removeButton.hovered ? "#E5484D" : "#8F9399")
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    }
                    Label {
                        Layout.fillWidth: true
                        Layout.leftMargin: 2
                        visible: page.list.userPatterns.length > 0
                        text: qsTr("«Можно удалять» — если такие файлы мешают удалить папку, которую удалили на Диске, они удаляются вместе с ней.")
                        font.pixelSize: 12
                        color: "#8F9399"
                        wrapMode: Text.Wrap
                    }
                }

                RagradSection {
                    Layout.leftMargin: 20
                    Layout.rightMargin: 20
                    Layout.bottomMargin: 20

                    AbstractButton {
                        id: systemToggle
                        Layout.fillWidth: true
                        hoverEnabled: true
                        focusPolicy: Qt.StrongFocus
                        leftPadding: 16
                        rightPadding: 16
                        topPadding: 12
                        bottomPadding: 12
                        Accessible.role: Accessible.Button
                        Accessible.name: text
                        text: page.showSystem ? qsTr("Скрыть встроенные правила") : qsTr("Встроенные правила программы: %1").arg(page.list.systemPatterns.length)
                        onClicked: page.showSystem = !page.showSystem

                        background: Rectangle {
                            color: systemToggle.hovered ? "#F7F8FA" : "transparent"
                            radius: 13
                            border.width: systemToggle.visualFocus ? 2 : 0
                            border.color: "#2F6FEB"
                        }
                        contentItem: RowLayout {
                            spacing: 16
                            ColumnLayout {
                                Layout.fillWidth: true
                                spacing: 2
                                Label {
                                    Layout.fillWidth: true
                                    text: systemToggle.text
                                    font.pixelSize: 14
                                    color: "#1A1A1A"
                                    wrapMode: Text.Wrap
                                }
                                Label {
                                    Layout.fillWidth: true
                                    text: qsTr("Временные и служебные файлы Windows и программ. Изменить их нельзя.")
                                    font.pixelSize: 12
                                    color: "#8F9399"
                                    wrapMode: Text.Wrap
                                }
                            }
                            Image {
                                Layout.alignment: Qt.AlignVCenter
                                sourceSize: Qt.size(12, 12)
                                source: OCUtils.resourcePath("fontawesome", page.showSystem ? "" : "", true, 0, "#8F9399")
                            }
                        }
                    }

                    Flow {
                        Layout.fillWidth: true
                        Layout.leftMargin: 16
                        Layout.rightMargin: 16
                        Layout.bottomMargin: 14
                        visible: page.showSystem
                        spacing: 6

                        Repeater {
                            model: page.showSystem ? page.list.systemPatterns : []
                            delegate: Rectangle {
                                required property string modelData
                                implicitWidth: systemLabel.implicitWidth + 16
                                implicitHeight: 26
                                radius: 8
                                color: "#F7F8FA"
                                border.width: 1
                                border.color: "#ECEEF0"
                                Label {
                                    id: systemLabel
                                    anchors.centerIn: parent
                                    text: parent.modelData
                                    font.pixelSize: 12
                                    color: "#5B6167"
                                }
                            }
                        }
                    }
                }
            }
        }

        Rectangle {
            Layout.fillWidth: true
            implicitHeight: 1
            color: "#ECEEF0"
        }
        RowLayout {
            Layout.fillWidth: true
            Layout.margins: 14
            Layout.leftMargin: 20
            Layout.rightMargin: 20
            spacing: 10

            Label {
                Layout.fillWidth: true
                text: page.list.errorText !== "" ? page.list.errorText : qsTr("Изменения вступят в силу при следующей синхронизации.")
                font.pixelSize: 12
                color: page.list.errorText !== "" ? "#E5484D" : "#8F9399"
                wrapMode: Text.Wrap
            }
            RagradButton {
                text: qsTr("Отмена")
                onClicked: page.list.cancel()
            }
            RagradButton {
                primary: true
                text: qsTr("Сохранить")
                enabled: page.list.dirty
                onClicked: page.list.save()
            }
        }
    }
}
