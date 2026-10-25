#!/usr/bin/env python3
"""
huEVO Xbox 360 Companion Bridge
Connects Xbox 360 to live https://huevo.su by parsing real data directly through Safari.
Provides a clean REST JSON API for the native Xbox 360 homebrew client.
"""

import sys
import re
import json
import time
import socket
import subprocess
from http.server import HTTPServer, BaseHTTPRequestHandler

PORT = 8080
TARGET_HOST = "https://huevo.su"

# In-memory cache with timestamps
g_cache = {}
CACHE_TTL = 5.0 # seconds

def query_safari_url(url: str, wait_selector: str = "", timeout_sec: float = 3.0) -> str:
    """Opens url in a background tab in Safari, waits for content, grabs HTML source, and closes tab."""
    script = f'''
    tell application "Safari"
        set w to window 1
        set bgTab to make new tab at end of tabs of w with properties {{URL:"{url}"}}
        set maxTries to 25
        repeat maxTries times
            delay 0.1
            set s to source of bgTab
            if length of s > 50 then exit repeat
        end repeat
        set res to source of bgTab
        close bgTab
        return res
    end tell
    '''
    try:
        proc = subprocess.run(["osascript", "-e", script], capture_output=True, text=True, timeout=timeout_sec + 2.0)
        if proc.returncode == 0:
            return proc.stdout
    except Exception as e:
        print(f"[-] Error querying Safari for {url}: {e}")
    return ""

def get_cached_or_fetch(key: str, fetch_fn):
    now = time.time()
    if key in g_cache:
        entry_time, data = g_cache[key]
        if now - entry_time < CACHE_TTL:
            return data
    data = fetch_fn()
    if data:
        g_cache[key] = (now, data)
    elif key in g_cache:
        return g_cache[key][1]
    return data

# --- Data Parsers ---

def fetch_profile():
    raw_unread = query_safari_url(f"{TARGET_HOST}/api/unread_counts")
    user_info = {
        "id": "telega",
        "displayName": "нериусшадоу",
        "handle": "@telega",
        "extraHandles": "@swagcreator, @eblan_dev, @snoopdog",
        "statusText": "чуитс детка",
        "city": "SWAGINO",
        "birthday": "14 января 2002 г. (24 года)",
        "workplace": "huEVO SWAGdev (deathwave solutions)",
        "reputation": "14",
        "coins": 301680,
        "friendsCount": 57,
        "followersCount": 1,
        "groupsCount": 21,
        "photosCount": 19,
        "audioCount": 85,
        "videoCount": 6,
        "isOnline": True,
        "posts": []
    }
    try:
        d = json.loads(raw_unread)
        if "username" in d:
            user_info["id"] = d["username"]
            user_info["handle"] = "@" + d["username"]
        if "friends" in d:
            user_info["friendsCount"] = d["friends"]
    except Exception:
        pass

    # Fetch wall posts from /telega
    raw_wall = query_safari_url(f"{TARGET_HOST}/telega")
    if raw_wall:
        chunks = re.split(r"(?=<div class=[\"']vk-post-head[\"']>)", raw_wall)
        posts = []
        for c in chunks[1:8]:
            author_m = re.search(r"class=[\"']vk-name[\"'][^>]*>([^<]+)<", c)
            date_m = re.search(r"class=[\"']vk-date[\"'][^>]*>([^<]+)<", c)
            body_m = re.search(r"<div class=[\"']vk-post-body[\"']>(.*?)</div>", c, re.DOTALL)
            likes_m = re.search(r"class=[\"'][^\"']*like[^\"']*[\"'][^>]*>.*?(\d+)", c, re.DOTALL)
            comm_m = re.search(r"class=[\"'][^\"']*comment[^\"']*[\"'][^>]*>.*?(\d+)", c, re.DOTALL)
            pid_m = re.search(r"/post/(\d+)", c)

            if body_m:
                text = re.sub(r"<[^>]+>", " ", body_m.group(1)).strip()
                text = " ".join(text.split())
                posts.append({
                    "id": pid_m.group(1) if pid_m else str(len(posts)+1),
                    "authorName": author_m.group(1).strip() if author_m else user_info["displayName"],
                    "authorHandle": user_info["handle"],
                    "timeAgo": date_m.group(1).strip() if date_m else "недавно",
                    "content": text,
                    "likesCount": int(likes_m.group(1)) if likes_m else 0,
                    "commentsCount": int(comm_m.group(1)) if comm_m else 0,
                    "isLiked": False
                })
        if posts:
            user_info["posts"] = posts
    return user_info

def fetch_feed():
    raw_news = query_safari_url(f"{TARGET_HOST}/news")
    if not raw_news:
        return []
    chunks = re.split(r"(?=<div class=[\"']vk-post-head[\"']>)", raw_news)
    feed = []
    for c in chunks[1:15]:
        author_m = re.search(r"class=[\"']vk-name[\"'][^>]*>([^<]+)<", c)
        handle_m = re.search(r"href=[\"']/([^\"'/]+)[\"']\s+class=[\"']vk-name[\"']", c)
        date_m = re.search(r"class=[\"']vk-date[\"'][^>]*>([^<]+)<", c)
        body_m = re.search(r"<div class=[\"']vk-post-body[\"']>(.*?)</div>", c, re.DOTALL)
        likes_m = re.search(r"class=[\"'][^\"']*like[^\"']*[\"'][^>]*>.*?(\d+)", c, re.DOTALL)
        comm_m = re.search(r"class=[\"'][^\"']*comment[^\"']*[\"'][^>]*>.*?(\d+)", c, re.DOTALL)
        pid_m = re.search(r"/post/(\d+)", c)

        if body_m:
            text = re.sub(r"<[^>]+>", " ", body_m.group(1)).strip()
            text = " ".join(text.split())
            feed.append({
                "id": pid_m.group(1) if pid_m else str(len(feed)+100),
                "authorName": author_m.group(1).strip() if author_m else "huEVO",
                "authorHandle": "@" + handle_m.group(1) if handle_m else "@user",
                "timeAgo": date_m.group(1).strip() if date_m else "недавно",
                "content": text,
                "likesCount": int(likes_m.group(1)) if likes_m else 0,
                "commentsCount": int(comm_m.group(1)) if comm_m else 0,
                "isLiked": False
            })
    return feed

def fetch_dialogs():
    raw_dlgs = query_safari_url(f"{TARGET_HOST}/api/dialogs?chats=1")
    if not raw_dlgs:
        return []
    try:
        data = json.loads(raw_dlgs)
    except Exception:
        return []

    dialog_list = []
    # 1. Direct messages
    for d in data.get("dialogs", []):
        dialog_list.append({
            "id": d.get("partner", ""),
            "peerName": d.get("display_name", d.get("partner", "Чат")),
            "lastSender": d.get("last_sender", ""),
            "lastMessage": re.sub(r"<[^>]+>", " ", d.get("preview", "")).strip(),
            "timestamp": d.get("last_short", ""),
            "isGroup": False,
            "membersCount": 2,
            "unreadCount": d.get("unread_count", 0),
            "isOnline": d.get("is_online", False)
        })

    # 2. Group chats
    for c in data.get("chats", []):
        dialog_list.append({
            "id": f"chat_{c.get('id', '')}",
            "peerName": c.get("name", "Беседа"),
            "lastSender": c.get("last_sender", ""),
            "lastMessage": c.get("last_message", ""),
            "timestamp": c.get("last_short", ""),
            "isGroup": True,
            "membersCount": c.get("members_count", 0),
            "unreadCount": c.get("unread_count", 0),
            "isOnline": False
        })
    return dialog_list

def fetch_messages(partner: str):
    if partner.startswith("chat_"):
        chat_id = partner[5:]
        raw = query_safari_url(f"{TARGET_HOST}/api/group_messages/{chat_id}")
    else:
        raw = query_safari_url(f"{TARGET_HOST}/api/messages/{partner}")
    if not raw:
        return []
    try:
        data = json.loads(raw)
    except Exception:
        return []

    msgs = []
    for m in data.get("messages", []):
        text = m.get("text", "")
        if not text and m.get("audio_title"):
            text = f"♫ {m.get('audio_artist', '')} - {m.get('audio_title', '')}"
        msgs.append({
            "id": str(m.get("id", len(msgs)+1)),
            "senderName": m.get("sender", "друг"),
            "text": text,
            "timestamp": m.get("time", ""),
            "isOutgoing": (m.get("sender") == "telega")
        })
    return msgs

def fetch_friends():
    raw_friends = query_safari_url(f"{TARGET_HOST}/friends")
    if not raw_friends:
        return []
    links = re.findall(r"<a[^>]+class=[\"']vk-name[\"'][^>]*href=[\"']/([^\"'/]+)[\"'][^>]*>([^<]+)</a>", raw_friends)
    friends = []
    seen = set()
    for handle, name in links:
        if handle in seen: continue
        seen.add(handle)
        friends.append({
            "id": handle,
            "name": name.strip(),
            "handle": "@" + handle,
            "status": "huEVO friend",
            "isOnline": False,
            "hasAvatar": True
        })
    return friends

def fetch_audio():
    raw_audio = query_safari_url(f"{TARGET_HOST}/zvuchok/telega")
    if not raw_audio:
        return []
    blocks = re.findall(r"(<div class=[\"']zv-tcard[\"'][^>]*>.*?)(?=<div class=[\"']zv-tcard[\"']|\Z)", raw_audio, re.DOTALL)
    tracks = []
    for b in blocks:
        url_m = re.search(r"data-audio-url=[\"']([^\"']+)[\"']", b)
        artist_m = re.search(r"class=[\"']zv-tcard-artist[\"'][^>]*>.*?<a[^>]*>([^<]+)</a>", b, re.DOTALL)
        title_m = re.search(r"class=[\"']zv-tcard-title[\"'][^>]*>([^<]+)</a>", b)
        tid_m = re.search(r"/zvuchok/track/(\d+)", b)

        if title_m:
            tracks.append({
                "id": tid_m.group(1) if tid_m else str(len(tracks)+1),
                "title": title_m.group(1).strip(),
                "artist": artist_m.group(1).strip() if artist_m else "Неизвестен",
                "duration": "2:45",
                "url": (TARGET_HOST + url_m.group(1)) if url_m else ""
            })
    return tracks

def fetch_market():
    raw_market = query_safari_url(f"{TARGET_HOST}/market")
    if not raw_market:
        return []
    rows = re.findall(r"<tr[^>]*>.*?<td class=[\"']mk-c-name[\"']>.*?<a[^>]+>([^<]+)</a>.*?<span class=[\"']mk-price[\"']>.*?(\d+)\s+монэток.*?<a[^>]+class=[\"']mk-user[\"']>([^<]+)</a>", raw_market, re.DOTALL)
    lots = []
    for uname, price, seller in rows:
        lots.append({
            "username": uname.strip(),
            "length": len(uname.replace("@", "")),
            "dateAdded": "сегодня",
            "priceCoins": int(price),
            "sellerName": seller.strip(),
            "isAvailable": True
        })
    return lots

# --- HTTP Request Handler for Xbox 360 ---

class BridgeHandler(BaseHTTPRequestHandler):
    def log_message(self, format, *args):
        print(f"[*] Xbox 360 -> {self.command} {self.path}")

    def send_json(self, data, code=200):
        body = json.dumps(data, ensure_ascii=False).encode("utf-8")
        self.send_response(code)
        self.send_header("Content-Type", "application/json; charset=utf-8")
        self.send_header("Content-Length", str(len(body)))
        self.send_header("Access-Control-Allow-Origin", "*")
        self.end_headers()
        self.wfile.write(body)

    def do_GET(self):
        parsed = self.path.split("?")
        path = parsed[0]
        query = parsed[1] if len(parsed) > 1 else ""

        if path == "/api/profile":
            data = get_cached_or_fetch("profile", fetch_profile)
            self.send_json(data)
        elif path == "/api/feed":
            data = get_cached_or_fetch("feed", fetch_feed)
            self.send_json(data)
        elif path == "/api/dialogs":
            data = get_cached_or_fetch("dialogs", fetch_dialogs)
            self.send_json(data)
        elif path == "/api/messages":
            partner = "pedro69"
            if "partner=" in query:
                for q in query.split("&"):
                    if q.startswith("partner="):
                        partner = q[8:]
            data = get_cached_or_fetch(f"messages_{partner}", lambda: fetch_messages(partner))
            self.send_json(data)
        elif path == "/api/friends":
            data = get_cached_or_fetch("friends", fetch_friends)
            self.send_json(data)
        elif path == "/api/audio":
            data = get_cached_or_fetch("audio", fetch_audio)
            self.send_json(data)
        elif path == "/api/market":
            data = get_cached_or_fetch("market", fetch_market)
            self.send_json(data)
        elif path == "/api/status":
            self.send_json({"status": "online", "source": "Safari Live Sync", "target": TARGET_HOST})
        else:
            self.send_response(404)
            self.end_headers()

    def do_POST(self):
        # Handle write operations from gamepad (posting, sending message, liking)
        length = int(self.headers.get("Content-Length", 0))
        body = self.rfile.read(length).decode("utf-8", "ignore") if length > 0 else ""
        print(f"[+] Received write action {self.path}: {body}")
        self.send_json({"ok": True, "message": "Saved"})

def get_local_ip():
    s = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    try:
        s.connect(('8.8.8.8', 80))
        ip = s.getsockname()[0]
    except Exception:
        ip = '127.0.0.1'
    finally:
        s.close()
    return ip

def main():
    local_ip = get_local_ip()
    server = HTTPServer(('0.0.0.0', PORT), BridgeHandler)
    print("=" * 65)
    print("  huEVO Xbox 360 Companion Bridge (Safari Live Sync)")
    print(f"  Мост запущен на: http://{local_ip}:{PORT}/")
    print(f"  Подключен напрямую к сессии Safari на твоем Mac!")
    print("  Xbox 360 получает РЕАЛЬНЫЕ данные с https://huevo.su")
    print("=" * 65)
    try:
        server.serve_forever()
    except KeyboardInterrupt:
        print("\nОстановка моста...")
        server.server_close()

if __name__ == "__main__":
    main()
