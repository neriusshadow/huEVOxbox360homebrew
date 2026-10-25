#pragma once

#include <string>
#include <vector>
#include <cstdio>
#include <cstdint>

namespace huEVO {

template<typename T>
inline std::string ToString(T val) {
    char buf[32];
    snprintf(buf, sizeof(buf), "%lld", static_cast<long long>(val));
    return std::string(buf);
}

enum class ViewTab {
    PROFILE = 0,       // Моя Страница
    FEED = 1,          // Новости
    MESSAGES = 2,      // Сообщения (диалоги и переписка)
    FRIENDS = 3,       // Друзья
    NOTIFICATIONS = 4, // Уведомления
    COMMUNITIES = 5,   // Сообщества
    MUSIC = 6,         // Звучок
    VIDEOS = 7,        // Видеозаписи
    MARKET = 8,        // Маркет и игры (Копилка, Рулетка)
    SETTINGS = 9,      // Настройки
    COUNT = 10
};

struct User {
    std::string id;
    std::string displayName;
    std::string handle;
    std::string extraHandles;
    std::string statusText;
    std::string city;
    std::string birthday;
    std::string workplace;
    std::string reputation;
    int friendsCount = 88;
    int followersCount = 1;
    int groupsCount = 21;
    int photosCount = 19;
    int audioCount = 85;
    int videoCount = 6;
    bool isOnline = true;
    uint32_t avatarColor = 0xFF5181B8;
};

struct GiftItem {
    std::string id;
    std::string title;
    std::string author;
    std::string remainingInfo;
    int number = 1;
    int total = 100;
};

struct Post {
    std::string id;
    std::string authorName;
    std::string authorHandle;
    std::string timeAgo;
    std::string content;
    int likesCount = 0;
    int commentsCount = 0;
    bool isLiked = false;
    uint32_t authorColor = 0xFF4A76A8;
};

struct DirectMessage {
    std::string id;
    std::string senderName;
    std::string text;
    std::string timestamp;
    bool isOutgoing = false;
};

struct Dialog {
    std::string id;
    std::string peerName;
    std::string lastSender;
    std::string lastMessage;
    std::string timestamp;
    bool isGroup = false;
    int membersCount = 0;
    int unreadCount = 0;
    bool isOnline = false;
    uint32_t peerColor = 0xFF4A76A8;
    std::vector<DirectMessage> messages;
};

struct Friend {
    std::string id;
    std::string displayName;
    std::string handle;
    std::string statusText;
    bool isOnline = false;
    bool isPending = false;
    uint32_t avatarColor = 0xFF3E6594;
};

struct NotificationItem {
    std::string id;
    std::string actorName;
    std::string actorHandle;
    std::string actionText;
    std::string previewText;
    std::string timeAgo;
    bool isUnread = true;
};

struct Community {
    std::string id;
    std::string name;
    std::string description;
    int membersCount = 0;
    int postsCount = 0;
    bool is18Plus = false;
    uint32_t avatarColor = 0xFF5B697A;
};

struct AudioTrack {
    std::string id;
    std::string title;
    std::string artist;
    std::string duration;
    bool isPlaying = false;
};

struct VideoItem {
    std::string id;
    std::string title;
    std::string author;
    std::string duration;
    int viewsCount = 0;
    int likesCount = 0;
    bool is18Plus = false;
};

struct MarketUsername {
    std::string username;
    int charCount = 0;
    std::string date;
    int priceCoins = 0;
    std::string seller;
    bool isAvailable = true;
};

} // namespace huEVO
