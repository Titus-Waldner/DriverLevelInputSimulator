#include <DriverLevelInputSimulator/DeviceProtocol.hpp>

#include <windows.h>

#include <cerrno>
#include <cstdlib>
#include <iostream>
#include <limits>
#include <string>

namespace
{

bool ParseMovement(
    const char* text,
    signed char& movement
)
{
    char* end = nullptr;

    errno = 0;
    const long value = std::strtol(
        text,
        &end,
        10
    );

    if (errno != 0 ||
        end == text ||
        *end != '\0' ||
        value < -127 ||
        value > 127)
    {
        return false;
    }

    movement = static_cast<signed char>(value);
    return true;
}

int RunMovementCommand(
    signed char movementX,
    signed char movementY
)
{
    const HANDLE device = CreateFileW(
        L"\\\\.\\DriverLevelInputSimulator",
        GENERIC_READ | GENERIC_WRITE,
        0,
        nullptr,
        OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL,
        nullptr
    );

    if (device == INVALID_HANDLE_VALUE)
    {
        std::cerr
            << "Unable to open the driver. Windows error: "
            << GetLastError()
            << '\n';

        return EXIT_FAILURE;
    }

    DriverLevelInputSimulator::MouseCommand command
    {
        0,
        movementX,
        movementY,
        0,
        0
    };

    DWORD bytesReturned = 0;

    const BOOL result = DeviceIoControl(
        device,
        DriverLevelInputSimulator::IoctlSubmitMouseReport,
        &command,
        static_cast<DWORD>(sizeof(command)),
        nullptr,
        0,
        &bytesReturned,
        nullptr
    );

    const DWORD errorCode =
        result ? ERROR_SUCCESS : GetLastError();

    CloseHandle(device);

    if (!result)
    {
        std::cerr
            << "The driver rejected the mouse report. Windows error: "
            << errorCode
            << '\n';

        return EXIT_FAILURE;
    }

    std::cout
        << "Submitted relative mouse movement: "
        << static_cast<int>(movementX)
        << ", "
        << static_cast<int>(movementY)
        << '\n';

    return EXIT_SUCCESS;
}

}

int main(
    int argumentCount,
    char* argumentValues[]
)
{
    if (argumentCount != 4 ||
        std::string(argumentValues[1]) != "move")
    {
        std::cout
            << "Usage:\n"
            << "  DriverLevelInputSimulator move <x> <y>\n"
            << "\n"
            << "Each movement value must be between -127 and 127.\n";

        return EXIT_FAILURE;
    }

    signed char movementX = 0;
    signed char movementY = 0;

    if (!ParseMovement(argumentValues[2], movementX) ||
        !ParseMovement(argumentValues[3], movementY))
    {
        std::cerr
            << "Movement values must be integers from -127 to 127.\n";

        return EXIT_FAILURE;
    }

    return RunMovementCommand(
        movementX,
        movementY
    );
}