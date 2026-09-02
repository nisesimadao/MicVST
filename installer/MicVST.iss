#define MyAppName "MicVST"
#define MyAppVersion "1.3.0"
#define MyAppPublisher "MicVST"
#define MyAppExeName "MicVST.exe"

[Setup]
AppId={{EC4BA65D-5A2E-44AA-9F92-9B256B2A88D2}
AppName={#MyAppName}
AppVersion={#MyAppVersion}
AppPublisher={#MyAppPublisher}
DefaultDirName={autopf}\MicVST
DefaultGroupName=MicVST
DisableProgramGroupPage=yes
OutputDir=out
OutputBaseFilename=MicVST-Setup-{#MyAppVersion}
Compression=lzma2
SolidCompression=yes
ArchitecturesAllowed=x64compatible
ArchitecturesInstallIn64BitMode=x64compatible
PrivilegesRequired=admin
WizardStyle=modern
InfoBeforeFile=VB-CABLE-NOTICE.txt
RestartIfNeededByRun=yes
UninstallDisplayIcon={app}\{#MyAppExeName}

[Files]
Source: "..\build\MicVST_artefacts\Release\MicVST.exe"; DestDir: "{app}"; Flags: ignoreversion
Source: "install-vbcable.ps1"; DestDir: "{tmp}\MicVSTInstaller"; Flags: deleteafterinstall
Source: "VB-CABLE-NOTICE.txt"; DestDir: "{app}\licenses"; DestName: "VB-CABLE-NOTICE.txt"; Flags: ignoreversion
Source: "..\LICENSE"; DestDir: "{app}"; DestName: "LICENSE.txt"; Flags: ignoreversion

[Icons]
Name: "{autoprograms}\MicVST"; Filename: "{app}\{#MyAppExeName}"

[Tasks]
Name: "autostart"; Description: "Start MicVST with Windows (minimized to the system tray)"; GroupDescription: "Startup:"; Flags: unchecked

[Registry]
; Same value used by AutostartRegistry.cpp inside MicVST. --tray starts the engine
; without opening the main window; the tray icon can restore it with a click.
Root: HKCU; Subkey: "Software\Microsoft\Windows\CurrentVersion\Run"; ValueType: string; ValueName: "MicVST"; ValueData: """{app}\{#MyAppExeName}"" --tray"; Tasks: autostart; Flags: uninsdeletevalue

[Run]
; VB-Audio explicitly permits the base VB-CABLE package to be bundled/silently
; installed with another application while its donationware identity remains visible.
Filename: "{sys}\WindowsPowerShell\v1.0\powershell.exe"; Parameters: "-NoProfile -ExecutionPolicy Bypass -File ""{tmp}\MicVSTInstaller\install-vbcable.ps1"""; StatusMsg: "Installing VB-CABLE virtual microphone backend..."; Flags: runhidden waituntilterminated
Filename: "{app}\{#MyAppExeName}"; Description: "Launch MicVST"; Flags: nowait postinstall skipifsilent

[UninstallDelete]
; VB-CABLE is intentionally NOT removed here. It is a shared third-party component
; and another application may be using it.
Type: filesandordirs; Name: "{app}"

[Code]
procedure CurStepChanged(CurStep: TSetupStep);
begin
  { The task checkbox is authoritative. This also handles an upgrade where the user
    previously enabled startup from MicVST's tray menu but now unchecks it in Setup. }
  if (CurStep = ssPostInstall) and (not WizardIsTaskSelected('autostart')) then
    RegDeleteValue(HKCU,
      'Software\Microsoft\Windows\CurrentVersion\Run',
      'MicVST');
end;
