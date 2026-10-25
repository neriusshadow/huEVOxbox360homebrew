#pragma once

#ifndef NOMINMAX
#define NOMINMAX
#endif

#ifdef _XBOX
#include <xtl.h>
typedef unsigned short uint16_t;
typedef unsigned char uint8_t;
typedef short int16_t;
typedef unsigned int uint32_t;
#else
#include <cstdint>
// Fallback definitions for local PC testing / non-XDK compilation
struct XINPUT_GAMEPAD {
    uint16_t wButtons;
    uint8_t  bLeftTrigger;
    uint8_t  bRightTrigger;
    int16_t  sThumbLX;
    int16_t  sThumbLY;
    int16_t  sThumbRX;
    int16_t  sThumbRY;
};
struct XINPUT_STATE {
    uint32_t dwPacketNumber;
    XINPUT_GAMEPAD Gamepad;
};
#define XINPUT_GAMEPAD_DPAD_UP        0x0001
#define XINPUT_GAMEPAD_DPAD_DOWN      0x0002
#define XINPUT_GAMEPAD_DPAD_LEFT      0x0004
#define XINPUT_GAMEPAD_DPAD_RIGHT     0x0008
#define XINPUT_GAMEPAD_START          0x0010
#define XINPUT_GAMEPAD_BACK           0x0020
#define XINPUT_GAMEPAD_LEFT_THUMB     0x0040
#define XINPUT_GAMEPAD_RIGHT_THUMB    0x0080
#define XINPUT_GAMEPAD_LEFT_SHOULDER  0x0100
#define XINPUT_GAMEPAD_RIGHT_SHOULDER 0x0200
#define XINPUT_GAMEPAD_A              0x1000
#define XINPUT_GAMEPAD_B              0x2000
#define XINPUT_GAMEPAD_X              0x4000
#define XINPUT_GAMEPAD_Y              0x8000
#endif

namespace huEVO {

class InputManager {
public:
    InputManager();

    void Update();

    bool IsConnected() const { return m_Connected; }

    // Instantaneous button state (down this frame)
    bool IsDown(uint16_t buttonMask) const;

    // Single trigger (pressed down on this frame, not held)
    bool IsPressed(uint16_t buttonMask) const;

    // Released on this frame
    bool IsReleased(uint16_t buttonMask) const;

    // Analog stick values normalized from -1.0f to 1.0f
    float GetLeftStickX() const { return m_LeftStickX; }
    float GetLeftStickY() const { return m_LeftStickY; }

    // Navigation helpers with auto-repeat for scrolling
    bool NavUp();
    bool NavDown();
    bool NavLeft();
    bool NavRight();

private:
    bool m_Connected;
    XINPUT_STATE m_CurrentState;
    XINPUT_STATE m_PreviousState;

    float m_LeftStickX;
    float m_LeftStickY;

    // Auto-repeat timers for smooth list scrolling
    int m_RepeatTimer;
    uint16_t m_LastNavDirection;
};

extern InputManager g_Input;

} // namespace huEVO
