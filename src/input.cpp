#include "input.h"
#include <cstring>
#include <cmath>

namespace huEVO {

InputManager g_Input;

InputManager::InputManager()
    : m_Connected(false)
    , m_LeftStickX(0.0f)
    , m_LeftStickY(0.0f)
    , m_RepeatTimer(0)
    , m_LastNavDirection(0)
{
    std::memset(&m_CurrentState, 0, sizeof(m_CurrentState));
    std::memset(&m_PreviousState, 0, sizeof(m_PreviousState));
}

void InputManager::Update() {
    m_PreviousState = m_CurrentState;

#ifdef _XBOX
    DWORD result = XInputGetState(0, &m_CurrentState);
    m_Connected = (result == ERROR_SUCCESS);
#else
    m_Connected = true;
#endif

    if (m_Connected) {
        // Deadzone calculation
        int lx = m_CurrentState.Gamepad.sThumbLX;
        int ly = m_CurrentState.Gamepad.sThumbLY;
        const int deadzone = 7849;

        if (std::abs(lx) > deadzone) {
            m_LeftStickX = (lx > 0) ? (float)(lx - deadzone) / (32767.0f - deadzone)
                                    : (float)(lx + deadzone) / (32768.0f - deadzone);
        } else {
            m_LeftStickX = 0.0f;
        }

        if (std::abs(ly) > deadzone) {
            m_LeftStickY = (ly > 0) ? (float)(ly - deadzone) / (32767.0f - deadzone)
                                    : (float)(ly + deadzone) / (32768.0f - deadzone);
        } else {
            m_LeftStickY = 0.0f;
        }
    } else {
        m_LeftStickX = 0.0f;
        m_LeftStickY = 0.0f;
    }
}

bool InputManager::IsDown(uint16_t buttonMask) const {
    if (!m_Connected) return false;
    return (m_CurrentState.Gamepad.wButtons & buttonMask) != 0;
}

bool InputManager::IsPressed(uint16_t buttonMask) const {
    if (!m_Connected) return false;
    return ((m_CurrentState.Gamepad.wButtons & buttonMask) != 0) &&
           ((m_PreviousState.Gamepad.wButtons & buttonMask) == 0);
}

bool InputManager::IsReleased(uint16_t buttonMask) const {
    if (!m_Connected) return false;
    return ((m_CurrentState.Gamepad.wButtons & buttonMask) == 0) &&
           ((m_PreviousState.Gamepad.wButtons & buttonMask) != 0);
}

bool InputManager::NavUp() {
    bool raw = IsPressed(XINPUT_GAMEPAD_DPAD_UP) || (m_LeftStickY > 0.65f && m_PreviousState.Gamepad.sThumbLY <= 15000);
    bool held = IsDown(XINPUT_GAMEPAD_DPAD_UP) || (m_LeftStickY > 0.65f);

    if (raw) {
        m_LastNavDirection = XINPUT_GAMEPAD_DPAD_UP;
        m_RepeatTimer = 18; // Initial delay
        return true;
    }
    if (held && m_LastNavDirection == XINPUT_GAMEPAD_DPAD_UP) {
        m_RepeatTimer--;
        if (m_RepeatTimer <= 0) {
            m_RepeatTimer = 5; // Repeat speed
            return true;
        }
    }
    return false;
}

bool InputManager::NavDown() {
    bool raw = IsPressed(XINPUT_GAMEPAD_DPAD_DOWN) || (m_LeftStickY < -0.65f && m_PreviousState.Gamepad.sThumbLY >= -15000);
    bool held = IsDown(XINPUT_GAMEPAD_DPAD_DOWN) || (m_LeftStickY < -0.65f);

    if (raw) {
        m_LastNavDirection = XINPUT_GAMEPAD_DPAD_DOWN;
        m_RepeatTimer = 18;
        return true;
    }
    if (held && m_LastNavDirection == XINPUT_GAMEPAD_DPAD_DOWN) {
        m_RepeatTimer--;
        if (m_RepeatTimer <= 0) {
            m_RepeatTimer = 5;
            return true;
        }
    }
    return false;
}

bool InputManager::NavLeft() {
    return IsPressed(XINPUT_GAMEPAD_DPAD_LEFT) || (m_LeftStickX < -0.65f && m_PreviousState.Gamepad.sThumbLX >= -15000);
}

bool InputManager::NavRight() {
    return IsPressed(XINPUT_GAMEPAD_DPAD_RIGHT) || (m_LeftStickX > 0.65f && m_PreviousState.Gamepad.sThumbLX <= 15000);
}

} // namespace huEVO
