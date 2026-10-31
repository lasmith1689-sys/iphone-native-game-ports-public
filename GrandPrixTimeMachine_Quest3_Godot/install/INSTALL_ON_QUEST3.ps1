# Grand Prix Time Machine: Windows Quest 3 USB installer
# Launch via INSTALL_ON_QUEST3.cmd. Uses only Google's official Android platform-tools.
$ErrorActionPreference = 'Stop'
$ProgressPreference = 'SilentlyContinue'
$root = Split-Path -Parent $MyInvocation.MyCommand.Path
$apk = Join-Path $root 'GrandPrixTimeMachine_Quest3.apk'
$checksum = Join-Path $root 'SHA256SUMS.txt'

function Fail([string] $message) {
    Write-Host "`nERROR: $message" -ForegroundColor Red
    exit 1
}

Write-Host '==========================================' -ForegroundColor Cyan
Write-Host ' Grand Prix Time Machine  |  Quest 3' -ForegroundColor Cyan
Write-Host '==========================================' -ForegroundColor Cyan
Write-Host ''
if (-not (Test-Path -LiteralPath $apk -PathType Leaf)) {
    Fail 'The APK is not beside this installer. This is a preparation kit, not yet a playable app. Use the completed Easy Install download containing GrandPrixTimeMachine_Quest3.apk.'
}

# Ensure the file is complete before allowing Android installation.
if (Test-Path -LiteralPath $checksum -PathType Leaf) {
    $lines = Get-Content -LiteralPath $checksum
    $expected = $null
    foreach ($line in $lines) {
        if ($line -match '^([A-Fa-f0-9]{64})\s+\*?(.+)$') {
            $name = [IO.Path]::GetFileName($matches[2])
            if ($name -eq 'GrandPrixTimeMachine_Quest3.apk') {
                $expected = $matches[1].ToLowerInvariant()
                break
            }
        }
    }
    if (-not $expected) { Fail 'SHA256SUMS.txt does not list the expected APK.' }
    $actual = (Get-FileHash -LiteralPath $apk -Algorithm SHA256).Hash.ToLowerInvariant()
    if ($actual -ne $expected) { Fail 'APK checksum mismatch. Download the Easy Install ZIP again.' }
    Write-Host 'APK checksum: VERIFIED' -ForegroundColor Green
} else {
    Fail 'SHA256SUMS.txt is missing. Use the complete Easy Install ZIP, not a loose APK.'
}

$adbCommand = Get-Command adb.exe -ErrorAction SilentlyContinue
$adb = if ($adbCommand) { $adbCommand.Source } else { Join-Path $root 'platform-tools\adb.exe' }
if (-not (Test-Path -LiteralPath $adb -PathType Leaf)) {
    Write-Host 'Installing the official Google Android USB tool into this folder.'
    $url = 'https://dl.google.com/android/repository/platform-tools-latest-windows.zip'
    $tempZip = Join-Path $root 'google-platform-tools.zip'
    try {
        # PowerShell's HTTPS downloader validates Google's TLS certificate.
        Invoke-WebRequest -Uri $url -OutFile $tempZip -UseBasicParsing
        Expand-Archive -LiteralPath $tempZip -DestinationPath $root -Force
        Remove-Item -LiteralPath $tempZip -Force
    } catch {
        Fail "Could not download Google's platform-tools. Verify internet access or use SideQuest Advanced Installer. $($_.Exception.Message)"
    }
}
if (-not (Test-Path -LiteralPath $adb -PathType Leaf)) {
    Fail 'adb.exe is missing. Install Google Android Platform Tools or use SideQuest Advanced Installer.'
}

Write-Host ''
Write-Host '1. Turn ON your Quest 3 and connect it to this Windows PC using a USB-C DATA cable.'
Write-Host '2. Put on the headset. If it asks, choose Allow USB debugging (and optionally Always allow).'
Write-Host '3. Return to this computer.'
Write-Host ''
& $adb start-server | Out-Null
$deviceSerial = $null
for ($attempt = 1; $attempt -le 4; $attempt++) {
    $results = @(& $adb devices 2>&1 | ForEach-Object { "$_" })
    $connected = @($results | Where-Object { $_ -match '^([^\s]+)\s+device\s*$' })
    $unauthorized = @($results | Where-Object { $_ -match '^([^\s]+)\s+unauthorized\s*$' })
    if ($connected.Count -eq 1) {
        $deviceSerial = ($connected[0] -split '\s+')[0]
        break
    }
    if ($connected.Count -gt 1) { Fail 'More than one Android device is connected. Disconnect the extras, then retry.' }
    if ($unauthorized.Count -gt 0) {
        Write-Host 'Quest connected but USB debugging is not authorized. Put the headset on and tap Allow.' -ForegroundColor Yellow
    } else {
        Write-Host 'Quest not detected. Check Developer Mode, the USB data cable, and the headset USB prompt.' -ForegroundColor Yellow
    }
    if ($attempt -lt 4) { [void](Read-Host 'Once resolved, press ENTER to check again') }
}
if (-not $deviceSerial) { Fail 'Quest not ready. See READ_ME_FIRST.txt for quick troubleshooting or use Meta Quest Developer Hub.' }

Write-Host "`nQuest connected: $deviceSerial" -ForegroundColor Green
Write-Host 'Installing Grand Prix Time Machine ...'
$installOutput = @(& $adb -s $deviceSerial install -r $apk 2>&1 | ForEach-Object { "$_" })
$installSucceeded = ($LASTEXITCODE -eq 0 -and ($installOutput -contains 'Success'))
if (-not $installSucceeded -and (($installOutput -join "`n") -match 'INSTALL_FAILED_UPDATE_INCOMPATIBLE')) {
    Write-Host 'The earlier copy was signed with a different GitHub debug key.' -ForegroundColor Yellow
    Write-Host 'Removing it will delete ONLY this app and its local data.'
    Write-Host 'No other Quest apps or files will be touched.'
    $confirmation = Read-Host 'Type REPLACE to remove the older copy and install this version; Enter cancels'
    if ($confirmation -cne 'REPLACE') { Fail 'Nothing was removed. Update cancelled by user.' }
    $removeOutput = @(& $adb -s $deviceSerial uninstall com.gptimemachine.quest3 2>&1 | ForEach-Object { "$_" })
    if ($LASTEXITCODE -ne 0 -or -not ($removeOutput -contains 'Success')) {
        Fail ('Could not remove the older copy: ' + ($removeOutput -join '; '))
    }
    Write-Host 'Old app removed with permission. Installing this version ...'
    $installOutput = @(& $adb -s $deviceSerial install -r $apk 2>&1 | ForEach-Object { "$_" })
    $installSucceeded = ($LASTEXITCODE -eq 0 -and ($installOutput -contains 'Success'))
}
if (-not $installSucceeded) {
    Write-Host ($installOutput -join "`n") -ForegroundColor Yellow
    Fail 'Android did not complete installation. See READ_ME_FIRST.txt.'
}
$verify = @(& $adb -s $deviceSerial shell pm path com.gptimemachine.quest3 2>&1 | ForEach-Object { "$_" })
if ($LASTEXITCODE -ne 0 -or -not (($verify -join "`n") -match 'package:')) {
    Fail 'ADB reported success, but the app package could not be confirmed. Please check the headset app library.'
}
Write-Host ''
Write-Host 'SUCCESS: Grand Prix Time Machine is installed on your Quest 3!' -ForegroundColor Green
Write-Host 'In the headset: Apps -> filter/menu -> Unknown Sources -> Grand Prix Time Machine.'
Write-Host 'Use EITHER controller trigger to select floating buttons; B/Y recenters.'
Write-Host 'This app is independent of your gaming PC after installation.'
exit 0
