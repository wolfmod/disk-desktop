/*
 * Диск Ра Град: пункт «Скопировать ссылку на Диске» в меню Проводника (как у Яндекс.Диска).
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

#include "gui/opencloudguilib.h"

#include <QString>

namespace OCC::Ragrad {

/**
 * Пункт меню Проводника для файлов и папок Диска (только Windows, только текущий пользователь, без прав администратора):
 * HKCU\Software\Classes\{*,Directory}\shell\RagradDiskCopyLink с иконкой программы и фильтром AppliesTo по папкам Диска
 * (System.ParsingPath начинается с папки раздела), поэтому в остальных папках компьютера пункта нет.
 * Записывается при запуске и при изменении списка папок; папок нет — пункт убирается. Удаление программы убирает его тоже.
 * 4.0.11: там же запускается сверка значка «Есть ссылка» (цепочка в колонке «Состояние» Проводника) — раз в 5 минут.
 */
OPENCLOUD_GUI_EXPORT void startExplorerMenu();

/**
 * Публичная ссылка на файл или папку Диска — как «Поделиться» в веб-Диске (тот же запрос createLink через сервер, те же
 * права: «Редактор» и «Управляющий»). Ссылка кладётся в буфер обмена, итог — уведомлением Windows.
 */
OPENCLOUD_GUI_EXPORT void copyPublicLink(const QString &localPath);

}
