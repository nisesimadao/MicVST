param(
    [string]$WorkDir = (Join-Path $PSScriptRoot "work\Virtual-Audio-Driver")
)

$ErrorActionPreference = "Stop"

$Upstream = "https://github.com/VirtualDrivers/Virtual-Audio-Driver.git"
$Commit = "bb34fba15faf569a6ae9bdea360bc1cf4821354e"
$Patches = @(
    (Join-Path $PSScriptRoot "patches\micvst-routing.patch"),
    (Join-Path $PSScriptRoot "patches\micvst-win11.patch")
)

function Invoke-Git {
    param([Parameter(ValueFromRemainingArguments = $true)][string[]]$Args)
    & git @Args
    if ($LASTEXITCODE -ne 0) {
        throw "git $($Args -join ' ') failed with exit code $LASTEXITCODE"
    }
}

if (-not (Get-Command git -ErrorAction SilentlyContinue)) {
    throw "Git for Windows is required to prepare the MicVST driver source."
}

foreach ($patch in $Patches) {
    if (-not (Test-Path $patch)) {
        throw "Driver patch not found: $patch"
    }
}

$parent = Split-Path -Parent $WorkDir
New-Item -ItemType Directory -Force -Path $parent | Out-Null

if (-not (Test-Path (Join-Path $WorkDir ".git"))) {
    Write-Host "Cloning Virtual-Audio-Driver..."
    Invoke-Git clone $Upstream $WorkDir
}

Push-Location $WorkDir
try {
    Invoke-Git fetch origin $Commit
    Invoke-Git checkout --detach $Commit
    Invoke-Git reset --hard $Commit
    Invoke-Git clean -fdx

    $actual = (& git rev-parse HEAD).Trim()
    if ($actual -ne $Commit) {
        throw "Unexpected upstream commit. Expected $Commit, got $actual"
    }

    foreach ($patch in $Patches) {
        Write-Host "Applying $(Split-Path -Leaf $patch)..."
        Invoke-Git apply --check $patch
        Invoke-Git apply $patch
    }

    Write-Host ""
    Write-Host "MicVST driver source prepared at: $WorkDir"
    Write-Host "Pinned upstream commit: $Commit"
}
finally {
    Pop-Location
}
