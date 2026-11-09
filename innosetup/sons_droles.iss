; Inno Setup Script for Sons Drôles
; SEE THE DOCUMENTATION FOR DETAILS ON CREATING INNO SETUP SCRIPT FILES!

#define MyAppName "Sons Drôles"
#define MyAppVersion "1.0"
#define MyAppPublisher "FromTheRags"
#define MyAppURL "https://github.com/FromTheRags/sons-droles"
#define MyAppExeName "sons-droles.exe"

[Setup]
; NOTE: The value of AppId uniquely identifies this application.
AppId={{BFB1691F-B18B-4351-9E0A-A0FB91581901}
AppName={#MyAppName}
AppVersion={#MyAppVersion}
AppPublisher={#MyAppPublisher}
AppPublisherURL={#MyAppURL}
AppSupportURL={#MyAppURL}
AppUpdatesURL={#MyAppURL}
DefaultDirName={autopf}\{#MyAppName}
DefaultGroupName={#MyAppName}
OutputDir=..\dist
OutputBaseFilename=sons-droles-setup-v{#MyAppVersion}
SetupIconFile=..\assets\sons.ico
Compression=lzma2/max
SolidCompression=yes
WizardStyle=modern

[Languages]
Name: "french"; MessagesFile: "compiler:Languages\French.isl"
Name: "english"; MessagesFile: "compiler:Default.isl"

[Tasks]
Name: "desktopicon"; Description: "{cm:CreateDesktopIcon}"; GroupDescription: "{cm:AdditionalIcons}"; Flags: unchecked

[Files]
Source: "..\dist\sons-droles-windows\{#MyAppExeName}"; DestDir: "{app}"; Flags: ignoreversion
Source: "..\dist\sons-droles-windows\assets\*"; DestDir: "{app}\assets"; Flags: ignoreversion recursesubdirs
Source: "..\dist\sons-droles-windows\assets\*"; DestDir: "{app}"; Flags: ignoreversion
Source: "..\dist\sons-droles-windows\*.dll"; DestDir: "{app}"; Flags: ignoreversion
Source: "..\dist\sons-droles-windows\LICENSE"; DestDir: "{app}"; Flags: ignoreversion
Source: "..\dist\sons-droles-windows\OFL.txt"; DestDir: "{app}"; Flags: ignoreversion
Source: "..\dist\sons-droles-windows\README.md"; DestDir: "{app}"; Flags: ignoreversion

[Icons]
Name: "{group}\{#MyAppName}"; Filename: "{app}\{#MyAppExeName}"; WorkingDir: "{app}"
Name: "{group}\{cm:UninstallProgram,{#MyAppName}}"; Filename: "{uninstallexe}"
Name: "{autodesktop}\{#MyAppName}"; Filename: "{app}\{#MyAppExeName}"; WorkingDir: "{app}"; Tasks: desktopicon

[Run]
Filename: "{app}\{#MyAppExeName}"; Description: "{cm:LaunchProgram,{#MyAppName}}"; Flags: nowait postinstall skipifsilent
