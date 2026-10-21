/*
 * Диск Ра Град: список для страницы «Активность» (QML) поверх таблиц ProtocolItemModel.
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

#include "gui/ragradactivitymodel.h"

#include "common/syncjournaldb.h"
#include "gui/folder.h"
#include "gui/folderman.h"
#include "gui/models/models.h"
#include "gui/models/protocolitemmodel.h"
#include "gui/openfilemanager.h"
#include "gui/ragradtexts.h"
#include "libsync/progressdispatcher.h"
#include "libsync/syncfileitem.h"

#include <QDate>
#include <QDateTime>
#include <QDir>
#include <QFileInfo>

using namespace Qt::Literals::StringLiterals;

namespace OCC {

namespace {
    using Column = ProtocolItemModel::ProtocolItemRole;

    bool isError(SyncFileItem::Status status)
    {
        return status == SyncFileItem::NormalError || status == SyncFileItem::FatalError || status == SyncFileItem::DetailError
            || status == SyncFileItem::BlacklistedError || status == SyncFileItem::SoftError;
    }
}

RagradActivityModel::RagradActivityModel(QAbstractItemModel *source, QObject *parent)
    : QSortFilterProxyModel(parent)
{
    setSourceModel(source);
    // свежие сверху; сортировка по исходному значению времени, а не по тексту
    setSortRole(Models::UnderlyingDataRole);
    setDynamicSortFilter(true);
    sort(static_cast<int>(Column::Time), Qt::DescendingOrder);
}

QHash<int, QByteArray> RagradActivityModel::roleNames() const
{
    return {
        {FileNameRole, "fileName"},
        {LocationRole, "location"},
        {MessageRole, "message"},
        {TimeRole, "timeText"},
        {SizeRole, "sizeText"},
        {KindRole, "kind"},
        {CanRetryRole, "canRetry"},
    };
}

QVariant RagradActivityModel::column(int row, int column, int role) const
{
    const QModelIndex sourceIndex = mapToSource(index(row, 0));
    if (!sourceIndex.isValid()) {
        return {};
    }
    return sourceModel()->index(sourceIndex.row(), column).data(role);
}

QString RagradActivityModel::localPath(int row) const
{
    const QString folderPath = column(row, static_cast<int>(Column::Folder), Models::UnderlyingDataRole).toString();
    const QString itemPath = column(row, static_cast<int>(Column::File), Models::UnderlyingDataRole).toString();
    return QDir::toNativeSeparators(folderPath + itemPath);
}

QVariant RagradActivityModel::data(const QModelIndex &index, int role) const
{
    if (role < FileNameRole) {
        return QSortFilterProxyModel::data(index, role);
    }
    const int row = index.row();
    const QString itemPath = column(row, static_cast<int>(Column::File), Models::UnderlyingDataRole).toString();
    const auto status = static_cast<SyncFileItem::Status>(column(row, static_cast<int>(Column::Status), Models::UnderlyingDataRole).toInt());

    switch (role) {
    case FileNameRole:
        return QFileInfo(itemPath).fileName();
    case LocationRole: {
        const QString folderPath = column(row, static_cast<int>(Column::Folder), Models::UnderlyingDataRole).toString();
        auto *folder = FolderMan::instance()->folderForPath(folderPath);
        QString location = folder ? folder->displayName() : QString();
        QString dir = QFileInfo(itemPath).path();
        if (!dir.isEmpty() && dir != u"."_s) {
            dir.replace(u'/', u" / "_s);
            location = location.isEmpty() ? dir : u"%1 / %2"_s.arg(location, dir);
        }
        return location;
    }
    case MessageRole:
        // технические фразы движка и сети — понятным языком (ragradtexts)
        return Ragrad::humanizeError(column(row, static_cast<int>(Column::Action), Qt::DisplayRole).toString());
    case TimeRole: {
        const auto time = column(row, static_cast<int>(Column::Time), Models::UnderlyingDataRole).toDateTime().toLocalTime();
        if (!time.isValid()) {
            return QString();
        }
        return time.date() == QDate::currentDate() ? time.toString(u"HH:mm"_s) : time.toString(u"dd.MM HH:mm"_s);
    }
    case SizeRole:
        return column(row, static_cast<int>(Column::Size), Qt::DisplayRole).toString();
    case KindRole:
        if (isError(status)) {
            return u"error"_s;
        }
        if (Progress::isWarningKind(status) || status == SyncFileItem::Excluded) {
            return u"warn"_s;
        }
        return u"ok"_s;
    case CanRetryRole:
        return status == SyncFileItem::DetailError || status == SyncFileItem::SoftError || status == SyncFileItem::BlacklistedError;
    }
    return {};
}

void RagradActivityModel::openInExplorer(int row) const
{
    const QString path = localPath(row);
    if (QFileInfo::exists(path)) {
        showInFileManager(path);
        return;
    }
    // файла уже нет (удалён, переименован) — открыть папку, где он был
    const QString dir = QFileInfo(path).absolutePath();
    if (QFileInfo::exists(dir)) {
        showInFileManager(dir);
    }
}

void RagradActivityModel::retry(int row) const
{
    // как «Retry sync» в ProtocolWidget::showContextMenu
    const QString folderPath = column(row, static_cast<int>(Column::Folder), Models::UnderlyingDataRole).toString();
    const QString itemPath = column(row, static_cast<int>(Column::File), Models::UnderlyingDataRole).toString();
    if (auto *folder = FolderMan::instance()->folderForPath(folderPath)) {
        folder->journalDb()->wipeErrorBlacklistEntry(itemPath);
        FolderMan::instance()->scheduler()->enqueueFolder(folder, SyncScheduler::Priority::Medium);
    }
}

}
