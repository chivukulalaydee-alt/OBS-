[CmdletBinding()]
param(
    [string]$Destination,
    [switch]$Force
)

$ErrorActionPreference = 'Stop'
$scriptRoot = Split-Path -Parent $MyInvocation.MyCommand.Path
if ([string]::IsNullOrWhiteSpace($Destination)) {
    $Destination = Join-Path $scriptRoot '..\..\.deps\vlc-runtime-3.0.23-win64'
}
$version = '3.0.23'
$archiveName = "vlc-$version-win64.zip"
$downloadUrl = "https://download.videolan.org/pub/videolan/vlc/$version/win64/$archiveName"
$expectedSha256 = '992D19DBD0B8A7CDE9167D2F7780B1EF6F92ACC8A71ACFA736101A21F35181E1'
$destinationPath = [System.IO.Path]::GetFullPath($Destination)
$dependencyRoot = [System.IO.Path]::GetDirectoryName($destinationPath)
$downloadRoot = Join-Path $dependencyRoot '.downloads'
$archivePath = Join-Path $downloadRoot $archiveName
$extractRoot = Join-Path $downloadRoot "vlc-$version-extract"
$extractedRuntime = Join-Path $extractRoot "vlc-$version"
$stagedRuntime = Join-Path $extractRoot 'obs-vlc-runtime'

function Test-VLCRuntime([string]$Path) {
    return (Test-Path -LiteralPath (Join-Path $Path 'libvlc.dll') -PathType Leaf) -and
        (Test-Path -LiteralPath (Join-Path $Path 'libvlccore.dll') -PathType Leaf) -and
        (Test-Path -LiteralPath (Join-Path $Path 'plugins') -PathType Container)
}

if ((Test-VLCRuntime $destinationPath) -and -not $Force) {
    Write-Output "VLC runtime $version is already available at $destinationPath"
    Write-Output $destinationPath
    exit 0
}

New-Item -ItemType Directory -Force -Path $downloadRoot | Out-Null

if (-not (Test-Path -LiteralPath $archivePath -PathType Leaf)) {
    $partialPath = "$archivePath.part"
    Remove-Item -LiteralPath $partialPath -Force -ErrorAction SilentlyContinue
    Invoke-WebRequest -UseBasicParsing -Uri $downloadUrl -OutFile $partialPath
    Move-Item -LiteralPath $partialPath -Destination $archivePath -Force
}

$actualSha256 = (Get-FileHash -LiteralPath $archivePath -Algorithm SHA256).Hash.ToUpperInvariant()
if ($actualSha256 -ne $expectedSha256) {
    throw "VLC archive SHA-256 mismatch. Expected $expectedSha256, got $actualSha256"
}

Remove-Item -LiteralPath $extractRoot -Recurse -Force -ErrorAction SilentlyContinue
New-Item -ItemType Directory -Force -Path $extractRoot | Out-Null
Expand-Archive -LiteralPath $archivePath -DestinationPath $extractRoot -Force

if (-not (Test-VLCRuntime $extractedRuntime)) {
    throw "The verified VLC archive does not contain the expected Windows runtime layout."
}

New-Item -ItemType Directory -Force -Path $stagedRuntime | Out-Null
foreach ($fileName in @('libvlc.dll', 'libvlccore.dll', 'COPYING.txt')) {
    Copy-Item -LiteralPath (Join-Path $extractedRuntime $fileName) -Destination $stagedRuntime
}
foreach ($directoryName in @('plugins', 'lua', 'locale', 'hrtfs')) {
    Copy-Item -LiteralPath (Join-Path $extractedRuntime $directoryName) -Destination $stagedRuntime -Recurse
}

if (-not (Test-VLCRuntime $stagedRuntime)) {
    throw "Failed to stage the minimal libVLC runtime."
}

if (Test-Path -LiteralPath $destinationPath) {
    Remove-Item -LiteralPath $destinationPath -Recurse -Force
}

Move-Item -LiteralPath $stagedRuntime -Destination $destinationPath
Remove-Item -LiteralPath $extractRoot -Recurse -Force

$manifest = [ordered]@{
    version = $version
    sourceCommit = '578d28f6c9f2379164516e689418f92ac74a3445'
    downloadUrl = $downloadUrl
    archiveSha256 = $expectedSha256
    downloadedAtUtc = [DateTime]::UtcNow.ToString('o')
}
$manifest | ConvertTo-Json | Set-Content -LiteralPath (Join-Path $destinationPath 'obs-vlc-runtime.json') -Encoding UTF8

Write-Output "Downloaded and verified VLC runtime $version."
Write-Output $destinationPath
