#pragma once

#include "models.h"
#include <string>
#include <vector>

namespace huEVO {

class NetworkClient {
public:
    NetworkClient();
    ~NetworkClient();

    bool Initialize();
    void Shutdown();

    // Data queries
    bool FetchFeed(std::vector<Post>& outPosts);
    bool FetchProfile(const std::string& userId, User& outUser, std::vector<Post>& outUserPosts);
    bool FetchDialogs(std::vector<Dialog>& outDialogs);
    bool FetchMessages(const std::string& dialogId, std::vector<DirectMessage>& outMessages);
    bool FetchFriends(std::vector<Friend>& outFriends);
    bool FetchNotifications(std::vector<NotificationItem>& outNotifications);
    bool FetchCommunities(std::vector<Community>& outCommunities);
    bool FetchAudioTracks(std::vector<AudioTrack>& outTracks);
    bool FetchVideos(std::vector<VideoItem>& outVideos);
    bool FetchMarket(std::vector<MarketUsername>& outItems);

    // Write actions
    bool CreatePost(const std::string& text, Post& outCreatedPost);
    bool SendDirectMessage(const std::string& dialogId, const std::string& text, DirectMessage& outMsg);
    bool ToggleLike(const std::string& postId, bool& outLiked, int& outCount);

    bool IsConnected() const { return m_Connected; }
    std::string GetLastError() const { return m_LastError; }

private:
    std::string HttpRequest(const std::string& method, const std::string& path, const std::string& body = "");
    void PopulateMockData();

    bool m_Connected;
    std::string m_LastError;

    // Cache mirroring screenshots for instant responsiveness
    User m_CachedUser;
    std::vector<Post> m_CachedFeed;
    std::vector<Post> m_CachedUserPosts;
    std::vector<Dialog> m_CachedDialogs;
    std::vector<Friend> m_CachedFriends;
    std::vector<NotificationItem> m_CachedNotifications;
    std::vector<Community> m_CachedCommunities;
    std::vector<AudioTrack> m_CachedTracks;
    std::vector<VideoItem> m_CachedVideos;
    std::vector<MarketUsername> m_CachedMarket;
};

extern NetworkClient g_Network;

} // namespace huEVO
