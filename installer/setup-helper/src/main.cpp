#include <windows.h>
#include <cfgmgr32.h>
#include <devguid.h>
#include <newdev.h>
#include <setupapi.h>

#include <cstdlib>
#include <iostream>
#include <string>
#include <vector>
#include <cwchar>

namespace
{

constexpr wchar_t HardwareId[] =
    L"Root\\DriverLevelInputSimulator";

constexpr wchar_t RootDeviceName[] =
    L"DriverLevelInputSimulator";

constexpr wchar_t DeviceDescription[] =
    L"Generic Virtual Input Device";

void PrintWindowsError(
    const char* operation,
    DWORD errorCode
)
{
    LPWSTR messageBuffer = nullptr;

    const DWORD characterCount = FormatMessageW(
        FORMAT_MESSAGE_ALLOCATE_BUFFER |
            FORMAT_MESSAGE_FROM_SYSTEM |
            FORMAT_MESSAGE_IGNORE_INSERTS,
        nullptr,
        errorCode,
        0,
        reinterpret_cast<LPWSTR>(&messageBuffer),
        0,
        nullptr
    );

    std::cerr
        << operation
        << " failed. Windows error "
        << errorCode;

    if (characterCount != 0 &&
        messageBuffer != nullptr)
    {
        std::wcerr
            << L": "
            << messageBuffer;
    }
    else
    {
        std::cerr << '\n';
    }

    if (messageBuffer != nullptr)
    {
        LocalFree(messageBuffer);
    }
}

bool IsRunningAsAdministrator()
{
    BOOL isAdministrator = FALSE;

    SID_IDENTIFIER_AUTHORITY authority =
        SECURITY_NT_AUTHORITY;

    PSID administratorsGroup = nullptr;

    const BOOL sidCreated = AllocateAndInitializeSid(
        &authority,
        2,
        SECURITY_BUILTIN_DOMAIN_RID,
        DOMAIN_ALIAS_RID_ADMINS,
        0,
        0,
        0,
        0,
        0,
        0,
        &administratorsGroup
    );

    if (!sidCreated)
    {
        return false;
    }

    if (!CheckTokenMembership(
        nullptr,
        administratorsGroup,
        &isAdministrator
    ))
    {
        isAdministrator = FALSE;
    }

    FreeSid(administratorsGroup);
    return isAdministrator == TRUE;
}

bool ConvertToWideString(
    const char* text,
    std::wstring& result
)
{
    const int requiredCharacters = MultiByteToWideChar(
        CP_UTF8,
        MB_ERR_INVALID_CHARS,
        text,
        -1,
        nullptr,
        0
    );

    if (requiredCharacters == 0)
    {
        PrintWindowsError(
            "Converting the path to Unicode",
            GetLastError()
        );

        return false;
    }

    std::vector<wchar_t> buffer(
        static_cast<std::size_t>(requiredCharacters)
    );

    if (MultiByteToWideChar(
        CP_UTF8,
        MB_ERR_INVALID_CHARS,
        text,
        -1,
        buffer.data(),
        requiredCharacters
    ) == 0)
    {
        PrintWindowsError(
            "Converting the path to Unicode",
            GetLastError()
        );

        return false;
    }

    result.assign(buffer.data());
    return true;
}

bool GetAbsolutePath(
    const std::wstring& suppliedPath,
    std::wstring& absolutePath
)
{
    const DWORD requiredCharacters = GetFullPathNameW(
        suppliedPath.c_str(),
        0,
        nullptr,
        nullptr
    );

    if (requiredCharacters == 0)
    {
        PrintWindowsError(
            "Resolving the INF path",
            GetLastError()
        );

        return false;
    }

    std::vector<wchar_t> buffer(
        static_cast<std::size_t>(requiredCharacters)
    );

    const DWORD copiedCharacters = GetFullPathNameW(
        suppliedPath.c_str(),
        requiredCharacters,
        buffer.data(),
        nullptr
    );

    if (copiedCharacters == 0 ||
        copiedCharacters >= requiredCharacters)
    {
        PrintWindowsError(
            "Resolving the INF path",
            GetLastError()
        );

        return false;
    }

    absolutePath.assign(buffer.data());
    return true;
}

bool FileExists(
    const std::wstring& path
)
{
    const DWORD attributes = GetFileAttributesW(
        path.c_str()
    );

    return attributes != INVALID_FILE_ATTRIBUTES &&
        (attributes & FILE_ATTRIBUTE_DIRECTORY) == 0;
}

bool RemoveCreatedDevice(
    HDEVINFO deviceInfoSet,
    SP_DEVINFO_DATA& deviceInfoData
)
{
    SP_REMOVEDEVICE_PARAMS removeParameters {};

    removeParameters.ClassInstallHeader.cbSize =
        sizeof(SP_CLASSINSTALL_HEADER);

    removeParameters.ClassInstallHeader.InstallFunction =
        DIF_REMOVE;

    removeParameters.Scope = DI_REMOVEDEVICE_GLOBAL;
    removeParameters.HwProfile = 0;

    if (!SetupDiSetClassInstallParamsW(
        deviceInfoSet,
        &deviceInfoData,
        &removeParameters.ClassInstallHeader,
        sizeof(removeParameters)
    ))
    {
        return false;
    }

    return SetupDiCallClassInstaller(
        DIF_REMOVE,
        deviceInfoSet,
        &deviceInfoData
    ) == TRUE;
}

bool CreateRootDevice(
    HDEVINFO& deviceInfoSet,
    SP_DEVINFO_DATA& deviceInfoData
)
{
    deviceInfoSet = SetupDiCreateDeviceInfoList(
        &GUID_DEVCLASS_SYSTEM,
        nullptr
    );

    if (deviceInfoSet == INVALID_HANDLE_VALUE)
    {
        PrintWindowsError(
            "Creating the device information set",
            GetLastError()
        );

        return false;
    }

    deviceInfoData = {};
    deviceInfoData.cbSize = sizeof(SP_DEVINFO_DATA);

    if (!SetupDiCreateDeviceInfoW(
        deviceInfoSet,
        RootDeviceName,
        &GUID_DEVCLASS_SYSTEM,
        DeviceDescription,
        nullptr,
        DICD_GENERATE_ID,
        &deviceInfoData
    ))
    {
        PrintWindowsError(
            "Creating the root device information",
            GetLastError()
        );

        return false;
    }

    const DWORD hardwareIdBytes =
        static_cast<DWORD>(sizeof(HardwareId));

    if (!SetupDiSetDeviceRegistryPropertyW(
        deviceInfoSet,
        &deviceInfoData,
        SPDRP_HARDWAREID,
        reinterpret_cast<const BYTE*>(HardwareId),
        hardwareIdBytes
    ))
    {
        PrintWindowsError(
            "Setting the root device hardware ID",
            GetLastError()
        );

        return false;
    }

    if (!SetupDiCallClassInstaller(
        DIF_REGISTERDEVICE,
        deviceInfoSet,
        &deviceInfoData
    ))
    {
        PrintWindowsError(
            "Registering the root device",
            GetLastError()
        );

        return false;
    }

    return true;
}

bool HardwareIdMatches(
    HDEVINFO deviceInfoSet,
    SP_DEVINFO_DATA& deviceInfoData
)
{
    DWORD requiredBytes = 0;
    DWORD propertyType = 0;

    SetupDiGetDeviceRegistryPropertyW(
        deviceInfoSet,
        &deviceInfoData,
        SPDRP_HARDWAREID,
        &propertyType,
        nullptr,
        0,
        &requiredBytes
    );

    if (requiredBytes == 0)
    {
        return false;
    }

    std::vector<BYTE> buffer(
        static_cast<std::size_t>(requiredBytes)
    );

    if (!SetupDiGetDeviceRegistryPropertyW(
        deviceInfoSet,
        &deviceInfoData,
        SPDRP_HARDWAREID,
        &propertyType,
        buffer.data(),
        requiredBytes,
        nullptr
    ))
    {
        return false;
    }

    const wchar_t* currentId =
        reinterpret_cast<const wchar_t*>(buffer.data());

    while (*currentId != L'\0')
    {
        if (_wcsicmp(currentId, HardwareId) == 0)
        {
            return true;
        }

        currentId += std::wcslen(currentId) + 1;
    }

    return false;
}

bool GetDeviceInstanceId(
    HDEVINFO deviceInfoSet,
    SP_DEVINFO_DATA& deviceInfoData,
    std::wstring& instanceId
)
{
    DWORD requiredCharacters = 0;

    SetupDiGetDeviceInstanceIdW(
        deviceInfoSet,
        &deviceInfoData,
        nullptr,
        0,
        &requiredCharacters
    );

    if (requiredCharacters == 0)
    {
        return false;
    }

    std::vector<wchar_t> buffer(
        static_cast<std::size_t>(requiredCharacters)
    );

    if (!SetupDiGetDeviceInstanceIdW(
        deviceInfoSet,
        &deviceInfoData,
        buffer.data(),
        requiredCharacters,
        nullptr
    ))
    {
        return false;
    }

    instanceId.assign(buffer.data());
    return true;
}
int ShowDeviceStatus()
{
    HDEVINFO deviceInfoSet = SetupDiGetClassDevsW(
        &GUID_DEVCLASS_SYSTEM,
        nullptr,
        nullptr,
        0
    );

    if (deviceInfoSet == INVALID_HANDLE_VALUE)
    {
        PrintWindowsError(
            "Opening the System device list",
            GetLastError()
        );

        return EXIT_FAILURE;
    }

    DWORD matchingDevices = 0;
    DWORD startedDevices = 0;

    for (DWORD index = 0; ; ++index)
    {
        SP_DEVINFO_DATA deviceInfoData {};
        deviceInfoData.cbSize = sizeof(SP_DEVINFO_DATA);

        if (!SetupDiEnumDeviceInfo(
            deviceInfoSet,
            index,
            &deviceInfoData
        ))
        {
            const DWORD errorCode = GetLastError();

            if (errorCode == ERROR_NO_MORE_ITEMS)
            {
                break;
            }

            SetupDiDestroyDeviceInfoList(deviceInfoSet);

            PrintWindowsError(
                "Enumerating System devices",
                errorCode
            );

            return EXIT_FAILURE;
        }

        if (!HardwareIdMatches(
            deviceInfoSet,
            deviceInfoData
        ))
        {
            continue;
        }

        ++matchingDevices;

        std::wstring instanceId;

        if (GetDeviceInstanceId(
            deviceInfoSet,
            deviceInfoData,
            instanceId
        ))
        {
            std::wcout
                << L"Device instance: "
                << instanceId
                << L'\n';
        }

        ULONG deviceStatus = 0;
        ULONG problemCode = 0;

        const CONFIGRET configResult = CM_Get_DevNode_Status(
            &deviceStatus,
            &problemCode,
            deviceInfoData.DevInst,
            0
        );

        if (configResult != CR_SUCCESS)
        {
            std::cout
                << "Unable to read the device status. Configuration Manager result: "
                << configResult
                << '\n';

            continue;
        }

        if (problemCode != 0)
        {
            std::cout
                << "Device problem code: "
                << problemCode
                << '\n';

            continue;
        }

        if ((deviceStatus & DN_STARTED) != 0)
        {
            ++startedDevices;
            std::cout << "Status: running\n";
        }
        else
        {
            std::cout << "Status: installed but not running\n";
        }
    }

    SetupDiDestroyDeviceInfoList(deviceInfoSet);

    if (matchingDevices == 0)
    {
        std::cout
            << "Generic Virtual Input Device is not installed.\n";

        return 3;
    }

    std::cout
        << "Matching devices: "
        << matchingDevices
        << '\n';

    if (startedDevices == matchingDevices)
    {
        return EXIT_SUCCESS;
    }

    return 4;
}

int UninstallDevices()
{
    using DiUninstallDeviceFunction = BOOL(WINAPI*)(
        HWND,
        HDEVINFO,
        PSP_DEVINFO_DATA,
        DWORD,
        PBOOL
    );

    HMODULE newDevLibrary = LoadLibraryW(
        L"newdev.dll"
    );

    if (newDevLibrary == nullptr)
    {
        PrintWindowsError(
            "Loading newdev.dll",
            GetLastError()
        );

        return EXIT_FAILURE;
    }

    const auto diUninstallDevice =
        reinterpret_cast<DiUninstallDeviceFunction>(
            GetProcAddress(
                newDevLibrary,
                "DiUninstallDevice"
            )
        );

    if (diUninstallDevice == nullptr)
    {
        const DWORD errorCode = GetLastError();

        FreeLibrary(newDevLibrary);

        PrintWindowsError(
            "Finding DiUninstallDevice",
            errorCode
        );

        return EXIT_FAILURE;
    }

    HDEVINFO deviceInfoSet = SetupDiGetClassDevsW(
        &GUID_DEVCLASS_SYSTEM,
        nullptr,
        nullptr,
        0
    );

    if (deviceInfoSet == INVALID_HANDLE_VALUE)
    {
        const DWORD errorCode = GetLastError();

        FreeLibrary(newDevLibrary);

        PrintWindowsError(
            "Opening the System device list",
            errorCode
        );

        return EXIT_FAILURE;
    }

    DWORD removedDevices = 0;
    bool restartRequired = false;

    for (DWORD index = 0; ; )
    {
        SP_DEVINFO_DATA deviceInfoData {};
        deviceInfoData.cbSize = sizeof(SP_DEVINFO_DATA);

        if (!SetupDiEnumDeviceInfo(
            deviceInfoSet,
            index,
            &deviceInfoData
        ))
        {
            const DWORD errorCode = GetLastError();

            if (errorCode == ERROR_NO_MORE_ITEMS)
            {
                break;
            }

            SetupDiDestroyDeviceInfoList(deviceInfoSet);
            FreeLibrary(newDevLibrary);

            PrintWindowsError(
                "Enumerating System devices",
                errorCode
            );

            return EXIT_FAILURE;
        }

        if (!HardwareIdMatches(
            deviceInfoSet,
            deviceInfoData
        ))
        {
            ++index;
            continue;
        }

        std::wstring instanceId;

        if (GetDeviceInstanceId(
            deviceInfoSet,
            deviceInfoData,
            instanceId
        ))
        {
            std::wcout
                << L"Removing device: "
                << instanceId
                << L'\n';
        }

        BOOL deviceRestartRequired = FALSE;

        if (!diUninstallDevice(
            nullptr,
            deviceInfoSet,
            &deviceInfoData,
            0,
            &deviceRestartRequired
        ))
        {
            const DWORD errorCode = GetLastError();

            SetupDiDestroyDeviceInfoList(deviceInfoSet);
            FreeLibrary(newDevLibrary);

            PrintWindowsError(
                "Uninstalling the virtual input device",
                errorCode
            );

            return EXIT_FAILURE;
        }

        if (deviceRestartRequired)
        {
            restartRequired = true;
        }

        ++removedDevices;

        // Do not increment index after removal. The next device
        // now occupies the current position in the device set.
    }

    SetupDiDestroyDeviceInfoList(deviceInfoSet);
    FreeLibrary(newDevLibrary);

    if (removedDevices == 0)
    {
        std::cout
            << "Generic Virtual Input Device was not installed.\n";

        return EXIT_SUCCESS;
    }

    std::cout
        << "Removed "
        << removedDevices
        << " Generic Virtual Input Device instance";

    if (removedDevices != 1)
    {
        std::cout << 's';
    }

    std::cout << ".\n";

    if (restartRequired)
    {
        std::cout
            << "Windows reported that a restart is required.\n";

        return 2;
    }

    return EXIT_SUCCESS;
}

int InstallDriver(
    const char* suppliedInfPath
)
{
    std::wstring widePath;
    std::wstring absolutePath;

    if (!ConvertToWideString(
        suppliedInfPath,
        widePath
    ))
    {
        return EXIT_FAILURE;
    }

    if (!GetAbsolutePath(
        widePath,
        absolutePath
    ))
    {
        return EXIT_FAILURE;
    }

    if (!FileExists(absolutePath))
    {
        std::wcerr
            << L"The INF file does not exist:\n"
            << absolutePath
            << L'\n';

        return EXIT_FAILURE;
    }

    HDEVINFO deviceInfoSet = INVALID_HANDLE_VALUE;
    SP_DEVINFO_DATA deviceInfoData {};

    if (!CreateRootDevice(
        deviceInfoSet,
        deviceInfoData
    ))
    {
        if (deviceInfoSet != INVALID_HANDLE_VALUE)
        {
            SetupDiDestroyDeviceInfoList(deviceInfoSet);
        }

        return EXIT_FAILURE;
    }

    BOOL restartRequired = FALSE;

    const BOOL driverInstalled =
        UpdateDriverForPlugAndPlayDevicesW(
            nullptr,
            HardwareId,
            absolutePath.c_str(),
            INSTALLFLAG_FORCE,
            &restartRequired
        );

    if (!driverInstalled)
    {
        const DWORD errorCode = GetLastError();

        RemoveCreatedDevice(
            deviceInfoSet,
            deviceInfoData
        );

        SetupDiDestroyDeviceInfoList(deviceInfoSet);

        PrintWindowsError(
            "Installing the driver package",
            errorCode
        );

        return EXIT_FAILURE;
    }

    SetupDiDestroyDeviceInfoList(deviceInfoSet);

    std::wcout
        << L"Generic Virtual Input Device installed successfully.\n"
        << L"INF: "
        << absolutePath
        << L'\n';

    if (restartRequired)
    {
        std::cout
            << "Windows reported that a restart is required.\n";

        return 2;
    }

    std::cout
        << "The device was installed without requiring a restart.\n";

    return EXIT_SUCCESS;
}

void PrintUsage()
{
    std::cout
        << "Generic Virtual Input Device setup helper\n"
        << "\n"
        << "Usage:\n"
        << "  GenericInputDeviceSetup install <INF path>\n"
        << "  GenericInputDeviceSetup status\n"
        << "  GenericInputDeviceSetup uninstall\n";
}

}

int main(
    int argumentCount,
    char* argumentValues[]
)
{
    if (argumentCount < 2)
    {
        PrintUsage();
        return EXIT_FAILURE;
    }

    if (!IsRunningAsAdministrator())
    {
        std::cerr
            << "This operation requires administrator privileges.\n";

        return EXIT_FAILURE;
    }

    const std::string command = argumentValues[1];

    if (command == "install")
    {
        if (argumentCount != 3)
        {
            PrintUsage();
            return EXIT_FAILURE;
        }

        return InstallDriver(argumentValues[2]);
    }

    if (command == "status")
	{
		if (argumentCount != 2)
		{
			PrintUsage();
			return EXIT_FAILURE;
		}

		return ShowDeviceStatus();
	}

	if (command == "uninstall")
	{
		if (argumentCount != 2)
		{
			PrintUsage();
			return EXIT_FAILURE;
		}

		return UninstallDevices();
	}
}