# Диск Ра Град: вызов CraftMaster в GitHub Actions (как .craft.ps1 апстрима, но с craft_ragrad.ini).
# Вывод дублируется в $env:RUNNER_TEMP/craft.log — при ошибке хвост лога выводится в аннотации сборки.
$PSNativeCommandUseErrorActionPreference = $true

$python = (py -c "import sys; print(sys.executable)")
$env:HOME = ${HOME}
# не дать CraftMaster принять платформу за Android
$env:ANDROID_NDK = $null

$RepoRoot = (Get-Item $PSScriptRoot).Parent.Parent
$command = @("${HOME}/craft/CraftMaster/CraftMaster/CraftMaster.py",
             "--config", "${RepoRoot}/.craft.ini",
             "--config-override", "${RepoRoot}/.github/workflows/craft_ragrad.ini",
             "--target", "${env:CRAFT_TARGET}",
             "--variables", "WORKSPACE=${HOME}/craft") + $args

Write-Host "Exec: ${python} ${command}"

& $python @command 2>&1 | Tee-Object -FilePath "${env:RUNNER_TEMP}/craft.log" -Append
if ($LASTEXITCODE -ne 0) {
    exit 1
}
