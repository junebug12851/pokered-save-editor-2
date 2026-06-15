; Inno Setup script for Pokered Save Editor (Windows installer).
; Built in CI by .github/workflows/release.yml via ISCC with these /D defines:
;   /DMyAppVersion=<x.y.z[-label]>   the VERSION string
;   /DMySourceDir=<path>             the windeployqt'd app folder (the portable build)
;   /DMyOutputDir=<path>             where to write the setup .exe
;   /DMyIconFile=<path>              projects/app/assets/icons/app/icon.ico
;
; Build locally to test:
;   ISCC /DMyAppVersion=0.7.2-alpha /DMySourceDir=dist\PokeredSaveEditor ^
;        /DMyOutputDir=out /DMyIconFile=projects\app\assets\icons\app\icon.ico ^
;        packaging\windows\installer.iss

#define MyAppName "Pokered Save Editor"
#define MyAppExeName "PokeredSaveEditor.exe"
#define MyAppPublisher "Twilight"
#define MyAppURL "https://github.com/junebug12851/pokered-save-editor-2"

#ifndef MyAppVersion
  #define MyAppVersion "0.0.0-dev"
#endif

[Setup]
AppName={#MyAppName}
AppVersion={#MyAppVersion}
AppPublisher={#MyAppPublisher}
AppPublisherURL={#MyAppURL}
AppSupportURL={#MyAppURL}
DefaultDirName={autopf}\Pokered Save Editor
DefaultGroupName=Pokered Save Editor
DisableProgramGroupPage=yes
OutputDir={#MyOutputDir}
OutputBaseFilename=PokeredSaveEditor-{#MyAppVersion}-windows-x64-setup
Compression=lzma2
SolidCompression=yes
WizardStyle=modern
; The app is 64-bit (Qt 6 llvm-mingw x64): install into the 64-bit Program Files.
ArchitecturesAllowed=x64compatible
ArchitecturesInstallIn64BitMode=x64compatible
#ifdef MyIconFile
SetupIconFile={#MyIconFile}
#endif

[Languages]
Name: "english"; MessagesFile: "compiler:Default.isl"

[Tasks]
Name: "desktopicon"; Description: "{cm:CreateDesktopIcon}"; GroupDescription: "{cm:AdditionalIcons}"; Flags: unchecked

[Files]
; The entire windeployqt'd folder (exe + Qt DLLs + QML + plugins).
Source: "{#MySourceDir}\*"; DestDir: "{app}"; Flags: recursesubdirs createallsubdirs ignoreversion

[Icons]
Name: "{group}\{#MyAppName}"; Filename: "{app}\{#MyAppExeName}"
Name: "{group}\{cm:UninstallProgram,{#MyAppName}}"; Filename: "{uninstallexe}"
Name: "{autodesktop}\{#MyAppName}"; Filename: "{app}\{#MyAppExeName}"; Tasks: desktopicon

[Run]
Filename: "{app}\{#MyAppExeName}"; Description: "{cm:LaunchProgram,{#MyAppName}}"; Flags: nowait postinstall skipifsilent
