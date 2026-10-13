$ErrorActionPreference = 'Stop'
$taskProjectRoot = Split-Path -Parent $PSScriptRoot
& 'C:\Program Files\Epic Games\UE_5.8\Engine\Binaries\Win64\UnrealEditor-Cmd.exe' "$taskProjectRoot\四肢受损.uproject" -run=BuildInjuryLab -MinecraftHUD -unattended -NullRHI -nosplash -nop4 -nosound "-abslog=$taskProjectRoot\Saved\Verification\MinecraftHUDUpdate.log"
if ($LASTEXITCODE -ne 0) { throw "HUD 更新失败：$LASTEXITCODE" }
