[CmdletBinding()]
param(
    [switch]$SkipBuild,
    [switch]$NoStageVerify,
    [string]$ExecCmds = '',
    [string]$Map = '',
    [int]$TimeoutSeconds = 300,
    [string]$EngineDir = 'C:\Program Files\Epic Games\UE_5.8'
)

$ErrorActionPreference = 'Stop'
$Project = Join-Path $PSScriptRoot 'InBang\InBang.uproject'
$OutDir = Join-Path $PSScriptRoot 'InBang\Saved\Verify'
$BuildLog = Join-Path $OutDir 'build.log'
$RunLog = Join-Path $OutDir 'run.log'
New-Item -ItemType Directory -Force $OutDir | Out-Null

if (-not $SkipBuild) {
    $BuildWatch = [Diagnostics.Stopwatch]::StartNew()
    & (Join-Path $EngineDir 'Engine\Build\BatchFiles\Build.bat') InBangEditor Win64 Development "-Project=$Project" -WaitMutex -NoHotReloadFromIDE *> $BuildLog
    $BuildExit = $LASTEXITCODE
    $BuildSeconds = [int]$BuildWatch.Elapsed.TotalSeconds
    if ($BuildExit -ne 0) {
        Write-Output "BUILD FAIL exit=$BuildExit seconds=$BuildSeconds log=$BuildLog"
        Get-Content $BuildLog -Encoding UTF8 | Select-String -Pattern ': error |: fatal error |error LNK|ERROR:' | Select-Object -First 20 | ForEach-Object { $_.Line.Trim() }
        exit 2
    }
    Write-Output "BUILD ok seconds=$BuildSeconds"
}

$RunArgs = @("`"$Project`"")
if ($Map) { $RunArgs += $Map }
$RunArgs += @('-game', '-nullrhi', '-nosound', '-unattended', '-nosplash', '-NoVerifyGC', "-abslog=`"$RunLog`"")
if (-not $NoStageVerify) { $RunArgs += '-InBangVerify' }
if ($ExecCmds) { $RunArgs += "-ExecCmds=`"$ExecCmds`"" }

if (Test-Path $RunLog) { Remove-Item $RunLog -Force }
$RunWatch = [Diagnostics.Stopwatch]::StartNew()
$Process = Start-Process -FilePath (Join-Path $EngineDir 'Engine\Binaries\Win64\UnrealEditor-Cmd.exe') -ArgumentList $RunArgs -PassThru -WindowStyle Hidden
if (-not $Process.WaitForExit($TimeoutSeconds * 1000)) {
    Stop-Process -Id $Process.Id -Force
    Write-Output "RUN TIMEOUT seconds=$TimeoutSeconds log=$RunLog"
}
$RunSeconds = [int]$RunWatch.Elapsed.TotalSeconds
Write-Output "RUN exit=$($Process.ExitCode) seconds=$RunSeconds log=$RunLog"

if (-not (Test-Path $RunLog)) {
    Write-Output 'RUN no log produced'
    exit 3
}

$Lines = Get-Content $RunLog -Encoding UTF8
$Lines | Select-String -Pattern 'Fatal error|Unhandled Exception|Assertion failed|LogWindows: Error' | Select-Object -First 10 | ForEach-Object { 'ENGINE ' + $_.Line.Trim() }
$Lines | Select-String -Pattern 'LogInBang: ' | ForEach-Object { ($_.Line -replace '^\[[^\]]*\]\[[^\]]*\]', '') -replace '^LogInBang: (Warning: |Error: )?', '$1' }

if ($NoStageVerify) { exit 0 }
$Result = $Lines | Select-String -Pattern 'VERIFY result=(\w+)' | Select-Object -Last 1
if ($Result -and $Result.Matches[0].Groups[1].Value -eq 'PASS') { exit 0 }
exit 1
