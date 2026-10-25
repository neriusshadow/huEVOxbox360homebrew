#pragma once

#include <string>

namespace huEVO {

enum class ConnectionMode {
    DIRECT_STANDALONE = 0, // Direct connection from Xbox 360 over Ethernet to huevo.su
    COMPANION_BRIDGE = 1   // Optional local PC bridge
};

// Target direct server
#define HUEVO_DIRECT_HOST "huevo.su"
#define HUEVO_DIRECT_PORT 443

// Bridge defaults (configured to current Mac host IP)
#define HUEVO_BRIDGE_HOST "192.168.31.219"
#define HUEVO_BRIDGE_PORT 8080

// Screen resolution (Standard 720p 16:9 for Xbox 360)
#define SCREEN_WIDTH 1280
#define SCREEN_HEIGHT 720

// UI Colors (32-bit ARGB) - Authentic huevo.su Dark Theme
#define COLOR_BG            0xFF1B1C1E  // Official huevo.su dark background
#define COLOR_HEADER        0xFF27282B  // Official huevo.su top navigation bar
#define COLOR_PANEL         0xFF222326  // Official huevo.su post card surface
#define COLOR_PANEL_BORDER  0xFF36383D  // Official huevo.su card border
#define COLOR_ACCENT        0xFF5181B8  // Official huevo.su electric blue link
#define COLOR_ACCENT_ALT    0xFF107C10  // Xbox 360 controller green
#define COLOR_TEXT_PRIMARY  0xFFEDEDED  // Crisp light text
#define COLOR_TEXT_MUTED    0xFF8E929A  // Slate grey secondary text
#define COLOR_TEXT_LINK     0xFF5181B8  // Web blue
#define COLOR_SELECTION     0xFF2A2F3D  // Focused item highlight
#define COLOR_BADGE         0xFFE64646  // Notification / like red

// Safe Area margins (Title Safe Area for CRT/HDTV standards)
#define SAFE_MARGIN_X 64
#define SAFE_MARGIN_Y 36

struct AppConfig {
    ConnectionMode mode = ConnectionMode::DIRECT_STANDALONE;
    std::string directHost = HUEVO_DIRECT_HOST;
    int directPort = HUEVO_DIRECT_PORT;
    bool directUseTls = true;

    // Optional Bridge settings
    std::string bridgeHost = HUEVO_BRIDGE_HOST;
    int bridgePort = HUEVO_BRIDGE_PORT;

    // Session and Auth
    std::string sessionCookie = "";
    std::string username = "";
    std::string password = "";
    bool isLoggedIn = false;

    // Preferences
    bool soundEffects = true;
    bool debugOverlay = false;
};

extern AppConfig g_Config;

} // namespace huEVO
