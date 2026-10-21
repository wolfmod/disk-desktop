/*
 * Диск Ра Град: список для страницы «Активность» (QML) поверх таблиц ProtocolItemModel.
 * Источник — модель прежних виджетов (ProtocolWidget / IssuesWidget), их логика сбора не меняется;
 * здесь — только свежие сверху и удобные для строки списка поля.
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

#include <QSortFilterProxyModel>

namespace OCC {

class RagradActivityModel : public QSortFilterProxyModel
{
    Q_OBJECT
public:
    enum Roles {
        FileNameRole = Qt::UserRole + 500, // имя файла
        LocationRole, // «Объекты / 2626. ВТ Карасино»
        MessageRole, // что произошло («Скачано», текст ошибки)
        TimeRole, // «12:48» сегодня, иначе «20.10 12:48»
        SizeRole, // «3,4 МБ» или пусто
        KindRole, // ok / warn / error — для значка
        CanRetryRole, // можно повторить синхронизацию
    };

    explicit RagradActivityModel(QAbstractItemModel *source, QObject *parent = nullptr);

    QHash<int, QByteArray> roleNames() const override;
    QVariant data(const QModelIndex &index, int role) const override;

    Q_INVOKABLE void openInExplorer(int row) const;
    Q_INVOKABLE void retry(int row) const;

private:
    QVariant column(int row, int column, int role) const;
    QString localPath(int row) const;
};
}
