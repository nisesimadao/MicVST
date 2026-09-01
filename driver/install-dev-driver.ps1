param(
    [ValidateSet("Debug", "Release")]
    [string]$Configuration = "Release",

    [ValidateSet("x64", "ARM64")]
    [string]$Platform = "x64"
)

$ErrorActionPreference = "Stop"

$repoRoot = Split-Path -Parent $PSScriptRoot
$WorkDir = Join-Path $PSScriptRoot "work\Virtual-Audio-Driver"
$packageDir = Join-Path $WorkDir "$Platform\$Configuration\package"
$inf = Join-Path $packageDir "VirtualAudioDriver.inf"
$toolBuild = Join-Path $repoRoot "build-driver-tools"

if (-not (Test-Path $inf)) {
    throw "Built INF not found at $inf. Run build-driver.ps1 first."
}

$principal = New-Object Security.Principal.WindowsPrincipal([Security.Principal.WindowsIdentity]::GetCurrent())
if (-not $principal.IsInRole([Security.Principal.WindowsBuiltInRole]::Administrator)) {
    throw "Run this script from an elevated PowerShell window."
}

$installer = Get-ChildItem -Path $toolBuild -Recurse -Filter "MicVSTDriverInstaller.exe" -ErrorAction SilentlyContinue |
             Select-Object -First 1 -ExpandProperty FullName

if (-not $installer) {
    if (-not (Get-Command cmake -ErrorAction SilentlyContinue)) {
        throw "CMake is required to build MicVSTDriverInstaller."
    }

    Write-Host "Building MicVSTDriverInstaller..."
    & cmake -S $repoRoot -B $toolBuild -A $Platform
    if ($LASTEXITCODE -ne 0) { throw "CMake configure failed with exit code $LASTEXITCODE" }

    & cmake --build $toolBuild --config Release --target MicVSTDriverInstaller
    if ($LASTEXITCODE -ne 0) { throw "MicVSTDriverInstaller build failed with exit code $LASTEXITCODE" }

    $installer = Get-ChildItem -Path $toolBuild -Recurse -Filter "MicVSTDriverInstaller.exe" |
                 Select-Object -First 1 -ExpandProperty FullName
}

if (-not $installer -or -not (Test-Path $installer)) {
    throw "MicVSTDriverInstaller.exe was not produced."
}

Write-Warning "This is a DEVELOPMENT install. The custom driver is not production-signed. Windows test-signing or an appropriately trusted test certificate is required."
Write-Host "Installing ROOT\MicVSTVirtualAudio without Device Manager/devcon..."
& $installer install $inf
if ($LASTEXITCODE -ne 0) {
    throw "MicVSTDriverInstaller failed with exit code $LASTEXITCODE"
}

Write-Host ""
Write-Host "Installed endpoints:"
Write-Host "  Output (internal): MicVST Internal Output"
Write-Host "  Input:             MicVST Microphone"
