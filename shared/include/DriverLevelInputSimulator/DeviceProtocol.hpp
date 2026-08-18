#pragma once

namespace DriverLevelInputSimulator
{

constexpr unsigned long IoctlSubmitMouseReport = 0x0022A000UL;
constexpr unsigned long IoctlSubmitKeyboardReport = 0x0022A004UL;

#pragma pack(push, 1)

struct MouseCommand
{
    unsigned char buttons;
    signed char movementX;
    signed char movementY;
    signed char verticalWheel;
    signed char horizontalWheel;
};

struct KeyboardCommand
{
    unsigned char modifiers;
    unsigned char keys[6];
};

#pragma pack(pop)

static_assert(
    sizeof(MouseCommand) == 5,
    "MouseCommand must contain exactly five bytes."
);

static_assert(
    sizeof(KeyboardCommand) == 7,
    "KeyboardCommand must contain exactly seven bytes."
);

}