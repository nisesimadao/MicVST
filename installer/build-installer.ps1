param(
    [string]$Configuration = 'Release'
)

$ErrorActionPreference = 'Stop'
$RepoRoot = Split-Path -Parent $PSScriptRoot
$AppExe = Join-Path $RepoRoot "build\MicVST_artefacts\$Configuration\MicVST.exe"
$IssFile = Join-Path $PSScriptRoot 'MicVST.iss'

if (-not (Test-Path $AppExe)) {
    throw "MicVST.exe not found at $AppExe. Build the Release app first."
}

$candidates = @(
    "$env:ProgramFiles(x86)\Inno Setup 6\ISCC.exe",
    "$env:ProgramFiles\Inno Setup 6\ISCC.exe"
)

$iscc = $candidates | Where-Object { Test-Path $_ } | Select-Object -First 1
if (-not $iscc) {
    $cmd = Get-Command ISCC.exe -ErrorAction SilentlyContinue
    if ($cmd) { $iscc = $cmd.Source }
}

if (-not $iscc) {
    throw 'Inno Setup 6 was not found. Install it from https://jrsoftware.org/isinfo.php'
}

Push-Location $PSScriptRoot
try {
    & $iscc $IssFile
    if ($LASTEXITCODE -ne 0) {
        throw "Inno Setup failed with exit code $LASTEXITCODE"
    }
}
finally {
    Pop-Location
}

Write-Host "Installer built under $(Join-Path $PSScriptRoot 'out')"
