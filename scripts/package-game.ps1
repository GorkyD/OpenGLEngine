param(
    [string]$Game = "Lad",
    [string]$Target = "",
    [string]$Version = "0.1",
    [string]$Preset = "clang-cl-release-local",
    [string[]]$AssetDirs = @("Shaders", "Fonts", "Scenes", "Sounds"),
    [switch]$SkipBuild,
    [switch]$NoInstaller
)

$ErrorActionPreference = "Stop"
$root = Split-Path -Parent $PSScriptRoot
$buildDir = Join-Path $root "Build/$Preset"
$distDir = Join-Path $root "dist"
if (-not $Target) { $Target = $Game }
$gameDist = Join-Path $distDir $Target
$gameAssets = Join-Path $root "OpenGLEngine/Game/$Game/Assets"

if (-not $SkipBuild) {
    Push-Location $root
    cmake --preset $Preset
    if ($LASTEXITCODE -ne 0) { throw "configure failed" }
    cmake --build --preset $Preset --target $Target -j 8
    if ($LASTEXITCODE -ne 0) { throw "build failed" }
    Pop-Location
}

if (Test-Path $gameDist) { Remove-Item -Recurse -Force $gameDist }
New-Item -ItemType Directory -Force $gameDist | Out-Null

Copy-Item (Join-Path $buildDir "$Target.exe") $gameDist
Set-Content -Path (Join-Path $gameDist "user_data.txt") -Value "saves and settings live in %APPDATA%\Muzremont90" -Encoding UTF8
Get-ChildItem $buildDir -Filter *.dll | Copy-Item -Destination $gameDist
$crt = Get-ChildItem -Directory -ErrorAction SilentlyContinue @(
    "${env:ProgramFiles(x86)}\Microsoft Visual Studio\*\*\VC\Redist\MSVC\*\x64\Microsoft.VC143.CRT",
    "$env:ProgramFiles\Microsoft Visual Studio\*\*\VC\Redist\MSVC\*\x64\Microsoft.VC143.CRT"
) | Sort-Object FullName -Descending | Select-Object -First 1
if ($crt) {
    foreach ($dll in @("msvcp140.dll", "vcruntime140.dll", "vcruntime140_1.dll")) {
        Copy-Item (Join-Path $crt.FullName $dll) $gameDist -ErrorAction SilentlyContinue
    }
} else {
    Write-Host "VC++ runtime not found: players need the Visual C++ 2015-2022 redistributable"
}
$assetsDist = Join-Path $gameDist "Assets"
New-Item -ItemType Directory -Force $assetsDist | Out-Null
foreach ($dir in $AssetDirs) {
    $source = Join-Path $buildDir "Assets/$dir"
    if (Test-Path $source) {
        $destination = Join-Path $assetsDist $dir
        New-Item -ItemType Directory -Force (Split-Path -Parent $destination) | Out-Null
        Copy-Item -Recurse $source $destination
    }
}
Copy-Item (Join-Path $buildDir "Assets/CREDITS.md") $assetsDist -ErrorAction SilentlyContinue
if (Test-Path $gameAssets) {
    $gameAssets = (Resolve-Path $gameAssets).Path
    Get-ChildItem $gameAssets -Recurse -File | ForEach-Object {
        $relative = $_.FullName.Substring($gameAssets.Length).TrimStart([char]92, [char]47)
        $destination = Join-Path $assetsDist $relative
        New-Item -ItemType Directory -Force (Split-Path -Parent $destination) | Out-Null
        Copy-Item $_.FullName $destination -Force
    }
}

$zip = Join-Path $distDir "$Target-$Version.zip"
if (Test-Path $zip) { Remove-Item -Force $zip }
Compress-Archive -Path (Join-Path $gameDist "*") -DestinationPath $zip
Write-Host "Portable archive: $zip"

$iss = Join-Path $root "OpenGLEngine/Game/$Game/Installer/$Game.iss"
$iscc = @(
    (Get-Command iscc -ErrorAction SilentlyContinue | Select-Object -ExpandProperty Source),
    "$env:LOCALAPPDATA\Programs\Inno Setup 6\ISCC.exe",
    "${env:ProgramFiles(x86)}\Inno Setup 6\ISCC.exe",
    "$env:ProgramFiles\Inno Setup 6\ISCC.exe"
) | Where-Object { $_ -and (Test-Path $_) } | Select-Object -First 1

if ($NoInstaller -or $Target -ne $Game) {
    Write-Host "Installer skipped for $Target"
} elseif ((Test-Path $iss) -and $iscc) {
    & $iscc "/DSourceDir=$gameDist" "/DOutputDir=$distDir" "/DAppVersion=$Version" $iss
    if ($LASTEXITCODE -ne 0) { throw "installer build failed" }
    Write-Host "Installer written to $distDir"
} elseif (-not $iscc) {
    Write-Host "Inno Setup not found: only the portable archive was made (winget install JRSoftware.InnoSetup)"
}
