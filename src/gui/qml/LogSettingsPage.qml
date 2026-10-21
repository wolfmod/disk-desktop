/*
 * Диск Ра Град: окно «Журнал работы» в стиле мобильного «Диска» (вместо серого LogBrowser):
 * запись журнала, подробная запись обмена с сервером, сколько файлов хранить, папка журнала.
 * Данные и сохранение — RagradLogSettings (C++), настройки те же, что у прежнего окна.
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
    readonly property RagradLogSettings log: ocContext

    padding: 0
    Accessible.role: Accessible.Pane
    Accessible.name: qsTr("Журнал работы")

    background: Rectangle {
        color: "#FFFFFF"
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
                        text: qsTr("Журнал работы")
                        font.pixelSize: 20
                        font.weight: Font.Bold
                        color: "#1A1A1A"
                    }
                    Label {
                        Layout.fillWidth: true
                        text: qsTr("Журнал помогает разобраться, если Диск работает не так, как ожидается. Включите запись, повторите действие, при котором возникла проблема, и передайте файлы журнала администратору.")
                        font.pixelSize: 13
                        color: "#8F9399"
                        wrapMode: Text.Wrap
                    }
                }

                RagradSection {
                    Layout.leftMargin: 20
                    Layout.rightMargin: 20
                    title: qsTr("Запись")

                    RagradToggleRow {
                        text: qsTr("Записывать журнал")
                        hint: qsTr("Запись продолжится и после перезапуска программы. Старые файлы удаляются сами.")
                        on: page.log.enabled
                        onClicked: page.log.enabled = !page.log.enabled
                    }
                    RagradDivider {}
                    RagradToggleRow {
                        text: qsTr("Подробно записывать обмен с сервером")
                        hint: qsTr("Включайте только по просьбе администратора: журнал станет намного больше.")
                        on: page.log.httpLog
                        onClicked: page.log.httpLog = !page.log.httpLog
                    }
                    RagradDivider {}
                    RowLayout {
                        Layout.fillWidth: true
                        Layout.leftMargin: 16
                        Layout.rightMargin: 16
                        Layout.topMargin: 10
                        Layout.bottomMargin: 10
                        spacing: 12

                        Label {
                            Layout.fillWidth: true
                            text: qsTr("Сколько последних файлов журнала хранить")
                            font.pixelSize: 14
                            color: "#1A1A1A"
                            wrapMode: Text.Wrap
                        }
                        TextField {
                            id: keepField
                            implicitWidth: 64
                            implicitHeight: 32
                            leftPadding: 10
                            rightPadding: 10
                            text: String(page.log.keepCount)
                            font.pixelSize: 13
                            color: "#1A1A1A"
                            horizontalAlignment: Text.AlignRight
                            selectByMouse: true
                            Accessible.name: qsTr("Сколько последних файлов журнала хранить")
                            validator: IntValidator {
                                bottom: 5
                                top: 99
                            }
                            onEditingFinished: {
                                const value = parseInt(text);
                                if (!isNaN(value) && value > 0) {
                                    page.log.keepCount = value;
                                }
                                text = String(page.log.keepCount);
                            }
                            background: Rectangle {
                                radius: 10
                                color: "#FFFFFF"
                                border.width: keepField.activeFocus ? 2 : 1
                                border.color: keepField.activeFocus ? "#2F6FEB" : "#DADDE1"
                            }
                        }
                    }
                }

                RagradSection {
                    Layout.leftMargin: 20
                    Layout.rightMargin: 20
                    title: qsTr("Файлы журнала")

                    RowLayout {
                        Layout.fillWidth: true
                        Layout.margins: 15
                        spacing: 12

                        Label {
                            Layout.fillWidth: true
                            text: qsTr("Файлы журнала хранятся на этом компьютере. Отправьте их администратору, если он попросит.")
                            font.pixelSize: 13
                            color: "#5B6167"
                            wrapMode: Text.Wrap
                        }
                        RagradButton {
                            text: qsTr("Открыть папку")
                            onClicked: page.log.openFolder()
                        }
                    }
                }

                Rectangle {
                    Layout.fillWidth: true
                    Layout.leftMargin: 20
                    Layout.rightMargin: 20
                    Layout.bottomMargin: 20
                    implicitHeight: noteRow.implicitHeight + 24
                    radius: 14
                    color: "#FFF8E1"

                    RowLayout {
                        id: noteRow
                        anchors.left: parent.left
                        anchors.right: parent.right
                        anchors.verticalCenter: parent.verticalCenter
                        anchors.leftMargin: 16
                        anchors.rightMargin: 16
                        spacing: 12

                        Image {
                            Layout.alignment: Qt.AlignTop
                            Layout.topMargin: 2
                            sourceSize: Qt.size(14, 14)
                            source: OCUtils.resourcePath("fontawesome", "", true, 0, "#C98A00")
                        }
                        Label {
                            Layout.fillWidth: true
                            text: qsTr("В журнале есть имена файлов и папок и сведения о работе с сервером. Не публикуйте его и передавайте только администратору.")
                            font.pixelSize: 12
                            color: "#5B4A12"
                            wrapMode: Text.Wrap
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

            Item {
                Layout.fillWidth: true
            }
            RagradButton {
                primary: true
                text: qsTr("Закрыть")
                onClicked: page.log.close()
            }
        }
    }
}
