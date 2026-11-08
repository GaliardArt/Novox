; NOVOX 1.0.0 optional Inno Setup 6 installer.
; Run from the repository root after: cmake --install build --config Release --prefix dist

#define AppName "NOVOX"
#define AppVersion "1.0.0"
#define AppPublisher "NOVOX Project"

[Setup]
AppId={{B3F3D52A-42CF-4B1A-A2F8-9F0A6F0C1D10}
AppName={#AppName}
AppVersion={#AppVersion}
AppPublisher={#AppPublisher}
DefaultDirName={autopf}\NOVOX
DefaultGroupName=NOVOX
DisableProgramGroupPage=yes
PrivilegesRequired=lowest
ArchitecturesAllowed=x64
ArchitecturesInstallIn64BitMode=x64
OutputDir=dist-installer
OutputBaseFilename=NOVOX-Setup-{#AppVersion}-Windows-x64
Compression=lzma2
SolidCompression=yes
WizardStyle=modern
UninstallDisplayIcon={app}\NOVOX.exe

[Tasks]
Name: "desktopicon"; Description: "Create a desktop shortcut"; GroupDescription: "Additional shortcuts:"; Flags: unchecked

[Files]
Source: "dist\NOVOX.exe"; DestDir: "{app}"; Flags: ignoreversion
Source: "dist\docs\*"; DestDir: "{app}\docs"; Flags: ignoreversion recursesubdirs createallsubdirs

[Icons]
Name: "{group}\NOVOX"; Filename: "{app}\NOVOX.exe"
Name: "{autodesktop}\NOVOX"; Filename: "{app}\NOVOX.exe"; Tasks: desktopicon

[Run]
Filename: "{app}\NOVOX.exe"; Description: "Launch NOVOX"; Flags: postinstall nowait skipifsilent
