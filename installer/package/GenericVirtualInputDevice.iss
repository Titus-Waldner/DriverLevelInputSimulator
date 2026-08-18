#define AppName "Generic Virtual Input Device"
#define AppVersion "0.1.0"
#define AppPublisher "Generic Input Device"
#define AppExecutable "DriverLevelInputSimulator.exe"
#define SetupHelper "GenericInputDeviceSetup.exe"
#define DriverInf "DriverLevelInputSimulatorDriver.inf"
#define DriverCertificate "DriverLevelInputSimulatorDriver.cer"

[Setup]
AppId={{84CE2EB4-36AE-49C4-B4C3-AC999221F840}
AppName={#AppName}
AppVersion={#AppVersion}
AppPublisher={#AppPublisher}
AppVerName={#AppName} {#AppVersion} Test Build
DefaultDirName={autopf64}\Generic Virtual Input Device
DefaultGroupName={#AppName}
DisableProgramGroupPage=yes
PrivilegesRequired=admin
ArchitecturesAllowed=x64compatible
ArchitecturesInstallIn64BitMode=x64compatible
OutputDir=..\output
OutputBaseFilename=GenericVirtualInputDevice-Setup-{#AppVersion}-test-x64
Compression=lzma2
SolidCompression=yes
WizardStyle=modern
SetupLogging=yes
UninstallDisplayName={#AppName}
ChangesEnvironment=yes
RestartIfNeededByRun=no
CloseApplications=yes

[Languages]
Name: "english"; MessagesFile: "compiler:Default.isl"

[Files]
Source: "..\staging\{#AppExecutable}"; DestDir: "{app}"; Flags: ignoreversion
Source: "..\staging\{#SetupHelper}"; DestDir: "{app}"; Flags: ignoreversion
Source: "..\staging\Driver\DriverLevelInputSimulatorDriver.sys"; DestDir: "{app}\Driver"; Flags: ignoreversion
Source: "..\staging\Driver\{#DriverInf}"; DestDir: "{app}\Driver"; Flags: ignoreversion
Source: "..\staging\Driver\driverlevelinputsimulatordriver.cat"; DestDir: "{app}\Driver"; Flags: ignoreversion
Source: "..\staging\Driver\{#DriverCertificate}"; DestDir: "{app}\Driver"; Flags: ignoreversion

[Icons]
Name: "{group}\Open Command Prompt"; Filename: "{cmd}"; Parameters: "/k cd /d ""{app}"""; WorkingDir: "{app}"
Name: "{group}\Uninstall {#AppName}"; Filename: "{uninstallexe}"

[UninstallRun]
Filename: "{app}\{#AppExecutable}"; Parameters: "release-all"; Flags: runhidden waituntilterminated; RunOnceId: "ReleaseVirtualInput"
Filename: "{app}\{#SetupHelper}"; Parameters: "uninstall"; Flags: runhidden waituntilterminated; RunOnceId: "RemoveVirtualDevice"

[Code]

var
  PrerequisitePage: TWizardPage;
  InstructionsMemo: TNewMemo;
  TestModeCommandEdit: TNewEdit;
  CopyCommandButton: TNewButton;
  PrerequisitesCheckBox: TNewCheckBox;
  
const
  EnvironmentKey =
    'SYSTEM\CurrentControlSet\Control\Session Manager\Environment';

  EM_SETSEL = $00B1;
  WM_COPY = $0301;

function GetSystemPath(var CurrentPath: string): Boolean;
begin
  Result := RegQueryStringValue(
    HKLM64,
    EnvironmentKey,
    'Path',
    CurrentPath
  );
end;

function PathContainsEntry(
  CurrentPath: string;
  PathEntry: string
): Boolean;
begin
  Result :=
    Pos(
      ';' + Uppercase(PathEntry) + ';',
      ';' + Uppercase(CurrentPath) + ';'
    ) <> 0;
end;

procedure AddApplicationToPath();
var
  CurrentPath: string;
  ApplicationPath: string;
begin
  ApplicationPath := ExpandConstant('{app}');

  if not GetSystemPath(CurrentPath) then
  begin
    CurrentPath := '';
  end;

  if PathContainsEntry(CurrentPath, ApplicationPath) then
  begin
    exit;
  end;

  if CurrentPath = '' then
  begin
    CurrentPath := ApplicationPath;
  end
  else
  begin
    CurrentPath := ApplicationPath + ';' + CurrentPath;
  end;

  if not RegWriteExpandStringValue(
    HKLM64,
    EnvironmentKey,
    'Path',
    CurrentPath
  ) then
  begin
    RaiseException(
      'The installer could not add the application directory to PATH.'
    );
  end;
end;

procedure RemoveApplicationFromPath();
var
  CurrentPath: string;
  ApplicationPath: string;
begin
  if not GetSystemPath(CurrentPath) then
  begin
    exit;
  end;

  ApplicationPath := ExpandConstant('{app}');

  StringChangeEx(
    CurrentPath,
    ApplicationPath + ';',
    '',
    True
  );

  StringChangeEx(
    CurrentPath,
    ';' + ApplicationPath,
    '',
    True
  );

  if CompareText(CurrentPath, ApplicationPath) = 0 then
  begin
    CurrentPath := '';
  end;

  RegWriteExpandStringValue(
    HKLM64,
    EnvironmentKey,
    'Path',
    CurrentPath
  );
end;

function RunChecked(
  FileName: string;
  Parameters: string;
  Description: string
): Boolean;
var
  ResultCode: Integer;
  NewLine: string;
begin
  NewLine := Chr(13) + Chr(10);

  WizardForm.StatusLabel.Caption := Description;

  Result := Exec(
    FileName,
    Parameters,
    '',
    SW_HIDE,
    ewWaitUntilTerminated,
    ResultCode
  );

  if not Result then
  begin
    MsgBox(
      Description + NewLine + NewLine +
      'The process could not be started.',
      mbError,
      MB_OK
    );

    exit;
  end;

  if ResultCode <> 0 then
  begin
    MsgBox(
      Description + NewLine + NewLine +
      'The process returned error code ' +
      IntToStr(ResultCode) + '.',
      mbError,
      MB_OK
    );

    Result := False;
  end;
end;

procedure CopyTestModeCommand(
  Sender: TObject
);
begin
  SendMessage(
    TestModeCommandEdit.Handle,
    EM_SETSEL,
    0,
    -1
  );

  SendMessage(
    TestModeCommandEdit.Handle,
    WM_COPY,
    0,
    0
  );

  MsgBox(
    'The PowerShell command was copied to the clipboard.',
    mbInformation,
    MB_OK
  );
end;

procedure InitializeWizard();
var
  NewLine: string;
  BottomMargin: Integer;
  CheckBoxHeight: Integer;
  CommandHeight: Integer;
  CommandTop: Integer;
begin
  NewLine := Chr(13) + Chr(10);

  //
  // Use a larger wizard so the prerequisite instructions,
  // command field, and confirmation checkbox are not crowded.
  //
  WizardForm.ClientWidth := ScaleX(700);
  WizardForm.ClientHeight := ScaleY(560);

  PrerequisitePage := CreateCustomPage(
    wpWelcome,
    'Driver Test Mode Requirements',
    'Complete these steps before installing the test-signed driver.'
  );

  BottomMargin := ScaleY(8);
  CheckBoxHeight := ScaleY(42);
  CommandHeight := ScaleY(25);

  PrerequisitesCheckBox :=
    TNewCheckBox.Create(PrerequisitePage);

  PrerequisitesCheckBox.Parent :=
    PrerequisitePage.Surface;

  PrerequisitesCheckBox.Left := 0;

  PrerequisitesCheckBox.Top :=
    PrerequisitePage.SurfaceHeight -
    CheckBoxHeight -
    BottomMargin;

  PrerequisitesCheckBox.Width :=
    PrerequisitePage.SurfaceWidth;

  PrerequisitesCheckBox.Height :=
    CheckBoxHeight;

  PrerequisitesCheckBox.Caption :=
    'I confirm that Secure Boot is disabled, Windows Test Mode is ' +
    'active, and Windows has been restarted.';

  PrerequisitesCheckBox.Checked := False;

  CommandTop :=
    PrerequisitesCheckBox.Top -
    CommandHeight -
    ScaleY(14);

  TestModeCommandEdit :=
    TNewEdit.Create(PrerequisitePage);

  TestModeCommandEdit.Parent :=
    PrerequisitePage.Surface;

  TestModeCommandEdit.Left := 0;
  TestModeCommandEdit.Top := CommandTop;

  TestModeCommandEdit.Width :=
    PrerequisitePage.SurfaceWidth -
    ScaleX(130);

  TestModeCommandEdit.Height :=
    CommandHeight;

  TestModeCommandEdit.ReadOnly := True;

  TestModeCommandEdit.Text :=
    'bcdedit.exe /set testsigning on';

  CopyCommandButton :=
    TNewButton.Create(PrerequisitePage);

  CopyCommandButton.Parent :=
    PrerequisitePage.Surface;

  CopyCommandButton.Left :=
    TestModeCommandEdit.Left +
    TestModeCommandEdit.Width +
    ScaleX(10);

  CopyCommandButton.Top :=
    CommandTop - ScaleY(1);

  CopyCommandButton.Width :=
    ScaleX(120);

  CopyCommandButton.Height :=
    CommandHeight + ScaleY(2);

  CopyCommandButton.Caption :=
    'Copy Command';

  CopyCommandButton.OnClick :=
    @CopyTestModeCommand;

  InstructionsMemo :=
    TNewMemo.Create(PrerequisitePage);

  InstructionsMemo.Parent :=
    PrerequisitePage.Surface;

  InstructionsMemo.Left := 0;
  InstructionsMemo.Top := 0;

  InstructionsMemo.Width :=
    PrerequisitePage.SurfaceWidth;

  InstructionsMemo.Height :=
    CommandTop - ScaleY(14);

  InstructionsMemo.ReadOnly := True;
  InstructionsMemo.ScrollBars := ssVertical;
  InstructionsMemo.WordWrap := True;

  InstructionsMemo.Text :=
    'This installer contains a TEST-SIGNED kernel driver.' +
    NewLine + NewLine +

    'Complete these steps before continuing:' +
    NewLine + NewLine +

    '1. Save the BitLocker recovery key if BitLocker or Device ' +
    'Encryption is enabled.' +
    NewLine + NewLine +

    '2. Restart into the computer''s UEFI or BIOS settings.' +
    NewLine + NewLine +

    '3. Disable Secure Boot, save the firmware settings, and ' +
    'restart Windows.' +
    NewLine + NewLine +

    '4. Open Windows PowerShell as Administrator:' +
    NewLine +
    '   - Open the Start menu.' +
    NewLine +
    '   - Search for PowerShell.' +
    NewLine +
    '   - Right-click Windows PowerShell.' +
    NewLine +
    '   - Select Run as administrator.' +
    NewLine + NewLine +

    '5. Copy the command below, paste it into PowerShell, and ' +
    'press Enter.' +
    NewLine + NewLine +

    '6. Confirm that PowerShell reports:' +
    NewLine +
    '   The operation completed successfully.' +
    NewLine + NewLine +

    '7. Restart Windows.' +
    NewLine + NewLine +

    '8. Confirm that a Test Mode watermark appears on the desktop.' +
    NewLine + NewLine +

    '9. Run this installer again and select the confirmation ' +
    'checkbox below.' +
    NewLine + NewLine +

    'This package is intended only for development and testing.';
end;

function NextButtonClick(
  CurrentPageId: Integer
): Boolean;
begin
  Result := True;

  if CurrentPageId = PrerequisitePage.ID then
  begin
    if not PrerequisitesCheckBox.Checked then
    begin
      MsgBox(
        'Complete the driver prerequisites and select the checkbox ' +
        'before continuing.',
        mbError,
        MB_OK
      );

      Result := False;
    end;
  end;
end;

procedure CurStepChanged(CurrentStep: TSetupStep);
var
  CertificatePath: string;
  DriverInfPath: string;
  HelperPath: string;
  NewLine: string;
begin
  if CurrentStep <> ssPostInstall then
  begin
    exit;
  end;

  NewLine := Chr(13) + Chr(10);

  CertificatePath :=
    ExpandConstant('{app}\Driver\{#DriverCertificate}');

  DriverInfPath :=
    ExpandConstant('{app}\Driver\{#DriverInf}');

  HelperPath :=
    ExpandConstant('{app}\{#SetupHelper}');

  if not RunChecked(
    ExpandConstant('{sys}\certutil.exe'),
    '-f -addstore Root "' + CertificatePath + '"',
    'Trusting the test driver certificate...'
  ) then
  begin
    RaiseException(
      'The test driver certificate could not be trusted.'
    );
  end;

  if not RunChecked(
    ExpandConstant('{sys}\certutil.exe'),
    '-f -addstore TrustedPublisher "' + CertificatePath + '"',
    'Trusting the test driver publisher...'
  ) then
  begin
    RaiseException(
      'The test driver publisher could not be trusted.'
    );
  end;

  if not RunChecked(
    HelperPath,
    'install "' + DriverInfPath + '"',
    'Installing the Generic Virtual Input Device driver...'
  ) then
  begin
    RaiseException(
      'The virtual input driver could not be installed.'
    );
  end;

  if not RunChecked(
    HelperPath,
    'status',
    'Verifying the Generic Virtual Input Device...'
  ) then
  begin
    RaiseException(
      'The driver package was installed, but the device is not running.' +
      NewLine +
      'Verify that Windows Test Mode is active, then restart Windows.'
    );
  end;

  AddApplicationToPath();
end;

procedure CurUninstallStepChanged(
  CurrentUninstallStep: TUninstallStep
);
begin
  if CurrentUninstallStep = usUninstall then
  begin
    RemoveApplicationFromPath();
  end;
end;