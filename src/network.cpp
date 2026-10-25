#include "network.h"
#include "config.h"
#include <cstdio>
#include <cstring>
#include <algorithm>

#ifdef _XBOX
#include <xtl.h>
#include <winsockx.h>
#else
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <netdb.h>
#define closesocket close
#define INVALID_SOCKET -1
#define SOCKET_ERROR -1
typedef int SOCKET;
#endif

namespace huEVO {

NetworkClient g_Network;

// --- Lightweight JSON Parsing Helpers ---

static std::string GetHttpBody(const std::string& httpResp) {
    size_t pos = httpResp.find("\r\n\r\n");
    if (pos != std::string::npos) {
        return httpResp.substr(pos + 4);
    }
    pos = httpResp.find("\n\n");
    if (pos != std::string::npos) {
        return httpResp.substr(pos + 2);
    }
    return httpResp;
}

static std::string ExtractJsonString(const std::string& json, const std::string& key) {
    std::string needle = "\"" + key + "\":";
    size_t pos = json.find(needle);
    if (pos == std::string::npos) {
        needle = "\"" + key + "\" :";
        pos = json.find(needle);
        if (pos == std::string::npos) return "";
    }
    pos += needle.length();
    while (pos < json.length() && (json[pos] == ' ' || json[pos] == '\t')) pos++;
    if (pos < json.length() && json[pos] == '\"') {
        pos++;
        size_t endPos = pos;
        while (endPos < json.length()) {
            if (json[endPos] == '\"' && json[endPos - 1] != '\\') break;
            endPos++;
        }
        std::string val = json.substr(pos, endPos - pos);
        std::string res;
        for (size_t i = 0; i < val.length(); ++i) {
            if (val[i] == '\\' && i + 1 < val.length()) {
                if (val[i+1] == '\"') { res += '\"'; i++; }
                else if (val[i+1] == 'n') { res += '\n'; i++; }
                else if (val[i+1] == 't') { res += '\t'; i++; }
                else if (val[i+1] == '\\') { res += '\\'; i++; }
                else { res += val[i]; }
            } else {
                res += val[i];
            }
        }
        return res;
    }
    return "";
}

static int ExtractJsonInt(const std::string& json, const std::string& key, int defaultVal = 0) {
    std::string needle = "\"" + key + "\":";
    size_t pos = json.find(needle);
    if (pos == std::string::npos) {
        needle = "\"" + key + "\" :";
        pos = json.find(needle);
        if (pos == std::string::npos) return defaultVal;
    }
    pos += needle.length();
    while (pos < json.length() && (json[pos] == ' ' || json[pos] == '\t')) pos++;
    int sign = 1;
    if (pos < json.length() && json[pos] == '-') { sign = -1; pos++; }
    int res = 0;
    bool found = false;
    while (pos < json.length() && json[pos] >= '0' && json[pos] <= '9') {
        res = res * 10 + (json[pos] - '0');
        found = true;
        pos++;
    }
    return found ? (res * sign) : defaultVal;
}

static bool ExtractJsonBool(const std::string& json, const std::string& key, bool defaultVal = false) {
    std::string needle = "\"" + key + "\":";
    size_t pos = json.find(needle);
    if (pos == std::string::npos) return defaultVal;
    pos += needle.length();
    while (pos < json.length() && (json[pos] == ' ' || json[pos] == '\t')) pos++;
    if (pos + 4 <= json.length() && json.substr(pos, 4) == "true") return true;
    if (pos + 5 <= json.length() && json.substr(pos, 5) == "false") return false;
    return defaultVal;
}

static std::vector<std::string> ExtractJsonObjects(const std::string& json) {
    std::vector<std::string> objs;
    int depth = 0;
    size_t start = 0;
    bool inString = false;

    for (size_t i = 0; i < json.length(); ++i) {
        char c = json[i];
        if (c == '\"' && (i == 0 || json[i-1] != '\\')) {
            inString = !inString;
        } else if (!inString) {
            if (c == '{') {
                if (depth == 0) start = i;
                depth++;
            } else if (c == '}') {
                depth--;
                if (depth == 0 && start <= i) {
                    objs.push_back(json.substr(start, i - start + 1));
                }
            }
        }
    }
    return objs;
}

NetworkClient::NetworkClient()
    : m_Connected(false)
{
}

NetworkClient::~NetworkClient() {
    Shutdown();
}

bool NetworkClient::Initialize() {
#ifdef _XBOX
    XNetStartupParams xnsp;
    std::memset(&xnsp, 0, sizeof(xnsp));
    xnsp.cfgSizeOfStruct = sizeof(XNetStartupParams);
    xnsp.cfgFlags = XNET_STARTUP_BYPASS_SECURITY;
    INT xnetErr = XNetStartup(&xnsp);

    WSADATA wsaData;
    INT wsaErr = WSAStartup(MAKEWORD(2, 2), &wsaData);
    m_Connected = (wsaErr == 0);
#else
    m_Connected = true;
#endif

    PopulateMockData();
    return m_Connected;
}

void NetworkClient::Shutdown() {
#ifdef _XBOX
    WSACleanup();
    XNetCleanup();
#endif
    m_Connected = false;
}

std::string NetworkClient::HttpRequest(const std::string& method, const std::string& path, const std::string& body) {
    std::string host = g_Config.bridgeHost;
    int port = g_Config.bridgePort;

    SOCKET sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (sock == INVALID_SOCKET) {
        m_LastError = "Не удалось открыть сокет";
        return "";
    }

#ifdef _XBOX
    DWORD timeout = 2500;
    setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, (const char*)&timeout, sizeof(timeout));
    setsockopt(sock, SOL_SOCKET, SO_SNDTIMEO, (const char*)&timeout, sizeof(timeout));
#else
    struct timeval tv;
    tv.tv_sec = 2;
    tv.tv_usec = 500000;
    setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, (const char*)&tv, sizeof(tv));
    setsockopt(sock, SOL_SOCKET, SO_SNDTIMEO, (const char*)&tv, sizeof(tv));
#endif

    sockaddr_in serverAddr;
    std::memset(&serverAddr, 0, sizeof(serverAddr));
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(static_cast<uint16_t>(port));
    serverAddr.sin_addr.s_addr = inet_addr(host.c_str());

    if (connect(sock, (sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
        closesocket(sock);
        m_LastError = "Не удалось подключиться к мосту " + host;
        return "";
    }

    std::string req = method + " " + path + " HTTP/1.1\r\n";
    req += "Host: " + host + "\r\n";
    req += "User-Agent: huEVO-Xbox360/1.0\r\n";
    req += "Connection: close\r\n";
    if (!body.empty()) {
        char lenBuf[32];
        snprintf(lenBuf, sizeof(lenBuf), "%zu", body.length());
        req += "Content-Type: application/x-www-form-urlencoded\r\n";
        req += "Content-Length: ";
        req += lenBuf;
        req += "\r\n";
    }
    req += "\r\n";
    if (!body.empty()) {
        req += body;
    }

    send(sock, req.c_str(), static_cast<int>(req.length()), 0);

    std::string resp;
    char buf[4096];
    int bytes = 0;
    while ((bytes = recv(sock, buf, sizeof(buf) - 1, 0)) > 0) {
        buf[bytes] = '\0';
        resp.append(buf, bytes);
    }
    closesocket(sock);
    return resp;
}

void NetworkClient::PopulateMockData() {
    m_CachedUser.id = "telega";
    m_CachedUser.displayName = "нериусшадоу";
    m_CachedUser.handle = "@telega";
    m_CachedUser.extraHandles = "@swagcreator, @eblan_dev, @snoopdog";
    m_CachedUser.statusText = "чуитс детка";
    m_CachedUser.city = "SWAGINO";
    m_CachedUser.birthday = "14 января 2002 г. (24 года)";
    m_CachedUser.workplace = "huEVO SWAGdev (deathwave solutions)";
    m_CachedUser.reputation = "14";
    m_CachedUser.friendsCount = 57;
    m_CachedUser.followersCount = 1;
    m_CachedUser.groupsCount = 21;
    m_CachedUser.photosCount = 19;
    m_CachedUser.audioCount = 85;
    m_CachedUser.videoCount = 6;
    m_CachedUser.isOnline = true;
    m_CachedUser.avatarColor = 0xFF5181B8;

    Post p;
    p.id = "1";
    p.authorName = "нериусшадоу";
    p.authorHandle = "@telega";
    p.timeAgo = "сегодня в 20:01";
    p.content = "нериусшадоу замечен за употреблением американского киэфси";
    p.likesCount = 10;
    p.commentsCount = 2;
    p.authorColor = 0xFF5181B8;
    m_CachedFeed.push_back(p);
    m_CachedUserPosts.push_back(p);

    Dialog d;
    d.id = "pedro69";
    d.peerName = "PEDRO";
    d.lastSender = "pedro69";
    d.lastMessage = "зайди на стрим";
    d.timestamp = "сегодня";
    d.isGroup = false;
    d.unreadCount = 1;
    d.peerColor = 0xFF6D5494;
    m_CachedDialogs.push_back(d);

    Friend f;
    f.id = "wwiper";
    f.displayName = "wwiper";
    f.handle = "@wwiper";
    f.statusText = "онлайн";
    f.isOnline = true;
    f.isPending = false;
    f.avatarColor = 0xFF3D6E8C;
    m_CachedFriends.push_back(f);
}

bool NetworkClient::FetchProfile(const std::string& userId, User& outUser, std::vector<Post>& outUserPosts) {
    std::string resp = HttpRequest("GET", "/api/profile");
    std::string body = GetHttpBody(resp);
    if (!body.empty() && body[0] == '{') {
        outUser.id = ExtractJsonString(body, "id");
        outUser.displayName = ExtractJsonString(body, "displayName");
        outUser.handle = ExtractJsonString(body, "handle");
        outUser.extraHandles = ExtractJsonString(body, "extraHandles");
        outUser.statusText = ExtractJsonString(body, "statusText");
        outUser.city = ExtractJsonString(body, "city");
        outUser.birthday = ExtractJsonString(body, "birthday");
        outUser.workplace = ExtractJsonString(body, "workplace");
        outUser.reputation = ExtractJsonString(body, "reputation");
        outUser.friendsCount = ExtractJsonInt(body, "friendsCount", 57);
        outUser.followersCount = ExtractJsonInt(body, "followersCount", 1);
        outUser.groupsCount = ExtractJsonInt(body, "groupsCount", 21);
        outUser.photosCount = ExtractJsonInt(body, "photosCount", 19);
        outUser.audioCount = ExtractJsonInt(body, "audioCount", 85);
        outUser.videoCount = ExtractJsonInt(body, "videoCount", 6);
        outUser.isOnline = ExtractJsonBool(body, "isOnline", true);
        outUser.avatarColor = 0xFF5181B8;

        int coins = ExtractJsonInt(body, "coins", 301680);
        g_Config.coinsBalance = coins;

        size_t ppos = body.find("\"posts\":");
        if (ppos != std::string::npos) {
            std::vector<std::string> postObjs = ExtractJsonObjects(body.substr(ppos));
            outUserPosts.clear();
            for (size_t i = 0; i < postObjs.size(); ++i) {
                Post p;
                p.id = ExtractJsonString(postObjs[i], "id");
                p.authorName = ExtractJsonString(postObjs[i], "authorName");
                p.authorHandle = ExtractJsonString(postObjs[i], "authorHandle");
                p.timeAgo = ExtractJsonString(postObjs[i], "timeAgo");
                p.content = ExtractJsonString(postObjs[i], "content");
                p.likesCount = ExtractJsonInt(postObjs[i], "likesCount");
                p.commentsCount = ExtractJsonInt(postObjs[i], "commentsCount");
                p.isLiked = ExtractJsonBool(postObjs[i], "isLiked");
                p.authorColor = 0xFF5181B8;
                outUserPosts.push_back(p);
            }
        }
        m_CachedUser = outUser;
        m_CachedUserPosts = outUserPosts;
        return true;
    }
    outUser = m_CachedUser;
    outUserPosts = m_CachedUserPosts;
    return false;
}

bool NetworkClient::FetchFeed(std::vector<Post>& outPosts) {
    std::string resp = HttpRequest("GET", "/api/feed");
    std::string body = GetHttpBody(resp);
    if (!body.empty() && body[0] == '[') {
        std::vector<std::string> items = ExtractJsonObjects(body);
        if (!items.empty()) {
            outPosts.clear();
            for (size_t i = 0; i < items.size(); ++i) {
                Post p;
                p.id = ExtractJsonString(items[i], "id");
                p.authorName = ExtractJsonString(items[i], "authorName");
                p.authorHandle = ExtractJsonString(items[i], "authorHandle");
                p.timeAgo = ExtractJsonString(items[i], "timeAgo");
                p.content = ExtractJsonString(items[i], "content");
                p.likesCount = ExtractJsonInt(items[i], "likesCount");
                p.commentsCount = ExtractJsonInt(items[i], "commentsCount");
                p.isLiked = ExtractJsonBool(items[i], "isLiked");
                p.authorColor = 0xFF5181B8;
                outPosts.push_back(p);
            }
            m_CachedFeed = outPosts;
            return true;
        }
    }
    outPosts = m_CachedFeed;
    return false;
}

bool NetworkClient::FetchDialogs(std::vector<Dialog>& outDialogs) {
    std::string resp = HttpRequest("GET", "/api/dialogs");
    std::string body = GetHttpBody(resp);
    if (!body.empty() && body[0] == '[') {
        std::vector<std::string> items = ExtractJsonObjects(body);
        if (!items.empty()) {
            outDialogs.clear();
            for (size_t i = 0; i < items.size(); ++i) {
                Dialog d;
                d.id = ExtractJsonString(items[i], "id");
                d.peerName = ExtractJsonString(items[i], "peerName");
                d.lastSender = ExtractJsonString(items[i], "lastSender");
                d.lastMessage = ExtractJsonString(items[i], "lastMessage");
                d.timestamp = ExtractJsonString(items[i], "timestamp");
                d.isGroup = ExtractJsonBool(items[i], "isGroup");
                d.membersCount = ExtractJsonInt(items[i], "membersCount", 2);
                d.unreadCount = ExtractJsonInt(items[i], "unreadCount");
                d.isOnline = ExtractJsonBool(items[i], "isOnline");
                d.peerColor = d.isGroup ? 0xFF6D5494 : 0xFF3D6E8C;
                outDialogs.push_back(d);
            }
            m_CachedDialogs = outDialogs;
            return true;
        }
    }
    outDialogs = m_CachedDialogs;
    return false;
}

bool NetworkClient::FetchMessages(const std::string& dialogId, std::vector<DirectMessage>& outMessages) {
    std::string resp = HttpRequest("GET", "/api/messages?partner=" + dialogId);
    std::string body = GetHttpBody(resp);
    if (!body.empty() && body[0] == '[') {
        std::vector<std::string> items = ExtractJsonObjects(body);
        if (!items.empty()) {
            outMessages.clear();
            for (size_t i = 0; i < items.size(); ++i) {
                DirectMessage m;
                m.id = ExtractJsonString(items[i], "id");
                m.senderName = ExtractJsonString(items[i], "senderName");
                m.text = ExtractJsonString(items[i], "text");
                m.timestamp = ExtractJsonString(items[i], "timestamp");
                m.isOutgoing = ExtractJsonBool(items[i], "isOutgoing");
                outMessages.push_back(m);
            }
            return true;
        }
    }
    return false;
}

bool NetworkClient::FetchFriends(std::vector<Friend>& outFriends) {
    std::string resp = HttpRequest("GET", "/api/friends");
    std::string body = GetHttpBody(resp);
    if (!body.empty() && body[0] == '[') {
        std::vector<std::string> items = ExtractJsonObjects(body);
        if (!items.empty()) {
            outFriends.clear();
            for (size_t i = 0; i < items.size(); ++i) {
                Friend f;
                f.id = ExtractJsonString(items[i], "id");
                f.displayName = ExtractJsonString(items[i], "name");
                f.handle = ExtractJsonString(items[i], "handle");
                f.statusText = ExtractJsonString(items[i], "status");
                f.isOnline = ExtractJsonBool(items[i], "isOnline");
                f.isPending = false;
                f.avatarColor = 0xFF3D6E8C + (i * 0x051020);
                outFriends.push_back(f);
            }
            m_CachedFriends = outFriends;
            return true;
        }
    }
    outFriends = m_CachedFriends;
    return false;
}

bool NetworkClient::FetchNotifications(std::vector<NotificationItem>& outNotifications) {
    outNotifications = m_CachedNotifications;
    return true;
}

bool NetworkClient::FetchCommunities(std::vector<Community>& outCommunities) {
    outCommunities = m_CachedCommunities;
    return true;
}

bool NetworkClient::FetchAudioTracks(std::vector<AudioTrack>& outTracks) {
    std::string resp = HttpRequest("GET", "/api/audio");
    std::string body = GetHttpBody(resp);
    if (!body.empty() && body[0] == '[') {
        std::vector<std::string> items = ExtractJsonObjects(body);
        if (!items.empty()) {
            outTracks.clear();
            for (size_t i = 0; i < items.size(); ++i) {
                AudioTrack t;
                t.id = ExtractJsonString(items[i], "id");
                t.title = ExtractJsonString(items[i], "title");
                t.artist = ExtractJsonString(items[i], "artist");
                t.duration = ExtractJsonString(items[i], "duration");
                t.isPlaying = false;
                outTracks.push_back(t);
            }
            m_CachedTracks = outTracks;
            return true;
        }
    }
    outTracks = m_CachedTracks;
    return false;
}

bool NetworkClient::FetchVideos(std::vector<VideoItem>& outVideos) {
    outVideos = m_CachedVideos;
    return true;
}

bool NetworkClient::FetchMarket(std::vector<MarketUsername>& outItems) {
    std::string resp = HttpRequest("GET", "/api/market");
    std::string body = GetHttpBody(resp);
    if (!body.empty() && body[0] == '[') {
        std::vector<std::string> items = ExtractJsonObjects(body);
        if (!items.empty()) {
            outItems.clear();
            for (size_t i = 0; i < items.size(); ++i) {
                MarketUsername m;
                m.username = ExtractJsonString(items[i], "username");
                m.charCount = ExtractJsonInt(items[i], "length");
                m.date = ExtractJsonString(items[i], "dateAdded");
                m.priceCoins = ExtractJsonInt(items[i], "priceCoins");
                m.seller = ExtractJsonString(items[i], "sellerName");
                m.isAvailable = ExtractJsonBool(items[i], "isAvailable", true);
                outItems.push_back(m);
            }
            m_CachedMarket = outItems;
            return true;
        }
    }
    outItems = m_CachedMarket;
    return false;
}

bool NetworkClient::CreatePost(const std::string& text, Post& outCreatedPost) {
    HttpRequest("POST", "/api/post", "text=" + text);

    outCreatedPost.id = ToString(m_CachedFeed.size() + 100);
    outCreatedPost.authorName = m_CachedUser.displayName;
    outCreatedPost.authorHandle = m_CachedUser.handle;
    outCreatedPost.timeAgo = "только что";
    outCreatedPost.content = text;
    outCreatedPost.likesCount = 0;
    outCreatedPost.commentsCount = 0;
    outCreatedPost.isLiked = false;
    outCreatedPost.authorColor = m_CachedUser.avatarColor;

    m_CachedFeed.insert(m_CachedFeed.begin(), outCreatedPost);
    m_CachedUserPosts.insert(m_CachedUserPosts.begin(), outCreatedPost);
    return true;
}

bool NetworkClient::SendDirectMessage(const std::string& dialogId, const std::string& text, DirectMessage& outMsg) {
    HttpRequest("POST", "/api/send_message", "dialog=" + dialogId + "&text=" + text);

    outMsg.id = ToString(100);
    outMsg.senderName = m_CachedUser.displayName;
    outMsg.text = text;
    outMsg.timestamp = "сейчас";
    outMsg.isOutgoing = true;

    for (size_t i = 0; i < m_CachedDialogs.size(); ++i) {
        if (m_CachedDialogs[i].id == dialogId) {
            m_CachedDialogs[i].messages.push_back(outMsg);
            m_CachedDialogs[i].lastMessage = text;
            m_CachedDialogs[i].lastSender = m_CachedUser.displayName;
            m_CachedDialogs[i].timestamp = "сейчас";
            return true;
        }
    }
    return true;
}

bool NetworkClient::ToggleLike(const std::string& postId, bool& outLiked, int& outCount) {
    HttpRequest("POST", "/api/like", "id=" + postId);

    for (size_t i = 0; i < m_CachedFeed.size(); ++i) {
        if (m_CachedFeed[i].id == postId) {
            m_CachedFeed[i].isLiked = !m_CachedFeed[i].isLiked;
            if (m_CachedFeed[i].isLiked) m_CachedFeed[i].likesCount++;
            else m_CachedFeed[i].likesCount--;
            outLiked = m_CachedFeed[i].isLiked;
            outCount = m_CachedFeed[i].likesCount;
            return true;
        }
    }
    return false;
}

} // namespace huEVO
