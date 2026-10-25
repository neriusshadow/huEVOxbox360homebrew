#pragma once

#include <string>

namespace huEVO {

enum class ConnectionMode {
    STANDALONE_FALLBACK = 0,
    COMPANION_BRIDGE = 1
};

// Target endpoints
#define HUEVO_DIRECT_HOST "huevo.su"
#define HUEVO_DIRECT_PORT 443

#define HUEVO_BRIDGE_HOST "192.168.31.219"
#define HUEVO_BRIDGE_PORT 8080

// Standard Xbox 360 720p 16:9 Display
#define SCREEN_WIDTH  1280
#define SCREEN_HEIGHT 720

// Authentic Color Palette (Sampled directly from huevo.su screenshots)
#define COLOR_BG               0xFF141517  // Core page background
#define COLOR_TOPBAR           0xFF1F2022  // Header bar
#define COLOR_SIDEBAR          0xFF141517  // Left navigation panel
#define COLOR_CARD             0xFF1C1D1F  // Cards and post containers
#define COLOR_CARD_BORDER      0xFF2A2B2E  // Subtle container outlines
#define COLOR_CARD_HOVER       0xFF252629  // Hover/focus surface
#define COLOR_SELECTION        0xFF223247  // Selected item background
#define COLOR_SELECTION_BORDER 0xFF5181B8  // Focused item border
#define COLOR_TEXT_PRIMARY     0xFFE1E3E6  // Crisp readable text
#define COLOR_TEXT_MUTED       0xFF828282  // Secondary / timestamps / handles
#define COLOR_ACCENT_BLUE      0xFF5181B8  // huevo.su link & branding blue
#define COLOR_ACCENT_GREEN     0xFF4BB34B  // Online dot / status green
#define COLOR_BADGE_RED        0xFFE64646  // Notification / unread count red
#define COLOR_COIN_GOLD        0xFFFFB300  // Монэтки currency gold
#define COLOR_XBOX_GREEN       0xFF107C10  // Xbox 360 green

// Safe Area Margins for standard HDTVs
#define SAFE_MARGIN_X 56
#define SAFE_MARGIN_Y 32

struct AppConfig {
    ConnectionMode mode = ConnectionMode::COMPANION_BRIDGE;
    std::string bridgeHost = HUEVO_BRIDGE_HOST;
    int bridgePort = HUEVO_BRIDGE_PORT;

    std::string sessionCookie = "";
    std::string username = "нериусшадоу";
    std::string userHandle = "@telega";
    bool isLoggedIn = true;
    int coinsBalance = 301680;

    bool soundEnabled = true;
    bool showDebug = false;
};

extern AppConfig g_Config;

} // namespace huEVO
