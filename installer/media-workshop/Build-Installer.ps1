param(
    [Parameter(Mandatory = $true)]
    [string]$OutputExe,
    [string]$CscPath = "C:\Windows\Microsoft.NET\Framework64\v4.0.30319\csc.exe",
    [switch]$KeepPayloadArchive
)

$ErrorActionPreference = "Stop"
$projectRoot = Split-Path -Parent $MyInvocation.MyCommand.Path
$sourcePath = Join-Path $projectRoot "InstallerBootstrap.cs"
$payloadRoot = Join-Path $projectRoot "payload"
$installScript = Join-Path $payloadRoot "install.ps1"
$installCommand = Join-Path $payloadRoot "install.cmd"
$obsArchive = Join-Path $payloadRoot "obs-studio-media-workshop.zip"
$expectedObsHash = "A1F0A26C77A993B0A7B53DFDB361D61FEA75A7386BD5C7078DF9A4B5D1F900A6"

foreach ($requiredPath in @($CscPath, $sourcePath, $installScript, $installCommand, $obsArchive)) {
    if (-not (Test-Path -LiteralPath $requiredPath -PathType Leaf)) {
        throw "缺少构建文件：$requiredPath"
    }
}

$actualObsHash = (Get-FileHash -LiteralPath $obsArchive -Algorithm SHA256).Hash
if ($actualObsHash -ne $expectedObsHash) {
    throw "OBS payload 哈希不匹配。期望：$expectedObsHash，实际：$actualObsHash"
}

$scriptBytes = [IO.File]::ReadAllBytes($installScript)
if ($scriptBytes.Length -lt 3 -or $scriptBytes[0] -ne 0xEF -or $scriptBytes[1] -ne 0xBB -or $scriptBytes[2] -ne 0xBF) {
    throw "install.ps1 必须使用 UTF-8 BOM，以兼容 Windows PowerShell 5.1。"
}

$outputDirectory = Split-Path -Parent ([IO.Path]::GetFullPath($OutputExe))
New-Item -ItemType Directory -Force -Path $outputDirectory | Out-Null
$payloadArchive = Join-Path $outputDirectory ("bootstrap-payload-" + [Guid]::NewGuid().ToString("N") + ".zip")

Add-Type -AssemblyName System.IO.Compression
Add-Type -AssemblyName System.IO.Compression.FileSystem

try {
    $stream = [IO.File]::Open($payloadArchive, [IO.FileMode]::CreateNew, [IO.FileAccess]::ReadWrite, [IO.FileShare]::None)
    try {
        $archive = New-Object IO.Compression.ZipArchive($stream, [IO.Compression.ZipArchiveMode]::Create, $true)
        try {
            [IO.Compression.ZipFileExtensions]::CreateEntryFromFile(
                $archive, $installScript, "install.ps1", [IO.Compression.CompressionLevel]::NoCompression) | Out-Null
            [IO.Compression.ZipFileExtensions]::CreateEntryFromFile(
                $archive, $installCommand, "install.cmd", [IO.Compression.CompressionLevel]::NoCompression) | Out-Null
            [IO.Compression.ZipFileExtensions]::CreateEntryFromFile(
                $archive, $obsArchive, "obs-studio-media-workshop.zip", [IO.Compression.CompressionLevel]::NoCompression) | Out-Null
        }
        finally {
            $archive.Dispose()
        }
    }
    finally {
        $stream.Dispose()
    }

    $compilerArguments = @(
        "/nologo",
        "/target:winexe",
        "/platform:x64",
        "/optimize+",
        "/out:$OutputExe",
        "/reference:System.Windows.Forms.dll",
        "/reference:System.Drawing.dll",
        "/reference:System.IO.Compression.dll",
        "/reference:System.IO.Compression.FileSystem.dll",
        "/resource:$payloadArchive,ObsMediaWorkshopPayload",
        $sourcePath
    )
    & $CscPath $compilerArguments
    if ($LASTEXITCODE -ne 0) {
        throw "C# 编译失败，退出码：$LASTEXITCODE"
    }

    $outputItem = Get-Item -LiteralPath $OutputExe
    $outputHash = (Get-FileHash -LiteralPath $OutputExe -Algorithm SHA256).Hash
    Write-Output "OUTPUT=$($outputItem.FullName)"
    Write-Output "SIZE=$($outputItem.Length)"
    Write-Output "SHA256=$outputHash"
    Write-Output "OBS_PAYLOAD_SHA256=$actualObsHash"
}
finally {
    if (-not $KeepPayloadArchive -and (Test-Path -LiteralPath $payloadArchive)) {
        Remove-Item -LiteralPath $payloadArchive -Force
    }
}
