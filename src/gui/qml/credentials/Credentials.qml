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

import QtQuick

// specifically use the basic style, as we modify the palette
import QtQuick.Controls.Basic
import QtQuick.Layouts

import eu.OpenCloud.resources 1.0
import eu.OpenCloud.gui 1.0
import eu.OpenCloud.libsync 1.0

Pane {
    id: credentialsView

    //TODO
    //required property QmlCredentials credentials
    readonly property OCQuickWidget widget: ocQuickWidget

    default property alias content: contentLayout.data
    property alias logOutButton: logutButtonComponent

    palette.window: JsonTheme.primaryBackgroundColor
    palette.windowText: JsonTheme.primaryForegroundColor
    palette.button: JsonTheme.primaryButtonColor.color
    palette.buttonText: JsonTheme.primaryButtonColor.textColor
    palette.disabled.buttonText: JsonTheme.primaryButtonColor.textColorDisabled

    Component {
        id: logutButtonComponent

        Button {
            id: logOutButton
            // this button is hidden when used in the wizard
            visible: credentials.isRefresh
            text: qsTr("Stay logged out")
            onClicked: credentials.logOutRequested()

            palette.button: JsonTheme.secondaryButtonColor.color
            palette.buttonText: JsonTheme.secondaryButtonColor.textColor
            palette.disabled.buttonText: JsonTheme.secondaryButtonColor.textColorDisabled

            Keys.onTabPressed: event => {
                if (!visible) {
                    event.accepted = false;
                } else {
                    widget.parentFocusWidget.focusNext();
                }
            }
        }
    }

    ColumnLayout {
        anchors.fill: parent
        Item {
            Layout.fillHeight: true
        }

        Image {
            Layout.alignment: Qt.AlignHCenter
            fillMode: Image.PreserveAspectFit
            source: OCUtils.resourcePath("jsontheme", "wizardLogo", true)
            sourceSize.height: 112
            sourceSize.width: 112
        }

        // Диск Ра Град: вместо «Подключение к: <адрес сервера>» — название программы (при повторном входе — сотрудник);
        // адрес сервера людям не нужен
        Label {
            Layout.fillWidth: true
            Layout.topMargin: 12
            text: credentials.isRefresh ? credentials.displayName : JsonTheme.applicationDisplayName
            horizontalAlignment: Text.AlignHCenter
            font.pixelSize: credentials.isRefresh ? 17 : 26
            font.weight: credentials.isRefresh ? Font.DemiBold : Font.Bold
            elide: Text.ElideRight
        }

        Item {
            Layout.fillHeight: true
            Layout.maximumHeight: 28
        }

        ColumnLayout {
            id: contentLayout
            Layout.alignment: Qt.AlignHCenter
            // иначе колонка растягивается на всю ширину окна вслед за полями ввода
            Layout.fillWidth: false
        }

        Item {
            Layout.fillHeight: true
        }
    }

    Connections {
        target: widget

        function onFocusLast() {
            if (logOutButton.visible) {
                logOutButton.forceActiveFocus(Qt.TabFocusReason);
            } else {
                logOutButton.nextItemInFocusChain(false).forceActiveFocus(Qt.TabFocusReason);
            }
        }
    }
}
