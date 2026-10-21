/*
 * Диск Ра Град: вход по логину и паролю (пароль меняется на ключ устройства).
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

// specifically use the basic style, as we modify the palette
import QtQuick.Controls.Basic
import QtQuick.Layouts

import eu.OpenCloud.resources 1.0
import eu.OpenCloud.gui 1.0
import eu.OpenCloud.libsync 1.0

Credentials {
    readonly property QmlAppTokenCredentials credentials: ocContext
    readonly property OCQuickWidget widget: ocQuickWidget
    // повторный вход: логин известен, спрашиваем только пароль
    readonly property bool knownUser: credentials.userName !== ""

    function submit() {
        if (loginButton.enabled) {
            credentials.login(loginField.text, passwordField.text);
        }
    }

    // поле ввода как в мобильном «Диске»: скругление 12, серая рамка, при вводе — синяя
    component RoundField: TextField {
        id: field
        implicitHeight: 44
        leftPadding: 14
        rightPadding: 14
        font.pixelSize: 14
        color: "#1A1A1A"
        placeholderTextColor: "#8F9399"
        selectByMouse: true
        background: Rectangle {
            radius: 12
            color: field.enabled ? "#FFFFFF" : "#F7F8FA"
            border.width: field.activeFocus ? 2 : 1
            border.color: field.activeFocus ? "#2F6FEB" : "#DADDE1"
        }
    }

    ColumnLayout {
        Layout.alignment: Qt.AlignHCenter
        Layout.preferredWidth: 340
        Layout.maximumWidth: 340
        spacing: 10

        Label {
            Layout.fillWidth: true
            Layout.bottomMargin: 6
            horizontalAlignment: Text.AlignHCenter
            wrapMode: Text.WordWrap
            font.pixelSize: 14
            color: "#8F9399"
            text: knownUser ? qsTr("Введите пароль, чтобы продолжить работу с Диском") : qsTr("Войдите с логином и паролем сотрудника")
        }

        RoundField {
            id: loginField
            Layout.fillWidth: true
            visible: !knownUser
            enabled: !credentials.busy
            placeholderText: qsTr("Телефон, почта или логин")
            inputMethodHints: Qt.ImhNoAutoUppercase | Qt.ImhNoPredictiveText
            onAccepted: passwordField.forceActiveFocus(Qt.TabFocusReason)
            Keys.onBacktabPressed: widget.parentFocusWidget.focusPrevious()
        }

        RoundField {
            id: passwordField
            Layout.fillWidth: true
            enabled: !credentials.busy
            echoMode: TextInput.Password
            placeholderText: qsTr("Пароль")
            onAccepted: submit()
        }

        Label {
            Layout.fillWidth: true
            visible: credentials.errorMessage !== ""
            horizontalAlignment: Text.AlignHCenter
            wrapMode: Text.WordWrap
            font.pixelSize: 13
            color: "#C62828"
            text: credentials.errorMessage
        }

        RagradButton {
            id: loginButton
            Layout.fillWidth: true
            Layout.topMargin: 4
            implicitHeight: 44
            primary: true
            enabled: !credentials.busy && passwordField.text !== "" && (knownUser || loginField.text.trim() !== "")
            text: credentials.busy ? qsTr("Вход…") : qsTr("Войти")
            onClicked: submit()
            Keys.onTabPressed: event => {
                // в мастере кнопки «Остаться не в сети» нет
                if (!credentials.isRefresh) {
                    widget.parentFocusWidget.focusNext();
                } else {
                    event.accepted = false;
                }
            }
        }

        Loader {
            Layout.fillWidth: true
            Layout.maximumHeight: 42
            sourceComponent: logOutButton
        }
    }

    Connections {
        target: widget

        function onFocusFirst() {
            if (knownUser) {
                passwordField.forceActiveFocus(Qt.TabFocusReason);
            } else {
                loginField.forceActiveFocus(Qt.TabFocusReason);
            }
        }
    }
}
