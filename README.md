# DriverLevelInputSimulator

A Windows C++20 project that creates a software-backed virtual HID keyboard and mouse through the Windows Virtual HID Framework (VHF).

The controller communicates with a KMDF kernel driver by using `DeviceIoControl`. The driver submits HID input reports through VHF, rather than using `SendInput`, `mouse_event`, or `keybd_event`.

> [!WARNING]
> The current release is a **development and test build**. The kernel driver is test-signed, not production-signed. Installation requires Secure Boot to be disabled and Windows Test Mode to be enabled. Do not install this build on a production computer.

## Current features

### Virtual mouse

- Relative X and Y movement
- Left, right, and middle click
- Back and forward buttons
- Button-down and button-up reports
- Vertical mouse wheel
- Horizontal mouse wheel
- Release-all safety command

### Virtual keyboard

- Letters `A-Z` and `a-z`
- Digits `0-9`
- Enter, Escape, Backspace, Tab, and Space
- Function keys `F1-F12`
- Arrow keys
- Key-down and key-up reports
- Release-all safety command

### Installer and setup

- x64 graphical test installer
- Installs under `C:\Program Files\Generic Virtual Input Device`
- Adds the controller directory to the system `PATH`
- Installs the WDK test certificate
- Creates and installs the root-enumerated virtual input device
- Includes device status and uninstall support
- Removes the virtual device during uninstall

## Architecture

```text
DriverLevelInputSimulator.exe
        |
        | DeviceIoControl
        v
DriverLevelInputSimulatorDriver.sys
        |
        | VhfReadReportSubmit
        v
Windows Virtual HID Framework
        |
        +-- HID-compliant mouse
        +-- HID keyboard device
        |
        v
Windows input stack
```

The virtual mouse and a physical mouse control the same Windows system cursor. Raw Input applications can still identify the originating device.

## Requirements

### Running the test build

- 64-bit Windows 10 or later
- Administrator privileges
- Secure Boot disabled in UEFI/BIOS
- Windows Test Mode enabled
- A restart after enabling Test Mode

### Building the controller and setup helper

- Windows with MSYS2 UCRT64
- GCC/MinGW-w64
- C++20
- CMake
- Ninja

### Building the kernel driver

- Visual Studio 2026
- Windows Driver Kit 10.0.28000.0 or a compatible supported WDK
- KMDF
- Virtual HID Framework (`vhf.h` and `VhfKm.lib`)

### Building the graphical installer

- Inno Setup 7

## Enabling Windows Test Mode

Before changing Secure Boot settings, save the BitLocker recovery key if BitLocker or Device Encryption is enabled.

1. Restart into the computer's UEFI/BIOS settings.
2. Disable Secure Boot.
3. Save the firmware changes and restart Windows.
4. Open Windows PowerShell as Administrator.
5. Run:

```powershell
bcdedit.exe /set testsigning on
```

6. Confirm that PowerShell reports `The operation completed successfully.`
7. Restart Windows.
8. Confirm that a **Test Mode** watermark appears on the desktop.

To disable Test Mode later, open Windows PowerShell as Administrator and run:

```powershell
bcdedit.exe /set testsigning off
```

Restart Windows afterward. Secure Boot can then be re-enabled in UEFI/BIOS if desired.

## Installing the test build

Run:

```text
GenericVirtualInputDevice-Setup-0.1.0-test-x64.exe
```

The installer will:

1. Display the test-driver prerequisites.
2. Install the controller and setup helper.
3. Trust the included WDK test certificate.
4. Install the root-enumerated VHF source driver.
5. Verify that the virtual input device is running.
6. Add the installation directory to the system `PATH`.

Open a new PowerShell or terminal window after installation so it receives the updated `PATH`.

## Commands

### Mouse movement

```text
DriverLevelInputSimulator.exe move 20 0
DriverLevelInputSimulator.exe move -20 15
```

Movement values must be between `-127` and `127` per report.

### Mouse clicks

```text
DriverLevelInputSimulator.exe click left
DriverLevelInputSimulator.exe click right
DriverLevelInputSimulator.exe click middle
```

### Mouse button state

```text
DriverLevelInputSimulator.exe button-down left
DriverLevelInputSimulator.exe button-up left
```

Always release a button that was pressed with `button-down`.

### Mouse wheel

```text
DriverLevelInputSimulator.exe wheel 1
DriverLevelInputSimulator.exe wheel -1
DriverLevelInputSimulator.exe horizontal-wheel 1
```

### Keyboard press

```text
DriverLevelInputSimulator.exe key A
DriverLevelInputSimulator.exe key a
DriverLevelInputSimulator.exe key ENTER
DriverLevelInputSimulator.exe key F5
DriverLevelInputSimulator.exe key LEFT
```

### Keyboard state

```text
DriverLevelInputSimulator.exe key-down a
DriverLevelInputSimulator.exe key-up a
```

### Emergency release

```text
DriverLevelInputSimulator.exe release-all
```

This releases all virtual mouse buttons, keyboard modifiers, and keyboard keys.

## Build instructions

### Controller and setup helper: Debug

```bash
cmake -S . -B build -G Ninja -DCMAKE_CXX_COMPILER=/ucrt64/bin/g++.exe
```

```bash
cmake --build build
```

### Controller and setup helper: Release

```bash
cmake -S . -B build-release -G Ninja -DCMAKE_CXX_COMPILER=/ucrt64/bin/g++.exe -DCMAKE_BUILD_TYPE=Release
```

```bash
cmake --build build-release
```

### Kernel driver: Release x64

```bash
MSYS_NO_PATHCONV=1 "/c/Program Files/Microsoft Visual Studio/18/Community/MSBuild/Current/Bin/amd64/MSBuild.exe" "driver/DriverLevelInputSimulatorDriver/DriverLevelInputSimulatorDriver/DriverLevelInputSimulatorDriver.vcxproj" /m /p:Configuration=Release /p:Platform=x64
```

### Graphical installer

Stage the Release application, setup helper, driver package, catalog, and certificate under `installer/staging`, then compile:

```bash
"/c/Program Files/Inno Setup 7/ISCC.exe" "$(cygpath -w installer/package/GenericVirtualInputDevice.iss)"
```

The generated installer is written to `installer/output`.

## Setup helper

The native x64 setup helper supports:

```text
GenericInputDeviceSetup.exe install <INF path>
GenericInputDeviceSetup.exe status
GenericInputDeviceSetup.exe uninstall
```

The helper uses Windows device-installation APIs and does not require DevCon or the WDK on the target computer.

## Repository structure

```text
app/                         Controller application
shared/                      Shared IOCTL protocol definitions
driver/                      KMDF/VHF kernel driver
installer/setup-helper/      Native driver setup helper
installer/package/           Inno Setup source
tools/                       Development inspection tools
docs/                        Project documentation
```

Generated build folders, installer staging files, installer output, and packaged release artifacts are intentionally excluded from Git.

## Uninstalling

Use **Settings > Apps > Installed apps > Generic Virtual Input Device > Uninstall**.

The uninstaller attempts to:

1. Release all virtual keyboard and mouse state.
2. Remove the virtual input device.
3. Remove the application directory from the system `PATH`.
4. Remove installed application files.

The current test uninstaller leaves the test certificate and staged driver-store package in place. Full certificate and driver-store cleanup is planned for a later development release.

## Safety notes

- Save work before installing or updating a kernel driver.
- Keep a physical keyboard and mouse connected during testing.
- Use `release-all` if a virtual key or button remains held.
- Test driver updates and uninstall behavior before publishing a release.
- Prefer a virtual machine or dedicated development computer for Driver Verifier and failure testing.
- Do not represent this project as a driver or product from another hardware manufacturer.

## Release status

Version `0.1.0-test` is intended for development evaluation only.

Before publishing a release, verify:

- Clean installation through the graphical installer
- Device status reports `running`
- Mouse movement, clicks, and wheel input
- Keyboard input
- `release-all`
- Installation-directory access through a new terminal's `PATH`
- Graphical uninstall
- Device absence after uninstall
- Installer SHA-256 checksum

## License

No license has been selected yet. Add a license before encouraging third-party redistribution or contributions.
