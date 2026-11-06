# Grand Prix Time Machine - optional USB startup smoke check (Windows PowerShell 5.1)
# Local-only report: never uploads logs or installs/removes apps.
$ErrorActionPreference = 'Stop'
$root = Split-Path -Parent $MyInvocation.MyCommand.Path
$package = 'com.gptimemachine.quest3'
function Fail([string] $message) {
    Write-Host ('ERROR: ' + $message) -ForegroundColor Red
    exit 1
}
Write-Host 'Grand Prix Time Machine | Quest 3 startup check' -ForegroundColor Cyan
$adbCommand = Get-Command adb.exe -ErrorAction SilentlyContinue
$adb = if ($adbCommand) { $adbCommand.Source } else { Join-Path $root 'platform-tools\adb.exe' }
if (-not (Test-Path -LiteralPath $adb -PathType Leaf)) {
    Fail 'ADB not found. Run INSTALL_ON_QUEST3.cmd first to install official Google platform-tools.'
}
$deviceLines = @(& $adb devices 2>&1 | ForEach-Object { [string]$_ })
$connected = @($deviceLines | Where-Object { $_ -match '^([^\s]+)\s+device\s*$' })
$unauthorized = @($deviceLines | Where-Object { $_ -match '^([^\s]+)\s+unauthorized\s*$' })
if ($unauthorized.Count -gt 0) { Fail 'USB debugging not authorized. Approve the prompt in the Quest, then retry.' }
if ($connected.Count -ne 1) { Fail 'Connect exactly one authorized Quest by a USB-C data cable.' }
$deviceSerial = ($connected[0] -split '\s+')[0]
$packagePaths = @(& $adb -s $deviceSerial shell pm path $package 2>&1 | ForEach-Object { [string]$_ })
if ($LASTEXITCODE -ne 0 -or -not (($packagePaths -join ' ') -match 'package:')) {
    Fail 'Grand Prix Time Machine is not installed. Run INSTALL_ON_QUEST3.cmd first.'
}
Write-Host 'Installed package found. Attempting to launch on the connected headset.'
$launchOutput = @(& $adb -s $deviceSerial shell monkey -p $package -c android.intent.category.LAUNCHER 1 2>&1 | ForEach-Object { [string]$_ })
$launchExit = $LASTEXITCODE
Start-Sleep -Seconds 5
$pidLines = @(& $adb -s $deviceSerial shell pidof $package 2>&1 | ForEach-Object { [string]$_ })
$pidText = ($pidLines -join ' ').Trim()
$validPids = @($pidText -split '\s+' | Where-Object { $_ -match '^\d+$' })
$appPid = if ($validPids.Count -gt 0) { $validPids[0] } else { '' }
$report = Join-Path $root ('QUEST3_STARTUP_REPORT_' + (Get-Date -Format 'yyyyMMdd_HHmmss') + '.txt')
$lines = @(
    'Grand Prix Time Machine - local Quest startup report'
    ('Created: ' + (Get-Date -Format 'yyyy-MM-dd HH:mm:ss'))
    'Package: com.gptimemachine.quest3'
    ('Launch command exit code: ' + $launchExit)
    ('App process present after 5 seconds: ' + [bool]($appPid -ne ''))
    'The process check does not prove OpenXR rendering, controller tracking, passthrough or frame rate.'
    'No device serial, account details, or system-wide logs are collected.'
    ''
    'Launch command output:'
) + $launchOutput
if ($appPid -ne '') {
    Write-Host 'App process detected; capturing ONLY its own Android logs locally.'
    $appLogs = @(& $adb -s $deviceSerial logcat "--pid=$appPid" -d -t 300 -v time 2>&1 | ForEach-Object { [string]$_ })
    $lines += ''
    $lines += 'App-process logcat (last 300 entries, if supported):'
    $lines += $appLogs
} else {
    $lines += ''
    $lines += 'No app process found after five seconds. App may have exited, crashed or failed to launch.'
    $lines += 'To diagnose an early exit, capture logs separately using official Meta Quest Developer Hub.'
}
$lines | Set-Content -LiteralPath $report -Encoding UTF8
Write-Host ('Report saved locally: ' + $report) -ForegroundColor Green
Write-Host 'Nothing was uploaded. Review the file before sharing it.'
if ($appPid -eq '') { Fail 'Startup not confirmed. Read the local report. The app is not declared headset-tested.' }
Write-Host 'Android process observed. Check the picture and controllers inside your headset.'
exit 0
