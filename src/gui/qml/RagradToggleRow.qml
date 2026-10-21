/*
 * Диск Ра Град: строка раздела с переключателем — название, пояснение серым, справа переключатель
 * (жёлтый, как в мобильном «Диске»). Состояние — свойство on, нажатие — сигнал clicked.
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
import QtQuick.Controls
import QtQuick.Layouts

AbstractButton {
    id: toggle
    property string hint
    property bool on: false

    Layout.fillWidth: true
    hoverEnabled: true
    focusPolicy: Qt.StrongFocus
    leftPadding: 16
    rightPadding: 16
    topPadding: 12
    bottomPadding: 12
    Accessible.role: Accessible.CheckBox
    Accessible.name: text
    Accessible.checkable: true
    Accessible.checked: on

    background: Rectangle {
        color: "transparent"
    }

    contentItem: RowLayout {
        spacing: 16
        ColumnLayout {
            Layout.fillWidth: true
            spacing: 2
            Label {
                Layout.fillWidth: true
                text: toggle.text
                font.pixelSize: 14
                color: toggle.enabled ? "#1A1A1A" : "#8F9399"
                wrapMode: Text.Wrap
            }
            Label {
                Layout.fillWidth: true
                visible: toggle.hint !== ""
                text: toggle.hint
                font.pixelSize: 12
                color: "#8F9399"
                wrapMode: Text.Wrap
            }
        }
        Rectangle {
            Layout.alignment: Qt.AlignVCenter
            implicitWidth: 44
            implicitHeight: 24
            radius: 12
            opacity: toggle.enabled ? 1.0 : 0.5
            color: toggle.on ? "#FFD21E" : (toggle.hovered ? "#CDD1D6" : "#DADDE1")
            border.width: toggle.visualFocus ? 2 : 0
            border.color: "#2F6FEB"
            Behavior on color {
                ColorAnimation {
                    duration: 120
                }
            }
            Rectangle {
                y: 2
                x: toggle.on ? 22 : 2
                width: 20
                height: 20
                radius: 10
                color: "#FFFFFF"
                Behavior on x {
                    NumberAnimation {
                        duration: 120
                    }
                }
            }
        }
    }
}
