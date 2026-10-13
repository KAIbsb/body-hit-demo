$ErrorActionPreference = 'Stop'
$taskProjectRoot = Split-Path -Parent $PSScriptRoot
$taskBuildLink = 'C:\Users\23102\InjuryLabBuildLink'
if (-not (Test-Path -LiteralPath $taskBuildLink)) {
    New-Item -ItemType Junction -Path $taskBuildLink -Target $taskProjectRoot | Out-Null
}
$taskLinkTarget = (Get-Item -LiteralPath $taskBuildLink).Target
if ([IO.Path]::GetFullPath($taskLinkTarget) -ne [IO.Path]::GetFullPath($taskProjectRoot)) {
    throw '构建目录链接指向其他项目，请检查 InjuryLabBuildLink。'
}
New-Item -ItemType Directory -Path "$taskProjectRoot\Saved\Verification" -Force | Out-Null
& 'C:\Program Files\Epic Games\UE_5.8\Engine\Build\BatchFiles\Build.bat' InjuryLabEditor Win64 Development "-Project=$taskBuildLink\四肢受损.uproject" -NoUBA -DisableUnity -DisableAdaptiveUnity -NoHotReloadFromIDE | Tee-Object -FilePath "$taskProjectRoot\Saved\Verification\Build.log"
if ($LASTEXITCODE -ne 0) { throw "编译失败：$LASTEXITCODE" }
