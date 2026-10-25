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
    DWORD timeout = 3000;
    setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, (const char*)&timeout, sizeof(timeout));
    setsockopt(sock, SOL_SOCKET, SO_SNDTIMEO, (const char*)&timeout, sizeof(timeout));
#else
    struct timeval tv;
    tv.tv_sec = 3;
    tv.tv_usec = 0;
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
        m_LastError = "Не удалось подключиться к " + host;
        return "";
    }

    std::string req = method + " " + path + " HTTP/1.1\r\n";
    req += "Host: " + host + "\r\n";
    req += "User-Agent: huEVO-Xbox360/1.0\r\n";
    req += "Connection: close\r\n";
    if (!g_Config.sessionCookie.empty()) {
        req += "Cookie: " + g_Config.sessionCookie + "\r\n";
    }
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
    char buf[2048];
    int bytes = 0;
    while ((bytes = recv(sock, buf, sizeof(buf) - 1, 0)) > 0) {
        buf[bytes] = '\0';
        resp.append(buf, bytes);
    }
    closesocket(sock);
    return resp;
}

void NetworkClient::PopulateMockData() {
    // 1. User Profile from screenshots
    m_CachedUser.id = "1";
    m_CachedUser.displayName = "нериусшадоу";
    m_CachedUser.handle = "@telega";
    m_CachedUser.extraHandles = "@swagcreator, @eblan_dev, @snoopdog";
    m_CachedUser.statusText = "чуитс детка";
    m_CachedUser.city = "SWAGINO";
    m_CachedUser.birthday = "14 января 2002 г. (24 года)";
    m_CachedUser.workplace = "huEVO SWAGdev (deathwave solutions)";
    m_CachedUser.reputation = "14";
    m_CachedUser.friendsCount = 88;
    m_CachedUser.followersCount = 1;
    m_CachedUser.groupsCount = 21;
    m_CachedUser.photosCount = 19;
    m_CachedUser.audioCount = 85;
    m_CachedUser.videoCount = 6;
    m_CachedUser.isOnline = true;
    m_CachedUser.avatarColor = 0xFF5181B8;

    // 2. Feed & Wall Posts
    Post p1;
    p1.id = "101";
    p1.authorName = "huEVO SWAGdev (deathwave solutions)";
    p1.authorHandle = "@swagdev";
    p1.timeAgo = "5 окт в 23:30";
    p1.content = "всему huEVO передаю большой мозольный приветик! huEVO themes 1.0 RELEASE: конфиги (.hvthm), обои, кастомные темы.";
    p1.likesCount = 19;
    p1.commentsCount = 2;
    p1.authorColor = 0xFF4A76A8;
    m_CachedFeed.push_back(p1);

    Post p2;
    p2.id = "102";
    p2.authorName = "если мир меня не слушает я его изменю";
    p2.authorHandle = "@selfhurm";
    p2.timeAgo = "сегодня в 17:09";
    p2.content = "отдам самой сочной чиксе фотки в коменты";
    p2.likesCount = 7;
    p2.commentsCount = 22;
    p2.authorColor = 0xFF7D4E88;
    m_CachedFeed.push_back(p2);

    Post p3;
    p3.id = "103";
    p3.authorName = "нериусшадоу";
    p3.authorHandle = "@telega";
    p3.timeAgo = "вчера в 14:15";
    p3.content = "Тестирую нативный клиент huEVO прямо на Xbox 360!";
    p3.likesCount = 42;
    p3.commentsCount = 5;
    p3.authorColor = 0xFF5181B8;
    m_CachedFeed.push_back(p3);
    m_CachedUserPosts.push_back(p3);

    // 3. Dialogs & Messages
    Dialog d1;
    d1.id = "chat_1";
    d1.peerName = "хуева.чАТ";
    d1.lastSender = "aloegarten";
    d1.lastMessage = "Аудиозапись";
    d1.timestamp = "14:25";
    d1.isGroup = true;
    d1.membersCount = 322;
    d1.unreadCount = 99;
    d1.peerColor = 0xFF6D5494;

    DirectMessage m1_1 = { "1", "aloegarten", "кто в доту го", "14:20", false };
    DirectMessage m1_2 = { "2", "canor", "я занят кодом", "14:22", false };
    DirectMessage m1_3 = { "3", "нериусшадоу", "собираю билд на Xbox 360", "14:24", true };
    DirectMessage m1_4 = { "4", "aloegarten", "Аудиозапись", "14:25", false };
    d1.messages.push_back(m1_1);
    d1.messages.push_back(m1_2);
    d1.messages.push_back(m1_3);
    d1.messages.push_back(m1_4);
    m_CachedDialogs.push_back(d1);

    Dialog d2;
    d2.id = "chat_2";
    d2.peerName = "Император";
    d2.lastSender = "john";
    d2.lastMessage = "ЫХЪАХЪЫАЫА";
    d2.timestamp = "17:58";
    d2.isGroup = true;
    d2.membersCount = 424;
    d2.unreadCount = 99;
    d2.peerColor = 0xFF8A5A36;
    DirectMessage m2_1 = { "1", "john", "ЫХЪАХЪЫАЫА", "17:58", false };
    d2.messages.push_back(m2_1);
    m_CachedDialogs.push_back(d2);

    Dialog d3;
    d3.id = "chat_3";
    d3.peerName = "влад сораши";
    d3.lastSender = "влад";
    d3.lastMessage = "sorashi double eight";
    d3.timestamp = "19.10";
    d3.isGroup = false;
    d3.unreadCount = 4;
    d3.isOnline = true;
    d3.peerColor = 0xFF3D6E8C;
    DirectMessage m3_1 = { "1", "влад", "привет, че делаешь?", "19:00", false };
    DirectMessage m3_2 = { "2", "нериусшадоу", "пилю хоумбрю клиент", "19:05", true };
    DirectMessage m3_3 = { "3", "влад", "sorashi double eight", "19:10", false };
    d3.messages.push_back(m3_1);
    d3.messages.push_back(m3_2);
    d3.messages.push_back(m3_3);
    m_CachedDialogs.push_back(d3);

    Dialog d4;
    d4.id = "chat_4";
    d4.peerName = "САПОГ";
    d4.lastSender = "canor";
    d4.lastMessage = "X3";
    d4.timestamp = "02:15";
    d4.isGroup = false;
    d4.isOnline = true;
    d4.peerColor = 0xFF587A4A;
    DirectMessage m4_1 = { "1", "canor", "X3", "02:15", false };
    d4.messages.push_back(m4_1);
    m_CachedDialogs.push_back(d4);

    // 4. Friends
    Friend f1 = { "1", "влад сораши", "@selfhurm", "sorashi double eight", true, false, 0xFF3D6E8C };
    Friend f2 = { "2", "САПОГ", "@canor", "САПОГ, VFX", true, false, 0xFF587A4A };
    Friend f3 = { "3", "screampunk", "@screampunk", "ноунеймы из саундклауда", true, false, 0xFF884D72 };
    Friend f4 = { "4", "rane4 /// ranewh", "@rane4", "online", true, false, 0xFF4A76A8 };
    Friend f5 = { "5", "wwiper", "@wwiper", "был(-а) 4 дня назад", false, true, 0xFF666666 };
    m_CachedFriends.push_back(f1);
    m_CachedFriends.push_back(f2);
    m_CachedFriends.push_back(f3);
    m_CachedFriends.push_back(f4);
    m_CachedFriends.push_back(f5);

    // 5. Notifications
    NotificationItem n1 = { "1", "PuppyLover184", "@puppylover184", "оценил(а) ваш пост", "", "5 мин назад", true };
    NotificationItem n2 = { "2", "МЦ Леонидик", "@mcleonidick", "опубликовал(а) новую запись:", "«что лучше: начать читать рэп...»", "1 час назад", true };
    NotificationItem n3 = { "3", "майнкрафт", "@minecraft", "принял(а) вашу заявку в друзья", "", "3 часа назад", false };
    m_CachedNotifications.push_back(n1);
    m_CachedNotifications.push_back(n2);
    m_CachedNotifications.push_back(n3);

    // 6. Communities
    Community c1 = { "1", "VETMO 18+", "хайп джугарики teomova сватают бабуль", 7, 8, true, 0xFF7D4E88 };
    Community c2 = { "2", "семьсот семь", "музыкальное объединение", 142, 65, false, 0xFF4A76A8 };
    Community c3 = { "3", "huEVO SWAGdev", "официальная разработка huEVO", 4120, 290, false, 0xFF5181B8 };
    m_CachedCommunities.push_back(c1);
    m_CachedCommunities.push_back(c2);
    m_CachedCommunities.push_back(c3);

    // 7. Audio Tracks
    AudioTrack t1 = { "1", "clout", "MPT", "2:41", false };
    AudioTrack t2 = { "2", "double eight", "sorashi", "1:58", false };
    AudioTrack t3 = { "3", "JUUL DROP", "САПОГ", "3:12", false };
    AudioTrack t4 = { "4", "deathwave", "SWAGdev", "2:20", false };
    m_CachedTracks.push_back(t1);
    m_CachedTracks.push_back(t2);
    m_CachedTracks.push_back(t3);
    m_CachedTracks.push_back(t4);

    // 8. Video Items
    VideoItem v1 = { "1", "обзор на курилку", "plan a", "0:06", 12, 1, false };
    VideoItem v2 = { "2", "vibecode$$$$$", "только я", "0:15", 9, 2, false };
    VideoItem v3 = { "3", "КОДЕРСКАЯ ЯМА", "swagdev", "1:05", 14, 4, false };
    m_CachedVideos.push_back(v1);
    m_CachedVideos.push_back(v2);
    m_CachedVideos.push_back(v3);

    // 9. Marketplace
    MarketUsername u1 = { "@19cm", 4, "20.10.26", 90000, "Матье Балл", true };
    MarketUsername u2 = { "@18cm", 4, "20.10.26", 80000, "Матье Балл", true };
    MarketUsername u3 = { "@16cm", 4, "20.10.26", 60000, "Матье Балл", true };
    MarketUsername u4 = { "@mp3music", 8, "20.10.26", 33333, "4икуся", true };
    MarketUsername u5 = { "@xoy", 3, "20.10.26", 444444, "4икуся", true };
    m_CachedMarket.push_back(u1);
    m_CachedMarket.push_back(u2);
    m_CachedMarket.push_back(u3);
    m_CachedMarket.push_back(u4);
    m_CachedMarket.push_back(u5);
}

bool NetworkClient::FetchFeed(std::vector<Post>& outPosts) {
    outPosts = m_CachedFeed;
    return true;
}

bool NetworkClient::FetchProfile(const std::string& userId, User& outUser, std::vector<Post>& outUserPosts) {
    outUser = m_CachedUser;
    outUserPosts = m_CachedUserPosts;
    return true;
}

bool NetworkClient::FetchDialogs(std::vector<Dialog>& outDialogs) {
    outDialogs = m_CachedDialogs;
    return true;
}

bool NetworkClient::FetchMessages(const std::string& dialogId, std::vector<DirectMessage>& outMessages) {
    for (size_t i = 0; i < m_CachedDialogs.size(); ++i) {
        if (m_CachedDialogs[i].id == dialogId) {
            outMessages = m_CachedDialogs[i].messages;
            return true;
        }
    }
    return false;
}

bool NetworkClient::FetchFriends(std::vector<Friend>& outFriends) {
    outFriends = m_CachedFriends;
    return true;
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
    outTracks = m_CachedTracks;
    return true;
}

bool NetworkClient::FetchVideos(std::vector<VideoItem>& outVideos) {
    outVideos = m_CachedVideos;
    return true;
}

bool NetworkClient::FetchMarket(std::vector<MarketUsername>& outItems) {
    outItems = m_CachedMarket;
    return true;
}

bool NetworkClient::CreatePost(const std::string& text, Post& outCreatedPost) {
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
    return false;
}

bool NetworkClient::ToggleLike(const std::string& postId, bool& outLiked, int& outCount) {
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
