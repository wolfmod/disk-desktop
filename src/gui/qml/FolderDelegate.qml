/*
 * Copyright (C) by Hannah von Reth <hannah.vonreth@owncloud.com>
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

// Диск Ра Град: главная страница окна в палитре мобильного «Диска» (макет work/disk-desktop/mockup-window.html,
// утверждён Иваном 20.10.2026): общий блок состояния (как у Яндекс.Диска — одно состояние на всю программу,
// RagradSyncSummary) с кнопками «Открыть папку Диска» и «Открыть в браузере», ниже компактный список папок
// с коротким статусом и меню ⋮, внизу — подсказка и версия.

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import eu.OpenCloud.gui 1.0
import eu.OpenCloud.libsync 1.0
import eu.OpenCloud.resources 1.0

Pane {
    id: folderSyncPanel
    readonly property AccountSettings accountSettings: ocContext
    readonly property OCQuickWidget widget: ocQuickWidget

    // палитра мобильного «Диска» (disk-app/lib/ui/common.dart)
    readonly property color cText: "#1A1A1A"
    readonly property color cMuted: "#8F9399"
    readonly property color cLine: "#ECEEF0"
    readonly property color cPanel: "#F7F8FA"
    readonly property color cSel: "#EAF2FF"
    readonly property color cBlue: "#2F6FEB"
    readonly property color cFolder: "#FFCE3A"
    readonly property color cAccent: "#FFD21E"
    readonly property color cOk: "#1E9E5A"
    readonly property color cError: "#C62828"

    // веб-Диск сотрудников — /app/ (стоковый веб OpenCloud у нас не настроен и не используется)
    function webDiskUrl() {
        let url = String(accountSettings.accountState.account.url);
        if (!url.endsWith("/")) {
            url += "/";
        }
        return url + "app/";
    }

    function stateGlyph(state) {
        switch (state) {
        case RagradSyncSummary.Synced:
            return ""; // галочка в круге
        case RagradSyncSummary.Syncing:
            return ""; // стрелки по кругу
        case RagradSyncSummary.Paused:
            return ""; // пауза
        case RagradSyncSummary.Offline:
            return ""; // нет связи
        case RagradSyncSummary.SignedOut:
            return ""; // сотрудник
        case RagradSyncSummary.Error:
            return ""; // крестик в круге
        default:
            return ""; // часы
        }
    }

    function stateColor(state) {
        switch (state) {
        case RagradSyncSummary.Synced:
            return cOk;
        case RagradSyncSummary.Syncing:
            return cBlue;
        case RagradSyncSummary.Error:
            return cError;
        default:
            return cMuted;
        }
    }

    padding: 0
    Accessible.role: Accessible.List
    Accessible.name: qsTr("Папки Диска")

    background: Rectangle {
        color: "#FFFFFF"
    }

    Connections {
        target: widget

        function onFocusFirst() {
            openFolderButton.forceActiveFocus(Qt.TabFocusReason);
        }

        function onFocusLast() {
            listView.forceActiveFocus(Qt.TabFocusReason);
        }
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.leftMargin: 20
        anchors.rightMargin: 20
        anchors.topMargin: 18
        anchors.bottomMargin: 14
        spacing: 0

        // ---------- общее состояние ----------
        Rectangle {
            Layout.fillWidth: true
            implicitHeight: statusRow.implicitHeight + 36
            radius: 14
            color: folderSyncPanel.cPanel

            Accessible.role: Accessible.StaticText
            Accessible.name: RagradSyncSummary.title + ". " + RagradSyncSummary.details

            RowLayout {
                id: statusRow
                anchors.fill: parent
                anchors.leftMargin: 18
                anchors.rightMargin: 18
                spacing: 16

                Rectangle {
                    Layout.alignment: Qt.AlignVCenter
                    implicitWidth: 46
                    implicitHeight: 46
                    radius: 23
                    color: "#FFFFFF"
                    Image {
                        id: stateIcon
                        anchors.centerIn: parent
                        width: 26
                        height: 26
                        sourceSize: Qt.size(width, height)
                        source: OCUtils.resourcePath("fontawesome", folderSyncPanel.stateGlyph(RagradSyncSummary.state), true, 0,
                            folderSyncPanel.stateColor(RagradSyncSummary.state))

                        readonly property bool spinning: RagradSyncSummary.state === RagradSyncSummary.Syncing
                        onSpinningChanged: {
                            if (!spinning) {
                                rotation = 0;
                            }
                        }
                        RotationAnimator on rotation {
                            running: stateIcon.spinning
                            from: 0
                            to: 360
                            duration: 1400
                            loops: Animation.Infinite
                        }
                    }
                }

                ColumnLayout {
                    Layout.fillWidth: true
                    Layout.alignment: Qt.AlignVCenter
                    spacing: 2

                    Label {
                        Layout.fillWidth: true
                        text: RagradSyncSummary.title
                        font.pixelSize: 17
                        font.weight: Font.DemiBold
                        color: folderSyncPanel.cText
                        elide: Text.ElideRight
                    }
                    Label {
                        Layout.fillWidth: true
                        text: RagradSyncSummary.details
                        visible: text !== ""
                        font.pixelSize: 13
                        color: folderSyncPanel.cMuted
                        elide: Text.ElideMiddle
                    }

                    // полоса хода: жёлтая; пока объём неизвестен — бегущий отрезок
                    Rectangle {
                        id: track
                        Layout.fillWidth: true
                        Layout.topMargin: 9
                        visible: RagradSyncSummary.state === RagradSyncSummary.Syncing
                        implicitHeight: 6
                        radius: 3
                        color: "#E3E7EC"
                        clip: true

                        Rectangle {
                            id: bar
                            readonly property bool known: RagradSyncSummary.progress >= 0
                            height: parent.height
                            radius: 3
                            color: folderSyncPanel.cAccent
                            width: known ? Math.max(height, track.width * RagradSyncSummary.progress) : track.width * 0.3
                            onKnownChanged: {
                                if (known) {
                                    x = 0;
                                }
                            }

                            Behavior on width {
                                enabled: bar.known
                                NumberAnimation {
                                    duration: 300
                                }
                            }
                            NumberAnimation on x {
                                running: track.visible && !bar.known
                                from: -track.width * 0.3
                                to: track.width
                                duration: 1300
                                loops: Animation.Infinite
                            }
                        }
                    }

                    // действие, когда синхронизация стоит
                    Label {
                        id: stateAction
                        Layout.topMargin: 6
                        visible: RagradSyncSummary.state === RagradSyncSummary.Paused || RagradSyncSummary.state === RagradSyncSummary.SignedOut
                        text: RagradSyncSummary.state === RagradSyncSummary.SignedOut ? qsTr("Войти в аккаунт") : qsTr("Возобновить синхронизацию")
                        font.pixelSize: 13
                        font.weight: Font.DemiBold
                        font.underline: stateActionArea.containsMouse
                        color: folderSyncPanel.cBlue
                        Accessible.role: Accessible.Link
                        Accessible.name: text
                        MouseArea {
                            id: stateActionArea
                            anchors.fill: parent
                            hoverEnabled: true
                            cursorShape: Qt.PointingHandCursor
                            onClicked: {
                                if (RagradSyncSummary.state === RagradSyncSummary.SignedOut) {
                                    accountSettings.slotToggleSignInState();
                                } else {
                                    RagradSyncSummary.resume();
                                }
                            }
                        }
                    }
                }

                ColumnLayout {
                    Layout.alignment: Qt.AlignVCenter
                    spacing: 8

                    RagradButton {
                        id: openFolderButton
                        Layout.fillWidth: true
                        primary: true
                        text: qsTr("Открыть папку Диска")
                        enabled: accountSettings.accountState.account.defaultSyncRoot !== ""
                        onClicked: OCUtils.showInFileManager(accountSettings.accountState.account.defaultSyncRoot)
                        Keys.onBacktabPressed: widget.parentFocusWidget.focusPrevious()
                    }
                    RagradButton {
                        id: openWebButton
                        Layout.fillWidth: true
                        text: qsTr("Открыть в браузере")
                        onClicked: Qt.openUrlExternally(folderSyncPanel.webDiskUrl())
                    }
                }
            }
        }

        // ---------- папки ----------
        Label {
            Layout.topMargin: 20
            Layout.bottomMargin: 8
            Layout.leftMargin: 2
            text: qsTr("Папки на компьютере")
            font.pixelSize: 13
            font.weight: Font.DemiBold
            color: folderSyncPanel.cMuted
        }

        Rectangle {
            id: listFrame
            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.preferredHeight: listView.contentHeight + 2
            Layout.maximumHeight: listView.contentHeight + 2
            radius: 14
            color: "#FFFFFF"
            border.width: 1
            border.color: folderSyncPanel.cLine
            visible: listView.count > 0

            ListView {
                id: listView
                anchors.fill: parent
                anchors.margins: 1
                clip: true
                focus: true
                activeFocusOnTab: true
                boundsBehavior: Flickable.StopAtBounds
                model: accountSettings.model
                ScrollBar.vertical: ScrollBar {
                    policy: ScrollBar.AsNeeded
                }

                Keys.onSpacePressed: {
                    if (currentItem) {
                        currentItem.openMenu();
                    }
                }
                Keys.onTabPressed: widget.parentFocusWidget.focusNext()
                Keys.onBacktabPressed: openWebButton.forceActiveFocus(Qt.TabFocusReason)

                delegate: Item {
                    id: folderDelegate

                    required property string displayName
                    required property var errorMsg
                    required property Folder folder
                    required property string statusChipText
                    required property string statusChipKind
                    required property string localPath
                    required property string accessibleDescription
                    required property int index

                    property bool errorsExpanded: false

                    function openMenu() {
                        contextMenu.popup(moreButton, 0, moreButton.height);
                    }

                    width: ListView.view.width
                    implicitHeight: rowColumn.implicitHeight

                    Accessible.role: Accessible.ListItem
                    Accessible.name: accessibleDescription

                    Rectangle {
                        anchors.fill: parent
                        color: rowHover.hovered || (folderDelegate.ListView.isCurrentItem && listView.activeFocus) ? folderSyncPanel.cSel : "#FFFFFF"
                        // скругляем только внешние углы первой и последней строки — по рамке списка
                        topLeftRadius: folderDelegate.index === 0 ? 13 : 0
                        topRightRadius: topLeftRadius
                        bottomLeftRadius: folderDelegate.index === listView.count - 1 ? 13 : 0
                        bottomRightRadius: bottomLeftRadius
                    }
                    Rectangle {
                        // разделитель между строками
                        visible: folderDelegate.index > 0
                        anchors.top: parent.top
                        anchors.left: parent.left
                        anchors.right: parent.right
                        height: 1
                        color: folderSyncPanel.cLine
                    }

                    HoverHandler {
                        id: rowHover
                    }

                    MouseArea {
                        anchors.fill: parent
                        acceptedButtons: Qt.LeftButton | Qt.RightButton
                        onClicked: mouse => {
                            folderDelegate.ListView.view.currentIndex = folderDelegate.index;
                            if (mouse.button === Qt.RightButton) {
                                folderDelegate.openMenu();
                            }
                        }
                        // двойной щелчок — открыть папку в Проводнике
                        onDoubleClicked: OCUtils.showInFileManager(folderDelegate.folder.path)
                    }

                    ColumnLayout {
                        id: rowColumn
                        width: parent.width
                        spacing: 0

                        RowLayout {
                            Layout.fillWidth: true
                            Layout.leftMargin: 14
                            Layout.rightMargin: 8
                            Layout.topMargin: 11
                            Layout.bottomMargin: 11
                            spacing: 12

                            Image {
                                Layout.preferredWidth: 30
                                Layout.preferredHeight: 30
                                sourceSize: Qt.size(30, 30)
                                fillMode: Image.PreserveAspectFit
                                source: OCUtils.resourcePath("fontawesome", "", true, 0, folderSyncPanel.cFolder)
                            }

                            ColumnLayout {
                                Layout.fillWidth: true
                                spacing: 1
                                Label {
                                    Layout.fillWidth: true
                                    text: folderDelegate.displayName
                                    font.pixelSize: 14
                                    font.weight: Font.DemiBold
                                    color: folderSyncPanel.cText
                                    elide: Text.ElideRight
                                }
                                Label {
                                    Layout.fillWidth: true
                                    text: folderDelegate.localPath
                                    font.pixelSize: 12
                                    color: folderSyncPanel.cMuted
                                    elide: Text.ElideMiddle
                                }
                            }

                            // короткий статус
                            Rectangle {
                                readonly property var chipColors: ({
                                        "ok": ["#E8F6EE", "#1E9E5A"],
                                        "sync": ["#FFF6CF", "#8A6A00"],
                                        "error": ["#FDECEC", "#C62828"]
                                    })
                                readonly property var colors: chipColors[folderDelegate.statusChipKind] || ["#EEF0F2", "#5F6670"]
                                implicitWidth: chipLabel.implicitWidth + 20
                                implicitHeight: 24
                                radius: 12
                                color: colors[0]
                                Label {
                                    id: chipLabel
                                    anchors.centerIn: parent
                                    text: folderDelegate.statusChipText
                                    font.pixelSize: 12
                                    color: parent.colors[1]
                                }
                            }

                            // меню ⋮
                            AbstractButton {
                                id: moreButton
                                implicitWidth: 30
                                implicitHeight: 30
                                hoverEnabled: true
                                Accessible.role: Accessible.ButtonMenu
                                Accessible.name: qsTr("Действия с папкой «%1»").arg(folderDelegate.displayName)
                                onClicked: folderDelegate.openMenu()
                                background: Rectangle {
                                    radius: 8
                                    color: moreButton.hovered ? folderSyncPanel.cLine : "transparent"
                                }
                                contentItem: Image {
                                    sourceSize: Qt.size(16, 16)
                                    fillMode: Image.Pad
                                    source: OCUtils.resourcePath("fontawesome", "", true, 0, folderSyncPanel.cMuted)
                                }
                            }
                        }

                        // ошибки папки: первые две строки, остальные — по «Показать все»
                        ColumnLayout {
                            Layout.fillWidth: true
                            Layout.leftMargin: 56
                            Layout.rightMargin: 14
                            Layout.bottomMargin: 10
                            visible: folderDelegate.errorMsg.length > 0
                            spacing: 4

                            Repeater {
                                model: folderDelegate.errorsExpanded ? folderDelegate.errorMsg : folderDelegate.errorMsg.slice(0, 2)
                                delegate: RowLayout {
                                    required property string modelData
                                    Layout.fillWidth: true
                                    spacing: 6
                                    Image {
                                        Layout.alignment: Qt.AlignTop
                                        Layout.topMargin: 2
                                        sourceSize: Qt.size(13, 13)
                                        source: OCUtils.resourcePath("fontawesome", "", true, 0, folderSyncPanel.cError)
                                    }
                                    Label {
                                        Layout.fillWidth: true
                                        text: modelData
                                        wrapMode: Text.Wrap
                                        maximumLineCount: folderDelegate.errorsExpanded ? 6 : 2
                                        elide: Text.ElideRight
                                        font.pixelSize: 12
                                        color: folderSyncPanel.cError
                                    }
                                }
                            }
                            Label {
                                visible: folderDelegate.errorMsg.length > 2
                                text: folderDelegate.errorsExpanded ? qsTr("Скрыть") : qsTr("Показать все (%1)").arg(folderDelegate.errorMsg.length)
                                font.pixelSize: 12
                                font.underline: errorsToggle.containsMouse
                                color: folderSyncPanel.cBlue
                                MouseArea {
                                    id: errorsToggle
                                    anchors.fill: parent
                                    hoverEnabled: true
                                    cursorShape: Qt.PointingHandCursor
                                    onClicked: folderDelegate.errorsExpanded = !folderDelegate.errorsExpanded
                                }
                            }
                        }
                    }

                    Menu {
                        id: contextMenu
                        // отдельным окном, чтобы у нижних строк меню не обрезалось краем страницы
                        popupType: Popup.Window
                        property bool shiftPressed: false
                        onAboutToShow: {
                            shiftPressed = OCUtils.hasKeyboardModifiers(Qt.ShiftModifier);
                        }

                        MenuItem {
                            text: qsTr("Открыть в Проводнике")
                            onTriggered: OCUtils.showInFileManager(folderDelegate.folder.path)
                        }
                        MenuItem {
                            text: CommonStrings.copyFilePathToClipBoard()
                            onTriggered: OCUtils.setClipBoardFromFilePath(folderDelegate.folder.path)
                            visible: contextMenu.shiftPressed
                            height: visible ? implicitHeight : 0
                            enabled: visible
                        }
                        MenuItem {
                            text: qsTr("Открыть в браузере")
                            onTriggered: Qt.openUrlExternally(folderSyncPanel.webDiskUrl())
                        }
                        MenuItem {
                            text: CommonStrings.copyUrlToClipBoard()
                            onTriggered: OCUtils.setClipBoard(folderDelegate.folder.webUrl)
                            visible: contextMenu.shiftPressed
                            height: visible ? implicitHeight : 0
                            enabled: folderDelegate.folder.webUrl && visible
                        }

                        MenuSeparator {}

                        MenuItem {
                            text: folderDelegate.folder.isSyncRunning ? qsTr("Перезапустить синхронизацию") : qsTr("Синхронизировать сейчас")
                            enabled: accountSettings.accountState.state === AccountState.Connected && !folderDelegate.folder.isSyncPaused && visible
                            onTriggered: accountSettings.slotForceSyncCurrentFolder(folderDelegate.folder)
                            visible: folderDelegate.folder.isReady
                            height: visible ? implicitHeight : 0
                        }
                        MenuItem {
                            text: folderDelegate.folder.isSyncPaused ? qsTr("Возобновить синхронизацию") : qsTr("Приостановить синхронизацию")
                            enabled: accountSettings.accountState.state === AccountState.Connected && visible
                            onTriggered: accountSettings.slotEnableCurrentFolder(folderDelegate.folder, true)
                            visible: folderDelegate.folder.isReady
                            height: visible ? implicitHeight : 0
                        }
                        // выбор подпапок — только без файлов по запросу (у нас файлы по запросу — пункт скрыт)
                        MenuItem {
                            text: qsTr("Choose what to sync")
                            onTriggered: accountSettings.showSelectiveSyncDialog(folderDelegate.folder)
                            visible: folderDelegate.folder.isReady && folderDelegate.folder.vfsMode !== 1
                            height: visible ? implicitHeight : 0
                            enabled: visible
                        }

                        onOpened: {
                            Accessible.announce(qsTr("Меню папки"));
                        }
                    }
                }
            }
        }

        Item {
            // spacer
            Layout.fillWidth: true
            Layout.fillHeight: true
        }

        // ---------- подвал ----------
        RowLayout {
            Layout.fillWidth: true
            Layout.topMargin: 14
            spacing: 16

            Label {
                Layout.fillWidth: true
                text: qsTr("Файлы видны все, а на компьютер скачиваются, когда вы их открываете.")
                font.pixelSize: 12
                color: folderSyncPanel.cMuted
                wrapMode: Text.Wrap
            }
            Label {
                text: qsTr("Версия %1").arg(OCUtils.appVersion)
                font.pixelSize: 12
                color: folderSyncPanel.cMuted
            }
        }
    }
}
