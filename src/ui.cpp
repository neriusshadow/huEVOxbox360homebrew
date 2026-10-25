#ifndef NOMINMAX
#define NOMINMAX
#endif

#include "ui.h"
#include "renderer.h"
#include "input.h"
#include "network.h"
#include "keyboard.h"
#include "config.h"
#include <cmath>
#include <algorithm>

namespace huEVO {

UIManager g_UI;

enum class KeyboardTarget {
    NONE,
    NEW_POST,
    LOGIN_USERNAME,
    LOGIN_PASSWORD,
    CUSTOM_HOST
};

static KeyboardTarget s_KeyboardTarget = KeyboardTarget::NONE;

UIManager::UIManager()
    : m_CurrentTab(ViewTab::FEED)
    , m_SelectedIndex(0)
    , m_ScrollOffset(0)
    , m_CurrentTrackIndex(0)
    , m_IsMusicPlaying(false)
    , m_ToastTimer(0)
    , m_VisualizerPhase(0.0f)
{
}

void UIManager::Initialize() {
    Post p1;
    p1.id = "1";
    p1.authorName = "huEVO";
    p1.timeAgo = "онлайн";
    p1.content = "Добро пожаловать в huEVO для Xbox 360!";
    p1.likesCount = 84;
    p1.commentsCount = 19;
    m_Posts.push_back(p1);

    Post p2;
    p2.id = "2";
    p2.authorName = "huevo.su";
    p2.timeAgo = "система";
    p2.content = "Нажмите [Y] на геймпаде для загрузки свежих постов из сети.";
    p2.likesCount = 142;
    p2.commentsCount = 37;
    m_Posts.push_back(p2);

    Post p3;
    p3.id = "3";
    p3.authorName = "Xbox 360";
    p3.timeAgo = "RetroNet";
    p3.content = "Используйте [X] для публикации и LB/RB для навигации.";
    p3.likesCount = 56;
    p3.commentsCount = 8;
    m_Posts.push_back(p3);

    ShowToast("huEVO: Нажмите [Y] для обновления ленты");
}

void UIManager::SetTab(ViewTab tab) {
    if (m_CurrentTab != tab) {
        m_CurrentTab = tab;
        m_SelectedIndex = 0;
        m_ScrollOffset = 0;
        RefreshCurrentView();
    }
}

void UIManager::NextTab() {
    int next = (static_cast<int>(m_CurrentTab) + 1) % static_cast<int>(ViewTab::COUNT);
    SetTab(static_cast<ViewTab>(next));
}

void UIManager::PrevTab() {
    int prev = (static_cast<int>(m_CurrentTab) - 1 + static_cast<int>(ViewTab::COUNT)) % static_cast<int>(ViewTab::COUNT);
    SetTab(static_cast<ViewTab>(prev));
}

void UIManager::ShowToast(const std::string& message, int durationFrames) {
    m_ToastMessage = message;
    m_ToastTimer = durationFrames;
}

void UIManager::RefreshCurrentView() {
    switch (m_CurrentTab) {
        case ViewTab::FEED:
            g_Network.FetchFeed(m_Posts);
            break;
        case ViewTab::PROFILE:
            g_Network.FetchProfile("", m_UserProfile, m_UserPosts);
            break;
        case ViewTab::MESSAGES:
            g_Network.FetchDialogs(m_Dialogs);
            break;
        case ViewTab::MUSIC:
            g_Network.FetchAudioTracks(m_Tracks);
            break;
        case ViewTab::SETTINGS:
            break;
        default:
            break;
    }
}

void UIManager::OnActionPress() {
    if (m_CurrentTab == ViewTab::FEED) {
        if (!m_Posts.empty() && m_SelectedIndex < static_cast<int>(m_Posts.size())) {
            OnLikeToggle();
        }
    } else if (m_CurrentTab == ViewTab::PROFILE) {
        if (!g_Config.isLoggedIn) {
            if (m_SelectedIndex == 0) {
                s_KeyboardTarget = KeyboardTarget::LOGIN_USERNAME;
                g_Keyboard.Prompt("Логин huEVO", "Введите логин или e-mail на huevo.su", g_Config.username);
            } else if (m_SelectedIndex == 1) {
                s_KeyboardTarget = KeyboardTarget::LOGIN_PASSWORD;
                g_Keyboard.Prompt("Пароль huEVO", "Введите пароль от huevo.su", "");
            } else {
                if (g_Config.username.empty() || g_Config.password.empty()) {
                    ShowToast("Сначала укажите логин и пароль");
                } else {
                    std::string err;
                    if (g_Network.Login(g_Config.username, g_Config.password, err)) {
                        ShowToast("Успешный вход: @" + g_Config.username);
                        RefreshCurrentView();
                    } else {
                        ShowToast("Ошибка: " + err);
                    }
                }
            }
        } else {
            g_Network.Logout();
            ShowToast("Вы вышли из аккаунта");
            RefreshCurrentView();
        }
    } else if (m_CurrentTab == ViewTab::MUSIC) {
        if (!m_Tracks.empty() && m_SelectedIndex < static_cast<int>(m_Tracks.size())) {
            m_CurrentTrackIndex = m_SelectedIndex;
            m_IsMusicPlaying = !m_IsMusicPlaying;
            ShowToast(m_IsMusicPlaying ? "Воспроизведение: " + m_Tracks[m_CurrentTrackIndex].title : "Пауза");
        }
    } else if (m_CurrentTab == ViewTab::SETTINGS) {
        if (m_SelectedIndex == 0) {
            // Toggle Direct vs Bridge mode
            if (g_Config.mode == ConnectionMode::DIRECT_STANDALONE) {
                g_Config.mode = ConnectionMode::COMPANION_BRIDGE;
                ShowToast("Режим переключен: Companion Bridge");
            } else {
                g_Config.mode = ConnectionMode::DIRECT_STANDALONE;
                ShowToast("Режим переключен: Прямой интернет (Ethernet)");
            }
        } else if (m_SelectedIndex == 1) {
            s_KeyboardTarget = KeyboardTarget::CUSTOM_HOST;
            g_Keyboard.Prompt("Адрес сервера", "Введите домен или IP", g_Config.directHost);
        }
    }
}

void UIManager::OnNewPostPrompt() {
    s_KeyboardTarget = KeyboardTarget::NEW_POST;
    g_Keyboard.Prompt("Новая запись в huEVO", "Напишите что у вас нового...", "");
}

void UIManager::OnLikeToggle() {
    if (m_CurrentTab == ViewTab::FEED && !m_Posts.empty()) {
        auto& p = m_Posts[m_SelectedIndex];
        g_Network.ToggleLike(p.id, p.isLiked, p.likesCount);
        ShowToast(p.isLiked ? "Вам понравилась запись!" : "Лайк убран");
    }
}

void UIManager::Update() {
    // Process async keyboard results
    g_Keyboard.Update();
    if (g_Keyboard.HasResult()) {
        std::string text = g_Keyboard.GetResult();
        if (!text.empty()) {
            if (s_KeyboardTarget == KeyboardTarget::NEW_POST) {
                Post newPost;
                if (g_Network.CreatePost(text, newPost)) {
                    m_Posts.insert(m_Posts.begin(), newPost);
                    m_SelectedIndex = 0;
                    ShowToast("Запись отправлена на huevo.su!");
                }
            } else if (s_KeyboardTarget == KeyboardTarget::LOGIN_USERNAME) {
                g_Config.username = text;
                s_KeyboardTarget = KeyboardTarget::LOGIN_PASSWORD;
                g_Keyboard.Prompt("Пароль huEVO", "Введите пароль от huevo.su", "");
            } else if (s_KeyboardTarget == KeyboardTarget::LOGIN_PASSWORD) {
                g_Config.password = text;
                std::string err;
                if (g_Network.Login(g_Config.username, g_Config.password, err)) {
                    ShowToast("Успешный вход: @" + g_Config.username);
                    RefreshCurrentView();
                } else {
                    ShowToast("Ошибка входа: " + err);
                }
                s_KeyboardTarget = KeyboardTarget::NONE;
            } else if (s_KeyboardTarget == KeyboardTarget::CUSTOM_HOST) {
                g_Config.directHost = text;
                ShowToast("Сервер изменен: " + text);
                s_KeyboardTarget = KeyboardTarget::NONE;
            }
        }
    }

    // Input Handling
    if (g_Input.IsPressed(XINPUT_GAMEPAD_LEFT_SHOULDER)) {
        PrevTab();
    }
    if (g_Input.IsPressed(XINPUT_GAMEPAD_RIGHT_SHOULDER)) {
        NextTab();
    }

    if (g_Input.NavUp()) {
        if (m_SelectedIndex > 0) {
            m_SelectedIndex--;
            if (m_SelectedIndex < m_ScrollOffset) {
                m_ScrollOffset = m_SelectedIndex;
            }
        }
    }

    if (g_Input.NavDown()) {
        int maxIndex = 0;
        if (m_CurrentTab == ViewTab::FEED) maxIndex = static_cast<int>(m_Posts.size()) - 1;
        else if (m_CurrentTab == ViewTab::MESSAGES) maxIndex = static_cast<int>(m_Dialogs.size()) - 1;
        else if (m_CurrentTab == ViewTab::MUSIC) maxIndex = static_cast<int>(m_Tracks.size()) - 1;
        else if (m_CurrentTab == ViewTab::SETTINGS) maxIndex = 2;
        else if (m_CurrentTab == ViewTab::PROFILE) maxIndex = g_Config.isLoggedIn ? 0 : 2;

        if (m_SelectedIndex < maxIndex) {
            m_SelectedIndex++;
            int visibleCards = 3;
            if (m_SelectedIndex >= m_ScrollOffset + visibleCards) {
                m_ScrollOffset = m_SelectedIndex - visibleCards + 1;
            }
        }
    }

    if (g_Input.IsPressed(XINPUT_GAMEPAD_A)) {
        OnActionPress();
    }

    if (g_Input.IsPressed(XINPUT_GAMEPAD_X)) {
        OnNewPostPrompt();
    }

    if (g_Input.IsPressed(XINPUT_GAMEPAD_Y)) {
        if (m_CurrentTab == ViewTab::FEED) {
            RefreshCurrentView();
            ShowToast("Прямой поток обновлен с huevo.su");
        }
    }

    // Update animations
    m_VisualizerPhase += 0.15f;
    if (m_ToastTimer > 0) {
        m_ToastTimer--;
    }
}

void UIManager::Render() {
    RenderHeader();

    switch (m_CurrentTab) {
        case ViewTab::FEED:     RenderFeedView(); break;
        case ViewTab::PROFILE:  RenderProfileView(); break;
        case ViewTab::MESSAGES: RenderMessagesView(); break;
        case ViewTab::MUSIC:    RenderMusicView(); break;
        case ViewTab::SETTINGS: RenderSettingsView(); break;
        default: break;
    }

    RenderFooter();
    RenderToast();
}

void UIManager::RenderHeader() {
    // Top Bar Background - Authentic huevo.su dark header
    g_Renderer.DrawRect(0, 0, SCREEN_WIDTH, 56, COLOR_HEADER);
    g_Renderer.DrawLine(0, 56, SCREEN_WIDTH, 56, COLOR_PANEL_BORDER, 1.5f);

    // huEVO Logo (Clean authentic white logo)
    g_Renderer.DrawText("huEVO", SAFE_MARGIN_X, 16, 0xFFFFFFFF, 1.5f, true);
    g_Renderer.DrawText(".su", SAFE_MARGIN_X + 80, 20, COLOR_TEXT_MUTED, 1.0f, false);

    // Navigation Tabs matching authentic huEVO sections
    const char* tabNames[] = { "Новости", "Моя страница", "Сообщения", "Музыка", "Настройки" };
    float tabStartX = SAFE_MARGIN_X + 160.0f;
    float tabSpacing = 160.0f;

    for (int i = 0; i < static_cast<int>(ViewTab::COUNT); ++i) {
        float tx = tabStartX + i * tabSpacing;
        bool isActive = (i == static_cast<int>(m_CurrentTab));

        if (isActive) {
            g_Renderer.DrawRoundedRect(tx - 10, 10, 140, 36, 4.0f, COLOR_SELECTION, COLOR_ACCENT, 1.5f);
            g_Renderer.DrawText(tabNames[i], tx + 12, 19, 0xFFFFFFFF, 1.1f, true);
        } else {
            g_Renderer.DrawText(tabNames[i], tx + 12, 19, COLOR_TEXT_MUTED, 1.1f, false);
        }
    }

    // Tab shoulder hints
    g_Renderer.DrawBadge("LB", tabStartX - 42.0f, 16, 0xFF36383D, 0xFFFFFFFF);
    g_Renderer.DrawBadge("RB", tabStartX + static_cast<int>(ViewTab::COUNT) * tabSpacing - 10.0f, 16, 0xFF36383D, 0xFFFFFFFF);

    // Right status indicator: user badge or login status
    std::string userBadge = g_Config.isLoggedIn ? ("@" + g_Config.username) : "Вход [A]";
    g_Renderer.DrawBadge(userBadge, SCREEN_WIDTH - SAFE_MARGIN_X - 160, 16, g_Config.isLoggedIn ? COLOR_ACCENT : 0xFF36383D, 0xFFFFFFFF);
}

void UIManager::RenderFeedView() {
    float startY = 90.0f;
    float cardW = SCREEN_WIDTH - (SAFE_MARGIN_X * 2);
    float cardH = 145.0f;
    float cardGap = 16.0f;

    if (m_Posts.empty()) {
        g_Renderer.DrawRoundedRect(SAFE_MARGIN_X, startY + 50, cardW, 200, 8.0f, COLOR_PANEL);
        g_Renderer.DrawText("Лента пуста. Нажмите (X) для публикации первой записи!", SAFE_MARGIN_X + 40, startY + 140, COLOR_TEXT_PRIMARY, 1.25f);
        return;
    }

    int visibleCount = 3;
    for (int i = 0; i < visibleCount; ++i) {
        int postIndex = m_ScrollOffset + i;
        if (postIndex >= static_cast<int>(m_Posts.size())) break;

        const auto& post = m_Posts[postIndex];
        float cy = startY + i * (cardH + cardGap);
        bool isSelected = (postIndex == m_SelectedIndex);

        uint32_t panelColor = isSelected ? COLOR_SELECTION : COLOR_PANEL;
        uint32_t borderColor = isSelected ? COLOR_ACCENT : COLOR_PANEL_BORDER;

        // Card Panel
        g_Renderer.DrawRoundedRect(SAFE_MARGIN_X, cy, cardW, cardH, 8.0f, panelColor, borderColor, isSelected ? 2.5f : 1.0f);

        // Avatar placeholder box
        uint32_t avatarBg = isSelected ? COLOR_ACCENT : 0xFF394B69;
        g_Renderer.DrawRoundedRect(SAFE_MARGIN_X + 20, cy + 18, 48, 48, 6.0f, avatarBg);
        g_Renderer.DrawText("360", SAFE_MARGIN_X + 28, cy + 34, 0xFFFFFFFF, 1.0f, false);

        // Author Name & Time
        g_Renderer.DrawText(post.authorName, SAFE_MARGIN_X + 80, cy + 20, COLOR_TEXT_PRIMARY, 1.25f, true);
        g_Renderer.DrawText(post.timeAgo, SAFE_MARGIN_X + 80, cy + 44, COLOR_TEXT_MUTED, 0.95f, false);

        // Post Content
        g_Renderer.DrawText(post.content, SAFE_MARGIN_X + 80, cy + 74, COLOR_TEXT_PRIMARY, 1.15f, false);

        // Bottom stats: Likes & Comments
        std::string likesStr = (post.isLiked ? "[*] " : "[ ] ") + IntToString(post.likesCount) + " Лайков";
        std::string commentsStr = IntToString(post.commentsCount) + " Комментариев";
        g_Renderer.DrawBadge(likesStr, SAFE_MARGIN_X + 80, cy + 110, post.isLiked ? 0xFF991B1B : 0xFF283344, 0xFFFFFFFF);
        g_Renderer.DrawBadge(commentsStr, SAFE_MARGIN_X + 220, cy + 110, 0xFF283344, COLOR_TEXT_MUTED);

        if (isSelected) {
            g_Renderer.DrawText("-> Выбрано (A - лайк, X - написать)", cardW - 200, cy + 24, COLOR_ACCENT, 1.0f, true);
        }
    }
}

void UIManager::RenderProfileView() {
    float startY = 85.0f;
    float cardW = SCREEN_WIDTH - (SAFE_MARGIN_X * 2);

    if (!g_Config.isLoggedIn) {
        // Authentic huEVO Login Card matching huevo.su /login
        float formW = 620.0f;
        float formX = (SCREEN_WIDTH - formW) * 0.5f;

        g_Renderer.DrawRoundedRect(formX, startY, formW, 370, 8.0f, COLOR_PANEL, COLOR_PANEL_BORDER, 1.5f);

        // Header
        g_Renderer.DrawText("Вход в huEVO", formX + 30, startY + 25, COLOR_TEXT_PRIMARY, 1.5f, true);
        g_Renderer.DrawText("социальная сеть в духе старого интернета", formX + 30, startY + 55, COLOR_TEXT_MUTED, 1.05f, false);
        g_Renderer.DrawLine(formX + 30, startY + 80, formX + formW - 30, startY + 80, COLOR_PANEL_BORDER, 1.0f);

        // Field 0: Username
        bool sel0 = (m_SelectedIndex == 0);
        float uY = startY + 105;
        g_Renderer.DrawText("Логин или e-mail:", formX + 30, uY, COLOR_TEXT_MUTED, 1.1f, false);
        g_Renderer.DrawRoundedRect(formX + 30, uY + 22, formW - 60, 42, 5.0f, sel0 ? COLOR_SELECTION : 0xFF1B1C1E, sel0 ? COLOR_ACCENT : COLOR_PANEL_BORDER, 1.5f);
        std::string uText = g_Config.username.empty() ? "Нажмите [A] для ввода логина" : g_Config.username;
        g_Renderer.DrawText(uText, formX + 45, uY + 34, g_Config.username.empty() ? COLOR_TEXT_MUTED : COLOR_TEXT_PRIMARY, 1.15f, false);

        // Field 1: Password
        bool sel1 = (m_SelectedIndex == 1);
        float pY = uY + 80;
        g_Renderer.DrawText("Пароль:", formX + 30, pY, COLOR_TEXT_MUTED, 1.1f, false);
        g_Renderer.DrawRoundedRect(formX + 30, pY + 22, formW - 60, 42, 5.0f, sel1 ? COLOR_SELECTION : 0xFF1B1C1E, sel1 ? COLOR_ACCENT : COLOR_PANEL_BORDER, 1.5f);
        std::string pText = g_Config.password.empty() ? "Нажмите [A] для ввода пароля" : "********";
        g_Renderer.DrawText(pText, formX + 45, pY + 34, g_Config.password.empty() ? COLOR_TEXT_MUTED : COLOR_TEXT_PRIMARY, 1.15f, false);

        // Field 2: Submit Button
        bool sel2 = (m_SelectedIndex == 2);
        float bY = pY + 80;
        g_Renderer.DrawRoundedRect(formX + 30, bY, formW - 60, 46, 6.0f, sel2 ? 0xFF6598D0 : COLOR_ACCENT);
        g_Renderer.DrawText("Войти на huevo.su", formX + 210, bY + 14, 0xFFFFFFFF, 1.25f, true);

        g_Renderer.DrawText("Прямое подключение к https://huevo.su через кабель Ethernet", formX + 40, startY + 385, COLOR_TEXT_MUTED, 1.0f, false);
        return;
    }

    // Profile Header Card for logged in user
    g_Renderer.DrawRoundedRect(SAFE_MARGIN_X, startY, cardW, 190, 8.0f, COLOR_PANEL, COLOR_PANEL_BORDER, 1.0f);

    // Avatar Frame
    g_Renderer.DrawRoundedRect(SAFE_MARGIN_X + 30, startY + 25, 96, 96, 10.0f, COLOR_ACCENT);
    g_Renderer.DrawText("huEVO", SAFE_MARGIN_X + 44, startY + 65, 0xFFFFFFFF, 1.1f, false);

    std::string displayName = g_Config.username;
    std::string handle = "@" + g_Config.username + " (huevo.su)";

    g_Renderer.DrawText(displayName, SAFE_MARGIN_X + 150, startY + 30, COLOR_TEXT_PRIMARY, 1.6f, true);
    g_Renderer.DrawText(handle, SAFE_MARGIN_X + 150, startY + 65, COLOR_TEXT_LINK, 1.2f, false);
    g_Renderer.DrawText("\"Социальная сеть в духе старого интернета на Xbox 360\"", SAFE_MARGIN_X + 150, startY + 98, COLOR_TEXT_MUTED, 1.15f, false);

    g_Renderer.DrawBadge("[A] Выйти из профиля", SAFE_MARGIN_X + 150, startY + 138, 0xFF36383D, 0xFFFFFFFF);
    g_Renderer.DrawBadge("ONLINE", SAFE_MARGIN_X + 380, startY + 138, COLOR_ACCENT, 0xFFFFFFFF);

    // Wall section
    g_Renderer.DrawRoundedRect(SAFE_MARGIN_X, startY + 220, cardW, 280, 8.0f, COLOR_PANEL, COLOR_PANEL_BORDER, 1.0f);
    g_Renderer.DrawText("Моя стена на huEVO", SAFE_MARGIN_X + 30, startY + 240, COLOR_TEXT_PRIMARY, 1.3f, true);
    g_Renderer.DrawLine(SAFE_MARGIN_X + 30, startY + 270, SAFE_MARGIN_X + cardW - 30, startY + 270, COLOR_PANEL_BORDER, 1.0f);
    g_Renderer.DrawText("Нажмите [X] для добавления новой записи на стену.", SAFE_MARGIN_X + 30, startY + 300, COLOR_TEXT_MUTED, 1.15f, false);
}

void UIManager::RenderMessagesView() {
    float startY = 100.0f;
    float cardW = SCREEN_WIDTH - (SAFE_MARGIN_X * 2);
    float itemH = 75.0f;

    g_Renderer.DrawText("Личные диалоги huEVO", SAFE_MARGIN_X, startY - 10, COLOR_TEXT_PRIMARY, 1.3f, true);

    for (size_t i = 0; i < m_Dialogs.size(); ++i) {
        float cy = startY + 30 + i * (itemH + 12.0f);
        bool isSelected = (static_cast<int>(i) == m_SelectedIndex);

        uint32_t panelColor = isSelected ? COLOR_SELECTION : COLOR_PANEL;
        uint32_t borderColor = isSelected ? COLOR_ACCENT : COLOR_PANEL_BORDER;

        g_Renderer.DrawRoundedRect(SAFE_MARGIN_X, cy, cardW, itemH, 6.0f, panelColor, borderColor, isSelected ? 2.0f : 1.0f);
        g_Renderer.DrawText(m_Dialogs[i].peerName, SAFE_MARGIN_X + 20, cy + 16, COLOR_TEXT_PRIMARY, 1.2f, true);
        g_Renderer.DrawText(m_Dialogs[i].lastMessage, SAFE_MARGIN_X + 20, cy + 44, COLOR_TEXT_MUTED, 1.05f, false);
        g_Renderer.DrawText(m_Dialogs[i].timestamp, cardW - 60, cy + 16, COLOR_TEXT_MUTED, 0.95f, false);

        if (m_Dialogs[i].unreadCount > 0) {
            g_Renderer.DrawBadge("NEW", cardW - 60, cy + 42, COLOR_BADGE, 0xFFFFFFFF);
        }
    }
}

void UIManager::RenderMusicView() {
    float startY = 100.0f;
    float cardW = SCREEN_WIDTH - (SAFE_MARGIN_X * 2);

    // Audio Player HUD
    g_Renderer.DrawRoundedRect(SAFE_MARGIN_X, startY, cardW, 170, 8.0f, COLOR_PANEL, COLOR_PANEL_BORDER, 1.5f);

    std::string nowPlayingTitle = m_Tracks.empty() ? "Нет треков" : m_Tracks[m_CurrentTrackIndex].title;
    std::string nowPlayingArtist = m_Tracks.empty() ? "-" : m_Tracks[m_CurrentTrackIndex].artist;

    g_Renderer.DrawText("СЕЙЧАС ИГРАЕТ:", SAFE_MARGIN_X + 30, startY + 25, COLOR_TEXT_MUTED, 1.0f, false);
    g_Renderer.DrawText(nowPlayingTitle, SAFE_MARGIN_X + 30, startY + 50, COLOR_TEXT_PRIMARY, 1.5f, true);
    g_Renderer.DrawText(nowPlayingArtist, SAFE_MARGIN_X + 30, startY + 85, COLOR_TEXT_LINK, 1.2f, false);

    // Retro Audio Visualizer Bars
    for (int b = 0; b < 24; ++b) {
        float barH = m_IsMusicPlaying ? (15.0f + 25.0f * std::sin(m_VisualizerPhase + b * 0.45f) * std::cos(m_VisualizerPhase * 0.5f)) : 4.0f;
        barH = (std::max)(4.0f, barH);
        float bx = SAFE_MARGIN_X + cardW - 280.0f + b * 10.0f;
        g_Renderer.DrawRect(bx, startY + 110.0f - barH, 7.0f, barH, COLOR_ACCENT);
    }

    // Progress bar
    g_Renderer.DrawProgressBar(SAFE_MARGIN_X + 30, startY + 125, cardW - 320, 10.0f, m_IsMusicPlaying ? 0.42f : 0.0f, COLOR_ACCENT, 0xFF101520);
    g_Renderer.DrawText("01:28 / 03:32", SAFE_MARGIN_X + cardW - 260, startY + 122, COLOR_TEXT_MUTED, 0.95f, false);

    // Tracklist
    float listY = startY + 200.0f;
    g_Renderer.DrawText("Плейлист huEVO", SAFE_MARGIN_X, listY, COLOR_TEXT_PRIMARY, 1.3f, true);

    for (size_t i = 0; i < m_Tracks.size(); ++i) {
        float ty = listY + 30.0f + i * 50.0f;
        bool isSelected = (static_cast<int>(i) == m_SelectedIndex);

        uint32_t panelColor = isSelected ? COLOR_SELECTION : COLOR_PANEL;
        g_Renderer.DrawRoundedRect(SAFE_MARGIN_X, ty, cardW, 44.0f, 5.0f, panelColor, isSelected ? COLOR_ACCENT : 0, 1.5f);

        std::string trackLabel = IntToString(i + 1) + ". " + m_Tracks[i].title + " - " + m_Tracks[i].artist;
        g_Renderer.DrawText(trackLabel, SAFE_MARGIN_X + 20, ty + 14, isSelected ? 0xFFFFFFFF : COLOR_TEXT_MUTED, 1.15f, true);
        g_Renderer.DrawText(m_Tracks[i].duration, cardW - 20, ty + 14, COLOR_TEXT_MUTED, 1.0f, false);
    }
}

void UIManager::RenderSettingsView() {
    float startY = 110.0f;
    float cardW = SCREEN_WIDTH - (SAFE_MARGIN_X * 2);

    g_Renderer.DrawText("Настройки huEVO", SAFE_MARGIN_X, startY, COLOR_TEXT_PRIMARY, 1.4f, true);

    // Row 0: Network Mode
    float rowY0 = startY + 40.0f;
    bool sel0 = (m_SelectedIndex == 0);
    g_Renderer.DrawRoundedRect(SAFE_MARGIN_X, rowY0, cardW, 70.0f, 6.0f, sel0 ? COLOR_SELECTION : COLOR_PANEL, sel0 ? COLOR_ACCENT : COLOR_PANEL_BORDER, 1.5f);
    g_Renderer.DrawText("Режим подключения (Нажмите A для переключения):", SAFE_MARGIN_X + 20, rowY0 + 16, COLOR_TEXT_PRIMARY, 1.1f, true);

    std::string modeStr = (g_Config.mode == ConnectionMode::DIRECT_STANDALONE)
        ? ">> ПРЯМОЕ ПОДКЛЮЧЕНИЕ (Проводной интернет Ethernet / Wi-Fi) <<"
        : ">> COMPANION BRIDGE (Локальный ПК) <<";
    g_Renderer.DrawText(modeStr, SAFE_MARGIN_X + 20, rowY0 + 42, COLOR_ACCENT, 1.2f, false);

    // Row 1: Target Host
    float rowY1 = rowY0 + 85.0f;
    bool sel1 = (m_SelectedIndex == 1);
    g_Renderer.DrawRoundedRect(SAFE_MARGIN_X, rowY1, cardW, 70.0f, 6.0f, sel1 ? COLOR_SELECTION : COLOR_PANEL, sel1 ? COLOR_ACCENT : COLOR_PANEL_BORDER, 1.0f);
    g_Renderer.DrawText("Адрес сервера huEVO:", SAFE_MARGIN_X + 20, rowY1 + 16, COLOR_TEXT_PRIMARY, 1.1f, true);
    std::string endpointStr = g_Config.directHost + ":" + IntToString(g_Config.directPort) + " (HTTPS / TLS 1.2)";
    g_Renderer.DrawText(endpointStr, SAFE_MARGIN_X + 20, rowY1 + 42, COLOR_TEXT_LINK, 1.25f, false);

    // Row 2: Console & Network Status
    float rowY2 = rowY1 + 85.0f;
    g_Renderer.DrawRoundedRect(SAFE_MARGIN_X, rowY2, cardW, 140.0f, 6.0f, COLOR_PANEL, COLOR_PANEL_BORDER, 1.0f);
    g_Renderer.DrawText("Автономная работа на Xbox 360:", SAFE_MARGIN_X + 20, rowY2 + 18, COLOR_TEXT_PRIMARY, 1.15f, true);
    g_Renderer.DrawText("Консоль использует встроенный TLS/HTTP стек для связи с huevo.su.", SAFE_MARGIN_X + 20, rowY2 + 50, COLOR_TEXT_MUTED, 1.1f, false);
    g_Renderer.DrawText("Все сетевые запросы идут через сетевой провод консоли напрямую.", SAFE_MARGIN_X + 20, rowY2 + 78, COLOR_TEXT_MUTED, 1.1f, false);
    g_Renderer.DrawText("Никаких сторонних программ на ПК не требуется!", SAFE_MARGIN_X + 20, rowY2 + 106, COLOR_ACCENT, 1.1f, true);
}

void UIManager::RenderFooter() {
    float barY = SCREEN_HEIGHT - 54.0f;

    // Bottom Bar Background
    g_Renderer.DrawGradientV(0, barY, SCREEN_WIDTH, 54, 0xEE0B0E14, 0xFF141923);
    g_Renderer.DrawLine(0, barY, SCREEN_WIDTH, barY, COLOR_PANEL_BORDER, 1.5f);

    float px = SAFE_MARGIN_X;
    g_Renderer.DrawButtonPrompt("A", "Выбрать / Действие", px, barY + 16, 0xFF107C10); px += 230;
    g_Renderer.DrawButtonPrompt("X", "Написать", px, barY + 16, 0xFF0078D7); px += 160;
    g_Renderer.DrawButtonPrompt("Y", "Лайк / Обновить", px, barY + 16, 0xFFFFB900); px += 210;
    g_Renderer.DrawButtonPrompt("Back", "В Aurora", px, barY + 16, 0xFF5C6BC0);
}

void UIManager::RenderToast() {
    if (m_ToastTimer <= 0) return;

    float toastW = g_Renderer.MeasureTextWidth(m_ToastMessage, 1.1f) + 40.0f;
    float toastH = 40.0f;
    float tx = (SCREEN_WIDTH - toastW) * 0.5f;
    float ty = 85.0f;

    g_Renderer.DrawRoundedRect(tx, ty, toastW, toastH, 8.0f, 0xF0107C10, 0xFFFFFFFF, 1.5f);
    g_Renderer.DrawText(m_ToastMessage, tx + 20.0f, ty + 12.0f, 0xFFFFFFFF, 1.1f, true);
}

} // namespace huEVO
