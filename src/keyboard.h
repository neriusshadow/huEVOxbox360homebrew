#pragma once

#include <string>

namespace huEVO {

class SystemKeyboard {
public:
    SystemKeyboard();

    // Trigger the native Xbox 360 On-Screen Virtual Keyboard
    bool Prompt(const std::string& title, const std::string& description, const std::string& defaultText = "");

    // Must be called in the main loop to check if user finished typing
    void Update();

    bool IsActive() const { return m_Active; }
    bool HasResult() const { return m_HasResult; }
    bool WasCancelled() const { return m_Cancelled; }

    std::string GetResult();

private:
    bool m_Active;
    bool m_HasResult;
    bool m_Cancelled;
    std::string m_ResultString;

#ifdef _XBOX
    void* m_OverlappedPtr;
    wchar_t m_WideTitle[256];
    wchar_t m_WideDesc[256];
    wchar_t m_WideDefault[256];
    wchar_t m_WideResult[512];
#endif
};

extern SystemKeyboard g_Keyboard;

} // namespace huEVO
