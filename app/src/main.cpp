#include <DriverLevelInputSimulator/DeviceProtocol.hpp>

#include <windows.h>

#include <cerrno>
#include <cstdlib>
#include <iostream>
#include <string>

namespace
{

constexpr unsigned char LeftButton = 0x01;
constexpr unsigned char RightButton = 0x02;
constexpr unsigned char MiddleButton = 0x04;
constexpr unsigned char BackButton = 0x08;
constexpr unsigned char ForwardButton = 0x10;

bool ParseSignedByte(
    const char* text,
    signed char& value
)
{
    char* end = nullptr;

    errno = 0;

    const long parsedValue = std::strtol(
        text,
        &end,
        10
    );

    if (errno != 0 ||
        end == text ||
        *end != '\0' ||
        parsedValue < -127 ||
        parsedValue > 127)
    {
        return false;
    }

    value = static_cast<signed char>(parsedValue);
    return true;
}

bool ParseMouseButton(
    const std::string& name,
    unsigned char& button
)
{
    if (name == "left")
    {
        button = LeftButton;
        return true;
    }

    if (name == "right")
    {
        button = RightButton;
        return true;
    }

    if (name == "middle")
    {
        button = MiddleButton;
        return true;
    }

    if (name == "back")
    {
        button = BackButton;
        return true;
    }

    if (name == "forward")
    {
        button = ForwardButton;
        return true;
    }

    return false;
}

HANDLE OpenDriver()
{
    return CreateFileW(
        L"\\\\.\\DriverLevelInputSimulator",
        GENERIC_READ | GENERIC_WRITE,
        0,
        nullptr,
        OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL,
        nullptr
    );
}

bool SubmitMouseCommand(
    HANDLE device,
    const DriverLevelInputSimulator::MouseCommand& command
)
{
    DWORD bytesReturned = 0;

    const BOOL result = DeviceIoControl(
        device,
        DriverLevelInputSimulator::IoctlSubmitMouseReport,
        const_cast<DriverLevelInputSimulator::MouseCommand*>(&command),
        static_cast<DWORD>(sizeof(command)),
        nullptr,
        0,
        &bytesReturned,
        nullptr
    );

    if (!result)
    {
        std::cerr
            << "The driver rejected the mouse report. Windows error: "
            << GetLastError()
            << '\n';

        return false;
    }

    return true;
}

int RunMovementCommand(
    HANDLE device,
    signed char movementX,
    signed char movementY
)
{
    const DriverLevelInputSimulator::MouseCommand command
    {
        0,
        movementX,
        movementY,
        0,
        0
    };

    if (!SubmitMouseCommand(device, command))
    {
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

int RunButtonStateCommand(
    HANDLE device,
    unsigned char button,
    bool pressed
)
{
    const DriverLevelInputSimulator::MouseCommand command
    {
        pressed ? button : static_cast<unsigned char>(0),
        0,
        0,
        0,
        0
    };

    if (!SubmitMouseCommand(device, command))
    {
        return EXIT_FAILURE;
    }

    std::cout
        << "Submitted mouse button "
        << (pressed ? "down" : "up")
        << " report.\n";

    return EXIT_SUCCESS;
}

int RunClickCommand(
    HANDLE device,
    unsigned char button
)
{
    const DriverLevelInputSimulator::MouseCommand buttonDown
    {
        button,
        0,
        0,
        0,
        0
    };

    const DriverLevelInputSimulator::MouseCommand buttonUp
    {
        0,
        0,
        0,
        0,
        0
    };

    if (!SubmitMouseCommand(device, buttonDown))
    {
        SubmitMouseCommand(device, buttonUp);
        return EXIT_FAILURE;
    }

    Sleep(30);

    if (!SubmitMouseCommand(device, buttonUp))
    {
        return EXIT_FAILURE;
    }

    std::cout << "Submitted mouse click.\n";
    return EXIT_SUCCESS;
}

int RunWheelCommand(
    HANDLE device,
    signed char amount,
    bool horizontal
)
{
    const DriverLevelInputSimulator::MouseCommand command
    {
        0,
        0,
        0,
        horizontal ? static_cast<signed char>(0) : amount,
        horizontal ? amount : static_cast<signed char>(0)
    };

    if (!SubmitMouseCommand(device, command))
    {
        return EXIT_FAILURE;
    }

    std::cout
        << "Submitted "
        << (horizontal ? "horizontal" : "vertical")
        << " wheel movement: "
        << static_cast<int>(amount)
        << '\n';

    return EXIT_SUCCESS;
}

bool ParseKeyboardKey(
    const std::string& name,
    unsigned char& usage,
    unsigned char& modifiers
)
{
    modifiers = 0;

    if (name.size() == 1)
    {
        const char character = name[0];

        if (character >= 'a' && character <= 'z')
        {
            usage = static_cast<unsigned char>(
                0x04 + character - 'a'
            );

            return true;
        }

        if (character >= 'A' && character <= 'Z')
        {
            usage = static_cast<unsigned char>(
                0x04 + character - 'A'
            );

            modifiers = 0x02;
            return true;
        }

        if (character >= '1' && character <= '9')
        {
            usage = static_cast<unsigned char>(
                0x1E + character - '1'
            );

            return true;
        }

        if (character == '0')
        {
            usage = 0x27;
            return true;
        }
    }

    if (name == "ENTER")
    {
        usage = 0x28;
        return true;
    }

    if (name == "ESCAPE" || name == "ESC")
    {
        usage = 0x29;
        return true;
    }

    if (name == "BACKSPACE")
    {
        usage = 0x2A;
        return true;
    }

    if (name == "TAB")
    {
        usage = 0x2B;
        return true;
    }

    if (name == "SPACE")
    {
        usage = 0x2C;
        return true;
    }

    if (name == "F1")
    {
        usage = 0x3A;
        return true;
    }

    if (name == "F2")
    {
        usage = 0x3B;
        return true;
    }

    if (name == "F3")
    {
        usage = 0x3C;
        return true;
    }

    if (name == "F4")
    {
        usage = 0x3D;
        return true;
    }

    if (name == "F5")
    {
        usage = 0x3E;
        return true;
    }

    if (name == "F6")
    {
        usage = 0x3F;
        return true;
    }

    if (name == "F7")
    {
        usage = 0x40;
        return true;
    }

    if (name == "F8")
    {
        usage = 0x41;
        return true;
    }

    if (name == "F9")
    {
        usage = 0x42;
        return true;
    }

    if (name == "F10")
    {
        usage = 0x43;
        return true;
    }

    if (name == "F11")
    {
        usage = 0x44;
        return true;
    }

    if (name == "F12")
    {
        usage = 0x45;
        return true;
    }

    if (name == "RIGHT")
    {
        usage = 0x4F;
        return true;
    }

    if (name == "LEFT")
    {
        usage = 0x50;
        return true;
    }

    if (name == "DOWN")
    {
        usage = 0x51;
        return true;
    }

    if (name == "UP")
    {
        usage = 0x52;
        return true;
    }

    return false;
}

bool SubmitKeyboardCommand(
    HANDLE device,
    const DriverLevelInputSimulator::KeyboardCommand& command
)
{
    DWORD bytesReturned = 0;

    const BOOL result = DeviceIoControl(
        device,
        DriverLevelInputSimulator::IoctlSubmitKeyboardReport,
        const_cast<DriverLevelInputSimulator::KeyboardCommand*>(&command),
        static_cast<DWORD>(sizeof(command)),
        nullptr,
        0,
        &bytesReturned,
        nullptr
    );

    if (!result)
    {
        std::cerr
            << "The driver rejected the keyboard report. Windows error: "
            << GetLastError()
            << '\n';

        return false;
    }

    return true;
}

int RunKeyCommand(
    HANDLE device,
    const std::string& keyName
)
{
    unsigned char usage = 0;
    unsigned char modifiers = 0;

    if (!ParseKeyboardKey(
        keyName,
        usage,
        modifiers
    ))
    {
        std::cerr << "Unknown keyboard key.\n";
        return EXIT_FAILURE;
    }

    const DriverLevelInputSimulator::KeyboardCommand keyDown
    {
        modifiers,
        {usage, 0, 0, 0, 0, 0}
    };

    const DriverLevelInputSimulator::KeyboardCommand keyUp
    {
        0,
        {0, 0, 0, 0, 0, 0}
    };

    if (!SubmitKeyboardCommand(device, keyDown))
    {
        SubmitKeyboardCommand(device, keyUp);
        return EXIT_FAILURE;
    }

    Sleep(30);

    if (!SubmitKeyboardCommand(device, keyUp))
    {
        return EXIT_FAILURE;
    }

    std::cout
        << "Submitted keyboard key: "
        << keyName
        << '\n';

    return EXIT_SUCCESS;
}

int RunKeyStateCommand(
    HANDLE device,
    const std::string& keyName,
    bool pressed
)
{
    unsigned char usage = 0;
    unsigned char modifiers = 0;

    if (!ParseKeyboardKey(
        keyName,
        usage,
        modifiers
    ))
    {
        std::cerr << "Unknown keyboard key.\n";
        return EXIT_FAILURE;
    }

    DriverLevelInputSimulator::KeyboardCommand command
    {
        0,
        {0, 0, 0, 0, 0, 0}
    };

    if (pressed)
    {
        command.modifiers = modifiers;
        command.keys[0] = usage;
    }

    if (!SubmitKeyboardCommand(device, command))
    {
        return EXIT_FAILURE;
    }

    std::cout
        << "Submitted keyboard key "
        << (pressed ? "down: " : "up: ")
        << keyName
        << '\n';

    return EXIT_SUCCESS;
}

int RunReleaseAllCommand(
    HANDLE device
)
{
    const DriverLevelInputSimulator::MouseCommand mouseRelease
    {
        0,
        0,
        0,
        0,
        0
    };

    const DriverLevelInputSimulator::KeyboardCommand keyboardRelease
    {
        0,
        {0, 0, 0, 0, 0, 0}
    };

    const bool mouseReleased = SubmitMouseCommand(
        device,
        mouseRelease
    );

    const bool keyboardReleased = SubmitKeyboardCommand(
        device,
        keyboardRelease
    );

    if (!mouseReleased || !keyboardReleased)
    {
        std::cerr
            << "One or more input states could not be released.\n";

        return EXIT_FAILURE;
    }

    std::cout
        << "Released all virtual mouse buttons and keyboard keys.\n";

    return EXIT_SUCCESS;
}

bool ParseScreenCoordinate(
    const char* text,
    long& coordinate
)
{
    char* end = nullptr;

    errno = 0;

    const long parsedValue = std::strtol(
        text,
        &end,
        10
    );

    if (errno != 0 ||
        end == text ||
        *end != '\0')
    {
        return false;
    }

    coordinate = parsedValue;
    return true;
}

unsigned short NormalizeScreenCoordinate(
    long pixelCoordinate,
    long pixelCount
)
{
    constexpr unsigned long long AbsoluteMaximum =
        32767ULL;

    if (pixelCount <= 1)
    {
        return 1;
    }

    const unsigned long long numerator =
        static_cast<unsigned long long>(pixelCoordinate) *
        AbsoluteMaximum;

    const unsigned long long denominator =
        static_cast<unsigned long long>(pixelCount - 1);

    unsigned long long normalizedCoordinate =
        (numerator + denominator / 2) /
        denominator;

    if (normalizedCoordinate == 0)
    {
        normalizedCoordinate = 1;
    }

    return static_cast<unsigned short>(
        normalizedCoordinate
    );
}

bool SubmitAbsoluteMouseCommand(
    HANDLE device,
    const DriverLevelInputSimulator::AbsoluteMouseCommand& command
)
{
    DWORD bytesReturned = 0;

    const BOOL result = DeviceIoControl(
        device,
        DriverLevelInputSimulator::IoctlSubmitAbsoluteMouseReport,
        const_cast<
            DriverLevelInputSimulator::AbsoluteMouseCommand*
        >(&command),
        static_cast<DWORD>(sizeof(command)),
        nullptr,
        0,
        &bytesReturned,
        nullptr
    );

    if (!result)
    {
        std::cerr
            << "The driver rejected the absolute mouse report. "
            << "Windows error: "
            << GetLastError()
            << '\n';

        return false;
    }

    return true;
}

int RunMoveToCommand(
    HANDLE device,
    const char* xText,
    const char* yText
)
{
    long pixelX = 0;
    long pixelY = 0;

    if (!ParseScreenCoordinate(xText, pixelX) ||
        !ParseScreenCoordinate(yText, pixelY))
    {
        std::cerr
            << "Absolute coordinates must be integers.\n";

        return EXIT_FAILURE;
    }

    const int screenWidth = GetSystemMetrics(
        SM_CXSCREEN
    );

    const int screenHeight = GetSystemMetrics(
        SM_CYSCREEN
    );

    if (screenWidth <= 0 ||
        screenHeight <= 0)
    {
        std::cerr
            << "Unable to determine the primary monitor size.\n";

        return EXIT_FAILURE;
    }

    if (pixelX < 0 ||
        pixelX >= screenWidth ||
        pixelY < 0 ||
        pixelY >= screenHeight)
    {
        std::cerr
            << "Coordinates are outside the primary monitor.\n"
            << "Valid X range: 0 through "
            << (screenWidth - 1)
            << '\n'
            << "Valid Y range: 0 through "
            << (screenHeight - 1)
            << '\n';

        return EXIT_FAILURE;
    }

    const unsigned short normalizedX =
        NormalizeScreenCoordinate(
            pixelX,
            screenWidth
        );

    const unsigned short normalizedY =
        NormalizeScreenCoordinate(
            pixelY,
            screenHeight
        );

    const DriverLevelInputSimulator::AbsoluteMouseCommand command
    {
        0,
        normalizedX,
        normalizedY
    };

    if (!SubmitAbsoluteMouseCommand(
        device,
        command
    ))
    {
        return EXIT_FAILURE;
    }

    Sleep(10);

    POINT observedPosition {};

    if (!GetCursorPos(&observedPosition))
    {
        std::cerr
            << "The report was submitted, but GetCursorPos failed. "
            << "Windows error: "
            << GetLastError()
            << '\n';

        return EXIT_FAILURE;
    }

    std::cout
        << "Submitted absolute mouse position: "
        << pixelX
        << ", "
        << pixelY
        << '\n'
        << "Normalized HID position: "
        << normalizedX
        << ", "
        << normalizedY
        << '\n'
        << "Observed cursor position: "
        << observedPosition.x
        << ", "
        << observedPosition.y
        << '\n';

    return EXIT_SUCCESS;
}

void PrintUsage()
{
    std::cout
        << "Usage:\n"
        << "  DriverLevelInputSimulator move <x> <y>\n"
		<< "Absolute coordinates apply to the primary monitor.\n"
        << "  DriverLevelInputSimulator click <button>\n"
        << "  DriverLevelInputSimulator button-down <button>\n"
        << "  DriverLevelInputSimulator button-up <button>\n"
        << "  DriverLevelInputSimulator wheel <amount>\n"
        << "  DriverLevelInputSimulator horizontal-wheel <amount>\n"
        << "  DriverLevelInputSimulator key <key>\n"
        << "  DriverLevelInputSimulator key-down <key>\n"
        << "  DriverLevelInputSimulator key-up <key>\n"
        << "  DriverLevelInputSimulator release-all\n"
        << "\n"
        << "Mouse buttons:\n"
        << "  left, right, middle, back, forward\n"
        << "\n"
        << "Keyboard examples:\n"
        << "  A, a, 0-9, ENTER, ESCAPE, BACKSPACE, TAB, SPACE\n"
        << "  F1-F12, LEFT, RIGHT, UP, DOWN\n"
        << "\n"
        << "Movement and wheel values must be between -127 and 127.\n";
}

int ProcessCommand(
    HANDLE device,
    int argumentCount,
    char* argumentValues[]
)
{
    const std::string command = argumentValues[1];
	if (command == "move-to" &&
        argumentCount == 4)
    {
        return RunMoveToCommand(
            device,
            argumentValues[2],
            argumentValues[3]
        );
    }

    if (command == "release-all" &&
        argumentCount == 2)
    {
        return RunReleaseAllCommand(device);
    }

    if (command == "key" &&
        argumentCount == 3)
    {
        return RunKeyCommand(
            device,
            argumentValues[2]
        );
    }

    if ((command == "key-down" ||
         command == "key-up") &&
        argumentCount == 3)
    {
        return RunKeyStateCommand(
            device,
            argumentValues[2],
            command == "key-down"
        );
    }

    if (command == "move" &&
        argumentCount == 4)
    {
        signed char movementX = 0;
        signed char movementY = 0;

        if (!ParseSignedByte(argumentValues[2], movementX) ||
            !ParseSignedByte(argumentValues[3], movementY))
        {
            std::cerr
                << "Movement values must be integers from -127 to 127.\n";

            return EXIT_FAILURE;
        }

        return RunMovementCommand(
            device,
            movementX,
            movementY
        );
    }

    if ((command == "click" ||
         command == "button-down" ||
         command == "button-up") &&
        argumentCount == 3)
    {
        unsigned char button = 0;

        if (!ParseMouseButton(argumentValues[2], button))
        {
            std::cerr << "Unknown mouse button.\n";
            return EXIT_FAILURE;
        }

        if (command == "click")
        {
            return RunClickCommand(device, button);
        }

        return RunButtonStateCommand(
            device,
            button,
            command == "button-down"
        );
    }

    if ((command == "wheel" ||
         command == "horizontal-wheel") &&
        argumentCount == 3)
    {
        signed char amount = 0;

        if (!ParseSignedByte(argumentValues[2], amount))
        {
            std::cerr
                << "Wheel amount must be an integer from -127 to 127.\n";

            return EXIT_FAILURE;
        }

        return RunWheelCommand(
            device,
            amount,
            command == "horizontal-wheel"
        );
    }

    PrintUsage();
    return EXIT_FAILURE;
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

    const HANDLE device = OpenDriver();

    if (device == INVALID_HANDLE_VALUE)
    {
        std::cerr
            << "Unable to open the driver. Windows error: "
            << GetLastError()
            << '\n';

        return EXIT_FAILURE;
    }

    const int result = ProcessCommand(
        device,
        argumentCount,
        argumentValues
    );

    CloseHandle(device);
    return result;
}