/*
 * Диск Ра Град: тексты ошибок для людей (см. ragradtexts.h).
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

#include "gui/ragradtexts.h"

#include <QRegularExpression>

using namespace Qt::Literals::StringLiterals;

namespace OCC::Ragrad {

namespace {
    // сетевые ошибки Qt (не переводятся нашим файлом перевода) -> понятные фразы
    struct Phrase
    {
        QRegularExpression pattern;
        QString replacement;
    };

    const QList<Phrase> &networkPhrases()
    {
        static const QList<Phrase> phrases = {
            {QRegularExpression(u"Remote host signaled shutdown"_s, QRegularExpression::CaseInsensitiveOption), u"сервер закрыл соединение"_s},
            {QRegularExpression(u"Remote host closed the connection"_s, QRegularExpression::CaseInsensitiveOption), u"сервер закрыл соединение"_s},
            {QRegularExpression(u"Connection closed"_s, QRegularExpression::CaseInsensitiveOption), u"соединение с сервером прервалось"_s},
            {QRegularExpression(u"Connection refused"_s, QRegularExpression::CaseInsensitiveOption), u"сервер не отвечает"_s},
            {QRegularExpression(u"(Socket operation|Connection) timed out"_s, QRegularExpression::CaseInsensitiveOption), u"сервер долго не отвечает"_s},
            {QRegularExpression(u"Host \\S+ not found"_s, QRegularExpression::CaseInsensitiveOption),
                u"сервер не найден — проверьте подключение к интернету"_s},
            {QRegularExpression(
                 u"Network (unreachable|access is disabled|session error)|Temporary network failure"_s, QRegularExpression::CaseInsensitiveOption),
                u"нет подключения к интернету"_s},
            {QRegularExpression(u"SSL handshake failed[^.;]*"_s, QRegularExpression::CaseInsensitiveOption), u"не удалось установить защищённое соединение"_s},
            {QRegularExpression(u"Operation canceled"_s, QRegularExpression::CaseInsensitiveOption), u"операция прервана"_s},
        };
        return phrases;
    }

    // «Сервер ответил "503 Service Unavailable" на "PROPFIND https://…"» -> по смыслу кода, без адреса и метода
    QString httpCodeText(int code)
    {
        switch (code) {
        case 401:
        case 403:
            return u"нет доступа"_s;
        case 404:
        case 410:
            return u"на сервере этого уже нет"_s;
        case 409:
        case 412:
            return u"файл одновременно изменили в другом месте"_s;
        case 423:
            return u"файл сейчас открыт или заблокирован на сервере"_s;
        case 429:
            return u"сервер перегружен запросами"_s;
        case 507:
            return u"на Диске не хватает места"_s;
        default:
            if (code >= 500) {
                return u"сервер временно недоступен"_s;
            }
            return u"сервер вернул ошибку (код %1)"_s.arg(code);
        }
    }
}

QString humanizeError(const QString &text)
{
    QString result = text;

    for (const auto &phrase : networkPhrases()) {
        result.replace(phrase.pattern, phrase.replacement);
    }

    static const QRegularExpression httpReply(u"Сервер ответил \"(\\d{3})[^\"]*\" на \"[^\"]*\""_s);
    for (auto match = httpReply.match(result); match.hasMatch(); match = httpReply.match(result)) {
        result.replace(match.capturedStart(), match.capturedLength(), httpCodeText(match.captured(1).toInt()));
    }

    // «сервер прислал неполные сведения о файле (нет: checksum, etag)» — названия полей по-русски
    static const QRegularExpression missingFields(u"\\(нет: ([^)]*)\\)"_s);
    const auto missing = missingFields.match(result);
    if (missing.hasMatch()) {
        QString fields = missing.captured(1);
        fields.replace(u"checksum"_s, u"контрольная сумма"_s)
            .replace(u"etag"_s, u"версия"_s)
            .replace(u"permissions"_s, u"права"_s)
            .replace(QRegularExpression(u"\\bid\\b"_s), u"номер"_s);
        result.replace(missing.capturedStart(1), missing.capturedLength(1), fields);
    }

    // в переводе апстрима кавычки «вывернуты»: »папка« -> «папка»
    static const QRegularExpression reversedQuotes(u"»([^«»]*)«"_s);
    result.replace(reversedQuotes, u"«\\1»"_s);

    // фраза может прийти отдельной строкой («сервер долго не отвечает») — с заглавной буквы
    if (!result.isEmpty() && result.at(0).isLower()) {
        result[0] = result.at(0).toUpper();
    }
    return result;
}

QStringList humanizeErrors(const QStringList &texts)
{
    QStringList result;
    result.reserve(texts.size());
    for (const auto &text : texts) {
        result.append(humanizeError(text));
    }
    return result;
}

}
