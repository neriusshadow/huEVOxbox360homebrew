#pragma once

#include "models.h"
#include <string>
#include <vector>

namespace huEVO {

enum class FocusArea {
    SIDEBAR,
    CONTENT
};

class UIManager {
public:
    UIManager();

    void Initialize();
    void Update();
    void Render();

    void SetTab(ViewTab tab);
    void NextTab();
    void PrevTab();

    void ShowToast(const std::string& message, int durationFrames = 120);

private:
    void HandleNavigation();
    void HandleActions();

    // Screen Renderers
    void RenderHeader();
    void RenderSidebar();
    void RenderBottomBar();
    void RenderToast();

    void RenderProfileTab();
    void RenderFeedTab();
    void RenderMessagesTab();
    void RenderFriendsTab();
    void RenderNotificationsTab();
    void RenderCommunitiesTab();
    void RenderMusicTab();
    void RenderVideosTab();
    void RenderMarketTab();
    void RenderSettingsTab();

    // Sub-renderers
    void RenderChatWindow();

    ViewTab m_CurrentTab;
    FocusArea m_FocusArea;
    int m_SidebarIndex;
    int m_ContentIndex;
    int m_ScrollOffset;

    // Messenger active chat state
    bool m_InChatMode;
    std::string m_ActiveDialogId;
    std::string m_ActiveDialogTitle;
    std::vector<DirectMessage> m_ActiveMessages;
    int m_ChatScrollOffset;

    // Market / Games sub-tab
    int m_MarketSubTab; // 0: Usernames, 1: Copilka, 2: Roulette
    int m_CopilkaClicks;

    // Music playback state
    int m_CurrentTrackIndex;
    bool m_IsPlaying;
    float m_TrackProgress;
    float m_VisualizerPhase;

    // Toast
    std::string m_ToastMessage;
    int m_ToastTimer;

    // Cached views
    User m_User;
    std::vector<Post> m_FeedPosts;
    std::vector<Post> m_UserPosts;
    std::vector<Dialog> m_Dialogs;
    std::vector<Friend> m_Friends;
    std::vector<NotificationItem> m_Notifications;
    std::vector<Community> m_Communities;
    std::vector<AudioTrack> m_Tracks;
    std::vector<VideoItem> m_Videos;
    std::vector<MarketUsername> m_MarketItems;
};

extern UIManager g_UI;

} // namespace huEVO
