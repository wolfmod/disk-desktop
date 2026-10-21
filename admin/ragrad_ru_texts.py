# Диск Ра Град: точечная правка русского перевода translations/desktop_ru.ts (исходные английские строки апстрима не трогаем).
# Запуск: python -I admin/ragrad_ru_texts.py  (из корня исходников). Повторный запуск безопасен.
import re
import sys
from xml.sax.saxutils import escape

TS = "translations/desktop_ru.ts"

# (контекст, исходная строка) -> наш перевод
FIX = {
    ("AccountBar", "Quit"): "Закрыть",
    ("OCC::Application", "Quit"): "Закрыть",
    ("OCC::Systray", "Quit"): "Закрыть",
    ("OCC::SettingsDialog", "Quit %1"): "Закрыть «%1»",
    ("OCC::SettingsDialog", "Are you sure you want to quit %1?"): "Закрыть программу «%1»? Синхронизация остановится до следующего запуска.",
    ("FolderDelegate", "Log out"): "Выйти из аккаунта",
    ("FolderDelegate", "Log in"): "Войти в аккаунт",
    ("FolderDelegate", "Remove"): "Отключить этот компьютер",
    ("FolderDelegate", "Manage Account"): "Аккаунт",
    ("FolderDelegate", "Force sync now"): "Синхронизировать сейчас",
    ("Credentials", "Stay logged out"): "Не входить",
    ("Credentials", "Connecting %1 to:\n%2"): "%1\n%2",
    ("OCC::AccountSettings", "Disconnected"): "Нет соединения",
    ("OCC::AccountSettings", "Signed out"): "Вы вышли из аккаунта",
    ("OCC::CommonStrings", "Show in %1"): "Открыть в %1",
    ("OCC::CommonStrings", "Show »%1« in %2"): "Открыть «%1» в %2",
    ("OCC::CommonStrings", "Show in web browser"): "Открыть в браузере",
    # заголовок окна мастера: «Диск - Вход» (раньше «Диск - Вход в Диск» — название дважды)
    ("SetupWizardWidget", "Add New Account"): "Вход",
    # окно «Отключить этот компьютер»: аккаунт на сервере не удаляется — отключается только компьютер
    ("OCC::AccountSettings", "Confirm Account Removal"): "Отключение компьютера",
    # ошибки синхронизации — понятным языком (английские сетевые фразы Qt и коды ответа заменяет gui/ragradtexts.cpp)
    ("OCC::SyncEngine", "Aborted due to: %1"): "Синхронизация прервалась, Диск повторит её сам. Причина: %1",
    ("OCC::ProcessDirectoryJob", "Server replied with an error while reading directory »%1«: %2"):
        "не удалось получить с сервера содержимое папки «%1» (%2)",
    ("OCC::SyncEngine", "%1 (skipped due to earlier error, trying again in %2)"): "%1 (Диск повторит попытку через %2)",
    ("RemoteInfo", "server reported no %1"): "сервер прислал неполные сведения о файле (нет: %1)",
    ("OCC::AbstractNetworkJob", "Connection timed out"): "сервер долго не отвечает",
    ("OCC::GETFileJob", "Connection Timeout"): "сервер долго не отвечает",
    # в исходной строке апстрима лишняя «« — в переводе убираем и говорим по-человечески
    ("OCC::AccountSettings",
     "<p>Do you really want to remove the connection to the account %1«?</p><p><b>Note:</b> This will <b>not</b> delete any files.</p>"):
        "<p>Отключить этот компьютер от аккаунта «%1»?</p><p><b>Файлы на компьютере останутся</b>, синхронизация прекратится.</p>",
    ("OCC::AccountSettings", "Remove connection"): "Отключить",
    # отображаемое имя программы — «Диск»: фразы, где оно стоит в падеже, переписаны так, чтобы имя шло без склонения
    ("OCC::SocketApi", "Share with %1"): "Поделиться через %1",
    ("Utility", "Shutting down %1"): "Завершение работы: %1",
    ("OCC::Wizard::ServerUrlSetupWizardPage", "%1 logo"): "Логотип: %1",
}

# строки, которых в переводе апстрима нет (стали переводимыми в нашем форке)
ADD = {
    ("OCC::CommonStrings", "Explorer"): "Проводнике",
}


def xml_text(s):
    return escape(s, {'"': "&quot;", "'": "&apos;"})


def main():
    data = open(TS, encoding="utf-8").read()
    changed = 0
    for (ctx, src), tr in FIX.items():
        ctx_re = re.compile(r"(<context>\s*<name>" + re.escape(ctx) + r"</name>)(.*?)(</context>)", re.S)
        m = ctx_re.search(data)
        if not m:
            sys.exit("нет контекста: " + ctx)
        body = m.group(2)
        # между source и translation могут стоять comment/extracomment (пояснения для переводчика)
        msg_re = re.compile(r"(<source>" + re.escape(xml_text(src)) + r"</source>(?:\s*<(?:extra)?comment>[^<]*</(?:extra)?comment>)*\s*<translation)([^>]*)>(.*?)(</translation>)", re.S)
        new_body, n = msg_re.subn(lambda mm: mm.group(1) + ">" + xml_text(tr) + mm.group(4), body)
        if n == 0:
            sys.exit("нет строки: %s / %r" % (ctx, src))
        data = data[: m.start(2)] + new_body + data[m.end(2):]
        changed += n
    for (ctx, src), tr in ADD.items():
        if "<source>%s</source>" % xml_text(src) in data.split("<name>%s</name>" % ctx, 1)[1].split("</context>", 1)[0]:
            continue
        marker = "<name>%s</name>" % ctx
        entry = "\n    <message>\n        <source>%s</source>\n        <translation>%s</translation>\n    </message>" % (xml_text(src), xml_text(tr))
        data = data.replace(marker, marker + entry, 1)
        changed += 1
    # в рабочей копии Windows (core.autocrlf=true) файл с CRLF — сохраняем так же, чтобы git не видел правку всего файла
    open(TS, "w", encoding="utf-8", newline="\r\n").write(data)
    print("изменено переводов:", changed)


main()
