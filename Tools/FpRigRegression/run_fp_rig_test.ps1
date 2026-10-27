# Run the first-person rig automation test (FpsCore.FirstPersonRig.Quick / Full) from a command line:
# no editor window, no rendering. The test itself is C++:
#   Plugins/GameFeatures/FpsCore/Source/FpsCoreEditor/Private/Tests/FpsFirstPersonRigTest.cpp
# Usage: .\Tools\FpRigRegression\run_fp_rig_test.ps1 [-Mode Quick|Full] [-EngineDir <UE 5.8 root>]
#   -Mode Quick   while iterating (6 states); Full before committing (11 states)
#   -EngineDir    engine root (the folder holding Engine\Binaries); defaults to $env:UE_ENGINE_DIR
#   -Rendered     render off screen instead of -nullrhi (if a check ever depends on rendering)
# Build the editor first (LyraEditor Win64 Development) and close any GUI editor of this project.
# Exit code 0 = ALL PASS. Prints the FAIL lines and the time taken. Results:
#   Saved/FpRigCheck/report_<mode>.txt (every check, plain text) and Saved/FpRigCheck/automation/
#   (the automation report: index.json, index.html for the Session Frontend style view).
param([ValidateSet('Quick', 'Full')][string]$Mode = 'Full', [switch]$Rendered, [int]$TimeoutSec = 600,
      [string]$EngineDir = $env:UE_ENGINE_DIR)
$Here = Split-Path -Parent $MyInvocation.MyCommand.Path
$ProjectDir = (Resolve-Path "$Here\..\..").Path
$Project = "$ProjectDir\UE5NetworkFps.uproject"
if (-not $EngineDir) { Write-Output 'Set -EngineDir or $env:UE_ENGINE_DIR to the Unreal Engine 5.8 root'; exit 2 }
$EditorCmd = Join-Path $EngineDir 'Engine\Binaries\Win64\UnrealEditor-Cmd.exe'
if (-not (Test-Path $EditorCmd)) { Write-Output "Not found: $EditorCmd"; exit 2 }
$Out = "$ProjectDir\Saved\FpRigCheck"
$Report = "$Out\report_$($Mode.ToLower()).txt"
New-Item -ItemType Directory -Force $Out | Out-Null
Remove-Item $Report -ErrorAction SilentlyContinue

# Fixed time step: runs as fast as the CPU allows and every run sees the same frame times.
$Test = "FpsCore.FirstPersonRig.$Mode"
$editorArgs = @("`"$Project`"", '-unattended', '-nosplash', '-nopause', '-nosound', '-UseFixedTimeStep', '-FPS=50',
                "-ExecCmds=`"Automation RunTests $Test; Quit`"", '-TestExit="Automation Test Queue Empty"',
                "-ReportExportPath=`"$Out\automation`"", "-abslog=`"$Out\editor.log`"")
$editorArgs += $(if ($Rendered) { @('-RenderOffscreen', '-ResX=1280', '-ResY=720') } else { @('-nullrhi') })

$sw = [Diagnostics.Stopwatch]::StartNew()
$p = Start-Process $EditorCmd -ArgumentList $editorArgs -PassThru -WindowStyle Hidden
if (-not $p.WaitForExit($TimeoutSec * 1000)) {
    $p | Stop-Process -Force
    Write-Output "TIMEOUT after $TimeoutSec s (see $Out\editor.log)"
}
$sw.Stop()
if (-not (Test-Path $Report)) { Write-Output "NO REPORT: the test did not run to the end (see $Out\editor.log, Saved\Crashes)"; exit 2 }
$lines = Get-Content $Report
$lines | Where-Object { $_ -notlike 'PASS *' }
Write-Output ("{0} checks, {1}, {2:N0} s end to end" -f ($lines.Count - 1), $Test, $sw.Elapsed.TotalSeconds)
if ($lines[-1] -eq 'ALL PASS') { exit 0 } else { exit 1 }
