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

#include "activitywidget.h"


#include "gui/ragradactivitymodel.h"
#include "issueswidget.h"
#include "protocolwidget.h"
#include "theme.h"

#include <QtGui>
#include <QtWidgets>


using namespace std::chrono;
using namespace std::chrono_literals;
using namespace Qt::Literals::StringLiterals;


namespace OCC {

ActivitySettings::ActivitySettings(QWidget *parent)
    : QWidget(parent)
{
    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);

    // Диск Ра Град: прежние таблицы — только источники данных (собирают действия и проблемы), на экран не выводятся
    _protocolWidget = new ProtocolWidget(this);
    _protocolWidget->hide();
    _issuesWidget = new IssuesWidget(this);
    _issuesWidget->hide();

    _recentModel = new RagradActivityModel(_protocolWidget->model(), this);
    _issuesModel = new RagradActivityModel(_issuesWidget->filteredModel(), this);

    connect(_issuesWidget, &IssuesWidget::issueCountUpdated, this, [this](int issueCount) {
        if (_issueCount != issueCount) {
            _issueCount = issueCount;
            Q_EMIT issueCountChanged();
        }
    });

    _quickWidget = new QmlUtils::OCQuickWidget(this);
    layout->addWidget(_quickWidget);
    _quickWidget->setOCContext(QUrl(u"qrc:/qt/qml/eu/OpenCloud/gui/qml/ActivityPage.qml"_s), this);
}

QAbstractItemModel *ActivitySettings::recentModel() const
{
    return _recentModel;
}

QAbstractItemModel *ActivitySettings::issuesModel() const
{
    return _issuesModel;
}

void ActivitySettings::setCurrentTab(int tab)
{
    if (_currentTab != tab) {
        _currentTab = tab;
        Q_EMIT currentTabChanged();
    }
}

void ActivitySettings::slotShowIssuesTab()
{
    setCurrentTab(1);
}

ActivitySettings::~ActivitySettings()
{
}
}
