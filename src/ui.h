#pragma once

#include "models.h"
#include <string>
#include <vector>

namespace huEVO {

class UIManager {
public:
    UIManager();

    void Initialize();
    void Update();
    void Render();

    // Tab navigation
    void SetTab(ViewTab tab);
    ViewTab GetCurrentTab() const { return m_CurrentTab; }
    void NextTab();
    void PrevTab();

    // Notifications / Toasts
    void ShowToast(const std::string& message, int durationFrames = 180);

    // Data refresh triggers
    void RefreshCurrentView();

    // Actions
    void OnActionPress();
    void OnNewPostPrompt();
    void OnLikeToggle();

private:
    void RenderHeader();
    void RenderFooter();
    void RenderFeedView();
    void RenderProfileView();
    void RenderMessagesView();
    void RenderMusicView();
    void RenderSettingsView();
    void RenderToast();

    ViewTab m_CurrentTab;
    int m_SelectedIndex;
    int m_ScrollOffset;

    // View data
    std::vector<Post> m_Posts;
    User m_UserProfile;
    std::vector<Post> m_UserPosts;
    std::vector<Dialog> m_Dialogs;
    std::vector<AudioTrack> m_Tracks;
    int m_CurrentTrackIndex;
    bool m_IsMusicPlaying;

    // Toast state
    std::string m_ToastMessage;
    int m_ToastTimer;

    // Visualizer animation frame
    float m_VisualizerPhase;
};

extern UIManager g_UI;

} // namespace huEVO
