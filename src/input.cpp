#include "input.h"
#include <cstring>
#include <cmath>

namespace huEVO {

InputManager g_Input;

static const int THUMBSTICK_DEADZONE = 8000;
static const int REPEAT_INITIAL_DELAY = 18; // frames (~300ms at 60fps)
static const int REPEAT_INTERVAL = 6;       // frames (~100ms)

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
    DWORD dwResult = XInputGetState(0, &m_CurrentState);
    m_Connected = (dwResult == ERROR_SUCCESS);
#else
    m_Connected = true;
#endif

    if (!m_Connected) {
        m_LeftStickX = 0.0f;
        m_LeftStickY = 0.0f;
        return;
    }

    // Process Left Thumbstick with Deadzone filtering
    int rawX = m_CurrentState.Gamepad.sThumbLX;
    int rawY = m_CurrentState.Gamepad.sThumbLY;

    if (std::abs(rawX) > THUMBSTICK_DEADZONE) {
        m_LeftStickX = static_cast<float>(rawX) / 32768.0f;
    } else {
        m_LeftStickX = 0.0f;
    }

    if (std::abs(rawY) > THUMBSTICK_DEADZONE) {
        m_LeftStickY = static_cast<float>(rawY) / 32768.0f;
    } else {
        m_LeftStickY = 0.0f;
    }
}

bool InputManager::IsDown(uint16_t buttonMask) const {
    if (!m_Connected) return false;
    return (m_CurrentState.Gamepad.wButtons & buttonMask) != 0;
}

bool InputManager::IsPressed(uint16_t buttonMask) const {
    if (!m_Connected) return false;
    bool current = (m_CurrentState.Gamepad.wButtons & buttonMask) != 0;
    bool previous = (m_PreviousState.Gamepad.wButtons & buttonMask) != 0;
    return current && !previous;
}

bool InputManager::IsReleased(uint16_t buttonMask) const {
    if (!m_Connected) return false;
    bool current = (m_CurrentState.Gamepad.wButtons & buttonMask) != 0;
    bool previous = (m_PreviousState.Gamepad.wButtons & buttonMask) != 0;
    return !current && previous;
}

bool InputManager::NavUp() {
    bool raw = IsDown(XINPUT_GAMEPAD_DPAD_UP) || (m_LeftStickY > 0.55f);
    if (!raw) {
        if (m_LastNavDirection == XINPUT_GAMEPAD_DPAD_UP) {
            m_LastNavDirection = 0;
            m_RepeatTimer = 0;
        }
        return false;
    }

    if (m_LastNavDirection != XINPUT_GAMEPAD_DPAD_UP) {
        m_LastNavDirection = XINPUT_GAMEPAD_DPAD_UP;
        m_RepeatTimer = REPEAT_INITIAL_DELAY;
        return true;
    }

    if (--m_RepeatTimer <= 0) {
        m_RepeatTimer = REPEAT_INTERVAL;
        return true;
    }
    return false;
}

bool InputManager::NavDown() {
    bool raw = IsDown(XINPUT_GAMEPAD_DPAD_DOWN) || (m_LeftStickY < -0.55f);
    if (!raw) {
        if (m_LastNavDirection == XINPUT_GAMEPAD_DPAD_DOWN) {
            m_LastNavDirection = 0;
            m_RepeatTimer = 0;
        }
        return false;
    }

    if (m_LastNavDirection != XINPUT_GAMEPAD_DPAD_DOWN) {
        m_LastNavDirection = XINPUT_GAMEPAD_DPAD_DOWN;
        m_RepeatTimer = REPEAT_INITIAL_DELAY;
        return true;
    }

    if (--m_RepeatTimer <= 0) {
        m_RepeatTimer = REPEAT_INTERVAL;
        return true;
    }
    return false;
}

bool InputManager::NavLeft() {
    return IsPressed(XINPUT_GAMEPAD_DPAD_LEFT) || (m_LeftStickX < -0.6f && m_PreviousState.Gamepad.sThumbLX >= -THUMBSTICK_DEADZONE);
}

bool InputManager::NavRight() {
    return IsPressed(XINPUT_GAMEPAD_DPAD_RIGHT) || (m_LeftStickX > 0.6f && m_PreviousState.Gamepad.sThumbLX <= THUMBSTICK_DEADZONE);
}

} // namespace huEVO
