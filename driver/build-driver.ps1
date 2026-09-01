param(
    [ValidateSet("Debug", "Release")]
    [string]$Configuration = "Release",

    [ValidateSet("x64", "ARM64")]
    [string]$Platform = "x64",

    [switch]$SkipPrepare
)

$ErrorActionPreference = "Stop"
$WorkDir = Join-Path $PSScriptRoot "work\Virtual-Audio-Driver"

if (-not $SkipPrepare) {
    & (Join-Path $PSScriptRoot "prepare-driver.ps1") -WorkDir $WorkDir
    if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
}

$msbuild = Get-Command msbuild -ErrorAction SilentlyContinue
if (-not $msbuild) {
    $vswhere = "${env:ProgramFiles(x86)}\Microsoft Visual Studio\Installer\vswhere.exe"
    if (Test-Path $vswhere) {
        $candidate = & $vswhere -latest -products * -requires Microsoft.Component.MSBuild -find "MSBuild\**\Bin\MSBuild.exe" | Select-Object -First 1
        if ($candidate) { $msbuild = Get-Item $candidate }
    }
}

if (-not $msbuild) {
    throw "MSBuild was not found. Install Visual Studio 2022 with Desktop C++ and Windows Driver Kit 11."
}

$solution = Join-Path $WorkDir "VirtualAudioDriver.sln"
if (-not (Test-Path $solution)) {
    throw "Prepared driver solution not found: $solution"
}

Write-Host "Building MicVST virtual audio driver ($Configuration / $Platform)..."
& $msbuild.Source $solution "/m" "/p:Configuration=$Configuration" "/p:Platform=$Platform"
if ($LASTEXITCODE -ne 0) {
    throw "Driver build failed with exit code $LASTEXITCODE"
}

$packageDir = Join-Path $WorkDir "$Platform\$Configuration\package"
Write-Host ""
Write-Host "Build complete. Package directory: $packageDir"
if (Test-Path $packageDir) {
    Get-ChildItem $packageDir | Format-Table Name, Length
}
