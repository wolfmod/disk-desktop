/*
 * Диск Ра Град: служебные окна в стиле мобильного «Диска» (QML) вместо серых форм апстрима —
 * «Файлы, которые не синхронизируются» (вместо IgnoreListEditor) и «Журнал работы» (вместо LogBrowser).
 * Логика и файлы настроек — те же, что у прежних форм.
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

#include <QDialog>
#include <QObject>
#include <QStringList>
#include <QUrl>
#include <QVariantList>
#include <QtQml/qqmlregistration.h>

namespace OCC {

/// Окно с QML-страницей: белый фон, страница получает объект с логикой как ocContext.
/// Объект становится дочерним окну; его сигнал finished() закрывает окно.
class RagradQmlDialog : public QDialog
{
    Q_OBJECT
public:
    RagradQmlDialog(const QUrl &page, QObject *context, const QString &title, const QSize &size, QWidget *parent = nullptr);
};

/// Маски имён файлов, которые не отправляются на Диск: встроенные (только просмотр) и свои (файл исключений пользователя).
class RagradIgnoreList : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QStringList systemPatterns READ systemPatterns CONSTANT)
    Q_PROPERTY(QVariantList userPatterns READ userPatterns NOTIFY changed)
    Q_PROPERTY(bool dirty READ dirty NOTIFY changed)
    Q_PROPERTY(QString errorText READ errorText NOTIFY changed)
    QML_ELEMENT
    QML_UNCREATABLE("C++ only")

public:
    explicit RagradIgnoreList(QObject *parent = nullptr);

    QStringList systemPatterns() const { return _system; }
    /// [{pattern, deletable, readOnly}]
    QVariantList userPatterns() const;
    bool dirty() const { return _dirty; }
    QString errorText() const { return _errorText; }

    /// добавить свою маску; вернёт текст ошибки или пустую строку
    Q_INVOKABLE QString add(const QString &pattern);
    Q_INVOKABLE void remove(int index);
    Q_INVOKABLE void setDeletable(int index, bool deletable);
    /// записать свои маски и перечитать папки; при успехе закрывает окно
    Q_INVOKABLE void save();
    Q_INVOKABLE void cancel();

Q_SIGNALS:
    void changed();
    void finished();

private:
    struct Entry
    {
        QString pattern;
        bool deletable = false;
        bool readOnly = false;
        QStringList skippedLines; // пустые строки и комментарии перед маской — сохраняются как были
    };
    void readUserFile();

    QStringList _system;
    QList<Entry> _user;
    QStringList _trailingLines;
    bool _dirty = false;
    QString _errorText;
};

/// Настройки журнала работы (то же, что окно LogBrowser апстрима).
class RagradLogSettings : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool enabled READ enabled WRITE setEnabled NOTIFY changed)
    Q_PROPERTY(bool httpLog READ httpLog WRITE setHttpLog NOTIFY changed)
    Q_PROPERTY(int keepCount READ keepCount WRITE setKeepCount NOTIFY changed)
    QML_ELEMENT
    QML_UNCREATABLE("C++ only")

public:
    using QObject::QObject;

    bool enabled() const;
    void setEnabled(bool enabled);
    bool httpLog() const;
    void setHttpLog(bool enabled);
    int keepCount() const;
    void setKeepCount(int count);

    Q_INVOKABLE void openFolder();
    Q_INVOKABLE void close();

Q_SIGNALS:
    void changed();
    void finished();
};

} // namespace OCC
