$ErrorActionPreference = 'Stop'
$taskProjectRoot = Split-Path -Parent $PSScriptRoot
$taskArguments = @(
    ('"' + $taskProjectRoot + '\四肢受损.uproject"'),
    '/Game/InjuryLab/Maps/L_InjuryLab',
    '-game', '-windowed', '-ResX=1920', '-ResY=1080', '-unattended', '-nosplash', '-nop4', '-nosound',
    '-InjuryLabSmokeTest', '-ExecCmds="t.MaxFPS 60,r.ScreenPercentage 100"',
    ('-abslog="' + $taskProjectRoot + '\Saved\Verification\RenderedSmoke.log"')
)
$taskGameProcess = Start-Process -FilePath 'C:\Program Files\Epic Games\UE_5.8\Engine\Binaries\Win64\UnrealEditor.exe' -ArgumentList $taskArguments -WindowStyle Hidden -PassThru
$taskGameProcess.WaitForExit()
if ($taskGameProcess.ExitCode -ne 0) { throw "带画面验证失败：$($taskGameProcess.ExitCode)" }
Get-Content -LiteralPath "$taskProjectRoot\Saved\Verification\SmokeResult.txt"
