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

// Диск Ра Град: шапка окна как в мобильном «Диске» — облако и «Диск» (главная), «Активность», «Настройки»,
// справа сотрудник с меню (браузер, выход, отключение компьютера, закрыть программу). Один аккаунт на компьютер.

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import eu.OpenCloud.gui 1.0
import eu.OpenCloud.libsync 1.0
import eu.OpenCloud.resources 1.0

Pane {
    id: bar
    readonly property SettingsDialog settingsDialog: ocContext
    readonly property OCQuickWidget widget: ocQuickWidget

    // палитра мобильного «Диска»
    readonly property color cText: "#1A1A1A"
    readonly property color cMuted: "#8F9399"
    readonly property color cLine: "#ECEEF0"
    readonly property color cPanel: "#F7F8FA"
    readonly property color cSel: "#EAF2FF"
    readonly property color cBlue: "#2F6FEB"

    readonly property AccountState accountState: AccountManager.accounts.length > 0 ? AccountManager.accounts[0] : null

    function goHome() {
        if (accountState) {
            settingsDialog.currentAccount = accountState.account;
        }
    }

    function webDiskUrl() {
        let url = String(accountState.account.url);
        if (!url.endsWith("/")) {
            url += "/";
        }
        return url + "app/";
    }

    padding: 0
    Accessible.name: qsTr("Панель навигации")

    background: Rectangle {
        color: "#FFFFFF"
        Rectangle {
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.bottom: parent.bottom
            height: 1
            color: bar.cLine
        }
    }

    Component.onCompleted: {
        if (OCUtils.compareQVersion(6, 8, 3) >= 0) {
            ToolTip.toolTip.popupType = Popup.Native;
        }
    }

    Connections {
        target: widget

        function onFocusFirst() {
            homeButton.forceActiveFocus(Qt.TabFocusReason);
        }

        function onFocusLast() {
            if (userChip.visible) {
                userChip.forceActiveFocus(Qt.TabFocusReason);
            } else {
                settingsButton.forceActiveFocus(Qt.TabFocusReason);
            }
        }
    }

    // вкладка с значком над подписью (цвета — те же, что у bar: встроенный компонент не видит id файла)
    component NavTab: AbstractButton {
        id: tab
        property string glyph
        property bool active: false
        readonly property color tint: active ? "#2F6FEB" : (hovered ? "#1A1A1A" : "#8F9399")

        hoverEnabled: true
        focusPolicy: Qt.StrongFocus
        implicitWidth: Math.max(76, tabColumn.implicitWidth + 20)
        implicitHeight: 48
        // роль кнопки: вкладка вне списка вкладок не видна экранным дикторам и автоматизации Windows
        Accessible.role: Accessible.Button
        Accessible.name: text
        Keys.onReturnPressed: tab.clicked()

        background: Rectangle {
            radius: 10
            color: tab.active ? "#EAF2FF" : (tab.hovered ? "#F7F8FA" : "transparent")
            border.width: tab.visualFocus ? 2 : 0
            border.color: "#2F6FEB"
        }

        contentItem: Item {
            Column {
                id: tabColumn
                anchors.centerIn: parent
                spacing: 3
                Image {
                    anchors.horizontalCenter: parent.horizontalCenter
                    width: 18
                    height: 18
                    sourceSize: Qt.size(width, height)
                    source: OCUtils.resourcePath("fontawesome", tab.glyph, tab.enabled, 0, tab.tint)
                }
                Label {
                    anchors.horizontalCenter: parent.horizontalCenter
                    text: tab.text
                    font.pixelSize: 12
                    color: tab.tint
                }
            }
        }
    }

    RowLayout {
        anchors.fill: parent
        anchors.leftMargin: 18
        anchors.rightMargin: 14
        spacing: 6

        // don't modify the enabled state directly as it messes with the palette in Qt 6.7.2
        opacity: widget.enabled ? 1.0 : 0.5

        // облако и «Диск» — возврат на главную
        AbstractButton {
            id: homeButton
            hoverEnabled: true
            focusPolicy: Qt.StrongFocus
            implicitHeight: 44
            implicitWidth: homeRow.implicitWidth + 12
            Accessible.role: Accessible.Button
            Accessible.name: JsonTheme.applicationDisplayName
            Keys.onBacktabPressed: widget.parentFocusWidget.focusPrevious()
            Keys.onReturnPressed: bar.goHome()
            onClicked: bar.goHome()

            background: Rectangle {
                radius: 10
                color: "transparent"
                border.width: homeButton.visualFocus ? 2 : 0
                border.color: bar.cBlue
            }
            contentItem: Item {
                Row {
                    id: homeRow
                    anchors.verticalCenter: parent.verticalCenter
                    x: 6
                    spacing: 10
                    Image {
                        anchors.verticalCenter: parent.verticalCenter
                        width: 34
                        height: 34
                        sourceSize: Qt.size(width * 2, height * 2)
                        fillMode: Image.PreserveAspectFit
                        source: OCUtils.resourcePath("jsontheme", "applicationIcon")
                    }
                    Label {
                        anchors.verticalCenter: parent.verticalCenter
                        text: JsonTheme.applicationDisplayName
                        font.pixelSize: 19
                        font.weight: Font.Bold
                        color: bar.cText
                    }
                }
            }
        }

        Item {
            // spacer
            Layout.fillWidth: true
        }

        NavTab {
            id: logButton
            glyph: ""
            text: qsTr("Активность")
            active: settingsDialog.currentPage === SettingsDialog.Activity
            // повторное нажатие возвращает на главную
            onClicked: {
                if (active) {
                    bar.goHome();
                } else {
                    settingsDialog.currentPage = SettingsDialog.Activity;
                }
            }
        }
        NavTab {
            id: settingsButton
            glyph: ""
            text: qsTr("Настройки")
            active: settingsDialog.currentPage === SettingsDialog.Settings
            onClicked: {
                if (active) {
                    bar.goHome();
                } else {
                    settingsDialog.currentPage = SettingsDialog.Settings;
                }
            }
        }
        Repeater {
            // branded buttons with a URL
            model: JsonTheme.urlButtons

            delegate: NavTab {
                required property int index
                required property urlButton modelData
                text: modelData.text
                onClicked: Qt.openUrlExternally(modelData.url)
            }
        }

        // сотрудник: аватар, имя и адрес Диска; по нажатию — меню
        AbstractButton {
            id: userChip
            visible: bar.accountState !== null
            Layout.leftMargin: 8
            hoverEnabled: true
            focusPolicy: Qt.StrongFocus
            leftPadding: 4
            rightPadding: 12
            topPadding: 4
            bottomPadding: 4
            Accessible.role: Accessible.ButtonMenu
            Accessible.name: bar.accountState ? bar.accountState.account.davDisplayName : ""
            Keys.onReturnPressed: userChip.clicked()
            Keys.onTabPressed: widget.parentFocusWidget.focusNext()
            onClicked: accountMenu.popup(userChip, 0, userChip.height + 4)

            background: Rectangle {
                radius: height / 2
                color: userChip.down || accountMenu.opened ? bar.cLine : (userChip.hovered ? "#EFF1F3" : bar.cPanel)
                border.width: userChip.visualFocus ? 2 : 0
                border.color: bar.cBlue
            }

            contentItem: RowLayout {
                spacing: 8

                Item {
                    implicitWidth: 32
                    implicitHeight: 32
                    Image {
                        id: avatarImage
                        anchors.fill: parent
                        visible: bar.accountState !== null && bar.accountState.account.hasAvatar
                        source: visible ? OCUtils.avatarPath(bar.accountState.account.uid, true) : ""
                        sourceSize: Qt.size(64, 64)
                        cache: false
                    }
                    Rectangle {
                        anchors.fill: parent
                        visible: !avatarImage.visible
                        radius: width / 2
                        color: "#B8C7FF"
                        Label {
                            anchors.centerIn: parent
                            text: bar.accountState ? bar.accountState.account.initials : ""
                            font.pixelSize: 12
                            font.weight: Font.Bold
                            color: "#2B3A80"
                        }
                    }
                }
                ColumnLayout {
                    spacing: 0
                    Label {
                        text: bar.accountState ? bar.accountState.account.davDisplayName : ""
                        font.pixelSize: 13
                        font.weight: Font.DemiBold
                        color: bar.cText
                        elide: Text.ElideRight
                        Layout.maximumWidth: 180
                    }
                    Label {
                        text: bar.accountState ? bar.accountState.account.hostName : ""
                        font.pixelSize: 12
                        color: bar.cMuted
                        elide: Text.ElideRight
                        Layout.maximumWidth: 180
                    }
                }
                Image {
                    Layout.preferredWidth: 10
                    Layout.preferredHeight: 10
                    sourceSize: Qt.size(10, 10)
                    source: OCUtils.resourcePath("fontawesome", "", true, 0, bar.cMuted)
                }
            }

            Connections {
                target: bar.accountState ? bar.accountState.account : null
                function onAvatarChanged() {
                    avatarImage.source = "";
                    if (bar.accountState.account.hasAvatar) {
                        avatarImage.source = OCUtils.avatarPath(bar.accountState.account.uid, true);
                    }
                }
            }

            Menu {
                id: accountMenu
                // отдельным окном: внутри шапки высотой 64 меню обрезалось
                popupType: Popup.Window

                // берём в момент нажатия: шапка загружается раньше, чем окно создаёт страницу аккаунта,
                // поэтому привязка-свойство осталась бы пустой (null) навсегда
                function accountSettings() {
                    return bar.accountState ? bar.settingsDialog.accountSettings(bar.accountState.account) : null;
                }

                MenuItem {
                    text: qsTr("Открыть Диск в браузере")
                    onTriggered: Qt.openUrlExternally(bar.webDiskUrl())
                }
                MenuItem {
                    text: bar.accountState && bar.accountState.state === AccountState.SignedOut ? qsTr("Войти в аккаунт") : qsTr("Выйти из аккаунта")
                    onTriggered: accountMenu.accountSettings().slotToggleSignInState()
                }
                MenuItem {
                    text: qsTr("Переподключиться")
                    visible: enabled
                    height: visible ? implicitHeight : 0
                    enabled: bar.accountState !== null && bar.accountState.state !== AccountState.SignedOut && bar.accountState.state !== AccountState.Connected
                    onTriggered: bar.accountState.checkConnectivity(true)
                }
                MenuSeparator {}
                MenuItem {
                    text: qsTr("Отключить этот компьютер")
                    onTriggered: accountMenu.accountSettings().slotDeleteAccount()
                }
                MenuSeparator {}
                MenuItem {
                    text: qsTr("Закрыть программу")
                    onTriggered: Qt.quit()
                }
            }
        }

        // без аккаунта (первый запуск) — вход
        RagradButton {
            visible: bar.accountState === null
            primary: true
            text: qsTr("Войти")
            onClicked: settingsDialog.addAccount()
        }
    }
}
