#pragma once
#include <stdint.h>

enum MouseButtonFlags
{
    None,
    LeftDown = 0x01,
    RightDown = 0x02,
    MiddleDown = 0x04,
    WheelPlus = 0x08,
    WheelMinus = 0x10,
};

struct WindowMouseState
{
    uint32_t Time;
    MouseButtonFlags MouseFlag;        
    int16_t Width;
    int16_t Height;
    int16_t X;
    int16_t Y;
};
static_assert(sizeof(WindowMouseState)==16, "sizeof(WindowMouseState)");
