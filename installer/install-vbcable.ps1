[CmdletBinding()]
param(
    [switch]$Force
)

$ErrorActionPreference = 'Stop'
$ProgressPreference = 'SilentlyContinue'

$PackageUrl = 'https://download.vb-audio.com/Download_CABLE/VBCABLE_Driver_Pack45.zip'
$WorkRoot = Join-Path $env:TEMP 'MicVST-VBCABLE'
$ZipPath = Join-Path $WorkRoot 'VBCABLE_Driver_Pack45.zip'
$ExtractDir = Join-Path $WorkRoot 'package'

function Test-VBCablePresent {
    try {
        $devices = Get-PnpDevice -PresentOnly -ErrorAction Stop
        return [bool]($devices | Where-Object {
            $_.FriendlyName -like 'CABLE Input*' -or
            $_.FriendlyName -like 'CABLE Output*' -or
            $_.FriendlyName -like '*VB-Audio Virtual Cable*'
        } | Select-Object -First 1)
    }
    catch {
        # Fallback for stripped PowerShell environments: MMDevices are also visible
        # in the registry under the audio endpoint class. Presence here is enough to
        # avoid reinstalling a shared third-party driver unnecessarily.
        $roots = @(
            'HKLM:\SOFTWARE\Microsoft\Windows\CurrentVersion\MMDevices\Audio\Render',
            'HKLM:\SOFTWARE\Microsoft\Windows\CurrentVersion\MMDevices\Audio\Capture'
        )
        foreach ($root in $roots) {
            if (-not (Test-Path $root)) { continue }
            foreach ($key in Get-ChildItem $root -ErrorAction SilentlyContinue) {
                $props = Join-Path $key.PSPath 'Properties'
                $text = (Get-ItemProperty $props -ErrorAction SilentlyContinue | Out-String)
                if ($text -match 'CABLE (Input|Output)|VB-Audio Virtual Cable') { return $true }
            }
        }
        return $false
    }
}

$principal = New-Object Security.Principal.WindowsPrincipal([Security.Principal.WindowsIdentity]::GetCurrent())
if (-not $principal.IsInRole([Security.Principal.WindowsBuiltInRole]::Administrator)) {
    throw 'VB-CABLE installation must run elevated. Launch the MicVST installer as administrator.'
}

if ((-not $Force) -and (Test-VBCablePresent)) {
    Write-Host 'VB-CABLE is already installed; leaving the existing shared driver untouched.'
    exit 0
}

Remove-Item $WorkRoot -Recurse -Force -ErrorAction SilentlyContinue
New-Item -ItemType Directory -Force -Path $WorkRoot | Out-Null

Write-Host 'Downloading the official VB-CABLE package from VB-Audio...'
Invoke-WebRequest -Uri $PackageUrl -OutFile $ZipPath -UseBasicParsing

New-Item -ItemType Directory -Force -Path $ExtractDir | Out-Null
Expand-Archive -LiteralPath $ZipPath -DestinationPath $ExtractDir -Force

$setup = Get-ChildItem $ExtractDir -Recurse -Filter 'VBCABLE_Setup_x64.exe' | Select-Object -First 1
if (-not $setup) {
    throw 'The official VB-CABLE package did not contain VBCABLE_Setup_x64.exe.'
}

# Do not execute an unsigned/repacked download. We intentionally consume the
# official VB-Audio binary as-is rather than modifying or re-signing it.
$setupSignature = Get-AuthenticodeSignature -LiteralPath $setup.FullName
if ($setupSignature.Status -ne 'Valid' -or -not $setupSignature.SignerCertificate) {
    throw "VB-CABLE setup signature is not valid: $($setupSignature.Status)"
}

# Seed only the certificate that Windows just validated for the official package.
# This avoids the legacy 'Do you trust this publisher?' driver prompt during the
# hidden install. If this fails, installation may still succeed with a prompt.
try {
    $store = New-Object System.Security.Cryptography.X509Certificates.X509Store('TrustedPublisher', 'LocalMachine')
    $store.Open('ReadWrite')
    $store.Add($setupSignature.SignerCertificate)
    $store.Close()
    Write-Host "Trusted VB-CABLE publisher: $($setupSignature.SignerCertificate.Subject)"
}
catch {
    Write-Warning "Could not pre-seed the VB-CABLE publisher certificate: $($_.Exception.Message)"
}

Write-Host 'Installing VB-CABLE in hidden mode...'
$proc = Start-Process -FilePath $setup.FullName -ArgumentList '-i', '-h' -Wait -PassThru -WindowStyle Hidden
if ($proc.ExitCode -notin @(0, 1641, 3010)) {
    throw "VB-CABLE setup failed with exit code $($proc.ExitCode)."
}

# The audio endpoint can appear a few seconds after the setup process returns.
for ($i = 0; $i -lt 12; $i++) {
    Start-Sleep -Seconds 1
    if (Test-VBCablePresent) {
        Write-Host 'VB-CABLE installed; CABLE Input / CABLE Output are available.'
        Remove-Item $WorkRoot -Recurse -Force -ErrorAction SilentlyContinue
        exit 0
    }
}

Write-Warning 'VB-CABLE setup completed, but the endpoint is not visible yet. A Windows reboot is required.'
Remove-Item $WorkRoot -Recurse -Force -ErrorAction SilentlyContinue
exit 3010
