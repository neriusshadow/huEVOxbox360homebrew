#include "keyboard.h"
#include <cstring>
#include <cstdint>

#ifdef _XBOX
#include <xtl.h>
#endif

namespace huEVO {

SystemKeyboard g_Keyboard;

#ifdef _XBOX
static XOVERLAPPED s_Overlapped;

static void Utf8ToWide(const std::string& utf8, wchar_t* outWide, size_t maxChars) {
    if (!outWide || maxChars == 0) return;
    size_t outIdx = 0;
    size_t inIdx = 0;
    size_t len = utf8.length();

    while (inIdx < len && outIdx + 1 < maxChars) {
        uint32_t codepoint = 0;
        unsigned char c = (unsigned char)utf8[inIdx++];

        if (c < 0x80) {
            codepoint = c;
        } else if ((c & 0xE0) == 0xC0) {
            if (inIdx >= len) break;
            codepoint = (c & 0x1F) << 6;
            codepoint |= ((unsigned char)utf8[inIdx++] & 0x3F);
        } else if ((c & 0xF0) == 0xE0) {
            if (inIdx + 1 >= len) break;
            codepoint = (c & 0x0F) << 12;
            codepoint |= (((unsigned char)utf8[inIdx++] & 0x3F) << 6);
            codepoint |= ((unsigned char)utf8[inIdx++] & 0x3F);
        } else {
            continue;
        }

        outWide[outIdx++] = (wchar_t)codepoint;
    }
    outWide[outIdx] = 0;
}

static std::string WideToUtf8(const wchar_t* inWide) {
    std::string result;
    if (!inWide) return result;

    for (size_t i = 0; inWide[i] != 0; ++i) {
        uint32_t cp = (uint32_t)(uint16_t)inWide[i];
        if (cp < 0x80) {
            result += (char)cp;
        } else if (cp < 0x800) {
            result += (char)(0xC0 | (cp >> 6));
            result += (char)(0x80 | (cp & 0x3F));
        } else {
            result += (char)(0xE0 | (cp >> 12));
            result += (char)(0x80 | ((cp >> 6) & 0x3F));
            result += (char)(0x80 | (cp & 0x3F));
        }
    }
    return result;
}
#endif

SystemKeyboard::SystemKeyboard()
    : m_Active(false)
    , m_HasResult(false)
    , m_Cancelled(false)
{
#ifdef _XBOX
    m_OverlappedPtr = &s_Overlapped;
    std::memset(m_WideTitle, 0, sizeof(m_WideTitle));
    std::memset(m_WideDesc, 0, sizeof(m_WideDesc));
    std::memset(m_WideDefault, 0, sizeof(m_WideDefault));
    std::memset(m_WideResult, 0, sizeof(m_WideResult));
#endif
}

bool SystemKeyboard::Prompt(const std::string& title, const std::string& description, const std::string& defaultText) {
    if (m_Active) return false;

    m_HasResult = false;
    m_Cancelled = false;
    m_ResultString.clear();

#ifdef _XBOX
    std::memset(&s_Overlapped, 0, sizeof(s_Overlapped));
    std::memset(m_WideTitle, 0, sizeof(m_WideTitle));
    std::memset(m_WideDesc, 0, sizeof(m_WideDesc));
    std::memset(m_WideDefault, 0, sizeof(m_WideDefault));
    std::memset(m_WideResult, 0, sizeof(m_WideResult));

    // Convert strings to persistent wchar buffers that remain valid during UI dialog
    Utf8ToWide(title, m_WideTitle, sizeof(m_WideTitle) / sizeof(wchar_t));
    Utf8ToWide(description, m_WideDesc, sizeof(m_WideDesc) / sizeof(wchar_t));
    Utf8ToWide(defaultText, m_WideDefault, sizeof(m_WideDefault) / sizeof(wchar_t));

    DWORD ret = XShowKeyboardUI(
        0,                      // Player index 0
        VKBD_DEFAULT,           // Flags
        m_WideDefault,          // Persistent default text pointer
        m_WideTitle,            // Persistent title pointer
        m_WideDesc,             // Persistent description pointer
        m_WideResult,           // Destination buffer
        512,                    // Max characters
        &s_Overlapped           // Async overlapped structure
    );

    if (ret == ERROR_IO_PENDING) {
        m_Active = true;
        return true;
    }
    return false;
#else
    m_Active = false;
    m_HasResult = true;
    m_ResultString = "Привет из huEVO на Xbox 360!";
    return true;
#endif
}

void SystemKeyboard::Update() {
    if (!m_Active) return;

#ifdef _XBOX
    if (XHasOverlappedIoCompleted(&s_Overlapped)) {
        m_Active = false;
        if (s_Overlapped.dwExtendedError == ERROR_SUCCESS) {
            m_ResultString = WideToUtf8(m_WideResult);
            m_HasResult = true;
        } else {
            m_Cancelled = true;
        }
    }
#endif
}

std::string SystemKeyboard::GetResult() {
    m_HasResult = false;
    return m_ResultString;
}

} // namespace huEVO
