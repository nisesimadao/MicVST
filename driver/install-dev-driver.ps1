param(
    [ValidateSet("Debug", "Release")]
    [string]$Configuration = "Release",

    [ValidateSet("x64", "ARM64")]
    [string]$Platform = "x64"
)

$ErrorActionPreference = "Stop"

$WorkDir = Join-Path $PSScriptRoot "work\Virtual-Audio-Driver"
$packageDir = Join-Path $WorkDir "$Platform\$Configuration\package"
$inf = Join-Path $packageDir "VirtualAudioDriver.inf"

if (-not (Test-Path $inf)) {
    throw "Built INF not found at $inf. Run build-driver.ps1 first."
}

$principal = New-Object Security.Principal.WindowsPrincipal([Security.Principal.WindowsIdentity]::GetCurrent())
if (-not $principal.IsInRole([Security.Principal.WindowsBuiltInRole]::Administrator)) {
    throw "Run this script from an elevated PowerShell window."
}

Write-Warning "This is a DEVELOPMENT install. The custom driver is not production-signed. Windows test-signing or an appropriately trusted test certificate is required."
Write-Host "Adding driver package to the Driver Store..."
& pnputil.exe /add-driver $inf /install
if ($LASTEXITCODE -ne 0) {
    throw "pnputil failed with exit code $LASTEXITCODE"
}

Write-Host ""
Write-Host "The package is in the Driver Store. If the ROOT\MicVSTVirtualAudio devnode does not already exist, use the WDK devcon tool to create it during development:"
Write-Host "  devcon install `"$inf`" ROOT\MicVSTVirtualAudio"
Write-Host ""
Write-Host "The production installer will create the root-enumerated devnode automatically; this helper intentionally does not bundle devcon."
