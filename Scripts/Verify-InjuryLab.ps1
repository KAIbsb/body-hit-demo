$ErrorActionPreference = 'Stop'
$taskProjectRoot = Split-Path -Parent $PSScriptRoot
$taskEditor = 'C:\Program Files\Epic Games\UE_5.8\Engine\Binaries\Win64\UnrealEditor-Cmd.exe'
& $taskEditor "$taskProjectRoot\四肢受损.uproject" -unattended -NullRHI -nosplash -nop4 -nosound '-ExecCmds=Automation RunTests InjuryLab' '-TestExit=Automation Test Queue Empty' "-ReportExportPath=$taskProjectRoot\Saved\Verification\Automation" "-abslog=$taskProjectRoot\Saved\Verification\Automation.log"
if ($LASTEXITCODE -ne 0) { throw "UE 自动化测试失败：$LASTEXITCODE" }
$taskReport = Get-Content -LiteralPath "$taskProjectRoot\Saved\Verification\Automation\index.json" -Raw | ConvertFrom-Json
if ($taskReport.failed -ne 0 -or $taskReport.succeeded -lt 5) { throw '测试报告未确认全部通过。' }
$taskReport | Select-Object succeeded,failed,notRun
