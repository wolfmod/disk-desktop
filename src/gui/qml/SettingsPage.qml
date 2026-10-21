/*
 * Диск Ра Град: страница «Настройки» в палитре мобильного «Диска» (как главное окно):
 * разделы в рамке со скруглением 14, переключатели, выбор ограничения скорости, «О программе» с версией.
 * Данные и сохранение — RagradSettingsPage (C++), настройки те же, что у прежней формы.
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
// Basic: у полей ввода свой фон (нативный стиль Windows его не даёт настроить)
import QtQuick.Controls.Basic
import QtQuick.Layouts
import eu.OpenCloud.gui 1.0
import eu.OpenCloud.resources 1.0

Pane {
    id: page
    readonly property RagradSettingsPage settings: ocContext
    readonly property OCQuickWidget widget: ocQuickWidget

    padding: 0
    Accessible.role: Accessible.Pane
    Accessible.name: qsTr("Настройки")

    background: Rectangle {
        color: "#FFFFFF"
    }

    // ---------- элементы (встроенный компонент не видит id файла — цвета записаны прямо);
    // раздел, разделитель и строка с переключателем — общие файлы RagradSection/RagradDivider/RagradToggleRow ----------

    // строка-ссылка с галочкой-стрелкой: открывает отдельное окно
    component LinkRow: AbstractButton {
        id: link
        property string hint
        readonly property color chevronColor: "#8F9399"

        Layout.fillWidth: true
        hoverEnabled: true
        focusPolicy: Qt.StrongFocus
        leftPadding: 16
        rightPadding: 16
        topPadding: 12
        bottomPadding: 12
        Accessible.role: Accessible.Button
        Accessible.name: text

        background: Rectangle {
            color: link.hovered ? "#F7F8FA" : "transparent"
            radius: 13
            border.width: link.visualFocus ? 2 : 0
            border.color: "#2F6FEB"
        }

        contentItem: RowLayout {
            spacing: 16
            ColumnLayout {
                Layout.fillWidth: true
                spacing: 2
                Label {
                    Layout.fillWidth: true
                    text: link.text
                    font.pixelSize: 14
                    color: "#1A1A1A"
                    wrapMode: Text.Wrap
                }
                Label {
                    Layout.fillWidth: true
                    visible: link.hint !== ""
                    text: link.hint
                    font.pixelSize: 12
                    color: "#8F9399"
                    wrapMode: Text.Wrap
                }
            }
            Image {
                Layout.alignment: Qt.AlignVCenter
                sourceSize: Qt.size(12, 12)
                source: OCUtils.resourcePath("fontawesome", "", true, 0, link.chevronColor)
            }
        }
    }

    // ограничение скорости: «Без ограничений» / «Автоматически» / «Не больше N КБ/с»
    component LimitRow: ColumnLayout {
        id: limitRow
        property string title
        property int mode: 0
        property int limit: 0
        signal modeChosen(int newMode)
        signal limitChosen(int newLimit)

        Layout.fillWidth: true
        Layout.leftMargin: 16
        Layout.rightMargin: 16
        Layout.topMargin: 12
        Layout.bottomMargin: 14
        spacing: 10

        Label {
            text: limitRow.title
            font.pixelSize: 14
            color: "#1A1A1A"
        }

        Flow {
            Layout.fillWidth: true
            spacing: 8

            Repeater {
                model: [
                    {
                        "label": qsTr("Без ограничений"),
                        "value": 0
                    },
                    {
                        "label": qsTr("Автоматически"),
                        "value": -1
                    },
                    {
                        "label": qsTr("Не больше"),
                        "value": 1
                    }
                ]
                delegate: AbstractButton {
                    id: chip
                    required property var modelData
                    readonly property bool selected: limitRow.mode === modelData.value

                    text: modelData.label
                    hoverEnabled: true
                    focusPolicy: Qt.StrongFocus
                    implicitHeight: 32
                    implicitWidth: chipLabel.implicitWidth + 28
                    Accessible.role: Accessible.RadioButton
                    Accessible.name: text
                    Accessible.checkable: true
                    Accessible.checked: selected
                    onClicked: limitRow.modeChosen(modelData.value)

                    background: Rectangle {
                        radius: 16
                        color: chip.selected ? "#EAF2FF" : (chip.hovered ? "#F7F8FA" : "#FFFFFF")
                        border.width: chip.visualFocus ? 2 : 1
                        border.color: chip.selected || chip.visualFocus ? "#2F6FEB" : "#DADDE1"
                    }
                    contentItem: Label {
                        id: chipLabel
                        text: chip.text
                        font.pixelSize: 13
                        color: chip.selected ? "#2F6FEB" : "#1A1A1A"
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
                    }
                }
            }

            TextField {
                id: limitField
                visible: limitRow.mode === 1
                width: 96
                height: 32
                leftPadding: 10
                rightPadding: 10
                text: String(limitRow.limit)
                font.pixelSize: 13
                color: "#1A1A1A"
                horizontalAlignment: Text.AlignRight
                selectByMouse: true
                validator: IntValidator {
                    bottom: 1
                    top: 1000000
                }
                onEditingFinished: {
                    const value = parseInt(text);
                    if (!isNaN(value) && value > 0) {
                        limitRow.limitChosen(value);
                    }
                }
                background: Rectangle {
                    radius: 10
                    color: "#FFFFFF"
                    border.width: limitField.activeFocus ? 2 : 1
                    border.color: limitField.activeFocus ? "#2F6FEB" : "#DADDE1"
                }
            }
            Label {
                visible: limitRow.mode === 1
                height: 32
                verticalAlignment: Text.AlignVCenter
                text: qsTr("КБ/с")
                font.pixelSize: 13
                color: "#8F9399"
            }
        }
    }

    // ---------- страница ----------

    ScrollView {
        id: scroll
        anchors.fill: parent
        contentWidth: availableWidth
        clip: true

        ColumnLayout {
            width: scroll.availableWidth
            spacing: 20

            Label {
                Layout.leftMargin: 22
                Layout.topMargin: 18
                text: qsTr("Настройки")
                font.pixelSize: 20
                font.weight: Font.Bold
                color: "#1A1A1A"
            }

            RagradSection {
                Layout.leftMargin: 20
                Layout.rightMargin: 20
                title: qsTr("Основное")
                visible: page.settings.autostartAvailable

                RagradToggleRow {
                    text: qsTr("Запускать вместе с Windows")
                    hint: page.settings.autostartLocked ? qsTr("Включено администратором для всех пользователей компьютера.")
                                                       : qsTr("Диск откроется сам после входа в компьютер.")
                    enabled: !page.settings.autostartLocked
                    on: page.settings.autostart
                    onClicked: page.settings.autostart = !page.settings.autostart
                }
            }

            RagradSection {
                Layout.leftMargin: 20
                Layout.rightMargin: 20
                title: qsTr("Синхронизация")

                RagradToggleRow {
                    visible: page.settings.moveToTrashAvailable
                    text: qsTr("Удалённое на Диске — в корзину компьютера")
                    hint: qsTr("Если файл удалят на Диске, его копия с этого компьютера попадёт в корзину Windows, и его можно будет восстановить.")
                    on: page.settings.moveToTrash
                    onClicked: page.settings.moveToTrash = !page.settings.moveToTrash
                }
                RagradDivider {
                    visible: page.settings.moveToTrashAvailable
                }
                RagradToggleRow {
                    text: qsTr("Синхронизировать скрытые файлы")
                    hint: qsTr("Обычно не нужно: служебные скрытые файлы Windows и программ остаются только на этом компьютере.")
                    on: page.settings.syncHiddenFiles
                    onClicked: page.settings.syncHiddenFiles = !page.settings.syncHiddenFiles
                }
                RagradDivider {}
                LinkRow {
                    text: qsTr("Файлы, которые не синхронизируются")
                    hint: qsTr("Временные и служебные файлы по маске имени.")
                    onClicked: page.settings.openIgnoredFiles()
                }
            }

            RagradSection {
                Layout.leftMargin: 20
                Layout.rightMargin: 20
                title: qsTr("Скорость")

                LimitRow {
                    title: qsTr("Скачивание с Диска")
                    mode: page.settings.downloadMode
                    limit: page.settings.downloadLimit
                    onModeChosen: newMode => page.settings.downloadMode = newMode
                    onLimitChosen: newLimit => page.settings.downloadLimit = newLimit
                }
                RagradDivider {}
                LimitRow {
                    title: qsTr("Отправка на Диск")
                    mode: page.settings.uploadMode
                    limit: page.settings.uploadLimit
                    onModeChosen: newMode => page.settings.uploadMode = newMode
                    onLimitChosen: newLimit => page.settings.uploadLimit = newLimit
                }
                RagradDivider {
                    visible: page.settings.meteredAvailable
                }
                RagradToggleRow {
                    visible: page.settings.meteredAvailable
                    text: qsTr("Пауза при лимитном подключении")
                    hint: qsTr("Например, когда интернет раздаётся с телефона.")
                    on: page.settings.pauseWhenMetered
                    onClicked: page.settings.pauseWhenMetered = !page.settings.pauseWhenMetered
                }
            }

            RagradSection {
                Layout.leftMargin: 20
                Layout.rightMargin: 20
                Layout.bottomMargin: 20
                title: qsTr("О программе")

                RowLayout {
                    Layout.fillWidth: true
                    Layout.margins: 16
                    spacing: 14

                    Image {
                        Layout.preferredWidth: 44
                        Layout.preferredHeight: 44
                        sourceSize: Qt.size(88, 88)
                        fillMode: Image.PreserveAspectFit
                        source: OCUtils.resourcePath("jsontheme", "applicationIcon")
                    }
                    ColumnLayout {
                        Layout.fillWidth: true
                        spacing: 1
                        Label {
                            text: JsonTheme.applicationDisplayName
                            font.pixelSize: 16
                            font.weight: Font.Bold
                            color: "#1A1A1A"
                        }
                        Label {
                            text: qsTr("Версия %1").arg(OCUtils.appVersion)
                            font.pixelSize: 13
                            color: "#1A1A1A"
                        }
                        Label {
                            text: qsTr("ООО «Ра Град»")
                            font.pixelSize: 12
                            color: "#8F9399"
                        }
                    }
                    Item {
                        // распорка: колонка подписей без растягиваемых элементов сама не растягивается
                        Layout.fillWidth: true
                    }
                    RagradButton {
                        text: qsTr("Журнал работы")
                        onClicked: page.settings.openLog()
                    }
                }
            }
        }
    }
}
