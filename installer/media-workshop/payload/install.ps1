$ErrorActionPreference = "Stop"

[Console]::OutputEncoding = New-Object Text.UTF8Encoding($false)

$packageDir = Split-Path -Parent $MyInvocation.MyCommand.Path
$zipPath = Join-Path $packageDir "obs-studio-media-workshop.zip"
$expectedHash = "A1F0A26C77A993B0A7B53DFDB361D61FEA75A7386BD5C7078DF9A4B5D1F900A6"
$defaultRoot = Join-Path $env:LOCALAPPDATA "OBS素材工作台"
$requestedRoot = if ($env:OBS_MEDIA_WORKSHOP_INSTALL_ROOT) { $env:OBS_MEDIA_WORKSHOP_INSTALL_ROOT } else { $defaultRoot }
if ([string]::IsNullOrWhiteSpace($requestedRoot)) {
    throw "安装目录不能为空。"
}
$installRoot = [IO.Path]::GetFullPath([Environment]::ExpandEnvironmentVariables($requestedRoot.Trim()))
$pathRoot = [IO.Path]::GetPathRoot($installRoot)
if ($installRoot.TrimEnd('\') -eq $pathRoot.TrimEnd('\')) {
    throw "不能直接安装到磁盘根目录，请选择一个专用文件夹。"
}
$windowsRoot = [IO.Path]::GetFullPath($env:WINDIR).TrimEnd('\')
if ($installRoot.TrimEnd('\') -eq $windowsRoot -or $installRoot.StartsWith($windowsRoot + '\', [StringComparison]::OrdinalIgnoreCase)) {
    throw "不能安装到 Windows 系统目录。"
}
$installPath = Join-Path $installRoot "obs-studio"
$stagingRoot = Join-Path $installRoot (".installing-" + [Guid]::NewGuid().ToString("N"))
$shortcutRoot = if ($env:OBS_MEDIA_WORKSHOP_SHORTCUT_ROOT) {
    $env:OBS_MEDIA_WORKSHOP_SHORTCUT_ROOT
} else {
    [Environment]::GetFolderPath("Desktop")
}
$shortcutPath = Join-Path $shortcutRoot "OBS素材工作台.lnk"
$backupPath = $null
$newInstallMoved = $false

if (-not (Test-Path -LiteralPath $zipPath)) {
    throw "安装数据不存在：$zipPath"
}

$actualHash = (Get-FileHash -LiteralPath $zipPath -Algorithm SHA256).Hash
if ($actualHash -ne $expectedHash) {
    throw "安装数据校验失败。期望：$expectedHash，实际：$actualHash"
}

if (Get-Process obs64,obs32,obs -ErrorAction SilentlyContinue) {
    throw "OBS 正在运行。请先关闭 OBS，再重新运行安装程序。"
}

if (Test-Path -LiteralPath $installPath -PathType Leaf) {
    throw "安装目录中的 obs-studio 不是文件夹：$installPath"
}
if ((Test-Path -LiteralPath $installPath -PathType Container) -and
    (Get-ChildItem -LiteralPath $installPath -Force | Select-Object -First 1) -and
    -not (Test-Path -LiteralPath (Join-Path $installPath "bin\64bit\obs64.exe") -PathType Leaf)) {
    throw "所选目录下已有非本软件管理的 obs-studio 文件夹，请更换安装目录。"
}

New-Item -ItemType Directory -Force -Path $installRoot | Out-Null
New-Item -ItemType Directory -Force -Path $shortcutRoot | Out-Null

try {
    Expand-Archive -LiteralPath $zipPath -DestinationPath $stagingRoot -Force
    $stagedInstall = Join-Path $stagingRoot "obs-studio"
    $stagedExe = Join-Path $stagedInstall "bin\64bit\obs64.exe"
    $stagedFFmpeg = Join-Path $stagedInstall "data\obs-studio\media-workshop\bin\ffmpeg.exe"
    $stagedPlugin = Join-Path $stagedInstall "obs-plugins\64bit\media-playlist-source.dll"
    if (-not (Test-Path -LiteralPath $stagedExe) -or
        -not (Test-Path -LiteralPath $stagedFFmpeg) -or
        -not (Test-Path -LiteralPath $stagedPlugin)) {
        throw "安装数据不完整，缺少 OBS、FFmpeg 或播放列表组件。"
    }

    if (Test-Path -LiteralPath $installPath) {
        $backupPath = Join-Path $installRoot ("obs-studio.backup-" + (Get-Date -Format "yyyyMMdd-HHmmss-fff"))
        Move-Item -LiteralPath $installPath -Destination $backupPath
    }
    Move-Item -LiteralPath $stagedInstall -Destination $installPath
    $newInstallMoved = $true

    $obsExe = Join-Path $installPath "bin\64bit\obs64.exe"
    $wsh = New-Object -ComObject WScript.Shell
    $shortcut = $wsh.CreateShortcut($shortcutPath)
    $shortcut.TargetPath = $obsExe
    $shortcut.WorkingDirectory = Split-Path -Parent $obsExe
    $shortcut.IconLocation = $obsExe
    $shortcut.Description = "OBS Studio 素材工作台"
    $shortcut.Save()

    $uninstallCmd = Join-Path $installRoot "卸载OBS素材工作台.cmd"
    $uninstallPs1 = Join-Path $installRoot "uninstall-obs-media-workshop.ps1"
    $shortcutPathForPs = $shortcutPath.Replace("'", "''")
    @"
`$ErrorActionPreference = "Stop"
if (Get-Process obs64,obs32,obs -ErrorAction SilentlyContinue) {
    throw "请先关闭 OBS。"
}
`$installPath = Join-Path `$PSScriptRoot "obs-studio"
Remove-Item -LiteralPath `$installPath -Recurse -Force
Remove-Item -LiteralPath '$shortcutPathForPs' -Force -ErrorAction SilentlyContinue
Write-Host "OBS 素材工作台已卸载。"
"@ | Set-Content -LiteralPath $uninstallPs1 -Encoding UTF8

    @"
@echo off
powershell.exe -NoProfile -ExecutionPolicy Bypass -File "%~dp0uninstall-obs-media-workshop.ps1"
if errorlevel 1 (
    echo Uninstall failed. See the error above.
    pause
    exit /b 1
)
echo Uninstall completed.
pause
"@ | Set-Content -LiteralPath $uninstallCmd -Encoding ASCII

    Write-Host "OBS 素材工作台安装完成。"
    Write-Host "安装目录：$installPath"
    Write-Host "桌面快捷方式：$shortcutPath"
    if ($env:OBS_MEDIA_WORKSHOP_NO_LAUNCH -ne "1") {
        $launchArguments = $env:OBS_MEDIA_WORKSHOP_LAUNCH_ARGUMENTS
        if ($launchArguments) {
            Start-Process -FilePath $obsExe -ArgumentList $launchArguments -WorkingDirectory (Split-Path -Parent $obsExe)
        } else {
            Start-Process -FilePath $obsExe -WorkingDirectory (Split-Path -Parent $obsExe)
        }
    }
}
catch {
    if ($newInstallMoved -and (Test-Path -LiteralPath $installPath)) {
        Remove-Item -LiteralPath $installPath -Recurse -Force -ErrorAction SilentlyContinue
    }
    if ($backupPath -and (Test-Path -LiteralPath $backupPath)) {
        Move-Item -LiteralPath $backupPath -Destination $installPath
    }
    throw
}
finally {
    if (Test-Path -LiteralPath $stagingRoot) {
        Remove-Item -LiteralPath $stagingRoot -Recurse -Force -ErrorAction SilentlyContinue
    }
}
