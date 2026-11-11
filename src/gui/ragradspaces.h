/*
 * Диск Ра Град: какие разделы Диска подключать и куда их класть — как в мобильном приложении.
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

#include "gui/accountstate.h"

#include <functional>

#include <QJsonObject>

namespace OCC::Ragrad {

/**
 * Раскладка разделов в общей папке «Диск Ра Град» (account->defaultSyncRoot()), как в мобильном приложении
 * (disk-app/lib/api.dart::loadSpaces):
 * - личное пространство не подключается;
 * - проектный раздел — по пути из описания без «Диск/» и «Ра Град/» («Диск/Ра Град/Объекты» -> «Объекты»);
 * - «общие» (Shares, папки, открытые по отдельности): сотруднику без участия в «Объектах» (доступ по годам) —
 *   как папка «Объекты» (внутри его открытые папки объектов), участнику «Объектов» — как «Открытые мне».
 * Уже подключённые разделы не трогаются. Возвращает true, если подключён хотя бы один новый раздел.
 */
bool connectSpaces(const AccountStatePtr &accountState, bool useVfs);

/**
 * Подключать новые разделы сами, когда сервер сообщит о них (список обновляется каждые 30 с):
 * сотруднику дали доступ — папка появляется без действий с его стороны.
 * Работает только для аккаунтов с общей папкой (defaultSyncRoot) и не во время первичной настройки.
 */
void watchSpaces(const AccountStatePtr &accountState);

/**
 * Снять с бывшей общей папки подпись и значок Диска (Desktop.ini + системный атрибут) — при «Отключить этот
 * компьютер». Иначе Проводник продолжает показывать старую папку как «Диск» рядом с новой.
 */
void clearFolderLabel(const QString &path);

/**
 * Кто и что сделал — по журналу действий Диска (сервер отвечает: actor — имя сотрудника или «Система», action, path,
 * old_path, parent — событие про папку над файлом). 4.0.4 — только автор; 4.0.9 — ещё и новый путь при переименовании
 * (targetPath) и само действие для понятного текста. done(ответ) — один раз; пустой объект — не найдено (после двух повторов).
 */
void lookupChange(const AccountPtr &account, const QString &spaceId, const QString &relPath, const QString &targetPath, QObject *context,
    std::function<void(const QJsonObject &)> done);

/// понятный текст про переименование/перенос папки («Папка «Геодезия» переименована в «Геодезия 2024» (в папке «1028. …»).»); пусто — не про папку
QString folderChangeText(const QJsonObject &ev);

} // namespace OCC::Ragrad
