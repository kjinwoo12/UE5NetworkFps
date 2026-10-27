# Run the first-person rig regression sweep in one command: no editor window, no rendering.
# Usage: .\Tools\FpRigRegression\fp_rig_headless.ps1 [-Mode quick|full] [-EngineDir <UE 5.8 root>]
#   -Mode quick   while iterating (the states that fail first); full before committing
#   -EngineDir    engine root (the folder holding Engine\Binaries); defaults to $env:UE_ENGINE_DIR
#   -Rendered     render off screen instead of -nullrhi (if a check ever depends on rendering)
# Exit code 0 = ALL PASS. Prints the non-PASS lines and the time taken. Results go to
# Saved/FpRigCheck/headless/ so a GUI editor running its own sweep is not overwritten.
param([ValidateSet('quick', 'full')][string]$Mode = 'full', [switch]$Rendered, [int]$TimeoutSec = 600,
      [string]$Level = '/FpsMaps/Maps/L_FpsRigTest', [string]$EngineDir = $env:UE_ENGINE_DIR)
$Here = Split-Path -Parent $MyInvocation.MyCommand.Path
$ProjectDir = (Resolve-Path "$Here\..\..").Path
$Project = "$ProjectDir\UE5NetworkFps.uproject"
if (-not $EngineDir) { Write-Output 'Set -EngineDir or $env:UE_ENGINE_DIR to the Unreal Engine 5.8 root'; exit 2 }
$EditorCmd = Join-Path $EngineDir 'Engine\Binaries\Win64\UnrealEditor-Cmd.exe'
if (-not (Test-Path $EditorCmd)) { Write-Output "Not found: $EditorCmd"; exit 2 }
$Out = "$ProjectDir\Saved\FpRigCheck\headless"
New-Item -ItemType Directory -Force $Out | Out-Null
Remove-Item "$Out\report.txt", "$Out\result.json" -ErrorAction SilentlyContinue

$env:FP_RIG_DIR = $Here
$env:FP_RIG_OUT_DIR = $Out
$env:FP_RIG_MODE = $Mode
$env:FP_RIG_LEVEL = $Level

# One standalone player whatever the local PIE settings are (often a listen server + clients for
# network checks). The override lives only in this process, but the editor writes its per-user
# settings on exit, so the file is put back afterwards.
$Pie = '-ini:EditorPerProjectUserSettings:[/Script/UnrealEd.LevelEditorPlaySettings]:PlayNetMode=PIE_Standalone,[/Script/UnrealEd.LevelEditorPlaySettings]:PlayNumberOfClients=1'
$UserIni = "$ProjectDir\Saved\Config\WindowsEditor\EditorPerProjectUserSettings.ini"
$UserIniBackup = "$Out\EditorPerProjectUserSettings.ini.bak"
if (Test-Path $UserIni) { Copy-Item $UserIni $UserIniBackup -Force }

# -ExecCmds, not -ExecutePythonScript: the latter quits the editor as soon as the script returns.
# Fixed time step: runs as fast as the CPU allows and every run sees the same frame times.
$py = "$Here\fp_rig_headless.py" -replace '\\', '/'
$editorArgs = @("`"$Project`"", '-unattended', '-nosplash', '-nopause', '-nosound', '-UseFixedTimeStep', '-FPS=50', $Pie,
                "-ExecCmds=`"py $py`"", "-abslog=`"$Out\editor.log`"")
$editorArgs += $(if ($Rendered) { @('-RenderOffscreen', '-ResX=1280', '-ResY=720') } else { @('-nullrhi') })

$sw = [Diagnostics.Stopwatch]::StartNew()
$p = Start-Process $EditorCmd -ArgumentList $editorArgs -PassThru -WindowStyle Hidden
if (-not $p.WaitForExit($TimeoutSec * 1000)) {
    $p | Stop-Process -Force
    Write-Output "TIMEOUT after $TimeoutSec s (see $Out\editor.log)"
}
$sw.Stop()
if (Test-Path $UserIniBackup) { Copy-Item $UserIniBackup $UserIni -Force }
Get-Process CrashReportClient, CrashReportClientEditor -ErrorAction SilentlyContinue | Stop-Process -Force
Select-String -Path "$Out\editor.log" -Pattern '\[fp_rig_headless\]' | ForEach-Object { $_.Line -replace '^.*\[fp_rig_headless\] ', '  ' }
if (-not (Test-Path "$Out\report.txt")) { Write-Output "NO REPORT (see $Out\editor.log, Saved\Crashes)"; exit 2 }
$report = Get-Content "$Out\report.txt"
$report | Where-Object { $_ -notlike 'PASS *' }
Write-Output ("{0} checks, mode {1}, {2:N0} s end to end" -f ($report.Count - 1), $Mode, $sw.Elapsed.TotalSeconds)
if ($report[-1] -eq 'ALL PASS') { exit 0 } else { exit 1 }
