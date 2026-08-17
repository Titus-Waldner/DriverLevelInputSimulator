#pragma once

namespace DriverLevelInputSimulator
{

constexpr unsigned long IoctlSubmitMouseReport = 0x0022A000UL;

#pragma pack(push, 1)

struct MouseCommand
{
    unsigned char buttons;
    signed char movementX;
    signed char movementY;
    signed char verticalWheel;
    signed char horizontalWheel;
};

#pragma pack(pop)

static_assert(
    sizeof(MouseCommand) == 5,
    "MouseCommand must contain exactly five bytes."
);

}