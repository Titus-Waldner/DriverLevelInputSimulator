#pragma once

#include <cstdint>

namespace DriverLevelInputSimulator
{

enum class MouseButton : std::uint8_t
{
    None   = 0x00,
    Left   = 0x01,
    Right  = 0x02,
    Middle = 0x04,
    Back   = 0x08,
    Forward = 0x10
};

struct MouseReport
{
    std::uint8_t buttons;
    std::int16_t movementX;
    std::int16_t movementY;
    std::int8_t verticalWheel;
    std::int8_t horizontalWheel;
};

static_assert(
    sizeof(MouseReport) == 8,
    "MouseReport layout changed unexpectedly."
);

}