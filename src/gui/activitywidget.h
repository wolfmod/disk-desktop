/*
 * Copyright (C) by Klaas Freitag <freitag@owncloud.com>
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

#pragma once

#include "account.h"
#include "gui/qmlutils.h"

#include <QAbstractItemModel>
#include <QWidget>
#include <QtQml/qqmlregistration.h>

namespace OCC {
class ProtocolWidget;
class IssuesWidget;
class RagradActivityModel;


/**
 * @brief The ActivitySettings class
 * @ingroup gui
 *
 * Диск Ра Град: страница «Активность» в стиле мобильного «Диска» (qml/ActivityPage.qml):
 * «Последние действия» и «Не синхронизировано». Данные собирают прежние ProtocolWidget/IssuesWidget
 * (их логика не меняется, сами таблицы не показываются).
 */
class ActivitySettings : public QWidget
{
    Q_OBJECT
    Q_PROPERTY(QAbstractItemModel *recentModel READ recentModel CONSTANT)
    Q_PROPERTY(QAbstractItemModel *issuesModel READ issuesModel CONSTANT)
    Q_PROPERTY(int issueCount READ issueCount NOTIFY issueCountChanged)
    // 0 — последние действия, 1 — не синхронизировано
    Q_PROPERTY(int currentTab READ currentTab WRITE setCurrentTab NOTIFY currentTabChanged)
    QML_ELEMENT
    QML_UNCREATABLE("C++ only")
    OC_DECLARE_WIDGET_FOCUS

public:
    explicit ActivitySettings(QWidget *parent = nullptr);
    ~ActivitySettings() override;

    QAbstractItemModel *recentModel() const;
    QAbstractItemModel *issuesModel() const;
    int issueCount() const { return _issueCount; }
    int currentTab() const { return _currentTab; }
    void setCurrentTab(int tab);

public Q_SLOTS:
    void slotShowIssuesTab();

Q_SIGNALS:
    void issueCountChanged();
    void currentTabChanged();

private:
    ProtocolWidget *_protocolWidget;
    IssuesWidget *_issuesWidget;
    RagradActivityModel *_recentModel;
    RagradActivityModel *_issuesModel;
    QmlUtils::OCQuickWidget *_quickWidget;
    int _issueCount = 0;
    int _currentTab = 0;
};
}
