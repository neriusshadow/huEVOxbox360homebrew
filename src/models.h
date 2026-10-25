#pragma once

#include <string>
#include <vector>
#include <cstdio>

namespace huEVO {

template<typename T>
inline std::string IntToString(T val) {
    char buf[32];
    snprintf(buf, sizeof(buf), "%lld", static_cast<long long>(val));
    return std::string(buf);
}

enum class ViewTab {
    FEED = 0,
    PROFILE = 1,
    MESSAGES = 2,
    MUSIC = 3,
    SETTINGS = 4,
    COUNT = 5
};

struct User {
    std::string id;
    std::string username;
    std::string displayName;
    std::string avatarUrl;
    std::string statusText;
    int followersCount = 0;
    int friendsCount = 0;
    bool isOnline = false;
};

struct Post {
    std::string id;
    std::string authorId;
    std::string authorName;
    std::string authorAvatar;
    std::string timeAgo;
    std::string content;
    int likesCount = 0;
    int commentsCount = 0;
    bool isLiked = false;
    std::vector<std::string> attachments;
};

struct DirectMessage {
    std::string id;
    std::string senderId;
    std::string senderName;
    std::string text;
    std::string timestamp;
    bool isOutgoing = false;
};

struct Dialog {
    std::string peerId;
    std::string peerName;
    std::string lastMessage;
    std::string timestamp;
    int unreadCount = 0;
};

struct AudioTrack {
    std::string id;
    std::string title;
    std::string artist;
    std::string duration;
    std::string streamUrl;
    bool isPlaying = false;
};

} // namespace huEVO
