#pragma once

#include "models.h"
#include "tls_client.h"
#include <string>
#include <vector>

namespace huEVO {

class NetworkClient {
public:
    NetworkClient();
    ~NetworkClient();

    bool Initialize();
    void Shutdown();

    // Authentication (Directly on huevo.su from Xbox 360)
    bool Login(const std::string& username, const std::string& password, std::string& outError);
    bool Logout();
    bool IsLoggedIn() const;

    // Social Network Data Actions
    bool FetchFeed(std::vector<Post>& outPosts);
    bool FetchProfile(const std::string& userId, User& outUser, std::vector<Post>& outUserPosts);
    bool FetchDialogs(std::vector<Dialog>& outDialogs);
    bool FetchMessages(const std::string& peerId, std::vector<DirectMessage>& outMessages);
    bool FetchAudioTracks(std::vector<AudioTrack>& outTracks);

    // Write actions directly to huevo.su
    bool CreatePost(const std::string& text, Post& outCreatedPost);
    bool SendDirectMessage(const std::string& peerId, const std::string& text);
    bool ToggleLike(const std::string& postId, bool& outLiked, int& outCount);

    // Status
    bool IsConnected() const { return m_Initialized; }
    std::string GetLastError() const { return m_LastError; }

private:
    std::string ExecuteRequest(const std::string& method, const std::string& path, const std::string& body = "");
    void ParseHtmlFeed(const std::string& html, std::vector<Post>& outPosts);

    TlsClient m_TlsClient;
    bool m_Initialized;
    std::string m_LastError;
    std::string m_HchkToken;
};

extern NetworkClient g_Network;

} // namespace huEVO
