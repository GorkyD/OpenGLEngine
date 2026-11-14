param(
    [string]$Game = "Lad",
    [string]$Version = "0.1",
    [string]$Preset = "clang-cl-release-local",
    [string[]]$AssetDirs = @("Shaders", "Fonts", "Scenes"),
    [switch]$SkipBuild
)

$ErrorActionPreference = "Stop"
$root = Split-Path -Parent $PSScriptRoot
$buildDir = Join-Path $root "Build/$Preset"
$distDir = Join-Path $root "dist"
$gameDist = Join-Path $distDir $Game

if (-not $SkipBuild) {
    Push-Location $root
    cmake --preset $Preset
    if ($LASTEXITCODE -ne 0) { throw "configure failed" }
    cmake --build --preset $Preset --target $Game -j 8
    if ($LASTEXITCODE -ne 0) { throw "build failed" }
    Pop-Location
}

if (Test-Path $gameDist) { Remove-Item -Recurse -Force $gameDist }
New-Item -ItemType Directory -Force $gameDist | Out-Null

Copy-Item (Join-Path $buildDir "$Game.exe") $gameDist
Get-ChildItem $buildDir -Filter *.dll | Copy-Item -Destination $gameDist
$assetsDist = Join-Path $gameDist "Assets"
New-Item -ItemType Directory -Force $assetsDist | Out-Null
foreach ($dir in $AssetDirs) {
    $source = Join-Path $buildDir "Assets/$dir"
    if (Test-Path $source) { Copy-Item -Recurse $source (Join-Path $assetsDist $dir) }
}
Copy-Item (Join-Path $buildDir "Assets/CREDITS.md") $assetsDist -ErrorAction SilentlyContinue

$zip = Join-Path $distDir "$Game-$Version.zip"
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

if ((Test-Path $iss) -and $iscc) {
    & $iscc "/DSourceDir=$gameDist" "/DOutputDir=$distDir" "/DAppVersion=$Version" $iss
    if ($LASTEXITCODE -ne 0) { throw "installer build failed" }
    Write-Host "Installer written to $distDir"
} elseif (-not $iscc) {
    Write-Host "Inno Setup not found: only the portable archive was made (winget install JRSoftware.InnoSetup)"
}
