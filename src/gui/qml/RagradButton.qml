/*
 * Диск Ра Град: кнопка в стиле мобильного «Диска» (disk-app/lib/ui/common.dart) —
 * основная жёлтая (#FFD21E) или белая с рамкой, скругление 12.
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

AbstractButton {
    id: control

    property bool primary: false

    hoverEnabled: true
    focusPolicy: Qt.StrongFocus
    implicitHeight: 36
    implicitWidth: label.implicitWidth + 32

    Accessible.role: Accessible.Button
    Accessible.name: text

    Keys.onReturnPressed: control.clicked()
    Keys.onEnterPressed: control.clicked()

    contentItem: Label {
        id: label
        text: control.text
        font.pixelSize: 13
        font.weight: Font.DemiBold
        color: "#1A1A1A"
        horizontalAlignment: Text.AlignHCenter
        verticalAlignment: Text.AlignVCenter
        elide: Text.ElideRight
    }

    background: Rectangle {
        radius: 12
        opacity: control.enabled ? 1.0 : 0.5
        color: control.primary ? (control.down ? "#F5C400" : control.hovered ? "#FFDB47" : "#FFD21E") : (control.down ? "#ECEEF0" : control.hovered ? "#F7F8FA" : "#FFFFFF")
        border.width: control.primary ? 0 : 1
        border.color: "#DADDE1"

        // рамка фокуса с клавиатуры
        Rectangle {
            anchors.fill: parent
            anchors.margins: -3
            radius: 15
            color: "transparent"
            border.width: 2
            border.color: "#2F6FEB"
            visible: control.visualFocus
        }
    }
}
