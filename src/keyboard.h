#pragma once

#include <string>

namespace huEVO {

class SystemKeyboard {
public:
    SystemKeyboard();

    // Prompts the Xbox 360 Native Virtual Keyboard
    bool Prompt(const std::string& title, const std::string& description, const std::string& defaultText = "");

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
