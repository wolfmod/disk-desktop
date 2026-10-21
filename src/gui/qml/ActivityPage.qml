/*
 * Диск Ра Град: страница «Активность» в палитре мобильного «Диска» (как главное окно и настройки):
 * переключатель «Последние действия» / «Не синхронизировано», список файлов со значком состояния,
 * временем и размером, меню «Открыть в Проводнике» / «Повторить синхронизацию».
 * Данные — ActivitySettings (C++): модели прежних ProtocolWidget/IssuesWidget через RagradActivityModel.
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
import eu.OpenCloud.gui 1.0

Pane {
    id: page
    readonly property ActivitySettings activity: ocContext
    readonly property OCQuickWidget widget: ocQuickWidget
    readonly property bool showIssues: activity.currentTab === 1

    readonly property color cText: "#1A1A1A"
    readonly property color cMuted: "#8F9399"
    readonly property color cLine: "#ECEEF0"
    readonly property color cSel: "#EAF2FF"
    readonly property color cBlue: "#2F6FEB"
    readonly property color cOk: "#1E9E5A"
    readonly property color cWarn: "#C99700"
    readonly property color cError: "#C62828"

    function kindGlyph(kind) {
        return kind === "error" ? "" : (kind === "warn" ? "" : "");
    }
    function kindColor(kind) {
        return kind === "error" ? cError : (kind === "warn" ? cWarn : cOk);
    }

    padding: 0
    Accessible.role: Accessible.Pane
    Accessible.name: qsTr("Активность")

    background: Rectangle {
        color: "#FFFFFF"
    }

    // переключатель вкладки (как выбор скорости в настройках)
    component TabChip: AbstractButton {
        id: chip
        property bool selected: false
        property int badge: 0

        hoverEnabled: true
        focusPolicy: Qt.StrongFocus
        implicitHeight: 34
        implicitWidth: chipRow.implicitWidth + 28
        Accessible.role: Accessible.RadioButton
        Accessible.name: badge > 0 ? text + " (" + badge + ")" : text
        Accessible.checkable: true
        Accessible.checked: selected

        background: Rectangle {
            radius: 17
            color: chip.selected ? "#EAF2FF" : (chip.hovered ? "#F7F8FA" : "#FFFFFF")
            border.width: chip.visualFocus ? 2 : 1
            border.color: chip.selected || chip.visualFocus ? "#2F6FEB" : "#DADDE1"
        }
        contentItem: Item {
            Row {
                id: chipRow
                anchors.centerIn: parent
                spacing: 8
                Label {
                    anchors.verticalCenter: parent.verticalCenter
                    text: chip.text
                    font.pixelSize: 13
                    color: chip.selected ? "#2F6FEB" : "#1A1A1A"
                }
                Rectangle {
                    anchors.verticalCenter: parent.verticalCenter
                    visible: chip.badge > 0
                    implicitWidth: Math.max(20, badgeLabel.implicitWidth + 10)
                    implicitHeight: 20
                    radius: 10
                    color: "#FDECEC"
                    Label {
                        id: badgeLabel
                        anchors.centerIn: parent
                        text: chip.badge > 999 ? "999+" : String(chip.badge)
                        font.pixelSize: 11
                        font.weight: Font.DemiBold
                        color: "#C62828"
                    }
                }
            }
        }
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.leftMargin: 20
        anchors.rightMargin: 20
        anchors.topMargin: 18
        anchors.bottomMargin: 18
        spacing: 0

        Label {
            Layout.leftMargin: 2
            text: qsTr("Активность")
            font.pixelSize: 20
            font.weight: Font.Bold
            color: page.cText
        }

        Row {
            Layout.topMargin: 16
            Layout.bottomMargin: 14
            spacing: 8
            TabChip {
                text: qsTr("Последние действия")
                selected: !page.showIssues
                onClicked: page.activity.currentTab = 0
            }
            TabChip {
                text: qsTr("Не синхронизировано")
                selected: page.showIssues
                badge: page.activity.issueCount
                onClicked: page.activity.currentTab = 1
            }
        }

        Rectangle {
            Layout.fillWidth: true
            Layout.fillHeight: true
            radius: 14
            color: "#FFFFFF"
            border.width: 1
            border.color: page.cLine

            // пусто
            ColumnLayout {
                anchors.centerIn: parent
                width: Math.min(parent.width - 40, 420)
                visible: listView.count === 0
                spacing: 8
                Image {
                    Layout.alignment: Qt.AlignHCenter
                    sourceSize: Qt.size(40, 40)
                    source: OCUtils.resourcePath("fontawesome", page.showIssues ? "" : "", true, 0, page.showIssues ? page.cOk : page.cMuted)
                }
                Label {
                    Layout.fillWidth: true
                    horizontalAlignment: Text.AlignHCenter
                    text: page.showIssues ? qsTr("Проблем нет") : qsTr("Пока здесь пусто")
                    font.pixelSize: 16
                    font.weight: Font.DemiBold
                    color: page.cText
                }
                Label {
                    Layout.fillWidth: true
                    horizontalAlignment: Text.AlignHCenter
                    wrapMode: Text.Wrap
                    text: page.showIssues ? qsTr("Все файлы синхронизируются как положено.")
                                          : qsTr("Здесь появятся файлы, которые Диск скачал на компьютер или отправил на сервер.")
                    font.pixelSize: 13
                    color: page.cMuted
                }
            }

            ListView {
                id: listView
                anchors.fill: parent
                anchors.margins: 1
                clip: true
                focus: true
                activeFocusOnTab: true
                boundsBehavior: Flickable.StopAtBounds
                model: page.showIssues ? page.activity.issuesModel : page.activity.recentModel
                ScrollBar.vertical: ScrollBar {
                    policy: ScrollBar.AsNeeded
                }
                Keys.onSpacePressed: {
                    if (currentItem) {
                        currentItem.openMenu();
                    }
                }
                Keys.onReturnPressed: {
                    if (currentItem) {
                        model.openInExplorer(currentIndex);
                    }
                }

                delegate: Item {
                    id: row

                    required property int index
                    required property string fileName
                    required property string location
                    required property string message
                    required property string timeText
                    required property string sizeText
                    required property string kind
                    required property bool canRetry

                    // у ошибок синхронизации без файла заголовок — сам текст ошибки
                    readonly property string title: fileName !== "" ? fileName : message
                    readonly property string subtitle: {
                        const parts = [];
                        if (fileName !== "" && message !== "") {
                            parts.push(message);
                        }
                        if (location !== "") {
                            parts.push(location);
                        }
                        return parts.join(" · ");
                    }

                    function openMenu() {
                        rowMenu.popup(moreButton, 0, moreButton.height);
                    }

                    width: ListView.view.width
                    implicitHeight: rowLayout.implicitHeight + 20

                    Accessible.role: Accessible.ListItem
                    Accessible.name: title + ". " + subtitle + ". " + timeText

                    Rectangle {
                        anchors.fill: parent
                        color: rowHover.hovered || (row.ListView.isCurrentItem && listView.activeFocus) ? page.cSel : "#FFFFFF"
                        topLeftRadius: row.index === 0 ? 13 : 0
                        topRightRadius: topLeftRadius
                        bottomLeftRadius: row.index === listView.count - 1 ? 13 : 0
                        bottomRightRadius: bottomLeftRadius
                    }
                    Rectangle {
                        visible: row.index > 0
                        anchors.top: parent.top
                        anchors.left: parent.left
                        anchors.right: parent.right
                        anchors.leftMargin: 50
                        height: 1
                        color: page.cLine
                    }
                    HoverHandler {
                        id: rowHover
                    }
                    MouseArea {
                        anchors.fill: parent
                        acceptedButtons: Qt.LeftButton | Qt.RightButton
                        onClicked: mouse => {
                            row.ListView.view.currentIndex = row.index;
                            if (mouse.button === Qt.RightButton) {
                                row.openMenu();
                            }
                        }
                        onDoubleClicked: row.ListView.view.model.openInExplorer(row.index)
                    }

                    RowLayout {
                        id: rowLayout
                        anchors.left: parent.left
                        anchors.right: parent.right
                        anchors.verticalCenter: parent.verticalCenter
                        anchors.leftMargin: 16
                        anchors.rightMargin: 8
                        spacing: 14

                        Image {
                            Layout.alignment: Qt.AlignTop
                            Layout.topMargin: 2
                            Layout.preferredWidth: 20
                            Layout.preferredHeight: 20
                            sourceSize: Qt.size(20, 20)
                            source: OCUtils.resourcePath("fontawesome", page.kindGlyph(row.kind), true, 0, page.kindColor(row.kind))
                        }
                        ColumnLayout {
                            Layout.fillWidth: true
                            spacing: 2
                            Label {
                                Layout.fillWidth: true
                                text: row.title
                                font.pixelSize: 14
                                font.weight: Font.DemiBold
                                color: page.cText
                                elide: Text.ElideMiddle
                                maximumLineCount: row.fileName !== "" ? 1 : 2
                                wrapMode: row.fileName !== "" ? Text.NoWrap : Text.Wrap
                            }
                            // обычное действие — одна серая строка «Скачан · Объекты»;
                            // ошибка/предупреждение — текст цветом отдельно, где лежит файл — серой строкой ниже
                            Label {
                                Layout.fillWidth: true
                                readonly property string lineText: row.kind === "ok" ? row.subtitle : (row.fileName !== "" ? row.message : "")
                                visible: lineText !== ""
                                text: lineText
                                font.pixelSize: 12
                                color: row.kind === "error" ? page.cError : (row.kind === "warn" ? "#8A6A00" : page.cMuted)
                                wrapMode: Text.Wrap
                                maximumLineCount: 2
                                elide: Text.ElideRight
                            }
                            Label {
                                Layout.fillWidth: true
                                visible: row.kind !== "ok" && row.location !== ""
                                text: row.location
                                font.pixelSize: 12
                                color: page.cMuted
                                elide: Text.ElideMiddle
                            }
                        }
                        ColumnLayout {
                            Layout.alignment: Qt.AlignTop
                            spacing: 2
                            Label {
                                Layout.alignment: Qt.AlignRight
                                text: row.timeText
                                font.pixelSize: 12
                                color: page.cMuted
                            }
                            Label {
                                Layout.alignment: Qt.AlignRight
                                visible: row.sizeText !== ""
                                text: row.sizeText
                                font.pixelSize: 12
                                color: page.cMuted
                            }
                        }
                        AbstractButton {
                            id: moreButton
                            Layout.alignment: Qt.AlignVCenter
                            implicitWidth: 30
                            implicitHeight: 30
                            hoverEnabled: true
                            Accessible.role: Accessible.ButtonMenu
                            Accessible.name: qsTr("Действия с файлом «%1»").arg(row.title)
                            onClicked: row.openMenu()
                            background: Rectangle {
                                radius: 8
                                color: moreButton.hovered ? page.cLine : "transparent"
                            }
                            contentItem: Image {
                                sourceSize: Qt.size(16, 16)
                                fillMode: Image.Pad
                                source: OCUtils.resourcePath("fontawesome", "", true, 0, page.cMuted)
                            }
                        }
                    }

                    Menu {
                        id: rowMenu
                        popupType: Popup.Window
                        MenuItem {
                            text: qsTr("Открыть в Проводнике")
                            onTriggered: row.ListView.view.model.openInExplorer(row.index)
                        }
                        MenuItem {
                            text: qsTr("Повторить синхронизацию")
                            visible: row.canRetry
                            height: visible ? implicitHeight : 0
                            onTriggered: row.ListView.view.model.retry(row.index)
                        }
                    }
                }
            }
        }
    }
}
