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
#include <cstdio>

namespace huEVO {

UIManager g_UI;

enum class PendingKeyboardAction {
    NONE,
    CREATE_POST,
    SEND_MESSAGE,
    SEARCH,
    CONFIG_IP
};

static PendingKeyboardAction s_PendingAction = PendingKeyboardAction::NONE;

UIManager::UIManager()
    : m_CurrentTab(ViewTab::PROFILE)
    , m_FocusArea(FocusArea::SIDEBAR)
    , m_SidebarIndex(0)
    , m_ContentIndex(0)
    , m_ScrollOffset(0)
    , m_InChatMode(false)
    , m_ChatScrollOffset(0)
    , m_MarketSubTab(0)
    , m_CopilkaClicks(0)
    , m_CurrentTrackIndex(0)
    , m_IsPlaying(false)
    , m_TrackProgress(0.25f)
    , m_VisualizerPhase(0.0f)
    , m_ToastTimer(0)
{
}

void UIManager::Initialize() {
    g_Network.FetchProfile("", m_User, m_UserPosts);
    g_Network.FetchFeed(m_FeedPosts);
    g_Network.FetchDialogs(m_Dialogs);
    g_Network.FetchFriends(m_Friends);
    g_Network.FetchNotifications(m_Notifications);
    g_Network.FetchCommunities(m_Communities);
    g_Network.FetchAudioTracks(m_Tracks);
    g_Network.FetchVideos(m_Videos);
    g_Network.FetchMarket(m_MarketItems);

    ShowToast("huEVO Xbox 360 загружен! [Y] - обновить, [X] - написать", 150);
}

void UIManager::SetTab(ViewTab tab) {
    if (m_CurrentTab != tab) {
        m_CurrentTab = tab;
        m_SidebarIndex = static_cast<int>(tab);
        m_ContentIndex = 0;
        m_ScrollOffset = 0;
        m_InChatMode = false;
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

void UIManager::Update() {
    // Update active virtual keyboard
    if (g_Keyboard.IsActive()) {
        g_Keyboard.Update();
        if (g_Keyboard.HasResult()) {
            std::string text = g_Keyboard.GetResult();
            if (!text.empty()) {
                if (s_PendingAction == PendingKeyboardAction::CREATE_POST) {
                    Post newPost;
                    g_Network.CreatePost(text, newPost);
                    m_FeedPosts.insert(m_FeedPosts.begin(), newPost);
                    m_UserPosts.insert(m_UserPosts.begin(), newPost);
                    ShowToast("Запись опубликована на стене!", 90);
                } else if (s_PendingAction == PendingKeyboardAction::SEND_MESSAGE) {
                    if (m_InChatMode && !m_ActiveDialogId.empty()) {
                        DirectMessage msg;
                        g_Network.SendDirectMessage(m_ActiveDialogId, text, msg);
                        m_ActiveMessages.push_back(msg);
                        ShowToast("Сообщение отправлено", 60);
                    }
                } else if (s_PendingAction == PendingKeyboardAction::CONFIG_IP) {
                    g_Config.bridgeHost = text;
                    ShowToast("IP моста сохранён: " + text, 90);
                }
            }
            s_PendingAction = PendingKeyboardAction::NONE;
        } else if (g_Keyboard.WasCancelled()) {
            s_PendingAction = PendingKeyboardAction::NONE;
        }
        return;
    }

    // Music equalizer phase
    if (m_IsPlaying) {
        m_VisualizerPhase += 0.08f;
        m_TrackProgress += 0.0005f;
        if (m_TrackProgress > 1.0f) m_TrackProgress = 0.0f;
    }

    if (m_ToastTimer > 0) m_ToastTimer--;

    HandleNavigation();
    HandleActions();
}

void UIManager::HandleNavigation() {
    // Tab switching via Bumpers
    if (g_Input.IsPressed(XINPUT_GAMEPAD_LEFT_SHOULDER)) {
        PrevTab();
        return;
    }
    if (g_Input.IsPressed(XINPUT_GAMEPAD_RIGHT_SHOULDER)) {
        NextTab();
        return;
    }

    // Horizontal focus switch between Sidebar and Content
    if (m_FocusArea == FocusArea::SIDEBAR) {
        if (g_Input.NavRight() || g_Input.IsPressed(XINPUT_GAMEPAD_A)) {
            m_FocusArea = FocusArea::CONTENT;
            m_ContentIndex = 0;
            return;
        }
        if (g_Input.NavUp()) {
            if (m_SidebarIndex > 0) {
                m_SidebarIndex--;
                SetTab(static_cast<ViewTab>(m_SidebarIndex));
            }
        }
        if (g_Input.NavDown()) {
            if (m_SidebarIndex < static_cast<int>(ViewTab::COUNT) - 1) {
                m_SidebarIndex++;
                SetTab(static_cast<ViewTab>(m_SidebarIndex));
            }
        }
    } else { // FocusArea::CONTENT
        if (g_Input.NavLeft()) {
            if (!m_InChatMode) {
                m_FocusArea = FocusArea::SIDEBAR;
                return;
            }
        }

        if (g_Input.NavUp()) {
            if (m_ContentIndex > 0) {
                m_ContentIndex--;
                if (m_ContentIndex < m_ScrollOffset) {
                    m_ScrollOffset = m_ContentIndex;
                }
            }
        }
        if (g_Input.NavDown()) {
            m_ContentIndex++;
            if (m_ContentIndex > m_ScrollOffset + 4) {
                m_ScrollOffset = m_ContentIndex - 4;
            }
        }
    }
}

void UIManager::HandleActions() {
    // (B) Back button
    if (g_Input.IsPressed(XINPUT_GAMEPAD_B)) {
        if (m_InChatMode) {
            m_InChatMode = false;
            m_FocusArea = FocusArea::CONTENT;
            return;
        }
        if (m_FocusArea == FocusArea::CONTENT) {
            m_FocusArea = FocusArea::SIDEBAR;
            return;
        }
    }

    // (Y) Refresh / Action button
    if (g_Input.IsPressed(XINPUT_GAMEPAD_Y)) {
        if (m_CurrentTab == ViewTab::FEED) {
            g_Network.FetchFeed(m_FeedPosts);
            ShowToast("Лента новостей обновлена", 80);
        } else if (m_CurrentTab == ViewTab::MESSAGES) {
            g_Network.FetchDialogs(m_Dialogs);
            ShowToast("Диалоги обновлены", 80);
        } else {
            ShowToast("Данные синхронизированы", 60);
        }
    }

    // (X) Compose button
    if (g_Input.IsPressed(XINPUT_GAMEPAD_X)) {
        if (m_InChatMode) {
            s_PendingAction = PendingKeyboardAction::SEND_MESSAGE;
            g_Keyboard.Prompt("Сообщение", "Напишите сообщение в чат", "");
        } else if (m_CurrentTab == ViewTab::FEED || m_CurrentTab == ViewTab::PROFILE) {
            s_PendingAction = PendingKeyboardAction::CREATE_POST;
            g_Keyboard.Prompt("Новая запись", "Что у вас нового?", "");
        }
    }

    // (A) Select / Interact button
    if (g_Input.IsPressed(XINPUT_GAMEPAD_A) && m_FocusArea == FocusArea::CONTENT) {
        if (m_CurrentTab == ViewTab::FEED) {
            if (!m_FeedPosts.empty() && m_ContentIndex < static_cast<int>(m_FeedPosts.size())) {
                bool liked = false;
                int count = 0;
                g_Network.ToggleLike(m_FeedPosts[m_ContentIndex].id, liked, count);
                m_FeedPosts[m_ContentIndex].isLiked = liked;
                m_FeedPosts[m_ContentIndex].likesCount = count;
                ShowToast(liked ? "Вы оценили запись!" : "Оценка убрана", 50);
            }
        } else if (m_CurrentTab == ViewTab::MESSAGES) {
            if (!m_InChatMode && !m_Dialogs.empty() && m_ContentIndex < static_cast<int>(m_Dialogs.size())) {
                m_InChatMode = true;
                m_ActiveDialogId = m_Dialogs[m_ContentIndex].id;
                m_ActiveDialogTitle = m_Dialogs[m_ContentIndex].peerName;
                m_ActiveMessages = m_Dialogs[m_ContentIndex].messages;
                m_ChatScrollOffset = 0;
            } else if (m_InChatMode) {
                s_PendingAction = PendingKeyboardAction::SEND_MESSAGE;
                g_Keyboard.Prompt("Сообщение", "Напишите ответ в чат", "");
            }
        } else if (m_CurrentTab == ViewTab::MUSIC) {
            if (!m_Tracks.empty() && m_ContentIndex < static_cast<int>(m_Tracks.size())) {
                m_CurrentTrackIndex = m_ContentIndex;
                m_IsPlaying = !m_IsPlaying;
                ShowToast(m_IsPlaying ? "Воспроизведение: " + m_Tracks[m_CurrentTrackIndex].title
                                      : "Пауза", 60);
            }
        } else if (m_CurrentTab == ViewTab::MARKET) {
            if (m_MarketSubTab == 1) { // Copilka clicker
                m_CopilkaClicks++;
                if (m_CopilkaClicks % 10 == 0) {
                    g_Config.coinsBalance++;
                    ShowToast("+1 монэтка в копилку!", 50);
                }
            } else {
                ShowToast("Лот зарезервирован!", 60);
            }
        } else if (m_CurrentTab == ViewTab::SETTINGS) {
            s_PendingAction = PendingKeyboardAction::CONFIG_IP;
            g_Keyboard.Prompt("IP Моста", "Введите IP адрес компьютера с bridge.py", g_Config.bridgeHost);
        }
    }
}

void UIManager::Render() {
    RenderHeader();
    RenderSidebar();

    switch (m_CurrentTab) {
        case ViewTab::PROFILE:       RenderProfileTab();       break;
        case ViewTab::FEED:          RenderFeedTab();          break;
        case ViewTab::MESSAGES:      RenderMessagesTab();      break;
        case ViewTab::FRIENDS:       RenderFriendsTab();       break;
        case ViewTab::NOTIFICATIONS: RenderNotificationsTab(); break;
        case ViewTab::COMMUNITIES:   RenderCommunitiesTab();   break;
        case ViewTab::MUSIC:         RenderMusicTab();         break;
        case ViewTab::VIDEOS:        RenderVideosTab();        break;
        case ViewTab::MARKET:        RenderMarketTab();        break;
        case ViewTab::SETTINGS:      RenderSettingsTab();      break;
        default: break;
    }

    RenderBottomBar();
    RenderToast();
}

void UIManager::RenderHeader() {
    // Top Bar Background
    g_Renderer.DrawRect(0, 0, SCREEN_WIDTH, 48, COLOR_TOPBAR);
    g_Renderer.DrawDivider(0, 48, SCREEN_WIDTH, COLOR_CARD_BORDER);

    // huEVO Logo (VK classic 2016 style)
    g_Renderer.DrawRoundedRect(SAFE_MARGIN_X, 10, 28, 28, 5.0f, COLOR_ACCENT_BLUE);
    g_Renderer.DrawText("h", SAFE_MARGIN_X + 9, 14, 0xFFFFFFFF, 1.4f, false);
    g_Renderer.DrawText("huEVO", SAFE_MARGIN_X + 36, 14, COLOR_TEXT_PRIMARY, 1.4f, true);

    // Search / Info hint in center
    g_Renderer.DrawRoundedRect(SAFE_MARGIN_X + 160, 10, 360, 28, 4.0f, 0xFF2A2B2E);
    g_Renderer.DrawText("Q Поиск по huEVO", SAFE_MARGIN_X + 172, 16, COLOR_TEXT_MUTED, 1.0f, false);

    // Announcement snippet from screenshots
    g_Renderer.DrawText("добро пожаловать в ретро-сеть huEVO", SAFE_MARGIN_X + 540, 16, COLOR_TEXT_MUTED, 0.95f, true);

    // User Badge on right
    float userX = SCREEN_WIDTH - SAFE_MARGIN_X - 220;
    g_Renderer.DrawAvatar(userX, 8, 32, m_User.avatarColor, "Н");
    g_Renderer.DrawText(m_User.displayName, userX + 38, 11, COLOR_TEXT_PRIMARY, 1.05f, true);

    // Coins counter badge
    char coinBuf[32];
    snprintf(coinBuf, sizeof(coinBuf), "%d м.", g_Config.coinsBalance);
    g_Renderer.DrawBadge(coinBuf, userX + 38, 28, 0xFF2B2516, COLOR_COIN_GOLD);
}

void UIManager::RenderSidebar() {
    float x = SAFE_MARGIN_X;
    float y = 58;
    float w = 176;
    float itemH = 34;

    const char* tabs[] = {
        "Моя Страница",
        "Новости",
        "Сообщения",
        "Друзья",
        "Уведомления",
        "Сообщества",
        "Звучок",
        "Видеозаписи",
        "Маркет и Игры",
        "Настройки"
    };

    for (int i = 0; i < static_cast<int>(ViewTab::COUNT); ++i) {
        float itemY = y + i * itemH;
        bool isCurrent = (m_SidebarIndex == i);
        bool hasFocus = (m_FocusArea == FocusArea::SIDEBAR && isCurrent);

        if (hasFocus) {
            g_Renderer.DrawRoundedRect(x, itemY, w, itemH - 4, 4.0f, COLOR_SELECTION, COLOR_SELECTION_BORDER, 1.5f);
        } else if (isCurrent) {
            g_Renderer.DrawRoundedRect(x, itemY, w, itemH - 4, 4.0f, 0xFF24262A);
        }

        uint32_t textColor = hasFocus ? 0xFFFFFFFF : (isCurrent ? COLOR_ACCENT_BLUE : COLOR_TEXT_PRIMARY);
        g_Renderer.DrawText(tabs[i], x + 12, itemY + 8, textColor, 1.05f, hasFocus);

        // Badges on sidebar
        if (i == 2 && !m_Dialogs.empty()) { // Messages badge
            g_Renderer.DrawBadge("99+", x + w - 38, itemY + 6, COLOR_BADGE_RED, 0xFFFFFFFF);
        } else if (i == 3) { // Friends badge
            g_Renderer.DrawBadge("88", x + w - 30, itemY + 6, 0xFF36383D, COLOR_TEXT_MUTED);
        } else if (i == 4) { // Notifications badge
            g_Renderer.DrawBadge("3", x + w - 24, itemY + 6, COLOR_ACCENT_BLUE, 0xFFFFFFFF);
        }
    }

    // Sidebar footer info
    float footY = y + 10 * itemH + 20;
    g_Renderer.DrawDivider(x, footY, w, COLOR_CARD_BORDER);
    g_Renderer.DrawText("Язык: Русский", x + 12, footY + 12, COLOR_TEXT_MUTED, 0.85f, false);
    g_Renderer.DrawText("huEVO © 2026", x + 12, footY + 28, COLOR_TEXT_MUTED, 0.85f, false);
    g_Renderer.DrawText("Xbox 360 Edition", x + 12, footY + 44, COLOR_XBOX_GREEN, 0.85f, false);
}

void UIManager::RenderBottomBar() {
    float y = SCREEN_HEIGHT - 38;
    g_Renderer.DrawRect(0, y, SCREEN_WIDTH, 38, COLOR_TOPBAR);
    g_Renderer.DrawDivider(0, y, SCREEN_WIDTH, COLOR_CARD_BORDER);

    float curX = SAFE_MARGIN_X;
    g_Renderer.DrawButtonPrompt("A", "Выбрать", curX, y + 8, 0xFF107C10);
    curX += 130;
    g_Renderer.DrawButtonPrompt("B", "Назад", curX, y + 8, 0xFFC92828);
    curX += 110;
    g_Renderer.DrawButtonPrompt("X", "Написать", curX, y + 8, 0xFF0078D7);
    curX += 130;
    g_Renderer.DrawButtonPrompt("Y", "Обновить", curX, y + 8, 0xFFD89B00);
    curX += 130;
    g_Renderer.DrawButtonPrompt("LB/RB", "Вкладки", curX, y + 8, 0xFF555555);
}

void UIManager::RenderToast() {
    if (m_ToastTimer <= 0) return;
    float toastW = g_Renderer.MeasureTextWidth(m_ToastMessage, 1.05f) + 32.0f;
    float toastH = 34.0f;
    float toastX = (SCREEN_WIDTH - toastW) * 0.5f;
    float toastY = 56.0f;

    g_Renderer.DrawRoundedRect(toastX, toastY, toastW, toastH, 6.0f, 0xEE1E2430, COLOR_ACCENT_BLUE, 1.5f);
    g_Renderer.DrawText(m_ToastMessage, toastX + 16, toastY + 9, 0xFFFFFFFF, 1.05f, true);
}

// -------------------------------------------------------------------------------------------------
// TAB 0: PROFILE (Моя Страница)
// -------------------------------------------------------------------------------------------------
void UIManager::RenderProfileTab() {
    float startX = 250;
    float startY = 58;
    float w = 960;

    // 1. Profile Header Card
    g_Renderer.DrawRoundedRect(startX, startY, w, 150, 6.0f, COLOR_CARD, COLOR_CARD_BORDER, 1.0f);

    // Large Avatar
    g_Renderer.DrawAvatar(startX + 16, startY + 16, 80, m_User.avatarColor, "Н");

    // Name + Verified mark + Status
    float infoX = startX + 110;
    g_Renderer.DrawText(m_User.displayName, infoX, startY + 16, COLOR_TEXT_PRIMARY, 1.4f, true);
    g_Renderer.DrawBadge("✓", infoX + g_Renderer.MeasureTextWidth(m_User.displayName, 1.4f) + 10, startY + 17, COLOR_ACCENT_BLUE, 0xFFFFFFFF);
    g_Renderer.DrawText(m_User.handle + "  " + m_User.extraHandles, infoX, startY + 40, COLOR_TEXT_MUTED, 0.95f, false);

    // Online status
    g_Renderer.DrawRect(infoX, startY + 58, 6, 6, COLOR_ACCENT_GREEN);
    g_Renderer.DrawText("online", infoX + 12, startY + 56, COLOR_ACCENT_GREEN, 0.95f, false);
    g_Renderer.DrawText("«" + m_User.statusText + "»", infoX + 70, startY + 56, COLOR_TEXT_PRIMARY, 0.95f, true);

    // Detail rows
    g_Renderer.DrawText("Город: " + m_User.city, infoX, startY + 76, COLOR_TEXT_MUTED, 0.95f, false);
    g_Renderer.DrawText("День рождения: " + m_User.birthday, infoX + 220, startY + 76, COLOR_TEXT_MUTED, 0.95f, false);
    g_Renderer.DrawText("Работа: " + m_User.workplace, infoX, startY + 96, COLOR_TEXT_MUTED, 0.95f, false);
    g_Renderer.DrawText("Репутация: " + m_User.reputation, infoX + 480, startY + 96, COLOR_COIN_GOLD, 0.95f, true);

    // Stats Bar
    g_Renderer.DrawDivider(startX + 16, startY + 118, w - 32, COLOR_CARD_BORDER);
    char statBuf[128];
    snprintf(statBuf, sizeof(statBuf), "%d друзей   %d подписчик   %d групп   %d фото   %d аудио   %d видео",
        m_User.friendsCount, m_User.followersCount, m_User.groupsCount, m_User.photosCount, m_User.audioCount, m_User.videoCount);
    g_Renderer.DrawText(statBuf, startX + 24, startY + 126, COLOR_ACCENT_BLUE, 1.0f, false);

    // 2. Gifts Showcase Card
    float giftY = startY + 162;
    g_Renderer.DrawRoundedRect(startX, giftY, w, 76, 6.0f, COLOR_CARD, COLOR_CARD_BORDER, 1.0f);
    g_Renderer.DrawText("Витрина 36  все подарки", startX + 16, giftY + 10, COLOR_TEXT_MUTED, 0.95f, false);

    const char* gifts[] = { "nice guy #186", "derpina #331", "crying #220", "nya #197", "pepe #79" };
    for (int i = 0; i < 5; ++i) {
        float gx = startX + 16 + i * 184;
        g_Renderer.DrawRoundedRect(gx, giftY + 30, 172, 36, 4.0f, 0xFF24262B);
        g_Renderer.DrawAvatar(gx + 6, giftY + 34, 28, 0xFF8A5A36 + i * 0x112233, "★");
        g_Renderer.DrawTextClipped(gifts[i], gx + 40, giftY + 42, 120, COLOR_TEXT_PRIMARY, 0.85f, true);
    }

    // 3. Wall Posts
    float postY = giftY + 86;
    g_Renderer.DrawRoundedRect(startX, postY, w, 40, 5.0f, COLOR_CARD, COLOR_CARD_BORDER, 1.0f);
    g_Renderer.DrawText("Что у вас нового? (Нажмите [X] чтобы написать)", startX + 16, postY + 12, COLOR_TEXT_MUTED, 1.0f, false);

    float cardY = postY + 48;
    for (size_t i = 0; i < m_UserPosts.size() && i < 2; ++i) {
        const Post& p = m_UserPosts[i];
        bool focused = (m_FocusArea == FocusArea::CONTENT && m_ContentIndex == static_cast<int>(i));

        g_Renderer.DrawRoundedRect(startX, cardY, w, 110, 6.0f, focused ? COLOR_SELECTION : COLOR_CARD,
            focused ? COLOR_SELECTION_BORDER : COLOR_CARD_BORDER, focused ? 1.5f : 1.0f);

        g_Renderer.DrawAvatar(startX + 16, cardY + 14, 40, p.authorColor, "Н");
        g_Renderer.DrawText(p.authorName, startX + 66, cardY + 14, COLOR_ACCENT_BLUE, 1.1f, true);
        g_Renderer.DrawText(p.timeAgo, startX + 66, cardY + 32, COLOR_TEXT_MUTED, 0.85f, false);

        g_Renderer.DrawTextClipped(p.content, startX + 16, cardY + 60, w - 32, COLOR_TEXT_PRIMARY, 1.05f, false);

        // Likes & Comments
        char lcBuf[64];
        snprintf(lcBuf, sizeof(lcBuf), "❤ %d    💬 %d", p.likesCount, p.commentsCount);
        g_Renderer.DrawText(lcBuf, startX + 16, cardY + 86, p.isLiked ? COLOR_BADGE_RED : COLOR_TEXT_MUTED, 0.95f, true);

        cardY += 120;
    }
}

// -------------------------------------------------------------------------------------------------
// TAB 1: FEED (Новости)
// -------------------------------------------------------------------------------------------------
void UIManager::RenderFeedTab() {
    float startX = 250;
    float startY = 58;
    float w = 960;

    // Top Announcement Card (from screenshot: t.me/huEVOsocial)
    g_Renderer.DrawRoundedRect(startX, startY, w, 42, 6.0f, 0xFF2A2234, COLOR_ACCENT_BLUE, 1.0f);
    g_Renderer.DrawText("📢 Объявление: добейте 4к подписчиков в https://t.me/huEVOsocial", startX + 16, startY + 14, 0xFFFFFFFF, 1.05f, true);

    // Feed Tabs Pills
    float pillY = startY + 50;
    const char* feedTabs[] = { "Рекомендации", "Все записи", "Друзья и подписки" };
    for (int i = 0; i < 3; ++i) {
        float px = startX + i * 160;
        bool active = (i == 1);
        g_Renderer.DrawRoundedRect(px, pillY, 150, 32, 4.0f, active ? COLOR_ACCENT_BLUE : COLOR_CARD);
        g_Renderer.DrawText(feedTabs[i], px + 18, pillY + 8, active ? 0xFFFFFFFF : COLOR_TEXT_MUTED, 1.0f, false);
    }

    // Wall Post Composer prompt
    float compY = pillY + 40;
    g_Renderer.DrawRoundedRect(startX, compY, w, 40, 5.0f, COLOR_CARD, COLOR_CARD_BORDER, 1.0f);
    g_Renderer.DrawText("Что у вас нового? Нажмите [X] на геймпаде для публикации", startX + 16, compY + 12, COLOR_TEXT_MUTED, 1.0f, false);

    // Post cards stream
    float postY = compY + 48;
    int displayLimit = 3;
    for (int i = 0; i < displayLimit && (i + m_ScrollOffset) < static_cast<int>(m_FeedPosts.size()); ++i) {
        int idx = i + m_ScrollOffset;
        const Post& p = m_FeedPosts[idx];
        bool focused = (m_FocusArea == FocusArea::CONTENT && m_ContentIndex == idx);

        g_Renderer.DrawRoundedRect(startX, postY, w, 116, 6.0f, focused ? COLOR_SELECTION : COLOR_CARD,
            focused ? COLOR_SELECTION_BORDER : COLOR_CARD_BORDER, focused ? 1.5f : 1.0f);

        g_Renderer.DrawAvatar(startX + 16, postY + 14, 40, p.authorColor, "h");
        g_Renderer.DrawText(p.authorName, startX + 66, postY + 14, COLOR_ACCENT_BLUE, 1.1f, true);
        g_Renderer.DrawText(p.authorHandle + " • " + p.timeAgo, startX + 66, postY + 34, COLOR_TEXT_MUTED, 0.85f, false);

        g_Renderer.DrawTextClipped(p.content, startX + 16, postY + 62, w - 32, COLOR_TEXT_PRIMARY, 1.05f, false);

        char lkBuf[64];
        snprintf(lkBuf, sizeof(lkBuf), "[A] ❤ %d     💬 %d комментариев", p.likesCount, p.commentsCount);
        g_Renderer.DrawText(lkBuf, startX + 16, postY + 90, p.isLiked ? COLOR_BADGE_RED : COLOR_TEXT_MUTED, 0.95f, true);

        postY += 126;
    }
}

// -------------------------------------------------------------------------------------------------
// TAB 2: MESSAGES & CHAT (Сообщения)
// -------------------------------------------------------------------------------------------------
void UIManager::RenderMessagesTab() {
    if (m_InChatMode) {
        RenderChatWindow();
        return;
    }

    float startX = 250;
    float startY = 58;
    float w = 960;

    // Header bar
    g_Renderer.DrawRoundedRect(startX, startY, w, 44, 5.0f, COLOR_CARD, COLOR_CARD_BORDER, 1.0f);
    g_Renderer.DrawText("Сообщения (37 диалогов)", startX + 16, startY + 14, COLOR_TEXT_PRIMARY, 1.2f, true);
    g_Renderer.DrawText("Поиск по диалогам и сообщениям", startX + 320, startY + 14, COLOR_TEXT_MUTED, 1.0f, false);
    g_Renderer.DrawButtonPrompt("A", "Открыть чат", startX + w - 160, startY + 12, COLOR_ACCENT_BLUE);

    float cardY = startY + 54;
    for (size_t i = 0; i < m_Dialogs.size() && i < 6; ++i) {
        const Dialog& d = m_Dialogs[i];
        bool focused = (m_FocusArea == FocusArea::CONTENT && m_ContentIndex == static_cast<int>(i));

        g_Renderer.DrawRoundedRect(startX, cardY, w, 68, 5.0f, focused ? COLOR_SELECTION : COLOR_CARD,
            focused ? COLOR_SELECTION_BORDER : COLOR_CARD_BORDER, focused ? 1.5f : 1.0f);

        // Avatar
        g_Renderer.DrawAvatar(startX + 14, cardY + 12, 44, d.peerColor, d.isGroup ? "#" : "П");

        // Peer Name & Members
        g_Renderer.DrawText(d.peerName, startX + 68, cardY + 12, COLOR_ACCENT_BLUE, 1.1f, true);
        if (d.isGroup) {
            char memBuf[32];
            snprintf(memBuf, sizeof(memBuf), "беседа, %d уч.", d.membersCount);
            g_Renderer.DrawText(memBuf, startX + 68 + g_Renderer.MeasureTextWidth(d.peerName, 1.1f) + 12, cardY + 13, COLOR_TEXT_MUTED, 0.85f, false);
        }

        // Last Message
        std::string preview = d.lastSender + ": " + d.lastMessage;
        g_Renderer.DrawTextClipped(preview, startX + 68, cardY + 36, w - 240, COLOR_TEXT_MUTED, 0.95f, false);

        // Timestamp
        g_Renderer.DrawText(d.timestamp, startX + w - 120, cardY + 14, COLOR_TEXT_MUTED, 0.9f, false);

        // Unread Badge
        if (d.unreadCount > 0) {
            char unBuf[16];
            snprintf(unBuf, sizeof(unBuf), "%d+", d.unreadCount);
            g_Renderer.DrawBadge(unBuf, startX + w - 60, cardY + 34, COLOR_BADGE_RED, 0xFFFFFFFF);
        }

        cardY += 76;
    }
}

void UIManager::RenderChatWindow() {
    float startX = 250;
    float startY = 58;
    float w = 960;
    float h = 560;

    // Outer Container
    g_Renderer.DrawRoundedRect(startX, startY, w, h, 6.0f, COLOR_CARD, COLOR_CARD_BORDER, 1.0f);

    // Chat Header
    g_Renderer.DrawRoundedRect(startX, startY, w, 48, 6.0f, 0xFF24262A, COLOR_CARD_BORDER, 1.0f);
    g_Renderer.DrawButtonPrompt("B", "Назад к диалогам", startX + 16, startY + 14, 0xFFC92828);
    g_Renderer.DrawText(m_ActiveDialogTitle, startX + 220, startY + 14, COLOR_TEXT_PRIMARY, 1.25f, true);

    // Messages Stream
    float msgY = startY + 64;
    for (size_t i = 0; i < m_ActiveMessages.size(); ++i) {
        const DirectMessage& m = m_ActiveMessages[i];
        float bubbleW = std::min(w - 180, g_Renderer.MeasureTextWidth(m.text, 1.05f) + 40.0f);
        float bubbleH = 50.0f;

        float bubbleX = m.isOutgoing ? (startX + w - bubbleW - 20) : (startX + 20);
        uint32_t bubbleColor = m.isOutgoing ? 0xFF2B3A4C : 0xFF27282D;

        g_Renderer.DrawRoundedRect(bubbleX, msgY, bubbleW, bubbleH, 6.0f, bubbleColor, COLOR_CARD_BORDER, 1.0f);

        // Sender Name
        g_Renderer.DrawText(m.senderName, bubbleX + 12, msgY + 8, m.isOutgoing ? COLOR_ACCENT_BLUE : 0xFF9E8DB8, 0.9f, true);
        // Message Text
        g_Renderer.DrawTextClipped(m.text, bubbleX + 12, msgY + 26, bubbleW - 24, COLOR_TEXT_PRIMARY, 1.05f, false);
        // Timestamp
        g_Renderer.DrawText(m.timestamp, bubbleX + bubbleW - 46, msgY + 8, COLOR_TEXT_MUTED, 0.8f, false);

        msgY += 58;
    }

    // Bottom Input Bar
    float inputY = startY + h - 50;
    g_Renderer.DrawDivider(startX, inputY, w, COLOR_CARD_BORDER);
    g_Renderer.DrawRoundedRect(startX + 16, inputY + 8, w - 180, 34, 4.0f, 0xFF18191B, COLOR_CARD_BORDER, 1.0f);
    g_Renderer.DrawText("Нажмите [X] на геймпаде чтобы написать сообщение...", startX + 28, inputY + 18, COLOR_TEXT_MUTED, 0.95f, false);
    g_Renderer.DrawButtonPrompt("X", "Отправить", startX + w - 140, inputY + 14, COLOR_ACCENT_BLUE);
}

// -------------------------------------------------------------------------------------------------
// TAB 3: FRIENDS (Друзья)
// -------------------------------------------------------------------------------------------------
void UIManager::RenderFriendsTab() {
    float startX = 250;
    float startY = 58;
    float w = 960;

    // Sub-tabs bar
    g_Renderer.DrawRoundedRect(startX, startY, w, 42, 5.0f, COLOR_CARD, COLOR_CARD_BORDER, 1.0f);
    g_Renderer.DrawText("Все друзья 88   Онлайн 18   Заявки в друзья 1   Подписки 6", startX + 20, startY + 14, COLOR_TEXT_PRIMARY, 1.05f, true);

    float cardY = startY + 54;
    for (size_t i = 0; i < m_Friends.size(); ++i) {
        const Friend& f = m_Friends[i];
        bool focused = (m_FocusArea == FocusArea::CONTENT && m_ContentIndex == static_cast<int>(i));

        g_Renderer.DrawRoundedRect(startX, cardY, w, 68, 5.0f, focused ? COLOR_SELECTION : COLOR_CARD,
            focused ? COLOR_SELECTION_BORDER : COLOR_CARD_BORDER, focused ? 1.5f : 1.0f);

        g_Renderer.DrawAvatar(startX + 16, cardY + 12, 44, f.avatarColor, "Д");
        g_Renderer.DrawText(f.displayName, startX + 70, cardY + 14, COLOR_ACCENT_BLUE, 1.1f, true);
        g_Renderer.DrawText(f.handle + " • " + (f.isOnline ? "online" : "был(-а) недавно"), startX + 70, cardY + 34, f.isOnline ? COLOR_ACCENT_GREEN : COLOR_TEXT_MUTED, 0.9f, false);
        g_Renderer.DrawTextClipped(f.statusText, startX + 320, cardY + 24, 380, COLOR_TEXT_PRIMARY, 0.95f, false);

        g_Renderer.DrawButtonPrompt("A", "Написать", startX + w - 160, cardY + 22, COLOR_ACCENT_BLUE);

        cardY += 76;
    }
}

// -------------------------------------------------------------------------------------------------
// TAB 4: NOTIFICATIONS (Уведомления)
// -------------------------------------------------------------------------------------------------
void UIManager::RenderNotificationsTab() {
    float startX = 250;
    float startY = 58;
    float w = 960;

    g_Renderer.DrawRoundedRect(startX, startY, w, 40, 5.0f, COLOR_CARD, COLOR_CARD_BORDER, 1.0f);
    g_Renderer.DrawText("Уведомления", startX + 16, startY + 12, COLOR_TEXT_PRIMARY, 1.15f, true);

    float cardY = startY + 50;
    for (size_t i = 0; i < m_Notifications.size(); ++i) {
        const NotificationItem& n = m_Notifications[i];
        bool focused = (m_FocusArea == FocusArea::CONTENT && m_ContentIndex == static_cast<int>(i));

        g_Renderer.DrawRoundedRect(startX, cardY, w, 64, 5.0f, focused ? COLOR_SELECTION : COLOR_CARD,
            focused ? COLOR_SELECTION_BORDER : COLOR_CARD_BORDER, focused ? 1.5f : 1.0f);

        g_Renderer.DrawAvatar(startX + 16, cardY + 12, 40, 0xFF6D5494, "🔔");
        std::string line = n.actorName + " (" + n.actorHandle + ") " + n.actionText;
        g_Renderer.DrawText(line, startX + 68, cardY + 14, COLOR_TEXT_PRIMARY, 1.05f, true);
        if (!n.previewText.empty()) {
            g_Renderer.DrawTextClipped(n.previewText, startX + 68, cardY + 34, w - 200, COLOR_TEXT_MUTED, 0.95f, false);
        }
        g_Renderer.DrawText(n.timeAgo, startX + w - 130, cardY + 14, COLOR_TEXT_MUTED, 0.85f, false);

        cardY += 72;
    }
}

// -------------------------------------------------------------------------------------------------
// TAB 5: COMMUNITIES (Сообщества)
// -------------------------------------------------------------------------------------------------
void UIManager::RenderCommunitiesTab() {
    float startX = 250;
    float startY = 58;
    float w = 960;

    g_Renderer.DrawRoundedRect(startX, startY, w, 40, 5.0f, COLOR_CARD, COLOR_CARD_BORDER, 1.0f);
    g_Renderer.DrawText("Мои сообщества 21   Поиск сообществ", startX + 16, startY + 12, COLOR_TEXT_PRIMARY, 1.15f, true);

    float cardY = startY + 50;
    for (size_t i = 0; i < m_Communities.size(); ++i) {
        const Community& c = m_Communities[i];
        bool focused = (m_FocusArea == FocusArea::CONTENT && m_ContentIndex == static_cast<int>(i));

        g_Renderer.DrawRoundedRect(startX, cardY, w, 74, 5.0f, focused ? COLOR_SELECTION : COLOR_CARD,
            focused ? COLOR_SELECTION_BORDER : COLOR_CARD_BORDER, focused ? 1.5f : 1.0f);

        g_Renderer.DrawAvatar(startX + 16, cardY + 12, 50, c.avatarColor, "С");
        g_Renderer.DrawText(c.name, startX + 76, cardY + 14, COLOR_ACCENT_BLUE, 1.15f, true);
        if (c.is18Plus) {
            g_Renderer.DrawBadge("18+", startX + 76 + g_Renderer.MeasureTextWidth(c.name, 1.15f) + 12, cardY + 13, COLOR_BADGE_RED, 0xFFFFFFFF);
        }

        char memBuf[64];
        snprintf(memBuf, sizeof(memBuf), "%d участников • %d записей", c.membersCount, c.postsCount);
        g_Renderer.DrawText(memBuf, startX + 76, cardY + 34, COLOR_TEXT_MUTED, 0.85f, false);
        g_Renderer.DrawTextClipped(c.description, startX + 76, cardY + 50, w - 180, COLOR_TEXT_PRIMARY, 0.9f, false);

        cardY += 82;
    }
}

// -------------------------------------------------------------------------------------------------
// TAB 6: MUSIC / AUDIO (Звучок)
// -------------------------------------------------------------------------------------------------
void UIManager::RenderMusicTab() {
    float startX = 250;
    float startY = 58;
    float w = 960;

    // Player Card with Blades Equalizer
    g_Renderer.DrawRoundedRect(startX, startY, w, 110, 6.0f, COLOR_CARD, COLOR_CARD_BORDER, 1.0f);

    std::string nowPlaying = m_Tracks.empty() ? "Нет трека" : (m_Tracks[m_CurrentTrackIndex].artist + " — " + m_Tracks[m_CurrentTrackIndex].title);
    g_Renderer.DrawText("•|||!• Звучок huEVO", startX + 16, startY + 14, COLOR_ACCENT_BLUE, 1.25f, true);
    g_Renderer.DrawText(nowPlaying, startX + 16, startY + 40, COLOR_TEXT_PRIMARY, 1.2f, true);

    // Progress Bar
    g_Renderer.DrawProgressBar(startX + 16, startY + 70, w - 240, 6.0f, m_TrackProgress, COLOR_ACCENT_BLUE, 0xFF2A2B2E);
    g_Renderer.DrawText(m_Tracks.empty() ? "0:00" : m_Tracks[m_CurrentTrackIndex].duration, startX + w - 210, startY + 66, COLOR_TEXT_MUTED, 0.95f, false);

    // Animated Blades Equalizer Bars
    float eqX = startX + w - 130;
    for (int i = 0; i < 8; ++i) {
        float barH = m_IsPlaying ? (10.0f + 25.0f * std::sin(m_VisualizerPhase + i * 0.7f) * std::sin(m_VisualizerPhase * 0.5f + i)) : 4.0f;
        barH = std::max(4.0f, barH);
        g_Renderer.DrawRect(eqX + i * 14, startY + 80 - barH, 8, barH, COLOR_XBOX_GREEN);
    }

    // Tracklist
    float trackY = startY + 124;
    for (size_t i = 0; i < m_Tracks.size(); ++i) {
        const AudioTrack& t = m_Tracks[i];
        bool isCurrent = (static_cast<int>(i) == m_CurrentTrackIndex);
        bool focused = (m_FocusArea == FocusArea::CONTENT && m_ContentIndex == static_cast<int>(i));

        g_Renderer.DrawRoundedRect(startX, trackY, w, 44, 4.0f, focused ? COLOR_SELECTION : COLOR_CARD,
            focused ? COLOR_SELECTION_BORDER : COLOR_CARD_BORDER, focused ? 1.5f : 1.0f);

        const char* playIcon = (isCurrent && m_IsPlaying) ? "❚❚" : "▶";
        g_Renderer.DrawText(playIcon, startX + 16, trackY + 14, isCurrent ? COLOR_XBOX_GREEN : COLOR_TEXT_MUTED, 1.1f, false);
        g_Renderer.DrawText(t.artist + " — " + t.title, startX + 46, trackY + 14, isCurrent ? 0xFFFFFFFF : COLOR_TEXT_PRIMARY, 1.05f, true);
        g_Renderer.DrawText(t.duration, startX + w - 60, trackY + 14, COLOR_TEXT_MUTED, 0.95f, false);

        trackY += 52;
    }
}

// -------------------------------------------------------------------------------------------------
// TAB 7: VIDEOS (Видеозаписи)
// -------------------------------------------------------------------------------------------------
void UIManager::RenderVideosTab() {
    float startX = 250;
    float startY = 58;
    float w = 960;

    g_Renderer.DrawRoundedRect(startX, startY, w, 40, 5.0f, COLOR_CARD, COLOR_CARD_BORDER, 1.0f);
    g_Renderer.DrawText("Мои видеозаписи   Альбомы   Новые   Популярные", startX + 16, startY + 12, COLOR_TEXT_PRIMARY, 1.1f, true);

    float cardY = startY + 50;
    for (size_t i = 0; i < m_Videos.size(); ++i) {
        const VideoItem& v = m_Videos[i];
        bool focused = (m_FocusArea == FocusArea::CONTENT && m_ContentIndex == static_cast<int>(i));

        g_Renderer.DrawRoundedRect(startX, cardY, w, 80, 5.0f, focused ? COLOR_SELECTION : COLOR_CARD,
            focused ? COLOR_SELECTION_BORDER : COLOR_CARD_BORDER, focused ? 1.5f : 1.0f);

        // Thumbnail placeholder with duration
        g_Renderer.DrawRoundedRect(startX + 14, cardY + 10, 100, 60, 4.0f, 0xFF24262A);
        g_Renderer.DrawBadge(v.duration, startX + 66, cardY + 48, 0xCC000000, 0xFFFFFFFF);

        g_Renderer.DrawText(v.title, startX + 126, cardY + 16, COLOR_TEXT_PRIMARY, 1.15f, true);
        g_Renderer.DrawText("Автор: " + v.author, startX + 126, cardY + 38, COLOR_TEXT_MUTED, 0.9f, false);

        char statBuf[64];
        snprintf(statBuf, sizeof(statBuf), "%d просмотров • ❤ %d", v.viewsCount, v.likesCount);
        g_Renderer.DrawText(statBuf, startX + 126, cardY + 54, COLOR_TEXT_MUTED, 0.85f, false);

        cardY += 88;
    }
}

// -------------------------------------------------------------------------------------------------
// TAB 8: MARKET & GAMES (Маркетплейс, Копилка, Рулетка)
// -------------------------------------------------------------------------------------------------
void UIManager::RenderMarketTab() {
    float startX = 250;
    float startY = 58;
    float w = 960;

    // Header & Sub-tabs
    g_Renderer.DrawRoundedRect(startX, startY, w, 44, 5.0f, COLOR_CARD, COLOR_CARD_BORDER, 1.0f);
    char coinHeader[64];
    snprintf(coinHeader, sizeof(coinHeader), "Маркет & Игры   [Баланс: %d монэток]", g_Config.coinsBalance);
    g_Renderer.DrawText(coinHeader, startX + 16, startY + 14, COLOR_COIN_GOLD, 1.15f, true);

    // Sub-mode pill buttons
    float pillY = startY + 52;
    const char* subModes[] = { "1. Юзернеймы", "2. Копилка (Кликер)", "3. Рулетка" };
    for (int i = 0; i < 3; ++i) {
        float px = startX + i * 180;
        bool active = (m_MarketSubTab == i);
        g_Renderer.DrawRoundedRect(px, pillY, 170, 32, 4.0f, active ? COLOR_ACCENT_BLUE : COLOR_CARD);
        g_Renderer.DrawText(subModes[i], px + 16, pillY + 8, active ? 0xFFFFFFFF : COLOR_TEXT_MUTED, 1.0f, false);
    }

    float contentY = pillY + 44;

    if (m_MarketSubTab == 1) { // Copilka Clicker Mini-game
        g_Renderer.DrawRoundedRect(startX, contentY, w, 280, 6.0f, COLOR_CARD, COLOR_CARD_BORDER, 1.0f);
        g_Renderer.DrawText("💰 Копилка huEVO", startX + 24, contentY + 20, COLOR_COIN_GOLD, 1.4f, true);
        g_Renderer.DrawText("Каждые 10 нажатий кнопки [A] приносят +1 монэтку на ваш счёт!", startX + 24, contentY + 54, COLOR_TEXT_PRIMARY, 1.1f, false);

        char clickBuf[64];
        snprintf(clickBuf, sizeof(clickBuf), "Сделано нажатий: %d   (До монэтки: %d)", m_CopilkaClicks, 10 - (m_CopilkaClicks % 10));
        g_Renderer.DrawText(clickBuf, startX + 24, contentY + 88, COLOR_TEXT_MUTED, 1.05f, false);

        // Huge Clicker Button
        g_Renderer.DrawRoundedRect(startX + 300, contentY + 140, 360, 80, 8.0f, 0xFF2A3D24, COLOR_XBOX_GREEN, 2.0f);
        g_Renderer.DrawText("НАЖМИТЕ [A] ДЛЯ КЛИКА", startX + 350, contentY + 170, 0xFFFFFFFF, 1.3f, true);
    } else { // Username Marketplace Table (sampled from screenshot)
        g_Renderer.DrawRoundedRect(startX, contentY, w, 32, 4.0f, 0xFF24262A);
        g_Renderer.DrawText("Юзернейм", startX + 20, contentY + 9, COLOR_TEXT_MUTED, 0.95f, false);
        g_Renderer.DrawText("Символов", startX + 180, contentY + 9, COLOR_TEXT_MUTED, 0.95f, false);
        g_Renderer.DrawText("Дата", startX + 320, contentY + 9, COLOR_TEXT_MUTED, 0.95f, false);
        g_Renderer.DrawText("Продавец", startX + 460, contentY + 9, COLOR_TEXT_MUTED, 0.95f, false);
        g_Renderer.DrawText("Цена (монэтки)", startX + 680, contentY + 9, COLOR_TEXT_MUTED, 0.95f, false);

        float rowY = contentY + 38;
        for (size_t i = 0; i < m_MarketItems.size() && i < 5; ++i) {
            const MarketUsername& item = m_MarketItems[i];
            bool focused = (m_FocusArea == FocusArea::CONTENT && m_ContentIndex == static_cast<int>(i));

            g_Renderer.DrawRoundedRect(startX, rowY, w, 44, 4.0f, focused ? COLOR_SELECTION : COLOR_CARD,
                focused ? COLOR_SELECTION_BORDER : COLOR_CARD_BORDER, focused ? 1.5f : 1.0f);

            g_Renderer.DrawText(item.username, startX + 20, rowY + 14, COLOR_ACCENT_BLUE, 1.1f, true);

            char symBuf[16];
            snprintf(symBuf, sizeof(symBuf), "%d симв.", item.charCount);
            g_Renderer.DrawText(symBuf, startX + 180, rowY + 14, COLOR_TEXT_MUTED, 0.95f, false);
            g_Renderer.DrawText(item.date, startX + 320, rowY + 14, COLOR_TEXT_MUTED, 0.95f, false);
            g_Renderer.DrawText(item.seller, startX + 460, rowY + 14, COLOR_TEXT_PRIMARY, 0.95f, false);

            char prBuf[32];
            snprintf(prBuf, sizeof(prBuf), "• %d м.", item.priceCoins);
            g_Renderer.DrawText(prBuf, startX + 680, rowY + 14, COLOR_COIN_GOLD, 1.05f, true);

            g_Renderer.DrawButtonPrompt("A", "Купить", startX + w - 120, rowY + 10, COLOR_XBOX_GREEN);

            rowY += 50;
        }
    }
}

// -------------------------------------------------------------------------------------------------
// TAB 9: SETTINGS (Настройки)
// -------------------------------------------------------------------------------------------------
void UIManager::RenderSettingsTab() {
    float startX = 250;
    float startY = 58;
    float w = 960;

    g_Renderer.DrawRoundedRect(startX, startY, w, 40, 5.0f, COLOR_CARD, COLOR_CARD_BORDER, 1.0f);
    g_Renderer.DrawText("Настройки аккаунта и сети", startX + 16, startY + 12, COLOR_TEXT_PRIMARY, 1.15f, true);

    float cardY = startY + 54;
    struct SettingRow { const char* title; const char* val; };
    SettingRow rows[] = {
        { "Адрес страницы:", "https://huevo.su/telega" },
        { "Имя на странице:", m_User.displayName.c_str() },
        { "Companion Bridge IP:", g_Config.bridgeHost.c_str() },
        { "Звук уведомлений:", "Включён" },
        { "Безопасность консоли:", "Freeboot / RGH Title Bypass" }
    };

    for (int i = 0; i < 5; ++i) {
        bool focused = (m_FocusArea == FocusArea::CONTENT && m_ContentIndex == i);
        g_Renderer.DrawRoundedRect(startX, cardY, w, 60, 4.0f, focused ? COLOR_SELECTION : COLOR_CARD,
            focused ? COLOR_SELECTION_BORDER : COLOR_CARD_BORDER, focused ? 1.5f : 1.0f);

        g_Renderer.DrawText(rows[i].title, startX + 20, cardY + 20, COLOR_TEXT_MUTED, 1.05f, false);
        g_Renderer.DrawText(rows[i].val, startX + 320, cardY + 20, COLOR_TEXT_PRIMARY, 1.05f, true);

        if (i == 2) {
            g_Renderer.DrawButtonPrompt("A", "Изменить IP", startX + w - 180, cardY + 16, COLOR_ACCENT_BLUE);
        }

        cardY += 68;
    }
}

} // namespace huEVO
