/*
 * Диск Ра Град: раздел страницы в стиле мобильного «Диска» — подпись серым и рамка со скруглением 14,
 * в которую складываются строки (RagradToggleRow, RagradDivider и др.).
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

ColumnLayout {
    id: section
    property string title
    default property alias rows: box.data

    Layout.fillWidth: true
    spacing: 8

    Label {
        Layout.leftMargin: 2
        visible: section.title !== ""
        text: section.title
        font.pixelSize: 13
        font.weight: Font.DemiBold
        color: "#8F9399"
    }
    Rectangle {
        Layout.fillWidth: true
        implicitHeight: box.implicitHeight + 2
        radius: 14
        color: "#FFFFFF"
        border.width: 1
        border.color: "#ECEEF0"

        ColumnLayout {
            id: box
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.top: parent.top
            anchors.margins: 1
            spacing: 0
        }
    }
}
