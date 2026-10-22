#ifndef AppVersion
  #error AppVersion must be provided by build_release.ps1
#endif
#ifndef StageDir
  #error StageDir must be provided by build_release.ps1
#endif
#ifndef ReleaseDir
  #error ReleaseDir must be provided by build_release.ps1
#endif

[Setup]
AppId={{99AE202A-6C6B-413A-8320-D3C32F5A6A1A}
AppName=hwspec
AppVersion={#AppVersion}
AppPublisher=MuhammadMaaz
AppPublisherURL=https://github.com/maaz0166/hwspec
AppSupportURL=https://github.com/maaz0166/hwspec/issues
DefaultDirName={localappdata}\Programs\hwspec
DefaultGroupName=hwspec
DisableProgramGroupPage=yes
PrivilegesRequired=lowest
ArchitecturesAllowed=x64os
ArchitecturesInstallIn64BitMode=x64os
MinVersion=10.0.10240
LicenseFile={#StageDir}\LICENSE
OutputDir={#ReleaseDir}
OutputBaseFilename=hwspec-{#AppVersion}-windows-x64-setup
Compression=lzma2
SolidCompression=yes
WizardStyle=modern
UninstallDisplayIcon={app}\hwspec.exe
ChangesEnvironment=yes
CloseApplications=yes

[Tasks]
Name: "addtopath"; Description: "Add hwspec to my PATH (open a new terminal after installation)"

[Files]
Source: "{#StageDir}\hwspec.exe"; DestDir: "{app}"; Flags: ignoreversion
Source: "{#StageDir}\LICENSE"; DestDir: "{app}"; Flags: ignoreversion
Source: "{#StageDir}\README.txt"; DestDir: "{app}"; Flags: ignoreversion

[Icons]
Name: "{group}\hwspec"; Filename: "{app}\hwspec.exe"; Parameters: "--pause"; WorkingDir: "{app}"

[Registry]
Root: HKCU; Subkey: "Software\maaz0166\hwspec"; Flags: uninsdeletekeyifempty

[Code]
const
  StateKey = 'Software\maaz0166\hwspec';
var
  PreviousDirectory: String;

function ExpandEnvironmentStrings(Source, Dest: String; Size: Cardinal): Cardinal;
  external 'ExpandEnvironmentStringsW@kernel32.dll stdcall';

function NormalPath(Value: String): String;
var Buffer: String; Count: Cardinal;
begin
  Value := Trim(Value);
  if (Length(Value) >= 2) and (Value[1] = '"') and (Value[Length(Value)] = '"') then
    Value := Copy(Value, 2, Length(Value)-2);
  SetLength(Buffer, 32768);
  Count := ExpandEnvironmentStrings(Value, Buffer, Length(Buffer));
  if (Count > 0) and (Count <= Cardinal(Length(Buffer))) then begin
    SetLength(Buffer, Count-1);
    Value := Buffer;
  end;
  Result := RemoveBackslashUnlessRoot(Value);
end;

function SamePath(Left, Right: String): Boolean;
begin
  Result := CompareText(NormalPath(Left), NormalPath(Right)) = 0;
end;

function PathSegment(Path, Directory: String; var Start, Count: Integer): Boolean;
var Finish: Integer;
begin
  Start := 1;
  Result := False;
  while Start <= Length(Path) do begin
    Finish := Start;
    while (Finish <= Length(Path)) and (Path[Finish] <> ';') do Finish := Finish + 1;
    Count := Finish-Start;
    if SamePath(Copy(Path, Start, Count), Directory) then begin
      Result := True;
      Exit;
    end;
    Start := Finish+1;
  end;
end;

procedure RemoveOwnedPath(Directory: String);
var Path, BeforePath, AfterPath: String; Owns, Existed: Cardinal; Start, Count: Integer;
    CurrentExists, DeletePath: Boolean;
begin
  if not RegQueryDWordValue(HKCU, StateKey, 'OwnsPath', Owns) then Exit;
  if Owns <> 1 then Exit;
  CurrentExists := RegQueryStringValue(HKCU, 'Environment', 'Path', Path);
  RegQueryStringValue(HKCU, StateKey, 'PathBefore', BeforePath);
  RegQueryStringValue(HKCU, StateKey, 'PathAfter', AfterPath);
  DeletePath := False;
  if Path = AfterPath then begin
    if RegQueryDWordValue(HKCU, StateKey, 'PathExisted', Existed) then DeletePath := Existed = 0;
    Path := BeforePath;
  end
  else if PathSegment(Path, Directory, Start, Count) then begin
    if Start+Count <= Length(Path) then Count := Count+1
    else if Start > 1 then begin Start := Start-1; Count := Count+1; end;
    Delete(Path, Start, Count);
  end;
  if CurrentExists then begin
    if DeletePath then RegDeleteValue(HKCU, 'Environment', 'Path')
    else if not RegWriteExpandStringValue(HKCU, 'Environment', 'Path', Path) then
      RaiseException('Could not update your PATH.');
  end;
  RegDeleteValue(HKCU, StateKey, 'OwnsPath');
  RegDeleteValue(HKCU, StateKey, 'PathBefore');
  RegDeleteValue(HKCU, StateKey, 'PathAfter');
  RegDeleteValue(HKCU, StateKey, 'PathExisted');
end;

function InitializeSetup(): Boolean;
begin
  RegQueryStringValue(HKCU, StateKey, 'InstallDirectory', PreviousDirectory);
  Result := True;
end;

procedure CurStepChanged(Step: TSetupStep);
var Path, Updated, Directory: String; Start, Count: Integer; PathExisted: Boolean;
begin
  if Step <> ssPostInstall then Exit;
  Directory := ExpandConstant('{app}');
  if (PreviousDirectory <> '') and not SamePath(PreviousDirectory, Directory) then
    RemoveOwnedPath(PreviousDirectory);
  if not WizardIsTaskSelected('addtopath') then RemoveOwnedPath(Directory)
  else begin
    PathExisted := RegQueryStringValue(HKCU, 'Environment', 'Path', Path);
    if not PathSegment(Path, Directory, Start, Count) then begin
      Updated := Path;
      if (Updated <> '') and (Updated[Length(Updated)] <> ';') then Updated := Updated+';';
      Updated := Updated+Directory;
      if not RegWriteExpandStringValue(HKCU, 'Environment', 'Path', Updated) then
        RaiseException('Could not add hwspec to your PATH.');
      RegWriteStringValue(HKCU, StateKey, 'PathBefore', Path);
      RegWriteStringValue(HKCU, StateKey, 'PathAfter', Updated);
      if PathExisted then RegWriteDWordValue(HKCU, StateKey, 'PathExisted', 1)
      else RegWriteDWordValue(HKCU, StateKey, 'PathExisted', 0);
      RegWriteDWordValue(HKCU, StateKey, 'OwnsPath', 1);
    end;
  end;
  RegWriteStringValue(HKCU, StateKey, 'InstallDirectory', Directory);
end;

procedure CurUninstallStepChanged(Step: TUninstallStep);
var Directory: String;
begin
  if Step <> usUninstall then Exit;
  if RegQueryStringValue(HKCU, StateKey, 'InstallDirectory', Directory) and
     SamePath(Directory, ExpandConstant('{app}')) then begin
    RemoveOwnedPath(Directory);
    RegDeleteValue(HKCU, StateKey, 'InstallDirectory');
  end;
end;
