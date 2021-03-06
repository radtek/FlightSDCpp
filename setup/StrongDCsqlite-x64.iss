; Script generated by the Inno Setup Script Wizard.
; SEE THE DOCUMENTATION FOR DETAILS ON CREATING INNO SETUP SCRIPT FILES!

#if VER < 0x05050100
  #error Update your Inno Setup version
#endif
#ifndef UNICODE
  #error Use the Unicode Inno Setup
#endif

#define YANDEX_DOWNLOADER

;//#define YANDEX_DEBUG
#define YANDEX_VID_PREFIX 2
#define YANDEX_VID_INDEX 0

[Setup]
AppName=StrongDC++ sqlite x64
AppVerName=StrongDC++ sqlite x64
AppPublisher=StrongDC++ sqlite Team
AppPublisherURL=http://code.google.com/p/flylinkdc/
AppSupportURL=http://code.google.com/p/flylinkdc/issues/list
AppUpdatesURL=http://code.google.com/p/flylinkdc/downloads/list
DefaultDirName={sd}\StrongDC++sqlite-x64
DefaultGroupName=StrongDC++ sqlite x64
InfoBeforeFile="readme.rtf"
OutputBaseFilename=SetupStrongDC-sqlite-x64-r14735
SetupIconFile="..\res\StrongDC.ico"
Compression=lzma2/ultra64
SolidCompression=yes
WizardImageFile=setup-1.bmp
WizardSmallImageFile=setup-2.bmp
InfoAfterFile=infoafter.rtf
AppendDefaultDirName=false

ArchitecturesAllowed=x64

[Languages]
Name: "russian"; MessagesFile: "compiler:Languages\Russian.isl"

[Tasks]
Name: "desktopicon"; Description: "{cm:CreateDesktopIcon}"; GroupDescription: "{cm:AdditionalIcons}"; Flags: unchecked
Name: "quicklaunchicon"; Description: "{cm:CreateQuickLaunchIcon}"; GroupDescription: "{cm:AdditionalIcons}"; Flags: unchecked

[Components]
Name: "program"; Description: "Program Files"; Types: full compact custom; Flags: fixed
Name: "Lang"; Description: "����"; Types: full
Name: "Lang\ru"; Description: "�������"; Flags: exclusive
Name: "Lang\en"; Description: "����������"; Flags: exclusive
Name: "Lang\ukr"; Description: "����������"; Flags: exclusive
Name: "DCPlusPlus"; Description: "��������������� � �����"; Types: full
Name: "DCPlusPlus\vip_dchublist"; Description: "���������� ���� dchublist.ru"; Flags: exclusive

[Files]
Source: "..\compiled\StrongDC64.exe"; DestDir: "{app}"; Flags: overwritereadonly ignoreversion
Source: "..\compiled\readme.txt"; DestDir: "{app}"; Flags: ignoreversion
Source: "..\cvs-changelog.txt"; DestDir: "{app}"; Flags: ignoreversion
Source: "..\compiled\Settings\GeoIPCountryWhois.csv"; DestDir: "{app}\Settings"; Flags: overwritereadonly
Source: "..\compiled\mpc-hc.exe"; DestDir: "{app}"; Flags: overwritereadonly ignoreversion
Source: "..\compiled\crshhndl-x64.dll"; DestDir: "{app}"; Flags: overwritereadonly ignoreversion


Source: "..\changelog-sqlite-svn.txt"; DestDir: "{app}"; Flags: overwritereadonly ignoreversion


Source: "..\compiled\Russian.xml"; DestDir: "{app}";  Components: Lang\ru; Flags: ignoreversion
Source: "lang_eng\Russian.xml"; DestDir: "{app}";  Components: Lang\en; Flags: ignoreversion
Source: "lang_ukr\Russian.xml"; DestDir: "{app}";  Components: Lang\ukr; Flags: ignoreversion
;Source: "..\compiled\BackUp\BackupProfile.bat"; DestDir: "{app}\BackUp"; Flags: ignoreversion

Source: "readme.rtf"; DestDir: "{app}"; 

Source: "..\compiled\EmoPacks\*.*"; Excludes: "\.svn\"; DestDir: "{app}\EmoPacks"; Flags: createallsubdirs overwritereadonly ignoreversion recursesubdirs


Source: "..\compiled\Settings\CustomLocations.bmp"; DestDir: "{app}\Settings"; Flags:  onlyifdoesntexist ignoreversion
Source: "..\compiled\Settings\CustomLocations.ini"; DestDir: "{app}\Settings"; Flags:  onlyifdoesntexist ignoreversion
Source: "..\compiled\Settings\IPTrust.ini"; DestDir: "{app}\Settings"; Flags: onlyifdoesntexist ignoreversion
Source: "DCPlusPlus.xml"; DestDir: "{app}\Settings"; Flags: onlyifdoesntexist ignoreversion
Source: "Favorites.xml"; DestDir: "{app}\Settings"; Components: DCPlusPlus\vip_dchublist; Flags: onlyifdoesntexist ignoreversion


[Icons]
Name: "{group}\StrongDC++ sqlite x64"; Filename: "{app}\StrongDC64.exe"; WorkingDir: "{app}"
Name: "{group}\� ���������"; Filename: "{app}\Readme.rtf"
Name: "{group}\{cm:ProgramOnTheWeb,StrongDC++ sqlite x64}"; Filename: "http://flylinkdc.googlecode.com/"
Name: "{group}\{cm:UninstallProgram,StrongDC++ sqlite x64}"; Filename: "{uninstallexe}"
Name: "{commondesktop}\StrongDC++ sqlite x64"; Filename: "{app}\StrongDC64.exe"; Tasks: desktopicon ; WorkingDir: "{app}"
Name: "{userappdata}\Microsoft\Internet Explorer\Quick Launch\StrongDC++ sqlite x64"; Filename: "{app}\StrongDC64.exe"; Tasks: quicklaunchicon ; WorkingDir: "{app}"

[Run]
Filename: "{app}\StrongDC64.exe"; Description: "{cm:LaunchProgram,StrongDC++ sqlite x64}"; Flags: nowait postinstall skipifsilent

#include "D:\ppa-doc\Dropbox\src\install-yandex\inc_finally_yandex_strongdc.hss"

#include "custom_lang\custom_messages-RU-RU.isl"
