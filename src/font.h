#pragma once

#include <cstdint>
#include <string>

namespace huEVO {

// Standard ASCII 8x8 Font (32..127)
extern const uint8_t s_FontAscii[96][8];

// Cyrillic 8x8 Font (33 uppercase, 33 lowercase)
// Order: А, Б, В, Г, Д, Е, Ж, З, И, Й, К, Л, М, Н, О, П, Р, С, Т, У, Ф, Х, Ц, Ч, Ш, Щ, Ъ, Ы, Ь, Э, Ю, Я, Ё,
//        а, б, в, г, д, е, ж, з, и, й, к, л, м, н, о, п, р, с, т, у, ф, х, ц, ч, ш, щ, ъ, ы, ь, э, ю, я, ё
extern const uint8_t s_FontCyrillic[66][8];

// Xbox Controller button badges 8x8
enum XboxButtonIcon {
    BTN_ICON_A = 0,
    BTN_ICON_B = 1,
    BTN_ICON_X = 2,
    BTN_ICON_Y = 3,
    BTN_ICON_LB = 4,
    BTN_ICON_RB = 5,
    BTN_ICON_DPAD = 6,
    BTN_ICON_CHECK = 7
};

extern const uint8_t s_FontXboxIcons[8][8];

// Decodes one UTF-8 codepoint and advances pointer
uint32_t DecodeUtf8Codepoint(const char*& p, size_t& remaining);

// Retrieves 8x8 bitmap pointer for any UTF-8 codepoint (ASCII, Russian, or Icons)
const uint8_t* GetGlyphBitmap(uint32_t codepoint);

} // namespace huEVO
