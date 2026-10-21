; Диск Ра Град: сценарий установщика NSIS (на основе шаблона Craft Packager/Nsis/NullsoftInstaller.nsi).
; Отличия от шаблона: Unicode и русский язык, шрифт Segoe UI; установка только для текущего пользователя (без прав
; администратора, в %LOCALAPPDATA%\Programs, как Яндекс.Диск/OneDrive); два экрана — приветствие и ход установки
; (нет шагов «для кого ставить», «папка установки», «папка в меню Пуск», «Готово»); после установки программа
; запускается сама; свои тексты окна «программа запущена».
; Имена: внутреннее «Диск Ра Град» (папка программы, реестр) и отображаемое «Диск» (заголовки, ярлык, «Приложения»).
; Подстановки вида «собачка + фигурные скобки» заполняет Craft (NullsoftInstallerPackager + рецепт admin/craft/ragrad/disk-desktop);
; в комментариях этот вид не писать — Craft примет его за подстановку и упадёт.
; Файл в UTF-8 с BOM — по BOM makensis понимает кириллицу.

Unicode true

; реестр — по внутреннему имени
!define regkey "Software\@{company}\@{productname}"
!define uninstkey "Software\Microsoft\Windows\CurrentVersion\Uninstall\@{productname}"

BrandingText " "

XPStyle on
ManifestDPIAware true
SetFont "Segoe UI" 9

Name "@{ragrad_display_name}"
Caption "Установка Диска"
UninstallCaption "Удаление Диска"

OutFile "@{setupname}"

ShowInstDetails nevershow
ShowUninstDetails nevershow

; только текущий пользователь: права администратора не нужны
!define MULTIUSER_EXECUTIONLEVEL Standard
!define MULTIUSER_MUI
!define MULTIUSER_INSTALLMODE_DEFAULT_REGISTRY_KEY "${regkey}"
!define MULTIUSER_INSTALLMODE_DEFAULT_REGISTRY_VALUENAME "Install_Mode"
!define MULTIUSER_INSTALLMODE_INSTDIR "@{productname}"
!define MULTIUSER_INSTALLMODE_INSTDIR_REGISTRY_KEY "${regkey}"
!define MULTIUSER_INSTALLMODE_INSTDIR_REGISTRY_VALUENAME "Install_Dir"

@{nsis_include_internal}
@{nsis_include}

!include "MultiUser.nsh"
!include "MUI2.nsh"
!include "LogicLib.nsh"
!include "x64.nsh"
!include "process.nsh"
!include "WordFunc.nsh"

Function SnoreWinVer
    ReadRegStr $R0 HKLM "SOFTWARE\Microsoft\Windows NT\CurrentVersion" CurrentVersion
    ${VersionCompare} "6.2" $R0 $R0
    ${If} $R0 == 1
        Push "NotWin8"
    ${Else}
        Push "AtLeastWin8"
    ${EndIf}
FunctionEnd

!macro SnoreShortcut path exe appID
    Call SnoreWinVer
    Pop $0
    ${If} $0 == "AtLeastWin8"
    ${AndIf} "@{SnoreToastExe}" != ""
    ${AndIf} ${FileExists} "@{SnoreToastExe}";did we install snoretoast
        nsExec::ExecToLog '"@{SnoreToastExe}" -install "${path}" "${exe}" "${appID}"'
    ${Else}
        CreateShortCut "${path}" "${exe}"
    ${EndIf}
!macroend

; программа запущена — закрыть её перед установкой/удалением (свои русские тексты вместо английских из process.nsh)
; метки локальны внутри раздела (вставляется в установку и в удаление — по разу в каждом), ${__LINE__} не годится:
; он разный на каждой строке макроса, и метка с переходом получали разные имена
!macro RagradEndProcessWithDialog
IfFileExists $INSTDIR ragrad_retry_kill
Goto ragrad_end_kill
ragrad_retry_kill:
    !insertmacro ListProcess "$INSTDIR"
    ${If} $ProcessList != ""
        MessageBox MB_YESNOCANCEL "Программа «@{ragrad_display_name}» сейчас запущена.$\n$\nЗакрыть её и продолжить?$\n«Да» — закрыть, «Нет» — проверить ещё раз, «Отмена» — выйти." /SD IDYES IDNO ragrad_retry_kill IDCANCEL ragrad_abort_kill
        !insertmacro StopProcess "$INSTDIR"
        Goto ragrad_retry_kill
        ragrad_abort_kill:
            Quit
    ${EndIf}
ragrad_end_kill:
!macroend

;!define MUI_ICON
@{installerIcon}
;!define MUI_ICON

; ---------- экран приветствия ----------
; Стандартная страница MUI перестраивается при показе (RagradWelcomeShow): облако по центру — готовая картинка
; ровно под масштаб экрана (cloud-100 … cloud-300.bmp, admin/win/make_cloud_bmps.ps1), без растягивания;
; под ним крупное «Диск» и серым «ООО «Ра Град»» — живым шрифтом Segoe UI, поэтому чётко при любом масштабе.
; Картинка ниже нужна MUI только для первичной отрисовки, затем заменяется.
!define MUI_WELCOMEFINISHPAGE_BITMAP "@{ragrad_win_dir}\cloud-100.bmp"
!define MUI_WELCOMEPAGE_TITLE "@{ragrad_display_name}"
!define MUI_WELCOMEPAGE_TEXT "@{company}"
!define MUI_PAGE_CUSTOMFUNCTION_SHOW RagradWelcomeShow
!define MUI_PAGE_CUSTOMFUNCTION_DESTROYED RagradWelcomeDestroyed
!insertmacro MUI_PAGE_WELCOME

; ---------- ход установки ----------
!define MUI_PAGE_HEADER_TEXT "Установка Диска"
!define MUI_PAGE_HEADER_SUBTEXT "Это займёт несколько секунд."
!define MUI_INSTFILESPAGE_FINISHHEADER_TEXT "Готово"
!define MUI_INSTFILESPAGE_FINISHHEADER_SUBTEXT "Диск запускается."
!define MUI_INSTFILESPAGE_ABORTHEADER_TEXT "Установка не завершена"
!define MUI_INSTFILESPAGE_ABORTHEADER_SUBTEXT "Запустите установщик ещё раз. Если ошибка повторится, обратитесь к администратору."
!insertmacro MUI_PAGE_INSTFILES

; ---------- удаление ----------
!define MUI_PAGE_HEADER_TEXT "Удаление Диска"
!define MUI_PAGE_HEADER_SUBTEXT "Программа будет удалена с компьютера."
!define MUI_UNCONFIRMPAGE_TEXT_TOP "Папка Диска и файлы в ней останутся на компьютере."
!insertmacro MUI_UNPAGE_CONFIRM
!define MUI_PAGE_HEADER_TEXT "Удаление Диска"
!define MUI_PAGE_HEADER_SUBTEXT "Это займёт несколько секунд."
!define MUI_INSTFILESPAGE_FINISHHEADER_TEXT "Готово"
!define MUI_INSTFILESPAGE_FINISHHEADER_SUBTEXT "Программа удалена."
!insertmacro MUI_UNPAGE_INSTFILES

!insertmacro MUI_LANGUAGE "Russian"

Var RagradCloud

Function RagradWelcomeShow
    ; масштаб экрана, точек на дюйм: 96 = 100 %
    System::Call 'user32::GetDC(p 0) p .r1'
    System::Call 'gdi32::GetDeviceCaps(p r1, i 90) i .r0'
    System::Call 'user32::ReleaseDC(p 0, p r1)'
    ${If} $0 < 96
        StrCpy $0 96
    ${EndIf}

    ; облако под масштаб: $2 x $3 — размер картинки в точках
    InitPluginsDir
    ${If} $0 >= 288
        File "/oname=$PLUGINSDIR\ragrad-cloud.bmp" "@{ragrad_win_dir}\cloud-300.bmp"
        StrCpy $2 384
        StrCpy $3 236
    ${ElseIf} $0 >= 240
        File "/oname=$PLUGINSDIR\ragrad-cloud.bmp" "@{ragrad_win_dir}\cloud-250.bmp"
        StrCpy $2 320
        StrCpy $3 197
    ${ElseIf} $0 >= 192
        File "/oname=$PLUGINSDIR\ragrad-cloud.bmp" "@{ragrad_win_dir}\cloud-200.bmp"
        StrCpy $2 256
        StrCpy $3 158
    ${ElseIf} $0 >= 168
        File "/oname=$PLUGINSDIR\ragrad-cloud.bmp" "@{ragrad_win_dir}\cloud-175.bmp"
        StrCpy $2 224
        StrCpy $3 138
    ${ElseIf} $0 >= 144
        File "/oname=$PLUGINSDIR\ragrad-cloud.bmp" "@{ragrad_win_dir}\cloud-150.bmp"
        StrCpy $2 192
        StrCpy $3 118
    ${ElseIf} $0 >= 120
        File "/oname=$PLUGINSDIR\ragrad-cloud.bmp" "@{ragrad_win_dir}\cloud-125.bmp"
        StrCpy $2 160
        StrCpy $3 98
    ${Else}
        File "/oname=$PLUGINSDIR\ragrad-cloud.bmp" "@{ragrad_win_dir}\cloud-100.bmp"
        StrCpy $2 128
        StrCpy $3 79
    ${EndIf}
    ; LoadImage без растягивания (IMAGE_BITMAP, LR_LOADFROMFILE) → STM_SETIMAGE
    System::Call 'user32::LoadImageW(p 0, w "$PLUGINSDIR\ragrad-cloud.bmp", i 0, i 0, i 0, i 0x10) p .s'
    Pop $RagradCloud
    SendMessage $mui.WelcomePage.Image 0x0172 0 $RagradCloud

    ; размер страницы: $5 x $6
    System::Call '*(i 0, i 0, i 0, i 0) p .r4'
    System::Call 'user32::GetClientRect(p $mui.WelcomePage, p r4)'
    System::Call '*$4(i, i, i .r5, i .r6)'
    System::Free $4

    ; отступ облако→название $7, высота названия $8, высота подписи $9 (в точках при 100 %, умножаются на масштаб)
    IntOp $7 18 * $0
    IntOp $7 $7 / 96
    IntOp $8 46 * $0
    IntOp $8 $8 / 96
    IntOp $9 24 * $0
    IntOp $9 $9 / 96
    ; весь блок — по центру страницы, чуть выше середины
    IntOp $R0 $3 + $7
    IntOp $R0 $R0 + $8
    IntOp $R0 $R0 + $9
    IntOp $R1 $6 - $R0
    IntOp $R1 $R1 / 2
    IntOp $R2 12 * $0
    IntOp $R2 $R2 / 96
    IntOp $R1 $R1 - $R2
    IntOp $R3 $5 - $2
    IntOp $R3 $R3 / 2
    ; SWP_NOZORDER | SWP_NOACTIVATE
    System::Call 'user32::SetWindowPos(p $mui.WelcomePage.Image, p 0, i $R3, i $R1, i $2, i $3, i 0x14)'
    IntOp $R4 $R1 + $3
    IntOp $R4 $R4 + $7
    System::Call 'user32::SetWindowPos(p $mui.WelcomePage.Title, p 0, i 0, i $R4, i $5, i $8, i 0x14)'
    IntOp $R5 $R4 + $8
    System::Call 'user32::SetWindowPos(p $mui.WelcomePage.Text, p 0, i 0, i $R5, i $5, i $9, i 0x14)'

    ; шрифты и цвета — палитра мобильного «Диска» (текст #1A1A1A, второстепенный #8F9399)
    CreateFont $R6 "Segoe UI" 24 600
    SendMessage $mui.WelcomePage.Title ${WM_SETFONT} $R6 0
    CreateFont $R7 "Segoe UI" 10 400
    SendMessage $mui.WelcomePage.Text ${WM_SETFONT} $R7 0
    SetCtlColors $mui.WelcomePage.Title 1A1A1A FFFFFF
    SetCtlColors $mui.WelcomePage.Text 8F9399 FFFFFF
    ; SS_CENTER
    ${NSD_AddStyle} $mui.WelcomePage.Title 1
    ${NSD_AddStyle} $mui.WelcomePage.Text 1
FunctionEnd

Function RagradWelcomeDestroyed
    System::Call 'gdi32::DeleteObject(p $RagradCloud)'
FunctionEnd

SetDateSave on
SetDatablockOptimize on
CRCCheck on
SilentInstall normal

Function .onInit
    !insertmacro MULTIUSER_INIT
    !if "@{architecture}" == "x86_64"
        ${IfNot} ${RunningX64}
            MessageBox MB_OK|MB_ICONEXCLAMATION "Программе нужна 64-разрядная Windows."
            Abort
        ${EndIf}
    !endif
FunctionEnd

Function un.onInit
    !insertmacro MULTIUSER_UNINIT
FunctionEnd

; после успешной установки — сразу запустить программу (вместо страницы «Готово» с галочкой)
Function .onInstSuccess
    IfSilent +2
    Exec '"$INSTDIR\bin\opencloud.exe"'
FunctionEnd

;--------------------------------

; окно закрывается само после установки
AutoCloseWindow true


; основной (невидимый) раздел
Section
  SetDetailsPrint textonly
  DetailPrint "Подготовка…"
  SetDetailsPrint none
  !insertmacro RagradEndProcessWithDialog
  ExecWait '"$MultiUser.InstDir\uninstall.exe" /S _?=$MultiUser.InstDir'
  @{preInstallHook}

  ; папку программы создаём до записей в реестр: если создать нельзя — понятное сообщение вместо
  ; «Невозможно открыть файл для записи» на каждом файле, и в реестре ничего не остаётся
  ClearErrors
  SetOutPath $INSTDIR
  ${If} ${Errors}
    MessageBox MB_OK|MB_ICONSTOP "Не удалось создать папку программы:$\n$INSTDIR$\n$\nСкопируйте установщик в папку «Загрузки» или на рабочий стол и запустите его оттуда. Если ошибка повторится, обратитесь к администратору." /SD IDOK
    Abort
  ${EndIf}

  WriteRegStr SHCTX "${regkey}" "Install_Dir" "$INSTDIR"
  WriteRegStr SHCTX "${MULTIUSER_INSTALLMODE_INSTDIR_REGISTRY_KEY}" "${MULTIUSER_INSTALLMODE_DEFAULT_REGISTRY_VALUENAME}" "$MultiUser.InstallMode"
  ; запись для «Установленных приложений» — отображаемое имя
  WriteRegStr SHCTX "${uninstkey}" "DisplayName" "@{ragrad_display_name}"
  WriteRegStr SHCTX "${uninstkey}" "UninstallString" '"$INSTDIR\uninstall.exe"'
  WriteRegStr SHCTX "${uninstkey}" "DisplayIcon" "$INSTDIR\@{iconname}"
  WriteRegStr SHCTX "${uninstkey}" "URLInfoAbout" "@{website}"
  WriteRegStr SHCTX "${uninstkey}" "Publisher" "@{company}"
  WriteRegStr SHCTX "${uninstkey}" "DisplayVersion" "@{version}"
  WriteRegDWORD SHCTX "${uninstkey}" "EstimatedSize" "@{estimated_size}"

  @{registry_hook}

  SetDetailsPrint textonly
  DetailPrint "Копирование файлов…"
  SetDetailsPrint none

File /a "@{dataPath}"
File /a "@{7za}"
File /a "@{icon}"

  SetDetailsPrint textonly
  DetailPrint "Распаковка файлов…"
  SetDetailsPrint none

nsExec::ExecToLog '"$INSTDIR\7za.exe" x -r -y "$INSTDIR\@{dataName}" -o"$INSTDIR"'
Delete "$INSTDIR\7za.exe"
Delete "$INSTDIR\@{dataName}"

AddSize @{installSize}

WriteUninstaller "uninstall.exe"

SectionEnd

; ярлык в меню «Пуск» — один, сразу в корне меню, с отображаемым именем (без страницы выбора папки; стандартный
; блок ярлыков Craft рассчитан на эту страницу и без неё не собирается). Идентификатор — для уведомлений Windows
; (= organizationDomain темы).
Section
  SetOutPath "$INSTDIR"
  !insertmacro SnoreShortcut "$SMPROGRAMS\@{ragrad_display_name}.lnk" "$INSTDIR\bin\opencloud.exe" "@{ragrad_app_id}"
SectionEnd

@{sections}


; удаление
Section "Uninstall"
!insertmacro RagradEndProcessWithDialog

DeleteRegKey SHCTX "${uninstkey}"
DeleteRegKey SHCTX "${regkey}"

Delete "$SMPROGRAMS\@{ragrad_display_name}.lnk"

@{uninstallFiles}
@{uninstallDirs}

SectionEnd

@{un_sections}
