/*
 * Диск Ра Град: тексты ошибок для людей. Движок и Qt присылают технические фразы («Remote host signaled
 * shutdown», «Server replied "503 …" to "PROPFIND https://…"», «checksum») — перед показом в окне
 * заменяем их понятными русскими (правило: людям — без внутренней кухни; подробности остаются в журнале).
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

#include <QString>
#include <QStringList>

namespace OCC::Ragrad {

QString humanizeError(const QString &text);
QStringList humanizeErrors(const QStringList &texts);

}
